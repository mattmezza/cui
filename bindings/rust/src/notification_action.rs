//! Optional ordinary Button retained by a Notification.
use super::{check, string, App, Builder, Error, Node, RawNode, Result};
use std::ffi::{c_char, c_void};
type Create = unsafe extern "C" fn(*mut c_void, RawNode, *const c_char, *mut RawNode) -> u32;
#[cfg(cui_bridge_static)]
fn factory() -> Option<Create> {
    extern "C" {
        fn cui_bridge_notification_action_create_v1(
            app: *mut c_void,
            parent: RawNode,
            key: *const c_char,
            out: *mut RawNode,
        ) -> u32;
    }
    Some(cui_bridge_notification_action_create_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn factory() -> Option<Create> {
    unsafe {
        let p = super::notification_symbol_v1(c"cui_bridge_notification_action_create_v1".as_ptr());
        (!p.is_null()).then(|| std::mem::transmute::<*mut c_void, Create>(p))
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn factory() -> Option<Create> {
    None
}
impl Error {
    pub const NOTIFICATION_ACTION_UNAVAILABLE: Self = Self(9);
}
impl App {
    /// Whether the provider can retain one ordinary Button in a Notification.
    pub fn supports_notification_action(&self) -> bool {
        factory().is_some()
    }
    /// Create the Notification's action Button. Ordinary click events and styling
    /// apply; activation leaves the parent open. Parent removal removes the Button.
    pub fn notification_action(&mut self, parent: Node, key: &str, label: &str) -> Result<Node> {
        let parent = self.valid(parent)?;
        let create = factory().ok_or(Error::NOTIFICATION_ACTION_UNAVAILABLE)?;
        let key = string(key)?;
        let _ = string(label)?;
        let mut raw = RawNode::default();
        check(unsafe { create(self.raw, parent, key.as_ptr(), &mut raw) })?;
        let node = Node {
            raw,
            app_id: self.id,
        };
        if let Err(err) = self.text(node, label) {
            let _ = self.remove(node);
            return Err(err);
        }
        Ok(node)
    }
    /// Create the action and borrow the app for ordinary fluent Button properties.
    pub fn build_notification_action(
        &mut self,
        parent: Node,
        key: &str,
        label: &str,
    ) -> Result<Builder<'_>> {
        let node = self.notification_action(parent, key, label)?;
        Ok(Builder { app: self, node })
    }
}
