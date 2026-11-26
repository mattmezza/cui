//! Optional retained Radio and RadioGroup presentation.
use super::{check, App, Builder, Error, Kind, Node, RawNode, Result};
use std::ffi::c_void;
type Set = unsafe extern "C" fn(*mut c_void, RawNode, u32) -> u32;

#[derive(Clone, Copy)]
enum Setting {
    Motion,
    ItemContent,
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub(super) enum LegacyRadioNode {
    Standalone,
    Group,
    Item,
}

#[cfg(cui_bridge_static)]
fn setter(setting: Setting) -> Option<Set> {
    extern "C" {
        fn cui_bridge_radio_set_motion_v1(a: *mut c_void, n: RawNode, enabled: u32) -> u32;
        fn cui_bridge_radio_item_set_content_v1(a: *mut c_void, n: RawNode, enabled: u32) -> u32;
    }
    Some(match setting {
        Setting::Motion => cui_bridge_radio_set_motion_v1,
        Setting::ItemContent => cui_bridge_radio_item_set_content_v1,
    })
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn setter(setting: Setting) -> Option<Set> {
    let name = match setting {
        Setting::Motion => b"cui_bridge_radio_set_motion_v1\0".as_slice(),
        Setting::ItemContent => b"cui_bridge_radio_item_set_content_v1\0".as_slice(),
    };
    unsafe {
        let p = super::notification_symbol_v1(name.as_ptr().cast());
        if p.is_null() {
            None
        } else {
            Some(std::mem::transmute::<*mut c_void, Set>(p))
        }
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn setter(_: Setting) -> Option<Set> {
    None
}
impl App {
    /// Whether this provider offers retained Radio and RadioGroup motion.
    pub fn supports_radio_motion(&self) -> bool {
        setter(Setting::Motion).is_some()
    }
    /// Whether this provider can paint passive children inside a direct RadioGroup Item.
    pub fn supports_radio_item_content(&self) -> bool {
        setter(Setting::ItemContent).is_some()
    }
    pub(super) fn track_legacy_radio_node(&mut self, raw: RawNode, parent: RawNode, kind: Kind) {
        let legacy = match kind {
            Kind::Radio if !self.supports_radio_motion() => LegacyRadioNode::Standalone,
            Kind::RadioGroup
                if !self.supports_radio_motion() || !self.supports_radio_item_content() =>
            {
                LegacyRadioNode::Group
            }
            Kind::Item
                if self.legacy_radio_nodes.get(&parent) == Some(&LegacyRadioNode::Group)
                    && !self.supports_radio_item_content() =>
            {
                LegacyRadioNode::Item
            }
            _ => return,
        };
        self.legacy_radio_nodes.insert(raw, legacy);
    }
    fn legacy_radio_node(&self, raw: RawNode) -> Result<Option<LegacyRadioNode>> {
        let mut required = 0;
        let status = unsafe {
            super::cui_bridge_get_text(self.raw, raw, std::ptr::null_mut(), 0, &mut required)
        };
        if status != 0 && status != 5 {
            return Err(Error(status));
        }
        Ok(self.legacy_radio_nodes.get(&raw).copied())
    }
    /// Opt into retained mark/fill animation. False preserves immediate presentation
    /// even with an older provider; unsupported opt-in returns Error(9).
    /// Only Radio and RadioGroup nodes are accepted. Events keep their existing timing.
    pub fn animate_selection(&mut self, node: Node, enabled: bool) -> Result<&mut Self> {
        let raw = self.valid(node)?;
        if let Some(set) = setter(Setting::Motion) {
            check(unsafe { set(self.raw, raw, enabled as u32) })?;
        } else {
            if !matches!(
                self.legacy_radio_node(raw)?,
                Some(LegacyRadioNode::Group | LegacyRadioNode::Standalone)
            ) {
                return Err(Error(8));
            }
            if enabled {
                return Err(Error(9));
            }
        }
        Ok(self)
    }

    /// Show or hide a direct RadioGroup Item's passive children. Earlier providers
    /// keep them hidden and return Error(9) when opt-in is requested.
    pub fn decorate_content(&mut self, item: Node, enabled: bool) -> Result<&mut Self> {
        let raw = self.valid(item)?;
        if let Some(set) = setter(Setting::ItemContent) {
            check(unsafe { set(self.raw, raw, enabled as u32) })?;
        } else {
            if self.legacy_radio_node(raw)? != Some(LegacyRadioNode::Item) {
                return Err(Error(8));
            }
            if enabled {
                return Err(Error(9));
            }
        }
        Ok(self)
    }
}
impl<'a> Builder<'a> {
    pub fn animate_selection(self, enabled: bool) -> Result<Self> {
        self.app.animate_selection(self.node, enabled)?;
        Ok(self)
    }

    pub fn decorate_content(self, enabled: bool) -> Result<Self> {
        self.app.decorate_content(self.node, enabled)?;
        Ok(self)
    }
}
