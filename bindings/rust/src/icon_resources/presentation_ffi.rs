// SPDX-License-Identifier: MIT
//! Exact native-endian retained Icon and Badge bridge records.
use super::ffi::{optional_group, record, RawHandle};
use crate::{RawNode, Rect};
use std::ffi::{c_char, c_void};

#[repr(C)]
#[derive(Clone, Copy, Default, PartialEq)]
pub(super) struct RawStyle {
    pub size: u32,
    pub version: u32,
    pub flags: u32,
    pub reserved: u32,
    pub width: f32,
    pub height: f32,
    pub padding: f32,
    pub gap: f32,
    pub radius: f32,
    pub background_rgba: u32,
    pub border_rgba: u32,
    pub border_width: f32,
    pub align_x: u32,
    pub align_y: u32,
    pub offset_x: f32,
    pub offset_y: f32,
    pub corners: [f32; 4],
    pub border_edges: [f32; 4],
    pub max_width: f32,
    pub max_height: f32,
    pub opacity: f32,
    pub padding_edges: [f32; 4],
    pub reserved_tail: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default, PartialEq)]
pub(super) struct RawConstraints {
    pub size: u32,
    pub version: u32,
    pub flags: u32,
    pub reserved: u32,
    pub min_width: f32,
    pub min_height: f32,
    pub max_width: f32,
    pub max_height: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Default, PartialEq)]
pub(super) struct RawOptions {
    pub size: u32,
    pub version: u32,
    pub flags: u32,
    pub icon_size: u32,
    pub design_size: u32,
    pub reserved: [u32; 3],
    pub custom_size: f32,
    pub inherited_text_size: f32,
    pub width: f32,
    pub height: f32,
    pub opacity: f32,
    pub rem_size: f32,
    pub color_rgba: u32,
    pub inherited_color_rgba: u32,
    pub transform: [f32; 6],
    pub style: RawStyle,
    pub constraints: RawConstraints,
    pub reserved_tail: [u32; 2],
}

#[repr(C)]
#[derive(Default)]
pub(super) struct RawIconResult {
    pub size: u32,
    pub version: u32,
    pub icon_error: u32,
    pub frame_status: u32,
    pub node: RawNode,
    pub frame_sequence: u64,
    pub reserved: u64,
}

#[repr(C)]
#[derive(Default, PartialEq)]
pub(super) struct RawBadgeConfig {
    pub size: u32,
    pub version: u32,
    pub reserved: [u32; 2],
    pub variant: u32,
    pub control_size: u32,
    pub design_size: u32,
    pub flags: u32,
    pub count: u64,
    pub maximum: u64,
    pub color_rgba: u32,
    pub reserved_tail: u32,
    pub artwork: RawHandle,
}

#[repr(C)]
#[derive(Default)]
pub(super) struct RawBadgeResult {
    pub size: u32,
    pub version: u32,
    pub submitted: u32,
    pub icon_error: u32,
    pub node: RawNode,
    pub frame_sequence: u64,
    pub frame_status: u32,
    pub flags: u32,
}

record!(RawStyle, 128);
record!(RawConstraints, 32);
record!(RawOptions, 256);
record!(RawIconResult, 40);
record!(RawBadgeConfig, 64);
record!(RawBadgeResult, 40);

#[cfg(any(cui_bridge_static, unix, windows))]
unsafe fn artwork_record_matches() -> bool {
    #[cfg(cui_bridge_static)]
    let record_size = cui_bridge_icon_presentation_record_size_v1;
    #[cfg(not(cui_bridge_static))]
    let record_size = {
        let name = c"cui_bridge_icon_presentation_record_size_v1";
        let symbol = crate::notification_symbol_v1(name.as_ptr());
        if symbol.is_null() {
            return false;
        }
        std::mem::transmute::<*mut c_void, unsafe extern "C" fn(u32) -> u32>(symbol)
    };
    record_size(2) == RawOptions::SIZE
}

optional_group! {
    IconApi, icon_api, resolve_icon, |api: &IconApi| unsafe {
        (api.versions)() & 1 != 0
            && (api.record_size)(0) == RawStyle::SIZE
            && (api.record_size)(1) == RawConstraints::SIZE
            && (api.record_size)(2) == RawOptions::SIZE
            && (api.record_size)(3) == RawIconResult::SIZE
    };
    versions = cui_bridge_icon_presentation_versions() -> u32;
    record_size = cui_bridge_icon_presentation_record_size_v1(kind: u32) -> u32;
    configure = cui_bridge_icon_configure_node_v1(app: *mut c_void, node: RawNode, handle: RawHandle, options: *const RawOptions, name: *const c_char, name_bytes: u32) -> u32;
    config = cui_bridge_icon_get_node_config_v1(app: *mut c_void, node: RawNode, handle: *mut RawHandle, options: *mut RawOptions) -> u32;
    name = cui_bridge_icon_copy_node_name_v1(app: *mut c_void, node: RawNode, out: *mut c_char, capacity: u32, required: *mut u32) -> u32;
    reset = cui_bridge_icon_reset_node_v1(app: *mut c_void, node: RawNode) -> u32;
    result = cui_bridge_icon_get_node_result_v1(app: *mut c_void, node: RawNode, out: *mut RawIconResult) -> u32;
}

optional_group! {
    BadgeApi, badge_api, resolve_badge, |api: &BadgeApi| unsafe {
        (api.versions)() & 1 != 0
            && (api.record_size)(0) == RawBadgeConfig::SIZE
            && (api.record_size)(1) == RawBadgeResult::SIZE
            && artwork_record_matches()
    };
    versions = cui_bridge_badge_versions() -> u32;
    record_size = cui_bridge_badge_record_size_v1(kind: u32) -> u32;
    default_config = cui_bridge_badge_default_config_v1(out: *mut RawBadgeConfig) -> u32;
    create = cui_bridge_badge_create_v1(app: *mut c_void, parent: RawNode, key: *const c_char, config: *const RawBadgeConfig, artwork: *const RawOptions, name: *const c_char, name_bytes: u32, named_icon: *const c_char, icon_bytes: u32, out: *mut RawNode) -> u32;
    configure = cui_bridge_badge_configure_v1(app: *mut c_void, node: RawNode, config: *const RawBadgeConfig, artwork: *const RawOptions, name: *const c_char, name_bytes: u32, named_icon: *const c_char, icon_bytes: u32) -> u32;
    config = cui_bridge_badge_get_config_v1(app: *mut c_void, node: RawNode, config: *mut RawBadgeConfig, artwork: *mut RawOptions) -> u32;
    text = cui_bridge_badge_copy_text_v1(app: *mut c_void, node: RawNode, field: u32, out: *mut c_char, capacity: u32, required: *mut u32) -> u32;
    result = cui_bridge_badge_get_result_v1(app: *mut c_void, node: RawNode, out: *mut RawBadgeResult) -> u32;
    bounds = cui_bridge_badge_indicator_bounds_v1(app: *mut c_void, node: RawNode, out: *mut Rect) -> u32;
}
