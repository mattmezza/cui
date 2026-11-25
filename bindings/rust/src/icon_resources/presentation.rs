// SPDX-License-Identifier: MIT
//! Retained Icon composition and copied per-frame results.
use super::ffi::RawHandle;
use super::presentation_ffi::{icon_api, RawConstraints, RawIconResult, RawOptions, RawStyle};
use super::{
    copy_string, string_span, IconError, IconResource, IconTransform, EMPTY, INVALID,
    MAX_TEXT_BYTES, UNAVAILABLE,
};
use crate::{check, App, Builder, DesignSize, Kind, Node, Result, Size};

/// Native Icon size preset or a custom logical size.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub enum IconSize {
    #[default]
    Inherited,
    ExtraSmall,
    Small,
    Medium,
    Large,
    Custom(f32),
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum IconAlign {
    #[default]
    Start,
    Center,
    End,
}

impl TryFrom<u32> for IconAlign {
    type Error = crate::Error;

    fn try_from(raw: u32) -> Result<Self> {
        match raw {
            0 => Ok(Self::Start),
            1 => Ok(Self::Center),
            2 => Ok(Self::End),
            _ => Err(INVALID),
        }
    }
}

pub(super) fn design_size(value: u32) -> Result<DesignSize> {
    const SIZES: [Size; 15] = [
        Size::Inherit,
        Size::Xs,
        Size::Sm,
        Size::Base,
        Size::Md,
        Size::Lg,
        Size::Xl,
        Size::X2l,
        Size::X3l,
        Size::X4l,
        Size::X5l,
        Size::X6l,
        Size::X7l,
        Size::X8l,
        Size::X9l,
    ];
    SIZES.get(value as usize).copied().ok_or(INVALID)
}

fn flag(present: bool, bit: u32) -> u32 {
    u32::from(present) << bit
}

fn set(flags: u32, bit: u32) -> bool {
    flags & (1 << bit) != 0
}

/// Optional box decoration around retained Icon artwork.
/// Corner order is top-left, top-right, bottom-left, bottom-right.
/// Edge order is left, right, top, bottom. Colors use numeric 0xRRGGBBAA.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct IconBoxStyle {
    pub width: f32,
    pub height: f32,
    pub max_width: f32,
    pub max_height: f32,
    pub padding: f32,
    pub gap: f32,
    pub radius: f32,
    pub background_rgba: u32,
    pub border_rgba: u32,
    pub border_width: f32,
    pub align_x: IconAlign,
    pub align_y: IconAlign,
    pub offset_x: f32,
    pub offset_y: f32,
    pub corner_radii: Option<[f32; 4]>,
    pub border_edges: Option<[f32; 4]>,
    pub padding_edges: Option<[f32; 4]>,
    pub opacity: Option<f32>,
    pub row: bool,
    pub clip: bool,
    pub fixed_width: bool,
    pub fixed_height: bool,
    pub layout_only: bool,
    pub inert: bool,
    pub width_percent: bool,
    pub height_percent: bool,
}

impl IconBoxStyle {
    fn encode(self) -> RawStyle {
        RawStyle {
            flags: flag(self.row, 0)
                | flag(self.clip, 1)
                | flag(self.corner_radii.is_some(), 2)
                | flag(self.border_edges.is_some(), 3)
                | flag(self.fixed_width, 4)
                | flag(self.fixed_height, 5)
                | flag(self.layout_only, 6)
                | flag(self.inert, 7)
                | flag(self.opacity.is_some(), 8)
                | flag(self.width_percent, 9)
                | flag(self.height_percent, 10)
                | flag(self.padding_edges.is_some(), 11),
            width: self.width,
            height: self.height,
            padding: self.padding,
            gap: self.gap,
            radius: self.radius,
            background_rgba: self.background_rgba,
            border_rgba: self.border_rgba,
            border_width: self.border_width,
            align_x: self.align_x as u32,
            align_y: self.align_y as u32,
            offset_x: self.offset_x,
            offset_y: self.offset_y,
            corners: self.corner_radii.unwrap_or_default(),
            border_edges: self.border_edges.unwrap_or_default(),
            max_width: self.max_width,
            max_height: self.max_height,
            opacity: self.opacity.unwrap_or_default(),
            padding_edges: self.padding_edges.unwrap_or_default(),
            ..RawStyle::with_header()
        }
    }

