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

const _: () = {
    assert!(mem::size_of::<ConfigRaw>() == 172);
    assert!(mem::offset_of!(ConfigRaw, width) == 16);
    assert!(mem::offset_of!(ConfigRaw, collapsed_icon) == 44);
    assert!(mem::size_of::<StyleRaw>() == 40);
    assert!(mem::offset_of!(StyleRaw, cursor) == 36);
};

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
        let valid = |value| (0. ..=4096.).contains(&value);
        let card = self.card.unwrap_or_default();
        if !(self.width == -1. || valid(self.width))
            || ![card.radius, card.border_width, card.padding, card.gap]
                .into_iter()
                .all(valid)
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

type Configure = unsafe extern "C" fn(*mut c_void, RawNode, *const ConfigRaw) -> u32;
type SetStyle = unsafe extern "C" fn(*mut c_void, RawNode, *const StyleRaw) -> u32;

fn api() -> Option<(Configure, SetStyle)> {
    static API: OnceLock<Option<(Configure, SetStyle)>> = OnceLock::new();
    *API.get_or_init(resolve)
}

#[cfg(cui_bridge_static)]
fn resolve() -> Option<(Configure, SetStyle)> {
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
    }
    Some((
        cui_bridge_disclosure_configure_v1,
        cui_bridge_disclosure_set_trigger_style_v1,
    ))
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve() -> Option<(Configure, SetStyle)> {
    unsafe {
        let config = crate::notification_symbol_v1(c"cui_bridge_disclosure_configure_v1".as_ptr());
        let style =
            crate::notification_symbol_v1(c"cui_bridge_disclosure_set_trigger_style_v1".as_ptr());
        if config.is_null() || style.is_null() {
            return None;
        }
        Some((
            mem::transmute::<*mut c_void, Configure>(config),
            mem::transmute::<*mut c_void, SetStyle>(style),
        ))
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> Option<(Configure, SetStyle)> {
    None
}

impl Error {
    /// The provider lacks one or both retained disclosure setters.
    pub const DISCLOSURE_OPTIONS_UNAVAILABLE: Self = Self(9);
}

impl App {
    /// Probe both Disclosure V1 symbols. Older providers retain ordinary controls.
    pub fn supports_disclosure_presentation(&self) -> bool {
        api().is_some()
    }

    /// Copy Accordion or Collapsible options, or clear them with None.
    pub fn configure_disclosure(
        &mut self,
        node: Node,
        config: Option<&DisclosureConfig<'_>>,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let (configure, _) = api().ok_or(Error::DISCLOSURE_OPTIONS_UNAVAILABLE)?;
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
        let (_, set_style) = api().ok_or(Error::DISCLOSURE_OPTIONS_UNAVAILABLE)?;
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
}
