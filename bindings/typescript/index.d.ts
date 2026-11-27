/** Native CUI bindings. Build bindings/node first; no npm dependencies. */
export type Backend = 'headless' | 'sdl';
export interface AppOptions { initiallyHidden?: boolean; width?: number; height?: number; backend?: Backend; title?: string; fontPath?: string; maxNodes?: number; maxEvents?: number }
export interface Bounds { x: number; y: number; width: number; height: number }
export type Key = 'tab' | 'enter' | 'space' | 'escape' | 'left' | 'right' | 'up' | 'down';
export type KeyPhase = 'down' | 'up';
export type PointerPhase = 'move' | 'down' | 'up';
export interface ClickEvent<T extends Node = Button> { node: T; frame: number }
export interface Node { key(value: string): this; bounds(): Bounds; remove(): this }
export interface Container extends Node { children(...items: (Node | Node[])[]): this; gap(value: number): this; gap_px(value: number): this; gap_units(value: number): this }
export interface Row extends Container {}
export interface Column extends Container {}
export interface Label extends Node { label(value: string): this }
export type DesignSize = 'inherit' | 'xs' | 'sm' | 'base' | 'md' | 'lg' | 'xl' | '2xl' | '3xl' | '4xl' | '5xl' | '6xl' | '7xl' | '8xl' | '9xl';
/** Shared scale is currently supported by Button, Avatar and Stepper. */
export interface DesignSized {design_size(value: DesignSize): this}
export interface Button extends Node, DesignSized {
  label(value: string): this; primary(): this; secondary(): this; outline(): this; ghost(): this;
  inherit(): this; xs(): this; sm(): this; small(): this; base(): this; md(): this; medium(): this; lg(): this; large(): this; xl(): this; size(value: "inherit" | "xs" | "sm" | "base" | "md" | "lg" | "xl" | "2xl" | "3xl" | "4xl" | "5xl" | "6xl" | "7xl" | "8xl" | "9xl"): this;
  rounded_theme(): this; rounded_none(): this; rounded_sm(): this; rounded_md(): this; rounded_px(radius: number): this;
  disabled(value?: boolean): this; hover_color(value: string | [number, number, number, number]): this;
  on_click(callback: (event: ClickEvent) => void): this;
}
export function row(): Row & ShadowStyled & PartShadowStyled<0|1>;
export function column(): Column & ShadowStyled & PartShadowStyled<0|1>;
export function label(text?: string): Label & ShadowStyled & PartShadowStyled<0|1>;
export function button(key?: string): Button & ShadowStyled & PartShadowStyled<0|1>;
export class App {
  supportsVirtualListAxis(): boolean;
  supportsVirtualListRows(): boolean;
  /** Submit without dispatch. Caller owns draining the shared queue. */
  step_only(dt?: number): this;
  poll_notification_v1(): NotificationV1 | undefined;
  poll_value_notification_v1(): ValueNotificationV1 | undefined;
  poll_component_notification_v1(): ComponentNotificationV1 | undefined;
  supports_attachment_actions(): boolean;

  constructor(options?: AppOptions);
  set_window_visible(visible: boolean): this;
  window_visible(): boolean;
  mount(root: Row | Column | RootView): this;
  /** Optional per-app responsive in-app stack; legacy behavior is the default. */
  supports_responsive_notifications(): boolean;
  /** Enables wrapping and independent Close controls. Call outside a step. */
  set_responsive_notifications(enabled: boolean): this;
  /** Whether this app's native bridge offers retained Stepper Fill. */
  supports_stepper_fill(): boolean;
  supports_stepper_wrapped_fill(): boolean;
  /** Whether this app's native bridge supports fractional Carousel slides. */
  supports_carousel_axis(): boolean;
  supports_carousel_looping(): boolean;
  supports_carousel_controls(): boolean;
  supports_carousel_control_sizes(): boolean;
  supports_carousel_fit(): boolean;
  supports_carousel_navigation(): boolean;
  supports_carousel_basis(): boolean;
  /** Whether this app's native bridge can animate retained Radio selection. */
  supports_radio_motion(): boolean;
  supports_radio_paint(): boolean;
  /** Whether this app's bridge supports signed Radio and RadioGroup Tab order. */
  supports_radio_tab_index(): boolean;
  /** Whether this app's native bridge can paint passive children inside RadioGroup Items. */
  supports_radio_item_content(): boolean;
  /** Whether this app's native bridge can attach hints to retained radio options. */
  supports_radio_tooltip(): boolean;
  pointer(phase: PointerPhase, x: number, y: number): this;
  key(phase: KeyPhase, key: Key, modifiers?: number, repeat?: boolean): this;
  text_input(text: string): this;
  step(dt?: number): number;
  /** Blocking native runner; calls the optional synchronous hook after dispatch. */
  run(afterFrame?: (app: App) => void): this;
  start(fps?: number): this;
  stop(): this;
  close(): this;
  dispose(): void;
}

