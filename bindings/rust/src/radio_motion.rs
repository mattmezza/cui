//! Optional retained Radio and RadioGroup presentation; selection stays synchronous.
use super::{check, App, Builder, Error, Node, RawNode, Result};
use std::ffi::c_void;
type Set = unsafe extern "C" fn(*mut c_void, RawNode, u32) -> u32;
#[cfg(cui_bridge_static)]
fn setter() -> Option<Set> {
    extern "C" {
        fn cui_bridge_radio_set_motion_v1(a: *mut c_void, n: RawNode, enabled: u32) -> u32;
    }
    Some(cui_bridge_radio_set_motion_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn setter() -> Option<Set> {
    unsafe {
        let p = super::notification_symbol_v1(b"cui_bridge_radio_set_motion_v1\0".as_ptr().cast());
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
    /// Whether this provider offers retained Radio and RadioGroup motion.
    pub fn supports_radio_motion(&self) -> bool {
        setter().is_some()
    }
    /// Opt into retained mark/fill animation. False preserves immediate presentation
    /// even with an older provider; unsupported opt-in returns Error(9).
    /// Only Radio and RadioGroup nodes are accepted. Events keep their existing timing.
    pub fn animate_selection(&mut self, node: Node, enabled: bool) -> Result<&mut Self> {
        let raw = self.valid(node)?;
        if let Some(set) = setter() {
            check(unsafe { set(self.raw, raw, enabled as u32) })?;
        } else {
            // Validate lifecycle before applying the old-provider compatibility default.
            let mut required = 0;
            let status = unsafe {
                super::cui_bridge_get_text(self.raw, raw, std::ptr::null_mut(), 0, &mut required)
            };
            if status != 0 && status != 5 {
                return Err(Error(status));
            }
            if !self.radio_nodes.contains(&raw) {
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
}
