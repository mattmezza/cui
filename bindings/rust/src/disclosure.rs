// SPDX-License-Identifier: MIT
//! Optional retained Accordion and Collapsible presentation.
use crate::{check, App, Builder, DesignSize, Error, Node, RawNode, Result};
use std::{ffi::c_void, mem, ptr, sync::OnceLock};

/// The C bridge copies these options. Width -1 fills, 0 fits and a positive
/// value sets logical pixels. Empty icon names use chevrons.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct DisclosureConfig<'a> {
    pub design_size: DesignSize,
    pub width: f32,
    pub collapsed_icon: &'a str,
    pub expanded_icon: &'a str,
    /// Accordion only.
    pub joined: bool,
    /// Accordion only.
    pub bordered: bool,
    /// Accordion only. Opening an item closes its siblings.
    pub single: bool,
    /// Accordion only. Activation can close an expanded item.
    pub allow_collapse: bool,
    /// Collapsible only.
    pub borderless: bool,
    pub card: Option<DisclosureCard>,
}

impl Default for DisclosureConfig<'_> {
    fn default() -> Self {
        Self {
            design_size: DesignSize::Inherit,
            width: 0.,
            collapsed_icon: "",
            expanded_icon: "",
            joined: false,
            bordered: false,
            single: false,
            allow_collapse: false,
            borderless: false,
            card: None,
        }
    }
}

/// Outer box override. Colors use numeric 0xRRGGBBAA and dimensions use logical pixels.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct DisclosureCard {
    pub background_rgba: u32,
    pub border_rgba: u32,
    pub radius: f32,
    pub border_width: f32,
    pub padding: f32,
    pub gap: f32,
    pub clip: bool,
}

#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DisclosureCursor {
    Default,
    Pointer,
    Text,
    ResizeHorizontal,
    ResizeVertical,
    Move,
    NotAllowed,
    Crosshair,
    Grab,
    Grabbing,
    ZoomIn,
    ZoomOut,
    Wait,
    Help,
}

/// Per-trigger overrides for a Collapsible or a direct Accordion Item.
/// None inherits and Some(0) selects an explicit transparent color.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct DisclosureTriggerStyle {
    pub hover_rgba: Option<u32>,
    pub pressed_rgba: Option<u32>,
    pub expanded_rgba: Option<u32>,
    pub disabled_rgba: Option<u32>,
    pub focus_rgba: Option<u32>,
    pub cursor: Option<DisclosureCursor>,
}

#[repr(u32)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum DisclosureHeadingLayer {
    #[default]
    Normal,
    Expanded,
    Disabled,
}

#[repr(u32)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum DisclosureWeight {
    #[default]
    Normal,
    Medium,
    Semibold,
    Bold,
}

#[repr(u32)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum DisclosureAlignment {
    #[default]
    Left,
    Center,
    Right,
}

/// Title metrics in logical pixels. Set a positive font_size. Zero line_height
/// uses natural metrics. Nonzero letter_spacing requires host support.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct DisclosureTypography {
    pub font_size: f32,
    pub line_height: f32,
    pub letter_spacing: f32,
    pub weight: DisclosureWeight,
    pub align: DisclosureAlignment,
    pub wrap: bool,
    pub italic: bool,
    pub monospace: bool,
}

/// Complete, vertically centered trigger row. Width -1 fills, and zero
/// width or height fits.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct DisclosureHeadingBox {
    pub width: f32,
    pub height: f32,
    pub padding: f32,
    pub gap: f32,
    pub radius: f32,
    pub border_width: f32,
    pub background_rgba: u32,
    pub border_rgba: u32,
}

/// One style layer. None inherits and Some(0) selects a transparent title.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct DisclosureHeading {
    pub box_style: Option<DisclosureHeadingBox>,
    pub typography: Option<DisclosureTypography>,
    pub title_rgba: Option<u32>,
}

/// A joined Accordion separator. Zero thickness removes its layout space.
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct DisclosureDivider {
    pub thickness: f32,
    pub inset_left: f32,
    pub inset_right: f32,
    pub color_rgba: u32,
}

const JOINED: u32 = 1 << 0;
const BORDERED: u32 = 1 << 1;
const SINGLE: u32 = 1 << 2;
const ALLOW_COLLAPSE: u32 = 1 << 3;
const CUSTOM_CARD: u32 = 1 << 4;
const CLIP_CARD: u32 = 1 << 5;
const BORDERLESS: u32 = 1 << 6;

#[repr(C)]
struct ConfigRaw {
    struct_size: u32,
    schema_version: u32,
    flags: u32,
    design_size: u32,
    width: f32,
    card_radius: f32,
    card_border_width: f32,
    card_padding: f32,
    card_gap: f32,
    card_background_rgba: u32,
    card_border_rgba: u32,
    collapsed_icon: [u8; 64],
    expanded_icon: [u8; 64],
}