export interface ChangeEvent {node: Node; frame: number; checked: boolean; value: number; text: string; selectedHandle: {slot: number; generation: number}}
export interface BooleanControl extends Label {checked(value?: boolean): this; is_checked(): boolean; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface NumericControl extends Node {value(value: number): this; current_value(): number}
export interface Slider extends NumericControl {range(minimum: number, maximum: number, step?: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Input extends Label {current_text(): string; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface NumberInput extends Input, Slider {}
export function checkbox(key?: string, text?: string): BooleanControl & PartShadowStyled<0|1|2>;
/** Packed colors are numeric 0xRRGGBBAA. Explicit zero remains present.
 * Geometry and text metrics are BASE-only; description fields require direct Items.
 * RadioGroup accepts root fields on BASE and DISABLED only. */
export interface RadioPaint {
  rootBackground?: number; rootBorder?: number; rootBorderWidth?: number;
  rootRadius?: number; rootPadding?: number; rootGap?: number;
  indicatorBackground?: number; indicatorBorder?: number; indicatorBorderWidth?: number;
  indicatorRadius?: number; indicatorExtent?: number; markColor?: number;
  labelColor?: number; descriptionColor?: number; labelSize?: number; descriptionSize?: number;
  labelWeight?: 0|1|2|3; descriptionWeight?: 0|1|2|3;
}
export const RadioPaintLayer: Readonly<{BASE:0; CHECKED:1; HOVERED:2; PRESSED:3; FOCUSED:4; DISABLED:5}>;
export interface RadioPaintStyled {
  /** Replaces a layer; null or an empty object clears it. Paint emits no notification. */
  radio_paint(layer: 0|1|2|3|4|5, paint: RadioPaint | null): this;
}
export interface RadioSelectionMotion { animate_selection(enabled?: boolean): this }
export interface RadioTabOrder {
  /** Lower signed indices come first. null restores default order. */
  tab_index(index: number | null): this;
}
export interface RadioTooltip {
  /** For standalone Radio or direct RadioGroup Items. null or empty clears the hint. */
  radio_tooltip(text: string | null, delayMs?: number): this;
}
export function radio(key?: string, text?: string): BooleanControl & RadioPaintStyled & RadioSelectionMotion & RadioTabOrder & RadioTooltip & PartShadowStyled<0|1|2>;
export function switch_control(key?: string, text?: string): BooleanControl & PartShadowStyled<0|1|3|4>;
export function toggle(key?: string, text?: string): BooleanControl;
export function slider(key?: string): Slider & PartShadowStyled<0|1|3>;
export function progress(key?: string): NumericControl;
export function badge(key?: string, text?: string): Label & ShadowStyled & PartShadowStyled<0|1>;
export function input(key?: string, text?: string): Input & ShadowStyled & PartShadowStyled<0|1>;
export function textarea(key?: string, text?: string): Input;
export function number_input(key?: string): NumberInput;

export interface Composite extends Node {children(...items: (Node | Node[])[]): this}
export interface Disclosure extends Label, Composite, DisclosureTriggerStyled {open(value?: boolean): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Group extends Composite {select(item: Item): this; selected(): Item | undefined; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Item extends Disclosure, RadioTooltip, RadioPaintStyled {
  /** Direct Carousel Item viewport fraction in [0, 1]; zero resets. */
  carousel_fit(enabled?: boolean): this;
  carousel_basis(fraction: number): this;
  /** Enable or hide passive children of a direct RadioGroup Item. */
  decorate_content(enabled?: boolean): this;
}
export interface Alert extends Label {detail(text: string): this}
export function collapsible(key?: string, text?: string): Disclosure & DisclosureConfigured;
export function alert(key?: string, text?: string): Alert;
export function radio_group(key?: string): Group & RadioPaintStyled & RadioSelectionMotion & RadioTabOrder & PartShadowStyled<0>;
export function tabs(key?: string): Group;
export function accordion(key?: string): Composite & DisclosureConfigured & DisclosureDividerStyled;
export function item(key?: string, text?: string): Item;

export interface Select extends Group, Label {detail(text: string): this}
export interface ComboBox extends Select {current_text(): string}
export function select(key?: string, text?: string): Select;
export function combobox(key?: string, text?: string): ComboBox;

export interface DropdownButton extends Group, Label {on_click(callback: (event: ClickEvent<DropdownButton>) => void): this; part_bounds(part: 'action' | 'menu'): Bounds}
export function dropdown_button(key?: string, text?: string): DropdownButton;

/** Close result: 1 accepted, 2 cancelled, 3 closed. Callbacks run after the C frame. */
export interface CloseEvent { node: Node; frame: number; result: 1 | 2 | 3 }
export interface OpenState extends Node {open(value?: boolean): this; is_open(): boolean}
export interface Surface extends Label, OpenState {detail(text: string): this; on_close(callback: (event: CloseEvent) => void): this}
export interface Dialog extends Surface, Composite {}
export interface AlertDialog extends Surface {}
export interface Sheet extends Surface, Composite {}
export interface Popover extends Surface, Composite {anchor(node: Node): this}
export interface HoverSurface extends Label {on_change(callback: (event: ChangeEvent) => void): this; anchor(node: Node): this; is_open(): boolean; disabled(value?: boolean): this}
export interface HoverCard extends HoverSurface, Composite {}
export function dialog(key?: string, text?: string): Dialog;
export function alert_dialog(key?: string, text?: string): AlertDialog;
export function sheet(key?: string, text?: string): Sheet;
export function popover(key?: string, text?: string): Popover;
export function tooltip(key?: string, text?: string): HoverSurface;
export function hover_card(key?: string, text?: string): HoverCard;

export interface Visual extends Label {visual_size(width: number, height: number): this}
export interface Avatar extends Visual, DesignSized {identity_colors(enabled?: boolean, hue?: number | null): this; anonymous_icon(enabled?: boolean, name?: string): this}
export interface EmptyView extends Visual {detail(text: string): this}
export interface GroupBox extends Visual, Composite {}
export interface Message extends Visual {detail(text: string): this}
export function avatar(key?: string, text?: string): Avatar;
export function bubble(key?: string, text?: string): Label;
export function empty_view(key?: string, text?: string): EmptyView;
export function group_box(key?: string, text?: string): GroupBox;
export function kbd(key?: string, text?: string): Kbd;
export function skeleton(key?: string, text?: string): Visual;
export function spinner(key?: string, text?: string): Visual;
export function tag(key?: string, text?: string): Label;
export function marker(key?: string, text?: string): Label;
export function shimmer(key?: string, text?: string): Label;
export function icon(key?: string, name?: string): Visual;
export function message(key?: string, text?: string): Message;

export interface Item extends Label {detail(text: string): this; step_status(value: number): this; checked(value?: boolean): this; is_checked(): boolean}
export interface PagedNumericControl extends NumericControl {total(value: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this; numeric_part_bounds(part: number): Bounds}
export interface PaginationConfig {
  counter?: boolean; iconOnly?: boolean; chevrons?: boolean; jumpMenu?: boolean;
  visiblePages?: number; availableWidth?: number;
  group?: string; first?: string; previous?: string; next?: string; last?: string;
  earlierPages?: string; laterPages?: string;
}
export interface Pagination extends PagedNumericControl, DesignSized {
  /** Logical pixel height: zero restores the named design size; otherwise 1..512. */
  custom_size(value: number): this;
  get_custom_size(): number;
  configure(options: PaginationConfig | null): this;
  reset_config(): this;
  get_config(): Readonly<Required<PaginationConfig>> | undefined;
}
export interface RatingConfig {
  halfSteps?: boolean; hoverPreview?: boolean; stepDownFilled?: boolean; clearable?: boolean; readonly?: boolean;
  designSize?: DesignSize | null; width?: number;
  fillRgba?: number | null; outlineRgba?: number | null; previewRgba?: number | null;
  outlineIcon?: IconResource | null; fillIcon?: IconResource | null;
}
export interface Rating extends PagedNumericControl {
  configure(options: RatingConfig | null): this;
  reset_config(): this;
  get_config(): Readonly<Required<RatingConfig>> | undefined;
}
export function list_view(key?: string, text?: string): Group;
export function menu(key?: string, text?: string): Group;
export interface Stepper extends Group, DesignSized {
  stepper_fill(vertical?: boolean, density?: "md" | "xs" | "sm" | "lg", width?: number, height?: number): this;
  /** Wrap Title/description in equal horizontal lanes or beside vertical indicators; zero width grows, zero height is intrinsic. */
  stepper_wrapped_fill(vertical?: boolean, density?: "md" | "xs" | "sm" | "lg", width?: number, height?: number): this;
  stepper_visual(enabled?: boolean, vertical?: boolean, density?: "md" | "xs" | "sm" | "lg"): this;
  numeric_part_bounds(part: number): Bounds;
}
export function stepper(key?: string, text?: string): Stepper;
export function pagination(key?: string, text?: string): Pagination;
export function rating(key?: string, text?: string): Rating;
export function description_list(key?: string, text?: string): Composite;
export function status_bar(key?: string, text?: string): Composite;
export function title_bar(key?: string, text?: string): Composite;
export interface Toolbar extends Composite {disabled(value?: boolean): this}
export function toolbar(key?: string, text?: string): Toolbar;
export interface Scrollable extends Visual, Composite {scroll_extent(width: number, height: number): this; scroll_offset(): number; on_change(callback: (event: ChangeEvent) => void): this}
export function scrollable(key?: string, text?: string): Scrollable;
export interface App {scroll(x: number, y: number, dx: number, dy: number): this}

export type AttachmentStatus = 'pending' | 'uploading' | 'processing' | 'failed' | 'complete';
export interface Attachment extends Visual, NumericControl {detail(text: string): this; attachment_status(value: AttachmentStatus): this}
export interface Image extends Visual {image_rgba(pixels: Uint8Array, width: number, height: number, stride?: number): this}
export interface Tree extends Group {visual_size(width: number, height: number): this}
export interface TimeField extends Label {time_value(hour: number, minute: number, second?: number): this; current_time(): [number,number,number]; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface ColorPicker extends Visual {color_rgba(value: number): this; current_color(): number; color_part_bounds(part: 1 | 2 | 3): Bounds; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Calendar extends Label {date_value(year: number, month: number, day: number): this; current_date(): [number,number,number]; calendar_day_bounds(year: number, month: number, day: number): Bounds; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface DatePicker extends Label, OpenState {date_value(year: number, month: number, day: number): this; current_date(): [number,number,number]; visual_size(width: number, height: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface OtpInput extends Input {total(cells: number): this}
export function attachment(key?: string, text?: string): Attachment;
export function image(key?: string, text?: string): Image;
export function tree(key?: string, text?: string): Tree;
export function time_field(key?: string, text?: string): TimeField;
export function color_picker(key?: string, text?: string): ColorPicker;
export function calendar(key?: string, text?: string): Calendar;
export function date_picker(key?: string, text?: string): DatePicker;
export function otp_input(key?: string, text?: string): OtpInput;

export interface Disclosure {is_open(): boolean}

/** Item click callbacks are supported for InputGroup addons. */
export interface Item {on_click(callback: (event: ClickEvent<Item>) => void): this}
export interface Form extends Visual, Composite {disabled(value?: boolean): this}
export interface InputGroup extends Input, Composite {detail(text: string): this; visual_size(width: number, height: number): this}
export interface Notification extends Surface {on_click(callback: (event: ClickEvent<Notification>) => void): this}
export function form(key?: string, text?: string): Form;
export function input_group(key?: string, text?: string): InputGroup;
export function notification(key?: string, text?: string): Notification;

export interface FocusTrap extends Composite, OpenState {}
export function focus_trap(key?: string, text?: string): FocusTrap;

export interface App {font_fallbacks(paths: string[]): this}

export interface MessageScroller extends Label, Composite {visual_size(width: number,height: number): this; scroll_offset(): number; scroll_to(offset: number): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Carousel extends Group {carousel_adjacent(next?: boolean): Item | undefined; carousel_step(next?: boolean): Item | undefined; carousel_control_sizes(options?: {arrows?: DesignSize; pagination?: DesignSize}): this; carousel_controls(options?: {arrows?: boolean; pagination?: boolean}): this; numeric_part_bounds(part: number): Bounds; carousel_axis(axis: "horizontal" | "vertical"): this; carousel_looping(enabled?: boolean): this; visual_size(width: number,height: number): this}
export interface Resizable extends Label, Composite {visual_size(width: number,height: number): this; disabled(value?: boolean): this; resizable_handle_bounds(preceding: Item): Bounds}
export interface Item {panel_size(size: number): this; current_panel_size(): number}
export interface TextView extends Label {current_text(): string; text_format(format: 'markdown'|'html'|'plain'): this; select_text(anchor: number,cursor: number): this; selection_text(): string; visual_size(width: number,height: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Editor extends Input {detail(language: string): this; visual_size(width: number,height: number): this}
export function message_scroller(key?: string,text?: string): MessageScroller;
export function carousel(key?: string,text?: string): Carousel;
export function resizable(key?: string,text?: string): Resizable;
export function text_view(key?: string,text?: string): TextView;
export function editor(key?: string,text?: string): Editor;

export interface Chart extends Label {chart_data(data: Array<[string,number]>): this; chart_kind(kind: 'line'|'area'|'bar'|'pie'): this; current_datum(index: number): [string,number]; visual_size(width: number,height: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Table extends Label, Composite {visual_size(width: number,height: number): this}
export interface DataTableState {row: number; column: number; sortColumn: number; sortDirection: number}
export interface DataTablePagingOptions {
  /** Defaults to true when configuring. A new DataTable starts with paging off. */
  enabled?: boolean;
  /** Remaining rows beyond the visible window, an integer in 0..32. Defaults to 0. */
  threshold?: number;
}
export interface DataTableLoadMore {
  readonly frameSequence: bigint;
  readonly rowCount: number;
  /** The visible index range is half-open. */
  readonly visibleBegin: number;
  readonly visibleEnd: number;
  readonly lastRowKey: number;
  readonly capacity: number;
}
export interface DataTableCapacityOptions {
  /** Opt-in capacity in rows. Query the loaded provider before choosing a limit. */
  readonly rowCapacity: number;
}
export interface DataTable extends Label {
  /** Supply stable nonzero uint32 row keys to preserve the viewport on replacement. */
  table_data(headers: readonly string[], rows: readonly (readonly string[])[], rowKeys?: readonly number[]): this;
  table_data(headers: readonly string[], rows: readonly (readonly string[])[], rowKeys: readonly number[], options: DataTableCapacityOptions | undefined): this;
  configure_load_more(options?: DataTablePagingOptions): this;
  /** Consume one copied request from a mounted table after a successful frame. */
  poll_load_more(): DataTableLoadMore | undefined;
  selection_mode(mode: 'row' | 'column' | 'cell'): this;
  current_cell(row: number, column: number): string;
  table_state(): DataTableState;
  table_cell_bounds(row: number, column: number): Bounds;
  table_header_bounds(column: number): Bounds;
  visual_size(width: number, height: number): this;
  disabled(value?: boolean): this;
  on_change(callback: (event: ChangeEvent) => void): this;
}
export interface App {
  supports_keyed_data_table(): boolean;
  supports_data_table_paging(): boolean;
  /** Return the usable opt-in row limit, or undefined when the provider lacks it. */
  data_table_max_row_capacity(): number | undefined;
}
export interface Plot extends Label {plot_points(points: Array<[number,number]>): this; current_point(index: number): [number,number]; visual_size(width: number,height: number): this}
export function chart(key?: string,text?: string): Chart;
export function table(key?: string,text?: string): Table;
export function data_table(key?: string,text?: string): DataTable;
export function plot(key?: string,text?: string): Plot;
export interface Command extends Label, Composite {current_text(): string; selected(): Item | undefined; item_bounds(item: Item): Bounds; visual_size(width: number,height: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
/** Main-axis width or height in logical pixels; zero uses the default 32. */
export interface VirtualListRow {item: Item; extent: number}
export type VirtualListAxis = 'vertical' | 'horizontal';
export interface VirtualList extends Label, Composite {
  setVirtualListAxis(axis: VirtualListAxis): this;
  /** All current direct Items, each once; at most 64, extents zero or finite in [1,4096]. */
  setVirtualListRows(rows: readonly VirtualListRow[]): this;
  preserveVirtualListAnchor(enabled?: boolean): this;
  /** Mount one unattached leaf Item, then submit the complete projection. */
  appendVirtualListItem(item: Item): this;
  /** Remove a direct Item and retire its descendants. */
  removeVirtualListItem(item: Item): this;
  item_bounds(item: Item): Bounds;
  scroll_to(offset: number): this;
  scroll_offset(): number;
  visual_size(width: number, height: number): this;
  disabled(value?: boolean): this;
  on_change(callback: (event: ChangeEvent) => void): this;
}
export function command(key?: string,text?: string): Command;
export function virtual_list(key?: string,text?: string): VirtualList;
export interface DataTable {selection_mode(mode: 'row'|'column'|'cell'): this}
export interface Sidebar extends Group, OpenState {visual_size(width: number,height: number): this}
export function sidebar(key?: string,text?: string): Sidebar;
export interface App {theme_presets(): string[]; theme_preset(index: number): this; current_theme_preset(): number | undefined}
export interface RootView extends Composite {padding(pixels: number): this}
export function root_view(key?: string): RootView;
export interface App {clipboard_set(text: string): this; clipboard_get(): string}
export interface Speech {speech_start(): this; speech_finish(): this; speech_report(text: string,final?: boolean): this; speech_transcript(): string; speech_active(): boolean; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export function speech(key: string): Speech;
export interface Item {question(required?: boolean,multiple?: boolean): this; choice_selected(): boolean}
export interface Questionnaire extends Label, Composite {questionnaire_state(): {current: Item | undefined; complete: boolean}; questionnaire_next(): this; questionnaire_previous(): this; questionnaire_submit(): this; visual_size(width: number,height: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export function questionnaire(key: string,text?: string): Questionnaire;
export interface Dock extends Label, Composite {dock_activate(panel: Item): this; dock_move_split(panel: Item,reference: Item,axis?: 'horizontal'|'vertical',ratio?: number,before?: boolean): this; dock_active(): Item | undefined; dock_panel_bounds(panel: Item): Bounds; dock_serialize(): string; visual_size(width: number,height: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export function dock(key: string,text?: string): Dock;

export interface Settings extends Label, Composite {
  settings_query(query: string): this;
  settings_page(): Item | undefined;
  settings_select(page: Item): this;
  settings_reset(page: Item): this;
  settings_field_bounds(field: BooleanControl | Input | NumberInput): Bounds;
  visual_size(width: number, height: number): this;
  on_change(callback: (event: ChangeEvent) => void): this;
}
export function settings(key: string, text?: string): Settings;

export interface Input {detail(text: string): this}

/** Owning manual V1 views. No listeners, auto-dispatch or controlled requests. */
export interface NotificationHandleV1 {readonly slot: number; readonly generation: number}
export interface LegacySnapshotV1 {
  readonly type: number; readonly checkedRaw: number; readonly value: number;
  readonly text: string; readonly selectedHandle: NotificationHandleV1;
}
export interface NotificationBaseV1 {
  readonly schemaVersion: 1; readonly handle: NotificationHandleV1; readonly node?: Node;
  readonly frame: bigint; readonly hasFrame: boolean; readonly nodeKind: number;
  readonly legacy: LegacySnapshotV1;
}
export interface ActivationNotificationV1 extends NotificationBaseV1 {readonly kind: 'activation'}
export interface BooleanChangeNotificationV1 extends NotificationBaseV1 {readonly kind: 'boolean-change'; readonly value: boolean}
export interface TableChangeNotificationV1 extends NotificationBaseV1 {
  readonly kind: 'table-change'; readonly reason: 'selection' | 'sort';
  readonly row: number; readonly column: number; readonly sortColumn: number; readonly sortDirection: number;
}
export interface SurfaceCloseNotificationV1 extends NotificationBaseV1 {readonly kind: 'surface-close'; readonly result: 1 | 2 | 3}
export interface LegacyNotificationV1 extends NotificationBaseV1 {readonly kind: 'legacy'; readonly selectedNode?: Node}
export type NotificationV1 = ActivationNotificationV1 | BooleanChangeNotificationV1 |
  TableChangeNotificationV1 | SurfaceCloseNotificationV1 | LegacyNotificationV1;

/** Opt-in owning value views; same queue as legacy/V1 polling. */
export interface ScalarChangeNotificationV1 extends NotificationBaseV1 {readonly kind: 'scalar-change'; readonly value: number}
export interface TextChangeNotificationV1 extends NotificationBaseV1 {readonly kind: 'text-change'; readonly text: string}
export interface SelectionChangeNotificationV1 extends NotificationBaseV1 {
  readonly kind: 'selection-change'; readonly selectedHandle: NotificationHandleV1; readonly selectedNode?: Node;
}
export type ValueNotificationV1 = NotificationV1 | ScalarChangeNotificationV1 | TextChangeNotificationV1 | SelectionChangeNotificationV1;

/** RGBA icons apply only to Item children of Stepper; bytes are copied. */
export interface Item {image_rgba(pixels: Uint8Array, width: number, height: number, stride?: number): this}

/** Six root kinds only: row/column/label/button/input/badge. No nested parts or portals. */
export const ShadowKind: Readonly<{DROP:0;INSET:1}>;
export const ShadowPreset: Readonly<{NONE:0;XXS:1;XS:2;SM:3;MD:4;LG:5;XL:6;XXL:7}>;
export class CornerRadii {constructor(topLeft?:number,topRight?:number,bottomLeft?:number,bottomRight?:number);readonly values:readonly number[]}
export class BoxShadow {
 constructor(offsetX?:number,offsetY?:number,sigma?:number,spread?:number,kind?:0|1,rgba?:number);
 readonly values:readonly number[];
 static css(offsetX?:number,offsetY?:number,blurRadius?:number,spread?:number,rgba?:number,kind?:0|1):BoxShadow;
}
export class ShadowStyle {
 constructor(layers?:readonly BoxShadow[],radii?:CornerRadii);
 readonly layers:readonly BoxShadow[];readonly radii:CornerRadii;
 static preset(preset:0|1|2|3|4|5|6|7,radii?:CornerRadii):ShadowStyle;
}
export interface ShadowStyled {
 box_shadows(layers:readonly BoxShadow[]|ShadowStyle,radii?:CornerRadii):this;
 shadow_preset(preset:0|1|2|3|4|5|6|7,radii?:CornerRadii):this;
 reset_box_shadows():this;
}

export const ShadowPart:Readonly<{ROOT:0;MAIN:1;INDICATOR:2;TRACK:3;THUMB:4}>;
export type ShadowPart=0|1|2|3|4;
export interface ShadowTargetInfo {readonly canonicalPart:ShadowPart|null;readonly flags:number;readonly supported:boolean}
export interface PartShadowStyled<P extends ShadowPart> {
 part_shadows(part:P,layers:readonly BoxShadow[]|ShadowStyle,radii?:CornerRadii):this;
 part_shadow_preset(part:P,preset:0|1|2|3|4|5|6|7,radii?:CornerRadii):this;
 reset_part_shadows(part:P):this;
 shadow_target(part:ShadowPart):ShadowTargetInfo;
}
/** ITEM authoring helper requiring a RadioGroup owner at mount; ordinary ITEM remains generic. */
export function radio_item(key?:string,text?:string):Item&PartShadowStyled<0|1|2>;

/** Unreleased bridge-owned controlled Slider subset, separate from scalar slider(). */
export interface SliderSingle {readonly kind: 'single'; readonly end: number}
export interface SliderRange {readonly kind: 'range'; readonly start: number; readonly end: number}
export type SliderValue = SliderSingle | SliderRange;
export interface SliderDomain {readonly minimum: number; readonly maximum: number; readonly step: number; readonly scale: 'linear' | 'logarithmic'}
export interface SliderConfig {
  readonly domain: SliderDomain; readonly orientation: 'horizontal' | 'vertical';
  readonly layout: 'fixed' | 'parent'; readonly length: number; readonly rem: number;
  readonly sourceGeneration: bigint; readonly requests: boolean; readonly readonly: boolean;
  readonly reverseFill: boolean; readonly reducedMotion: boolean; readonly hideFocus: boolean;
}
export type SliderConfigInput = Partial<Omit<SliderConfig,'domain'>> & {domain?: Partial<SliderDomain>};
export interface SliderReceipt {
  readonly appEpoch: bigint; readonly node: Readonly<{slot:number;generation:number}>;
  readonly sourceGeneration: bigint; readonly configurationRevision: bigint;
  readonly submissionFrame: bigint; readonly requestSequence: bigint;
}
export interface SliderInput {
  readonly type: number; readonly flags: number; readonly key: number; readonly modifiers: number;
  readonly x: number; readonly y: number; readonly repeat: number; readonly pointerType: number;
  readonly pointerButton: number; readonly accessibilityAction: number; readonly pointerId: bigint;
  readonly pointerDevice: bigint; readonly accessibilityTarget: bigint; readonly accessibilityValue: number;
}
export interface SliderRequest {
  readonly phase: 0|1|2|3; readonly reason: number; readonly thumb: 0|1;
  readonly eventKind: 1|3|5; readonly eventSource: number; readonly eventFlags: number;
  readonly receipt: SliderReceipt; readonly eventTarget: bigint; readonly eventFrame: bigint;
  readonly eventIndex: bigint; readonly current: SliderValue; readonly requested: SliderValue;
  readonly domain: SliderDomain; readonly gestureDomain: SliderDomain; readonly input: SliderInput;
}
export interface SliderPresentation {readonly renderStatus:number;readonly coreTime:number;readonly nextWakeSeconds:number}
export class SliderModel implements Node {
  constructor(key:string,config:SliderConfigInput,confirmed:SliderValue,text?:string);
  key(value:string):this; bounds():Bounds; remove():this;
  label(text:string):this; disabled(value?:boolean):this;
  on_request(callback:((request:SliderRequest)=>void)|null):this;
  configure(config:SliderConfigInput,confirmed:SliderValue):this;
  set_value(confirmed:SliderValue):this; get_value():SliderValue; get_config():SliderConfig;
  /** Actual completed geometry; absent part returns undefined. */
  part_bounds(part:0|1|2|3|4):Readonly<Bounds>|undefined;
  presentation():SliderPresentation|undefined;
}
export function slider_single(end:number):SliderSingle;
export function slider_range(start:number,end:number):SliderRange;
export function slider_config(config?:SliderConfigInput):SliderConfig;
export function slider_model(key:string,config:SliderConfigInput,confirmed:SliderValue,text?:string):SliderModel;
export interface App {
  supports_slider_model():boolean;
  enable_slider_requests(capacity?:number):this;
  poll_slider_request():SliderRequest|undefined;
  /** Original returned receipt required; cloned numeric fields confer no authority. */
  accept_slider_request(receipt:SliderReceipt,value:SliderValue):void;
  dispatch_slider_requests():number;
}

/** Unreleased localized kind55 authoring; copied applied changes, never proposals.
 * Optional complete bridge group. Explicit locale tokens, no locale lookup/IME/AT promise. */
export type LocalizedTime = readonly [number, number, number];
export interface TimeFieldLocale {
  readonly digits: readonly string[];
  readonly separator: string; readonly periodSeparator: string;
  readonly am: string; readonly pm: string;
}
export interface LocalizedTimeConfigInput {
  locale?: TimeFieldLocale; cycle?: 'h23' | 'h12'; precision?: 'minute' | 'second';
  minimum?: LocalizedTime | null; maximum?: LocalizedTime | null; editGeneration?: bigint;
  disabled?: boolean; readonly?: boolean; ownerInvalid?: boolean; names?: readonly string[];
}
export interface LocalizedTimeConfig {
  readonly locale: TimeFieldLocale; readonly cycle: 'h23' | 'h12'; readonly precision: 'minute' | 'second';
  readonly minimum: LocalizedTime | null; readonly maximum: LocalizedTime | null;
  readonly editGeneration: bigint; readonly disabled: boolean; readonly readonly: boolean;
  readonly ownerInvalid: boolean; readonly names: readonly string[];
}
export interface LocalizedTimeChange {
  readonly handle: Readonly<{slot: number; generation: number}>;
  readonly frame: bigint; readonly node: LocalizedTimeField | undefined;
  readonly previous: LocalizedTime; readonly value: LocalizedTime;
  readonly validation: 0 | 3 | 4 | 5;
  readonly rejectedCount: number; readonly frameStatus: 0 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10;
}
export interface LocalizedTimeResult extends LocalizedTimeChange {
  readonly status: 0 | 1 | 2 | 3 | 4 | 5 | 6;
  readonly changed: boolean; readonly queuedChangeCount: 0 | 1;
}
export interface LocalizedTimeField extends Label {
  time_value(hour: number, minute: number, second?: number): this;
  current_time(): [number, number, number];
  configure(config: LocalizedTimeConfigInput): this;
  get_config(): LocalizedTimeConfig; latest_result(): LocalizedTimeResult;
  disabled(value?: boolean): this; readonly(value?: boolean): this;
  owner_invalid(value?: boolean): this; clear_validation(): this;
  on_localized_change(callback: ((change: LocalizedTimeChange) => void) | null): this;
}
export function localized_time_config(config?: LocalizedTimeConfigInput): LocalizedTimeConfig;
export function time_locale_english(): TimeFieldLocale;
export function time_locale_arabic(): TimeFieldLocale;
export function localized_time_field(key: string, config?: LocalizedTimeConfigInput, text?: string): LocalizedTimeField;
export interface App {
  supports_localized_time_field(): boolean;
  enable_localized_time_fields(capacity?: number): this;
  /** Copied applied record; explicitly drain after a failed step as well. */
  poll_localized_time_change(): LocalizedTimeChange | undefined;
  dispatch_localized_time_changes(): number;
}

export const LocalizedTimeField: {new(key: string, config?: LocalizedTimeConfigInput, text?: string): LocalizedTimeField};

export type SkeletonWidth = 'fixed' | 'fraction' | 'grow';
export interface SkeletonConfigInput {
  widthMode?: SkeletonWidth;
  width?: number;
  height?: number;
  maxWidth?: number;
  radius?: number;
  duration?: number;
  /** Numeric 0xRRGGBBAA. Null uses the active palette. */
  backgroundRgba?: number | null;
  secondary?: boolean;
  reducedMotion?: boolean;
}
export type SkeletonConfig = Readonly<Required<SkeletonConfigInput>>;
export interface SkeletonPresentation {
  readonly frameSequence: bigint;
  readonly coreTime: number;
  /** Absolute Core-clock deadline. Zero means static. */
  readonly nextWakeSeconds: number;
  readonly phase: number;
  readonly opacity: number;
}
export interface ResponsiveSkeleton extends Node {
  configure(config: SkeletonConfigInput): this;
  get_config(): SkeletonConfig;
  presentation(): SkeletonPresentation | undefined;
}
export const ResponsiveSkeleton: {new(key?: string, config?: SkeletonConfigInput): ResponsiveSkeleton};
export function responsive_skeleton(key?: string, config?: SkeletonConfigInput): ResponsiveSkeleton;
export interface App {
  supports_skeleton_presentation(): boolean;
  supports_spinner_icon_resource(): boolean;
  supports_spinner_presentation(): boolean;
  supports_pagination_custom_size(): boolean;
  supports_rating_options(): boolean;
  supports_pagination_options(): boolean;
}

/** Source values are copied into an app-owned Icon resource. */
export type IconSourceValue =
  | Readonly<{kind: 'empty'; text: ''}>
  | Readonly<{kind: 'name' | 'path'; text: string}>
  | Readonly<{kind: 'data'; text: string}>;
export const IconSource: Readonly<{
  empty(): IconSourceValue;
  named(name: string): IconSourceValue;
  path(path: string): IconSourceValue;
  svg(svg: string | Uint8Array): IconSourceValue;
}>;
export const IconError: Readonly<{OK: 0; INVALID: 1; NOT_FOUND: 2; PARSE: 3; LIMIT: 4; ALLOCATION: 5}>;
export type IconErrorValue = typeof IconError[keyof typeof IconError];
export const IconAvailability: Readonly<{UNKNOWN: 0; CATALOG_NOT_EMBEDDED: 1; EMBEDDED: 2}>;
export type IconAvailabilityValue = typeof IconAvailability[keyof typeof IconAvailability];
export type IconTransform = readonly [number, number, number, number, number, number];
export interface IconLimitsInput {
  /** Zero selects the C default for the corresponding limit. */
  maxSvgBytes?: number;
  maxPixels?: number;
  maxCacheBytes?: number;
  maxVersions?: number;
}
export type IconLimits = Readonly<Required<IconLimitsInput>>;
export interface IconResourceInfo {
  readonly error: IconErrorValue;
  readonly references: number;
  readonly parseCount: bigint;
  readonly rasterCount: bigint;
  readonly cacheBytes: bigint;
  readonly cacheVersions: bigint;
  readonly sourceRevision: bigint;
  readonly sourceKind: IconSourceValue['kind'];
}
export interface IconCatalogEntry {
  readonly name: string;
  readonly path: string;
  readonly defaultEmbedded: boolean;
}
export interface IconBitmap {
  readonly width: number;
  readonly height: number;
  readonly stride: number;
  readonly revision: bigint;
  readonly paint: Readonly<{x: number; y: number; width: number; height: number}>;
  /** Packed straight RGBA8 in an independent copy. */
  readonly rgba: Uint8Array;
}
export class IconResource {
  private readonly resourceBrand: never;
  constructor(app: App, source?: IconSourceValue, limits?: IconLimitsInput);
  set_source(source: IconSourceValue): this;
  source(): IconSourceValue;
  info(): IconResourceInfo;
  set_limits(limits: IconLimitsInput): this;
  limits(): IconLimits;
  put_asset(path: string, svg: string | Uint8Array): this;
  remove_asset(path: string): this;
  clear_assets(): this;
  reload(): this;
  collect(): this;
  prepare(width: number, height: number, transform?: IconTransform): IconErrorValue;
  bitmap(): IconBitmap;
  close(): void;
}
export interface App {
  supports_icon_resources(): boolean;
  icon_resource(source?: IconSourceValue, limits?: IconLimitsInput): IconResource;
  default_icon_limits(): IconLimits;
  icon_catalog_count(): number;
  icon_catalog_entry(index: number): IconCatalogEntry;
  query_icon(name: string): IconAvailabilityValue;
  copy_icon_asset(name: string): string | undefined;
}

export type IconSize = 'inherited' | 'xsmall' | 'small' | 'medium' | 'large' | 'custom';
export type IconAlign = 'start' | 'center' | 'end';
export type IconEdges = readonly [number, number, number, number];
export interface IconBoxStyleInput {
  width?: number;
  height?: number;
  padding?: number;
  gap?: number;
  radius?: number;
  backgroundRgba?: number;
  borderRgba?: number;
  borderWidth?: number;
  alignX?: IconAlign;
  alignY?: IconAlign;
  offsetX?: number;
  offsetY?: number;
  /** TL, TR, BL, BR. Null uses the common radius. */
  corners?: IconEdges | null;
  /** L, R, T, B. Null uses the common border width. */
  borderEdges?: IconEdges | null;
  maxWidth?: number;
  maxHeight?: number;
  opacity?: number | null;
  /** L, R, T, B. Null uses the common padding. */
  paddingEdges?: IconEdges | null;
  row?: boolean;
  clip?: boolean;
  fixedWidth?: boolean;
  fixedHeight?: boolean;
  layoutOnly?: boolean;
  inert?: boolean;
  widthPercent?: boolean;
  heightPercent?: boolean;
}
export interface IconConstraintsInput {
  minWidth?: number | null;
  minHeight?: number | null;
  maxWidth?: number | null;
  maxHeight?: number | null;
}
export interface IconOptionsInput {
  size?: IconSize;
  designSize?: DesignSize;
  customSize?: number;
  inheritedTextSize?: number;
  width?: number | null;
  height?: number | null;
  opacity?: number | null;
  remSize?: number;
  /** 0xRRGGBBAA. Null uses the inherited tint. */
  colorRgba?: number | null;
  inheritedColorRgba?: number | null;
  transform?: IconTransform | null;
  decorative?: boolean;
  style?: IconBoxStyleInput | null;
  constraints?: IconConstraintsInput | null;
  debugPlaceholder?: boolean;
}
export type IconOptions = Readonly<Omit<Required<IconOptionsInput>, 'style' | 'constraints'> & {
  style: Readonly<Required<IconBoxStyleInput>> | null;
  constraints: Readonly<Required<IconConstraintsInput>> | null;
}>;
export interface RetainedIconConfig {
  readonly resource: IconResource;
  readonly options: IconOptions;
  readonly name: string;
}
export interface RetainedIconResult {
  readonly frameSequence: bigint;
  readonly iconError: IconErrorValue;
  readonly frameStatus: number;
}
export interface ConfiguredIcon extends Node {
  configure(resource: IconResource, options?: IconOptionsInput, semantic?: {name?: string}): this;
  get_config(): RetainedIconConfig | undefined;
  result(): RetainedIconResult | undefined;
  reset(): this;
}
export const ConfiguredIcon: {
  new(key: string | undefined, resource: IconResource, options?: IconOptionsInput,
    semantic?: {name?: string; fallback?: string | null}): ConfiguredIcon;
};
export function configured_icon(key: string | undefined, resource: IconResource,
  options?: IconOptionsInput, semantic?: {name?: string; fallback?: string | null}): ConfiguredIcon;
export function icon_rotation(radians: number): IconTransform;

export type BadgeControlSize = 'medium' | 'small' | 'large' | 'xsmall';
type BadgeSharedInput = {
  count?: number | bigint;
  maximum?: number | bigint;
  size?: BadgeControlSize;
  designSize?: DesignSize;
  colorRgba?: number | null;
  name?: string;
};
export type BadgeConfigInput = BadgeSharedInput & (
  | {variant?: 'count' | 'dot'; namedIcon?: ''; artwork?: null; artworkOptions?: null}
  | {variant: 'icon'; namedIcon: string; artwork?: null; artworkOptions?: null}
  | {variant: 'icon'; namedIcon?: ''; artwork: IconResource; artworkOptions?: IconOptionsInput | null}
);
export type BadgeConfig = Readonly<Required<Omit<BadgeSharedInput, 'count' | 'maximum'>> & {
  count: bigint;
  maximum: bigint;
} & (
  | {variant: 'count' | 'dot'; namedIcon: ''; artwork: null; artworkOptions: null}
  | {variant: 'icon'; namedIcon: string; artwork: null; artworkOptions: null}
  | {variant: 'icon'; namedIcon: ''; artwork: IconResource; artworkOptions: IconOptions}
)>;
export interface ConfiguredBadgeResult {
  readonly frameSequence: bigint;
  readonly iconError: IconErrorValue | null;
  readonly frameStatus: number;
  readonly submitted: boolean;
}
export interface ConfiguredBadge extends Composite {
  configure(config: BadgeConfigInput): this;
  get_config(): BadgeConfig;
  result(): ConfiguredBadgeResult | undefined;
  indicator_bounds(): Readonly<Bounds> | undefined;
  disabled(value?: boolean): this;
}
export const ConfiguredBadge: {new(key?: string, config?: BadgeConfigInput): ConfiguredBadge};
export function configured_badge(key?: string, config?: BadgeConfigInput): ConfiguredBadge;
export interface App {
  supports_icon_presentation(): boolean;
  supports_configured_badge(): boolean;
}

export type DisclosureCursor = "default" | "pointer" | "text" | "resize_horizontal"
  | "resize_vertical" | "move" | "not_allowed" | "crosshair" | "grab" | "grabbing"
  | "zoom_in" | "zoom_out" | "wait" | "help";

export interface DisclosureConfig {
  /** Accordion only. */
  joined?: boolean;
  bordered?: boolean;
  single?: boolean;
  allowCollapse?: boolean;
  /** Collapsible only. */
  borderless?: boolean;
  designSize?: DesignSize | null;
  /** -1 fills, 0 fits, and a positive width sets logical pixels. */
  width?: number;
  customCard?: boolean;
  clipCard?: boolean;
  cardRadius?: number;
  cardBorderWidth?: number;
  cardPadding?: number;
  cardGap?: number;
  cardBackgroundRgba?: number;
  cardBorderRgba?: number;
  collapsedIcon?: string;
  expandedIcon?: string;
}

export interface DisclosureTriggerStyle {
  hoverRgba?: number | null;
  pressedRgba?: number | null;
  expandedRgba?: number | null;
  disabledRgba?: number | null;
  focusRgba?: number | null;
  cursor?: DisclosureCursor | null;
}

export interface DisclosureConfigured {
  disclosure_configure(config: DisclosureConfig | null): this;
}
export interface DisclosureTriggerStyled {
  set_trigger_style(style: DisclosureTriggerStyle | null): this;
}
export interface App {
  supports_disclosure_presentation(): boolean;
}

export type DisclosureTitleState = "hovered" | "focus-visible" | "pressed";
export interface DisclosureTitleStyle {
  typography?: DisclosureTypography | null; titleRgba?: number | null;
}
export type DisclosureHeadingLayer = "normal" | "expanded" | "disabled";
export interface DisclosureHeadingBox {
  width?: number; height?: number; padding?: number; gap?: number;
  radius?: number; borderWidth?: number; backgroundRgba?: number; borderRgba?: number;
}
export interface DisclosureTypography {
  /** Positive logical pixels; required when typography is supplied. */
  fontSize: number;
  lineHeight?: number;
  letterSpacing?: number;
  weight?: 0 | 1 | 2 | 3; align?: 0 | 1 | 2;
  wrap?: boolean; italic?: boolean; monospace?: boolean;
}
export interface DisclosureHeading {
  box?: DisclosureHeadingBox | null; typography?: DisclosureTypography | null; titleRgba?: number | null;
}
export interface DisclosureDivider { thickness?: number; insetLeft?: number; insetRight?: number; colorRgba?: number; }
export interface DisclosureConfigured {
  set_disclosure_title_state(state: DisclosureTitleState, style: DisclosureTitleStyle | null): this;
  set_disclosure_heading(layer: DisclosureHeadingLayer, style: DisclosureHeading | null): this;
}
export interface DisclosureTriggerStyled {
  set_disclosure_title_state(state: DisclosureTitleState, style: DisclosureTitleStyle | null): this;
  set_disclosure_heading(layer: DisclosureHeadingLayer, style: DisclosureHeading | null): this;
}
export interface App {
  supports_disclosure_title_state(): boolean;
  supports_disclosure_heading(): boolean;
  supports_disclosure_divider(): boolean;
}

export interface DisclosureDividerStyled { set_disclosure_divider(style: DisclosureDivider | null): this; }

/** Optional copied retained Attachment presentation. Width uses visual_size. */
export interface AttachmentTextStyle {
  fontSize?: number; weight?: 0 | 1 | 2 | 3; letterSpacing?: number;
  italic?: boolean; monospace?: boolean; colorRgba?: number;
}
export interface AttachmentOptions {
  vertical?: boolean; media?: boolean; indeterminate?: boolean; mediaAspect?: number;
  titleStatus?: AttachmentStatus; descriptionStatus?: AttachmentStatus;
  title?: AttachmentTextStyle; description?: AttachmentTextStyle;
}
export interface Attachment extends DesignSized {
  attachment_options(options: AttachmentOptions | null): this;
  attachment_preview_rgba(pixels: Uint8Array | Uint8ClampedArray, width: number, height: number, stride?: number): this;
  clear_attachment_preview(): this;
}
export interface App {
  supports_attachment_options(): boolean;
  supports_attachment_preview(): boolean;
}

export interface AttachmentActions {open?: boolean; remove?: boolean; retry?: boolean}
export interface Attachment {attachment_actions(options?: AttachmentActions): this}
export interface AttachmentActionNotificationV1 extends NotificationBaseV1 {
  readonly kind: 'attachment-action';
  readonly action: 'open' | 'remove' | 'retry';
  readonly part: 'unknown' | 'card' | 'remove' | 'media-retry' | 'description-retry';
  readonly source: 'pointer' | 'keyboard' | 'accessibility' | undefined;
  readonly eventIndex: number | undefined;
}
export type ComponentNotificationV1 = ValueNotificationV1 | AttachmentActionNotificationV1;

/** Structural containers created by the optional retained Attachment slot factory.
 * Actions require preferred_width in (0,4096]; other roles require zero.
 * Slots must be direct Attachment children. Descendant controls keep ordinary events. */
export type AttachmentSlotRole = 'media' | 'overlay' | 'metadata' | 'actions';
export interface AttachmentSlot extends Container { disabled(value?: boolean): this }
export function attachment_slot(key: string, role: 'actions', preferred_width: number): AttachmentSlot;
export function attachment_slot(key: string, role: Exclude<AttachmentSlotRole, 'actions'>, preferred_width?: 0): AttachmentSlot;
export interface Attachment { children(...items: (AttachmentSlot | AttachmentSlot[])[]): this }
export interface App { supports_attachment_slots(): boolean }

/** Passive control content; previous/next belong to Carousel, marker to a direct Carousel Item. */
export type CarouselSlotRole = 'previous' | 'next' | 'marker';
export interface CarouselSlot extends Container {}
export function carousel_slot(key: string, role: CarouselSlotRole, label?: string): CarouselSlot;
export interface App { supports_carousel_slots(): boolean }

export interface KbdStrokeOptions {
  /** SHIFT=1, CTRL=2, ALT=4, META=8. */
  modifiers?: number;
  plain?: boolean;
  outline?: boolean;
  macSymbols?: boolean;
  accessibleName?: string;
}
/** Retained typed keyboard stroke. Requires the optional Kbd V1 bridge. */
export function kbdStroke(key: string, strokeKey: string, options?: KbdStrokeOptions): Kbd;
export interface App { supportsKbdStroke(): boolean; }

export type SpinnerSize = 'custom' | 'xsmall' | 'small' | 'medium' | 'large';
export type SpinnerEasing = 'quadratic' | 'linear';
export interface SpinnerConfigInput {
  size?: SpinnerSize;
  pixels?: number;
  remSize?: number;
  duration?: number;
  rgba?: number | null;
  easing?: SpinnerEasing;
  /** Embedded catalog key, empty selects Loader. */
  icon?: string;
  reducedMotion?: boolean;
}
export type SpinnerConfig = Readonly<Required<SpinnerConfigInput>>;
export interface SpinnerPresentation {
  readonly frameSequence: bigint;
  readonly coreTime: number;
  readonly nextWakeSeconds: number;
  readonly phase: number;
}
export interface RotatingSpinner extends Node {
  /** Mounted-only shared SVG artwork; null restores configured catalog artwork. */
  set_icon_resource(resource: IconResource | null): this;
  configure(config: SpinnerConfigInput): this;
  get_config(): SpinnerConfig;
  presentation(): SpinnerPresentation | undefined;
}
export const RotatingSpinner: {new(key?: string, config?: SpinnerConfigInput): RotatingSpinner};
export function rotating_spinner(key?: string, config?: SpinnerConfigInput): RotatingSpinner;

/** Numeric RGBA colors (0xRRGGBBAA); logical pixel dimensions. */
export interface KbdGradient {
  stops: ReadonlyArray<{position: number; color: number}>;
  x0?: number; y0?: number; x1?: number; y1?: number;
}
/** Replacement style. Missing padding axis defaults to X=4/Y=2; border requires both fields. */
export type KbdStyle = {
  maxWidth?: number; fill?: boolean; wrap?: boolean;
  paddingX?: number; paddingY?: number; radius?: number; background?: number;
  fontSize?: number; lineHeight?: number;
  weight?: 'normal' | 'medium' | 'semibold' | 'bold'; italic?: boolean; monospace?: boolean;
} & ({borderWidth: number; borderColor: number} | {borderWidth?: never; borderColor?: never})
  & ({textColor?: number; textGradient?: never} | {textColor?: never; textGradient?: KbdGradient});
export interface Kbd extends Label { setKbdStyle(style?: KbdStyle | null): this; }
export interface App { supportsKbdStyle(): boolean; }
