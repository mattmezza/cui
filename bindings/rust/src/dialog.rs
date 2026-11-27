use crate::*;
use std::mem;
use std::sync::OnceLock;

/// Copied configuration for a composed Dialog or AlertDialog.
/// Zero width, height and margin use 420 logical units, automatic height
/// and 16 logical units. `show_close` and `outside_dismiss` apply to AlertDialog.
#[derive(Clone, Copy, Debug, Default)]
pub struct DialogOptions {
    pub width: f32,
    pub height: f32,
    pub margin: f32,
    pub centered: bool,
    pub no_escape_dismiss: bool,
    pub no_outside_dismiss: bool,
    pub hide_close: bool,
    pub show_close: bool,
    pub outside_dismiss: bool,
}

/// One of the independent child regions in a composed dialog.
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DialogSlot {
    Header = 1,
    Title,
    Description,
    Content,
    Body,
    Footer,
}

/// Owner result produced by a direct Footer Button.
#[repr(u32)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum DialogAction {
    #[default]
    None,
    Confirm,
    Cancel,
    Close,
}

#[repr(C)]
struct RawOptions {
    flags: u32,
    reserved: u32,
    width: f32,
    height: f32,
    margin: f32,
}

impl DialogOptions {
    fn raw(self) -> Result<RawOptions> {
        for value in [self.width, self.height, self.margin] {
            if !(0.0..=4096.0).contains(&value) {
                return Err(Error(3));
            }
        }
        let mut flags = 0;
        for (bit, enabled) in [
            self.centered,
            self.no_escape_dismiss,
            self.no_outside_dismiss,
            self.hide_close,
            self.show_close,
            self.outside_dismiss,
        ]
        .into_iter()
        .enumerate()
        {
            if enabled {
                flags |= 1 << bit;
            }
        }
        Ok(RawOptions {
            flags,
            reserved: 0,
            width: self.width,
            height: self.height,
            margin: self.margin,
        })
    }
}

type Configure = unsafe extern "C" fn(*mut c_void, RawNode, u32, *const RawOptions) -> u32;
type CreateSlot =
    unsafe extern "C" fn(*mut c_void, RawNode, u32, *const c_char, *mut RawNode) -> u32;
type SetAction = unsafe extern "C" fn(*mut c_void, RawNode, u32) -> u32;

#[derive(Clone, Copy)]
struct DialogApi {
    configure: Configure,
    create_slot: CreateSlot,
    set_action: SetAction,
}

fn api() -> Option<DialogApi> {
    static API: OnceLock<Option<DialogApi>> = OnceLock::new();
    *API.get_or_init(resolve)
}

#[cfg(cui_bridge_static)]
fn resolve() -> Option<DialogApi> {
    extern "C" {
        fn cui_bridge_dialog_configure_v1(
            app: *mut c_void,
            node: RawNode,
            size: u32,
            options: *const RawOptions,
        ) -> u32;
        fn cui_bridge_dialog_slot_create_v1(
            app: *mut c_void,
            node: RawNode,
            role: u32,
            key: *const c_char,
            out: *mut RawNode,
        ) -> u32;
        fn cui_bridge_dialog_set_action_v1(app: *mut c_void, button: RawNode, action: u32) -> u32;
    }
    Some(DialogApi {
        configure: cui_bridge_dialog_configure_v1,
        create_slot: cui_bridge_dialog_slot_create_v1,
        set_action: cui_bridge_dialog_set_action_v1,
    })
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve() -> Option<DialogApi> {
    unsafe {
        let configure = notification_symbol_v1(c"cui_bridge_dialog_configure_v1".as_ptr());
        let create_slot = notification_symbol_v1(c"cui_bridge_dialog_slot_create_v1".as_ptr());
        let set_action = notification_symbol_v1(c"cui_bridge_dialog_set_action_v1".as_ptr());
        if configure.is_null() || create_slot.is_null() || set_action.is_null() {
            return None;
        }
        Some(DialogApi {
            configure: mem::transmute::<*mut c_void, Configure>(configure),
            create_slot: mem::transmute::<*mut c_void, CreateSlot>(create_slot),
            set_action: mem::transmute::<*mut c_void, SetAction>(set_action),
        })
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> Option<DialogApi> {
    None
}

impl Error {
    /// The loaded provider lacks retained dialog composition.
    pub const DIALOG_COMPOSITION_UNAVAILABLE: Self = Self(9);
}

impl App {
    /// Whether the provider supports configuration, named slots and typed actions.
    pub fn supports_dialog_composition(&self) -> bool {
        api().is_some()
    }

    /// Configure a Dialog or AlertDialog. `None` restores legacy presentation
    /// after the dialog is closed and all named slots have been removed.
    pub fn dialog_options(
        &mut self,
        node: Node,
        options: Option<DialogOptions>,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let raw = options.map(DialogOptions::raw).transpose()?;
        let api = api().ok_or(Error::DIALOG_COMPOSITION_UNAVAILABLE)?;
        let size = raw
            .as_ref()
            .map_or(0, |_| mem::size_of::<RawOptions>() as u32);
        let options = raw.as_ref().map_or(std::ptr::null(), |r| r);
        check(unsafe { (api.configure)(self.raw, node, size, options) })?;
        Ok(self)
    }

    /// Add one named slot to a configured Dialog or AlertDialog.
    pub fn create_dialog_slot(&mut self, node: Node, role: DialogSlot, key: &str) -> Result<Node> {
        let node = self.valid(node)?;
        let api = api().ok_or(Error::DIALOG_COMPOSITION_UNAVAILABLE)?;
        let key = string(key)?;
        let mut raw = RawNode::default();
        check(unsafe { (api.create_slot)(self.raw, node, role as u32, key.as_ptr(), &mut raw) })?;
        Ok(Node {
            raw,
            app_id: self.id,
        })
    }

    /// Add a named slot and borrow the app for fluent container properties.
    pub fn build_dialog_slot(
        &mut self,
        node: Node,
        role: DialogSlot,
        key: &str,
    ) -> Result<Builder<'_>> {
        let node = self.create_dialog_slot(node, role, key)?;
        Ok(Builder { app: self, node })
    }

    /// Link a direct Footer Button to a typed close result on the owning dialog.
    pub fn dialog_action(&mut self, button: Node, action: DialogAction) -> Result<&mut Self> {
        let button = self.valid(button)?;
        let api = api().ok_or(Error::DIALOG_COMPOSITION_UNAVAILABLE)?;
        check(unsafe { (api.set_action)(self.raw, button, action as u32) })?;
        Ok(self)
    }
}

impl<'a> Builder<'a> {
    /// Apply copied Dialog or AlertDialog layout options.
    pub fn dialog_options(self, options: Option<DialogOptions>) -> Result<Self> {
        self.app.dialog_options(self.node, options)?;
        Ok(self)
    }

    /// Give a direct Footer Button a typed close result.
    pub fn dialog_action(self, action: DialogAction) -> Result<Self> {
        self.app.dialog_action(self.node, action)?;
        Ok(self)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn wire_layout_and_validation() {
        assert_eq!(mem::size_of::<RawOptions>(), 20);
        assert_eq!(mem::offset_of!(RawOptions, margin), 16);
        let options = DialogOptions {
            centered: true,
            outside_dismiss: true,
            ..Default::default()
        };
        assert_eq!(options.raw().unwrap().flags, 33);
        assert!(DialogOptions {
            width: f32::NAN,
            ..options
        }
        .raw()
        .is_err());
        assert!(DialogOptions {
            height: -1.,
            ..options
        }
        .raw()
        .is_err());
    }
}
