//! Optional copied presentation for retained Pagination. Page changes use CHANGE.
use super::{check, App, Builder, DesignSize, Error, Kind, Node, RawNode, Result};
use std::{ffi::c_void, mem::size_of, sync::OnceLock};

/// Navigation presentation. Labels are single-line and at most 127 UTF-8 bytes.
/// Empty labels use the native defaults.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct PaginationConfig {
    pub counter: bool,
    pub icon_only: bool,
    pub chevrons: bool,
    pub jump_menu: bool,
    /// Zero selects the default window; otherwise 5 through 32.
    pub visible_pages: u32,
    /// Inner container width for wrapping, or zero for one row.
    pub available_width: f32,
    pub group: String,
    pub first: String,
    pub previous: String,
    pub next: String,
    pub last: String,
    pub earlier_pages: String,
    pub later_pages: String,
}

const SIZE: u32 = 916;
const V1: u32 = 1;
const COUNTER: u32 = 1;
const ICON_ONLY: u32 = 2;
const CHEVRONS: u32 = 4;
const JUMP_MENU: u32 = 8;
const FLAGS: u32 = COUNTER | ICON_ONLY | CHEVRONS | JUMP_MENU;
#[repr(C)]
struct ConfigRaw {
    size: u32,
    version: u32,
    flags: u32,
    visible_pages: u32,
    available_width: f32,
    labels: [[u8; 128]; 7],
}
const _: [(); SIZE as usize] = [(); size_of::<ConfigRaw>()];

impl PaginationConfig {
    /// Validate before touching a retained tree or requiring a provider.
    pub fn validate(&self) -> Result<()> {
        if (self.visible_pages != 0 && !(5..=32).contains(&self.visible_pages))
            || !(0.0..=32768.0).contains(&self.available_width)
            || self.labels().into_iter().any(|label| {
                label.len() > 127
                    || label.chars().any(|c| {
                        c <= '\u{1f}' || matches!(c, '\u{7f}' | '\u{85}' | '\u{2028}' | '\u{2029}')
                    })
            })
        {
            return Err(Error(3));
        }
        Ok(())
    }

    fn labels(&self) -> [&str; 7] {
        [
            &self.group,
            &self.first,
            &self.previous,
            &self.next,
            &self.last,
            &self.earlier_pages,
            &self.later_pages,
        ]
    }

    fn to_raw(&self) -> Result<ConfigRaw> {
        self.validate()?;
        let mut raw = ConfigRaw {
            size: SIZE,
            version: V1,
            flags: (u32::from(self.counter) * COUNTER)
                | (u32::from(self.icon_only) * ICON_ONLY)
                | (u32::from(self.chevrons) * CHEVRONS)
                | (u32::from(self.jump_menu) * JUMP_MENU),
            visible_pages: self.visible_pages,
            available_width: self.available_width,
            labels: [[0; 128]; 7],
        };
        for (out, label) in raw.labels.iter_mut().zip(self.labels()) {
            out[..label.len()].copy_from_slice(label.as_bytes());
        }
        Ok(raw)
    }

    fn from_raw(raw: ConfigRaw) -> Result<Self> {
        if raw.size != SIZE || raw.version != V1 || raw.flags & !FLAGS != 0 {
            return Err(Error(8));
        }
        let [group, first, previous, next, last, earlier_pages, later_pages] = &raw.labels;
        let config = Self {
            counter: raw.flags & COUNTER != 0,
            icon_only: raw.flags & ICON_ONLY != 0,
            chevrons: raw.flags & CHEVRONS != 0,
            jump_menu: raw.flags & JUMP_MENU != 0,
            visible_pages: raw.visible_pages,
            available_width: raw.available_width,
            group: decode_label(group)?,
            first: decode_label(first)?,
            previous: decode_label(previous)?,
            next: decode_label(next)?,
            last: decode_label(last)?,
            earlier_pages: decode_label(earlier_pages)?,
            later_pages: decode_label(later_pages)?,
        };
        config.validate().map_err(|_| Error(8))?;
        Ok(config)
    }
}

fn decode_label(label: &[u8; 128]) -> Result<String> {
    let end = label.iter().position(|b| *b == 0).ok_or(Error(8))?;
    std::str::from_utf8(&label[..end])
        .map(str::to_owned)
        .map_err(|_| Error(8))
}

