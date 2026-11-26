// SPDX-License-Identifier: MIT
//! Optional retained Attachment presentation and decoded previews.
use crate::{check, App, AttachmentStatus, Builder, Error, Node, RawNode, Result};
use std::{ffi::c_void, mem, ptr, sync::OnceLock};

/// Metadata font weight.
#[repr(u32)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum AttachmentTextWeight {
    #[default]
    Normal,
    Medium,
    Semibold,
    Bold,
}

/// Metadata style. Nonzero letter spacing needs host text support.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct AttachmentTypography {
    pub weight: AttachmentTextWeight,
    pub letter_spacing: f32,
    pub italic: bool,
    pub monospace: bool,
}

/// Optional refinement for one metadata slot. Zero font size inherits.
/// A color is 0xRRGGBBAA. Some(0) selects transparent.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct AttachmentTextStyle {
    pub font_size: f32,
    pub color_rgba: Option<u32>,
    pub typography: Option<AttachmentTypography>,
}

/// One-line ellipsis with full semantic labels. Width and named size are set
/// separately. Vertical media aspect is width/height, or zero for square.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct AttachmentOptions {
    pub vertical: bool,
    /// Reserve a preview slot, including while decoded pixels are unavailable.
    pub media: bool,
    /// During Uploading, show indeterminate feedback instead of a percentage.
    pub indeterminate: bool,
    pub media_aspect: f32,
    pub title_status: Option<AttachmentStatus>,
    pub description_status: Option<AttachmentStatus>,
    pub title: AttachmentTextStyle,
    pub description: AttachmentTextStyle,
}

#[repr(C)]
#[derive(Default)]
struct TextRaw {
    flags: u32,
    weight: u32,
    font_size: f32,
    letter_spacing: f32,
    color_rgba: u32,
    reserved: u32,
}

#[repr(C)]
struct OptionsRaw {
    struct_size: u32,
    schema_version: u32,
    flags: u32,
    reserved: u32,
    media_aspect: f32,
    title_status: u32,
    description_status: u32,
    reserved2: u32,
    title: TextRaw,
    description: TextRaw,
}

const _: () = {
    assert!(mem::size_of::<TextRaw>() == 24);
    assert!(mem::offset_of!(TextRaw, font_size) == 8);
    assert!(mem::size_of::<OptionsRaw>() == 80);
    assert!(mem::offset_of!(OptionsRaw, title) == 32);
    assert!(mem::offset_of!(OptionsRaw, description) == 56);
};

impl AttachmentTextStyle {
    fn wire(self) -> TextRaw {
        let typography = self.typography.unwrap_or_default();
        TextRaw {
            flags: u32::from(self.typography.is_some())
                | (u32::from(self.color_rgba.is_some()) << 1)
                | (u32::from(typography.italic) << 2)
                | (u32::from(typography.monospace) << 3),
            weight: typography.weight as u32,
            font_size: self.font_size,
            letter_spacing: typography.letter_spacing,
            color_rgba: self.color_rgba.unwrap_or_default(),
            reserved: 0,
        }
    }
}

impl AttachmentOptions {
    fn wire(self) -> OptionsRaw {
        OptionsRaw {
            struct_size: mem::size_of::<OptionsRaw>() as u32,
            schema_version: 1,
            flags: u32::from(self.vertical)
                | (u32::from(self.media) << 1)
                | (u32::from(self.indeterminate) << 2)
                | (u32::from(self.title_status.is_some()) << 3)
                | (u32::from(self.description_status.is_some()) << 4),
            reserved: 0,
            media_aspect: self.media_aspect,
            title_status: self.title_status.map_or(0, |v| v as u32),
            description_status: self.description_status.map_or(0, |v| v as u32),
            reserved2: 0,
            title: self.title.wire(),
            description: self.description.wire(),
        }
    }
}

type Configure = unsafe extern "C" fn(*mut c_void, RawNode, *const OptionsRaw) -> u32;
type Preview = unsafe extern "C" fn(*mut c_void, RawNode, *const u8, u32, u32, u32, u32) -> u32;

