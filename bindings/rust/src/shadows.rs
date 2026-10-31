//! Six-kind owned root shadow authoring; no component parts or portals.
use super::{check, App, Builder, Error, Node, RawNode, Result};
use std::ffi::c_void;
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ShadowKind {
    Drop = 0,
    Inset = 1,
}
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ShadowPreset {
    None = 0,
    XXS = 1,
    XS = 2,
    SM = 3,
    MD = 4,
    LG = 5,
    XL = 6,
    XXL = 7,
}
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct CornerRadii {
    pub top_left: f64,
    pub top_right: f64,
    pub bottom_left: f64,
    pub bottom_right: f64,
}
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct BoxShadow {
    pub offset_x: f64,
    pub offset_y: f64,
    pub sigma: f64,
    pub spread: f64,
    pub kind: ShadowKind,
    pub rgba: u32,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct Layer {
    x: f32,
    y: f32,
    sigma: f32,
    spread: f32,
    kind: u32,
    rgba: u32,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct Style {
    size: u32,
    schema: u32,
    count: u32,
    reserved: u32,
    tl: f32,
    tr: f32,
    bl: f32,
    br: f32,
    layers: [Layer; 8],
}
const _: [(); 24] = [(); std::mem::size_of::<Layer>()];
const _: [(); 224] = [(); std::mem::size_of::<Style>()];
const _: [(); 32] = [(); std::mem::offset_of!(Style, layers)];
const _: [(); 20] = [(); std::mem::offset_of!(Layer, rgba)];
fn number(v: f64, min: f64, max: f64) -> Result<f32> {
    if v.is_finite() && (v as f32).is_finite() && v >= min && v <= max {
        Ok(v as f32)
    } else {
        Err(Error(3))
    }
}
fn radii(r: CornerRadii) -> Result<[f32; 4]> {
    Ok([
        number(r.top_left, 0., 16384.)?,
        number(r.top_right, 0., 16384.)?,
        number(r.bottom_left, 0., 16384.)?,
        number(r.bottom_right, 0., 16384.)?,
    ])
}
impl BoxShadow {
    fn wire(self) -> Result<Layer> {
        Ok(Layer {
            x: number(self.offset_x, -4096., 4096.)?,
            y: number(self.offset_y, -4096., 4096.)?,
            sigma: number(self.sigma, 0., 256.)?,
            spread: number(self.spread, -4096., 4096.)?,
            kind: self.kind as u32,
            rgba: self.rgba,
        })
    }
    fn from_wire(l: Layer) -> Self {
        Self {
            offset_x: l.x as f64,
            offset_y: l.y as f64,
            sigma: l.sigma as f64,
            spread: l.spread as f64,
            kind: if l.kind == 0 {
                ShadowKind::Drop
            } else {
                ShadowKind::Inset
            },
            rgba: l.rgba,
        }
    }
    pub fn css(
        x: f64,
        y: f64,
        blur_radius: f64,
        spread: f64,
        rgba: u32,
        kind: ShadowKind,
    ) -> Result<Self> {
        let (x, y, r, s) = (
            number(x, -4096., 4096.)?,
            number(y, -4096., 4096.)?,
            number(blur_radius, 0., 512.)?,
            number(spread, -4096., 4096.)?,
        );
        let api = functions()?;
        let mut layer = Layer::default();
        check(unsafe { (api.css)(x, y, r, s, rgba, kind as u32, 24, &mut layer) })?;
        Ok(Self::from_wire(layer))
    }
}
#[derive(Clone, Debug, PartialEq)]
pub struct ShadowStyle {
    layers: Vec<BoxShadow>,
    radii: CornerRadii,
}
impl ShadowStyle {
    pub fn new(layers: &[BoxShadow], r: CornerRadii) -> Result<Self> {
        let s = Self {
            layers: layers.to_vec(),
            radii: r,
        };
        s.wire()?;
        Ok(s)
    }
    pub fn layers(&self) -> &[BoxShadow] {
        &self.layers
    }
    pub fn radii(&self) -> CornerRadii {
        self.radii
    }
    fn wire(&self) -> Result<Style> {
        if self.layers.len() > 8 {
            return Err(Error(3));
        }
        let r = radii(self.radii)?;
        let mut v = Style {
            size: 224,
            schema: 1,
            count: self.layers.len() as u32,
            tl: r[0],
            tr: r[1],
            bl: r[2],
            br: r[3],
            ..Style::default()
        };
        for (i, l) in self.layers.iter().enumerate() {
            v.layers[i] = l.wire()?
        }
        Ok(v)
    }
    pub fn preset(p: ShadowPreset, r: CornerRadii) -> Result<Self> {
        let f = radii(r)?;
        let api = functions()?;
        let mut v = Style::default();
        check(unsafe { (api.preset)(p as u32, f[0], f[1], f[2], f[3], 224, &mut v) })?;
        Self::new(
            &v.layers[..v.count as usize]
                .iter()
                .map(|l| BoxShadow::from_wire(*l))
                .collect::<Vec<_>>(),
            r,
        )
    }
}
#[cfg(not(cui_bridge_static))]
type Size = unsafe extern "C" fn() -> u32;
type Set = unsafe extern "C" fn(*mut c_void, RawNode, u32, *const Style) -> u32;
type Reset = unsafe extern "C" fn(*mut c_void, RawNode) -> u32;
type Css = unsafe extern "C" fn(f32, f32, f32, f32, u32, u32, u32, *mut Layer) -> u32;
type Preset = unsafe extern "C" fn(u32, f32, f32, f32, f32, u32, *mut Style) -> u32;
struct Api {
    set: Set,
    reset: Reset,
    css: Css,
    preset: Preset,
}
#[cfg(cui_bridge_static)]
fn functions() -> Result<Api> {
    extern "C" {
        fn cui_bridge_box_shadows_v1_size() -> u32;
        fn cui_bridge_set_box_shadows_v1(
            a: *mut c_void,
            n: RawNode,
            size: u32,
            v: *const Style,
        ) -> u32;
        fn cui_bridge_reset_box_shadows(a: *mut c_void, n: RawNode) -> u32;
        fn cui_bridge_box_shadow_from_css_v1(
            x: f32,
            y: f32,
            r: f32,
            s: f32,
            c: u32,
            k: u32,
            size: u32,
            v: *mut Layer,
        ) -> u32;
        fn cui_bridge_shadow_preset_v1(
            p: u32,
            tl: f32,
            tr: f32,
            bl: f32,
            br: f32,
            size: u32,
            v: *mut Style,
        ) -> u32;
    }
    if unsafe { cui_bridge_box_shadows_v1_size() } != 224 {
        return Err(Error(3));
    }
    Ok(Api {
        set: cui_bridge_set_box_shadows_v1,
        reset: cui_bridge_reset_box_shadows,
        css: cui_bridge_box_shadow_from_css_v1,
        preset: cui_bridge_shadow_preset_v1,
    })
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn functions() -> Result<Api> {
    unsafe {
        macro_rules! resolve {
            ($name:literal,$ty:ty) => {{
                let p = super::notification_symbol_v1(concat!($name, "\0").as_ptr().cast());
                if p.is_null() {
                    return Err(Error(9));
                }
                std::mem::transmute::<*mut c_void, $ty>(p)
            }};
        }
        let size = resolve!("cui_bridge_box_shadows_v1_size", Size);
        if size() != 224 {
            return Err(Error(3));
        }
        Ok(Api {
            set: resolve!("cui_bridge_set_box_shadows_v1", Set),
            reset: resolve!("cui_bridge_reset_box_shadows", Reset),
            css: resolve!("cui_bridge_box_shadow_from_css_v1", Css),
            preset: resolve!("cui_bridge_shadow_preset_v1", Preset),
        })
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn functions() -> Result<Api> {
    Err(Error(9))
}
impl App {
    pub fn box_shadows(&mut self, n: Node, style: &ShadowStyle) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let v = style.wire()?;
        let api = functions()?;
        check(unsafe { (api.set)(self.raw, n, 224, &v) })?;
        Ok(self)
    }
    pub fn reset_box_shadows(&mut self, n: Node) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let api = functions()?;
        check(unsafe { (api.reset)(self.raw, n) })?;
        Ok(self)
    }
    pub fn shadow_preset(&mut self, n: Node, p: ShadowPreset, r: CornerRadii) -> Result<&mut Self> {
        let v = ShadowStyle::preset(p, r)?;
        self.box_shadows(n, &v)
    }
}
impl<'a> Builder<'a> {
    pub fn box_shadows(self, style: &ShadowStyle) -> Result<Self> {
        self.app.box_shadows(self.node, style)?;
        Ok(self)
    }
    pub fn reset_box_shadows(self) -> Result<Self> {
        self.app.reset_box_shadows(self.node)?;
        Ok(self)
    }
    pub fn shadow_preset(self, p: ShadowPreset, r: CornerRadii) -> Result<Self> {
        self.app.shadow_preset(self.node, p, r)?;
        Ok(self)
    }
}
