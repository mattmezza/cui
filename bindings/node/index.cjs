// SPDX-License-Identifier: MIT
'use strict';
const path = require('node:path');
const native = require(process.env.CUI_NODE_ADDON || path.resolve(__dirname, '../../build-node/cui.node'));
const NODE = Symbol('CUI node');
let nextKey = 1;
const booleans = new Set([5,6,7,8]);
const numerics = new Set([9,10,14]);
const editable = new Set([12,13,14,22]);
const groups = new Set([17,18,19,21,22,23]);
const containers = new Set([1,2,15,17,18,19,20,21,22,23]);
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
    this.openValue = false; this.detailValue = ""; this.selectedValue = null; this.checkedValue = false; this.numericValue = 0; this.rangeValue = [0,1,.01]; this.changeValue = undefined;
    this.hoverValue = undefined; this.clickValue = undefined;
  }
  key(value) {
    if (this.owner) throw new Error('key cannot change after mount');
    if (typeof value !== 'string' || !value) throw new TypeError('key must be a nonempty string');
    this.keyValue = value; return this;
  }
  children(...items) {
    if (!containers.has(this.kind)) throw new Error('component does not accept children');
    if (this.owner) throw new Error('children cannot change after mount');
    for (const item of items.flat()) {
      if (!(item instanceof Node) || item.parent || item.owner) throw new TypeError('child must be an unattached CUI node');
      if(groups.has(this.kind) && item.kind!==20) throw new TypeError('groups require item children');
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
    if (this.kind < 3) throw new Error('label applies to label or button');
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
  disabled(value = true) { if (![4,5,6,7,8,9,12,13,14,15,17,18,19,20,21,22,23].includes(this.kind)) throw new Error('component does not support disabled'); if (typeof value !== 'boolean') throw new TypeError('disabled must be boolean'); this.disabledValue = !!value; if (this.owner) native.setDisabled(this.owner.handle, this[NODE], this.disabledValue ? 1 : 0); return this; }
  checked(value = true) {
    if (!booleans.has(this.kind) || typeof value !== 'boolean') throw new TypeError('checked requires a boolean component and value');
    if(this.owner) native.setChecked(this.owner.handle,this[NODE],value ? 1 : 0);
    this.checkedValue=value; return this;
  }
  is_checked() { if(!booleans.has(this.kind)) throw new Error('no checked state'); return this.owner ? native.getChecked(this.owner.handle,this[NODE]) : this.checkedValue; }
  value(value) {
    if(!numerics.has(this.kind) || !Number.isFinite(value) || value<this.rangeValue[0] || value>this.rangeValue[1]) throw new TypeError('invalid numeric value');
    if(this.owner) native.setValue(this.owner.handle,this[NODE],value); this.numericValue=value; return this;
  }
  current_value() { if(!numerics.has(this.kind)) throw new Error('no numeric state'); return this.owner ? native.getValue(this.owner.handle,this[NODE]) : this.numericValue; }
  range(minimum,maximum,step=1) {
    if(![9,14].includes(this.kind) || ![minimum,maximum,step].every(Number.isFinite) || minimum>=maximum || step<=0) throw new TypeError('invalid range');
    if(this.owner) native.setRange(this.owner.handle,this[NODE],minimum,maximum,step);
    this.rangeValue=[minimum,maximum,step]; this.numericValue=Math.max(minimum,Math.min(maximum,this.numericValue)); return this;
  }
  current_text() { if(!editable.has(this.kind)) throw new Error('no editable text'); return this.owner ? native.getText(this.owner.handle,this[NODE]) : this.textValue; }
  on_change(callback) { if(![5,6,7,8,9,12,13,14,15,17,18,19,20,21,22,23].includes(this.kind) || typeof callback!=='function') throw new TypeError('invalid change callback'); this.changeValue=callback; return this; }
  detail(text) { if(![16,21,22].includes(this.kind) || typeof text!=='string') throw new TypeError('detail requires alert text');if(this.owner)native.setDetail(this.owner.handle,this[NODE],text);this.detailValue=text;return this; }
  open(value=true) {if(![15,20].includes(this.kind)||typeof value!=='boolean')throw new TypeError('invalid open state');if(this.owner)native.setOpen(this.owner.handle,this[NODE],value?1:0);this.openValue=value;return this;}
  select(item) {if(![17,18,21,22,23].includes(this.kind)||!this.childrenValue.includes(item))throw new TypeError('selection must belong to group');if(this.owner)native.setSelected(this.owner.handle,this[NODE],item[NODE]);this.selectedValue=item;return this;}
  selected() {if(![17,18,21,22,23].includes(this.kind))throw new Error('component has no selection');return this.owner ? this.owner.nodes.get(keyFor(native.getSelected(this.owner.handle,this[NODE]))) : this.selectedValue;}
  hover_color(value) { this._button(); this.hoverValue = rgba(value); if (this.owner) native.setHoverRgba(this.owner.handle, this[NODE], ...this.hoverValue); return this; }
  on_click(callback) { if(![4,23].includes(this.kind))throw new Error("click requires button"); if (typeof callback !== 'function') throw new TypeError('callback must be a function'); this.clickValue = callback; return this; }
  part_bounds(part) {if(!this.owner || !['action','menu'].includes(part))throw new Error('invalid dropdown part');return native.dropdownBounds(this.owner.handle,this[NODE],part==='action'?0:1);}
  bounds() { if (!this.owner) throw new Error('node is not mounted'); this.owner._alive(); return native.bounds(this.owner.handle, this[NODE]); }
  remove() { if (!this.owner) throw new Error('node is not mounted'); this.owner._remove(this); return this; }
}
function row() { return new Node(1); }
function column() { return new Node(2); }
function label(value = '') { return new Node(3, value); }
function button(key) { const node = new Node(4, ''); return key === undefined ? node : node.key(key); }
function checkbox(key, text = "") { const node = new Node(5,text); return key === undefined ? node : node.key(key); }
function radio(key, text = "") { const node = new Node(6,text); return key === undefined ? node : node.key(key); }
function switch_control(key, text = "") { const node = new Node(7,text); return key === undefined ? node : node.key(key); }
function toggle(key, text = "") { const node = new Node(8,text); return key === undefined ? node : node.key(key); }
function slider(key, text = "") { const node = new Node(9,text); return key === undefined ? node : node.key(key); }
function progress(key, text = "") { const node = new Node(10,text); return key === undefined ? node : node.key(key); }
function badge(key, text = "") { const node = new Node(11,text); return key === undefined ? node : node.key(key); }
function input(key, text = "") { const node = new Node(12,text); return key === undefined ? node : node.key(key); }
function textarea(key, text = "") { const node = new Node(13,text); return key === undefined ? node : node.key(key); }
function number_input(key, text = "") { const node = new Node(14,text); return key === undefined ? node : node.key(key); }
function collapsible(key,text="") {const node=new Node(15,text);return key===undefined?node:node.key(key);}
function alert(key,text="") {const node=new Node(16,text);return key===undefined?node:node.key(key);}
function radio_group(key,text="") {const node=new Node(17,text);return key===undefined?node:node.key(key);}
function tabs(key,text="") {const node=new Node(18,text);return key===undefined?node:node.key(key);}
function accordion(key,text="") {const node=new Node(19,text);return key===undefined?node:node.key(key);}
function item(key,text="") {const node=new Node(20,text);return key===undefined?node:node.key(key);}
function select(key,text="") {const node=new Node(21,text);return key===undefined?node:node.key(key);}
function combobox(key,text="") {const node=new Node(22,text);return key===undefined?node:node.key(key);}

function dropdown_button(key,text="") {const node=new Node(23,text);return key===undefined?node:node.key(key);}

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
      if (booleans.has(node.kind)) native.setChecked(this.handle,node[NODE],node.checkedValue ? 1 : 0);
      if ([9,14].includes(node.kind)) native.setRange(this.handle,node[NODE],...node.rangeValue);
      if (numerics.has(node.kind)) native.setValue(this.handle,node[NODE],node.numericValue);
      if ([5,6,7,8,9,12,13,14,15,17,18,19,20,21,22,23].includes(node.kind)) native.setDisabled(this.handle,node[NODE],node.disabledValue ? 1 : 0);
      if(node.kind===15 || (node.kind===20 && parent && parent.kind===19))native.setOpen(this.handle,node[NODE],node.openValue?1:0);
      if([16,21,22].includes(node.kind))native.setDetail(this.handle,node[NODE],node.detailValue);
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
    try { attach(root, null); for(const node of this.nodes.values())if(node.selectedValue)native.setSelected(this.handle,node[NODE],node.selectedValue[NODE]); this.root = root; } catch (error) { this.dispose(); throw error; }
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
  text_input(text) { this._alive(); native.textInput(this.handle,text); return this; }
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
      if (event.type === 2 && node && node.changeValue) { node.changeValue({node,frame:event.frame,checked:event.checked,value:event.value,text:event.text,selectedHandle:{slot:event.selectedSlot,generation:event.selectedGeneration}}); delivered++; }
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
module.exports = {App,row,column,label,button,checkbox,radio,switch: switch_control,switch_control,toggle,slider,progress,badge,input,textarea,number_input,collapsible,alert,radio_group,tabs,accordion,item,select,combobox,dropdown_button};