    fn decode(raw: RawStyle) -> Result<Self> {
        Ok(Self {
            width: raw.width,
            height: raw.height,
            max_width: raw.max_width,
            max_height: raw.max_height,
            padding: raw.padding,
            gap: raw.gap,
            radius: raw.radius,
            background_rgba: raw.background_rgba,
            border_rgba: raw.border_rgba,
            border_width: raw.border_width,
            align_x: raw.align_x.try_into()?,
            align_y: raw.align_y.try_into()?,
            offset_x: raw.offset_x,
            offset_y: raw.offset_y,
            corner_radii: set(raw.flags, 2).then_some(raw.corners),
            border_edges: set(raw.flags, 3).then_some(raw.border_edges),
            padding_edges: set(raw.flags, 11).then_some(raw.padding_edges),
            opacity: set(raw.flags, 8).then_some(raw.opacity),
            row: set(raw.flags, 0),
            clip: set(raw.flags, 1),
            fixed_width: set(raw.flags, 4),
            fixed_height: set(raw.flags, 5),
            layout_only: set(raw.flags, 6),
            inert: set(raw.flags, 7),
            width_percent: set(raw.flags, 9),
            height_percent: set(raw.flags, 10),
        })
    }
}

/// Optional min and max logical extents. Some(0.) sets an explicit zero.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct IconConstraints {
    pub min_width: Option<f32>,
    pub min_height: Option<f32>,
    pub max_width: Option<f32>,
    pub max_height: Option<f32>,
}

impl IconConstraints {
    fn encode(self) -> RawConstraints {
        RawConstraints {
            flags: flag(self.min_width.is_some(), 0)
                | flag(self.min_height.is_some(), 1)
                | flag(self.max_width.is_some(), 2)
                | flag(self.max_height.is_some(), 3),
            min_width: self.min_width.unwrap_or_default(),
            min_height: self.min_height.unwrap_or_default(),
            max_width: self.max_width.unwrap_or_default(),
            max_height: self.max_height.unwrap_or_default(),
            ..RawConstraints::with_header()
        }
    }

    fn decode(raw: RawConstraints) -> Self {
        Self {
            min_width: set(raw.flags, 0).then_some(raw.min_width),
            min_height: set(raw.flags, 1).then_some(raw.min_height),
            max_width: set(raw.flags, 2).then_some(raw.max_width),
            max_height: set(raw.flags, 3).then_some(raw.max_height),
        }
    }
}

/// Per-node appearance. None inherits native appearance; Some specifies a
/// value even when it is zero or transparent black. Colors use 0xRRGGBBAA.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct IconOptions {
    pub size: IconSize,
    pub design_size: DesignSize,
    pub inherited_text_size: f32,
    pub width: Option<f32>,
    pub height: Option<f32>,
    pub opacity: Option<f32>,
    pub rem_size: f32,
    pub color_rgba: Option<u32>,
    pub inherited_color_rgba: Option<u32>,
    pub transform: Option<IconTransform>,
    pub decorative: bool,
    pub style: Option<IconBoxStyle>,
    pub constraints: Option<IconConstraints>,
    pub debug_placeholder: bool,
}

impl Default for IconOptions {
    fn default() -> Self {
        Self {
            size: IconSize::Inherited,
            design_size: Size::Inherit,
            inherited_text_size: 0.,
            width: None,
            height: None,
            opacity: None,
            rem_size: 0.,
            color_rgba: None,
            inherited_color_rgba: None,
            transform: None,
            decorative: false,
            style: None,
            constraints: None,
            debug_placeholder: false,
        }
    }
}

