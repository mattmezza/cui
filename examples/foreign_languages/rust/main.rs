// SPDX-License-Identifier: MIT
use std::{ffi::{c_char, c_void, CString}, env, ptr};
unsafe extern "C" {
    fn cui_example_run(font: *const c_char, frames: u32,
        callback: extern "C" fn(*mut c_void, i32), user: *mut c_void) -> i32;
}
extern "C" fn changed(user: *mut c_void, count: i32) {
    // The host invokes synchronously and never stores this pointer.
    unsafe { *(user as *mut i32) = count; }
    println!("Rust count: {count}");
}
fn main() {
    let args: Vec<_> = env::args().collect();
    let font = args.get(1).map(|s| CString::new(s.as_str()).unwrap());
    let frames = args.get(2).map(|s| s.parse::<u32>().unwrap()).unwrap_or(0);
    let mut observed: i32 = 0;
    let result = unsafe { cui_example_run(font.as_ref().map_or(ptr::null(), |s| s.as_ptr()),
        frames, changed, &mut observed as *mut i32 as *mut c_void) };
    assert_eq!(result, 0, "CUI host failed");
    if font.is_none() { assert_eq!(observed, 2); }
}
