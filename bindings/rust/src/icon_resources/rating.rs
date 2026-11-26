// SPDX-License-Identifier: MIT
//! Optional presentation for retained Rating, with shared app-owned SVG masks.
use super::{ffi::RawHandle, presentation::design_size, IconResource};
use crate::{check, App, Builder, DesignSize, Error, Kind, Node, RawNode, Result};
use std::{ffi::c_void, mem::size_of, ptr, sync::OnceLock};

/// The copied Rating presentation. No mask uses the built-in star for that
/// layer. Both masks must come from the app mounting the Rating. An explicit
/// color uses numeric 0xRRGGBBAA.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct RatingConfig {
    pub half_steps: bool,
    pub hover_preview: bool,
    pub step_down_filled: bool,
    pub clearable: bool,
    pub readonly: bool,
    pub design_size: DesignSize,
    /// Logical pixels from 1 through 4096, or zero for the natural width.
    pub width: f32,
    pub fill_rgba: Option<u32>,
    pub outline_rgba: Option<u32>,
    pub preview_rgba: Option<u32>,
    pub outline_icon: Option<IconResource>,
    pub fill_icon: Option<IconResource>,
}

impl Default for RatingConfig {
    fn default() -> Self {
        Self {
            half_steps: false,
            hover_preview: false,
            step_down_filled: false,
            clearable: false,
            readonly: false,
            design_size: DesignSize::Inherit,
            width: 0.,
            fill_rgba: None,
            outline_rgba: None,
            preview_rgba: None,
            outline_icon: None,
            fill_icon: None,
        }
    }
}

const SIZE: u32 = 48;
const VERSION: u32 = 1;
const HALF: u32 = 1 << 0;
const HOVER: u32 = 1 << 1;
const STEP_DOWN: u32 = 1 << 2;
const CLEAR: u32 = 1 << 3;
const READONLY: u32 = 1 << 4;
const FILL: u32 = 1 << 5;
const OUTLINE: u32 = 1 << 6;
const PREVIEW: u32 = 1 << 7;
const FLAGS: u32 = HALF | HOVER | STEP_DOWN | CLEAR | READONLY | FILL | OUTLINE | PREVIEW;

#[repr(C)]
#[derive(Default)]
struct ConfigRaw {
    size: u32,
    version: u32,
    flags: u32,
    design_size: u32,
    width: f32,
    fill_rgba: u32,
    outline_rgba: u32,
    preview_rgba: u32,
    outline_icon: RawHandle,
    fill_icon: RawHandle,
}
const _: [(); SIZE as usize] = [(); size_of::<ConfigRaw>()];

impl ConfigRaw {
    fn with_header() -> Self {
        Self {
            size: SIZE,
            version: VERSION,
            ..Self::default()
        }
    }
}

fn valid_width(width: f32) -> bool {
    width == 0. || (1. ..=4096.).contains(&width)
}

impl RatingConfig {
    fn encode(&self, app: &App) -> Result<ConfigRaw> {
        if !valid_width(self.width) {
            return Err(Error(3));
        }
        let handle = |mask: Option<IconResource>| {
            mask.map_or(Ok(RawHandle::default()), |resource| {
                app.icon_resource_handle(resource)
            })
        };
        Ok(ConfigRaw {
            flags: u32::from(self.half_steps) * HALF
                | u32::from(self.hover_preview) * HOVER
                | u32::from(self.step_down_filled) * STEP_DOWN
                | u32::from(self.clearable) * CLEAR
                | u32::from(self.readonly) * READONLY
                | u32::from(self.fill_rgba.is_some()) * FILL
                | u32::from(self.outline_rgba.is_some()) * OUTLINE
                | u32::from(self.preview_rgba.is_some()) * PREVIEW,
            design_size: self.design_size as u32,
            width: self.width,
            fill_rgba: self.fill_rgba.unwrap_or_default(),
            outline_rgba: self.outline_rgba.unwrap_or_default(),
            preview_rgba: self.preview_rgba.unwrap_or_default(),
            outline_icon: handle(self.outline_icon)?,
            fill_icon: handle(self.fill_icon)?,
            ..ConfigRaw::with_header()
        })
    }

    fn decode(raw: ConfigRaw, app: &App) -> Result<Self> {
        if raw.size != SIZE
            || raw.version != VERSION
            || raw.flags & !FLAGS != 0
            || !valid_width(raw.width)
        {
            return Err(Error(8));
        }
        let color = |bit, value| {
            if raw.flags & bit != 0 {
                Ok(Some(value))
            } else if value == 0 {
                Ok(None)
            } else {
                Err(Error(8))
            }
        };
        let handle = |raw: RawHandle| match (raw.slot, raw.generation) {
            (0, 0) => Ok(None),
            (0, _) | (_, 0) => Err(Error(8)),
            _ => Ok(Some(IconResource {
                raw,
                app_id: app.id,
            })),
        };
        Ok(Self {
            half_steps: raw.flags & HALF != 0,
            hover_preview: raw.flags & HOVER != 0,
            step_down_filled: raw.flags & STEP_DOWN != 0,
            clearable: raw.flags & CLEAR != 0,
            readonly: raw.flags & READONLY != 0,
            design_size: design_size(raw.design_size).map_err(|_| Error(8))?,
            width: raw.width,
            fill_rgba: color(FILL, raw.fill_rgba)?,
            outline_rgba: color(OUTLINE, raw.outline_rgba)?,
            preview_rgba: color(PREVIEW, raw.preview_rgba)?,
            outline_icon: handle(raw.outline_icon)?,
            fill_icon: handle(raw.fill_icon)?,
        })
    }
}

