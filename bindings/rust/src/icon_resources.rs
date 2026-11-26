// SPDX-License-Identifier: MIT
//! App-owned SVG resources and copied catalog and raster data.
mod badge;
mod ffi;
mod presentation;
mod presentation_ffi;
mod rating;

pub use badge::{BadgeConfig, BadgeResult, BadgeSize, BadgeVariant};
pub use presentation::{
    IconAlign, IconBoxStyle, IconConfig, IconConstraints, IconOptions, IconResult, IconSize,
};

pub use rating::RatingConfig;

use self::ffi::{api, RawBitmap, RawHandle, RawInfo, RawLimits};
use super::{check, App, Error, Rect, Result};
use std::{
    ffi::c_char,
    ptr::{null, null_mut},
};

const MAX_TEXT_BYTES: u32 = 4096;
const MAX_SVG_BYTES: u32 = 1 << 20;
const MAX_PIXELS: u32 = 16 << 20;
const MAX_BITMAP_BYTES: u32 = 64 << 20;
const MAX_VERSIONS: u32 = 128;

const OK: u32 = 0;
const EMPTY: u32 = 1;
const INVALID: Error = Error(3);
const CAPACITY: u32 = 5;
const OUT_OF_MEMORY: Error = Error(6);
const UNAVAILABLE: Error = Error(9);

/// An app-scoped resource handle. Copies share the same native resource.
/// Release it explicitly with `App::release_icon_resource` or let app
/// destruction release it after all retained widgets have been removed.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct IconResource {
    raw: RawHandle,
    app_id: u64,
}

/// Source classification reported by the bridge.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum IconSourceKind {
    Empty,
    Name,
    Path,
    Data,
}

impl TryFrom<u32> for IconSourceKind {
    type Error = Error;
    fn try_from(value: u32) -> Result<Self> {
        match value {
            0 => Ok(Self::Empty),
            1 => Ok(Self::Name),
            2 => Ok(Self::Path),
            3 => Ok(Self::Data),
            _ => Err(INVALID),
        }
    }
}

/// SVG from the embedded catalog, a resource-local virtual path, or raw data.
/// Paths resolve only through the resource's copied asset map and C catalog.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub enum IconSource {
    #[default]
    Empty,
    Name(String),
    Path(String),
    Data(String),
}

impl IconSource {
    pub fn named(name: impl Into<String>) -> Self {
        Self::Name(name.into())
    }

    pub fn path(path: impl Into<String>) -> Self {
        Self::Path(path.into())
    }

    pub fn svg(svg: impl Into<String>) -> Self {
        Self::Data(svg.into())
    }

    fn parts(&self) -> Result<(u32, *const c_char, u32)> {
        let (kind, text, max) = match self {
            Self::Empty => return Ok((0, null(), 0)),
            Self::Name(text) => (1, text, MAX_TEXT_BYTES),
            Self::Path(text) => (2, text, MAX_TEXT_BYTES),
            Self::Data(text) => (3, text, MAX_SVG_BYTES),
        };
        let (ptr, bytes) = string_span(text, max)?;
        Ok((kind, ptr, bytes))
    }

    fn copied(kind: u32, text: String) -> Result<Self> {
        match IconSourceKind::try_from(kind)? {
            IconSourceKind::Empty if text.is_empty() => Ok(Self::Empty),
            IconSourceKind::Empty => Err(INVALID),
            IconSourceKind::Name if text.len() <= MAX_TEXT_BYTES as usize => Ok(Self::Name(text)),
            IconSourceKind::Path if text.len() <= MAX_TEXT_BYTES as usize => Ok(Self::Path(text)),
            IconSourceKind::Data => Ok(Self::Data(text)),
            _ => Err(INVALID),
        }
    }
}

/// Bounds for SVG admission and preparation. Each zero selects the native
/// default. The bridge also enforces independent app-wide quotas.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct IconLimits {
    pub max_svg_bytes: u64,
    pub max_pixels: u64,
    pub max_cache_bytes: u64,
    pub max_versions: u64,
}

