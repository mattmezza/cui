// SPDX-License-Identifier: MIT
//! Retained Badge composition and copied result/bounds queries.
use super::ffi::RawHandle;
use super::presentation::{design_size, frame_status_valid, IconOptions};
use super::presentation_ffi::{badge_api, RawBadgeConfig, RawBadgeResult, RawOptions};
use super::{
    copy_string, string_span, IconError, IconResource, EMPTY, INVALID, MAX_TEXT_BYTES, UNAVAILABLE,
};
use crate::{check, string, App, DesignSize, Node, RawNode, Rect, Result, Size};
use std::ptr::null;

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum BadgeSize {
    #[default]
    Medium,
    Small,
    Large,
    ExtraSmall,
}

impl TryFrom<u32> for BadgeSize {
    type Error = crate::Error;

    fn try_from(raw: u32) -> Result<Self> {
        match raw {
            0 => Ok(Self::Medium),
            1 => Ok(Self::Small),
            2 => Ok(Self::Large),
            3 => Ok(Self::ExtraSmall),
            _ => Err(INVALID),
        }
    }
}

/// Indicator content. Zero count hides the indicator. A count greater
/// than the maximum uses the maximum followed by a plus.
#[derive(Clone, Debug, PartialEq)]
pub enum BadgeVariant {
    Count {
        value: u64,
        maximum: u64,
    },
    Dot,
    NamedIcon(String),
    Artwork {
        resource: IconResource,
        options: IconOptions,
    },
}

/// Appearance and accessible name of the indicator. The children are
/// independently authored retained controls.
#[derive(Clone, Debug, PartialEq)]
pub struct BadgeConfig {
    pub variant: BadgeVariant,
    pub size: BadgeSize,
    pub design_size: DesignSize,
    pub color_rgba: Option<u32>,
    pub name: String,
}

impl Default for BadgeConfig {
    fn default() -> Self {
        Self::count(0)
    }
}

impl BadgeConfig {
    /// Use the medium, theme-colored count indicator with a maximum of 99.
    pub fn count(value: u64) -> Self {
        Self {
            variant: BadgeVariant::Count { value, maximum: 99 },
            size: BadgeSize::Medium,
            design_size: Size::Inherit,
            color_rgba: None,
            name: String::new(),
        }
    }
}

/// A copied result from the latest frame in which the Badge was invoked.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct BadgeResult {
    pub frame_sequence: u64,
    pub frame_status: u32,
    pub submitted: bool,
    /// Exact preparation result for owned artwork. Named Icons have no receipt.
    pub icon_error: Option<IconError>,
}

struct BadgeInput<'a> {
    raw: RawBadgeConfig,
    artwork: Option<RawOptions>,
    name: &'a str,
    named_icon: &'a str,
}

impl<'a> BadgeInput<'a> {
    fn encode(app: &App, config: &'a BadgeConfig) -> Result<Self> {
        let mut raw = RawBadgeConfig {
            control_size: config.size as u32,
            design_size: config.design_size as u32,
            flags: u32::from(config.color_rgba.is_some()),
            color_rgba: config.color_rgba.unwrap_or_default(),
            ..RawBadgeConfig::with_header()
        };
        let mut artwork = None;
        let mut named_icon = "";
        match &config.variant {
            BadgeVariant::Count { value, maximum } => {
                usize::try_from(*value).map_err(|_| INVALID)?;
                usize::try_from(*maximum).map_err(|_| INVALID)?;
                raw.count = *value;
                raw.maximum = *maximum;
            }
            BadgeVariant::Dot => raw.variant = 1,
            BadgeVariant::NamedIcon(icon) => {
                if icon.is_empty() {
                    return Err(INVALID);
                }
                raw.variant = 2;
                named_icon = icon;
            }
            BadgeVariant::Artwork { resource, options } => {
                raw.artwork = app.icon_resource_handle(*resource)?;
                if !app.supports_icon_resources() {
                    return Err(UNAVAILABLE);
                }
                raw.variant = 2;
                artwork = Some(options.encode());
            }
        }
        Ok(Self {
            raw,
            artwork,
            name: &config.name,
            named_icon,
        })
    }

    fn artwork_ptr(&self) -> *const RawOptions {
        self.artwork.as_ref().map_or(null(), std::ptr::from_ref)
    }
}

fn decode_badge(
    app: &App,
    raw: RawBadgeConfig,
    artwork: RawOptions,
    name: String,
    named_icon: String,
) -> Result<BadgeConfig> {
    let has_artwork = raw.artwork != RawHandle::default();
    let variant = match raw.variant {
        0 if !has_artwork && named_icon.is_empty() => BadgeVariant::Count {
            value: raw.count,
            maximum: raw.maximum,
        },
        1 if !has_artwork && named_icon.is_empty() => BadgeVariant::Dot,
        2 if !has_artwork && !named_icon.is_empty() => BadgeVariant::NamedIcon(named_icon),
        2 if raw.artwork.slot != 0 && raw.artwork.generation != 0 && named_icon.is_empty() => {
            BadgeVariant::Artwork {
                resource: IconResource {
                    raw: raw.artwork,
                    app_id: app.id,
                },
                options: IconOptions::decode(artwork)?,
            }
        }
        _ => return Err(INVALID),
    };
    let config = BadgeConfig {
        variant,
        size: raw.control_size.try_into()?,
        design_size: design_size(raw.design_size)?,
        color_rgba: (raw.flags & 1 != 0).then_some(raw.color_rgba),
        name,
    };
    let encoded = BadgeInput::encode(app, &config)?;
    if encoded.raw != raw || encoded.artwork.unwrap_or_else(RawOptions::with_header) != artwork {
        return Err(INVALID);
    }
    Ok(config)
}