type SizeQuery = unsafe extern "C" fn() -> u32;
type Configure = unsafe extern "C" fn(*mut c_void, RawNode, *const ConfigRaw) -> u32;
type Get = unsafe extern "C" fn(*mut c_void, RawNode, *mut ConfigRaw) -> u32;

struct Api {
    configure: Configure,
    get: Get,
}

fn api() -> Option<&'static Api> {
    static API: OnceLock<Option<Api>> = OnceLock::new();
    API.get_or_init(resolve).as_ref()
}

#[cfg(any(cui_bridge_static, unix, windows))]
fn resolve() -> Option<Api> {
    #[cfg(cui_bridge_static)]
    extern "C" {
        fn cui_bridge_rating_config_size_v1() -> u32;
        fn cui_bridge_rating_configure_v1(a: *mut c_void, n: RawNode, c: *const ConfigRaw) -> u32;
        fn cui_bridge_rating_get_config_v1(a: *mut c_void, n: RawNode, c: *mut ConfigRaw) -> u32;
    }
    #[cfg(cui_bridge_static)]
    macro_rules! symbol {
        ($name:ident, $ty:ty) => {
            $name as $ty
        };
    }
    #[cfg(not(cui_bridge_static))]
    macro_rules! symbol {
        ($name:ident, $ty:ty) => {{
            let name = concat!(stringify!($name), "\0");
            let pointer = unsafe { crate::notification_symbol_v1(name.as_ptr().cast()) };
            if pointer.is_null() {
                return None;
            }
            unsafe { std::mem::transmute::<*mut c_void, $ty>(pointer) }
        }};
    }
    let size = symbol!(cui_bridge_rating_config_size_v1, SizeQuery);
    let configure = symbol!(cui_bridge_rating_configure_v1, Configure);
    let get = symbol!(cui_bridge_rating_get_config_v1, Get);
    (unsafe { size() } == SIZE).then_some(Api { configure, get })
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> Option<Api> {
    None
}

impl App {
    /// Reports whether the selected provider has the complete Rating V1 group.
    pub fn supports_rating_options(&self) -> bool {
        api().is_some()
    }

    /// Mount a Rating with copied options. A failed configuration frees the key.
    pub fn configured_rating(
        &mut self,
        parent: Node,
        key: &str,
        config: &RatingConfig,
    ) -> Result<Node> {
        let bridge = api().ok_or(Error(9))?;
        let raw = config.encode(self)?;
        let node = self.child(parent, Kind::Rating, key)?;
        if let Err(error) = check(unsafe { (bridge.configure)(self.raw, self.valid(node)?, &raw) })
        {
            self.remove(node)?;
            return Err(error);
        }
        Ok(node)
    }

    /// Replace the presentation without dispatching a change.
    pub fn configure_rating(&mut self, node: Node, config: &RatingConfig) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let bridge = api().ok_or(Error(9))?;
        let raw = config.encode(self)?;
        check(unsafe { (bridge.configure)(self.raw, node, &raw) })?;
        Ok(self)
    }

    /// Restore the integer presenter. The next step may normalize the value.
    pub fn reset_rating(&mut self, node: Node) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let bridge = api().ok_or(Error(9))?;
        check(unsafe { (bridge.configure)(self.raw, node, ptr::null()) })?;
        Ok(self)
    }

    /// Copy the current options. Earlier providers and ordinary Rating nodes
    /// have no override. An Icon handle in the snapshot remains app-owned.
    pub fn rating_config(&self, node: Node) -> Result<Option<RatingConfig>> {
        let node = self.valid(node)?;
        let Some(bridge) = api() else {
            return Ok(None);
        };
        let mut raw = ConfigRaw::with_header();
        let status = unsafe { (bridge.get)(self.raw, node, &mut raw) };
        if status == 1 {
            return Ok(None);
        }
        check(status)?;
        Ok(Some(RatingConfig::decode(raw, self)?))
    }
}

impl Builder<'_> {
    /// Apply Rating options to this existing child. Use
    /// App::configured_rating to preflight provider support during creation.
    pub fn rating_options(self, config: &RatingConfig) -> Result<Self> {
        self.app.configure_rating(self.node, config)?;
        Ok(self)
    }
}
