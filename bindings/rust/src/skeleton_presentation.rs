//! Optional retained responsive Skeleton presentation.
use super::{check, App, Builder, Error, Kind, Node, RawNode, Result};
use std::{ffi::c_void, mem::size_of, sync::OnceLock};

/// Width resolves in logical pixels or relative to the parent's inner width.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum SkeletonWidth {
    Fixed(f32),
    Fraction(f32),
    Grow,
}

/// Owned style and motion policy for a decorative loading placeholder.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SkeletonConfig {
    pub width: SkeletonWidth,
    pub height: f32,
    /// Optional cap for fixed and grow widths. Zero is uncapped.
    pub max_width: f32,
    pub radius: f32,
    /// Pulse duration in seconds. Zero selects two seconds.
    pub duration: f32,
    /// Numeric 0xRRGGBBAA. None uses the active theme.
    pub background_rgba: Option<u32>,
    pub secondary: bool,
    pub reduced_motion: bool,
}

impl Default for SkeletonConfig {
    fn default() -> Self {
        Self {
            width: SkeletonWidth::Fraction(1.),
            height: 16.,
            max_width: 0.,
            radius: 0.,
            duration: 0.,
            background_rgba: None,
            secondary: false,
            reduced_motion: false,
        }
    }
}

/// Snapshot from the latest successful frame, owned independently of the node.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SkeletonPresentation {
    pub frame_sequence: u64,
    pub core_time: f64,
    /// Absolute Core time, or zero for a static placeholder.
    pub next_wake_seconds: f64,
    pub phase: f32,
    pub opacity: f32,
}

const V1: u32 = 1;
const SIZE: u32 = 40;
const SECONDARY: u32 = 1;
const CUSTOM_COLOR: u32 = 2;
const REDUCED_MOTION: u32 = 4;

#[repr(C)]
#[derive(Default)]
struct ConfigRaw {
    size: u32,
    version: u32,
    flags: u32,
    width_mode: u32,
    width: f32,
    height: f32,
    max_width: f32,
    radius: f32,
    duration: f32,
    background_rgba: u32,
}

#[repr(C)]
#[derive(Default)]
struct PresentationRaw {
    size: u32,
    version: u32,
    frame_sequence: u64,
    core_time: f64,
    next_wake_seconds: f64,
    phase: f32,
    opacity: f32,
}

const _: [(); SIZE as usize] = [(); size_of::<ConfigRaw>()];
const _: [(); SIZE as usize] = [(); size_of::<PresentationRaw>()];

impl TryFrom<SkeletonConfig> for ConfigRaw {
    type Error = Error;

    fn try_from(c: SkeletonConfig) -> Result<Self> {
        if !(c.height > 0.
            && c.height <= 16384.
            && (0. ..=16384.).contains(&c.max_width)
            && (0. ..=16384.).contains(&c.radius)
            && (0. ..=60.).contains(&c.duration))
        {
            return Err(Error(3));
        }
        let (width_mode, width) = match c.width {
            SkeletonWidth::Fixed(w) if w > 0. && w <= 16384. => (0, w),
            SkeletonWidth::Fraction(w) if w > 0. && w <= 1. && c.max_width == 0. => (1, w),
            SkeletonWidth::Grow => (2, 0.),
            _ => return Err(Error(3)),
        };
        Ok(Self {
            size: SIZE,
            version: V1,
            flags: u32::from(c.secondary) * SECONDARY
                | u32::from(c.background_rgba.is_some()) * CUSTOM_COLOR
                | u32::from(c.reduced_motion) * REDUCED_MOTION,
            width_mode,
            width,
            height: c.height,
            max_width: c.max_width,
            radius: c.radius,
            duration: c.duration,
            background_rgba: c.background_rgba.unwrap_or_default(),
        })
    }
}

impl TryFrom<ConfigRaw> for SkeletonConfig {
    type Error = Error;

    fn try_from(r: ConfigRaw) -> Result<Self> {
        if r.size != SIZE
            || r.version != V1
            || r.flags & !(SECONDARY | CUSTOM_COLOR | REDUCED_MOTION) != 0
            || (r.flags & CUSTOM_COLOR == 0 && r.background_rgba != 0)
        {
            return Err(Error(3));
        }
        let c = Self {
            width: match r.width_mode {
                0 => SkeletonWidth::Fixed(r.width),
                1 => SkeletonWidth::Fraction(r.width),
                2 if r.width == 0. => SkeletonWidth::Grow,
                _ => return Err(Error(3)),
            },
            height: r.height,
            max_width: r.max_width,
            radius: r.radius,
            duration: r.duration,
            background_rgba: (r.flags & CUSTOM_COLOR != 0).then_some(r.background_rgba),
            secondary: r.flags & SECONDARY != 0,
            reduced_motion: r.flags & REDUCED_MOTION != 0,
        };
        ConfigRaw::try_from(c)?;
        Ok(c)
    }
}

impl TryFrom<PresentationRaw> for SkeletonPresentation {
    type Error = Error;

    fn try_from(r: PresentationRaw) -> Result<Self> {
        if r.size != SIZE
            || r.version != V1
            || r.frame_sequence == 0
            || !r.core_time.is_finite()
            || r.core_time < 0.
            || !r.next_wake_seconds.is_finite()
            || (r.next_wake_seconds != 0. && r.next_wake_seconds <= r.core_time)
            || !(0. ..=1.).contains(&r.phase)
            || !(0.5..=1.).contains(&r.opacity)
        {
            return Err(Error(3));
        }
        Ok(Self {
            frame_sequence: r.frame_sequence,
            core_time: r.core_time,
            next_wake_seconds: r.next_wake_seconds,
            phase: r.phase,
            opacity: r.opacity,
        })
    }
}

