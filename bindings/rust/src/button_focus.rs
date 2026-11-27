//! Optional retained Button keyboard focus treatment.
use super::{check, App, Builder, Error, Node, RawNode, Result};
use std::ffi::c_void;

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[repr(u32)]
pub enum ButtonFocusMode {
    #[default]
    Legacy = 0,
    Variant = 1,
    OuterRing = 2,
}
type Set = unsafe extern "C" fn(*mut c_void, RawNode, u32) -> u32;
#[cfg(cui_bridge_static)]
fn setter() -> Option<Set> {
    extern "C" {
        fn cui_bridge_set_button_focus_v1(app: *mut c_void, node: RawNode, mode: u32) -> u32;
    }
    Some(cui_bridge_set_button_focus_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn setter() -> Option<Set> {
    unsafe {
        let p = super::notification_symbol_v1(c"cui_bridge_set_button_focus_v1".as_ptr());
        (!p.is_null()).then(|| std::mem::transmute::<*mut c_void, Set>(p))
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn setter() -> Option<Set> {
    None
}
impl Error {
    pub const BUTTON_FOCUS_UNAVAILABLE: Self = Self(9);
}
impl App {
    pub fn supports_button_focus(&self) -> bool {
        setter().is_some()
    }
    /// Replace Button focus treatment. Legacy resets a current provider and
    /// remains usable on older providers. Other kinds are rejected by the bridge.
    pub fn button_focus(&mut self, node: Node, mode: ButtonFocusMode) -> Result<&mut Self> {
        let raw = self.valid(node)?;
        if let Some(set) = setter() {
            check(unsafe { set(self.raw, raw, mode as u32) })?;
        } else if mode != ButtonFocusMode::Legacy {
            return Err(Error::BUTTON_FOCUS_UNAVAILABLE);
        }
        Ok(self)
    }
}
impl Builder<'_> {
    pub fn button_focus(self, mode: ButtonFocusMode) -> Result<Self> {
        self.app.button_focus(self.node, mode)?;
        Ok(self)
    }
}