impl IconOptions {
    pub(super) fn encode(self) -> RawOptions {
        let (icon_size, custom_size) = match self.size {
            IconSize::Inherited => (0, 0.),
            IconSize::ExtraSmall => (1, 0.),
            IconSize::Small => (2, 0.),
            IconSize::Medium => (3, 0.),
            IconSize::Large => (4, 0.),
            IconSize::Custom(size) => (5, size),
        };
        RawOptions {
            flags: flag(self.width.is_some(), 0)
                | flag(self.height.is_some(), 1)
                | flag(self.color_rgba.is_some(), 2)
                | flag(self.inherited_color_rgba.is_some(), 3)
                | flag(self.opacity.is_some(), 4)
                | flag(self.transform.is_some(), 5)
                | flag(self.decorative, 6)
                | flag(self.style.is_some(), 7)
                | flag(self.constraints.is_some(), 8)
                | flag(self.debug_placeholder, 9),
            icon_size,
            design_size: self.design_size as u32,
            custom_size,
            inherited_text_size: self.inherited_text_size,
            width: self.width.unwrap_or_default(),
            height: self.height.unwrap_or_default(),
            opacity: self.opacity.unwrap_or_default(),
            rem_size: self.rem_size,
            color_rgba: self.color_rgba.unwrap_or_default(),
            inherited_color_rgba: self.inherited_color_rgba.unwrap_or_default(),
            transform: self.transform.map_or([0.; 6], |transform| transform.0),
            style: self.style.map(IconBoxStyle::encode).unwrap_or_default(),
            constraints: self
                .constraints
                .map(IconConstraints::encode)
                .unwrap_or_default(),
            ..RawOptions::with_header()
        }
    }

    pub(super) fn decode(raw: RawOptions) -> Result<Self> {
        let size = match raw.icon_size {
            0 => IconSize::Inherited,
            1 => IconSize::ExtraSmall,
            2 => IconSize::Small,
            3 => IconSize::Medium,
            4 => IconSize::Large,
            5 => IconSize::Custom(raw.custom_size),
            _ => return Err(INVALID),
        };
        let config = Self {
            size,
            design_size: design_size(raw.design_size)?,
            inherited_text_size: raw.inherited_text_size,
            width: set(raw.flags, 0).then_some(raw.width),
            height: set(raw.flags, 1).then_some(raw.height),
            opacity: set(raw.flags, 4).then_some(raw.opacity),
            rem_size: raw.rem_size,
            color_rgba: set(raw.flags, 2).then_some(raw.color_rgba),
            inherited_color_rgba: set(raw.flags, 3).then_some(raw.inherited_color_rgba),
            transform: set(raw.flags, 5).then_some(IconTransform(raw.transform)),
            decorative: set(raw.flags, 6),
            style: set(raw.flags, 7)
                .then(|| IconBoxStyle::decode(raw.style))
                .transpose()?,
            constraints: set(raw.flags, 8).then(|| IconConstraints::decode(raw.constraints)),
            debug_placeholder: set(raw.flags, 9),
        };
        // Confirm record headers, reserved fields, flags and absent values.
        if config.encode() != raw {
            return Err(INVALID);
        }
        Ok(config)
    }
}

#[derive(Clone, Debug, PartialEq)]
pub struct IconConfig {
    pub resource: IconResource,
    pub options: IconOptions,
    pub name: String,
}

impl IconConfig {
    /// Make an Icon using the theme's size and color with no accessible name.
    pub fn new(resource: IconResource) -> Self {
        Self {
            resource,
            options: IconOptions::default(),
            name: String::new(),
        }
    }
}

/// Copy of the latest invocation's artwork status and overall frame status.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct IconResult {
    pub frame_sequence: u64,
    pub frame_status: u32,
    pub icon_error: IconError,
}

