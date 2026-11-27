//! Optional retained style shared by literal and typed Kbd nodes.
use crate::{check, App, Builder, Error, Node, RawNode, Result};
use std::{ffi::c_void, sync::OnceLock};
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct KbdPadding {
    pub x: f32,
    pub y: f32,
}
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct KbdBorder {
    pub width: f32,
    pub color: u32,
}
#[derive(Clone, Copy, Debug, Default, PartialEq)]
#[repr(C)]
pub struct KbdGradientStop {
    pub position: f32,
    pub color: u32,
}
#[derive(Clone, Debug, PartialEq)]
pub struct KbdTextGradient {
    pub x0: f32,
    pub y0: f32,
    pub x1: f32,
    pub y1: f32,
    pub stops: Vec<KbdGradientStop>,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum KbdWeight {
    Normal,
    Medium,
    Semibold,
    Bold,
}
/// Complete replacement style. Colors are numeric 0xRRGGBBAA; None inherits.
/// Plain typed hints ignore style. Gradient coordinates are block normalized.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct KbdStyle {
    pub max_width: Option<f32>,
    pub padding: Option<KbdPadding>,
    pub radius: Option<f32>,
    pub background: Option<u32>,
    pub border: Option<KbdBorder>,
    pub font_size: Option<f32>,
    pub line_height: Option<f32>,
    pub weight: Option<KbdWeight>,
    pub fill: bool,
    pub wrap: bool,
    pub italic: bool,
    pub monospace: bool,
    pub text_color: Option<u32>,
    pub text_gradient: Option<KbdTextGradient>,
}
#[repr(C)]
#[derive(Default)]
struct Wire {
    size: u32,
    version: u32,
    fields: u32,
    reserved: u32,
    background: u32,
    border: u32,
    text: u32,
    weight: u32,
    max_width: f32,
    padding_x: f32,
    padding_y: f32,
    radius: f32,
    border_width: f32,
    font_size: f32,
    line_height: f32,
    count: u32,
    x0: f32,
    y0: f32,
    x1: f32,
    y1: f32,
    stops: [KbdGradientStop; 8],
}
type Set = unsafe extern "C" fn(*mut c_void, RawNode, *const Wire) -> u32;
fn setter() -> Option<Set> {
    static SET: OnceLock<Option<Set>> = OnceLock::new();
    *SET.get_or_init(resolve)
}
#[cfg(cui_bridge_static)]
fn resolve() -> Option<Set> {
    extern "C" {
        fn cui_bridge_set_kbd_style_v1(a: *mut c_void, n: RawNode, s: *const Wire) -> u32;
    }
    Some(cui_bridge_set_kbd_style_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve() -> Option<Set> {
    let p = unsafe { crate::notification_symbol_v1(c"cui_bridge_set_kbd_style_v1".as_ptr()) };
    if p.is_null() {
        None
    } else {
        Some(unsafe { std::mem::transmute::<*mut c_void, Set>(p) })
    }
}
#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> Option<Set> {
    None
}
impl KbdStyle {
    fn wire(&self) -> Result<Wire> {
        let mut w = Wire {
            size: 144,
            version: 1,
            ..Wire::default()
        };
        for (v, max, positive) in [
            (self.max_width, 4096.0, true),
            (self.radius, 4096.0, false),
            (self.font_size, 512.0, true),
            (self.line_height, 4096.0, false),
        ] {
            if let Some(v) = v {
                if !v.is_finite() || !(0.0..=max).contains(&v) || positive && v == 0.0 {
                    return Err(Error(3));
                }
            }
        }
        macro_rules! field {
            ($v:expr,$dst:ident,$bit:expr) => {
                if let Some(v) = $v {
                    w.fields |= 1 << $bit;
                    w.$dst = v;
                }
            };
        }
        field!(self.max_width, max_width, 0);
        field!(self.radius, radius, 4);
        field!(self.background, background, 5);
        field!(self.font_size, font_size, 7);
        field!(self.line_height, line_height, 8);
        field!(self.text_color, text, 12);
        for (v, bit) in [
            (self.fill, 1),
            (self.wrap, 2),
            (self.italic, 10),
            (self.monospace, 11),
        ] {
            if v {
                w.fields |= 1 << bit;
            }
        }
        if let Some(p) = self.padding {
            if !(0.0..=4096.0).contains(&p.x) || !(0.0..=4096.0).contains(&p.y) {
                return Err(Error(3));
            }
            w.fields |= 1 << 3;
            w.padding_x = p.x;
            w.padding_y = p.y;
        }
        if let Some(b) = self.border {
            if !(0.0..=4096.0).contains(&b.width) {
                return Err(Error(3));
            }
            w.fields |= 1 << 6;
            w.border_width = b.width;
            w.border = b.color;
        }
        if let Some(v) = self.weight {
            w.fields |= 1 << 9;
            w.weight = v as u32;
        }
        if let Some(g) = &self.text_gradient {
            if self.text_color.is_some()
                || !(2..=8).contains(&g.stops.len())
                || g.x0 == g.x1 && g.y0 == g.y1
            {
                return Err(Error(3));
            }
            for v in [g.x0, g.y0, g.x1, g.y1] {
                if !v.is_finite() || !(0.0..=1.0).contains(&v) {
                    return Err(Error(3));
                }
            }
            let mut last = 0.0;
            for (i, s) in g.stops.iter().enumerate() {
                if !s.position.is_finite()
                    || !(0.0..=1.0).contains(&s.position)
                    || i > 0 && s.position < last
                {
                    return Err(Error(3));
                }
                last = s.position;
                w.stops[i] = *s;
            }
            w.fields |= 1 << 13;
            w.count = g.stops.len() as u32;
            w.x0 = g.x0;
            w.y0 = g.y0;
            w.x1 = g.x1;
            w.y1 = g.y1;
        }
        Ok(w)
    }
}
impl Error {
    pub const KBD_STYLE_UNAVAILABLE: Self = Self(9);
}
impl App {
    pub fn supports_kbd_style(&self) -> bool {
        setter().is_some()
    }
    /// Replace the complete style; a failed update preserves the old style.
    pub fn kbd_style(&mut self, node: Node, style: &KbdStyle) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let w = style.wire()?;
        let set = setter().ok_or(Error::KBD_STYLE_UNAVAILABLE)?;
        check(unsafe { set(self.raw, node, &w) })?;
        Ok(self)
    }
    /// Restore the provider's default Kbd style.
    pub fn reset_kbd_style(&mut self, node: Node) -> Result<&mut Self> {
        let node = self.valid(node)?;
        let set = setter().ok_or(Error::KBD_STYLE_UNAVAILABLE)?;
        check(unsafe { set(self.raw, node, std::ptr::null()) })?;
        Ok(self)
    }
}
impl Builder<'_> {
    pub fn kbd_style(self, style: &KbdStyle) -> Result<Self> {
        self.app.kbd_style(self.node, style)?;
        Ok(self)
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn wire_and_validation() {
        assert_eq!(std::mem::size_of::<Wire>(), 144);
        assert_eq!(KbdStyle::default().wire().unwrap().fields, 0);
        let mut s = KbdStyle {
            max_width: Some(0.0),
            ..Default::default()
        };
        assert!(s.wire().is_err());
        s.max_width = Some(80.0);
        s.padding = Some(KbdPadding { x: 0.0, y: 0.0 });
        assert_eq!(s.wire().unwrap().fields, 9);
        s.text_gradient = Some(KbdTextGradient {
            x0: 0.0,
            y0: 0.0,
            x1: 1.0,
            y1: 0.0,
            stops: vec![
                KbdGradientStop {
                    position: 0.0,
                    color: 0,
                },
                KbdGradientStop {
                    position: 1.0,
                    color: u32::MAX,
                },
            ],
        });
        assert!(s.wire().is_ok());
        s.text_color = Some(0);
        assert!(s.wire().is_err());
    }
}