type Query = unsafe extern "C" fn() -> u32;
type Configure = unsafe extern "C" fn(*mut c_void, RawNode, *const ConfigRaw) -> u32;
type GetConfig = unsafe extern "C" fn(*mut c_void, RawNode, *mut ConfigRaw) -> u32;
type GetPresentation = unsafe extern "C" fn(*mut c_void, RawNode, *mut PresentationRaw) -> u32;

struct Api {
    configure: Configure,
    get_config: GetConfig,
    get_presentation: GetPresentation,
}

// Both the bundled library and the linked dynamic provider live for the process.
fn api() -> Option<&'static Api> {
    static API: OnceLock<Option<Api>> = OnceLock::new();
    API.get_or_init(resolve).as_ref()
}

#[cfg(any(cui_bridge_static, unix, windows))]
fn resolve() -> Option<Api> {
    #[cfg(cui_bridge_static)]
    macro_rules! resolve_symbol {
        ($name:ident, $ty:ty) => {
            $name as $ty
        };
    }
    #[cfg(not(cui_bridge_static))]
    macro_rules! resolve_symbol {
        ($name:ident, $ty:ty) => {{
            let name = concat!(stringify!($name), "\0");
            // Each optional export must exist before calling the group.
            let symbol = unsafe { super::notification_symbol_v1(name.as_ptr().cast()) };
            if symbol.is_null() {
                return None;
            }
            unsafe { std::mem::transmute::<*mut c_void, $ty>(symbol) }
        }};
    }

    #[cfg(cui_bridge_static)]
    extern "C" {
        fn cui_bridge_skeleton_versions() -> u32;
        fn cui_bridge_skeleton_config_size_v1() -> u32;
        fn cui_bridge_skeleton_presentation_size_v1() -> u32;
        fn cui_bridge_skeleton_configure_v1(a: *mut c_void, n: RawNode, c: *const ConfigRaw)
            -> u32;
        fn cui_bridge_skeleton_get_config_v1(a: *mut c_void, n: RawNode, c: *mut ConfigRaw) -> u32;
        fn cui_bridge_skeleton_get_presentation_v1(
            a: *mut c_void,
            n: RawNode,
            p: *mut PresentationRaw,
        ) -> u32;
    }

    let versions = resolve_symbol!(cui_bridge_skeleton_versions, Query);
    let config_size = resolve_symbol!(cui_bridge_skeleton_config_size_v1, Query);
    let presentation_size = resolve_symbol!(cui_bridge_skeleton_presentation_size_v1, Query);
    let api = Api {
        configure: resolve_symbol!(cui_bridge_skeleton_configure_v1, Configure),
        get_config: resolve_symbol!(cui_bridge_skeleton_get_config_v1, GetConfig),
        get_presentation: resolve_symbol!(cui_bridge_skeleton_get_presentation_v1, GetPresentation),
    };
    if unsafe { versions() & V1 != 0 && config_size() == SIZE && presentation_size() == SIZE } {
        Some(api)
    } else {
        None
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> Option<Api> {
    None
}

impl App {
    /// Check all optional V1 exports, including both record sizes.
    pub fn supports_skeleton_presentation(&self) -> bool {
        api().is_some()
    }

    /// Create a responsive Skeleton child. An unsupported or invalid style
    /// leaves the parent tree unchanged.
    pub fn responsive_skeleton(
        &mut self,
        parent: Node,
        key: &str,
        config: SkeletonConfig,
    ) -> Result<Node> {
        let c = ConfigRaw::try_from(config)?;
        let bridge = api().ok_or(Error(9))?;
        let node = self.child(parent, Kind::Skeleton, key)?;
        if let Err(error) = check(unsafe { (bridge.configure)(self.raw, node.raw, &c) }) {
            self.remove(node)?;
            return Err(error);
        }
        Ok(node)
    }

    /// Replace the node's configuration. The bridge copies the value.
    pub fn configure_skeleton(&mut self, node: Node, config: SkeletonConfig) -> Result<&mut Self> {
        let n = self.valid(node)?;
        let c = ConfigRaw::try_from(config)?;
        let bridge = api().ok_or(Error(9))?;
        check(unsafe { (bridge.configure)(self.raw, n, &c) })?;
        Ok(self)
    }

    /// Read the current configuration, or None for a legacy Skeleton.
    pub fn skeleton_config(&self, node: Node) -> Result<Option<SkeletonConfig>> {
        let n = self.valid(node)?;
        let bridge = api().ok_or(Error(9))?;
        let mut raw = ConfigRaw {
            size: SIZE,
            version: V1,
            ..Default::default()
        };
        let status = unsafe { (bridge.get_config)(self.raw, n, &mut raw) };
        if status == 1 {
            return Ok(None);
        }
        check(status)?;
        Ok(Some(raw.try_into()?))
    }

    /// Read the current sample. Omitted, reconfigured and failed frames have None.
    pub fn skeleton_presentation(&self, node: Node) -> Result<Option<SkeletonPresentation>> {
        let n = self.valid(node)?;
        let bridge = api().ok_or(Error(9))?;
        let mut raw = PresentationRaw {
            size: SIZE,
            version: V1,
            ..Default::default()
        };
        let status = unsafe { (bridge.get_presentation)(self.raw, n, &mut raw) };
        if status == 1 {
            return Ok(None);
        }
        check(status)?;
        Ok(Some(raw.try_into()?))
    }
}

impl Builder<'_> {
    /// Apply responsive presentation to an already created Skeleton.
    pub fn skeleton_presentation(self, config: SkeletonConfig) -> Result<Self> {
        self.app.configure_skeleton(self.node, config)?;
        Ok(self)
    }
}