impl App {
    /// Check all Badge V1 exports and the nested Icon options record size.
    pub fn supports_configured_badge(&self) -> bool {
        badge_api().is_some()
    }

    /// Read the loaded bridge's default indicator config.
    pub fn default_badge_config(&self) -> Result<BadgeConfig> {
        let api = badge_api().ok_or(UNAVAILABLE)?;
        let mut raw = RawBadgeConfig::with_header();
        check(unsafe { (api.default_config)(&mut raw) })?;
        decode_badge(
            self,
            raw,
            RawOptions::with_header(),
            String::new(),
            String::new(),
        )
    }

    /// Create a Badge container atomically. Its children participate in the
    /// ordinary retained tree and keep their own callbacks.
    pub fn configured_badge(
        &mut self,
        parent: Node,
        key: &str,
        config: &BadgeConfig,
    ) -> Result<Node> {
        let parent = self.valid(parent)?;
        let api = badge_api().ok_or(UNAVAILABLE)?;
        let input = BadgeInput::encode(self, config)?;
        let key = string(key)?;
        let (name, name_len) = string_span(input.name, MAX_TEXT_BYTES)?;
        let (named_icon, icon_len) = string_span(input.named_icon, MAX_TEXT_BYTES)?;
        let mut raw = RawNode::default();
        check(unsafe {
            (api.create)(
                self.raw,
                parent,
                key.as_ptr(),
                &input.raw,
                input.artwork_ptr(),
                name,
                name_len,
                named_icon,
                icon_len,
                &mut raw,
            )
        })?;
        Ok(Node {
            raw,
            app_id: self.id,
        })
    }

    /// Replace the indicator configuration while preserving child nodes.
    pub fn configure_badge(&mut self, node: Node, config: &BadgeConfig) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let api = badge_api().ok_or(UNAVAILABLE)?;
        let input = BadgeInput::encode(self, config)?;
        let (name, name_len) = string_span(input.name, MAX_TEXT_BYTES)?;
        let (named_icon, icon_len) = string_span(input.named_icon, MAX_TEXT_BYTES)?;
        check(unsafe {
            (api.configure)(
                self.raw,
                node,
                &input.raw,
                input.artwork_ptr(),
                name,
                name_len,
                named_icon,
                icon_len,
            )
        })?;
        Ok(self)
    }

    /// Copy the current indicator options. Any returned resource is app-owned.
    pub fn badge_config(&self, node: Node) -> Result<BadgeConfig> {
        let node = self.valid(node)?;
        let api = badge_api().ok_or(UNAVAILABLE)?;
        let mut raw = RawBadgeConfig::with_header();
        let mut artwork = RawOptions::with_header();
        check(unsafe { (api.config)(self.raw, node, &mut raw, &mut artwork) })?;
        let text = |field| {
            copy_string(MAX_TEXT_BYTES, false, |out, capacity, required| unsafe {
                (api.text)(self.raw, node, field, out, capacity, required)
            })?
            .ok_or(INVALID)
        };
        decode_badge(self, raw, artwork, text(0)?, text(1)?)
    }

    /// Read the current Badge result without dispatching child events.
    pub fn badge_result(&self, node: Node) -> Result<Option<BadgeResult>> {
        let node = self.valid(node)?;
        let api = badge_api().ok_or(UNAVAILABLE)?;
        let mut raw = RawBadgeResult::with_header();
        let status = unsafe { (api.result)(self.raw, node, &mut raw) };
        if status == EMPTY {
            return Ok(None);
        }
        check(status)?;
        if raw.size != RawBadgeResult::SIZE
            || raw.version != 1
            || raw.node != node
            || raw.frame_sequence == 0
            || raw.submitted > 1
            || raw.flags > 1
            || !frame_status_valid(raw.frame_status)
            || (raw.flags == 0 && raw.icon_error != 0)
        {
            return Err(INVALID);
        }
        Ok(Some(BadgeResult {
            frame_sequence: raw.frame_sequence,
            frame_status: raw.frame_status,
            submitted: raw.submitted != 0,
            icon_error: (raw.flags != 0)
                .then(|| IconError::try_from(raw.icon_error))
                .transpose()?,
        }))
    }

    /// Indicator rectangle from the latest successful frame, when visible.
    pub fn badge_indicator_bounds(&self, node: Node) -> Result<Option<Rect>> {
        let node = self.valid(node)?;
        let api = badge_api().ok_or(UNAVAILABLE)?;
        let mut rect = Rect::default();
        let status = unsafe { (api.bounds)(self.raw, node, &mut rect) };
        if status == EMPTY {
            return Ok(None);
        }
        check(status)?;
        if ![rect.x, rect.y, rect.width, rect.height]
            .iter()
            .all(|&coord| coord.is_finite())
            || rect.width <= 0.
            || rect.height <= 0.
        {
            return Err(INVALID);
        }
        Ok(Some(rect))
    }
}
