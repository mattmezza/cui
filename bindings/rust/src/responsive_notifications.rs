//! Optional retained notification layout; no additional node or event schema.
use super::{check, App, Error, Result};
use std::ffi::c_void;
type Set = unsafe extern "C" fn(*mut c_void, u32) -> u32;
#[cfg(cui_bridge_static)]
fn setter() -> Option<Set> {
    extern "C" {
        fn cui_bridge_set_responsive_notifications_v1(app: *mut c_void, enabled: u32) -> u32;
    }
    Some(cui_bridge_set_responsive_notifications_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn setter() -> Option<Set> {
    unsafe {
        let p = super::notification_symbol_v1(
            b"cui_bridge_set_responsive_notifications_v1\0"
                .as_ptr()
                .cast(),
        );
        if p.is_null() {
            None
        } else {
            Some(std::mem::transmute::<*mut c_void, Set>(p))
        }
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn setter() -> Option<Set> {
    None
}
impl App {
    /// Whether the provider offers responsive retained notifications.
    pub fn supports_responsive_notifications(&self) -> bool {
        setter().is_some()
    }
    /// Enable wrapping and independent Close for all retained notifications in this app.
    /// Defaults off, is reversible, and must be called outside a step. Missing providers
    /// return Error(9), including when disabling the feature.
    pub fn set_responsive_notifications(&mut self, enabled: bool) -> Result<&mut Self> {
        let set = setter().ok_or(Error(9))?;
        check(unsafe { set(self.raw, enabled as u32) })?;
        Ok(self)
    }
}
