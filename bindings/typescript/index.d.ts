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
  /** Submit without dispatch. Caller owns draining the shared queue. */
  step_only(dt?: number): this;
  poll_notification_v1(): NotificationV1 | undefined;
  poll_value_notification_v1(): ValueNotificationV1 | undefined;

  constructor(options?: AppOptions);
  set_window_visible(visible: boolean): this;
  window_visible(): boolean;
  mount(root: Row | Column): this;
  pointer(phase: PointerPhase, x: number, y: number): this;
  key(phase: KeyPhase, key: Key, modifiers?: number, repeat?: boolean): this;
  text_input(text: string): this;
  step(dt?: number): number;
  /** Blocking native runner; dispatches handlers after paint. */
  run(): this;
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
export function radio(key?: string, text?: string): BooleanControl & PartShadowStyled<0|1|2>;
export function switch_control(key?: string, text?: string): BooleanControl & PartShadowStyled<0|1|3|4>;
export function toggle(key?: string, text?: string): BooleanControl;
export function slider(key?: string): Slider & PartShadowStyled<0|1|3>;
export function progress(key?: string): NumericControl;
export function badge(key?: string, text?: string): Label & ShadowStyled & PartShadowStyled<0|1>;
export function input(key?: string, text?: string): Input & ShadowStyled & PartShadowStyled<0|1>;
export function textarea(key?: string, text?: string): Input;
export function number_input(key?: string): NumberInput;

export interface Composite extends Node {children(...items: (Node | Node[])[]): this}
export interface Disclosure extends Label, Composite {open(value?: boolean): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Group extends Composite {select(item: Item): this; selected(): Item | undefined; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Item extends Disclosure {}
export interface Alert extends Label {detail(text: string): this}
export function collapsible(key?: string, text?: string): Disclosure;
export function alert(key?: string, text?: string): Alert;
export function radio_group(key?: string): Group & PartShadowStyled<0>;
export function tabs(key?: string): Group;
export function accordion(key?: string): Composite;
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
export function kbd(key?: string, text?: string): Label;
export function skeleton(key?: string, text?: string): Visual;
export function spinner(key?: string, text?: string): Visual;
export function tag(key?: string, text?: string): Label;
export function marker(key?: string, text?: string): Label;
export function shimmer(key?: string, text?: string): Label;
export function icon(key?: string, name?: string): Visual;
export function message(key?: string, text?: string): Message;

export interface Item extends Label {detail(text: string): this; step_status(value: number): this; checked(value?: boolean): this; is_checked(): boolean}
export interface Pagination extends NumericControl {total(value: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this; numeric_part_bounds(part: number): Bounds}
export interface Rating extends Pagination {}
export function list_view(key?: string, text?: string): Group;
export function menu(key?: string, text?: string): Group;
export interface Stepper extends Group, DesignSized {stepper_visual(enabled?: boolean, vertical?: boolean, density?: "md" | "xs" | "sm" | "lg"): this; numeric_part_bounds(part: number): Bounds}
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
export interface Carousel extends Group {visual_size(width: number,height: number): this}
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
export interface DataTable extends Label {table_data(headers: string[],rows: string[][]): this; current_cell(row: number,column: number): string; table_state(): DataTableState; table_cell_bounds(row: number,column: number): Bounds; table_header_bounds(column: number): Bounds; visual_size(width: number,height: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Plot extends Label {plot_points(points: Array<[number,number]>): this; current_point(index: number): [number,number]; visual_size(width: number,height: number): this}
export function chart(key?: string,text?: string): Chart;
export function table(key?: string,text?: string): Table;
export function data_table(key?: string,text?: string): DataTable;
export function plot(key?: string,text?: string): Plot;
export interface Command extends Label, Composite {current_text(): string; selected(): Item | undefined; item_bounds(item: Item): Bounds; visual_size(width: number,height: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface VirtualList extends Label, Composite {item_bounds(item: Item): Bounds; scroll_to(offset: number): this; scroll_offset(): number; visual_size(width: number,height: number): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
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
  readonly validation: 0 | 1 | 2 | 3 | 4 | 5 | 6;
  readonly rejectedCount: number; readonly frameStatus: number;
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
