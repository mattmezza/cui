//! Scoped text defaults for retained Row and Column nodes.
use super::{check, App, Builder, Error, Node, RawNode, Result};
use std::ffi::c_void;
/// Zero font size and absent color inherit; the empty value resets both.
/// Packed RGBA includes explicit transparent zero. Icon inherits size/tint,
/// Label both and Spinner tint; explicit child refinements take precedence.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct TextDefaults {
    pub font_size: f32,
    pub color_rgba: Option<u32>,
}
type Set = unsafe extern "C" fn(*mut c_void, RawNode, f32, u32, u32) -> u32;
#[cfg(cui_bridge_static)]
fn setter() -> Option<Set> {
    extern "C" {
        fn cui_bridge_set_text_defaults_v1(
            app: *mut c_void,
            node: RawNode,
            size: f32,
            color: u32,
            custom: u32,
        ) -> u32;
    }
    Some(cui_bridge_set_text_defaults_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn setter() -> Option<Set> {
    unsafe {
        let p = super::notification_symbol_v1(c"cui_bridge_set_text_defaults_v1".as_ptr());
        (!p.is_null()).then(|| std::mem::transmute::<*mut c_void, Set>(p))
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn setter() -> Option<Set> {
    None
}
impl Error {
    pub const TEXT_DEFAULTS_UNAVAILABLE: Self = Self(9);
}
impl App {
    pub fn supports_text_defaults(&self) -> bool {
        setter().is_some()
    }
    /// Replace scoped defaults on a Row/Column. Invalid calls preserve prior state.
    pub fn text_defaults(&mut self, node: Node, options: TextDefaults) -> Result<&mut Self> {
        let raw = self.valid(node)?;
        if !options.font_size.is_finite() || !(0.0..=4096.0).contains(&options.font_size) {
            return Err(Error(3));
        }
        let set = setter().ok_or(Error::TEXT_DEFAULTS_UNAVAILABLE)?;
        check(unsafe {
            set(
                self.raw,
                raw,
                options.font_size,
                options.color_rgba.unwrap_or(0),
                u32::from(options.color_rgba.is_some()),
            )
        })?;
        Ok(self)
    }
}
impl Builder<'_> {
    pub fn text_defaults(self, options: TextDefaults) -> Result<Self> {
        self.app.text_defaults(self.node, options)?;
        Ok(self)
    }
}
