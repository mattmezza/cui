use crate::{check, string, App, Builder, DesignSize, Error, Node, RawNode, Result};
use std::{
    ffi::{c_char, c_void},
    sync::OnceLock,
};

type SetBasis = unsafe extern "C" fn(*mut c_void, RawNode, f32) -> u32;
type SetOption = unsafe extern "C" fn(*mut c_void, RawNode, u32) -> u32;
type SetControlSizes = unsafe extern "C" fn(*mut c_void, RawNode, u32, u32) -> u32;
type CreateSlot =
    unsafe extern "C" fn(*mut c_void, RawNode, u32, *const c_char, *mut RawNode) -> u32;

/// The primary direction of a retained Carousel.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum CarouselAxis {
    Horizontal = 0,
    Vertical = 1,
}

/// Opt-in navigation controls rendered below a retained Carousel.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct CarouselControls {
    pub arrows: bool,
    pub pagination: bool,
}

/// The passive content of a retained Carousel control.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u32)]
pub enum CarouselSlot {
    /// Owned by the Carousel.
    Previous = 1,
    /// Owned by the Carousel.
    Next = 2,
    /// Owned by the direct slide Item.
    Marker = 3,
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

fn fit_setter() -> Option<SetOption> {
    static SETTER: OnceLock<Option<SetOption>> = OnceLock::new();
    *SETTER.get_or_init(resolve_fit)
}

#[cfg(cui_bridge_static)]
fn resolve_fit() -> Option<SetOption> {
    extern "C" {
        fn cui_bridge_set_carousel_slide_fit_v1(
            app: *mut c_void,
            item: RawNode,
            enabled: u32,
        ) -> u32;
    }
    Some(cui_bridge_set_carousel_slide_fit_v1)
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve_fit() -> Option<SetOption> {
    unsafe {
        let symbol =
            crate::notification_symbol_v1(c"cui_bridge_set_carousel_slide_fit_v1".as_ptr());
        (!symbol.is_null()).then(|| std::mem::transmute::<*mut c_void, SetOption>(symbol))
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve_fit() -> Option<SetOption> {
    None
}

fn axis_setter() -> Option<SetOption> {
    static SETTER: OnceLock<Option<SetOption>> = OnceLock::new();
    *SETTER.get_or_init(resolve_axis)
}

#[cfg(cui_bridge_static)]
fn resolve_axis() -> Option<SetOption> {
    extern "C" {
        fn cui_bridge_set_carousel_axis_v1(app: *mut c_void, carousel: RawNode, axis: u32) -> u32;
    }
    Some(cui_bridge_set_carousel_axis_v1)
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve_axis() -> Option<SetOption> {
    unsafe {
        let symbol = crate::notification_symbol_v1(c"cui_bridge_set_carousel_axis_v1".as_ptr());
        (!symbol.is_null()).then(|| std::mem::transmute::<*mut c_void, SetOption>(symbol))
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve_axis() -> Option<SetOption> {
    None
}

fn looping_setter() -> Option<SetOption> {
    static SETTER: OnceLock<Option<SetOption>> = OnceLock::new();
    *SETTER.get_or_init(resolve_looping)
}

#[cfg(cui_bridge_static)]
fn resolve_looping() -> Option<SetOption> {
    extern "C" {
        fn cui_bridge_set_carousel_looping_v1(
            app: *mut c_void,
            carousel: RawNode,
            looping: u32,
        ) -> u32;
    }
    Some(cui_bridge_set_carousel_looping_v1)
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve_looping() -> Option<SetOption> {
    unsafe {
        let symbol = crate::notification_symbol_v1(c"cui_bridge_set_carousel_looping_v1".as_ptr());
        (!symbol.is_null()).then(|| std::mem::transmute::<*mut c_void, SetOption>(symbol))
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve_looping() -> Option<SetOption> {
    None
}

fn controls_setter() -> Option<SetOption> {
    static SETTER: OnceLock<Option<SetOption>> = OnceLock::new();
    *SETTER.get_or_init(resolve_controls)
}

#[cfg(cui_bridge_static)]
fn resolve_controls() -> Option<SetOption> {
    extern "C" {
        fn cui_bridge_set_carousel_controls_v1(
            app: *mut c_void,
            carousel: RawNode,
            mask: u32,
        ) -> u32;
    }
    Some(cui_bridge_set_carousel_controls_v1)
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve_controls() -> Option<SetOption> {
    unsafe {
        let symbol = crate::notification_symbol_v1(c"cui_bridge_set_carousel_controls_v1".as_ptr());
        (!symbol.is_null()).then(|| std::mem::transmute::<*mut c_void, SetOption>(symbol))
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve_controls() -> Option<SetOption> {
    None
}

fn control_sizes_setter() -> Option<SetControlSizes> {
    static SETTER: OnceLock<Option<SetControlSizes>> = OnceLock::new();
    *SETTER.get_or_init(resolve_control_sizes)
}

#[cfg(cui_bridge_static)]
fn resolve_control_sizes() -> Option<SetControlSizes> {
    extern "C" {
        fn cui_bridge_set_carousel_control_sizes_v1(
            app: *mut c_void,
            carousel: RawNode,
            arrows: u32,
            pagination: u32,
        ) -> u32;
    }
    Some(cui_bridge_set_carousel_control_sizes_v1)
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve_control_sizes() -> Option<SetControlSizes> {
    unsafe {
        let symbol =
            crate::notification_symbol_v1(c"cui_bridge_set_carousel_control_sizes_v1".as_ptr());
        (!symbol.is_null()).then(|| std::mem::transmute::<*mut c_void, SetControlSizes>(symbol))
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve_control_sizes() -> Option<SetControlSizes> {
    None
}

fn slot_factory() -> Option<CreateSlot> {
    static FACTORY: OnceLock<Option<CreateSlot>> = OnceLock::new();
    *FACTORY.get_or_init(resolve_slot_factory)
}

#[cfg(cui_bridge_static)]
fn resolve_slot_factory() -> Option<CreateSlot> {
    extern "C" {
        fn cui_bridge_carousel_slot_create_v1(
            app: *mut c_void,
            owner: RawNode,
            role: u32,
            key: *const c_char,
            out_slot: *mut RawNode,
        ) -> u32;
    }
    Some(cui_bridge_carousel_slot_create_v1)
}

#[cfg(all(not(cui_bridge_static), any(unix, windows)))]
fn resolve_slot_factory() -> Option<CreateSlot> {
    unsafe {
        let symbol = crate::notification_symbol_v1(c"cui_bridge_carousel_slot_create_v1".as_ptr());
        (!symbol.is_null()).then(|| std::mem::transmute::<*mut c_void, CreateSlot>(symbol))
    }
}

#[cfg(not(any(cui_bridge_static, unix, windows)))]
fn resolve_slot_factory() -> Option<CreateSlot> {
    None
}

impl Error {
    /// The loaded bridge lacks the optional Carousel basis setter.
    pub const CAROUSEL_BASIS_UNAVAILABLE: Self = Self(9);
    /// The loaded bridge lacks the optional Carousel content sizing setter.
    pub const CAROUSEL_FIT_UNAVAILABLE: Self = Self(9);
    /// The loaded bridge lacks the optional Carousel axis setter.
    pub const CAROUSEL_AXIS_UNAVAILABLE: Self = Self(9);
    /// The loaded bridge lacks the optional Carousel looping setter.
    pub const CAROUSEL_LOOPING_UNAVAILABLE: Self = Self(9);
    /// The loaded bridge lacks the optional Carousel controls setter.
    pub const CAROUSEL_CONTROLS_UNAVAILABLE: Self = Self(9);
    /// The loaded bridge lacks the optional Carousel control sizes setter.
    pub const CAROUSEL_CONTROL_SIZES_UNAVAILABLE: Self = Self(9);
    /// The loaded bridge lacks the optional Carousel content-slot factory.
    pub const CAROUSEL_SLOTS_UNAVAILABLE: Self = Self(9);
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

    /// Whether the loaded bridge supports edge wrapping for retained Carousels.
    pub fn supports_carousel_looping(&self) -> bool {
        looping_setter().is_some()
    }

    /// Enable edge wrapping on a track long enough to loop.
    pub fn carousel_looping(&mut self, carousel: Node, enabled: bool) -> Result<&mut Self> {
        let carousel = self.valid(carousel)?;
        let set = looping_setter().ok_or(Error::CAROUSEL_LOOPING_UNAVAILABLE)?;
        check(unsafe { set(self.raw, carousel, u32::from(enabled)) })?;
        Ok(self)
    }

    /// Whether the loaded bridge supports retained Carousel arrows and slide markers.
    pub fn supports_carousel_controls(&self) -> bool {
        controls_setter().is_some()
    }

    /// Whether the loaded bridge supports named sizes for Carousel controls.
    pub fn supports_carousel_control_sizes(&self) -> bool {
        control_sizes_setter().is_some()
    }

    /// Set both control group sizes together. Inherit restores each default.
    pub fn carousel_control_sizes(
        &mut self,
        carousel: Node,
        arrows: DesignSize,
        pagination: DesignSize,
    ) -> Result<&mut Self> {
        let carousel = self.valid(carousel)?;
        match control_sizes_setter() {
            Some(set) => {
                check(unsafe { set(self.raw, carousel, arrows as u32, pagination as u32) })?
            }
            None if arrows == DesignSize::Inherit && pagination == DesignSize::Inherit => {}
            None => return Err(Error::CAROUSEL_CONTROL_SIZES_UNAVAILABLE),
        }
        Ok(self)
    }

    /// Whether the loaded bridge can customize Carousel control content.
    pub fn supports_carousel_slots(&self) -> bool {
        slot_factory().is_some()
    }

    /// Create a passive row for a built-in control. Use the row's text for an
    /// accessible name override and add children for the artwork.
    pub fn build_carousel_slot(
        &mut self,
        owner: Node,
        role: CarouselSlot,
        key: &str,
    ) -> Result<Builder<'_>> {
        let owner = self.valid(owner)?;
        let create = slot_factory().ok_or(Error::CAROUSEL_SLOTS_UNAVAILABLE)?;
        let key = string(key)?;
        let mut raw = RawNode::default();
        check(unsafe { create(self.raw, owner, role as u32, key.as_ptr(), &mut raw) })?;
        let node = Node {
            raw,
            app_id: self.id,
        };
        Ok(Builder { app: self, node })
    }

    /// Configure the built-in controls without changing the selected slide.
    pub fn carousel_controls(
        &mut self,
        carousel: Node,
        options: CarouselControls,
    ) -> Result<&mut Self> {
        let carousel = self.valid(carousel)?;
        let set = controls_setter().ok_or(Error::CAROUSEL_CONTROLS_UNAVAILABLE)?;
        let mask = u32::from(options.arrows) | (u32::from(options.pagination) << 1);
        check(unsafe { set(self.raw, carousel, mask) })?;
        Ok(self)
    }

    /// Whether the loaded bridge supports fractional slide sizing.
    pub fn supports_carousel_basis(&self) -> bool {
        setter().is_some()
    }

    /// Set a direct Carousel Item's viewport fraction in [0, 1] and clear
    /// content sizing. Zero restores the full viewport. This queues no selection notification.
    pub fn carousel_basis(&mut self, item: Node, basis: f32) -> Result<&mut Self> {
        let item = self.valid(item)?;
        let set = setter().ok_or(Error::CAROUSEL_BASIS_UNAVAILABLE)?;
        check(unsafe { set(self.raw, item, basis) })?;
        Ok(self)
    }
    /// Whether the loaded bridge supports content-sized Carousel slides.
    pub fn supports_carousel_fit(&self) -> bool {
        fit_setter().is_some()
    }

    /// Size a direct Carousel Item to its in-flow content. Enabling this
    /// replaces its basis. Disabling it leaves any ordinary basis alone.
    pub fn carousel_fit(&mut self, item: Node, enabled: bool) -> Result<&mut Self> {
        let item = self.valid(item)?;
        if let Some(set) = fit_setter() {
            check(unsafe { set(self.raw, item, u32::from(enabled)) })?;
        } else if enabled {
            return Err(Error::CAROUSEL_FIT_UNAVAILABLE);
        }
        Ok(self)
    }
}

impl Builder<'_> {
    /// Set the direction of the Carousel being built.
    pub fn carousel_axis(self, axis: CarouselAxis) -> Result<Self> {
        self.app.carousel_axis(self.node, axis)?;
        Ok(self)
    }

    /// Enable edge wrapping on the Carousel being built.
    pub fn carousel_looping(self, enabled: bool) -> Result<Self> {
        self.app.carousel_looping(self.node, enabled)?;
        Ok(self)
    }

    /// Configure the built-in controls on the Carousel being built.
    pub fn carousel_controls(self, options: CarouselControls) -> Result<Self> {
        self.app.carousel_controls(self.node, options)?;
        Ok(self)
    }

    /// Set the independent arrow and marker sizes on the retained Carousel.
    pub fn carousel_control_sizes(
        self,
        arrows: DesignSize,
        pagination: DesignSize,
    ) -> Result<Self> {
        self.app
            .carousel_control_sizes(self.node, arrows, pagination)?;
        Ok(self)
    }

    /// Size the Item being built to its in-flow content.
    pub fn carousel_fit(self, enabled: bool) -> Result<Self> {
        self.app.carousel_fit(self.node, enabled)?;
        Ok(self)
    }

    /// Set the fraction on the Item being built inside a Carousel.
    pub fn carousel_basis(self, basis: f32) -> Result<Self> {
        self.app.carousel_basis(self.node, basis)?;
        Ok(self)
    }
}
