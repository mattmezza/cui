//! Optional shared SVG artwork for mounted rotating Spinners.
use super::{ffi::RawHandle, IconResource};
use crate::{check, App, Error, Node, RawNode, Result};
use std::{ffi::c_void, sync::OnceLock};
type Versions = unsafe extern "C" fn() -> u32;
type Set = unsafe extern "C" fn(*mut c_void, RawNode, RawHandle) -> u32;
type Get = unsafe extern "C" fn(*mut c_void, RawNode, *mut RawHandle) -> u32;
struct Api {
    set: Set,
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
        fn cui_bridge_spinner_icon_resource_versions() -> u32;
        fn cui_bridge_spinner_set_icon_resource_v1(a: *mut c_void, n: RawNode, h: RawHandle)
            -> u32;
        fn cui_bridge_spinner_get_icon_resource_v1(
            a: *mut c_void,
            n: RawNode,
            h: *mut RawHandle,
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
            let pointer = unsafe { crate::notification_symbol_v1(name.as_ptr().cast()) };
            if pointer.is_null() {
                return None;
            }
            unsafe { std::mem::transmute::<*mut c_void, $ty>(pointer) }
        }};
    }
    let versions = symbol!(cui_bridge_spinner_icon_resource_versions, Versions);
    let set = symbol!(cui_bridge_spinner_set_icon_resource_v1, Set);
    let get = symbol!(cui_bridge_spinner_get_icon_resource_v1, Get);
    (unsafe { versions() } & 1 != 0).then_some(Api { set, get })
}
#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> Option<Api> {
    None
}
impl App {
    /// Check the complete optional artwork group and existing Icon capability.
    pub fn supports_spinner_icon_resources(&self) -> bool {
        self.supports_icon_resources() && api().is_some()
    }
    /// Bind shared artwork on a mounted Spinner. None restores its catalog icon.
    /// The resource retains its original app ownership.
    pub fn set_spinner_icon_resource(
        &mut self,
        node: Node,
        resource: Option<IconResource>,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        if !self.supports_spinner_icon_resources() {
            return Err(Error(9));
        }
        let handle = resource.map_or(Ok(RawHandle::default()), |r| self.icon_resource_handle(r))?;
        check(unsafe { (api().unwrap().set)(self.raw, node, handle) })?;
        Ok(self)
    }
    /// Copy the app-scoped handle without creating another native owner.
    pub fn spinner_icon_resource(&self, node: Node) -> Result<Option<IconResource>> {
        let node = self.valid(node)?;
        if !self.supports_spinner_icon_resources() {
            return Err(Error(9));
        }
        let mut raw = RawHandle::default();
        let status = unsafe { (api().unwrap().get)(self.raw, node, &mut raw) };
        if status == 1 {
            return Ok(None);
        }
        check(status)?;
        if raw.slot == 0 || raw.generation == 0 {
            return Err(Error(8));
        }
        Ok(Some(IconResource {
            raw,
            app_id: self.id,
        }))
    }
}