#[derive(Default)]
struct AttachmentApi {
    configure: Option<Configure>,
    preview: Option<Preview>,
}

fn api() -> &'static AttachmentApi {
    static API: OnceLock<AttachmentApi> = OnceLock::new();
    API.get_or_init(resolve)
}

#[cfg(cui_bridge_static)]
fn resolve() -> AttachmentApi {
    extern "C" {
        fn cui_bridge_attachment_configure_v1(
            app: *mut c_void,
            node: RawNode,
            options: *const OptionsRaw,
        ) -> u32;
        fn cui_bridge_attachment_set_preview_rgba_v1(
            app: *mut c_void,
            node: RawNode,
            pixels: *const u8,
            width: u32,
            height: u32,
            stride: u32,
            byte_count: u32,
        ) -> u32;
    }
    AttachmentApi {
        configure: Some(cui_bridge_attachment_configure_v1),
        preview: Some(cui_bridge_attachment_set_preview_rgba_v1),
    }
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve() -> AttachmentApi {
    unsafe {
        let configure =
            crate::notification_symbol_v1(c"cui_bridge_attachment_configure_v1".as_ptr());
        let preview =
            crate::notification_symbol_v1(c"cui_bridge_attachment_set_preview_rgba_v1".as_ptr());
        AttachmentApi {
            configure: (!configure.is_null())
                .then(|| mem::transmute::<*mut c_void, Configure>(configure)),
            preview: (!preview.is_null()).then(|| mem::transmute::<*mut c_void, Preview>(preview)),
        }
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> AttachmentApi {
    AttachmentApi::default()
}

impl Error {
    /// The bridge lacks the requested Attachment V1 operation.
    pub const ATTACHMENT_OPTIONS_UNAVAILABLE: Self = Self(9);
}

impl App {
    pub fn supports_attachment_options(&self) -> bool {
        api().configure.is_some()
    }

    pub fn supports_attachment_preview(&self) -> bool {
        api().preview.is_some()
    }

    /// Copy options, or restore the legacy presentation with None. Preview pixels
    /// and named size are independent. Configured labels must be single lines.
    pub fn configure_attachment(
        &mut self,
        node: Node,
        options: Option<&AttachmentOptions>,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let configure = api()
            .configure
            .ok_or(Error::ATTACHMENT_OPTIONS_UNAVAILABLE)?;
        let raw = options.copied().map(AttachmentOptions::wire);
        check(unsafe {
            configure(
                self.raw,
                node,
                raw.as_ref().map_or(ptr::null(), ptr::from_ref),
            )
        })?;
        Ok(self)
    }

    /// Copy straight RGBA8 pixels. At most 16 MiB including row padding.
    pub fn set_attachment_preview_rgba(
        &mut self,
        node: Node,
        pixels: &[u8],
        width: u32,
        height: u32,
        stride: u32,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let preview = api().preview.ok_or(Error::ATTACHMENT_OPTIONS_UNAVAILABLE)?;
        let count = u32::try_from(pixels.len()).map_err(|_| Error(3))?;
        check(unsafe {
            preview(
                self.raw,
                node,
                pixels.as_ptr(),
                width,
                height,
                stride,
                count,
            )
        })?;
        Ok(self)
    }

    /// Release the preview without changing presentation or transfer status.
    pub fn clear_attachment_preview(&mut self, node: Node) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let preview = api().preview.ok_or(Error::ATTACHMENT_OPTIONS_UNAVAILABLE)?;
        check(unsafe { preview(self.raw, node, ptr::null(), 0, 0, 0, 0) })?;
        Ok(self)
    }
}

impl Builder<'_> {
    pub fn attachment_options(self, options: &AttachmentOptions) -> Result<Self> {
        self.app.configure_attachment(self.node, Some(options))?;
        Ok(self)
    }

    pub fn attachment_preview_rgba(
        self,
        pixels: &[u8],
        width: u32,
        height: u32,
        stride: u32,
    ) -> Result<Self> {
        self.app
            .set_attachment_preview_rgba(self.node, pixels, width, height, stride)?;
        Ok(self)
    }
}