pub(super) fn frame_status_valid(status: u32) -> bool {
    status <= 10 // CUI_BRIDGE_CORE_ERROR is the final V1 status.
}

impl App {
    /// Check the retained presentation group and the owned resource group.
    pub fn supports_icon_presentation(&self) -> bool {
        self.supports_icon_resources() && icon_api().is_some()
    }

    /// Create and configure an SVG Icon in the retained tree. Failed
    /// configuration releases the newly created node and leaves the key free.
    pub fn configured_icon(
        &mut self,
        parent: Node,
        key: &str,
        config: &IconConfig,
    ) -> Result<Node> {
        if !self.supports_icon_presentation() {
            return Err(UNAVAILABLE);
        }
        self.icon_resource_handle(config.resource)?;
        let node = self.child(parent, Kind::Icon, key)?;
        if let Err(error) = self.configure_icon(node, config) {
            self.remove(node)?;
            return Err(error);
        }
        Ok(node)
    }

    /// Atomically replace the retained SVG and its presentation options.
    pub fn configure_icon(&mut self, node: Node, config: &IconConfig) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let handle = self.icon_resource_handle(config.resource)?;
        let api = icon_api()
            .filter(|_| self.supports_icon_resources())
            .ok_or(UNAVAILABLE)?;
        let (name, length) = string_span(&config.name, MAX_TEXT_BYTES)?;
        let options = config.options.encode();
        check(unsafe { (api.configure)(self.raw, node, handle, &options, name, length) })?;
        Ok(self)
    }

    /// Clear the SVG presentation and restore the legacy named Icon route.
    pub fn reset_icon(&mut self, node: Node) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let api = icon_api().ok_or(UNAVAILABLE)?;
        check(unsafe { (api.reset)(self.raw, node) })?;
        Ok(self)
    }

    /// Copy the current options and name. A legacy Icon has no configuration.
    pub fn icon_config(&self, node: Node) -> Result<Option<IconConfig>> {
        let node = self.valid(node)?;
        let api = icon_api().ok_or(UNAVAILABLE)?;
        let mut handle = RawHandle::default();
        let mut options = RawOptions::with_header();
        let status = unsafe { (api.config)(self.raw, node, &mut handle, &mut options) };
        if status == EMPTY {
            return Ok(None);
        }
        check(status)?;
        if handle.slot == 0 || handle.generation == 0 {
            return Err(INVALID);
        }
        let name = copy_string(MAX_TEXT_BYTES, false, |out, capacity, required| unsafe {
            (api.name)(self.raw, node, out, capacity, required)
        })?
        .ok_or(INVALID)?;
        Ok(Some(IconConfig {
            resource: IconResource {
                raw: handle,
                app_id: self.id,
            },
            options: IconOptions::decode(options)?,
            name,
        }))
    }

    /// Read an Icon result without preparing artwork or replaying events.
    pub fn icon_result(&self, node: Node) -> Result<Option<IconResult>> {
        let node = self.valid(node)?;
        let api = icon_api().ok_or(UNAVAILABLE)?;
        let mut raw = RawIconResult::with_header();
        let status = unsafe { (api.result)(self.raw, node, &mut raw) };
        if status == EMPTY {
            return Ok(None);
        }
        check(status)?;
        if raw.size != RawIconResult::SIZE
            || raw.version != 1
            || raw.reserved != 0
            || !frame_status_valid(raw.frame_status)
            || raw.node != node
            || raw.frame_sequence == 0
        {
            return Err(INVALID);
        }
        Ok(Some(IconResult {
            frame_sequence: raw.frame_sequence,
            frame_status: raw.frame_status,
            icon_error: raw.icon_error.try_into()?,
        }))
    }
}

impl<'a> Builder<'a> {
    /// Attach owned SVG to an Icon created through the generic builder.
    pub fn icon_presentation(self, config: &IconConfig) -> Result<Self> {
        self.app.configure_icon(self.node, config)?;
        Ok(self)
    }
}
