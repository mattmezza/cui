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
struct ChartDatumRaw {
    label_utf8: *const c_char,
    value: f64,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ChartKind {
    Line = 0,
    Area = 1,
    Bar = 2,
    Pie = 3,
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
#[derive(Clone, Copy, Default, Debug, PartialEq)]
pub struct PlotPoint {
    pub x: f64,
    pub y: f64,
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
    pub checked_raw: u32,
    pub value: f64,
    pub text: String,
    pub selected: Option<Node>,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct DataTableState {
    pub row: u32,
    pub column: u32,
    pub sort_column: u32,
    pub sort_direction: u32,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct QuestionnaireState {
    pub current: Option<Node>,
    pub complete: bool,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DataTableSelectionMode {
    Row = 0,
    Column = 1,
    Cell = 2,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DockAxis {
    Horizontal = 0,
    Vertical = 1,
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
pub enum TextViewFormat {
    Markdown = 0,
    Html = 1,
    Plain = 2,
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
#[cfg_attr(not(cui_bridge_static), link(name = "cui_bridge"))]
extern "C" {
    fn cui_bridge_abi_version() -> u32;
    fn cui_bridge_theme_preset_count() -> u32;
    fn cui_bridge_set_root_padding(a: *mut c_void, n: RawNode, padding: f32) -> u32;
    fn cui_bridge_theme_preset_name(
        index: u32,
        destination: *mut c_char,
        capacity: u32,
        length: *mut u32,
    ) -> u32;
    fn cui_bridge_set_theme_preset(a: *mut c_void, index: u32) -> u32;
    fn cui_bridge_get_theme_preset(a: *const c_void, out: *mut u32) -> u32;
    fn cui_bridge_clipboard_set(a: *mut c_void, text: *const c_char) -> u32;
    fn cui_bridge_clipboard_get(
        a: *mut c_void,
        destination: *mut c_char,
        capacity: u32,
        length: *mut u32,
    ) -> u32;
    fn cui_bridge_speech_start(a: *mut c_void, n: RawNode) -> u32;
    fn cui_bridge_speech_report(
        a: *mut c_void,
        n: RawNode,
        text: *const c_char,
        final_phrase: u32,
    ) -> u32;
    fn cui_bridge_speech_finish(a: *mut c_void, n: RawNode) -> u32;
    fn cui_bridge_speech_transcript(
        a: *mut c_void,
        n: RawNode,
        destination: *mut c_char,
        capacity: u32,
        length: *mut u32,
    ) -> u32;
    fn cui_bridge_questionnaire_set_question(
        a: *mut c_void,
        question: RawNode,
        required: u32,
        multiple: u32,
    ) -> u32;
    fn cui_bridge_questionnaire_get_state(
        a: *mut c_void,
        form: RawNode,
        current: *mut RawNode,
        complete: *mut u32,
    ) -> u32;
    fn cui_bridge_questionnaire_choice_selected(
        a: *mut c_void,
        choice: RawNode,
        selected: *mut u32,
    ) -> u32;
    fn cui_bridge_questionnaire_next(a: *mut c_void, form: RawNode) -> u32;
    fn cui_bridge_questionnaire_previous(a: *mut c_void, form: RawNode) -> u32;
    fn cui_bridge_questionnaire_submit(a: *mut c_void, form: RawNode) -> u32;
    fn cui_bridge_dock_activate(a: *mut c_void, dock: RawNode, panel: RawNode) -> u32;
    fn cui_bridge_dock_move_split(
        a: *mut c_void,
        dock: RawNode,
        panel: RawNode,
        reference: RawNode,
        axis: u32,
        ratio: f64,
        before: u32,
    ) -> u32;
    fn cui_bridge_dock_get_active(a: *mut c_void, dock: RawNode, out: *mut RawNode) -> u32;
    fn cui_bridge_dock_panel_bounds(
        a: *mut c_void,
        dock: RawNode,
        panel: RawNode,
        out: *mut Rect,
    ) -> u32;
    fn cui_bridge_dock_serialize(
        a: *mut c_void,
        dock: RawNode,
        destination: *mut c_char,
        capacity: u32,
        required: *mut u32,
    ) -> u32;
    fn cui_bridge_settings_set_query(
        a: *mut c_void,
        settings: RawNode,
        query: *const c_char,
    ) -> u32;
    fn cui_bridge_settings_get_page(a: *mut c_void, settings: RawNode, out: *mut RawNode) -> u32;
    fn cui_bridge_settings_set_page(a: *mut c_void, settings: RawNode, page: RawNode) -> u32;
    fn cui_bridge_settings_reset_page(a: *mut c_void, settings: RawNode, page: RawNode) -> u32;
    fn cui_bridge_settings_field_bounds(
        a: *mut c_void,
        settings: RawNode,
        field: RawNode,
        out: *mut Rect,
    ) -> u32;
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
    fn cui_bridge_resizable_handle_bounds(
        a: *mut c_void,
        group: RawNode,
        item: RawNode,
        out: *mut Rect,
    ) -> u32;
    fn cui_bridge_command_item_bounds(
        a: *mut c_void,
        command: RawNode,
        item: RawNode,
        out: *mut Rect,
    ) -> u32;
    fn cui_bridge_virtual_list_item_bounds(
        a: *mut c_void,
        list: RawNode,
        item: RawNode,
        out: *mut Rect,
    ) -> u32;
    fn cui_bridge_chart_set_data(
        a: *mut c_void,
        n: RawNode,
        data: *const ChartDatumRaw,
        count: u32,
    ) -> u32;
    fn cui_bridge_chart_set_kind(a: *mut c_void, n: RawNode, kind: u32) -> u32;
    fn cui_bridge_chart_get_datum(
        a: *mut c_void,
        n: RawNode,
        index: u32,
        label: *mut c_char,
        capacity: u32,
        required: *mut u32,
        value: *mut f64,
    ) -> u32;
    fn cui_bridge_data_table_set(
        a: *mut c_void,
        n: RawNode,
        headers: *const *const c_char,
        column_count: u32,
        cells: *const *const c_char,
        row_count: u32,
    ) -> u32;
    fn cui_bridge_data_table_get_cell(
        a: *mut c_void,
        n: RawNode,
        row: u32,
        column: u32,
        destination: *mut c_char,
        capacity: u32,
        required: *mut u32,
    ) -> u32;
    fn cui_bridge_data_table_get_state(
        a: *mut c_void,
        n: RawNode,
        row: *mut u32,
        column: *mut u32,
        sort_column: *mut u32,
        sort_direction: *mut u32,
    ) -> u32;
    fn cui_bridge_data_table_set_selection_mode(a: *mut c_void, n: RawNode, mode: u32) -> u32;
    fn cui_bridge_data_table_cell_bounds(
        a: *mut c_void,
        n: RawNode,
        row: u32,
        column: u32,
        out: *mut Rect,
    ) -> u32;
    fn cui_bridge_data_table_header_bounds(
        a: *mut c_void,
        n: RawNode,
        column: u32,
        out: *mut Rect,
    ) -> u32;
    fn cui_bridge_plot_set_points(
        a: *mut c_void,
        n: RawNode,
        points: *const PlotPoint,
        count: u32,
    ) -> u32;
    fn cui_bridge_plot_get_point(
        a: *mut c_void,
        n: RawNode,
        index: u32,
        out: *mut PlotPoint,
    ) -> u32;
    fn cui_bridge_set_text_view_format(a: *mut c_void, n: RawNode, format: u32) -> u32;
    fn cui_bridge_text_view_select(a: *mut c_void, n: RawNode, anchor: u32, cursor: u32) -> u32;
    fn cui_bridge_text_view_copy_selection(
        a: *mut c_void,
        n: RawNode,
        destination: *mut c_char,
        capacity: u32,
        required: *mut u32,
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
    MessageScroller = 64,
    Carousel = 65,
    Resizable = 66,
    TextView = 67,
    Editor = 68,
    Chart = 69,
    Table = 70,
    DataTable = 71,
    Plot = 72,
    Command = 73,
    VirtualList = 74,
    Sidebar = 75,
    RootView = 76,
    Speech = 77,
    Questionnaire = 78,
    Dock = 79,
    Settings = 80,
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
    running: bool,
    _local: PhantomData<Rc<()>>,
}
type NativeRunner =
    unsafe extern "C" fn(*mut c_void, extern "C" fn(*mut c_void) -> u32, *mut c_void) -> u32;
struct RunContext<'a> {
    after_frame: &'a mut dyn FnMut(&mut App) -> Result<()>,
    app: *mut App,
    error: Option<Error>,
    panic: Option<Box<dyn std::any::Any + Send>>,
}
extern "C" fn run_after_frame(user: *mut c_void) -> u32 {
    let context = unsafe { &mut *user.cast::<RunContext>() };
    let outcome = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        let app = unsafe { &mut *context.app };
        app.dispatch()?;
        (context.after_frame)(app)
    }));
    match outcome {
        Ok(Ok(_)) => 0,
        Ok(Err(error)) => {
            context.error = Some(error);
            3
        }
        Err(payload) => {
            context.panic = Some(payload);
            3
        }
    }
}
#[cfg(cui_bridge_static)]
fn native_runner() -> Option<NativeRunner> {
    extern "C" {
        fn cui_bridge_run(
            app: *mut c_void,
            callback: extern "C" fn(*mut c_void) -> u32,
            user: *mut c_void,
        ) -> u32;
    }
    Some(cui_bridge_run)
}
#[cfg(all(not(cui_bridge_static), target_os = "linux"))]
fn native_runner() -> Option<NativeRunner> {
    #[link(name = "dl")]
    extern "C" {
        fn dlsym(handle: *mut c_void, symbol: *const c_char) -> *mut c_void;
    }
    let symbol = unsafe { dlsym(std::ptr::null_mut(), b"cui_bridge_run\0".as_ptr().cast()) };
    if symbol.is_null() {
        None
    } else {
        Some(unsafe { std::mem::transmute::<*mut c_void, NativeRunner>(symbol) })
    }
}
#[cfg(all(not(cui_bridge_static), not(target_os = "linux")))]
fn native_runner() -> Option<NativeRunner> {
    None
}
impl Drop for App {
    fn drop(&mut self) {
        unsafe { cui_bridge_destroy(self.raw) }
    }
}
impl App {
    pub fn theme_presets() -> Result<Vec<String>> {
        let mut names = Vec::new();
        for index in 0..unsafe { cui_bridge_theme_preset_count() } {
            let mut buf = vec![0u8; 4097];
            let mut length = 0;
            check(unsafe {
                cui_bridge_theme_preset_name(
                    index,
                    buf.as_mut_ptr().cast(),
                    buf.len() as u32,
                    &mut length,
                )
            })?;
            names.push(String::from_utf8(buf[..length as usize].to_vec()).map_err(|_| Error(7))?);
        }
        Ok(names)
    }
    pub fn set_theme_preset(&mut self, index: u32) -> Result<&mut Self> {
        check(unsafe { cui_bridge_set_theme_preset(self.raw, index) })?;
        Ok(self)
    }
    pub fn theme_preset(&self) -> Result<Option<u32>> {
        let mut index = 0;
        let status = unsafe { cui_bridge_get_theme_preset(self.raw, &mut index) };
        if status == 1 {
            return Ok(None);
        }
        check(status)?;
        Ok(Some(index))
    }
    pub fn clipboard_set(&mut self, text: &str) -> Result<&mut Self> {
        if text.len() > 65536 {
            return Err(Error(3));
        }
        let text = CString::new(text).map_err(|_| Error(3))?;
        check(unsafe { cui_bridge_clipboard_set(self.raw, text.as_ptr()) })?;
        Ok(self)
    }
    pub fn clipboard_get(&self) -> Result<String> {
        let mut buf = vec![0u8; 65537];
        let mut length = 0;
        check(unsafe {
            cui_bridge_clipboard_get(
                self.raw,
                buf.as_mut_ptr().cast(),
                buf.len() as u32,
                &mut length,
            )
        })?;
        String::from_utf8(buf[..length as usize].to_vec()).map_err(|_| Error(7))
    }
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
            running: false,
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
    pub fn text_view_format(&mut self, n: Node, format: TextViewFormat) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_text_view_format(self.raw, n, format as u32) })?;
        Ok(self)
    }
    pub fn text_view_select(&mut self, n: Node, anchor: u32, cursor: u32) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_text_view_select(self.raw, n, anchor, cursor) })?;
        Ok(self)
    }
    pub fn text_view_selection(&self, n: Node) -> Result<String> {
        let n = self.valid(n)?;
        let mut required = 0;
        let mut buf = vec![0u8; 4097];
        check(unsafe {
            cui_bridge_text_view_copy_selection(
                self.raw,
                n,
                buf.as_mut_ptr().cast(),
                buf.len() as u32,
                &mut required,
            )
        })?;
        String::from_utf8(buf[..required.saturating_sub(1) as usize].to_vec()).map_err(|_| Error(7))
    }
    pub fn chart_data(&mut self, n: Node, data: &[(&str, f64)]) -> Result<&mut Self> {
        let n = self.valid(n)?;
        if data.len() > 64 {
            return Err(Error(3));
        }
        let labels: Vec<CString> = data
            .iter()
            .map(|(label, _)| string(label))
            .collect::<Result<_>>()?;
        let raw: Vec<ChartDatumRaw> = labels
            .iter()
            .zip(data.iter())
            .map(|(label, (_, value))| ChartDatumRaw {
                label_utf8: label.as_ptr(),
                value: *value,
            })
            .collect();
        check(unsafe { cui_bridge_chart_set_data(self.raw, n, raw.as_ptr(), raw.len() as u32) })?;
        Ok(self)
    }
    pub fn chart_kind(&mut self, n: Node, kind: ChartKind) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_chart_set_kind(self.raw, n, kind as u32) })?;
        Ok(self)
    }
    pub fn chart_datum(&self, n: Node, index: u32) -> Result<(String, f64)> {
        let n = self.valid(n)?;
        let mut buf = vec![0u8; 4097];
        let mut required = 0;
        let mut value = 0.;
        check(unsafe {
            cui_bridge_chart_get_datum(
                self.raw,
                n,
                index,
                buf.as_mut_ptr().cast(),
                buf.len() as u32,
                &mut required,
                &mut value,
            )
        })?;
        let label = String::from_utf8(buf[..required.saturating_sub(1) as usize].to_vec())
            .map_err(|_| Error(7))?;
        Ok((label, value))
    }
    pub fn data_table(&mut self, n: Node, headers: &[&str], rows: &[&[&str]]) -> Result<&mut Self> {
        let n = self.valid(n)?;
        if headers.is_empty()
            || headers.len() > 16
            || rows.len() > 32
            || rows.iter().any(|r| r.len() != headers.len())
        {
            return Err(Error(3));
        }
        let header_strings: Vec<CString> =
            headers.iter().map(|h| string(h)).collect::<Result<_>>()?;
        let cell_strings: Vec<CString> = rows
            .iter()
            .flat_map(|r| r.iter())
            .map(|c| string(c))
            .collect::<Result<_>>()?;
        let header_ptrs: Vec<*const c_char> = header_strings.iter().map(|s| s.as_ptr()).collect();
        let cell_ptrs: Vec<*const c_char> = cell_strings.iter().map(|s| s.as_ptr()).collect();
        check(unsafe {
            cui_bridge_data_table_set(
                self.raw,
                n,
                header_ptrs.as_ptr(),
                headers.len() as u32,
                cell_ptrs.as_ptr(),
                rows.len() as u32,
            )
        })?;
        Ok(self)
    }
    pub fn data_table_cell(&self, n: Node, row: u32, column: u32) -> Result<String> {
        let n = self.valid(n)?;
        let mut buf = vec![0u8; 4097];
        let mut required = 0;
        check(unsafe {
            cui_bridge_data_table_get_cell(
                self.raw,
                n,
                row,
                column,
                buf.as_mut_ptr().cast(),
                buf.len() as u32,
                &mut required,
            )
        })?;
        String::from_utf8(buf[..required.saturating_sub(1) as usize].to_vec()).map_err(|_| Error(7))
    }
    pub fn data_table_state(&self, n: Node) -> Result<DataTableState> {
        let n = self.valid(n)?;
        let mut state = DataTableState {
            row: 0,
            column: 0,
            sort_column: 0,
            sort_direction: 0,
        };
        check(unsafe {
            cui_bridge_data_table_get_state(
                self.raw,
                n,
                &mut state.row,
                &mut state.column,
                &mut state.sort_column,
                &mut state.sort_direction,
            )
        })?;
        Ok(state)
    }
    pub fn data_table_selection_mode(
        &mut self,
        n: Node,
        mode: DataTableSelectionMode,
    ) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_data_table_set_selection_mode(self.raw, n, mode as u32) })?;
        Ok(self)
    }
    pub fn root_padding(&mut self, n: Node, padding: f32) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_set_root_padding(self.raw, n, padding) })?;
        Ok(self)
    }
    pub fn speech_start(&mut self, n: Node) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_speech_start(self.raw, n) })?;
        Ok(self)
    }
    pub fn speech_report(&mut self, n: Node, text: &str, final_phrase: bool) -> Result<&mut Self> {
        let n = self.valid(n)?;
        let text = string(text)?;
        check(unsafe {
            cui_bridge_speech_report(self.raw, n, text.as_ptr(), final_phrase as u32)
        })?;
        Ok(self)
    }
    pub fn speech_finish(&mut self, n: Node) -> Result<&mut Self> {
        let n = self.valid(n)?;
        check(unsafe { cui_bridge_speech_finish(self.raw, n) })?;
        Ok(self)
    }
    pub fn speech_transcript(&self, n: Node) -> Result<String> {
        let n = self.valid(n)?;
        let mut buf = vec![0u8; 4097];
        let mut length = 0;
        check(unsafe {
            cui_bridge_speech_transcript(
                self.raw,
                n,
                buf.as_mut_ptr().cast(),
                buf.len() as u32,
                &mut length,
            )
        })?;
        String::from_utf8(buf[..length as usize].to_vec()).map_err(|_| Error(7))
    }
    pub fn questionnaire_question(
        &mut self,
        question: Node,
        required: bool,
        multiple: bool,
    ) -> Result<&mut Self> {
        let question = self.valid(question)?;
        check(unsafe {
            cui_bridge_questionnaire_set_question(
                self.raw,
                question,
                required as u32,
                multiple as u32,
            )
        })?;
        Ok(self)
    }
    pub fn questionnaire_state(&self, form: Node) -> Result<QuestionnaireState> {
        let form = self.valid(form)?;
        let mut current = RawNode::default();
        let mut complete = 0;
        check(unsafe {
            cui_bridge_questionnaire_get_state(self.raw, form, &mut current, &mut complete)
        })?;
        Ok(QuestionnaireState {
            current: (current.slot != 0).then_some(Node {
                raw: current,
                app_id: self.id,
            }),
            complete: complete != 0,
        })
    }
    pub fn questionnaire_choice_selected(&self, choice: Node) -> Result<bool> {
        let choice = self.valid(choice)?;
        let mut selected = 0;
        check(unsafe {
            cui_bridge_questionnaire_choice_selected(self.raw, choice, &mut selected)
        })?;
        Ok(selected != 0)
    }
    pub fn questionnaire_next(&mut self, form: Node) -> Result<&mut Self> {
        let form = self.valid(form)?;
        check(unsafe { cui_bridge_questionnaire_next(self.raw, form) })?;
        Ok(self)
    }
    pub fn questionnaire_previous(&mut self, form: Node) -> Result<&mut Self> {
        let form = self.valid(form)?;
        check(unsafe { cui_bridge_questionnaire_previous(self.raw, form) })?;
        Ok(self)
    }
    pub fn questionnaire_submit(&mut self, form: Node) -> Result<&mut Self> {
        let form = self.valid(form)?;
        check(unsafe { cui_bridge_questionnaire_submit(self.raw, form) })?;
        Ok(self)
    }
    pub fn dock_activate(&mut self, dock: Node, panel: Node) -> Result<&mut Self> {
        let dock = self.valid(dock)?;
        let panel = self.valid(panel)?;
        check(unsafe { cui_bridge_dock_activate(self.raw, dock, panel) })?;
        Ok(self)
    }
    pub fn dock_move_split(
        &mut self,
        dock: Node,
        panel: Node,
        reference: Node,
        axis: DockAxis,
        ratio: f64,
        before: bool,
    ) -> Result<&mut Self> {
        let dock = self.valid(dock)?;
        let panel = self.valid(panel)?;
        let reference = self.valid(reference)?;
        check(unsafe {
            cui_bridge_dock_move_split(
                self.raw,
                dock,
                panel,
                reference,
                axis as u32,
                ratio,
                before as u32,
            )
        })?;
        Ok(self)
    }
    pub fn dock_active(&self, dock: Node) -> Result<Option<Node>> {
        let dock = self.valid(dock)?;
        let mut out = RawNode::default();
        check(unsafe { cui_bridge_dock_get_active(self.raw, dock, &mut out) })?;
        Ok((out.slot != 0).then_some(Node {
            raw: out,
            app_id: self.id,
        }))
    }
    pub fn dock_panel_bounds(&self, dock: Node, panel: Node) -> Result<Rect> {
        let dock = self.valid(dock)?;
        let panel = self.valid(panel)?;
        let mut out = Rect::default();
        check(unsafe { cui_bridge_dock_panel_bounds(self.raw, dock, panel, &mut out) })?;
        Ok(out)
    }
    pub fn dock_serialize(&self, dock: Node) -> Result<String> {
        let dock = self.valid(dock)?;
        let mut buf = vec![0u8; 65537];
        let mut required = 0;
        check(unsafe {
            cui_bridge_dock_serialize(
                self.raw,
                dock,
                buf.as_mut_ptr().cast(),
                buf.len() as u32,
                &mut required,
            )
        })?;
        String::from_utf8(buf[..required.saturating_sub(1) as usize].to_vec()).map_err(|_| Error(7))
    }
    pub fn settings_query(&mut self, settings: Node, query: &str) -> Result<&mut Self> {
        let settings = self.valid(settings)?;
        let query = string(query)?;
        check(unsafe { cui_bridge_settings_set_query(self.raw, settings, query.as_ptr()) })?;
        Ok(self)
    }
    pub fn settings_page(&self, settings: Node) -> Result<Option<Node>> {
        let settings = self.valid(settings)?;
        let mut out = RawNode::default();
        check(unsafe { cui_bridge_settings_get_page(self.raw, settings, &mut out) })?;
        Ok((out.slot != 0).then_some(Node {
            raw: out,
            app_id: self.id,
        }))
    }
    pub fn settings_set_page(&mut self, settings: Node, page: Node) -> Result<&mut Self> {
        let settings = self.valid(settings)?;
        let page = self.valid(page)?;
        check(unsafe { cui_bridge_settings_set_page(self.raw, settings, page) })?;
        Ok(self)
    }
    pub fn settings_reset_page(&mut self, settings: Node, page: Node) -> Result<&mut Self> {
        let settings = self.valid(settings)?;
        let page = self.valid(page)?;
        check(unsafe { cui_bridge_settings_reset_page(self.raw, settings, page) })?;
        Ok(self)
    }
    pub fn settings_field_bounds(&self, settings: Node, field: Node) -> Result<Rect> {
        let settings = self.valid(settings)?;
        let field = self.valid(field)?;
        let mut out = Rect::default();
        check(unsafe { cui_bridge_settings_field_bounds(self.raw, settings, field, &mut out) })?;
        Ok(out)
    }
    pub fn data_table_cell_bounds(&self, n: Node, row: u32, column: u32) -> Result<Rect> {
        let n = self.valid(n)?;
        let mut out = Rect::default();
        check(unsafe { cui_bridge_data_table_cell_bounds(self.raw, n, row, column, &mut out) })?;
        Ok(out)
    }
    pub fn data_table_header_bounds(&self, n: Node, column: u32) -> Result<Rect> {
        let n = self.valid(n)?;
        let mut out = Rect::default();
        check(unsafe { cui_bridge_data_table_header_bounds(self.raw, n, column, &mut out) })?;
        Ok(out)
    }
    pub fn plot_points(&mut self, n: Node, points: &[PlotPoint]) -> Result<&mut Self> {
        let n = self.valid(n)?;
        if points.len() > 256 {
            return Err(Error(3));
        }
        check(unsafe {
            cui_bridge_plot_set_points(self.raw, n, points.as_ptr(), points.len() as u32)
        })?;
        Ok(self)
    }
    pub fn plot_point(&self, n: Node, index: u32) -> Result<PlotPoint> {
        let n = self.valid(n)?;
        let mut out = PlotPoint::default();
        check(unsafe { cui_bridge_plot_get_point(self.raw, n, index, &mut out) })?;
        Ok(out)
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
        if self.running {
            return Err(Error(8));
        }
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
                checked_raw: e.checked,
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
    /// Own the native event loop, including live resize. Panics from callbacks
    /// resume only after the native runner has returned and released its host.
    pub fn run(&mut self) -> Result<()> {
        self.run_with(|_| Ok(()))
    }
    /// Optional after-frame hook for app updates or requesting close.
    pub fn run_with(&mut self, mut after_frame: impl FnMut(&mut App) -> Result<()>) -> Result<()> {
        if self.running {
            return Err(Error(8));
        }
        let runner = native_runner().ok_or(Error(9))?;
        let mut context = RunContext {
            after_frame: &mut after_frame,
            app: self as *mut App,
            error: None,
            panic: None,
        };
        self.running = true;
        let status = unsafe {
            runner(
                self.raw,
                run_after_frame,
                (&mut context as *mut RunContext).cast(),
            )
        };
        self.running = false;
        if let Some(payload) = context.panic {
            std::panic::resume_unwind(payload);
        }
        if let Some(error) = context.error {
            return Err(error);
        }
        if status == 2 {
            Ok(())
        } else {
            check(status)
        }
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
    pub fn resizable_handle_bounds(&self, group: Node, preceding_item: Node) -> Result<Rect> {
        let group = self.valid(group)?;
        let item = self.valid(preceding_item)?;
        let mut r = Rect::default();
        check(unsafe { cui_bridge_resizable_handle_bounds(self.raw, group, item, &mut r) })?;
        Ok(r)
    }
    pub fn command_item_bounds(&self, command: Node, item: Node) -> Result<Rect> {
        let command = self.valid(command)?;
        let item = self.valid(item)?;
        let mut r = Rect::default();
        check(unsafe { cui_bridge_command_item_bounds(self.raw, command, item, &mut r) })?;
        Ok(r)
    }
    pub fn virtual_list_item_bounds(&self, list: Node, item: Node) -> Result<Rect> {
        let list = self.valid(list)?;
        let item = self.valid(item)?;
        let mut r = Rect::default();
        check(unsafe { cui_bridge_virtual_list_item_bounds(self.raw, list, item, &mut r) })?;
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
    pub fn text_view_format(self, format: TextViewFormat) -> Result<Self> {
        self.app.text_view_format(self.node, format)?;
        Ok(self)
    }
    pub fn chart_data(self, data: &[(&str, f64)]) -> Result<Self> {
        self.app.chart_data(self.node, data)?;
        Ok(self)
    }
    pub fn chart_kind(self, kind: ChartKind) -> Result<Self> {
        self.app.chart_kind(self.node, kind)?;
        Ok(self)
    }
    pub fn data_table(self, headers: &[&str], rows: &[&[&str]]) -> Result<Self> {
        self.app.data_table(self.node, headers, rows)?;
        Ok(self)
    }
    pub fn data_table_selection_mode(self, mode: DataTableSelectionMode) -> Result<Self> {
        self.app.data_table_selection_mode(self.node, mode)?;
        Ok(self)
    }
    pub fn root_padding(self, padding: f32) -> Result<Self> {
        self.app.root_padding(self.node, padding)?;
        Ok(self)
    }
    pub fn questionnaire_question(self, required: bool, multiple: bool) -> Result<Self> {
        self.app
            .questionnaire_question(self.node, required, multiple)?;
        Ok(self)
    }
    pub fn settings_query(self, query: &str) -> Result<Self> {
        self.app.settings_query(self.node, query)?;
        Ok(self)
    }
    pub fn plot_points(self, points: &[PlotPoint]) -> Result<Self> {
        self.app.plot_points(self.node, points)?;
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