#[repr(C)]
struct StyleRaw {
    struct_size: u32,
    schema_version: u32,
    fields: u32,
    reserved: u32,
    hover_rgba: u32,
    pressed_rgba: u32,
    expanded_rgba: u32,
    disabled_rgba: u32,
    focus_rgba: u32,
    cursor: u32,
}

#[repr(C)]
#[derive(Default)]
struct HeadingRaw {
    struct_size: u32,
    schema_version: u32,
    fields: u32,
    reserved: u32,
    font_size: f32,
    line_height: f32,
    letter_spacing: f32,
    weight: u32,
    align: u32,
    text_flags: u32,
    title_rgba: u32,
    width: f32,
    height: f32,
    padding: f32,
    gap: f32,
    radius: f32,
    border_width: f32,
    background_rgba: u32,
    border_rgba: u32,
}

#[repr(C)]
struct DividerRaw {
    struct_size: u32,
    schema_version: u32,
    reserved: [u32; 2],
    thickness: f32,
    inset_left: f32,
    inset_right: f32,
    color_rgba: u32,
}

const _: () = {
    assert!(mem::size_of::<ConfigRaw>() == 172);
    assert!(mem::offset_of!(ConfigRaw, width) == 16);
    assert!(mem::offset_of!(ConfigRaw, collapsed_icon) == 44);
    assert!(mem::size_of::<StyleRaw>() == 40);
    assert!(mem::offset_of!(StyleRaw, cursor) == 36);
    assert!(mem::size_of::<HeadingRaw>() == 76);
    assert!(mem::offset_of!(HeadingRaw, font_size) == 16);
    assert!(mem::offset_of!(HeadingRaw, width) == 44);
    assert!(mem::size_of::<DividerRaw>() == 32);
    assert!(mem::offset_of!(DividerRaw, thickness) == 16);
};

fn dimension(value: f32) -> bool {
    (0. ..=4096.).contains(&value)
}

fn icon_name(name: &str) -> Result<[u8; 64]> {
    let value = name.as_bytes();
    if value.len() >= 64 || value.contains(&0) {
        return Err(Error(3));
    }
    let mut bytes = [0; 64];
    bytes[..value.len()].copy_from_slice(value);
    Ok(bytes)
}

impl DisclosureConfig<'_> {
    fn wire(&self) -> Result<ConfigRaw> {
        let card = self.card.unwrap_or_default();
        if !(self.width == -1. || dimension(self.width))
            || ![card.radius, card.border_width, card.padding, card.gap]
                .into_iter()
                .all(dimension)
        {
            return Err(Error(3));
        }
        Ok(ConfigRaw {
            struct_size: mem::size_of::<ConfigRaw>() as u32,
            schema_version: 1,
            flags: (u32::from(self.joined) * JOINED)
                | (u32::from(self.bordered) * BORDERED)
                | (u32::from(self.single) * SINGLE)
                | (u32::from(self.allow_collapse) * ALLOW_COLLAPSE)
                | (u32::from(self.card.is_some()) * CUSTOM_CARD)
                | (u32::from(card.clip) * CLIP_CARD)
                | (u32::from(self.borderless) * BORDERLESS),
            design_size: self.design_size as u32,
            width: self.width,
            card_radius: card.radius,
            card_border_width: card.border_width,
            card_padding: card.padding,
            card_gap: card.gap,
            card_background_rgba: card.background_rgba,
            card_border_rgba: card.border_rgba,
            collapsed_icon: icon_name(self.collapsed_icon)?,
            expanded_icon: icon_name(self.expanded_icon)?,
        })
    }
}

impl DisclosureTriggerStyle {
    fn wire(&self) -> StyleRaw {
        StyleRaw {
            struct_size: mem::size_of::<StyleRaw>() as u32,
            schema_version: 1,
            fields: u32::from(self.hover_rgba.is_some())
                | (u32::from(self.pressed_rgba.is_some()) << 1)
                | (u32::from(self.expanded_rgba.is_some()) << 2)
                | (u32::from(self.disabled_rgba.is_some()) << 3)
                | (u32::from(self.focus_rgba.is_some()) << 4)
                | (u32::from(self.cursor.is_some()) << 5),
            reserved: 0,
            hover_rgba: self.hover_rgba.unwrap_or_default(),
            pressed_rgba: self.pressed_rgba.unwrap_or_default(),
            expanded_rgba: self.expanded_rgba.unwrap_or_default(),
            disabled_rgba: self.disabled_rgba.unwrap_or_default(),
            focus_rgba: self.focus_rgba.unwrap_or_default(),
            cursor: self.cursor.unwrap_or(DisclosureCursor::Default) as u32,
        }
    }
}

