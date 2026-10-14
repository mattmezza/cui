// SPDX-License-Identifier: MIT
'use strict';
const path = require('node:path');
const native = require(process.env.CUI_NODE_ADDON || path.resolve(__dirname, '../../build-node/cui.node'));
const NODE = Symbol('CUI node');
let nextKey = 1;
function keyFor(node) { return `${node.slot}:${node.generation}`; }
function positive(value, name) {
  if (!Number.isFinite(value) || value <= 0) throw new TypeError(`${name} must be positive and finite`);
  return value;
}
function rgba(value) {
  if (typeof value === 'string') {
    const hex = value.startsWith('#') ? value.slice(1) : value;
    if (!/^(?:[0-9a-f]{6}|[0-9a-f]{8})$/i.test(hex)) throw new TypeError('color must be #RRGGBB or #RRGGBBAA');
    return [0, 2, 4, 6].map((at, index) => index === 3 && hex.length === 6 ? 255 : parseInt(hex.slice(at, at + 2), 16));
  }
  if (Array.isArray(value) && value.length === 4 && value.every(x => Number.isInteger(x) && x >= 0 && x <= 255)) return value;
  throw new TypeError('color must be #RRGGBB[AA] or [r,g,b,a]');
}
class Node {
  constructor(kind, text) {
    this.kind = kind; this.textValue = text; this.parent = null; this.owner = null;
    this[NODE] = null; this.keyValue = `node-${nextKey++}`; this.childrenValue = [];
    this.gapValue = undefined; this.variantValue = 0; this.sizeValue = 3;
    this.roundingValue = 0; this.radiusValue = 0; this.disabledValue = false;
    this.hoverValue = undefined; this.clickValue = undefined;
  }
  key(value) {
    if (this.owner) throw new Error('key cannot change after mount');
    if (typeof value !== 'string' || !value) throw new TypeError('key must be a nonempty string');
    this.keyValue = value; return this;
  }
  children(...items) {
    if (this.kind !== 1 && this.kind !== 2) throw new Error('only row and column accept children');
    if (this.owner) throw new Error('children cannot change after mount');
    for (const item of items.flat()) {
      if (!(item instanceof Node) || item.parent || item.owner) throw new TypeError('child must be an unattached CUI node');
      item.parent = this; this.childrenValue.push(item);
    }
    return this;
  }
  gap(value) {
    if (this.kind !== 1 && this.kind !== 2) throw new Error('gap applies to row or column');
    if (!Number.isFinite(value) || value < 0) throw new TypeError('gap must be nonnegative and finite');
    this.gapValue = value;
    if (this.owner) native.setGap(this.owner.handle, this[NODE], value);
    return this;
  }
  gap_px(value) { return this.gap(value); }
  gap_units(value) { return this.gap(value * 4); }
  label(value) {
    if (this.kind !== 3 && this.kind !== 4) throw new Error('label applies to label or button');
    if (typeof value !== 'string') throw new TypeError('label must be a string');
    this.textValue = value;
    if (this.owner) native.setText(this.owner.handle, this[NODE], value);
    return this;
  }
  _button() { if (this.kind !== 4) throw new Error('button style applies only to button'); }
  _variant(value) { this._button(); this.variantValue = value; if (this.owner) native.setVariant(this.owner.handle, this[NODE], value); return this; }
  primary() { return this._variant(0); }
  secondary() { return this._variant(1); }
  outline() { return this._variant(2); }
  ghost() { return this._variant(3); }
  _size(value) { this._button(); this.sizeValue = value; if (this.owner) native.setSize(this.owner.handle, this[NODE], value); return this; }
  inherit() { return this._size(0); }
  xs() { return this._size(1); }
  sm() { return this._size(2); }
  small() { return this.sm(); }
  base() { return this._size(3); }
  md() { return this._size(4); }
  medium() { return this.md(); }
  lg() { return this._size(5); }
  large() { return this.lg(); }
  xl() { return this._size(6); }
  size(value) { const names = {inherit:0,xs:1,sm:2,base:3,md:4,lg:5,xl:6,"2xl":7,"3xl":8,"4xl":9,"5xl":10,"6xl":11,"7xl":12,"8xl":13,"9xl":14}; if (!Object.hasOwn(names, value)) throw new TypeError("invalid button size"); return this._size(names[value]); }
  _round(value) { this._button(); this.roundingValue = value; if (this.owner) native.setRounding(this.owner.handle, this[NODE], value); return this; }
  rounded_theme() { return this._round(0); }
  rounded_none() { return this._round(1); }
  rounded_sm() { return this._round(2); }
  rounded_md() { return this._round(3); }
  rounded_px(value) {
    this._button(); positive(value, 'radius'); this.roundingValue = 4; this.radiusValue = value;
    if (this.owner) native.setCustomRounding(this.owner.handle, this[NODE], 4, value);
    return this;
  }
  disabled(value = true) { this._button(); this.disabledValue = !!value; if (this.owner) native.setDisabled(this.owner.handle, this[NODE], this.disabledValue ? 1 : 0); return this; }
  hover_color(value) { this._button(); this.hoverValue = rgba(value); if (this.owner) native.setHoverRgba(this.owner.handle, this[NODE], ...this.hoverValue); return this; }
  on_click(callback) { this._button(); if (typeof callback !== 'function') throw new TypeError('callback must be a function'); this.clickValue = callback; return this; }
  bounds() { if (!this.owner) throw new Error('node is not mounted'); this.owner._alive(); return native.bounds(this.owner.handle, this[NODE]); }
  remove() { if (!this.owner) throw new Error('node is not mounted'); this.owner._remove(this); return this; }
}
function row() { return new Node(1); }
function column() { return new Node(2); }
function label(value = '') { return new Node(3, value); }
function button(key) { const node = new Node(4, ''); return key === undefined ? node : node.key(key); }
class App {
  constructor(options = {}) {
    this.disposed = false; this.closed = false; this.nodes = new Map(); this.root = null; this.timer = null;
    this.handle = native.create({width: positive(options.width ?? 640, 'width'), height: positive(options.height ?? 480, 'height'),
      backend: options.backend === 'sdl' ? 1 : options.backend === undefined || options.backend === 'headless' ? 0 : NaN,
      maxNodes: options.maxNodes ?? 1024, maxEvents: options.maxEvents ?? 1024,
      title: options.title, fontPath: options.fontPath});
  }
  _alive() { if (this.disposed) throw new Error('app is disposed'); }
  mount(root) {
    this._alive();
    if (this.root) throw new Error('app already has a root');
    if (!(root instanceof Node) || root.parent || root.owner || (root.kind !== 1 && root.kind !== 2)) throw new TypeError('root must be an unattached row or column');
    const attach = (node, parent) => {
      node[NODE] = native.makeNode(this.handle, parent && parent[NODE], node.kind, node.keyValue);
      node.owner = this;
      const id = native.nodeId(node[NODE]); this.nodes.set(keyFor(id), node);
      if (node.textValue !== undefined) native.setText(this.handle, node[NODE], node.textValue);
      if (node.gapValue !== undefined) native.setGap(this.handle, node[NODE], node.gapValue);
      if (node.kind === 4) {
        native.setVariant(this.handle, node[NODE], node.variantValue);
        native.setSize(this.handle, node[NODE], node.sizeValue);
        if (node.roundingValue === 4) native.setCustomRounding(this.handle, node[NODE], 4, node.radiusValue);
        else native.setRounding(this.handle, node[NODE], node.roundingValue);
        if (node.disabledValue) native.setDisabled(this.handle, node[NODE], 1);
        if (node.hoverValue) native.setHoverRgba(this.handle, node[NODE], ...node.hoverValue);
      }
      for (const child of node.childrenValue) attach(child, node);
    };
    try { attach(root, null); this.root = root; } catch (error) { this.dispose(); throw error; }
    return this;
  }
  _remove(node) {
    this._alive();
    native.removeNode(this.handle, node[NODE]);
    const clear = current => { this.nodes.delete(keyFor(native.nodeId(current[NODE]))); current.owner = null; for (const child of current.childrenValue) clear(child); };
    clear(node);
    if (node === this.root) this.root = null;
    if (node.parent) node.parent.childrenValue = node.parent.childrenValue.filter(child => child !== node);
  }
  pointer(phase, x, y) {
    this._alive();
    const phases = {move: 0, down: 1, up: 2};
    if (!Object.hasOwn(phases, phase)) throw new TypeError('phase must be move, down, or up');
    native.pointer(this.handle, phases[phase], x, y); return this;
  }
  key(phase, key, modifiers = 0, repeat = false) {
    this._alive();
    const phases = {down: 0, up: 1};
    const keys = {tab: 1, enter: 2, space: 3, escape: 4, left: 5, right: 6, up: 7, down: 8};
    if (!Object.hasOwn(phases, phase) || !Object.hasOwn(keys, key)) throw new TypeError('invalid key input');
    native.key(this.handle, phases[phase], keys[key], modifiers, repeat ? 1 : 0);
    return this;
  }
  step(dt = 1 / 60) {
    this._alive(); native.step(this.handle, dt);
    let event; let delivered = 0;
    while ((event = native.poll(this.handle)) !== undefined) {
      const node = this.nodes.get(keyFor(event));
      if (event.type === 1 && node && node.clickValue) { node.clickValue({node, frame: event.frame}); delivered++; }
      if (this.disposed || this.closed) break;
    }
    return delivered;
  }
  start(fps = 60) {
    this._alive(); positive(fps, 'fps');
    if (this.timer) throw new Error('app is already running');
    const interval = 1000 / fps;
    this.timer = setInterval(() => { try { this.step(1 / fps); } catch (error) {
      this.stop();
      if (error.message === 'closed') { this.dispose(); return; }
      process.nextTick(() => { throw error; });
    } }, interval);
    return this;
  }
  stop() { if (this.timer) { clearInterval(this.timer); this.timer = null; } return this; }
  close() { this._alive(); this.stop(); native.close(this.handle); this.closed = true; return this; }
  dispose() { if (!this.disposed) { this.stop(); native.dispose(this.handle); this.disposed = true; this.nodes.clear(); } }
}
module.exports = {App, row, column, label, button};