impl IconLimits {
    fn within_caps(self) -> bool {
        self.max_svg_bytes <= MAX_SVG_BYTES.into()
            && self.max_pixels <= MAX_PIXELS.into()
            && self.max_cache_bytes <= MAX_BITMAP_BYTES.into()
            && self.max_versions <= MAX_VERSIONS.into()
    }

    fn encode(self) -> Result<RawLimits> {
        if !self.within_caps() {
            return Err(INVALID);
        }
        Ok(RawLimits {
            max_svg_bytes: self.max_svg_bytes,
            max_pixels: self.max_pixels,
            max_cache_bytes: self.max_cache_bytes,
            max_versions: self.max_versions,
            ..RawLimits::with_header()
        })
    }

    fn copied(raw: RawLimits) -> Result<Self> {
        let value = Self {
            max_svg_bytes: raw.max_svg_bytes,
            max_pixels: raw.max_pixels,
            max_cache_bytes: raw.max_cache_bytes,
            max_versions: raw.max_versions,
        };
        if raw.size != RawLimits::SIZE
            || raw.version != 1
            || raw.reserved != [0; 2]
            || !value.within_caps()
            || [
                raw.max_svg_bytes,
                raw.max_pixels,
                raw.max_cache_bytes,
                raw.max_versions,
            ]
            .contains(&0)
        {
            return Err(INVALID);
        }
        Ok(value)
    }
}

/// Artwork preparation status, independent of errors returned by the bridge.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum IconError {
    Ok,
    Invalid,
    NotFound,
    Parse,
    Limit,
    Allocation,
}

impl TryFrom<u32> for IconError {
    type Error = Error;
    fn try_from(value: u32) -> Result<Self> {
        match value {
            0 => Ok(Self::Ok),
            1 => Ok(Self::Invalid),
            2 => Ok(Self::NotFound),
            3 => Ok(Self::Parse),
            4 => Ok(Self::Limit),
            5 => Ok(Self::Allocation),
            _ => Err(INVALID),
        }
    }
}

/// Catalog availability reported by the loaded provider.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum IconAvailability {
    Unknown,
    CatalogNotEmbedded,
    Embedded,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct IconCatalogEntry {
    pub name: String,
    pub path: String,
    /// Whether the entry is part of the default embedded set.
    pub default_embedded: bool,
}

/// Copied resource counters and active widget references.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct IconInfo {
    pub error: IconError,
    pub references: u32,
    pub parse_count: u64,
    pub raster_count: u64,
    pub cache_bytes: u64,
    pub cache_versions: u64,
    pub source_revision: u64,
    pub source_kind: IconSourceKind,
}

/// An independently owned bitmap of packed straight RGBA8 coverage pixels.
#[derive(Clone, Debug)]
pub struct IconBitmap {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub revision: u64,
    pub paint: Rect,
    pub rgba: Vec<u8>,
}

/// Center-relative affine coefficients in a, b, c, d, tx, ty order.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct IconTransform(pub [f32; 6]);

impl IconTransform {
    pub const IDENTITY: Self = Self([1., 0., 0., 1., 0., 0.]);

    pub fn rotation(radians: f32) -> Self {
        let (sin, cos) = radians.sin_cos();
        Self([cos, sin, -sin, cos, 0., 0.])
    }
}

impl Default for IconTransform {
    fn default() -> Self {
        Self::IDENTITY
    }
}

fn string_span(text: &str, max: u32) -> Result<(*const c_char, u32)> {
    let bytes = text.as_bytes();
    if bytes.len() > max as usize || bytes.contains(&0) {
        return Err(INVALID);
    }
    let ptr = if bytes.is_empty() {
        null()
    } else {
        bytes.as_ptr().cast()
    };
    Ok((ptr, bytes.len() as u32))
}

fn zeroed_bytes(size: u32) -> Result<Vec<u8>> {
    let mut bytes = Vec::new();
    bytes
        .try_reserve_exact(size as usize)
        .map_err(|_| OUT_OF_MEMORY)?;
    bytes.resize(size as usize, 0);
    Ok(bytes)
}

