//! Optional decorative rotating Spinner presentation. Legacy Spinner remains unchanged.
use super::{check, App, Builder, Error, Kind, Node, RawNode, Result};
use std::{ffi::c_void, mem::size_of, sync::OnceLock};
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum SpinnerSize {
    Custom(f32),
    XSmall,
    Small,
    Medium,
    Large,
}
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum SpinnerEasing {
    Quadratic,
    Linear,
}
/// Owned configuration. Empty icon selects Loader. Duration zero selects .8 seconds.
/// Named sizes use rem_size (zero selects 16). Color is numeric 0xRRGGBBAA.
#[derive(Clone, Debug, PartialEq)]
pub struct SpinnerConfig {
    pub size: SpinnerSize,
    pub rem_size: f32,
    pub duration: f32,
    pub rgba: Option<u32>,
    pub easing: SpinnerEasing,
    pub icon: String,
    pub reduced_motion: bool,
}
impl Default for SpinnerConfig {
    fn default() -> Self {
        Self {
            size: SpinnerSize::Medium,
            rem_size: 0.,
            duration: 0.,
            rgba: None,
            easing: SpinnerEasing::Quadratic,
            icon: String::new(),
            reduced_motion: false,
        }
    }
}
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SpinnerPresentation {
    pub frame_sequence: u64,
    pub core_time: f64,
    pub next_wake_seconds: f64,
    pub phase: f32,
}
const V1: u32 = 1;
const SIZE: u32 = 40;
const CONFIG_SIZE: u32 = 100;
#[repr(C)]
struct ConfigRaw {
    size: u32,
    version: u32,
    flags: u32,
    named_size: u32,
    pixels: f32,
    rem_size: f32,
    duration: f32,
    rgba: u32,
    easing: u32,
    icon: [u8; 64],
}
impl Default for ConfigRaw {
    fn default() -> Self {
        Self {
            size: 0,
            version: 0,
            flags: 0,
            named_size: 0,
            pixels: 0.,
            rem_size: 0.,
            duration: 0.,
            rgba: 0,
            easing: 0,
            icon: [0; 64],
        }
    }
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
    reserved: u32,
}
const _: [(); 100] = [(); size_of::<ConfigRaw>()];
const _: [(); 40] = [(); size_of::<PresentationRaw>()];
impl TryFrom<SpinnerConfig> for ConfigRaw {
    type Error = Error;
    fn try_from(c: SpinnerConfig) -> Result<Self> {
        if !(0. ..=60.).contains(&c.duration)
            || !(0. ..=4096.).contains(&c.rem_size)
            || c.icon.len() > 63
            || c.icon.contains('\0')
        {
            return Err(Error(3));
        }
        let (named_size, pixels) = match c.size {
            SpinnerSize::Custom(p) if p > 0. && p <= 4096. && c.rem_size == 0. => (0, p),
            SpinnerSize::XSmall => (1, 0.),
            SpinnerSize::Small => (2, 0.),
            SpinnerSize::Medium => (3, 0.),
            SpinnerSize::Large => (4, 0.),
            _ => return Err(Error(3)),
        };
        let resolved = if named_size == 0 {
            pixels
        } else {
            (if c.rem_size == 0. { 16. } else { c.rem_size })
                * [0., 0.75, 0.875, 1., 1.5][named_size as usize]
        };
        if !(resolved > 0. && resolved <= 4096.) {
            return Err(Error(3));
        }
        let mut icon = [0; 64];
        icon[..c.icon.len()].copy_from_slice(c.icon.as_bytes());
        Ok(Self {
            size: CONFIG_SIZE,
            version: V1,
            flags: u32::from(c.rgba.is_some()) | u32::from(c.reduced_motion) * 2,
            named_size,
            pixels,
            rem_size: c.rem_size,
            duration: c.duration,
            rgba: c.rgba.unwrap_or_default(),
            easing: match c.easing {
                SpinnerEasing::Quadratic => 0,
                SpinnerEasing::Linear => 1,
            },
            icon,
        })
    }
}
impl TryFrom<ConfigRaw> for SpinnerConfig {
    type Error = Error;
    fn try_from(r: ConfigRaw) -> Result<Self> {
        if r.size != CONFIG_SIZE
            || r.version != V1
            || r.flags & !3 != 0
            || (r.flags & 1 == 0 && r.rgba != 0)
        {
            return Err(Error(3));
        }
        let end = r.icon.iter().position(|b| *b == 0).ok_or(Error(3))?;
        let c = Self {
            size: match r.named_size {
                0 => SpinnerSize::Custom(r.pixels),
                1 if r.pixels == 0. => SpinnerSize::XSmall,
                2 if r.pixels == 0. => SpinnerSize::Small,
                3 if r.pixels == 0. => SpinnerSize::Medium,
                4 if r.pixels == 0. => SpinnerSize::Large,
                _ => return Err(Error(3)),
            },
            rem_size: r.rem_size,
            duration: r.duration,
            rgba: (r.flags & 1 != 0).then_some(r.rgba),
            reduced_motion: r.flags & 2 != 0,
            easing: match r.easing {
                0 => SpinnerEasing::Quadratic,
                1 => SpinnerEasing::Linear,
                _ => return Err(Error(3)),
            },
            icon: std::str::from_utf8(&r.icon[..end])
                .map_err(|_| Error(3))?
                .to_owned(),
        };
        ConfigRaw::try_from(c.clone())?;
        Ok(c)
    }
}
impl TryFrom<PresentationRaw> for SpinnerPresentation {
    type Error = Error;
    fn try_from(r: PresentationRaw) -> Result<Self> {
        if r.size != SIZE
            || r.version != V1
            || r.reserved != 0
            || r.frame_sequence == 0
            || !r.core_time.is_finite()
            || r.core_time < 0.
            || !r.next_wake_seconds.is_finite()
            || (r.next_wake_seconds != 0. && r.next_wake_seconds <= r.core_time)
            || !(0. ..=1.).contains(&r.phase)
        {
            return Err(Error(3));
        }
        Ok(Self {
            frame_sequence: r.frame_sequence,
            core_time: r.core_time,
            next_wake_seconds: r.next_wake_seconds,
            phase: r.phase,
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
        fn cui_bridge_spinner_versions() -> u32;
        fn cui_bridge_spinner_config_size_v1() -> u32;
        fn cui_bridge_spinner_presentation_size_v1() -> u32;
        fn cui_bridge_spinner_configure_v1(a: *mut c_void, n: RawNode, c: *const ConfigRaw) -> u32;
        fn cui_bridge_spinner_get_config_v1(a: *mut c_void, n: RawNode, c: *mut ConfigRaw) -> u32;
        fn cui_bridge_spinner_get_presentation_v1(
            a: *mut c_void,
            n: RawNode,
            p: *mut PresentationRaw,
        ) -> u32;
    }

    let versions = resolve_symbol!(cui_bridge_spinner_versions, Query);
    let config_size = resolve_symbol!(cui_bridge_spinner_config_size_v1, Query);
    let presentation_size = resolve_symbol!(cui_bridge_spinner_presentation_size_v1, Query);
    let api = Api {
        configure: resolve_symbol!(cui_bridge_spinner_configure_v1, Configure),
        get_config: resolve_symbol!(cui_bridge_spinner_get_config_v1, GetConfig),
        get_presentation: resolve_symbol!(cui_bridge_spinner_get_presentation_v1, GetPresentation),
    };
    if unsafe {
        versions() & V1 != 0 && config_size() == CONFIG_SIZE && presentation_size() == SIZE
    } {
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
    pub fn supports_spinner_presentation(&self) -> bool {
        api().is_some()
    }

    /// Create a rotating Spinner child. An unsupported or invalid style
    /// leaves the parent tree unchanged.
    pub fn rotating_spinner(
        &mut self,
        parent: Node,
        key: &str,
        config: SpinnerConfig,
    ) -> Result<Node> {
        let c = ConfigRaw::try_from(config)?;
        let bridge = api().ok_or(Error(9))?;
        let node = self.child(parent, Kind::Spinner, key)?;
        if let Err(error) = check(unsafe { (bridge.configure)(self.raw, node.raw, &c) }) {
            self.remove(node)?;
            return Err(error);
        }
        Ok(node)
    }

    /// Replace the node's configuration. The bridge copies the value.
    pub fn configure_spinner(&mut self, node: Node, config: SpinnerConfig) -> Result<&mut Self> {
        let n = self.valid(node)?;
        let c = ConfigRaw::try_from(config)?;
        let bridge = api().ok_or(Error(9))?;
        check(unsafe { (bridge.configure)(self.raw, n, &c) })?;
        Ok(self)
    }

    /// Read the current configuration, or None for a legacy Spinner.
    pub fn spinner_config(&self, node: Node) -> Result<Option<SpinnerConfig>> {
        let n = self.valid(node)?;
        let bridge = api().ok_or(Error(9))?;
        let mut raw = ConfigRaw {
            size: CONFIG_SIZE,
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
    pub fn spinner_presentation(&self, node: Node) -> Result<Option<SpinnerPresentation>> {
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
    /// Apply rotating presentation to an already created Spinner.
    pub fn spinner_presentation(self, config: SpinnerConfig) -> Result<Self> {
        self.app.configure_spinner(self.node, config)?;
        Ok(self)
    }
}
