use crate::{check, App, Builder, Error, Node, RawNode, Result};
use std::{ffi::c_void, sync::OnceLock};

type SetBasis = unsafe extern "C" fn(*mut c_void, RawNode, f32) -> u32;
type SetAxis = unsafe extern "C" fn(*mut c_void, RawNode, u32) -> u32;

/// The primary direction of a retained Carousel.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum CarouselAxis {
    Horizontal = 0,
    Vertical = 1,
}

fn setter() -> Option<SetBasis> {
    static SETTER: OnceLock<Option<SetBasis>> = OnceLock::new();
    *SETTER.get_or_init(resolve)
}

#[cfg(cui_bridge_static)]
fn resolve() -> Option<SetBasis> {
    extern "C" {
        fn cui_bridge_set_carousel_slide_basis_v1(
            app: *mut c_void,
            item: RawNode,
            basis: f32,
        ) -> u32;
    }
    Some(cui_bridge_set_carousel_slide_basis_v1)
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve() -> Option<SetBasis> {
    unsafe {
        let symbol =
            crate::notification_symbol_v1(c"cui_bridge_set_carousel_slide_basis_v1".as_ptr());
        (!symbol.is_null()).then(|| std::mem::transmute::<*mut c_void, SetBasis>(symbol))
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve() -> Option<SetBasis> {
    None
}

fn axis_setter() -> Option<SetAxis> {
    static SETTER: OnceLock<Option<SetAxis>> = OnceLock::new();
    *SETTER.get_or_init(resolve_axis)
}

#[cfg(cui_bridge_static)]
fn resolve_axis() -> Option<SetAxis> {
    extern "C" {
        fn cui_bridge_set_carousel_axis_v1(app: *mut c_void, carousel: RawNode, axis: u32) -> u32;
    }
    Some(cui_bridge_set_carousel_axis_v1)
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve_axis() -> Option<SetAxis> {
    unsafe {
        let symbol = crate::notification_symbol_v1(c"cui_bridge_set_carousel_axis_v1".as_ptr());
        (!symbol.is_null()).then(|| std::mem::transmute::<*mut c_void, SetAxis>(symbol))
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve_axis() -> Option<SetAxis> {
    None
}

impl Error {
    /// The loaded bridge lacks the optional Carousel basis setter.
    pub const CAROUSEL_BASIS_UNAVAILABLE: Self = Self(9);
    /// The loaded bridge lacks the optional Carousel axis setter.
    pub const CAROUSEL_AXIS_UNAVAILABLE: Self = Self(9);
}

impl App {
    /// Whether the loaded bridge supports vertical layout for retained Carousels.
    pub fn supports_carousel_axis(&self) -> bool {
        axis_setter().is_some()
    }

    /// Set the Carousel's direction for the next frame without changing its selection.
    pub fn carousel_axis(&mut self, carousel: Node, axis: CarouselAxis) -> Result<&mut Self> {
        let carousel = self.valid(carousel)?;
        let set = axis_setter().ok_or(Error::CAROUSEL_AXIS_UNAVAILABLE)?;
        check(unsafe { set(self.raw, carousel, axis as u32) })?;
        Ok(self)
    }

    /// Whether the loaded bridge supports fractional slide sizing.
    pub fn supports_carousel_basis(&self) -> bool {
        setter().is_some()
    }

    /// Set a direct Carousel Item's viewport fraction in [0, 1].
    /// Zero restores the full viewport. This queues no selection notification.
    pub fn carousel_basis(&mut self, item: Node, basis: f32) -> Result<&mut Self> {
        let item = self.valid(item)?;
        let set = setter().ok_or(Error::CAROUSEL_BASIS_UNAVAILABLE)?;
        check(unsafe { set(self.raw, item, basis) })?;
        Ok(self)
    }
}

impl Builder<'_> {
    /// Set the direction of the Carousel being built.
    pub fn carousel_axis(self, axis: CarouselAxis) -> Result<Self> {
        self.app.carousel_axis(self.node, axis)?;
        Ok(self)
    }

    /// Set the fraction on the Item being built inside a Carousel.
    pub fn carousel_basis(self, basis: f32) -> Result<Self> {
        self.app.carousel_basis(self.node, basis)?;
        Ok(self)
    }
}