type Query = unsafe extern "C" fn() -> u32;
type Configure = unsafe extern "C" fn(*mut c_void, RawNode, *const ConfigRaw) -> u32;
type Get = unsafe extern "C" fn(*mut c_void, RawNode, *mut ConfigRaw) -> u32;
struct Api {
    configure: Configure,
    get: Get,
}
fn api() -> Option<&'static Api> {
    static API: OnceLock<Option<Api>> = OnceLock::new();
    API.get_or_init(resolve).as_ref()
}
#[cfg(any(cui_bridge_static, unix, windows))]
fn resolve() -> Option<Api> {
    #[cfg(cui_bridge_static)]
    extern "C" {
        fn cui_bridge_pagination_versions() -> u32;
        fn cui_bridge_pagination_config_size_v1() -> u32;
        fn cui_bridge_pagination_configure_v1(
            a: *mut c_void,
            n: RawNode,
            c: *const ConfigRaw,
        ) -> u32;
        fn cui_bridge_pagination_get_config_v1(
            a: *mut c_void,
            n: RawNode,
            c: *mut ConfigRaw,
        ) -> u32;
    }
    #[cfg(cui_bridge_static)]
    macro_rules! symbol {
        ($name:ident, $ty:ty) => {
            $name as $ty
        };
    }
    #[cfg(not(cui_bridge_static))]
    macro_rules! symbol {
        ($name:ident, $ty:ty) => {{
            let name = concat!(stringify!($name), "\0");
            let pointer = unsafe { super::notification_symbol_v1(name.as_ptr().cast()) };
            if pointer.is_null() {
                return None;
            }
            unsafe { std::mem::transmute::<*mut c_void, $ty>(pointer) }
        }};
    }
    let versions = symbol!(cui_bridge_pagination_versions, Query);
    let size = symbol!(cui_bridge_pagination_config_size_v1, Query);
    let api = Api {
        configure: symbol!(cui_bridge_pagination_configure_v1, Configure),
        get: symbol!(cui_bridge_pagination_get_config_v1, Get),
    };
    if unsafe { versions() & V1 != 0 && size() == SIZE } {
        Some(api)
    } else {
        None
    }
}
#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> Option<Api> {
    None
}

type SetCustom = unsafe extern "C" fn(*mut c_void, RawNode, f32) -> u32;
type GetCustom = unsafe extern "C" fn(*mut c_void, RawNode, *mut f32) -> u32;
struct CustomApi {
    set: SetCustom,
    get: GetCustom,
}
fn custom_api() -> Option<&'static CustomApi> {
    static API: OnceLock<Option<CustomApi>> = OnceLock::new();
    API.get_or_init(resolve_custom).as_ref()
}
#[cfg(cui_bridge_static)]
fn resolve_custom() -> Option<CustomApi> {
    extern "C" {
        fn cui_bridge_pagination_set_custom_size(a: *mut c_void, n: RawNode, size: f32) -> u32;
        fn cui_bridge_pagination_get_custom_size(a: *mut c_void, n: RawNode, size: *mut f32)
            -> u32;
    }
    Some(CustomApi {
        set: cui_bridge_pagination_set_custom_size,
        get: cui_bridge_pagination_get_custom_size,
    })
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve_custom() -> Option<CustomApi> {
    let set = unsafe {
        super::notification_symbol_v1(b"cui_bridge_pagination_set_custom_size\0".as_ptr().cast())
    };
    let get = unsafe {
        super::notification_symbol_v1(b"cui_bridge_pagination_get_custom_size\0".as_ptr().cast())
    };
    if set.is_null() || get.is_null() {
        return None;
    }
    Some(CustomApi {
        set: unsafe { std::mem::transmute::<*mut c_void, SetCustom>(set) },
        get: unsafe { std::mem::transmute::<*mut c_void, GetCustom>(get) },
    })
}
#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve_custom() -> Option<CustomApi> {
    None
}
fn validate_custom(size: f32) -> Result<()> {
    if size == 0. || (1.0..=512.0).contains(&size) {
        Ok(())
    } else {
        Err(Error(3))
    }
}

