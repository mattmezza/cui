//! Optional Notification auto-hide policy, sampled by the next successful open.
use super::{check, App, Builder, Error, Node, RawNode, Result};
use std::ffi::c_void;
type Set = unsafe extern "C" fn(*mut c_void, RawNode, u32) -> u32;
#[cfg(cui_bridge_static)]
fn setter() -> Option<Set> {
    extern "C" {
        fn cui_bridge_notification_set_auto_hide_v1(
            app: *mut c_void,
            node: RawNode,
            enabled: u32,
        ) -> u32;
    }
    Some(cui_bridge_notification_set_auto_hide_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn setter() -> Option<Set> {
    unsafe {
        let p = super::notification_symbol_v1(c"cui_bridge_notification_set_auto_hide_v1".as_ptr());
        (!p.is_null()).then(|| std::mem::transmute::<*mut c_void, Set>(p))
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn setter() -> Option<Set> {
    None
}
impl Error {
    pub const NOTIFICATION_AUTO_HIDE_UNAVAILABLE: Self = Self(9);
}
impl App {
    pub fn supports_notification_auto_hide(&self) -> bool {
        setter().is_some()
    }
    /// Configure the next successful open. The default is persistent. Enabled
    /// notifications expire after five seconds present, pausing on hover/focus.
    /// Changes while visible apply only to later opens.
    pub fn notification_auto_hide(&mut self, note: Node, enabled: bool) -> Result<&mut Self> {
        let raw = self.valid(note)?;
        let set = setter().ok_or(Error::NOTIFICATION_AUTO_HIDE_UNAVAILABLE)?;
        check(unsafe { set(self.raw, raw, u32::from(enabled)) })?;
        Ok(self)
    }
}
impl Builder<'_> {
    /// Configure Notification auto-hide before calling `open`.
    pub fn notification_auto_hide(self, enabled: bool) -> Result<Self> {
        self.app.notification_auto_hide(self.node, enabled)?;
        Ok(self)
    }
}
