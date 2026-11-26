use crate::{check, App, Builder, Error, Node, RawNode, Result};
use std::{ffi::c_void, sync::OnceLock};

type SetBasis = unsafe extern "C" fn(*mut c_void, RawNode, f32) -> u32;

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

impl Error {
    /// The loaded bridge lacks the optional Carousel basis setter.
    pub const CAROUSEL_BASIS_UNAVAILABLE: Self = Self(9);
}

impl App {
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
    /// Set the fraction on the Item being built inside a Carousel.
    pub fn carousel_basis(self, basis: f32) -> Result<Self> {
        self.app.carousel_basis(self.node, basis)?;
        Ok(self)
    }
}
