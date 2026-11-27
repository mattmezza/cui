use crate::*;
/// Which popup side is attached to the trigger.
#[repr(u32)]
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum PopoverAnchor {
    #[default]
    TopLeft,
    TopCenter,
    TopRight,
    BottomLeft,
    BottomCenter,
    BottomRight,
    LeftCenter,
    RightCenter,
}
/// Zero dimensions use native defaults; reference_rem zero means 16.
#[derive(Clone, Copy, Debug, Default)]
pub struct PopoverOptions {
    pub anchor: PopoverAnchor,
    pub width: f32,
    pub height: f32,
    pub margin: f32,
    pub offset: f32,
    pub reference_rem: f32,
    pub arrow: bool,
    pub hide_close: bool,
    pub keep_pointer_focus: bool,
    pub focus_content: bool,
    pub no_escape_dismiss: bool,
    pub no_outside_dismiss: bool,
    pub custom_offset: bool,
}
#[repr(C)]
struct Raw {
    anchor: u32,
    flags: u32,
    width: f32,
    height: f32,
    margin: f32,
    offset: f32,
    reference_rem: f32,
    reserved: u32,
}
type Configure = unsafe extern "C" fn(*mut c_void, RawNode, u32, *const Raw) -> u32;
fn function() -> Option<Configure> {
    #[cfg(cui_bridge_static)]
    {
        extern "C" {
            fn cui_bridge_popover_configure_v1(
                a: *mut c_void,
                n: RawNode,
                size: u32,
                c: *const Raw,
            ) -> u32;
        }
        Some(cui_bridge_popover_configure_v1)
    }
    #[cfg(all(not(cui_bridge_static), any(unix, windows)))]
    unsafe {
        let p = notification_symbol_v1(b"cui_bridge_popover_configure_v1\0".as_ptr().cast());
        if p.is_null() {
            None
        } else {
            Some(std::mem::transmute::<*mut c_void, Configure>(p))
        }
    }
    #[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
    {
        None
    }
}
impl PopoverOptions {
    fn raw(self) -> Result<Raw> {
        for (v, lo, hi) in [
            (self.width, 0., 4096.),
            (self.height, 0., 4096.),
            (self.margin, 0., 4096.),
            (self.offset, -4096., 4096.),
            (self.reference_rem, 0., 256.),
        ] {
            if !v.is_finite() || v < lo || v > hi {
                return Err(Error(3));
            }
        }
        let mut flags = 0;
        for (i, b) in [
            self.arrow,
            self.hide_close,
            self.keep_pointer_focus,
            self.focus_content,
            self.no_escape_dismiss,
            self.no_outside_dismiss,
            self.custom_offset,
        ]
        .iter()
        .enumerate()
        {
            if *b {
                flags |= 1 << i;
            }
        }
        Ok(Raw {
            anchor: self.anchor as u32,
            flags,
            width: self.width,
            height: self.height,
            margin: self.margin,
            offset: if self.custom_offset { self.offset } else { 0. },
            reference_rem: self.reference_rem,
            reserved: 0,
        })
    }
}
impl App {
    pub fn supports_popover_options(&self) -> bool {
        function().is_some()
    }
    /// None restores legacy presentation. An unavailable provider returns Error(8).
    pub fn popover_options(
        &mut self,
        n: Node,
        options: Option<PopoverOptions>,
    ) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let raw = options.map(PopoverOptions::raw).transpose()?;
        let f = function().ok_or(Error(8))?;
        check(unsafe {
            f(
                self.raw,
                n,
                if raw.is_some() {
                    std::mem::size_of::<Raw>() as u32
                } else {
                    0
                },
                raw.as_ref().map_or(std::ptr::null(), |r| r),
            )
        })?;
        Ok(self)
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn layout_and_validation() {
        assert_eq!(std::mem::size_of::<Raw>(), 32);
        let o = PopoverOptions {
            arrow: true,
            focus_content: true,
            offset: 42.,
            ..Default::default()
        };
        let r = o.raw().unwrap();
        assert_eq!(r.flags, 9);
        assert_eq!(r.offset, 0.);
        assert!(PopoverOptions {
            width: f32::NAN,
            ..o
        }
        .raw()
        .is_err());
        assert!(PopoverOptions {
            reference_rem: 257.,
            ..o
        }
        .raw()
        .is_err());
    }
}