impl DisclosureHeading {
    fn wire(&self) -> Result<HeadingRaw> {
        let mut heading = HeadingRaw {
            struct_size: mem::size_of::<HeadingRaw>() as u32,
            schema_version: 1,
            ..Default::default()
        };
        if let Some(b) = self.box_style {
            if !(b.width == -1. || dimension(b.width))
                || ![b.height, b.padding, b.gap, b.radius, b.border_width]
                    .into_iter()
                    .all(dimension)
            {
                return Err(Error(3));
            }
            heading.fields |= 1 << 0;
            heading.width = b.width;
            heading.height = b.height;
            heading.padding = b.padding;
            heading.gap = b.gap;
            heading.radius = b.radius;
            heading.border_width = b.border_width;
            heading.background_rgba = b.background_rgba;
            heading.border_rgba = b.border_rgba;
        }
        if let Some(t) = self.typography {
            if t.font_size <= 0.
                || !dimension(t.font_size)
                || !dimension(t.line_height)
                || !dimension(t.letter_spacing.abs())
            {
                return Err(Error(3));
            }
            heading.fields |= 1 << 1;
            heading.font_size = t.font_size;
            heading.line_height = t.line_height;
            heading.letter_spacing = t.letter_spacing;
            heading.weight = t.weight as u32;
            heading.align = t.align as u32;
            heading.text_flags =
                u32::from(t.wrap) | (u32::from(t.italic) << 1) | (u32::from(t.monospace) << 2);
        }
        if let Some(rgba) = self.title_rgba {
            heading.fields |= 1 << 2;
            heading.title_rgba = rgba;
        }
        Ok(heading)
    }
}

impl DisclosureDivider {
    fn wire(&self) -> Result<DividerRaw> {
        if ![self.thickness, self.inset_left, self.inset_right]
            .into_iter()
            .all(dimension)
        {
            return Err(Error(3));
        }
        Ok(DividerRaw {
            struct_size: mem::size_of::<DividerRaw>() as u32,
            schema_version: 1,
            reserved: [0; 2],
            thickness: self.thickness,
            inset_left: self.inset_left,
            inset_right: self.inset_right,
            color_rgba: self.color_rgba,
        })
    }
}

type Configure = unsafe extern "C" fn(*mut c_void, RawNode, *const ConfigRaw) -> u32;
type SetStyle = unsafe extern "C" fn(*mut c_void, RawNode, *const StyleRaw) -> u32;
type SetHeading = unsafe extern "C" fn(*mut c_void, RawNode, u32, *const HeadingRaw) -> u32;
type SetDivider = unsafe extern "C" fn(*mut c_void, RawNode, *const DividerRaw) -> u32;

#[derive(Default)]
struct DisclosureApi {
    presentation: Option<(Configure, SetStyle)>,
    heading: Option<SetHeading>,
    divider: Option<SetDivider>,
}

fn api() -> &'static DisclosureApi {
    static API: OnceLock<DisclosureApi> = OnceLock::new();
    API.get_or_init(resolve)
}