fn copied_string(mut bytes: Vec<u8>) -> Result<String> {
    if bytes.pop() != Some(0) || bytes.contains(&0) {
        return Err(INVALID);
    }
    String::from_utf8(bytes).map_err(|_| Error(7))
}

fn copy_string(
    max: u32,
    allows_absent: bool,
    mut read: impl FnMut(*mut c_char, u32, &mut u32) -> u32,
) -> Result<Option<String>> {
    let mut required = 0;
    let status = read(null_mut(), 0, &mut required);
    if allows_absent && status == EMPTY {
        return Ok(None);
    }
    if status != CAPACITY {
        check(status)?;
        return Err(INVALID);
    }
    if required == 0 || required > max + 1 {
        return Err(INVALID);
    }
    let mut buffer = zeroed_bytes(required)?;
    check(read(buffer.as_mut_ptr().cast(), required, &mut required))?;
    if required as usize != buffer.len() {
        return Err(INVALID);
    }
    copied_string(buffer).map(Some)
}

impl App {
    /// Check all owned Icon resource exports and record sizes.
    pub fn supports_icon_resources(&self) -> bool {
        api().is_some()
    }

    /// Read the loaded provider's normalized defaults.
    pub fn default_icon_limits(&self) -> Result<IconLimits> {
        let api = api().ok_or(UNAVAILABLE)?;
        let mut raw = RawLimits::with_header();
        check(unsafe { (api.default_limits)(&mut raw) })?;
        IconLimits::copied(raw)
    }

    /// Create app-owned resource state and copy its initial source.
    pub fn create_icon_resource(
        &mut self,
        source: &IconSource,
        limits: IconLimits,
    ) -> Result<IconResource> {
        let api = api().ok_or(UNAVAILABLE)?;
        let limits = limits.encode()?;
        let (kind, text, bytes) = source.parts()?;
        let mut raw = RawHandle::default();
        check(unsafe { (api.create)(self.raw, &limits, &mut raw) })?;
        if raw.slot == 0 || raw.generation == 0 {
            return Err(INVALID);
        }
        if kind != 0 {
            let status = unsafe { (api.set_source)(self.raw, raw, kind, text, bytes) };
            if status != OK {
                check(unsafe { (api.destroy)(self.raw, raw) })?;
                return Err(Error(status));
            }
        }
        Ok(IconResource {
            raw,
            app_id: self.id,
        })
    }

    fn icon_resource_handle(&self, resource: IconResource) -> Result<RawHandle> {
        if resource.app_id != self.id {
            return Err(INVALID);
        }
        Ok(resource.raw)
    }

    /// Release an unreferenced resource. Copies become stale. If a retained
    /// widget still references it, the resource remains live.
    pub fn release_icon_resource(&mut self, resource: IconResource) -> Result<()> {
        let raw = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        check(unsafe { (api.destroy)(self.raw, raw) })
    }

    /// Change the source. A failed update preserves the old source.
    pub fn set_icon_source(&mut self, resource: IconResource, source: &IconSource) -> Result<()> {
        let raw = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        let (kind, text, bytes) = source.parts()?;
        check(unsafe { (api.set_source)(self.raw, raw, kind, text, bytes) })
    }

    /// Read the source as an owned Rust value.
    pub fn icon_source(&self, resource: IconResource) -> Result<IconSource> {
        let raw = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        let mut kind = 0;
        let text = copy_string(MAX_SVG_BYTES, false, |out, capacity, required| unsafe {
            (api.get_source)(self.raw, raw, &mut kind, out, capacity, required)
        })?;
        IconSource::copied(kind, text.ok_or(INVALID)?)
    }

