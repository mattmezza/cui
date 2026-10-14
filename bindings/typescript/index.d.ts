/** Native CUI bindings. Build bindings/node first; no npm dependencies. */
export type Backend = 'headless' | 'sdl';
export interface AppOptions { width?: number; height?: number; backend?: Backend; title?: string; fontPath?: string; maxNodes?: number; maxEvents?: number }
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
export interface Button extends Node {
  label(value: string): this; primary(): this; secondary(): this; outline(): this; ghost(): this;
  inherit(): this; xs(): this; sm(): this; small(): this; base(): this; md(): this; medium(): this; lg(): this; large(): this; xl(): this; size(value: "inherit" | "xs" | "sm" | "base" | "md" | "lg" | "xl" | "2xl" | "3xl" | "4xl" | "5xl" | "6xl" | "7xl" | "8xl" | "9xl"): this;
  rounded_theme(): this; rounded_none(): this; rounded_sm(): this; rounded_md(): this; rounded_px(radius: number): this;
  disabled(value?: boolean): this; hover_color(value: string | [number, number, number, number]): this;
  on_click(callback: (event: ClickEvent) => void): this;
}
export function row(): Row;
export function column(): Column;
export function label(text?: string): Label;
export function button(key?: string): Button;
export class App {
  constructor(options?: AppOptions);
  mount(root: Row | Column): this;
  pointer(phase: PointerPhase, x: number, y: number): this;
  key(phase: KeyPhase, key: Key, modifiers?: number, repeat?: boolean): this;
  text_input(text: string): this;
  step(dt?: number): number;
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
export function checkbox(key?: string, text?: string): BooleanControl;
export function radio(key?: string, text?: string): BooleanControl;
export function switch_control(key?: string, text?: string): BooleanControl;
export function toggle(key?: string, text?: string): BooleanControl;
export function slider(key?: string): Slider;
export function progress(key?: string): NumericControl;
export function badge(key?: string, text?: string): Label;
export function input(key?: string, text?: string): Input;
export function textarea(key?: string, text?: string): Input;
export function number_input(key?: string): NumberInput;

export interface Composite extends Node {children(...items: (Node | Node[])[]): this}
export interface Disclosure extends Label, Composite {open(value?: boolean): this; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Group extends Composite {select(item: Item): this; selected(): Item | undefined; disabled(value?: boolean): this; on_change(callback: (event: ChangeEvent) => void): this}
export interface Item extends Disclosure {}
export interface Alert extends Label {detail(text: string): this}
export function collapsible(key?: string, text?: string): Disclosure;
export function alert(key?: string, text?: string): Alert;
export function radio_group(key?: string): Group;
export function tabs(key?: string): Group;
export function accordion(key?: string): Composite;
export function item(key?: string, text?: string): Item;

export interface Select extends Group, Label {detail(text: string): this}
export interface ComboBox extends Select {current_text(): string}
export function select(key?: string, text?: string): Select;
export function combobox(key?: string, text?: string): ComboBox;

export interface DropdownButton extends Group, Label {on_click(callback: (event: ClickEvent<DropdownButton>) => void): this; part_bounds(part: 'action' | 'menu'): Bounds}
export function dropdown_button(key?: string, text?: string): DropdownButton;
