//! Owned Rust builders over the CUI bridge. All operations stay on the creating thread.
use std::{
    cell::RefCell,
    collections::HashMap,
    ffi::{c_char, c_void, CString},
    marker::PhantomData,
    rc::Rc,
};

#[repr(C)]
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq, Hash)]
struct RawNode {
    slot: u32,
    generation: u32,
}
#[repr(C)]
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq)]
pub struct Date {
    pub year: i32,
    pub month: i32,
    pub day: i32,
}
#[repr(C)]
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq)]
pub struct Time {
    pub hour: u32,
    pub minute: u32,
    pub second: u32,
}
#[repr(C)]
#[derive(Clone, Copy, Default, Debug)]
pub struct Rect {
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
}
#[repr(C)]
struct Options {
    struct_size: u32,
    abi_version: u32,
    backend: u32,
    max_nodes: u32,
    max_events: u32,
    width: f32,
    height: f32,
    title: *const c_char,
    font: *const c_char,
}
#[repr(C)]
#[derive(Default)]
struct EventEx {
    kind: u32,
    node: RawNode,
    frame: u64,
    checked: u32,
    value: f64,
    text_length: u32,
    selected: RawNode,
}
#[derive(Clone, Debug)]
pub struct ChangeEvent {
    pub node: Node,
    pub frame: u64,
    pub checked: bool,
    pub value: f64,
    pub text: String,
    pub selected: Option<Node>,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum AttachmentStatus {
    Pending = 0,
    Uploading = 1,
    Processing = 2,
    Failed = 3,
    Complete = 4,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum SurfaceResult {
    Accepted,
    Cancelled,
    Closed,
}
impl ChangeEvent {
    pub fn surface_result(&self) -> Option<SurfaceResult> {
        match self.value as u32 {
            1 => Some(SurfaceResult::Accepted),
            2 => Some(SurfaceResult::Cancelled),
            3 => Some(SurfaceResult::Closed),
            _ => None,
        }
    }
}
#[link(name = "cui_bridge")]
extern "C" {
    fn cui_bridge_abi_version() -> u32;
    fn cui_bridge_options_size() -> u32;
    fn cui_bridge_create(o: *const Options, status: *mut u32) -> *mut c_void;
    fn cui_bridge_destroy(a: *mut c_void);
    fn cui_bridge_node_create(
        a: *mut c_void,
        p: RawNode,
        k: u32,
        key: *const c_char,
        out: *mut RawNode,
    ) -> u32;
    fn cui_bridge_node_remove(a: *mut c_void, n: RawNode) -> u32;
    fn cui_bridge_set_text(a: *mut c_void, n: RawNode, t: *const c_char) -> u32;
    fn cui_bridge_set_gap(a: *mut c_void, n: RawNode, g: f32) -> u32;
    fn cui_bridge_set_button_variant(a: *mut c_void, n: RawNode, v: u32) -> u32;
    fn cui_bridge_set_button_size(a: *mut c_void, n: RawNode, v: u32) -> u32;
    fn cui_bridge_set_button_rounding(a: *mut c_void, n: RawNode, v: u32, r: f32) -> u32;
    fn cui_bridge_set_button_disabled(a: *mut c_void, n: RawNode, v: u32) -> u32;
    fn cui_bridge_set_checked(a: *mut c_void, n: RawNode, v: u32) -> u32;
    fn cui_bridge_set_disabled(a: *mut c_void, n: RawNode, v: u32) -> u32;
    fn cui_bridge_set_value(a: *mut c_void, n: RawNode, v: f64) -> u32;
    fn cui_bridge_set_range(a: *mut c_void, n: RawNode, min: f64, max: f64, step: f64) -> u32;
    fn cui_bridge_get_checked(a: *mut c_void, n: RawNode, out: *mut u32) -> u32;
    fn cui_bridge_get_value(a: *mut c_void, n: RawNode, out: *mut f64) -> u32;
    fn cui_bridge_set_button_hover_rgba(
        a: *mut c_void,
        n: RawNode,
        r: u8,
        g: u8,
        b: u8,
        alpha: u8,
    ) -> u32;
    fn cui_bridge_queue_pointer(a: *mut c_void, p: u32, x: f32, y: f32) -> u32;
    fn cui_bridge_queue_key(a: *mut c_void, p: u32, k: u32, m: u32, r: u32) -> u32;
    fn cui_bridge_step(a: *mut c_void, dt: f32) -> u32;
    fn cui_bridge_poll_event_ex(
        a: *mut c_void,
        e: *mut EventEx,
        destination: *mut c_char,
        capacity: u32,
        required: *mut u32,
    ) -> u32;
    fn cui_bridge_get_text(
        a: *mut c_void,
        n: RawNode,
        destination: *mut c_char,
        capacity: u32,
        required: *mut u32,
    ) -> u32;
    fn cui_bridge_queue_text(a: *mut c_void, text: *const c_char) -> u32;
    fn cui_bridge_set_detail_text(a: *mut c_void, n: RawNode, text: *const c_char) -> u32;
    fn cui_bridge_set_open(a: *mut c_void, n: RawNode, open: u32) -> u32;
    fn cui_bridge_set_selected(a: *mut c_void, group: RawNode, item: RawNode) -> u32;
    fn cui_bridge_get_open(a: *mut c_void, n: RawNode, out: *mut u32) -> u32;
    fn cui_bridge_set_anchor(a: *mut c_void, surface: RawNode, anchor: RawNode) -> u32;
    fn cui_bridge_set_visual_size(a: *mut c_void, n: RawNode, width: f32, height: f32) -> u32;
    fn cui_bridge_set_total(a: *mut c_void, n: RawNode, total: u32) -> u32;
    fn cui_bridge_set_step_status(a: *mut c_void, n: RawNode, status: u32) -> u32;
    fn cui_bridge_set_scroll_extent(a: *mut c_void, n: RawNode, width: f32, height: f32) -> u32;
    fn cui_bridge_set_attachment_status(a: *mut c_void, n: RawNode, status: u32) -> u32;
    fn cui_bridge_set_time(a: *mut c_void, n: RawNode, value: Time) -> u32;
    fn cui_bridge_set_date(a: *mut c_void, n: RawNode, value: Date) -> u32;
    fn cui_bridge_get_date(a: *mut c_void, n: RawNode, out: *mut Date) -> u32;
    fn cui_bridge_calendar_day_bounds(a: *mut c_void, n: RawNode, day: Date, out: *mut Rect)
        -> u32;
    fn cui_bridge_set_color_rgba(a: *mut c_void, n: RawNode, color: u32) -> u32;
    fn cui_bridge_get_color_rgba(a: *mut c_void, n: RawNode, out: *mut u32) -> u32;
    fn cui_bridge_color_part_bounds(a: *mut c_void, n: RawNode, part: u32, out: *mut Rect) -> u32;
    fn cui_bridge_get_time(a: *mut c_void, n: RawNode, out: *mut Time) -> u32;
    fn cui_bridge_set_image_rgba(
        a: *mut c_void,
        n: RawNode,
        pixels: *const u8,
        width: u32,
        height: u32,
        stride: u32,
        byte_count: u32,
    ) -> u32;
    fn cui_bridge_queue_scroll(a: *mut c_void, x: f32, y: f32, dx: f32, dy: f32) -> u32;
    fn cui_bridge_numeric_part_bounds(a: *mut c_void, n: RawNode, part: u32, r: *mut Rect) -> u32;
    fn cui_bridge_get_selected(a: *mut c_void, group: RawNode, out: *mut RawNode) -> u32;
    fn cui_bridge_node_bounds(a: *mut c_void, n: RawNode, r: *mut Rect) -> u32;
    fn cui_bridge_dropdown_part_bounds(
        a: *mut c_void,
        n: RawNode,
        menu_part: u32,
        r: *mut Rect,
    ) -> u32;
    fn cui_bridge_set_font_fallbacks(
        a: *mut c_void,
        paths: *const *const c_char,
        count: u32,
    ) -> u32;
    fn cui_bridge_request_close(a: *mut c_void) -> u32;
}
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Error(pub u32);
pub type Result<T> = std::result::Result<T, Error>;
fn check(status: u32) -> Result<()> {
    if status == 0 {
        Ok(())
    } else {
        Err(Error(status))
    }
}
fn string(s: &str) -> Result<CString> {
    if s.len() > 4096 {
        return Err(Error(3));
    }
    CString::new(s).map_err(|_| Error(3))
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kind {
    Row = 1,
    Column = 2,
    Label = 3,
    Button = 4,
    Checkbox = 5,
    Radio = 6,
    Switch = 7,
    Toggle = 8,
    Slider = 9,
    Progress = 10,
    Badge = 11,
    Input = 12,
    Textarea = 13,
    NumberInput = 14,
    Collapsible = 15,
    Alert = 16,
    RadioGroup = 17,
    Tabs = 18,
    Accordion = 19,
    Item = 20,
    Select = 21,
    ComboBox = 22,
    DropdownButton = 23,
    Dialog = 24,
    AlertDialog = 25,
    Sheet = 26,
    Popover = 27,
    Tooltip = 28,
    HoverCard = 29,
    Avatar = 30,
    Bubble = 31,
    EmptyView = 32,
    GroupBox = 33,
    Kbd = 34,
    Skeleton = 35,
    Spinner = 36,
    Tag = 37,
    Marker = 38,
    Shimmer = 39,
    List = 40,
    Menu = 41,
    Pagination = 42,
    Rating = 43,
    Stepper = 44,
    DescriptionList = 45,
    Scrollable = 46,
    StatusBar = 47,
    Toolbar = 48,
    TitleBar = 49,
    Icon = 50,
    Message = 51,
    Attachment = 52,
    Image = 53,
    Tree = 54,
    TimeField = 55,
    ColorPicker = 56,
    Calendar = 57,
    DatePicker = 58,
    OtpInput = 59,
    Form = 60,
    InputGroup = 61,
    Notification = 62,
    FocusTrap = 63,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Variant {
    Primary = 0,
    Secondary = 1,
    Outline = 2,
    Ghost = 3,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Size {
    Inherit = 0,
    Xs = 1,
    Sm = 2,
    Base = 3,
    Md = 4,
    Lg = 5,
    Xl = 6,
    X2l = 7,
    X3l = 8,
    X4l = 9,
    X5l = 10,
    X6l = 11,
    X7l = 12,
    X8l = 13,
    X9l = 14,
}
#[derive(Clone, Copy, Debug, PartialEq)]
pub enum Rounding {
    Theme,
    None,
    Small,
    Large,
    Custom(f32),
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Key {
    Tab = 1,
    Enter = 2,
    Space = 3,
    Escape = 4,
    Left = 5,
    Right = 6,
    Up = 7,
    Down = 8,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Phase {
    Move = 0,
    Down = 1,
    Up = 2,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Node {
    raw: RawNode,
    app_id: u64,
}
static NEXT_ID: std::sync::atomic::AtomicU64 = std::sync::atomic::AtomicU64::new(1);
type Callback = Box<dyn FnMut(&ChangeEvent)>;
pub struct App {
    raw: *mut c_void,
    id: u64,
    callbacks: HashMap<(RawNode, u32), Rc<RefCell<Callback>>>,
    _local: PhantomData<Rc<()>>,
}
impl Drop for App {
    fn drop(&mut self) {
        unsafe { cui_bridge_destroy(self.raw) }
    }
}
impl App {
    pub fn headless(width: f32, height: f32) -> Result<Self> {
        Self::new(width, height, None, None)
    }
    pub fn sdl(width: f32, height: f32, title: &str, font: &str) -> Result<Self> {
        Self::new(width, height, Some(title), Some(font))
    }
    fn new(width: f32, height: f32, title: Option<&str>, font: Option<&str>) -> Result<Self> {
        unsafe {
            if cui_bridge_abi_version() != 1
                || cui_bridge_options_size() != std::mem::size_of::<Options>() as u32
            {
                return Err(Error(3));
            }
        }
        let title = title.map(string).transpose()?;
        let font = font.map(string).transpose()?;
        let o = Options {
            struct_size: std::mem::size_of::<Options>() as u32,
            abi_version: 1,
            backend: if font.is_some() { 1 } else { 0 },
            max_nodes: 0,
            max_events: 0,
            width,
            height,
            title: title.as_ref().map_or(std::ptr::null(), |x| x.as_ptr()),
            font: font.as_ref().map_or(std::ptr::null(), |x| x.as_ptr()),
        };
        let mut status = 0;
        let raw = unsafe { cui_bridge_create(&o, &mut status) };
        if raw.is_null() {
            return Err(Error(status));
        }
        let id = NEXT_ID.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
        Ok(Self {
            raw,
            id,
            callbacks: HashMap::new(),
            _local: PhantomData,
        })
    }
    fn valid(&self, n: Node) -> Result<RawNode> {
        if n.app_id != self.id {
            Err(Error(3))
        } else {
            Ok(n.raw)
        }
    }
    pub fn root(&mut self, kind: Kind, key: &str) -> Result<Node> {
        self.create(None, kind, key)
    }
    pub fn child(&mut self, parent: Node, kind: Kind, key: &str) -> Result<Node> {
        self.create(Some(parent), kind, key)
    }
    fn create(&mut self, parent: Option<Node>, kind: Kind, key: &str) -> Result<Node> {
        let p = parent
            .map(|n| self.valid(n))
            .transpose()?
            .unwrap_or_default();
        let key = string(key)?;
        let mut raw = RawNode::default();
        check(unsafe { cui_bridge_node_create(self.raw, p, kind as u32, key.as_ptr(), &mut raw) })?;
        Ok(Node {
            raw,
            app_id: self.id,
        })
    }
    pub fn remove(&mut self, n: Node) -> Result<()> {
        let raw = self.valid(n)?;
        check(unsafe { cui_bridge_node_remove(self.raw, raw) })?;
        self.callbacks.retain(|(node, _), _| *node != raw);
        Ok(())
    }
    pub fn text(&mut self, n: Node, value: &str) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let t = string(value)?;
        check(unsafe { cui_bridge_set_text(self.raw, n, t.as_ptr()) })?;
        Ok(self)
    }
    pub fn gap_px(&mut self, n: Node, g: f32) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_gap(self.raw, n, g) })?;
        Ok(self)
    }
    pub fn gap_units(&mut self, n: Node, g: f32) -> Result<&mut Self> {
        self.gap_px(n, g * 4.)
    }
    pub fn variant(&mut self, n: Node, v: Variant) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_button_variant(self.raw, n, v as u32) })?;
        Ok(self)
    }
    pub fn size(&mut self, n: Node, v: Size) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_button_size(self.raw, n, v as u32) })?;
        Ok(self)
    }
    pub fn rounding(&mut self, n: Node, v: Rounding) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let (k, r) = match v {
            Rounding::Theme => (0, 0.),
            Rounding::None => (1, 0.),
            Rounding::Small => (2, 0.),
            Rounding::Large => (3, 0.),
            Rounding::Custom(x) => (4, x),
        };
        check(unsafe { cui_bridge_set_button_rounding(self.raw, n, k, r) })?;
        Ok(self)
    }
    pub fn disabled(&mut self, n: Node, v: bool) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_button_disabled(self.raw, n, v as u32) })?;
        Ok(self)
    }
    pub fn hover_rgba(&mut self, n: Node, c: [u8; 4]) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_button_hover_rgba(self.raw, n, c[0], c[1], c[2], c[3]) })?;
        Ok(self)
    }
    pub fn on_click(&mut self, n: Node, mut f: impl FnMut() + 'static) -> Result<&mut Self> {
        let n = self.valid(n)?;
        self.callbacks
            .insert((n, 1), Rc::new(RefCell::new(Box::new(move |_| f()))));
        Ok(self)
    }
    pub fn checked(&mut self, n: Node, v: bool) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_checked(self.raw, n, v as u32) })?;
        Ok(self)
    }
    pub fn component_disabled(&mut self, n: Node, v: bool) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_disabled(self.raw, n, v as u32) })?;
        Ok(self)
    }
    pub fn value(&mut self, n: Node, v: f64) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_value(self.raw, n, v) })?;
        Ok(self)
    }
    pub fn range(&mut self, n: Node, min: f64, max: f64, step: f64) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_range(self.raw, n, min, max, step) })?;
        Ok(self)
    }
    pub fn is_checked(&self, n: Node) -> Result<bool> {
        let n = self.valid(n)?;
        let mut v = 0;
        check(unsafe { cui_bridge_get_checked(self.raw, n, &mut v) })?;
        Ok(v != 0)
    }
    pub fn current_value(&self, n: Node) -> Result<f64> {
        let n = self.valid(n)?;
        let mut v = 0.;
        check(unsafe { cui_bridge_get_value(self.raw, n, &mut v) })?;
        Ok(v)
    }
    pub fn on_change(&mut self, n: Node, mut f: impl FnMut() + 'static) -> Result<&mut Self> {
        self.on_change_event(n, move |_| f())
    }
    pub fn on_change_event(
        &mut self,
        n: Node,
        f: impl FnMut(&ChangeEvent) + 'static,
    ) -> Result<&mut Self> {
        let n = self.valid(n)?;
        self.callbacks
            .insert((n, 2), Rc::new(RefCell::new(Box::new(f))));
        Ok(self)
    }
    pub fn editable_text(&self, n: Node) -> Result<String> {
        let n = self.valid(n)?;
        let mut required = 0;
        let mut buf = vec![0u8; 4097];
        check(unsafe {
            cui_bridge_get_text(
                self.raw,
                n,
                buf.as_mut_ptr().cast(),
                buf.len() as u32,
                &mut required,
            )
        })?;
        String::from_utf8(buf[..required.saturating_sub(1) as usize].to_vec()).map_err(|_| Error(7))
    }
    pub fn date(&mut self, n: Node, value: Date) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_date(self.raw, n, value) })?;
        Ok(self)
    }
    pub fn current_date(&self, n: Node) -> Result<Date> {
        let n = self.valid(n)?;
        let mut value = Date::default();
        check(unsafe { cui_bridge_get_date(self.raw, n, &mut value) })?;
        Ok(value)
    }
    pub fn calendar_day_bounds(&self, n: Node, day: Date) -> Result<Rect> {
        let n = self.valid(n)?;
        let mut r = Rect::default();
        check(unsafe { cui_bridge_calendar_day_bounds(self.raw, n, day, &mut r) })?;
        Ok(r)
    }
    pub fn color_rgba(&mut self, n: Node, color: u32) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_color_rgba(self.raw, n, color) })?;
        Ok(self)
    }
    pub fn current_color_rgba(&self, n: Node) -> Result<u32> {
        let n = self.valid(n)?;
        let mut color = 0;
        check(unsafe { cui_bridge_get_color_rgba(self.raw, n, &mut color) })?;
        Ok(color)
    }
    pub fn color_part_bounds(&self, n: Node, part: u32) -> Result<Rect> {
        let n = self.valid(n)?;
        let mut r = Rect::default();
        check(unsafe { cui_bridge_color_part_bounds(self.raw, n, part, &mut r) })?;
        Ok(r)
    }
    pub fn time(&mut self, n: Node, value: Time) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_time(self.raw, n, value) })?;
        Ok(self)
    }
    pub fn current_time(&self, n: Node) -> Result<Time> {
        let n = self.valid(n)?;
        let mut value = Time::default();
        check(unsafe { cui_bridge_get_time(self.raw, n, &mut value) })?;
        Ok(value)
    }
    pub fn image_rgba(
        &mut self,
        n: Node,
        pixels: &[u8],
        width: u32,
        height: u32,
        stride: u32,
    ) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let count = u32::try_from(pixels.len()).map_err(|_| Error(3))?;
        check(unsafe {
            cui_bridge_set_image_rgba(self.raw, n, pixels.as_ptr(), width, height, stride, count)
        })?;
        Ok(self)
    }
    pub fn attachment_status(&mut self, n: Node, status: AttachmentStatus) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_attachment_status(self.raw, n, status as u32) })?;
        Ok(self)
    }
    pub fn scroll_extent(&mut self, n: Node, width: f32, height: f32) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_scroll_extent(self.raw, n, width, height) })?;
        Ok(self)
    }
    pub fn scroll(&mut self, x: f32, y: f32, dx: f32, dy: f32) -> Result<()> {
        check(unsafe { cui_bridge_queue_scroll(self.raw, x, y, dx, dy) })
    }
    pub fn total(&mut self, n: Node, total: u32) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_total(self.raw, n, total) })?;
        Ok(self)
    }
    pub fn step_status(&mut self, n: Node, status: u32) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_step_status(self.raw, n, status) })?;
        Ok(self)
    }
    pub fn numeric_part_bounds(&self, n: Node, part: u32) -> Result<Rect> {
        let n = self.valid(n)?;
        let mut r = Rect::default();
        check(unsafe { cui_bridge_numeric_part_bounds(self.raw, n, part, &mut r) })?;
        Ok(r)
    }
    pub fn visual_size(&mut self, n: Node, width: f32, height: f32) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_visual_size(self.raw, n, width, height) })?;
        Ok(self)
    }
    pub fn detail_text(&mut self, n: Node, text: &str) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let text = string(text)?;
        check(unsafe { cui_bridge_set_detail_text(self.raw, n, text.as_ptr()) })?;
        Ok(self)
    }
    pub fn open(&mut self, n: Node, open: bool) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_open(self.raw, n, open as u32) })?;
        Ok(self)
    }
    pub fn is_open(&self, n: Node) -> Result<bool> {
        let n = self.valid(n)?;
        let mut open = 0;
        check(unsafe { cui_bridge_get_open(self.raw, n, &mut open) })?;
        Ok(open != 0)
    }
    pub fn anchor(&mut self, surface: Node, anchor: Node) -> Result<&mut Self> {
        let surface = self.valid(surface)?;
        let anchor = self.valid(anchor)?;
        check(unsafe { cui_bridge_set_anchor(self.raw, surface, anchor) })?;
        Ok(self)
    }
    pub fn on_close_event(
        &mut self,
        n: Node,
        f: impl FnMut(&ChangeEvent) + 'static,
    ) -> Result<&mut Self> {
        let n = self.valid(n)?;
        self.callbacks
            .insert((n, 3), Rc::new(RefCell::new(Box::new(f))));
        Ok(self)
    }
    pub fn select(&mut self, group: Node, item: Node) -> Result<&mut Self> {
        let group = self.valid(group)?;
        let item = self.valid(item)?;
        check(unsafe { cui_bridge_set_selected(self.raw, group, item) })?;
        Ok(self)
    }
    pub fn selected(&self, group: Node) -> Result<Option<Node>> {
        let group = self.valid(group)?;
        let mut item = RawNode::default();
        check(unsafe { cui_bridge_get_selected(self.raw, group, &mut item) })?;
        Ok((item.slot != 0).then_some(Node {
            raw: item,
            app_id: self.id,
        }))
    }
    pub fn queue_text(&mut self, text: &str) -> Result<()> {
        let text = string(text)?;
        check(unsafe { cui_bridge_queue_text(self.raw, text.as_ptr()) })
    }
    pub fn pointer(&mut self, p: Phase, x: f32, y: f32) -> Result<()> {
        check(unsafe { cui_bridge_queue_pointer(self.raw, p as u32, x, y) })
    }
    pub fn key(&mut self, down: bool, k: Key, modifiers: u32, repeat: bool) -> Result<()> {
        check(unsafe {
            cui_bridge_queue_key(
                self.raw,
                if down { 0 } else { 1 },
                k as u32,
                modifiers,
                repeat as u32,
            )
        })
    }
    pub fn step(&mut self, dt: f32) -> Result<()> {
        check(unsafe { cui_bridge_step(self.raw, dt) })
    }
    pub fn dispatch(&mut self) -> Result<usize> {
        let mut count = 0;
        loop {
            let mut e = EventEx::default();
            let mut required = 0;
            let mut text = vec![0u8; 4097];
            let status = unsafe {
                cui_bridge_poll_event_ex(
                    self.raw,
                    &mut e,
                    text.as_mut_ptr().cast(),
                    text.len() as u32,
                    &mut required,
                )
            };
            if status == 1 {
                break;
            }
            check(status)?;
            let payload = ChangeEvent {
                node: Node {
                    raw: e.node,
                    app_id: self.id,
                },
                frame: e.frame,
                checked: e.checked != 0,
                value: e.value,
                text: String::from_utf8(text[..e.text_length as usize].to_vec())
                    .map_err(|_| Error(7))?,
                selected: (e.selected.slot != 0).then_some(Node {
                    raw: e.selected,
                    app_id: self.id,
                }),
            };
            if let Some(cb) = self.callbacks.get(&(e.node, e.kind)).cloned() {
                (cb.borrow_mut())(&payload);
                count += 1;
            }
        }
        Ok(count)
    }
    pub fn frame(&mut self, dt: f32) -> Result<usize> {
        self.step(dt)?;
        self.dispatch()
    }
    pub fn bounds(&self, n: Node) -> Result<Rect> {
        let n = self.valid(n)?;
        let mut r = Rect::default();
        check(unsafe { cui_bridge_node_bounds(self.raw, n, &mut r) })?;
        Ok(r)
    }
    pub fn dropdown_part_bounds(&self, n: Node, menu_part: bool) -> Result<Rect> {
        let n = self.valid(n)?;
        let mut r = Rect::default();
        check(unsafe { cui_bridge_dropdown_part_bounds(self.raw, n, menu_part as u32, &mut r) })?;
        Ok(r)
    }
    pub fn font_fallbacks(&mut self, paths: &[&str]) -> Result<&mut Self> {
        let count = u32::try_from(paths.len()).map_err(|_| Error(3))?;
        let owned: Vec<CString> = paths
            .iter()
            .map(|path| string(path))
            .collect::<Result<_>>()?;
        let pointers: Vec<*const c_char> = owned.iter().map(|path| path.as_ptr()).collect();
        check(unsafe { cui_bridge_set_font_fallbacks(self.raw, pointers.as_ptr(), count) })?;
        Ok(self)
    }
    pub fn close(&mut self) -> Result<()> {
        check(unsafe { cui_bridge_request_close(self.raw) })
    }
}
/// A fluent, app-borrowed node builder. `finish` releases the mutable app borrow.
pub struct Builder<'a> {
    app: &'a mut App,
    node: Node,
}
impl App {
    pub fn build_root(&mut self, kind: Kind, key: &str) -> Result<Builder<'_>> {
        let node = self.root(kind, key)?;
        Ok(Builder { app: self, node })
    }
    pub fn build_child(&mut self, parent: Node, kind: Kind, key: &str) -> Result<Builder<'_>> {
        let node = self.child(parent, kind, key)?;
        Ok(Builder { app: self, node })
    }
}
impl<'a> Builder<'a> {
    pub fn text(self, text: &str) -> Result<Self> {
        self.app.text(self.node, text)?;
        Ok(self)
    }
    pub fn gap_px(self, gap: f32) -> Result<Self> {
        self.app.gap_px(self.node, gap)?;
        Ok(self)
    }
    pub fn gap_units(self, gap: f32) -> Result<Self> {
        self.app.gap_units(self.node, gap)?;
        Ok(self)
    }
    pub fn variant(self, v: Variant) -> Result<Self> {
        self.app.variant(self.node, v)?;
        Ok(self)
    }
    pub fn size(self, v: Size) -> Result<Self> {
        self.app.size(self.node, v)?;
        Ok(self)
    }
    pub fn rounding(self, v: Rounding) -> Result<Self> {
        self.app.rounding(self.node, v)?;
        Ok(self)
    }
    pub fn disabled(self, v: bool) -> Result<Self> {
        self.app.component_disabled(self.node, v)?;
        Ok(self)
    }
    pub fn checked(self, v: bool) -> Result<Self> {
        self.app.checked(self.node, v)?;
        Ok(self)
    }
    pub fn value(self, v: f64) -> Result<Self> {
        self.app.value(self.node, v)?;
        Ok(self)
    }
    pub fn range(self, min: f64, max: f64, step: f64) -> Result<Self> {
        self.app.range(self.node, min, max, step)?;
        Ok(self)
    }
    pub fn hover_rgba(self, color: [u8; 4]) -> Result<Self> {
        self.app.hover_rgba(self.node, color)?;
        Ok(self)
    }
    pub fn on_click(self, f: impl FnMut() + 'static) -> Result<Self> {
        self.app.on_click(self.node, f)?;
        Ok(self)
    }
    pub fn on_change_event(self, f: impl FnMut(&ChangeEvent) + 'static) -> Result<Self> {
        self.app.on_change_event(self.node, f)?;
        Ok(self)
    }
    pub fn date(self, value: Date) -> Result<Self> {
        self.app.date(self.node, value)?;
        Ok(self)
    }
    pub fn color_rgba(self, color: u32) -> Result<Self> {
        self.app.color_rgba(self.node, color)?;
        Ok(self)
    }
    pub fn time(self, value: Time) -> Result<Self> {
        self.app.time(self.node, value)?;
        Ok(self)
    }
    pub fn image_rgba(self, pixels: &[u8], width: u32, height: u32, stride: u32) -> Result<Self> {
        self.app
            .image_rgba(self.node, pixels, width, height, stride)?;
        Ok(self)
    }
    pub fn attachment_status(self, status: AttachmentStatus) -> Result<Self> {
        self.app.attachment_status(self.node, status)?;
        Ok(self)
    }
    pub fn scroll_extent(self, width: f32, height: f32) -> Result<Self> {
        self.app.scroll_extent(self.node, width, height)?;
        Ok(self)
    }
    pub fn total(self, total: u32) -> Result<Self> {
        self.app.total(self.node, total)?;
        Ok(self)
    }
    pub fn step_status(self, status: u32) -> Result<Self> {
        self.app.step_status(self.node, status)?;
        Ok(self)
    }
    pub fn visual_size(self, width: f32, height: f32) -> Result<Self> {
        self.app.visual_size(self.node, width, height)?;
        Ok(self)
    }
    pub fn detail_text(self, text: &str) -> Result<Self> {
        self.app.detail_text(self.node, text)?;
        Ok(self)
    }
    pub fn open(self, open: bool) -> Result<Self> {
        self.app.open(self.node, open)?;
        Ok(self)
    }
    pub fn select(self, item: Node) -> Result<Self> {
        self.app.select(self.node, item)?;
        Ok(self)
    }
    pub fn anchor(self, anchor: Node) -> Result<Self> {
        self.app.anchor(self.node, anchor)?;
        Ok(self)
    }
    pub fn on_close_event(self, f: impl FnMut(&ChangeEvent) + 'static) -> Result<Self> {
        self.app.on_close_event(self.node, f)?;
        Ok(self)
    }
    pub fn finish(self) -> Node {
        self.node
    }
}