    /// Read counters and active-reference information.
    pub fn icon_info(&self, resource: IconResource) -> Result<IconInfo> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        let mut raw = RawInfo::with_header();
        check(unsafe { (api.get_info)(self.raw, handle, &mut raw) })?;
        if raw.size != RawInfo::SIZE
            || raw.version != 1
            || raw.flags != 0
            || raw.reserved != 0
            || raw.handle != handle
            || raw.source_revision == 0
        {
            return Err(INVALID);
        }
        Ok(IconInfo {
            error: IconError::try_from(raw.icon_error)?,
            references: raw.reference_count,
            parse_count: raw.parse_count,
            raster_count: raw.raster_count,
            cache_bytes: raw.cache_bytes,
            cache_versions: raw.cache_versions,
            source_revision: raw.source_revision,
            source_kind: IconSourceKind::try_from(raw.source_kind)?,
        })
    }

    pub fn icon_limits(&self, resource: IconResource) -> Result<IconLimits> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        let mut raw = RawLimits::with_header();
        check(unsafe { (api.get_limits)(self.raw, handle, &mut raw) })?;
        IconLimits::copied(raw)
    }

    /// Change resource bounds, subject to app-wide quotas.
    pub fn set_icon_limits(&mut self, resource: IconResource, limits: IconLimits) -> Result<()> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        let raw = limits.encode()?;
        check(unsafe { (api.set_limits)(self.raw, handle, &raw) })
    }

    /// Copy SVG into this resource's local asset map.
    pub fn put_icon_asset(&mut self, resource: IconResource, path: &str, svg: &str) -> Result<()> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        let (path, path_bytes) = string_span(path, MAX_TEXT_BYTES)?;
        let (svg, svg_bytes) = string_span(svg, MAX_SVG_BYTES)?;
        check(unsafe { (api.put_asset)(self.raw, handle, path, path_bytes, svg, svg_bytes) })
    }

    pub fn remove_icon_asset(&mut self, resource: IconResource, path: &str) -> Result<()> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        let (path, bytes) = string_span(path, MAX_TEXT_BYTES)?;
        check(unsafe { (api.remove_asset)(self.raw, handle, path, bytes) })
    }

    pub fn clear_icon_assets(&mut self, resource: IconResource) -> Result<()> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        check(unsafe { (api.clear_assets)(self.raw, handle) })
    }

    /// Invalidate preparation while retaining the resource's current source.
    pub fn reload_icon(&mut self, resource: IconResource) -> Result<()> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        check(unsafe { (api.reload)(self.raw, handle) })
    }

    /// Release inactive masks after all earlier frames have been consumed.
    pub fn collect_icon(&mut self, resource: IconResource) -> Result<()> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        check(unsafe { (api.collect)(self.raw, handle) })
    }

    /// Prepare coverage in logical pixels. Check the artwork status even
    /// when the bridge call succeeds. A failed prepare may leave an old mask.
    pub fn prepare_icon(
        &mut self,
        resource: IconResource,
        width: f32,
        height: f32,
        transform: IconTransform,
    ) -> Result<IconError> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        if !(0. ..=4096.).contains(&width)
            || !(0. ..=4096.).contains(&height)
            || !transform
                .0
                .iter()
                .all(|coefficient| coefficient.is_finite())
        {
            return Err(INVALID);
        }
        let mut raw = 0;
        check(unsafe {
            (api.prepare)(
                self.raw,
                handle,
                width,
                height,
                transform.0.as_ptr(),
                &mut raw,
            )
        })?;
        IconError::try_from(raw)
    }

    /// Copy packed RGBA8 pixels and paint bounds from the active bitmap.
    pub fn icon_bitmap(&self, resource: IconResource) -> Result<IconBitmap> {
        let handle = self.icon_resource_handle(resource)?;
        let api = api().ok_or(UNAVAILABLE)?;
        let mut bitmap = RawBitmap::with_header();
        let mut required = 0;
        let status = unsafe {
            (api.copy_bitmap)(self.raw, handle, &mut bitmap, null_mut(), 0, &mut required)
        };
        let rgba = if status == CAPACITY {
            if required == 0 || required > MAX_BITMAP_BYTES {
                return Err(INVALID);
            }
            let mut buffer = zeroed_bytes(required)?;
            check(unsafe {
                (api.copy_bitmap)(
                    self.raw,
                    handle,
                    &mut bitmap,
                    buffer.as_mut_ptr(),
                    required,
                    &mut required,
                )
            })?;
            buffer
        } else {
            check(status)?;
            Vec::new()
        };
        let paint = Rect {
            x: bitmap.paint_x,
            y: bitmap.paint_y,
            width: bitmap.paint_width,
            height: bitmap.paint_height,
        };
        if bitmap.size != RawBitmap::SIZE
            || bitmap.version != 1
            || bitmap.format != 0
            || bitmap.reserved != 0
            || bitmap.reserved2 != 0
            || bitmap.width > 4096
            || bitmap.height > 4096
            || bitmap.width * 4 != bitmap.stride
            || u64::from(bitmap.stride) * u64::from(bitmap.height) != bitmap.required_bytes
            || bitmap.required_bytes != rgba.len() as u64
            || required as usize != rgba.len()
            || (rgba.is_empty()
                && (bitmap.revision != 0 || bitmap.width != 0 || bitmap.height != 0))
            || (!rgba.is_empty() && bitmap.revision == 0)
            || ![paint.x, paint.y, paint.width, paint.height]
                .iter()
                .all(|value| value.is_finite())
            || paint.width < 0.
            || paint.height < 0.
        {
            return Err(INVALID);
        }
        Ok(IconBitmap {
            width: bitmap.width,
            height: bitmap.height,
            stride: bitmap.stride,
            revision: bitmap.revision,
            paint,
            rgba,
        })
    }

    pub fn icon_catalog_count(&self) -> Result<u32> {
        let api = api().ok_or(UNAVAILABLE)?;
        Ok(unsafe { (api.catalog_count)() })
    }

    /// Copy a catalog entry from the loaded provider.
    pub fn icon_catalog_entry(&self, index: u32) -> Result<IconCatalogEntry> {
        let api = api().ok_or(UNAVAILABLE)?;
        // Catalog strings have a smaller fixed cap than raw SVG data.
        let cap = MAX_TEXT_BYTES + 1;
        let mut name = [0u8; MAX_TEXT_BYTES as usize + 1];
        let mut path = [0u8; MAX_TEXT_BYTES as usize + 1];
        let (mut embedded, mut name_len, mut path_len) = (0, 0, 0);
        check(unsafe {
            (api.catalog_entry)(
                index,
                &mut embedded,
                name.as_mut_ptr().cast(),
                cap,
                &mut name_len,
                path.as_mut_ptr().cast(),
                cap,
                &mut path_len,
            )
        })?;
        if embedded > 1 || !(2..=cap).contains(&name_len) || !(2..=cap).contains(&path_len) {
            return Err(INVALID);
        }
        Ok(IconCatalogEntry {
            name: copied_string(name[..name_len as usize].to_vec())?,
            path: copied_string(path[..path_len as usize].to_vec())?,
            default_embedded: embedded != 0,
        })
    }

    pub fn query_icon(&self, name: &str) -> Result<IconAvailability> {
        let api = api().ok_or(UNAVAILABLE)?;
        let (name, bytes) = string_span(name, MAX_TEXT_BYTES)?;
        let mut availability = 0;
        check(unsafe { (api.query)(name, bytes, &mut availability) })?;
        match availability {
            0 => Ok(IconAvailability::Unknown),
            1 => Ok(IconAvailability::CatalogNotEmbedded),
            2 => Ok(IconAvailability::Embedded),
            _ => Err(INVALID),
        }
    }

    /// Copy embedded SVG. Unknown or catalog-only entries return None.
    pub fn copy_icon_asset(&self, name: &str) -> Result<Option<String>> {
        let api = api().ok_or(UNAVAILABLE)?;
        let (name, bytes) = string_span(name, MAX_TEXT_BYTES)?;
        copy_string(MAX_SVG_BYTES, true, |out, capacity, required| unsafe {
            (api.copy_asset)(name, bytes, out, capacity, required)
        })
    }
}
