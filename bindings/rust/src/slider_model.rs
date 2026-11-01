//! Owned controlled Single/Range bridge subset. Direct C Styled options remain unbound.
use super::{check, string, App, Error, Key, Node, Phase, RawNode, Rect, Result};
use std::{
    cell::RefCell,
    collections::HashMap,
    ffi::{c_char, c_void},
    rc::Rc,
};

/// Owned double values. A Single has no invented start value.
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum SliderValue {
    Single { end: f64 },
    Range { start: f64, end: f64 },
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SliderScale {
    Linear,
    Logarithmic,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SliderOrientation {
    Horizontal,
    Vertical,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SliderLayout {
    Fixed,
    Parent,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SliderPhase {
    Begin,
    Update,
    Commit,
    Cancel,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SliderPart {
    Root,
    Track,
    Fill,
    StartThumb,
    EndThumb,
}
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SliderDomain {
    pub minimum: f64,
    pub maximum: f64,
    pub step: f64,
    pub scale: SliderScale,
}
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SliderConfig {
    pub domain: SliderDomain,
    pub source_generation: u64,
    pub orientation: SliderOrientation,
    pub layout: SliderLayout,
    pub rem: f32,
    pub length: f32,
    pub requests: bool,
    pub readonly: bool,
    pub reverse_fill: bool,
    pub reduced_motion: bool,
    pub hide_focus: bool,
}
impl Default for SliderConfig {
    fn default() -> Self {
        Self {
            domain: SliderDomain {
                minimum: 0.,
                maximum: 100.,
                step: 1.,
                scale: SliderScale::Linear,
            },
            source_generation: 0,
            orientation: SliderOrientation::Horizontal,
            layout: SliderLayout::Fixed,
            rem: 16.,
            length: 200.,
            requests: true,
            readonly: false,
            reverse_fill: false,
            reduced_motion: false,
            hide_focus: false,
        }
    }
}
/// Inert metadata; constructing/copying it cannot manufacture receipt authority.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct SliderReceiptInfo {
    pub app_epoch: u64,
    pub slot: u32,
    pub generation: u32,
    pub source_generation: u64,
    pub configuration_revision: u64,
    pub submission_frame: u64,
    pub request_sequence: u64,
}
#[derive(Debug)]
struct Namespace {
    _nonzero: u8,
}
/// Clone retains issued authority within the same App and loaded bridge image.
/// Does not retain the App or native storage; validity still expires in C.
///
/// ```compile_fail
/// fn require_send<T: Send>() {}
/// require_send::<cui_builders::SliderReceipt>();
/// ```
#[derive(Clone, Debug)]
pub struct SliderReceipt {
    raw: RawReceipt,
    namespace: Rc<Namespace>,
}
impl SliderReceipt {
    pub fn info(&self) -> SliderReceiptInfo {
        let r = self.raw;
        SliderReceiptInfo {
            app_epoch: r.app_epoch,
            slot: r.node.slot,
            generation: r.node.generation,
            source_generation: r.source_generation,
            configuration_revision: r.configuration_revision,
            submission_frame: r.submission_frame,
            request_sequence: r.request_sequence,
        }
    }
}
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SliderInput {
    pub input_type: u32,
    pub flags: u32,
    pub key: u32,
    pub modifiers: u32,
    pub x: f32,
    pub y: f32,
    pub repeat: u32,
    pub pointer_type: u32,
    pub pointer_button: u32,
    pub accessibility_action: u32,
    pub pointer_id: u64,
    pub pointer_device: u64,
    pub accessibility_target: u64,
    pub accessibility_value: f64,
}
#[derive(Clone, Debug)]
pub struct SliderRequest {
    pub phase: SliderPhase,
    pub reason: u32,
    pub thumb: u32,
    pub event_kind: u32,
    pub event_source: u32,
    pub event_flags: u32,
    pub receipt: SliderReceipt,
    pub event_target: u64,
    pub event_frame: u64,
    pub event_index: u64,
    pub current: SliderValue,
    pub requested: SliderValue,
    pub domain: SliderDomain,
    pub gesture_domain: SliderDomain,
    pub input: SliderInput,
}
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SliderPresentation {
    pub render_status: u32,
    pub core_time: f64,
    pub next_wake_seconds: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct RawValue {
    kind: u32,
    reserved: u32,
    start: f64,
    end: f64,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct RawDomain {
    minimum: f64,
    maximum: f64,
    step: f64,
    scale: u32,
    reserved: u32,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct RawConfig {
    size: u32,
    schema: u32,
    flags: u32,
    orientation: u32,
    source_generation: u64,
    domain: RawDomain,
    rem: f32,
    length: f32,
    layout: u32,
    reserved: u32,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct RawQueue {
    size: u32,
    schema: u32,
    capacity: u32,
    reserved: u32,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct RawReceipt {
    size: u32,
    schema: u32,
    app_epoch: u64,
    node: RawNode,
    source_generation: u64,
    configuration_revision: u64,
    submission_frame: u64,
    request_sequence: u64,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct RawInput {
    input_type: u32,
    flags: u32,
    key: u32,
    modifiers: u32,
    x: f32,
    y: f32,
    repeat: u32,
    pointer_type: u32,
    pointer_button: u32,
    accessibility_action: u32,
    pointer_id: u64,
    pointer_device: u64,
    accessibility_target: u64,
    accessibility_value: f64,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct RawRequest {
    size: u32,
    schema: u32,
    phase: u32,
    reason: u32,
    thumb: u32,
    event_kind: u32,
    event_source: u32,
    event_flags: u32,
    receipt: RawReceipt,
    event_target: u64,
    event_frame: u64,
    event_index: u64,
    current: RawValue,
    requested: RawValue,
    domain: RawDomain,
    gesture_domain: RawDomain,
    input: RawInput,
}
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
struct RawPresentation {
    size: u32,
    schema: u32,
    render_status: u32,
    reserved: u32,
    core_time: f64,
    next_wake_seconds: f64,
}
const _: [(); 24] = [(); std::mem::size_of::<RawValue>()];
const _: [(); 32] = [(); std::mem::size_of::<RawDomain>()];
const _: [(); 72] = [(); std::mem::size_of::<RawConfig>()];
const _: [(); 16] = [(); std::mem::size_of::<RawQueue>()];
const _: [(); 56] = [(); std::mem::size_of::<RawReceipt>()];
const _: [(); 72] = [(); std::mem::size_of::<RawInput>()];
const _: [(); 296] = [(); std::mem::size_of::<RawRequest>()];
const _: [(); 32] = [(); std::mem::size_of::<RawPresentation>()];
const _: [(); 112] = [(); std::mem::offset_of!(RawRequest, current)];
const _: [(); 224] = [(); std::mem::offset_of!(RawRequest, input)];
const _: [(); 24] = [(); std::mem::offset_of!(RawConfig, domain)];

const INVALID: Error = Error(3);
const PROTOCOL: Error = Error(7);
fn ensure(ok: bool) -> Result<()> {
    if ok {
        Ok(())
    } else {
        Err(INVALID)
    }
}
impl SliderValue {
    fn wire(self) -> Result<RawValue> {
        let (kind, start, end) = match self {
            Self::Single { end } => (0, 0., end),
            Self::Range { start, end } => (1, start, end),
        };
        ensure(start.is_finite() && end.is_finite() && (kind == 0 || start <= end))?;
        Ok(RawValue {
            kind,
            reserved: 0,
            start,
            end,
        })
    }
    fn in_domain(self, d: SliderDomain) -> Result<RawValue> {
        let r = self.wire()?;
        ensure(
            r.end >= d.minimum
                && r.end <= d.maximum
                && (r.kind == 0 || (r.start >= d.minimum && r.start <= d.maximum)),
        )?;
        Ok(r)
    }
}
impl SliderDomain {
    fn wire(self) -> Result<RawDomain> {
        ensure(
            self.minimum.is_finite()
                && self.maximum.is_finite()
                && self.step.is_finite()
                && self.minimum <= self.maximum
                && (self.maximum - self.minimum).is_finite()
                && self.step >= 0.
                && (self.scale != SliderScale::Logarithmic
                    || (self.minimum > 0. && self.maximum > self.minimum)),
        )?;
        Ok(RawDomain {
            minimum: self.minimum,
            maximum: self.maximum,
            step: self.step,
            scale: self.scale as u32,
            reserved: 0,
        })
    }
}
impl SliderConfig {
    fn wire(self) -> Result<RawConfig> {
        let length = if self.layout == SliderLayout::Parent {
            0.
        } else {
            self.length
        };
        ensure(
            self.rem.is_finite()
                && self.rem > 0.
                && length.is_finite()
                && (self.layout != SliderLayout::Fixed || length > 0.),
        )?;
        Ok(RawConfig {
            size: 72,
            schema: 1,
            flags: self.requests as u32
                | ((self.readonly as u32) << 1)
                | ((self.reverse_fill as u32) << 2)
                | ((self.reduced_motion as u32) << 3)
                | ((self.hide_focus as u32) << 4),
            orientation: self.orientation as u32,
            source_generation: self.source_generation,
            domain: self.domain.wire()?,
            rem: self.rem,
            length,
            layout: self.layout as u32,
            reserved: 0,
        })
    }
}
fn read_value(r: RawValue) -> Result<SliderValue> {
    if r.reserved != 0 || r.kind > 1 || (r.kind == 0 && r.start != 0.) {
        return Err(PROTOCOL);
    }
    let v = if r.kind == 0 {
        SliderValue::Single { end: r.end }
    } else {
        SliderValue::Range {
            start: r.start,
            end: r.end,
        }
    };
    v.wire().map_err(|_| PROTOCOL)?;
    Ok(v)
}
fn read_domain(r: RawDomain) -> Result<SliderDomain> {
    if r.reserved != 0 || r.scale > 1 {
        return Err(PROTOCOL);
    }
    let d = SliderDomain {
        minimum: r.minimum,
        maximum: r.maximum,
        step: r.step,
        scale: if r.scale == 0 {
            SliderScale::Linear
        } else {
            SliderScale::Logarithmic
        },
    };
    d.wire().map_err(|_| PROTOCOL)?;
    Ok(d)
}
fn read_config(r: RawConfig) -> Result<SliderConfig> {
    if r.size != 72
        || r.schema != 1
        || r.reserved != 0
        || r.flags & !31 != 0
        || r.orientation > 1
        || r.layout > 1
        || (r.layout == 1 && r.length != 0.)
    {
        return Err(PROTOCOL);
    }
    let c = SliderConfig {
        domain: read_domain(r.domain)?,
        source_generation: r.source_generation,
        orientation: if r.orientation == 0 {
            SliderOrientation::Horizontal
        } else {
            SliderOrientation::Vertical
        },
        layout: if r.layout == 0 {
            SliderLayout::Fixed
        } else {
            SliderLayout::Parent
        },
        rem: r.rem,
        length: r.length,
        requests: r.flags & 1 != 0,
        readonly: r.flags & 2 != 0,
        reverse_fill: r.flags & 4 != 0,
        reduced_motion: r.flags & 8 != 0,
        hide_focus: r.flags & 16 != 0,
    };
    c.wire().map_err(|_| PROTOCOL)?;
    Ok(c)
}
fn read_input(i: RawInput) -> Result<SliderInput> {
    if i.flags & !3 != 0
        || i.modifiers & !15 != 0
        || !i.x.is_finite()
        || !i.y.is_finite()
        || !i.accessibility_value.is_finite()
    {
        return Err(PROTOCOL);
    }
    if i.flags & 1 == 0 {
        if i.input_type != 0
            || i.flags != 0
            || i.key != 0
            || i.modifiers != 0
            || i.x != 0.
            || i.y != 0.
            || i.repeat != 0
            || i.pointer_type != 0
            || i.pointer_button != 0
            || i.accessibility_action != 0
            || i.pointer_id != 0
            || i.pointer_device != 0
            || i.accessibility_target != 0
            || i.accessibility_value != 0.
        {
            return Err(PROTOCOL);
        }
    } else if !matches!(i.input_type, 0 | 1 | 2 | 3 | 7 | 13)
        || !matches!(i.key, 0 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 19 | 20)
        || i.repeat > 1
        || i.pointer_type > 2
        || i.pointer_button > 2
        || !matches!(i.accessibility_action, 0 | 2 | 3 | 4)
    {
        return Err(PROTOCOL);
    }
    if i.flags & 2 == 0 && i.accessibility_value != 0. {
        return Err(PROTOCOL);
    }
    if i.flags & 2 != 0 && (i.flags & 1 == 0 || i.input_type != 13 || i.accessibility_action != 4) {
        return Err(PROTOCOL);
    }
    Ok(SliderInput {
        input_type: i.input_type,
        flags: i.flags,
        key: i.key,
        modifiers: i.modifiers,
        x: i.x,
        y: i.y,
        repeat: i.repeat,
        pointer_type: i.pointer_type,
        pointer_button: i.pointer_button,
        accessibility_action: i.accessibility_action,
        pointer_id: i.pointer_id,
        pointer_device: i.pointer_device,
        accessibility_target: i.accessibility_target,
        accessibility_value: i.accessibility_value,
    })
}
fn read_request(w: RawRequest, namespace: Rc<Namespace>) -> Result<SliderRequest> {
    if w.size != 296
        || w.schema != 1
        || w.phase > 3
        || w.reason > 9
        || w.thumb > 1
        || !matches!(w.event_kind, 1 | 3 | 5)
        || w.event_source > 4
        || w.event_flags & !7 != 0
        || w.receipt.size != 56
        || w.receipt.schema != 1
        || (w.event_flags & 4 == 0 && w.event_source != 0)
        || (w.event_flags & 1 == 0 && w.event_frame != 0)
        || (w.event_flags & 2 == 0 && w.event_index != u64::MAX)
    {
        return Err(PROTOCOL);
    }
    Ok(SliderRequest {
        phase: match w.phase {
            0 => SliderPhase::Begin,
            1 => SliderPhase::Update,
            2 => SliderPhase::Commit,
            _ => SliderPhase::Cancel,
        },
        reason: w.reason,
        thumb: w.thumb,
        event_kind: w.event_kind,
        event_source: w.event_source,
        event_flags: w.event_flags,
        receipt: SliderReceipt {
            raw: w.receipt,
            namespace,
        },
        event_target: w.event_target,
        event_frame: w.event_frame,
        event_index: w.event_index,
        current: read_value(w.current)?,
        requested: read_value(w.requested)?,
        domain: read_domain(w.domain)?,
        gesture_domain: read_domain(w.gesture_domain)?,
        input: read_input(w.input)?,
    })
}

#[derive(Clone, Copy)]
struct Api {
    versions: unsafe extern "C" fn() -> u32,
    config_size: unsafe extern "C" fn() -> u32,
    value_size: unsafe extern "C" fn() -> u32,
    receipt_size: unsafe extern "C" fn() -> u32,
    request_size: unsafe extern "C" fn() -> u32,
    presentation_size: unsafe extern "C" fn() -> u32,
    enable: unsafe extern "C" fn(*mut c_void, u32, *const RawQueue) -> u32,
    create: unsafe extern "C" fn(
        *mut c_void,
        RawNode,
        *const c_char,
        u32,
        *const RawConfig,
        u32,
        *const RawValue,
        *mut RawNode,
    ) -> u32,
    configure: unsafe extern "C" fn(
        *mut c_void,
        RawNode,
        u32,
        *const RawConfig,
        u32,
        *const RawValue,
    ) -> u32,
    config: unsafe extern "C" fn(*mut c_void, RawNode, u32, *mut RawConfig) -> u32,
    set: unsafe extern "C" fn(*mut c_void, RawNode, u32, *const RawValue) -> u32,
    get: unsafe extern "C" fn(*mut c_void, RawNode, u32, *mut RawValue) -> u32,
    poll: unsafe extern "C" fn(*mut c_void, u32, *mut RawRequest) -> u32,
    accept: unsafe extern "C" fn(*mut c_void, u32, *const RawReceipt, u32, *const RawValue) -> u32,
    presentation: unsafe extern "C" fn(*mut c_void, RawNode, u32, *mut RawPresentation) -> u32,
    bounds: unsafe extern "C" fn(*mut c_void, RawNode, u32, *mut Rect) -> u32,
}
#[cfg(cui_bridge_static)]
extern "C" {
    fn cui_bridge_slider_model_versions() -> u32;
    fn cui_bridge_slider_config_v1_size() -> u32;
    fn cui_bridge_slider_value_v1_size() -> u32;
    fn cui_bridge_slider_receipt_v1_size() -> u32;
    fn cui_bridge_slider_request_v1_size() -> u32;
    fn cui_bridge_slider_presentation_v1_size() -> u32;
    fn cui_bridge_slider_queue_enable_v1(a0: *mut c_void, a1: u32, a2: *const RawQueue) -> u32;
    fn cui_bridge_slider_model_create_v1(
        a0: *mut c_void,
        a1: RawNode,
        a2: *const c_char,
        a3: u32,
        a4: *const RawConfig,
        a5: u32,
        a6: *const RawValue,
        a7: *mut RawNode,
    ) -> u32;
    fn cui_bridge_slider_model_configure_v1(
        a0: *mut c_void,
        a1: RawNode,
        a2: u32,
        a3: *const RawConfig,
        a4: u32,
        a5: *const RawValue,
    ) -> u32;
    fn cui_bridge_slider_model_get_config_v1(
        a0: *mut c_void,
        a1: RawNode,
        a2: u32,
        a3: *mut RawConfig,
    ) -> u32;
    fn cui_bridge_slider_model_set_value_v1(
        a0: *mut c_void,
        a1: RawNode,
        a2: u32,
        a3: *const RawValue,
    ) -> u32;
    fn cui_bridge_slider_model_get_value_v1(
        a0: *mut c_void,
        a1: RawNode,
        a2: u32,
        a3: *mut RawValue,
    ) -> u32;
    fn cui_bridge_poll_slider_request_v1(a0: *mut c_void, a1: u32, a2: *mut RawRequest) -> u32;
    fn cui_bridge_accept_slider_request_v1(
        a0: *mut c_void,
        a1: u32,
        a2: *const RawReceipt,
        a3: u32,
        a4: *const RawValue,
    ) -> u32;
    fn cui_bridge_slider_model_get_presentation_v1(
        a0: *mut c_void,
        a1: RawNode,
        a2: u32,
        a3: *mut RawPresentation,
    ) -> u32;
    fn cui_bridge_slider_model_part_bounds_v1(
        a0: *mut c_void,
        a1: RawNode,
        a2: u32,
        a3: *mut Rect,
    ) -> u32;
}
fn api() -> Option<Api> {
    #[cfg(cui_bridge_static)]
    let a = Api {
        versions: cui_bridge_slider_model_versions,
        config_size: cui_bridge_slider_config_v1_size,
        value_size: cui_bridge_slider_value_v1_size,
        receipt_size: cui_bridge_slider_receipt_v1_size,
        request_size: cui_bridge_slider_request_v1_size,
        presentation_size: cui_bridge_slider_presentation_v1_size,
        enable: cui_bridge_slider_queue_enable_v1,
        create: cui_bridge_slider_model_create_v1,
        configure: cui_bridge_slider_model_configure_v1,
        config: cui_bridge_slider_model_get_config_v1,
        set: cui_bridge_slider_model_set_value_v1,
        get: cui_bridge_slider_model_get_value_v1,
        poll: cui_bridge_poll_slider_request_v1,
        accept: cui_bridge_accept_slider_request_v1,
        presentation: cui_bridge_slider_model_get_presentation_v1,
        bounds: cui_bridge_slider_model_part_bounds_v1,
    };
    #[cfg(all(not(cui_bridge_static), any(unix, windows)))]
    let a = unsafe {
        let versions =
            super::notification_symbol_v1(b"cui_bridge_slider_model_versions\0".as_ptr().cast());
        if versions.is_null() {
            return None;
        }
        let config_size =
            super::notification_symbol_v1(b"cui_bridge_slider_config_v1_size\0".as_ptr().cast());
        if config_size.is_null() {
            return None;
        }
        let value_size =
            super::notification_symbol_v1(b"cui_bridge_slider_value_v1_size\0".as_ptr().cast());
        if value_size.is_null() {
            return None;
        }
        let receipt_size =
            super::notification_symbol_v1(b"cui_bridge_slider_receipt_v1_size\0".as_ptr().cast());
        if receipt_size.is_null() {
            return None;
        }
        let request_size =
            super::notification_symbol_v1(b"cui_bridge_slider_request_v1_size\0".as_ptr().cast());
        if request_size.is_null() {
            return None;
        }
        let presentation_size = super::notification_symbol_v1(
            b"cui_bridge_slider_presentation_v1_size\0".as_ptr().cast(),
        );
        if presentation_size.is_null() {
            return None;
        }
        let enable =
            super::notification_symbol_v1(b"cui_bridge_slider_queue_enable_v1\0".as_ptr().cast());
        if enable.is_null() {
            return None;
        }
        let create =
            super::notification_symbol_v1(b"cui_bridge_slider_model_create_v1\0".as_ptr().cast());
        if create.is_null() {
            return None;
        }
        let configure = super::notification_symbol_v1(
            b"cui_bridge_slider_model_configure_v1\0".as_ptr().cast(),
        );
        if configure.is_null() {
            return None;
        }
        let config = super::notification_symbol_v1(
            b"cui_bridge_slider_model_get_config_v1\0".as_ptr().cast(),
        );
        if config.is_null() {
            return None;
        }
        let set = super::notification_symbol_v1(
            b"cui_bridge_slider_model_set_value_v1\0".as_ptr().cast(),
        );
        if set.is_null() {
            return None;
        }
        let get = super::notification_symbol_v1(
            b"cui_bridge_slider_model_get_value_v1\0".as_ptr().cast(),
        );
        if get.is_null() {
            return None;
        }
        let poll =
            super::notification_symbol_v1(b"cui_bridge_poll_slider_request_v1\0".as_ptr().cast());
        if poll.is_null() {
            return None;
        }
        let accept =
            super::notification_symbol_v1(b"cui_bridge_accept_slider_request_v1\0".as_ptr().cast());
        if accept.is_null() {
            return None;
        }
        let presentation = super::notification_symbol_v1(
            b"cui_bridge_slider_model_get_presentation_v1\0"
                .as_ptr()
                .cast(),
        );
        if presentation.is_null() {
            return None;
        }
        let bounds = super::notification_symbol_v1(
            b"cui_bridge_slider_model_part_bounds_v1\0".as_ptr().cast(),
        );
        if bounds.is_null() {
            return None;
        }
        Api {
            versions: std::mem::transmute::<*mut c_void, unsafe extern "C" fn() -> u32>(versions),
            config_size: std::mem::transmute::<*mut c_void, unsafe extern "C" fn() -> u32>(
                config_size,
            ),
            value_size: std::mem::transmute::<*mut c_void, unsafe extern "C" fn() -> u32>(
                value_size,
            ),
            receipt_size: std::mem::transmute::<*mut c_void, unsafe extern "C" fn() -> u32>(
                receipt_size,
            ),
            request_size: std::mem::transmute::<*mut c_void, unsafe extern "C" fn() -> u32>(
                request_size,
            ),
            presentation_size: std::mem::transmute::<*mut c_void, unsafe extern "C" fn() -> u32>(
                presentation_size,
            ),
            enable: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(*mut c_void, u32, *const RawQueue) -> u32,
            >(enable),
            create: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(
                    *mut c_void,
                    RawNode,
                    *const c_char,
                    u32,
                    *const RawConfig,
                    u32,
                    *const RawValue,
                    *mut RawNode,
                ) -> u32,
            >(create),
            configure: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(
                    *mut c_void,
                    RawNode,
                    u32,
                    *const RawConfig,
                    u32,
                    *const RawValue,
                ) -> u32,
            >(configure),
            config: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(*mut c_void, RawNode, u32, *mut RawConfig) -> u32,
            >(config),
            set: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(*mut c_void, RawNode, u32, *const RawValue) -> u32,
            >(set),
            get: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(*mut c_void, RawNode, u32, *mut RawValue) -> u32,
            >(get),
            poll: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(*mut c_void, u32, *mut RawRequest) -> u32,
            >(poll),
            accept: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(
                    *mut c_void,
                    u32,
                    *const RawReceipt,
                    u32,
                    *const RawValue,
                ) -> u32,
            >(accept),
            presentation: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(*mut c_void, RawNode, u32, *mut RawPresentation) -> u32,
            >(presentation),
            bounds: std::mem::transmute::<
                *mut c_void,
                unsafe extern "C" fn(*mut c_void, RawNode, u32, *mut Rect) -> u32,
            >(bounds),
        }
    };
    #[cfg(any(cui_bridge_static, unix, windows))]
    {
        if unsafe { (a.versions)() } & 1 == 0
            || unsafe { (a.config_size)() } != 72
            || unsafe { (a.value_size)() } != 24
            || unsafe { (a.receipt_size)() } != 56
            || unsafe { (a.request_size)() } != 296
            || unsafe { (a.presentation_size)() } != 32
        {
            return None;
        }
        Some(a)
    }
    #[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
    {
        None
    }
}

type Handler = Box<dyn for<'a> FnMut(&mut SliderRequestContext<'a>, SliderRequest) -> Result<()>>;
pub(super) struct State {
    namespace: Rc<Namespace>,
    handlers: HashMap<RawNode, Rc<RefCell<Handler>>>,
    pub(super) dispatching: bool,
    closing: bool,
}
impl State {
    fn new() -> Self {
        Self {
            namespace: Rc::new(Namespace { _nonzero: 1 }),
            handlers: HashMap::new(),
            dispatching: false,
            closing: false,
        }
    }
}

impl App {
    /// Complete optional V1 group plus exact record sizes, without allocating a queue.
    /// Dynamic old libraries return false. Static mode requires the matching bundled
    /// source SDK; arbitrary older static archives are not a compatibility promise.
    pub fn supports_slider_model(&self) -> bool {
        api().is_some()
    }
    pub fn enable_slider_requests(&mut self, capacity: u32) -> Result<()> {
        ensure(capacity <= 4096)?;
        let a = api().ok_or(Error(9))?;
        let q = RawQueue {
            size: 16,
            schema: 1,
            capacity,
            reserved: 0,
        };
        check(unsafe { (a.enable)(self.raw, 16, &q) })?;
        if self.slider.is_none() {
            self.slider = Some(State::new())
        }
        Ok(())
    }
    /// Explicit existing Row/Column parent; rejection leaves existing UI untouched.
    /// Call enable_slider_requests first; this factory never implicitly allocates a FIFO.
    pub fn slider_model(
        &mut self,
        parent: Node,
        key: &str,
        config: SliderConfig,
        value: SliderValue,
    ) -> Result<Node> {
        let parent = self.valid(parent)?;
        let key = string(key)?;
        let c = config.wire()?;
        let v = value.in_domain(config.domain)?;
        let a = api().ok_or(Error(9))?;
        let mut raw = RawNode::default();
        check(unsafe { (a.create)(self.raw, parent, key.as_ptr(), 72, &c, 24, &v, &mut raw) })?;
        Ok(Node {
            raw,
            app_id: self.id,
        })
    }
    pub fn configure_slider(
        &mut self,
        node: Node,
        config: SliderConfig,
        value: SliderValue,
    ) -> Result<()> {
        let n = self.valid(node)?;
        let c = config.wire()?;
        let v = value.in_domain(config.domain)?;
        let a = api().ok_or(Error(9))?;
        check(unsafe { (a.configure)(self.raw, n, 72, &c, 24, &v) })
    }
    pub fn set_slider_value(&mut self, node: Node, value: SliderValue) -> Result<()> {
        let n = self.valid(node)?;
        let v = value.wire()?;
        let a = api().ok_or(Error(9))?;
        check(unsafe { (a.set)(self.raw, n, 24, &v) })
    }
    pub fn slider_value(&self, node: Node) -> Result<SliderValue> {
        let n = self.valid(node)?;
        let a = api().ok_or(Error(9))?;
        let mut v = RawValue::default();
        check(unsafe { (a.get)(self.raw, n, 24, &mut v) })?;
        read_value(v)
    }
    pub fn slider_config(&self, node: Node) -> Result<SliderConfig> {
        let n = self.valid(node)?;
        let a = api().ok_or(Error(9))?;
        let mut c = RawConfig::default();
        check(unsafe { (a.config)(self.raw, n, 72, &mut c) })?;
        read_config(c)
    }
    pub fn slider_part_bounds(&self, node: Node, part: SliderPart) -> Result<Option<Rect>> {
        let n = self.valid(node)?;
        let a = api().ok_or(Error(9))?;
        let mut r = Rect::default();
        let s = unsafe { (a.bounds)(self.raw, n, part as u32, &mut r) };
        if s == 1 {
            return Ok(None);
        }
        check(s)?;
        if !r.x.is_finite()
            || !r.y.is_finite()
            || !r.width.is_finite()
            || !r.height.is_finite()
            || r.width < 0.
            || r.height < 0.
        {
            return Err(PROTOCOL);
        }
        Ok(Some(r))
    }
    /// Absolute accumulated Core seconds; no Rust/native idle scheduler is implied.
    pub fn slider_presentation(&self, node: Node) -> Result<Option<SliderPresentation>> {
        let n = self.valid(node)?;
        let a = api().ok_or(Error(9))?;
        let mut r = RawPresentation::default();
        let s = unsafe { (a.presentation)(self.raw, n, 32, &mut r) };
        if s == 1 {
            return Ok(None);
        }
        check(s)?;
        if r.size != 32
            || r.schema != 1
            || r.reserved != 0
            || r.render_status > 5
            || !r.core_time.is_finite()
            || !r.next_wake_seconds.is_finite()
            || r.core_time < 0.
            || r.next_wake_seconds < 0.
        {
            return Err(PROTOCOL);
        }
        Ok(Some(SliderPresentation {
            render_status: r.render_status,
            core_time: r.core_time,
            next_wake_seconds: r.next_wake_seconds,
        }))
    }
    /// Pop one owned record, with no callback and no legacy FIFO drain.
    pub fn poll_slider_request(&mut self) -> Result<Option<SliderRequest>> {
        let namespace = self.slider.as_ref().ok_or(Error(8))?.namespace.clone();
        let a = api().ok_or(Error(9))?;
        let mut w = RawRequest::default();
        let status = unsafe { (a.poll)(self.raw, 296, &mut w) };
        if status == 1 {
            return Ok(None);
        }
        check(status)?;
        Ok(Some(read_request(w, namespace)?))
    }
    /// Explicit complete-pair acceptance. Clone preserves receipt identity; metadata does not.
    pub fn accept_slider_request(
        &mut self,
        receipt: &SliderReceipt,
        value: SliderValue,
    ) -> Result<()> {
        let state = self.slider.as_ref().ok_or(Error(8))?;
        ensure(Rc::ptr_eq(&state.namespace, &receipt.namespace))?;
        let v = value.wire()?;
        let a = api().ok_or(Error(9))?;
        check(unsafe { (a.accept)(self.raw, 56, &receipt.raw, 24, &v) })
    }
    pub fn on_slider_request(
        &mut self,
        node: Node,
        handler: impl for<'a> FnMut(&mut SliderRequestContext<'a>, SliderRequest) -> Result<()>
            + 'static,
    ) -> Result<()> {
        let raw = self.valid(node)?;
        // Validate live kind/identity before retaining caller state. Registration itself
        // never enables a queue or fabricates authority.
        self.slider_config(node)?;
        self.slider
            .as_mut()
            .ok_or(Error(8))?
            .handlers
            .insert(raw, Rc::new(RefCell::new(Box::new(handler))));
        Ok(())
    }
    pub fn remove_slider_handler(&mut self, node: Node) -> Result<()> {
        let raw = self.valid(node)?;
        if let Some(state) = &mut self.slider {
            state.handlers.remove(&raw);
        }
        Ok(())
    }
    /// Invoke copied requests after the C step and legacy FIFO. Errors/panics stop
    /// this drain and reset its local guard; no accepted work is rolled back.
    pub fn dispatch_slider_requests(&mut self) -> Result<usize> {
        if self.slider_dispatching() {
            return Err(Error(8));
        }
        if self.slider.as_ref().is_none_or(|s| s.closing) {
            return Ok(0);
        }
        self.slider.as_mut().unwrap().dispatching = true;
        let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            let mut count = 0;
            loop {
                if self.slider.as_ref().is_none_or(|s| s.closing) {
                    break;
                }
                let Some(request) = self.poll_slider_request()? else {
                    break;
                };
                let handler = self
                    .slider
                    .as_ref()
                    .and_then(|s| s.handlers.get(&request.receipt.raw.node).cloned());
                if let Some(handler) = handler {
                    let mut context = SliderRequestContext { app: self };
                    (handler.borrow_mut())(&mut context, request)?;
                    count += 1;
                }
            }
            Ok(count)
        }));
        if let Some(s) = &mut self.slider {
            s.dispatching = false
        }
        match result {
            Ok(value) => value,
            Err(panic) => std::panic::resume_unwind(panic),
        }
    }
    pub(super) fn slider_dispatching(&self) -> bool {
        self.slider.as_ref().is_some_and(|s| s.dispatching)
    }
    pub(super) fn slider_removed(&mut self, raw: RawNode) {
        if let Some(state) = &mut self.slider {
            state.handlers.remove(&raw);
            // Removing a parent also invalidates child handles. Release only the
            // new registrations; leave the legacy callback policy unchanged.
            if let Some(a) = api() {
                let app = self.raw;
                state.handlers.retain(|node, _| {
                    let mut config = RawConfig::default();
                    unsafe { (a.config)(app, *node, 72, &mut config) == 0 }
                });
            }
        }
    }
    pub(super) fn slider_close_requested(&mut self) {
        if let Some(s) = &mut self.slider {
            s.closing = true;
            s.handlers.clear();
        }
    }
}

/// Temporary restricted borrow for new typed handlers, never an owned App escape.
///
/// ```compile_fail
/// use cui_builders::SliderRequestContext;
/// fn reenter(context: &mut SliderRequestContext<'_>) {
///     context.step(0.016).unwrap(); // No execution/dispatch operation is exposed.
/// }
/// ```
/// External owner state may be mutated. No step/run/dispatch, Deref, or destruction
/// is exposed; `request_close` only requests shutdown and stops this typed drain.
pub struct SliderRequestContext<'a> {
    app: &'a mut App,
}
impl SliderRequestContext<'_> {
    pub fn accept(&mut self, receipt: &SliderReceipt, value: SliderValue) -> Result<()> {
        self.app.accept_slider_request(receipt, value)
    }
    pub fn configure(
        &mut self,
        node: Node,
        config: SliderConfig,
        value: SliderValue,
    ) -> Result<()> {
        self.app.configure_slider(node, config, value)
    }
    pub fn set_value(&mut self, node: Node, value: SliderValue) -> Result<()> {
        self.app.set_slider_value(node, value)
    }
    pub fn value(&self, node: Node) -> Result<SliderValue> {
        self.app.slider_value(node)
    }
    pub fn config(&self, node: Node) -> Result<SliderConfig> {
        self.app.slider_config(node)
    }
    pub fn text(&mut self, node: Node, value: &str) -> Result<()> {
        self.app.text(node, value).map(|_| ())
    }
    pub fn disabled(&mut self, node: Node, value: bool) -> Result<()> {
        self.app.component_disabled(node, value).map(|_| ())
    }
    pub fn remove(&mut self, node: Node) -> Result<()> {
        self.app.remove(node)
    }
    pub fn pointer(&mut self, phase: Phase, x: f32, y: f32) -> Result<()> {
        self.app.pointer(phase, x, y)
    }
    pub fn key(&mut self, down: bool, key: Key, modifiers: u32, repeat: bool) -> Result<()> {
        self.app.key(down, key, modifiers, repeat)
    }
    pub fn queue_text(&mut self, text: &str) -> Result<()> {
        self.app.queue_text(text)
    }
    pub fn request_close(&mut self) -> Result<()> {
        self.app.close()
    }
    pub fn on_slider_request(
        &mut self,
        node: Node,
        handler: impl for<'a> FnMut(&mut SliderRequestContext<'a>, SliderRequest) -> Result<()>
            + 'static,
    ) -> Result<()> {
        self.app.on_slider_request(node, handler)
    }
    pub fn remove_slider_handler(&mut self, node: Node) -> Result<()> {
        self.app.remove_slider_handler(node)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn request() -> RawRequest {
        RawRequest {
            size: 296,
            schema: 1,
            phase: 1,
            reason: 0,
            thumb: 1,
            event_kind: 3,
            event_source: 3,
            event_flags: 7,
            receipt: RawReceipt {
                size: 56,
                schema: 1,
                app_epoch: u64::MAX,
                node: RawNode {
                    slot: u32::MAX,
                    generation: u32::MAX,
                },
                source_generation: u64::MAX,
                configuration_revision: u64::MAX,
                submission_frame: u64::MAX,
                request_sequence: u64::MAX,
            },
            event_target: u64::MAX,
            event_frame: u64::MAX,
            event_index: u64::MAX,
            current: RawValue {
                kind: 1,
                start: 20.,
                end: 60.,
                ..Default::default()
            },
            requested: RawValue {
                kind: 1,
                start: 20.,
                end: 61.,
                ..Default::default()
            },
            domain: RawDomain {
                maximum: 100.,
                step: 1.,
                ..Default::default()
            },
            gesture_domain: RawDomain {
                maximum: 100.,
                step: 1.,
                ..Default::default()
            },
            input: RawInput {
                input_type: 3,
                flags: 1,
                key: 6,
                modifiers: 15,
                repeat: 1,
                pointer_id: u64::MAX,
                pointer_device: u64::MAX,
                ..Default::default()
            },
        }
    }
    #[test]
    fn wire_owned_extreme_identities_and_absence() {
        let namespace = Rc::new(Namespace { _nonzero: 1 });
        let w = request();
        let r = read_request(w, namespace.clone()).unwrap();
        assert_eq!(r.receipt.info().app_epoch, u64::MAX);
        assert_eq!(r.receipt.info().slot, u32::MAX);
        assert_eq!(r.receipt.info().generation, u32::MAX);
        assert_eq!(r.event_target, u64::MAX);
        assert_eq!(r.event_frame, u64::MAX);
        assert_eq!(r.event_index, u64::MAX);
        assert_eq!(r.input.pointer_id, u64::MAX);
        assert_eq!(r.input.pointer_device, u64::MAX);
        assert!(Rc::ptr_eq(&r.receipt.namespace, &namespace));
        let mut absent = w;
        absent.event_flags = 0;
        absent.event_source = 0;
        absent.event_frame = 0;
        absent.event_index = u64::MAX;
        absent.input = RawInput::default();
        let r = read_request(absent, namespace).unwrap();
        assert_eq!(r.input, SliderInput::default());
        assert_eq!(r.event_flags, 0);
    }
    #[test]
    fn wire_faults_reject_before_publishing_authority() {
        let namespace = Rc::new(Namespace { _nonzero: 1 });
        let faults: [fn(&mut RawRequest); 26] = [
            |r| r.size = 0,
            |r| r.schema = 2,
            |r| r.phase = 99,
            |r| r.reason = 99,
            |r| r.thumb = 2,
            |r| r.event_kind = 99,
            |r| r.event_source = 99,
            |r| r.event_flags = 8,
            |r| r.receipt.size = 0,
            |r| r.receipt.schema = 2,
            |r| r.event_flags = 3,
            |r| r.event_flags = 6,
            |r| {
                r.event_flags = 5;
                r.event_index = 0
            },
            |r| r.input.flags = 0,
            |r| r.input.flags = 4,
            |r| r.input.modifiers = 16,
            |r| r.input.key = 99,
            |r| r.input.input_type = 99,
            |r| r.input.flags = 2,
            |r| r.current.reserved = 1,
            |r| r.domain.reserved = 1,
            |r| r.input.x = f32::NAN,
            |r| r.input.accessibility_value = f64::INFINITY,
            |r| r.input.accessibility_value = 0.25,
            |r| r.requested.start = 80.,
            |r| r.gesture_domain.scale = 2,
        ];
        for change in faults {
            let mut w = request();
            change(&mut w);
            assert!(matches!(read_request(w, namespace.clone()), Err(Error(7))));
        }
        assert_eq!(Rc::strong_count(&namespace), 1);
    }
    #[test]
    fn config_active_fields_and_no_parent_normalization_on_copy() {
        let c = SliderConfig {
            layout: SliderLayout::Parent,
            length: f32::NAN,
            ..Default::default()
        };
        let w = c.wire().unwrap();
        assert_eq!(w.length, 0.);
        assert_eq!(read_config(w).unwrap().length, 0.);
        let mut malformed = w;
        malformed.length = 100.;
        assert_eq!(read_config(malformed), Err(Error(7)));
        assert!(SliderConfig { rem: f32::NAN, ..c }.wire().is_err());
        assert!(SliderValue::Single { end: f64::NAN }.wire().is_err());
        assert!(SliderValue::Range { start: 2., end: 1. }.wire().is_err());
    }
}
