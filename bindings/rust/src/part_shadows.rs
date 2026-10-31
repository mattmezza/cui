//! Bounded semantic paint-part authoring. Query reports capability, not presence.
use super::{
    check, App, Builder, CornerRadii, Error, Node, RawNode, Result, ShadowPreset, ShadowStyle,
};
use std::ffi::c_void;
#[repr(u32)]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ShadowPart {
    Root = 0,
    Main = 1,
    Indicator = 2,
    Track = 3,
    Thumb = 4,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ShadowTargetInfo {
    pub canonical_part: Option<ShadowPart>,
    pub flags: u32,
}
impl ShadowTargetInfo {
    pub fn supported(self) -> bool {
        self.flags & 1 != 0
    }
}
#[repr(C)]
#[derive(Clone, Copy)]
struct Target {
    size: u32,
    schema: u32,
    role: u32,
    reserved: u32,
}
#[repr(C)]
#[derive(Default)]
struct Info {
    size: u32,
    schema: u32,
    role: u32,
    flags: u32,
}
const _: () = {
    assert!(std::mem::size_of::<Target>() == 16);
    assert!(std::mem::offset_of!(Target, role) == 8);
    assert!(std::mem::size_of::<Info>() == 16);
    assert!(std::mem::offset_of!(Info, flags) == 12);
};
type Set =
    unsafe extern "C" fn(*mut c_void, RawNode, u32, *const Target, u32, *const c_void) -> u32;
type Reset = unsafe extern "C" fn(*mut c_void, RawNode, u32, *const Target) -> u32;
type Query = unsafe extern "C" fn(*mut c_void, RawNode, u32, *const Target, u32, *mut Info) -> u32;
struct Api {
    set: Set,
    reset: Reset,
    query: Query,
}
#[cfg(cui_bridge_static)]
fn functions() -> Result<Api> {
    extern "C" {
        fn cui_bridge_set_target_box_shadows_v1(
            a: *mut c_void,
            n: RawNode,
            size: u32,
            t: *const Target,
            style_size: u32,
            style: *const c_void,
        ) -> u32;
        fn cui_bridge_reset_target_box_shadows_v1(
            a: *mut c_void,
            n: RawNode,
            size: u32,
            t: *const Target,
        ) -> u32;
        fn cui_bridge_shadow_target_query_v1(
            a: *mut c_void,
            n: RawNode,
            size: u32,
            t: *const Target,
            info_size: u32,
            out: *mut Info,
        ) -> u32;
    }
    Ok(Api {
        set: cui_bridge_set_target_box_shadows_v1,
        reset: cui_bridge_reset_target_box_shadows_v1,
        query: cui_bridge_shadow_target_query_v1,
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
        Ok(Api {
            set: resolve!("cui_bridge_set_target_box_shadows_v1", Set),
            reset: resolve!("cui_bridge_reset_target_box_shadows_v1", Reset),
            query: resolve!("cui_bridge_shadow_target_query_v1", Query),
        })
    }
}
#[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
fn functions() -> Result<Api> {
    Err(Error(9))
}
fn target(part: ShadowPart) -> Target {
    Target {
        size: 16,
        schema: 1,
        role: part as u32,
        reserved: 0,
    }
}
impl App {
    pub fn part_box_shadows(
        &mut self,
        n: Node,
        part: ShadowPart,
        style: &ShadowStyle,
    ) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let value = style.wire()?;
        let api = functions()?;
        let t = target(part);
        check(unsafe {
            (api.set)(
                self.raw,
                n,
                16,
                &t,
                224,
                (&value as *const super::shadows::Style).cast(),
            )
        })?;
        Ok(self)
    }
    pub fn reset_part_box_shadows(&mut self, n: Node, part: ShadowPart) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let api = functions()?;
        let t = target(part);
        check(unsafe { (api.reset)(self.raw, n, 16, &t) })?;
        Ok(self)
    }
    pub fn part_shadow_preset(
        &mut self,
        n: Node,
        part: ShadowPart,
        preset: ShadowPreset,
        radii: CornerRadii,
    ) -> Result<&mut Self> {
        self.valid(n)?;
        let value = ShadowStyle::preset(preset, radii)?;
        self.part_box_shadows(n, part, &value)
    }
    pub fn shadow_target(&self, n: Node, part: ShadowPart) -> Result<ShadowTargetInfo> {
        let n = self.valid(n)?;
        let api = functions()?;
        let t = target(part);
        let mut info = Info::default();
        check(unsafe { (api.query)(self.raw, n, 16, &t, 16, &mut info) })?;
        let canonical_part = match info.role {
            0 => Some(ShadowPart::Root),
            1 => Some(ShadowPart::Main),
            2 => Some(ShadowPart::Indicator),
            3 => Some(ShadowPart::Track),
            4 => Some(ShadowPart::Thumb),
            _ => None,
        };
        Ok(ShadowTargetInfo {
            canonical_part,
            flags: info.flags,
        })
    }
}
impl<'a> Builder<'a> {
    pub fn part_box_shadows(self, part: ShadowPart, style: &ShadowStyle) -> Result<Self> {
        self.app.part_box_shadows(self.node, part, style)?;
        Ok(self)
    }
    pub fn reset_part_box_shadows(self, part: ShadowPart) -> Result<Self> {
        self.app.reset_part_box_shadows(self.node, part)?;
        Ok(self)
    }
    pub fn part_shadow_preset(
        self,
        part: ShadowPart,
        preset: ShadowPreset,
        radii: CornerRadii,
    ) -> Result<Self> {
        self.app
            .part_shadow_preset(self.node, part, preset, radii)?;
        Ok(self)
    }
}