/// Detached presentation builder. Mount preflights optional capabilities before creation.
#[derive(Clone, Debug, Default)]
pub struct PaginationOptions {
    config: Option<PaginationConfig>,
    design_size: Option<DesignSize>,
    custom_size: f32,
}
impl PaginationOptions {
    /// Set a logical-pixel height: zero restores named sizing; otherwise 1–512.
    pub fn custom_size(mut self, size: f32) -> Result<Self> {
        validate_custom(size)?;
        self.custom_size = size;
        Ok(self)
    }
    /// Store a validated, owned configuration.
    pub fn configure(mut self, config: PaginationConfig) -> Result<Self> {
        config.validate()?;
        self.config = Some(config);
        Ok(self)
    }
    /// Use the shared XS through 9XL scale independently of the configuration.
    pub fn design_size(mut self, size: DesignSize) -> Self {
        self.design_size = Some(size);
        self
    }
    pub fn get_config(&self) -> Option<&PaginationConfig> {
        self.config.as_ref()
    }
    /// Remove the configuration override while preserving the design size.
    pub fn reset(mut self) -> Self {
        self.config = None;
        self
    }
    pub fn mount(&self, app: &mut App, parent: Node, key: &str) -> Result<Node> {
        if self.config.is_some() || self.design_size.is_some() {
            api().ok_or(Error(9))?;
        }
        if self.custom_size != 0. {
            custom_api().ok_or(Error(9))?;
        }
        let node = app.child(parent, Kind::Pagination, key)?;
        let result = (|| {
            if self.custom_size != 0. {
                app.set_pagination_custom_size(node, self.custom_size)?;
            }
            if let Some(config) = &self.config {
                app.configure_pagination(node, config)?;
            }
            if let Some(size) = self.design_size {
                app.design_size(node, size)?;
            }
            Ok(())
        })();
        if let Err(error) = result {
            app.remove(node)?;
            return Err(error);
        }
        Ok(node)
    }
}
impl App {
    /// Independent of Pagination V1 presentation support.
    pub fn supports_pagination_custom_size(&self) -> bool {
        custom_api().is_some()
    }
    /// Zero restores named sizing; otherwise use a finite height from 1 to 512.
    pub fn set_pagination_custom_size(&mut self, node: Node, size: f32) -> Result<&mut Self> {
        validate_custom(size)?;
        let node = self.valid(node)?;
        let api = custom_api().ok_or(Error(9))?;
        check(unsafe { (api.set)(self.raw, node, size) })?;
        Ok(self)
    }
    /// Read the height override; zero means named sizing. Requires capability.
    pub fn pagination_custom_size(&self, node: Node) -> Result<f32> {
        let node = self.valid(node)?;
        let api = custom_api().ok_or(Error(9))?;
        let mut size = 0.;
        check(unsafe { (api.get)(self.raw, node, &mut size) })?;
        Ok(size)
    }

    /// Report whether the active bridge provides the complete Pagination V1 API.
    pub fn supports_pagination_options(&self) -> bool {
        api().is_some()
    }
    /// Update presentation without changing the page or dispatching callbacks.
    pub fn configure_pagination(
        &mut self,
        node: Node,
        config: &PaginationConfig,
    ) -> Result<&mut Self> {
        let raw = config.to_raw()?;
        let node = self.valid(node)?;
        let bridge = api().ok_or(Error(9))?;
        check(unsafe { (bridge.configure)(self.raw, node, &raw) })?;
        Ok(self)
    }
    /// Clear the override while preserving design size, total and page.
    pub fn reset_pagination(&mut self, node: Node) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let bridge = api().ok_or(Error(9))?;
        check(unsafe { (bridge.configure)(self.raw, node, std::ptr::null()) })?;
        Ok(self)
    }
    /// Return an owned snapshot. Ordinary pagers have no override.
    pub fn pagination_config(&self, node: Node) -> Result<Option<PaginationConfig>> {
        let node = self.valid(node)?;
        let Some(bridge) = api() else {
            return Ok(None);
        };
        let mut raw = PaginationConfig::default().to_raw()?;
        let status = unsafe { (bridge.get)(self.raw, node, &mut raw) };
        if status == 1 {
            return Ok(None);
        }
        check(status)?;
        Ok(Some(PaginationConfig::from_raw(raw)?))
    }
}
impl Builder<'_> {
    /// Set the height on an existing child; detached options preflight creation.
    pub fn pagination_custom_size(self, size: f32) -> Result<Self> {
        self.app.set_pagination_custom_size(self.node, size)?;
        Ok(self)
    }

    /// Configure an already created child. Use PaginationOptions to preflight
    /// provider compatibility before creating the child.
    pub fn pagination_options(self, config: &PaginationConfig) -> Result<Self> {
        self.app.configure_pagination(self.node, config)?;
        Ok(self)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn wire_round_trip_and_malformed_records() {
        let config = PaginationConfig {
            counter: true,
            icon_only: true,
            chevrons: true,
            jump_menu: true,
            group: "Résultats".into(),
            visible_pages: 5,
            available_width: 210.,
            ..Default::default()
        };
        assert_eq!(
            PaginationConfig::from_raw(config.to_raw().unwrap()).unwrap(),
            config
        );
        for corrupt in 0..5 {
            let mut raw = config.to_raw().unwrap();
            match corrupt {
                0 => raw.size += 4,
                1 => raw.version = 2,
                2 => raw.flags |= 16,
                3 => raw.labels[0] = [b'a'; 128],
                _ => raw.labels[0][0] = 0xff,
            }
            assert_eq!(PaginationConfig::from_raw(raw), Err(Error(8)));
        }
    }
}
