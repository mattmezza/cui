// SPDX-License-Identifier: MIT
//! Optional live rows for retained VirtualLists.
use super::{check, App, Builder, Error, Node, RawNode, Result};
use std::{ffi::c_void, mem::size_of};

/// An existing direct Item and its extent in the next layout.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct VirtualListRow {
    pub item: Node,
    /// Logical pixels from 1 through 4096, or zero for the default 32.
    pub extent: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
struct RowRaw {
    item: RawNode,
    extent: f32,
}
const _: [(); 12] = [(); size_of::<RowRaw>()];

type SetRows = unsafe extern "C" fn(*mut c_void, RawNode, u32, *const RowRaw, u32) -> u32;
type PreserveAnchor = unsafe extern "C" fn(*mut c_void, RawNode, u32) -> u32;

#[cfg(cui_bridge_static)]
fn functions() -> Option<(SetRows, PreserveAnchor)> {
    extern "C" {
        fn cui_bridge_virtual_list_set_rows_v1(
            app: *mut c_void,
            list: RawNode,
            row_size: u32,
            rows: *const RowRaw,
            count: u32,
        ) -> u32;
        fn cui_bridge_virtual_list_preserve_anchor_v1(
            app: *mut c_void,
            list: RawNode,
            enabled: u32,
        ) -> u32;
    }
    Some((
        cui_bridge_virtual_list_set_rows_v1,
        cui_bridge_virtual_list_preserve_anchor_v1,
    ))
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn functions() -> Option<(SetRows, PreserveAnchor)> {
    unsafe {
        let rows =
            super::notification_symbol_v1(b"cui_bridge_virtual_list_set_rows_v1\0".as_ptr().cast());
        let anchor = super::notification_symbol_v1(
            b"cui_bridge_virtual_list_preserve_anchor_v1\0"
                .as_ptr()
                .cast(),
        );
        if rows.is_null() || anchor.is_null() {
            return None;
        }
        Some((
            std::mem::transmute::<*mut c_void, SetRows>(rows),
            std::mem::transmute::<*mut c_void, PreserveAnchor>(anchor),
        ))
    }
}

#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn functions() -> Option<(SetRows, PreserveAnchor)> {
    None
}

impl Error {
    /// The loaded bridge lacks the complete live VirtualList API.
    pub const VIRTUAL_LIST_ROWS_UNAVAILABLE: Self = Self(0x564c52);
}

impl App {
    /// Reports support for complete row projections and opt-in anchoring.
    pub fn supports_virtual_list_rows(&self) -> bool {
        functions().is_some()
    }

    /// Keep the first visible Item and its offset during row edits.
    /// An explicit scroll value before the next step takes precedence.
    pub fn preserve_virtual_list_anchor(&mut self, list: Node, enabled: bool) -> Result<&mut Self> {
        let list = self.valid(list)?;
        let (_, preserve) = functions().ok_or(Error::VIRTUAL_LIST_ROWS_UNAVAILABLE)?;
        check(unsafe { preserve(self.raw, list, u32::from(enabled)) })?;
        Ok(self)
    }

    /// Copy an order and extent for every current direct Item, up to 64.
    /// Errors preserve prior order and extents. Use `child` and `remove` for
    /// membership changes before submitting a projection.
    pub fn set_virtual_list_rows(
        &mut self,
        list: Node,
        rows: &[VirtualListRow],
    ) -> Result<&mut Self> {
        let list = self.valid(list)?;
        let (set, _) = functions().ok_or(Error::VIRTUAL_LIST_ROWS_UNAVAILABLE)?;
        let mut data = [RowRaw::default(); 64];
        if rows.len() > data.len() {
            return Err(Error(5));
        }
        for (out, row) in data.iter_mut().zip(rows) {
            let item = self.valid(row.item)?;
            if row.extent != 0. && !(1. ..=4096.).contains(&row.extent) {
                return Err(Error(3));
            }
            *out = RowRaw {
                item,
                extent: row.extent,
            };
        }
        check(unsafe {
            set(
                self.raw,
                list,
                size_of::<RowRaw>() as u32,
                data.as_ptr(),
                rows.len() as u32,
            )
        })?;
        Ok(self)
    }
}

impl<'a> Builder<'a> {
    /// The list keeps its first visible Item when the application edits rows.
    /// Probe the provider before building when supporting earlier releases.
    pub fn preserve_virtual_list_anchor(self, enabled: bool) -> Result<Self> {
        self.app.preserve_virtual_list_anchor(self.node, enabled)?;
        Ok(self)
    }
}