#[cfg(cui_bridge_static)]
fn resolve() -> DisclosureApi {
    extern "C" {
        fn cui_bridge_disclosure_configure_v1(
            a: *mut c_void,
            n: RawNode,
            c: *const ConfigRaw,
        ) -> u32;
        fn cui_bridge_disclosure_set_trigger_style_v1(
            a: *mut c_void,
            n: RawNode,
            s: *const StyleRaw,
        ) -> u32;
        fn cui_bridge_disclosure_set_heading_v1(
            a: *mut c_void,
            n: RawNode,
            layer: u32,
            heading: *const HeadingRaw,
        ) -> u32;
        fn cui_bridge_disclosure_set_divider_v1(
            a: *mut c_void,
            n: RawNode,
            divider: *const DividerRaw,
        ) -> u32;
    }
    DisclosureApi {
        presentation: Some((
            cui_bridge_disclosure_configure_v1,
            cui_bridge_disclosure_set_trigger_style_v1,
        )),
        heading: Some(cui_bridge_disclosure_set_heading_v1),
        divider: Some(cui_bridge_disclosure_set_divider_v1),
    }
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve() -> DisclosureApi {
    unsafe {
        let config = crate::notification_symbol_v1(c"cui_bridge_disclosure_configure_v1".as_ptr());
        let style =
            crate::notification_symbol_v1(c"cui_bridge_disclosure_set_trigger_style_v1".as_ptr());
        let heading =
            crate::notification_symbol_v1(c"cui_bridge_disclosure_set_heading_v1".as_ptr());
        let divider =
            crate::notification_symbol_v1(c"cui_bridge_disclosure_set_divider_v1".as_ptr());
        DisclosureApi {
            presentation: (!config.is_null() && !style.is_null()).then(|| {
                (
                    mem::transmute::<*mut c_void, Configure>(config),
                    mem::transmute::<*mut c_void, SetStyle>(style),
                )
            }),
            heading: (!heading.is_null())
                .then(|| mem::transmute::<*mut c_void, SetHeading>(heading)),
            divider: (!divider.is_null())
                .then(|| mem::transmute::<*mut c_void, SetDivider>(divider)),
        }
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> DisclosureApi {
    DisclosureApi::default()
}

impl Error {
    /// The provider lacks the requested retained disclosure operation.
    pub const DISCLOSURE_OPTIONS_UNAVAILABLE: Self = Self(9);
}

impl App {
    /// Probe both Disclosure V1 symbols. Older providers retain ordinary controls.
    pub fn supports_disclosure_presentation(&self) -> bool {
        api().presentation.is_some()
    }

    /// Copy Accordion or Collapsible options, or clear them with None.
    pub fn configure_disclosure(
        &mut self,
        node: Node,
        config: Option<&DisclosureConfig<'_>>,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let (configure, _) = api()
            .presentation
            .ok_or(Error::DISCLOSURE_OPTIONS_UNAVAILABLE)?;
        let raw = config.map(DisclosureConfig::wire).transpose()?;
        check(unsafe {
            configure(
                self.raw,
                node,
                raw.as_ref().map_or(ptr::null(), ptr::from_ref),
            )
        })?;
        Ok(self)
    }

    /// Copy trigger feedback on a Collapsible or direct Accordion Item.
    /// None clears feedback while keeping configuration and expansion.
    pub fn set_disclosure_trigger_style(
        &mut self,
        node: Node,
        style: Option<&DisclosureTriggerStyle>,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let (_, set_style) = api()
            .presentation
            .ok_or(Error::DISCLOSURE_OPTIONS_UNAVAILABLE)?;
        let raw = style.map(DisclosureTriggerStyle::wire);
        check(unsafe {
            set_style(
                self.raw,
                node,
                raw.as_ref().map_or(ptr::null(), ptr::from_ref),
            )
        })?;
        Ok(self)
    }

    /// Probe the optional retained heading setter independently.
    pub fn supports_disclosure_heading(&self) -> bool {
        api().heading.is_some()
    }

    /// Probe the optional joined divider setter independently.
    pub fn supports_disclosure_divider(&self) -> bool {
        api().divider.is_some()
    }

    /// Copy one heading layer onto Accordion, Collapsible or a direct Accordion
    /// Item. None or a style with no fields clears that layer.
    pub fn set_disclosure_heading(
        &mut self,
        node: Node,
        layer: DisclosureHeadingLayer,
        style: Option<&DisclosureHeading>,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let set_heading = api().heading.ok_or(Error::DISCLOSURE_OPTIONS_UNAVAILABLE)?;
        let raw = style.map(DisclosureHeading::wire).transpose()?;
        check(unsafe {
            set_heading(
                self.raw,
                node,
                layer as u32,
                raw.as_ref().map_or(ptr::null(), ptr::from_ref),
            )
        })?;
        Ok(self)
    }

    /// Copy a joined Accordion divider. None restores the themed divider.
    pub fn set_disclosure_divider(
        &mut self,
        node: Node,
        style: Option<&DisclosureDivider>,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let set_divider = api().divider.ok_or(Error::DISCLOSURE_OPTIONS_UNAVAILABLE)?;
        let raw = style.map(DisclosureDivider::wire).transpose()?;
        check(unsafe {
            set_divider(
                self.raw,
                node,
                raw.as_ref().map_or(ptr::null(), ptr::from_ref),
            )
        })?;
        Ok(self)
    }
}

impl Builder<'_> {
    /// Apply presentation to the mounted child during a builder chain.
    pub fn disclosure_options(self, config: &DisclosureConfig<'_>) -> Result<Self> {
        self.app.configure_disclosure(self.node, Some(config))?;
        Ok(self)
    }

    /// Apply trigger feedback to the mounted child during a builder chain.
    pub fn disclosure_style(self, style: &DisclosureTriggerStyle) -> Result<Self> {
        self.app
            .set_disclosure_trigger_style(self.node, Some(style))?;
        Ok(self)
    }

    /// Apply heading style to the mounted child during a builder chain.
    pub fn disclosure_heading(
        self,
        layer: DisclosureHeadingLayer,
        style: &DisclosureHeading,
    ) -> Result<Self> {
        self.app
            .set_disclosure_heading(self.node, layer, Some(style))?;
        Ok(self)
    }

    /// Apply a joined divider to the mounted Accordion during a builder chain.
    pub fn disclosure_divider(self, style: &DisclosureDivider) -> Result<Self> {
        self.app.set_disclosure_divider(self.node, Some(style))?;
        Ok(self)
    }
}
