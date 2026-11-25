// SPDX-License-Identifier: MIT
//! Fixed-width records and optional adapter for owned SVG resources.
use std::{
    ffi::{c_char, c_void},
    mem::size_of,
    sync::OnceLock,
};

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub(super) struct RawHandle {
    pub(super) slot: u32,
    pub(super) generation: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub(super) struct RawLimits {
    pub(super) size: u32,
    pub(super) version: u32,
    pub(super) reserved: [u32; 2],
    pub(super) max_svg_bytes: u64,
    pub(super) max_pixels: u64,
    pub(super) max_cache_bytes: u64,
    pub(super) max_versions: u64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub(super) struct RawInfo {
    pub(super) size: u32,
    pub(super) version: u32,
    pub(super) icon_error: u32,
    pub(super) reference_count: u32,
    pub(super) parse_count: u64,
    pub(super) raster_count: u64,
    pub(super) cache_bytes: u64,
    pub(super) cache_versions: u64,
    pub(super) source_revision: u64,
    pub(super) source_kind: u32,
    pub(super) flags: u32,
    pub(super) handle: RawHandle,
    pub(super) reserved: u64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub(super) struct RawBitmap {
    pub(super) size: u32,
    pub(super) version: u32,
    pub(super) format: u32,
    pub(super) reserved: u32,
    pub(super) width: u32,
    pub(super) height: u32,
    pub(super) stride: u32,
    pub(super) reserved2: u32,
    pub(super) required_bytes: u64,
    pub(super) revision: u64,
    pub(super) paint_x: f32,
    pub(super) paint_y: f32,
    pub(super) paint_width: f32,
    pub(super) paint_height: f32,
}

macro_rules! record {
    ($ty:ident, $size:literal) => {
        const _: [(); $size] = [(); size_of::<$ty>()];
        impl $ty {
            pub(super) const SIZE: u32 = $size;

            pub(super) fn with_header() -> Self {
                Self {
                    size: Self::SIZE,
                    version: 1,
                    ..Self::default()
                }
            }
        }
    };
}

const _: [(); 8] = [(); size_of::<RawHandle>()];
record!(RawLimits, 48);
record!(RawInfo, 80);
record!(RawBitmap, 64);

// The linked static or shared provider remains live for the whole process.
pub(super) fn api() -> Option<&'static Api> {
    static API: OnceLock<Option<Api>> = OnceLock::new();
    API.get_or_init(resolve).as_ref()
}

macro_rules! resource_group {
    ($($field:ident = $func:ident($($arg:ident: $ty:ty),*) -> $ret:ty;)+) => {
        pub(super) struct Api {
            $(pub(super) $field: unsafe extern "C" fn($($ty),*) -> $ret,)+
        }

        #[cfg(cui_bridge_static)]
        extern "C" {
            $(fn $func($($arg: $ty),*) -> $ret;)+
        }

        #[cfg(any(cui_bridge_static, unix, windows))]
        fn resolve() -> Option<Api> {
            #[cfg(cui_bridge_static)]
            let api = Api { $($field: $func,)+ };

            #[cfg(not(cui_bridge_static))]
            let api = Api {
                $($field: {
                    let name = concat!(stringify!($func), "\0");
                    let symbol = unsafe { super::super::notification_symbol_v1(name.as_ptr().cast()) };
                    if symbol.is_null() {
                        return None;
                    }
                    // The signature matches the named export in cui_bridge.h.
                    unsafe { std::mem::transmute::<*mut c_void, unsafe extern "C" fn($($ty),*) -> $ret>(symbol) }
                },)+
            };

            let compatible = unsafe {
                (api.versions)() & 1 != 0
                    && (api.record_size)(3) == RawLimits::SIZE
                    && (api.record_size)(4) == RawInfo::SIZE
                    && (api.record_size)(5) == RawBitmap::SIZE
            };
            compatible.then_some(api)
        }

        #[cfg(not(any(cui_bridge_static, unix, windows)))]
        fn resolve() -> Option<Api> {
            None
        }
    };
}

resource_group! {
    versions = cui_bridge_icon_versions() -> u32;
    record_size = cui_bridge_icon_record_size_v1(kind: u32) -> u32;
    default_limits = cui_bridge_icon_default_limits_v1(out: *mut RawLimits) -> u32;
    create = cui_bridge_icon_create_v1(app: *mut c_void, limits: *const RawLimits, out: *mut RawHandle) -> u32;
    destroy = cui_bridge_icon_destroy_v1(app: *mut c_void, handle: RawHandle) -> u32;
    get_info = cui_bridge_icon_get_info_v1(app: *mut c_void, handle: RawHandle, out: *mut RawInfo) -> u32;
    get_limits = cui_bridge_icon_get_limits_v1(app: *mut c_void, handle: RawHandle, out: *mut RawLimits) -> u32;
    set_limits = cui_bridge_icon_set_limits_v1(app: *mut c_void, handle: RawHandle, limits: *const RawLimits) -> u32;
    set_source = cui_bridge_icon_set_source_v1(app: *mut c_void, handle: RawHandle, kind: u32, text: *const c_char, bytes: u32) -> u32;
    get_source = cui_bridge_icon_get_source_v1(app: *mut c_void, handle: RawHandle, kind: *mut u32, out: *mut c_char, capacity: u32, required: *mut u32) -> u32;
    reload = cui_bridge_icon_reload_v1(app: *mut c_void, handle: RawHandle) -> u32;
    collect = cui_bridge_icon_collect_v1(app: *mut c_void, handle: RawHandle) -> u32;
    put_asset = cui_bridge_icon_asset_put_v1(app: *mut c_void, handle: RawHandle, path: *const c_char, path_bytes: u32, svg: *const c_char, svg_bytes: u32) -> u32;
    remove_asset = cui_bridge_icon_asset_remove_v1(app: *mut c_void, handle: RawHandle, path: *const c_char, bytes: u32) -> u32;
    clear_assets = cui_bridge_icon_asset_clear_v1(app: *mut c_void, handle: RawHandle) -> u32;
    catalog_count = cui_bridge_icon_catalog_count_v1() -> u32;
    catalog_entry = cui_bridge_icon_catalog_entry_v1(index: u32, is_default: *mut u32, name: *mut c_char, name_capacity: u32, name_required: *mut u32, path: *mut c_char, path_capacity: u32, path_required: *mut u32) -> u32;
    query = cui_bridge_icon_query_v1(name: *const c_char, bytes: u32, availability: *mut u32) -> u32;
    copy_asset = cui_bridge_icon_asset_copy_v1(name: *const c_char, bytes: u32, out: *mut c_char, capacity: u32, required: *mut u32) -> u32;
    prepare = cui_bridge_icon_prepare_v1(app: *mut c_void, handle: RawHandle, width: f32, height: f32, transform: *const f32, icon_error: *mut u32) -> u32;
    copy_bitmap = cui_bridge_icon_bitmap_copy_v1(app: *mut c_void, handle: RawHandle, bitmap: *mut RawBitmap, out: *mut u8, capacity: u32, required: *mut u32) -> u32;
}
