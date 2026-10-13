// SPDX-License-Identifier: MIT
// Private ownership extension to pinned accesskit-c; no upstream ABI changes.
use std::{ffi::c_void, sync::Arc};
use accesskit::{ActivationHandler, ActionHandler, DeactivationHandler, TreeUpdate, ActionRequest};
use accesskit_unix::Adapter;
use crate::{ActivationHandlerCallback, ActionHandlerCallback, DeactivationHandlerCallback,
    FfiActivationHandler, FfiActionHandler, FfiDeactivationHandler, unix_adapter, BoxCastPtr};
struct Owner { userdata: *mut c_void, release: extern "C" fn(*mut c_void) }
unsafe impl Send for Owner {}
unsafe impl Sync for Owner {}
impl Drop for Owner { fn drop(&mut self) { (self.release)(self.userdata); } }
struct Activation { inner: FfiActivationHandler, _owner: Arc<Owner> }
impl ActivationHandler for Activation {
    fn request_initial_tree(&mut self) -> Option<TreeUpdate> { self.inner.request_initial_tree() }
}
struct Action { inner: FfiActionHandler, _owner: Arc<Owner> }
impl ActionHandler for Action {
    fn do_action(&mut self, request: ActionRequest) { self.inner.do_action(request); }
}
struct Deactivation { inner: FfiDeactivationHandler, _owner: Arc<Owner> }
impl DeactivationHandler for Deactivation {
    fn deactivate_accessibility(&mut self) { self.inner.deactivate_accessibility(); }
}
#[no_mangle]
pub extern "C" fn cui_accesskit_unix_adapter_new_owned(
    activation: ActivationHandlerCallback, action: ActionHandlerCallback,
    deactivation: DeactivationHandlerCallback, userdata: *mut c_void,
    release: extern "C" fn(*mut c_void),
) -> *mut unix_adapter {
    let owner = Arc::new(Owner { userdata, release });
    BoxCastPtr::to_mut_ptr(Adapter::new(
        Activation { inner: FfiActivationHandler::new(activation, userdata), _owner: owner.clone() },
        Action { inner: FfiActionHandler::new(action, userdata), _owner: owner.clone() },
        Deactivation { inner: FfiDeactivationHandler::new(deactivation, userdata), _owner: owner },
    ))
}
