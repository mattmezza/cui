//! Owned localized TimeField V1. No foreign reducer or draft borrowing.
use super::{check, App, Builder, Error, Kind, Node, RawNode, Result, Time};
use std::{
    cell::{Cell, RefCell},
    collections::HashMap,
    ffi::c_void,
    rc::Rc,
};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TimeCycle {
    H23,
    H12,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TimePrecision {
    Minute,
    Second,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct TimeFieldConfig {
    pub cycle: TimeCycle,
    pub precision: TimePrecision,
    pub disabled: bool,
    pub readonly: bool,
    pub owner_invalid: bool,
    pub minimum: Option<Time>,
    pub maximum: Option<Time>,
    pub edit_generation: u64,
    pub tokens: [String; 14],
    pub names: [String; 4],
}
impl Default for TimeFieldConfig {
    fn default() -> Self {
        Self::english()
    }
}
impl TimeFieldConfig {
    pub fn english() -> Self {
        Self {
            cycle: TimeCycle::H23,
            precision: TimePrecision::Minute,
            disabled: false,
            readonly: false,
            owner_invalid: false,
            minimum: None,
            maximum: None,
            edit_generation: 1,
            tokens: [
                "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", " ", "AM", "PM",
            ]
            .map(String::from),
            names: std::array::from_fn(|_| String::new()),
        }
    }
    pub fn arabic() -> Self {
        let mut c = Self::english();
        c.cycle = TimeCycle::H12;
        c.tokens = [
            "٠", "١", "٢", "٣", "٤", "٥", "٦", "٧", "٨", "٩", ":", " ", "ص", "م",
        ]
        .map(String::from);
        c.names = ["الساعة", "الدقيقة", "الثانية", "الفترة"].map(String::from);
        c
    }
    fn validate(&self) -> Result<()> {
        if self.edit_generation == 0 {
            return Err(Error(3));
        }
        for (i, t) in self.tokens.iter().enumerate() {
            if t.is_empty() || t.len() > 16 || t.contains('\0') {
                return Err(Error(3));
            }
            if self
                .tokens
                .iter()
                .enumerate()
                .any(|(j, u)| i != j && u.starts_with(t))
            {
                return Err(Error(3));
            }
        }
        if self.names.iter().any(|n| n.len() > 64 || n.contains('\0')) {
            return Err(Error(3));
        }
        for t in [self.minimum, self.maximum].into_iter().flatten() {
            if !valid_time(t) || (self.precision == TimePrecision::Minute && t.second != 0) {
                return Err(Error(3));
            }
        }
        if let (Some(a), Some(b)) = (self.minimum, self.maximum) {
            if seconds(a) > seconds(b) {
                return Err(Error(3));
            }
        }
        Ok(())
    }
}
fn valid_time(t: Time) -> bool {
    t.hour < 24 && t.minute < 60 && t.second < 60
}
fn seconds(t: Time) -> u32 {
    t.hour * 3600 + t.minute * 60 + t.second
}
#[repr(C)]
#[derive(Clone, Copy)]
struct Token {
    len: u32,
    bytes: [u8; 16],
}
#[repr(C)]
#[derive(Clone, Copy)]
struct Name {
    len: u32,
    bytes: [u8; 64],
}
#[repr(C)]
#[derive(Clone, Copy)]
struct ConfigRaw {
    size: u32,
    version: u32,
    flags: u32,
    cycle: u32,
    precision: u32,
    bounds: u32,
    reserved: [u32; 2],
    minimum: Time,
    maximum: Time,
    generation: u64,
    tokens: [Token; 14],
    names: [Name; 4],
}
#[repr(C)]
#[derive(Clone, Copy, Default)]
struct ResultRaw {
    size: u32,
    version: u32,
    reserved: [u32; 2],
    node: RawNode,
    frame: u64,
    status: u32,
    validation: u32,
    changed: u32,
    rejected: u32,
    previous: Time,
    value: Time,
    queued: u32,
    frame_status: u32,
}
#[repr(C)]
#[derive(Clone, Copy, Default)]
struct ChangeRaw {
    size: u32,
    version: u32,
    frame_status: u32,
    reserved: u32,
    node: RawNode,
    frame: u64,
    previous: Time,
    value: Time,
    validation: u32,
    rejected: u32,
}
fn config_wire(c: &TimeFieldConfig) -> Result<ConfigRaw> {
    c.validate()?;
    let mut r: ConfigRaw = unsafe { std::mem::zeroed() };
    r.size = 616;
    r.version = 1;
    r.flags = c.disabled as u32 | ((c.readonly as u32) << 1) | ((c.owner_invalid as u32) << 2);
    r.cycle = if c.cycle == TimeCycle::H12 { 1 } else { 0 };
    r.precision = if c.precision == TimePrecision::Second {
        1
    } else {
        0
    };
    r.generation = c.edit_generation;
    if let Some(t) = c.minimum {
        r.bounds |= 1;
        r.minimum = t
    }
    if let Some(t) = c.maximum {
        r.bounds |= 2;
        r.maximum = t
    }
    for (out, t) in r.tokens.iter_mut().zip(&c.tokens) {
        out.len = t.len() as u32;
        out.bytes[..t.len()].copy_from_slice(t.as_bytes())
    }
    for (out, t) in r.names.iter_mut().zip(&c.names) {
        out.len = t.len() as u32;
        out.bytes[..t.len()].copy_from_slice(t.as_bytes())
    }
    Ok(r)
}
fn read_string(bytes: &[u8], n: u32) -> Result<String> {
    let n = n as usize;
    if n > bytes.len() || bytes[n..].iter().any(|x| *x != 0) {
        return Err(Error(7));
    };
    String::from_utf8(bytes[..n].to_vec()).map_err(|_| Error(7))
}
fn config_read(r: ConfigRaw) -> Result<TimeFieldConfig> {
    if r.size != 616
        || r.version != 1
        || r.reserved != [0, 0]
        || r.flags & !7 != 0
        || r.bounds & !3 != 0
        || r.cycle > 1
        || r.precision > 1
    {
        return Err(Error(7));
    }
    let mut c = TimeFieldConfig::english();
    c.cycle = if r.cycle == 1 {
        TimeCycle::H12
    } else {
        TimeCycle::H23
    };
    c.precision = if r.precision == 1 {
        TimePrecision::Second
    } else {
        TimePrecision::Minute
    };
    c.disabled = r.flags & 1 != 0;
    c.readonly = r.flags & 2 != 0;
    c.owner_invalid = r.flags & 4 != 0;
    c.edit_generation = r.generation;
    if r.bounds & 1 != 0 {
        c.minimum = Some(r.minimum)
    } else if r.minimum != Time::default() {
        return Err(Error(7));
    }
    if r.bounds & 2 != 0 {
        c.maximum = Some(r.maximum)
    } else if r.maximum != Time::default() {
        return Err(Error(7));
    }
    for (out, t) in c.tokens.iter_mut().zip(r.tokens) {
        *out = read_string(&t.bytes, t.len)?
    }
    for (out, t) in c.names.iter_mut().zip(r.names) {
        *out = read_string(&t.bytes, t.len)?
    }
    c.validate()?;
    Ok(c)
}
/// Applied aggregate. Safe to retain after removal/app destruction, including failed frames.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct TimeFieldChange {
    pub node: Node,
    pub frame: u64,
    pub previous: Time,
    pub value: Time,
    pub validation: u32,
    pub rejected_count: u32,
    pub frame_status: u32,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct TimeFieldResult {
    pub change: TimeFieldChange,
    pub status: u32,
    pub changed: bool,
    pub queued_change_count: u32,
}
fn change_read(r: ChangeRaw, app_id: u64) -> Result<TimeFieldChange> {
    observation_read(r, app_id, true)
}
fn observation_read(r: ChangeRaw, app_id: u64, applied: bool) -> Result<TimeFieldChange> {
    if r.size != 64
        || r.version != 1
        || r.reserved != 0
        || r.node.slot == 0
        || r.node.generation == 0
        || !matches!(r.validation, 0 | 3 | 4 | 5)
        || r.frame == 0
        || r.frame_status == 1
        || (applied && r.previous == r.value)
        || r.frame_status > 10
        || !valid_time(r.previous)
        || !valid_time(r.value)
    {
        return Err(Error(7));
    }
    Ok(TimeFieldChange {
        node: Node {
            raw: r.node,
            app_id,
        },
        frame: r.frame,
        previous: r.previous,
        value: r.value,
        validation: r.validation,
        rejected_count: r.rejected,
        frame_status: r.frame_status,
    })
}
type Query = unsafe extern "C" fn() -> u32;
type Enable = unsafe extern "C" fn(*mut c_void, u32) -> u32;
type Configure = unsafe extern "C" fn(*mut c_void, RawNode, *const ConfigRaw) -> u32;
type GetConfig = unsafe extern "C" fn(*mut c_void, RawNode, *mut ConfigRaw) -> u32;
type GetResult = unsafe extern "C" fn(*mut c_void, RawNode, *mut ResultRaw) -> u32;
type Clear = unsafe extern "C" fn(*mut c_void, RawNode) -> u32;
type Poll = unsafe extern "C" fn(*mut c_void, *mut ChangeRaw) -> u32;
struct Api {
    versions: Query,
    config_size: Query,
    result_size: Query,
    change_size: Query,
    enable: Enable,
    configure: Configure,
    get_config: GetConfig,
    get_result: GetResult,
    clear: Clear,
    poll: Poll,
}
#[cfg(cui_bridge_static)]
extern "C" {
    fn cui_bridge_time_field_versions() -> u32;
    fn cui_bridge_time_field_config_size_v1() -> u32;
    fn cui_bridge_time_field_result_size_v1() -> u32;
    fn cui_bridge_time_field_change_size_v1() -> u32;
    fn cui_bridge_time_field_enable_v1(a: *mut c_void, c: u32) -> u32;
    fn cui_bridge_time_field_configure_v1(a: *mut c_void, n: RawNode, c: *const ConfigRaw) -> u32;
    fn cui_bridge_time_field_get_config_v1(a: *mut c_void, n: RawNode, c: *mut ConfigRaw) -> u32;
    fn cui_bridge_time_field_get_result_v1(a: *mut c_void, n: RawNode, r: *mut ResultRaw) -> u32;
    fn cui_bridge_time_field_clear_validation_v1(a: *mut c_void, n: RawNode) -> u32;
    fn cui_bridge_time_field_poll_change_v1(a: *mut c_void, c: *mut ChangeRaw) -> u32;
}
fn api() -> Option<Api> {
    #[cfg(cui_bridge_static)]
    let a = Api {
        versions: cui_bridge_time_field_versions,
        config_size: cui_bridge_time_field_config_size_v1,
        result_size: cui_bridge_time_field_result_size_v1,
        change_size: cui_bridge_time_field_change_size_v1,
        enable: cui_bridge_time_field_enable_v1,
        configure: cui_bridge_time_field_configure_v1,
        get_config: cui_bridge_time_field_get_config_v1,
        get_result: cui_bridge_time_field_get_result_v1,
        clear: cui_bridge_time_field_clear_validation_v1,
        poll: cui_bridge_time_field_poll_change_v1,
    };
    #[cfg(all(not(cui_bridge_static), any(unix, windows)))]
    let a = unsafe {
        let versions =
            super::notification_symbol_v1(b"cui_bridge_time_field_versions\0".as_ptr().cast());
        if versions.is_null() {
            return None;
        }
        let config_size = super::notification_symbol_v1(
            b"cui_bridge_time_field_config_size_v1\0".as_ptr().cast(),
        );
        if config_size.is_null() {
            return None;
        }
        let result_size = super::notification_symbol_v1(
            b"cui_bridge_time_field_result_size_v1\0".as_ptr().cast(),
        );
        if result_size.is_null() {
            return None;
        }
        let change_size = super::notification_symbol_v1(
            b"cui_bridge_time_field_change_size_v1\0".as_ptr().cast(),
        );
        if change_size.is_null() {
            return None;
        }
        let enable =
            super::notification_symbol_v1(b"cui_bridge_time_field_enable_v1\0".as_ptr().cast());
        if enable.is_null() {
            return None;
        }
        let configure =
            super::notification_symbol_v1(b"cui_bridge_time_field_configure_v1\0".as_ptr().cast());
        if configure.is_null() {
            return None;
        }
        let get_config =
            super::notification_symbol_v1(b"cui_bridge_time_field_get_config_v1\0".as_ptr().cast());
        if get_config.is_null() {
            return None;
        }
        let get_result =
            super::notification_symbol_v1(b"cui_bridge_time_field_get_result_v1\0".as_ptr().cast());
        if get_result.is_null() {
            return None;
        }
        let clear = super::notification_symbol_v1(
            b"cui_bridge_time_field_clear_validation_v1\0"
                .as_ptr()
                .cast(),
        );
        if clear.is_null() {
            return None;
        }
        let poll = super::notification_symbol_v1(
            b"cui_bridge_time_field_poll_change_v1\0".as_ptr().cast(),
        );
        if poll.is_null() {
            return None;
        }
        Api {
            versions: std::mem::transmute::<*mut c_void, Query>(versions),
            config_size: std::mem::transmute::<*mut c_void, Query>(config_size),
            result_size: std::mem::transmute::<*mut c_void, Query>(result_size),
            change_size: std::mem::transmute::<*mut c_void, Query>(change_size),
            enable: std::mem::transmute::<*mut c_void, Enable>(enable),
            configure: std::mem::transmute::<*mut c_void, Configure>(configure),
            get_config: std::mem::transmute::<*mut c_void, GetConfig>(get_config),
            get_result: std::mem::transmute::<*mut c_void, GetResult>(get_result),
            clear: std::mem::transmute::<*mut c_void, Clear>(clear),
            poll: std::mem::transmute::<*mut c_void, Poll>(poll),
        }
    };
    #[cfg(all(not(cui_bridge_static), not(any(unix, windows))))]
    return None;
    #[cfg(any(cui_bridge_static, unix, windows))]
    {
        if unsafe { (a.versions)() } & 1 == 0
            || unsafe { (a.config_size)() } != 616
            || unsafe { (a.result_size)() } != 80
            || unsafe { (a.change_size)() } != 64
        {
            return None;
        }
        Some(a)
    }
}
type Callback = Rc<RefCell<Box<dyn FnMut(&TimeFieldChange)>>>;
pub(super) struct State {
    capacity: u32,
    pub(super) closing: bool,
    pub(super) callbacks: HashMap<RawNode, Callback>,
    dispatching: Rc<Cell<bool>>,
}
struct DispatchGuard(Rc<Cell<bool>>);
impl Drop for DispatchGuard {
    fn drop(&mut self) {
        self.0.set(false)
    }
}
impl App {
    pub fn supports_localized_time_field(&self) -> bool {
        api().is_some()
    }
    pub fn enable_localized_time_fields(&mut self, capacity: u32) -> Result<()> {
        let capacity = if capacity == 0 { 4096 } else { capacity };
        if capacity > 4096 {
            return Err(Error(3));
        }
        if self
            .time_localized
            .as_ref()
            .is_some_and(|s| s.capacity != capacity)
        {
            return Err(Error(8));
        }
        let a = api().ok_or(Error(9))?;
        check(unsafe { (a.enable)(self.raw, capacity) })?;
        if self.time_localized.is_none() {
            self.time_localized = Some(State {
                capacity,
                closing: false,
                callbacks: HashMap::new(),
                dispatching: Rc::new(Cell::new(false)),
            })
        }
        Ok(())
    }
    /// App-owned constructor; invalid config is rejected before creation, and any
    /// later time/configuration failure removes the newly created node.
    pub fn localized_time_field(
        &mut self,
        parent: Node,
        key: &str,
        value: Time,
        c: &TimeFieldConfig,
    ) -> Result<Node> {
        c.validate()?;
        if !valid_time(value) {
            return Err(Error(3));
        }
        if self.time_localized.is_none() {
            self.enable_localized_time_fields(4096)?
        }
        let n = self.child(parent, Kind::TimeField, key)?;
        if let Err(e) = self
            .time(n, value)
            .and_then(|app| app.configure_localized_time(n, c))
        {
            let _ = self.remove(n);
            return Err(e);
        }
        Ok(n)
    }
    pub fn configure_localized_time(&mut self, n: Node, c: &TimeFieldConfig) -> Result<&mut Self> {
        let raw = self.valid(n)?;
        let r = config_wire(c)?;
        if self.time_localized.is_none() {
            return Err(Error(8));
        }
        let a = api().ok_or(Error(9))?;
        check(unsafe { (a.configure)(self.raw, raw, &r) })?;
        Ok(self)
    }
    pub fn localized_time_config(&self, n: Node) -> Result<TimeFieldConfig> {
        let raw = self.valid(n)?;
        let a = api().ok_or(Error(9))?;
        let mut r: ConfigRaw = unsafe { std::mem::zeroed() };
        r.size = 616;
        r.version = 1;
        check(unsafe { (a.get_config)(self.raw, raw, &mut r) })?;
        config_read(r)
    }
    pub fn clear_time_validation(&mut self, n: Node) -> Result<()> {
        let raw = self.valid(n)?;
        let a = api().ok_or(Error(9))?;
        check(unsafe { (a.clear)(self.raw, raw) })
    }
    pub fn localized_time_result(&self, n: Node) -> Result<TimeFieldResult> {
        let raw = self.valid(n)?;
        let a = api().ok_or(Error(9))?;
        let mut r = ResultRaw {
            size: 80,
            version: 1,
            ..Default::default()
        };
        check(unsafe { (a.get_result)(self.raw, raw, &mut r) })?;
        result_read(r, raw, self.id)
    }

    /// Independent typed queue. Call even after a failed step to inspect copied
    /// already-applied records. Poll consumes only on OK; no callback or replay.
    pub fn poll_time_field_change(&mut self) -> Result<Option<TimeFieldChange>> {
        let a = api().ok_or(Error(9))?;
        let mut r = ChangeRaw {
            size: 64,
            version: 1,
            ..Default::default()
        };
        let status = unsafe { (a.poll)(self.raw, &mut r) };
        if status == 1 {
            return Ok(None);
        }
        check(status)?;
        change_read(r, self.id).map(Some)
    }
    pub fn on_time_field_change(
        &mut self,
        n: Node,
        f: impl FnMut(&TimeFieldChange) + 'static,
    ) -> Result<&mut Self> {
        let raw = self.valid(n)?;
        self.localized_time_config(n)?;
        self.time_localized
            .as_mut()
            .ok_or(Error(8))?
            .callbacks
            .insert(raw, Rc::new(RefCell::new(Box::new(f))));
        Ok(self)
    }
    pub(super) fn time_dispatching(&self) -> bool {
        self.time_localized
            .as_ref()
            .is_some_and(|s| s.dispatching.get())
    }
    /// Post-step callbacks own copied payloads. Panics consume only the delivered
    /// record and unwind in Rust; the native run trampoline catches/resumes after C.
    pub fn dispatch_time_field_changes(&mut self) -> Result<usize> {
        let Some(s) = &self.time_localized else {
            return Ok(0);
        };
        if s.closing {
            return Ok(0);
        }
        if s.dispatching.replace(true) {
            return Err(Error(8));
        }
        let _guard = DispatchGuard(s.dispatching.clone());
        let mut count = 0;
        loop {
            if self.time_localized.as_ref().is_some_and(|s| s.closing) {
                return Ok(count);
            }
            let Some(c) = self.poll_time_field_change()? else {
                return Ok(count);
            };
            let cb = self
                .time_localized
                .as_ref()
                .and_then(|s| s.callbacks.get(&c.node.raw))
                .cloned();
            count += 1;
            if let Some(cb) = cb {
                (cb.borrow_mut())(&c);
            }
        }
    }
}
impl<'a> Builder<'a> {
    /// This existing builder already owns a live node. For atomic creation use
    /// App::localized_time_field; this setter never removes a preexisting node.
    pub fn localized_time(self, c: &TimeFieldConfig) -> Result<Self> {
        self.app.configure_localized_time(self.node, c)?;
        Ok(self)
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn wire_layout_and_copy() {
        assert_eq!(std::mem::size_of::<ConfigRaw>(), 616);
        assert_eq!(std::mem::size_of::<ResultRaw>(), 80);
        assert_eq!(std::mem::size_of::<ChangeRaw>(), 64);
        let mut c = TimeFieldConfig::arabic();
        c.minimum = Some(Time {
            hour: 8,
            minute: 0,
            second: 0,
        });
        c.maximum = Some(Time {
            hour: 18,
            minute: 0,
            second: 0,
        });
        c.edit_generation = 42;
        let raw = config_wire(&c).unwrap();
        let saved = c.clone();
        c.tokens[0] = "X".into();
        assert_eq!(config_read(raw).unwrap(), saved);
    }
    #[test]
    fn invalid_config_is_preflight() {
        let mut c = TimeFieldConfig::english();
        c.tokens[1] = "00".into();
        assert!(config_wire(&c).is_err());
        c = TimeFieldConfig::english();
        c.minimum = Some(Time {
            hour: 10,
            minute: 0,
            second: 1,
        });
        assert!(config_wire(&c).is_err());
        c = TimeFieldConfig::english();
        c.names[0] = "x".repeat(65);
        assert!(config_wire(&c).is_err());
    }
    #[test]
    fn copied_failed_observation() {
        let r = ChangeRaw {
            size: 64,
            version: 1,
            node: RawNode {
                slot: 1,
                generation: 2,
            },
            frame_status: 10,
            frame: 1,
            previous: Time {
                hour: 8,
                minute: 0,
                second: 0,
            },
            value: Time {
                hour: 9,
                minute: 0,
                second: 0,
            },
            ..Default::default()
        };
        let c = change_read(r, 99).unwrap();
        assert_eq!(c.frame_status, 10);
        assert_eq!(c.value.hour, 9);
        let mut bad = r;
        bad.reserved = 1;
        assert!(change_read(bad, 99).is_err());
    }
}

fn result_read(r: ResultRaw, expected: RawNode, app_id: u64) -> Result<TimeFieldResult> {
    if r.size != 80
        || r.version != 1
        || r.reserved != [0, 0]
        || r.status > 6
        || r.changed > 1
        || r.queued > r.changed
        || r.node != expected
        || r.frame_status != 0
        || (r.changed != 0) != (r.previous != r.value)
    {
        return Err(Error(7));
    }
    let change = observation_read(
        ChangeRaw {
            size: 64,
            version: 1,
            frame_status: r.frame_status,
            node: r.node,
            frame: r.frame,
            previous: r.previous,
            value: r.value,
            validation: r.validation,
            rejected: r.rejected,
            ..Default::default()
        },
        app_id,
        false,
    )?;
    Ok(TimeFieldResult {
        change,
        status: r.status,
        changed: r.changed != 0,
        queued_change_count: r.queued,
    })
}
#[cfg(test)]
mod malformed_tests {
    use super::*;
    fn change() -> ChangeRaw {
        ChangeRaw {
            size: 64,
            version: 1,
            node: RawNode {
                slot: 1,
                generation: 2,
            },
            frame: 1,
            previous: Time {
                hour: 8,
                minute: 0,
                second: 0,
            },
            value: Time {
                hour: 9,
                minute: 0,
                second: 0,
            },
            ..Default::default()
        }
    }
    #[test]
    fn invalid_applied_metadata() {
        let r = change();
        assert!(change_read(r, 1).is_ok());
        let mut bad = r;
        bad.frame = 0;
        assert!(change_read(bad, 1).is_err());
        bad = r;
        bad.frame_status = 1;
        assert!(change_read(bad, 1).is_err());
        for v in [1, 2, 6] {
            bad = r;
            bad.validation = v;
            assert!(change_read(bad, 1).is_err())
        }
        bad = r;
        bad.value = bad.previous;
        assert!(change_read(bad, 1).is_err());
    }
    #[test]
    fn unchanged_latest_result_is_not_an_applied_record() {
        let r = ResultRaw {
            size: 80,
            version: 1,
            node: RawNode {
                slot: 1,
                generation: 2,
            },
            frame: 1,
            previous: Time {
                hour: 8,
                minute: 0,
                second: 0,
            },
            value: Time {
                hour: 8,
                minute: 0,
                second: 0,
            },
            ..Default::default()
        };
        let out = result_read(r, r.node, 1).unwrap();
        assert!(!out.changed);
        assert_eq!(out.queued_change_count, 0);
        let mut bad = r;
        bad.changed = 1;
        assert!(result_read(bad, r.node, 1).is_err());
        bad = r;
        bad.queued = 1;
        assert!(result_read(bad, r.node, 1).is_err());
        bad = r;
        bad.frame_status = 1;
        assert!(result_read(bad, r.node, 1).is_err());
        assert!(result_read(
            r,
            RawNode {
                slot: 2,
                generation: 2
            },
            1
        )
        .is_err());
    }
}
