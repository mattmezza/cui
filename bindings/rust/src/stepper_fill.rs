//! Optional filled Stepper presentation, independent of legacy visual admission.
use super::{check, App, Builder, Error, Node, RawNode, Result, StepperDensity};
use std::ffi::c_void;
type Set = unsafe extern "C" fn(*mut c_void, RawNode, u32, u32, f32, f32) -> u32;

#[cfg(cui_bridge_static)]
fn setter() -> Option<Set> {
    extern "C" {
        fn cui_bridge_set_stepper_fill(
            a: *mut c_void,
            n: RawNode,
            vertical: u32,
            density: u32,
            width: f32,
            height: f32,
        ) -> u32;
    }
    Some(cui_bridge_set_stepper_fill)
}
#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn setter() -> Option<Set> {
    unsafe {
        let symbol =
            super::notification_symbol_v1(b"cui_bridge_set_stepper_fill\0".as_ptr().cast());
        if symbol.is_null() {
            None
        } else {
            Some(std::mem::transmute::<*mut c_void, Set>(symbol))
        }
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn setter() -> Option<Set> {
    None
}

impl App {
    /// Whether this provider exports filled Stepper presentation.
    pub fn supports_stepper_fill(&self) -> bool {
        setter().is_some()
    }
    /// Zero width grows with the parent; zero height uses intrinsic sizing.
    /// A subsequent `stepper_visual` call restores ordinary presentation.
    /// Returns `Error::STEPPER_FILL_UNAVAILABLE` on older providers.
    pub fn stepper_fill(
        &mut self,
        node: Node,
        vertical: bool,
        density: StepperDensity,
        width: f32,
        height: f32,
    ) -> Result<&mut Self> {
        let node = self.valid(node)?;
        if !width.is_finite()
            || !height.is_finite()
            || !(0. ..=4096.).contains(&width)
            || !(0. ..=4096.).contains(&height)
        {
            return Err(Error(3));
        }
        let set = setter().ok_or(Error::STEPPER_FILL_UNAVAILABLE)?;
        check(unsafe {
            set(
                self.raw,
                node,
                vertical as u32,
                density as u32,
                width,
                height,
            )
        })?;
        Ok(self)
    }
}
impl Error {
    /// Selected bridge provider does not export filled Stepper presentation.
    pub const STEPPER_FILL_UNAVAILABLE: Self = Self(0x535446);
}
impl<'a> Builder<'a> {
    pub fn stepper_fill(
        self,
        vertical: bool,
        density: StepperDensity,
        width: f32,
        height: f32,
    ) -> Result<Self> {
        self.app
            .stepper_fill(self.node, vertical, density, width, height)?;
        Ok(self)
    }
}
