//! Optional retained Radio paint. Colors are numeric 0xRRGGBBAA.
use super::{check, App, Builder, Error, Node, RawNode, Result};
use std::ffi::c_void;
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RadioPaintLayer {
    Base,
    Checked,
    Hovered,
    Pressed,
    Focused,
    Disabled,
}
/// A sparse replacement for one layer; None inherits, Some(0) is explicit zero.
/// Geometry and typography belong to Base only. Groups accept root fields on
/// Base/Disabled only; description fields belong to direct group Items only.
/// Widths: 0..=64; radii/padding/gap: 0..=256; extent: 4..=128;
/// text sizes: 1..=128; weights: normal=0, medium=1, semibold=2, bold=3.
#[derive(Clone, Copy, Debug, Default)]
pub struct RadioPaint {
    pub root_background_rgba: Option<u32>,
    pub root_border_rgba: Option<u32>,
    pub indicator_background_rgba: Option<u32>,
    pub indicator_border_rgba: Option<u32>,
    pub mark_rgba: Option<u32>,
    pub label_rgba: Option<u32>,
    pub description_rgba: Option<u32>,
    pub root_border_width: Option<f32>,
    pub root_radius: Option<f32>,
    pub root_padding: Option<f32>,
    pub root_gap: Option<f32>,
    pub indicator_border_width: Option<f32>,
    pub indicator_radius: Option<f32>,
    pub indicator_extent: Option<f32>,
    pub label_size: Option<f32>,
    pub description_size: Option<f32>,
    pub label_weight: Option<u32>,
    pub description_weight: Option<u32>,
}
#[repr(C)]
#[derive(Default)]
struct Wire {
    struct_size: u32,
    schema_version: u32,
    fields: u32,
    reserved: u32,
    root_background_rgba: u32,
    root_border_rgba: u32,
    indicator_background_rgba: u32,
    indicator_border_rgba: u32,
    mark_rgba: u32,
    label_rgba: u32,
    description_rgba: u32,
    root_border_width: f32,
    root_radius: f32,
    root_padding: f32,
    root_gap: f32,
    indicator_border_width: f32,
    indicator_radius: f32,
    indicator_extent: f32,
    label_size: f32,
    description_size: f32,
    label_weight: u32,
    description_weight: u32,
}
const _: () = {
    assert!(std::mem::size_of::<Wire>() == 88);
    assert!(std::mem::offset_of!(Wire, root_border_width) == 44);
    assert!(std::mem::offset_of!(Wire, label_weight) == 80);
};
impl RadioPaint {
    fn wire(&self) -> Wire {
        let mut wire = Wire {
            struct_size: 88,
            schema_version: 1,
            ..Wire::default()
        };
        if let Some(value) = self.root_background_rgba {
            wire.fields |= 1 << 0;
            wire.root_background_rgba = value;
        }
        if let Some(value) = self.root_border_rgba {
            wire.fields |= 1 << 1;
            wire.root_border_rgba = value;
        }
        if let Some(value) = self.indicator_background_rgba {
            wire.fields |= 1 << 6;
            wire.indicator_background_rgba = value;
        }
        if let Some(value) = self.indicator_border_rgba {
            wire.fields |= 1 << 7;
            wire.indicator_border_rgba = value;
        }
        if let Some(value) = self.mark_rgba {
            wire.fields |= 1 << 11;
            wire.mark_rgba = value;
        }
        if let Some(value) = self.label_rgba {
            wire.fields |= 1 << 12;
            wire.label_rgba = value;
        }
        if let Some(value) = self.description_rgba {
            wire.fields |= 1 << 13;
            wire.description_rgba = value;
        }
        if let Some(value) = self.root_border_width {
            wire.fields |= 1 << 2;
            wire.root_border_width = value;
        }
        if let Some(value) = self.root_radius {
            wire.fields |= 1 << 3;
            wire.root_radius = value;
        }
        if let Some(value) = self.root_padding {
            wire.fields |= 1 << 4;
            wire.root_padding = value;
        }
        if let Some(value) = self.root_gap {
            wire.fields |= 1 << 5;
            wire.root_gap = value;
        }
        if let Some(value) = self.indicator_border_width {
            wire.fields |= 1 << 8;
            wire.indicator_border_width = value;
        }
        if let Some(value) = self.indicator_radius {
            wire.fields |= 1 << 9;
            wire.indicator_radius = value;
        }
        if let Some(value) = self.indicator_extent {
            wire.fields |= 1 << 10;
            wire.indicator_extent = value;
        }
        if let Some(value) = self.label_size {
            wire.fields |= 1 << 14;
            wire.label_size = value;
        }
        if let Some(value) = self.description_size {
            wire.fields |= 1 << 15;
            wire.description_size = value;
        }
        if let Some(value) = self.label_weight {
            wire.fields |= 1 << 16;
            wire.label_weight = value;
        }
        if let Some(value) = self.description_weight {
            wire.fields |= 1 << 17;
            wire.description_weight = value;
        }
        wire
    }
}
type Set = unsafe extern "C" fn(*mut c_void, RawNode, u32, *const Wire) -> u32;
#[cfg(cui_bridge_static)]
fn setter() -> Option<Set> {
    extern "C" {
        fn cui_bridge_radio_set_paint_v1(
            a: *mut c_void,
            n: RawNode,
            layer: u32,
            paint: *const Wire,
        ) -> u32;
    }
    Some(cui_bridge_radio_set_paint_v1)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn setter() -> Option<Set> {
    unsafe {
        let p = super::notification_symbol_v1(b"cui_bridge_radio_set_paint_v1\0".as_ptr().cast());
        if p.is_null() {
            None
        } else {
            Some(std::mem::transmute::<*mut c_void, Set>(p))
        }
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn setter() -> Option<Set> {
    None
}
impl App {
    /// Preflight before creating nodes when an optional provider may be older.
    pub fn supports_radio_paint(&self) -> bool {
        setter().is_some()
    }
    /// Replace one layer on a Radio, RadioGroup or direct group Item. None or an
    /// empty paint clears only that layer. Errors preserve previous layers.
    /// Set between steps on the owner thread; this never emits notifications.
    /// Missing optional providers return Error(9) for nonempty paint.
    pub fn radio_paint(
        &mut self,
        node: Node,
        layer: RadioPaintLayer,
        paint: Option<&RadioPaint>,
    ) -> Result<&mut Self> {
        let raw = self.valid(node)?;
        let wire = paint
            .map(RadioPaint::wire)
            .filter(|value| value.fields != 0);
        let Some(set) = setter() else {
            return if wire.is_some() {
                Err(Error(9))
            } else {
                Ok(self)
            };
        };
        check(unsafe {
            set(
                self.raw,
                raw,
                layer as u32,
                wire.as_ref().map_or(std::ptr::null(), |v| v),
            )
        })?;
        Ok(self)
    }
}
impl<'a> Builder<'a> {
    /// Builders create nodes immediately. Check App::supports_radio_paint before
    /// build_child/build_root: a setter error does not roll back node creation.
    pub fn radio_paint(self, layer: RadioPaintLayer, paint: Option<&RadioPaint>) -> Result<Self> {
        self.app.radio_paint(self.node, layer, paint)?;
        Ok(self)
    }
}
