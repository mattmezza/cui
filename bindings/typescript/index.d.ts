/** Native CUI bindings. Build bindings/node first; no npm dependencies. */
export type Backend = 'headless' | 'sdl';
export interface AppOptions { width?: number; height?: number; backend?: Backend; title?: string; fontPath?: string; maxNodes?: number; maxEvents?: number }
export interface Bounds { x: number; y: number; width: number; height: number }
export type Key = 'tab' | 'enter' | 'space' | 'escape' | 'left' | 'right' | 'up' | 'down';
export type KeyPhase = 'down' | 'up';
export type PointerPhase = 'move' | 'down' | 'up';
export interface ClickEvent { node: Button; frame: number }
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
  step(dt?: number): number;
  start(fps?: number): this;
  stop(): this;
  close(): this;
  dispose(): void;
}
