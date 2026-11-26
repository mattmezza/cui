// SPDX-License-Identifier: MIT
'use strict';
const path = require('node:path');
const fs = require('node:fs');
const packaged = path.join(__dirname, 'native', 'cui.node');
const sourceArtifact = fs.existsSync(path.join(__dirname, '_source'));
const addonPath = process.env.CUI_NODE_ADDON || (fs.existsSync(packaged) || sourceArtifact
  ? packaged : path.resolve(__dirname, '../../build-node/cui.node'));
const native = require(addonPath);
const {BoxShadow,ShadowStyle,ShadowKind,ShadowPreset,CornerRadii,ShadowPart,canonicalPart}=require('./shadows.cjs')(native);
const designSizes = Object.freeze({inherit:0,xs:1,sm:2,base:3,md:4,lg:5,xl:6,"2xl":7,"3xl":8,"4xl":9,"5xl":10,"6xl":11,"7xl":12,"8xl":13,"9xl":14});
const NODE = Symbol('CUI node');
const ATTACHMENT_SLOT = Symbol('Attachment slot');
const attachmentStatuses = Object.freeze({pending:0, uploading:1, processing:2, failed:3, complete:4});
let nextKey = 1;
const disabledKinds = new Set([4,5,6,7,8,9,12,13,14,15,17,18,19,20,21,22,23,28,29,40,41,42,43,44,48,54,55,56,57,58,59,60,61,65,66,67,68,69,71,73,74,75,77,78,79]);
const surfaces = new Set([24,25,26,27,62]);
const detailKinds = new Set([12,14,16,20,21,22,23,24,25,26,27,32,51,52,61,62,68]);
const visualKinds = new Set([30,32,33,35,36,46,50,51,52,53,54,56,58,60,61,64,65,66,67,68,69,70,71,72,73,74,75,78,79,80]);
const selectedKinds = new Set([17,18,21,22,23,40,41,44,54,65,75]);
const booleans = new Set([5,6,7,8,20]);
const numerics = new Set([9,10,14,42,43,52]);
const editable = new Set([12,13,14,22,59,61,67,68,73]);
const groups = new Set([17,18,19,21,22,23,40,41,44,45,47,48,49,54,60,61,64,65,66,70,73,74,75,78,79,80]);
const containers = new Set([1,2,15,17,18,19,20,21,22,23,24,26,27,29,33,40,41,44,45,46,47,48,49,54,60,61,63,64,65,66,70,73,74,75,76,78,79,80]);
const RadioPaintLayer = Object.freeze({
  BASE: 0, CHECKED: 1, HOVERED: 2, PRESSED: 3, FOCUSED: 4, DISABLED: 5,
});
const radioPaintUnavailable = 'Radio paint is unavailable in this bridge';
const nativeLittleEndian = require('node:os').endianness() === 'LE';
const radioRgba = 0xffffffff;
// Each entry contains the byte offset, minimum, maximum, and optional integer flag.
// Entry order matches the C field mask.
const radioPaintFields = {
  rootBackground: [16, 0, radioRgba, true],
  rootBorder: [20, 0, radioRgba, true],
  rootBorderWidth: [44, 0, 64],
  rootRadius: [48, 0, 256],
  rootPadding: [52, 0, 256],
  rootGap: [56, 0, 256],
  indicatorBackground: [24, 0, radioRgba, true],
  indicatorBorder: [28, 0, radioRgba, true],
  indicatorBorderWidth: [60, 0, 64],
  indicatorRadius: [64, 0, 256],
  indicatorExtent: [68, 4, 128],
  markColor: [32, 0, radioRgba, true],
  labelColor: [36, 0, radioRgba, true],
  descriptionColor: [40, 0, radioRgba, true],
  labelSize: [72, 1, 128],
  descriptionSize: [76, 1, 128],
  labelWeight: [80, 0, 3, true],
  descriptionWeight: [84, 0, 3, true],
};
const radioPaintNames = Object.keys(radioPaintFields);
const radioBasePaint = new Set([
  'rootRadius', 'rootPadding', 'rootGap', 'indicatorRadius', 'indicatorExtent',
  'labelSize', 'descriptionSize', 'labelWeight', 'descriptionWeight',
]);
function radioPaint(node, layer, paint) {
  if (![6, 17, 20].includes(node.kind))
    throw new TypeError('radio_paint requires Radio, RadioGroup or a direct RadioGroup Item');
  if (!Number.isInteger(layer) || layer < RadioPaintLayer.BASE || layer > RadioPaintLayer.DISABLED)
    throw new RangeError('invalid Radio paint layer');
  if (node.kind === 17 && layer !== RadioPaintLayer.BASE && layer !== RadioPaintLayer.DISABLED)
    throw new RangeError('RadioGroup frame accepts BASE and DISABLED paint only');
  if (paint === null) return null;
  if (!paint || typeof paint !== 'object' || Array.isArray(paint))
    throw new TypeError('expected RadioPaint or null');
  const bytes = Buffer.alloc(88);
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  view.setUint32(0, bytes.length, nativeLittleEndian);
  view.setUint32(4, 1, nativeLittleEndian);
  let mask = 0;
  for (const [name, value] of Object.entries(paint)) {
    const bit = radioPaintNames.indexOf(name);
    if (bit < 0) throw new TypeError(`unknown Radio paint field ${name}`);
    if ((node.kind === 17 && !name.startsWith('root')) ||
        (node.kind !== 20 && name.startsWith('description')) ||
        (layer !== RadioPaintLayer.BASE && radioBasePaint.has(name)))
      throw new TypeError('Radio paint field is unavailable for this component or layer');
    const [offset, min, max, integer] = radioPaintFields[name];
    if (!Number.isFinite(value) || value < min || value > max || (integer && !Number.isInteger(value)))
      throw new RangeError(`invalid Radio paint ${name}`);
    if (integer) view.setUint32(offset, value, nativeLittleEndian);
    else view.setFloat32(offset, value, nativeLittleEndian);
    mask |= 1 << bit;
  }
  view.setUint32(8, mask, nativeLittleEndian);
  return mask ? bytes : null;
}
const radioMotionUnavailable = 'Radio motion is unavailable in this bridge';
const radioContentUnavailable = 'Radio decoration is unavailable in this bridge';
const radioTooltipUnavailable = 'Radio tooltips are unavailable in this bridge';
const radioTabIndexUnavailable = 'Radio tab order is unavailable in this bridge';
// Row, Column, Label, plain Badge, Avatar, Kbd, Tag, Icon and Image.
const radioContentKinds = new Set([1, 2, 3, 11, 30, 34, 37, 50, 53]);
function requestsRadioMotion(node) {
  return node.radioMotionValue || node.childrenValue.some(requestsRadioMotion);
}
const stepperFillUnavailable = 'Stepper Fill requires cui_bridge_set_stepper_fill';
function preflightNodeOptions(app, node, insideDecoration = false, insideAttachmentSlot = false) {
  if (node.carouselAxisValue === 'vertical' && !app.supports_carousel_axis()) throw new Error('Carousel axis unavailable');
  if (node.carouselControlsValue && !app.supports_carousel_controls()) throw new Error('Carousel controls unavailable');
  if (node.carouselLoopingValue && !app.supports_carousel_looping()) throw new Error('Carousel looping unavailable');
  if (node.carouselBasisValue) {
    if (node.parent?.kind !== 65) throw new TypeError('carousel_basis requires a direct Carousel Item');
    if (!app.supports_carousel_basis()) throw new Error('Carousel basis unavailable');
  }
  if (node.stepperFillValue && !app.supports_stepper_fill()) throw new Error(stepperFillUnavailable);
  if (insideDecoration && !radioContentKinds.has(node.kind))
    throw new TypeError('RadioGroup Item decoration requires passive children');
  if (node.radioPaintValues?.size) {
    if (node.kind === 20 && node.parent?.kind !== 17) throw new TypeError('radio_paint requires a direct RadioGroup Item');
    if (!app.supports_radio_paint()) throw new Error(radioPaintUnavailable);
  }
  if (node.radioContentValue) {
    if (node.parent?.kind !== 17) throw new TypeError('decorate_content requires a direct RadioGroup Item');
    if (!app.supports_radio_item_content()) throw new Error(radioContentUnavailable);
  }
  if (node.radioTooltipValue) {
    if (node.kind === 20 && node.parent?.kind !== 17)
      throw new TypeError('radio_tooltip requires a direct RadioGroup Item');
    if (!app.supports_radio_tooltip()) throw new Error(radioTooltipUnavailable);
  }
  if (node.radioTabIndexValue !== null && !app.supports_radio_tab_index())
    throw new Error(radioTabIndexUnavailable);
  const slot = node[ATTACHMENT_SLOT];
  if (slot) {
    if (node.parent?.kind !== 52) throw new TypeError('Attachment slot requires direct Attachment parent');
    if (!app.supports_attachment_slots()) throw new Error('Attachment slots unavailable');
  }
  if (insideAttachmentSlot && node.kind === 62)
    throw new TypeError('Notifications use the app-wide stack, outside Attachment slots');
  for (const child of node.childrenValue)
    preflightNodeOptions(app, child, insideDecoration || node.radioContentValue,
      insideAttachmentSlot || Boolean(slot));
}
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
function validateDate(year,month,day) {
  if(![year,month,day].every(Number.isInteger) || year<1 || year>9999 || month<1 || month>12 || day<1)
    throw new TypeError('invalid date');
  const leap=year%4===0 && (year%100!==0 || year%400===0);
  if(day>[31,leap?29:28,31,30,31,30,31,31,30,31,30,31][month-1])throw new TypeError('invalid date');
}
const legacyTableRows = 32;
const maxTableRows = 10_000;
function requireDataTableCapacity(app, rowCapacity) {
  const maximum = app.data_table_max_row_capacity();
  if (maximum === undefined) throw new Error('DataTable capacity is unavailable in this bridge');
  if (rowCapacity > maximum) throw new RangeError(`rowCapacity exceeds the provider's ${maximum}-row limit`);
}
function preflightDataTables(app, node) {
  const rowCapacity = node.matrixValue?.[3];
  if (rowCapacity !== undefined) requireDataTableCapacity(app, rowCapacity);
  else if (node.matrixValue?.[2] !== undefined && !app.supports_keyed_data_table())
    throw new Error('keyed DataTable replacement is unavailable in this bridge');
  if (node.loadMoreThreshold !== undefined) {
    if (!app.supports_data_table_paging())
      throw new Error('DataTable paging is unavailable in this bridge');
    if (!node.matrixValue) throw new Error('DataTable paging requires column headers');
  }
  for (const child of node.childrenValue) preflightDataTables(app, child);
}
function applyTableData(node, [headers, rows, rowKeys, rowCapacity]) {
  const app = node.owner;
  if (rowCapacity !== undefined) {
    native.setMatrixCapacity(app.handle, node[NODE], headers, rows, rowKeys, rowCapacity);
    return;
  }
  if (rowKeys === undefined) {
    native.setMatrix(app.handle, node[NODE], headers, rows);
    return;
  }
  if (!app.supports_keyed_data_table())
    throw new Error('keyed DataTable replacement is unavailable in this bridge');
  native.setMatrixKeyed(app.handle, node[NODE], headers, rows, rowKeys);
}
class Node {
  constructor(kind, text) {
    this.kind = kind; this.textValue = text; this.parent = null; this.owner = null;
    this[NODE] = null; this.keyValue = `node-${nextKey++}`; this.childrenValue = [];
    this.questionFlags=null;this.rootPaddingValue=0;this.gapValue = undefined; this.variantValue = 0; this.sizeValue = 3;
    this.roundingValue = 0; this.radiusValue = 0; this.disabledValue = false;
    this.extentValue=null; this.anchorValue=null; this.visualValue=null; this.closeValue=null; this.totalValue=kind===43?5:kind===59?6:10; this.attachmentValue=0; this.imageValue=null;this.chartDataValue=[];this.chartKindValue=0;this.plotPointsValue=[];this.matrixValue=null;this.selectionModeValue=0;this.textFormatValue=0;this.panelValue=null; this.timeValue=[0,0,0];this.dateValue=[2026,1,1];this.colorValue=0x000000ff; this.statusValue=null; this.openValue = false;this.openSet=false;this.checkedSet=false; this.detailValue = ""; this.selectedValue = null; this.checkedValue = false; this.numericValue = kind===42?1:0; this.rangeValue = kind===42?[1,10,1]:kind===43?[0,5,1]:[0,1,.01]; this.changeValue = undefined;
    this.radioPaintValues = null;
    this.carouselAxisValue = 'horizontal';
    this.carouselControlsValue = 0;
    this.carouselLoopingValue = false;
    this.carouselBasisValue = 0;
    this.radioMotionValue = false;
    this.radioTabIndexValue = null;
    this.radioContentValue = false;
    this.radioTooltipValue = null;
    this.partShadowStyles=new Map(); this.shadowStyleValue = null; this.hoverValue = undefined; this.clickValue = undefined;
  }
  box_shadows(layers=[],radii=new CornerRadii()) {
    if(![1,2,3,4,11,12].includes(this.kind))throw new Error('unsupported shadow root');
    const style=layers instanceof ShadowStyle?layers:new ShadowStyle(layers,radii);
    if(this.owner)native.setShadows(this.owner.handle,this[NODE],style.wire());
    this.partShadowStyles.delete(0);this.shadowStyleValue=style;return this;
  }
  shadow_preset(preset,radii=new CornerRadii()){return this.box_shadows(ShadowStyle.preset(preset,radii));}
  reset_box_shadows(){if(![1,2,3,4,11,12].includes(this.kind))throw new Error('unsupported shadow root');if(this.owner)native.resetShadows(this.owner.handle,this[NODE]);this.partShadowStyles.delete(0);this.shadowStyleValue=null;return this;}
  part_shadows(part,layers=[],radii=new CornerRadii()) {
    const role=canonicalPart(this.kind,part),style=layers instanceof ShadowStyle?layers:new ShadowStyle(layers,radii);
    if(this.owner)native.setPartShadows(this.owner.handle,this[NODE],part,style.wire());
    this.partShadowStyles.set(role,style);if(role===0)this.shadowStyleValue=null;return this;
  }
  part_shadow_preset(part,preset,radii=new CornerRadii()){return this.part_shadows(part,ShadowStyle.preset(preset,radii));}
  reset_part_shadows(part){const role=canonicalPart(this.kind,part);if(this.owner)native.resetPartShadows(this.owner.handle,this[NODE],part);this.partShadowStyles.delete(role);if(role===0)this.shadowStyleValue=null;return this;}
  shadow_target(part){if(!this.owner)throw new Error('capability query requires a mounted node');const info=native.shadowTarget(this.owner.handle,this[NODE],part);return Object.freeze({canonicalPart:info.canonicalPart===0xffffffff?null:info.canonicalPart,flags:info.flags,supported:!!(info.flags&1)});}
  key(value) {
    if (this.owner) throw new Error('key cannot change after mount');
    if (typeof value !== 'string' || !value) throw new TypeError('key must be a nonempty string');
    this.keyValue = value; return this;
  }
  children(...items) {
    const isAttachment = this.kind === 52;
    if (!containers.has(this.kind) && !isAttachment) throw new Error('component does not accept children');
    if (this.owner) throw new Error('children cannot change after mount');
    const children = items.flat();
    if (isAttachment) {
      const roles = new Set(this.childrenValue.map(child => child[ATTACHMENT_SLOT].role));
      for (const child of children) {
        if (!(child instanceof Node) || child.parent || child.owner || !child[ATTACHMENT_SLOT])
          throw new TypeError('Attachment children require unattached attachment_slot nodes');
        const role = child[ATTACHMENT_SLOT].role;
        if (roles.has(role)) throw new TypeError('duplicate Attachment slot role');
        roles.add(role);
      }
    }
    for (const item of children) {
      if (!(item instanceof Node) || item.parent || item.owner) throw new TypeError('child must be an unattached CUI node');
      if (item[ATTACHMENT_SLOT] && !isAttachment) throw new TypeError('Attachment slot requires direct Attachment parent');
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
    if (this.owner) native.setText(this.owner.handle, this[NODE], value);
    this.textValue = value;
    return this;
  }
  _button() { if (this.kind !== 4) throw new Error('button style applies only to button'); }
  _variant(value) { this._button(); this.variantValue = value; if (this.owner) native.setVariant(this.owner.handle, this[NODE], value); return this; }
  primary() { return this._variant(0); }
  secondary() { return this._variant(1); }
  outline() { return this._variant(2); }
  ghost() { return this._variant(3); }
  design_size(value) {
    if (this.kind === 4) return this.size(value);
    const names = designSizes;
    if (![30,42,44,52].includes(this.kind) || typeof value !== 'string' || !Object.hasOwn(names,value)) throw new TypeError('design_size requires Button, Avatar, Pagination, Stepper or Attachment and a supported size');
    if (this.owner && this.kind === 42 && !paginationOptions.supports(this.owner))
      throw new Error('Pagination V1 requires a complete, compatible bridge symbol group');
    if (this.owner) native.setDesignSize(this.owner.handle,this[NODE],names[value]);
    this.designSizeValue=names[value]; return this;
  }
  identity_colors(enabled=true,hue=null) {
    if (this.kind!==30 || typeof enabled!=='boolean' || (hue!==null && (!Number.isInteger(hue)||hue<0||hue>11))) throw new TypeError('identity_colors requires Avatar, bool, and optional hue 0..11');
    const value=[enabled?1:0,hue===null?0:1,hue??0];
    if(this.owner) native.setAvatarIdentity(this.owner.handle,this[NODE],...value);
    this.avatarIdentityValue=value; return this;
  }
  anonymous_icon(enabled=true,name='') {
    if(this.kind!==30 || typeof enabled!=='boolean' || typeof name!=='string' || name.includes('\0') || Buffer.byteLength(name,'utf8')>4096 || /[\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF]/u.test(name)) throw new TypeError('anonymous_icon requires Avatar, bool, and valid UTF-8 name up to 4096 bytes');
    if(this.owner) native.setAvatarPlaceholder(this.owner.handle,this[NODE],enabled?1:0,name);
    this.avatarPlaceholderValue=[enabled?1:0,name]; return this;
  }
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
  size(value) { const names = designSizes; if (!Object.hasOwn(names, value)) throw new TypeError("invalid button size"); return this._size(names[value]); }
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
  disabled(value = true) { if (!disabledKinds.has(this.kind) && !this[ATTACHMENT_SLOT]) throw new Error('component does not support disabled'); if (typeof value !== 'boolean') throw new TypeError('disabled must be boolean'); if (this.owner) native.setDisabled(this.owner.handle, this[NODE], value ? 1 : 0); this.disabledValue = value; return this; }
  checked(value = true) {
    if (!booleans.has(this.kind) || typeof value !== 'boolean') throw new TypeError('checked requires a boolean component and value');
    if(this.owner) native.setChecked(this.owner.handle,this[NODE],value ? 1 : 0);
    this.checkedValue=value;this.checkedSet=true; return this;
  }
  radio_paint(layer, paint) {
    const config = radioPaint(this, layer, paint);
    if (this.owner) {
      if (this.owner.supports_radio_paint()) native.setRadioPaint(this.owner.handle, this[NODE], layer, config);
      else if (config) throw new Error(radioPaintUnavailable);
    }
    if (config) (this.radioPaintValues ??= new Map()).set(layer, config);
    else this.radioPaintValues?.delete(layer);
    return this;
  }
  animate_selection(enabled = true) {
    if (this.kind !== 6 && this.kind !== 17) throw new TypeError('selection motion requires a Radio or RadioGroup');
    if (typeof enabled !== 'boolean') throw new TypeError('selection motion requires a boolean');
    if (this.owner) {
      if (this.owner.supports_radio_motion()) native.setRadioMotion(this.owner.handle, this[NODE], enabled);
      else if (enabled) throw new Error(radioMotionUnavailable);
    }
    this.radioMotionValue = enabled;
    return this;
  }
  tab_index(index) {
    if (this.kind !== 6 && this.kind !== 17) throw new TypeError('tab_index requires Radio or RadioGroup');
    if (index !== null && typeof index !== 'number') throw new TypeError('expected a tab index or null');
    if (index !== null && (!Number.isInteger(index) || index < -0x80000000 || index > 0x7fffffff))
      throw new RangeError('tab index must fit a signed 32-bit integer');
    if (this.owner) {
      if (this.owner.supports_radio_tab_index())
        native.setRadioTabIndex(this.owner.handle, this[NODE], index !== null, index ?? 0);
      else if (index !== null) throw new Error(radioTabIndexUnavailable);
    }
    this.radioTabIndexValue = index;
    return this;
  }
  decorate_content(enabled = true) {
    if (this.kind !== 20) throw new TypeError('decorate_content requires a RadioGroup Item');
    if (typeof enabled !== 'boolean') throw new TypeError('Radio decoration requires a boolean');
    if (this.owner) {
      if (this.owner.supports_radio_item_content())
        native.setRadioItemContent(this.owner.handle, this[NODE], enabled);
      else if (enabled) throw new Error(radioContentUnavailable);
    }
    this.radioContentValue = enabled;
    return this;
  }
  radio_tooltip(text, delayMs = 350) {
    if (this.kind !== 6 && this.kind !== 20)
      throw new TypeError('radio_tooltip requires Radio or a direct RadioGroup Item');
    if (text !== null && typeof text !== 'string') throw new TypeError('expected tooltip text or null');
    let config = null;
    if (text) {
      if (text.includes('\0') || Buffer.byteLength(text, 'utf8') > 256)
        throw new RangeError('Radio tooltip text must fit in 256 UTF-8 bytes without NUL');
      if (!Number.isInteger(delayMs) || delayMs < 0 || delayMs > 10000)
        throw new RangeError('Radio tooltip delay must be an integer from 0 to 10000');
      config = [text, delayMs];
    }
    if (this.owner) {
      if (this.owner.supports_radio_tooltip())
        native.setRadioTooltip(this.owner.handle, this[NODE], ...(config ?? ['', 0]));
      else if (config) throw new Error(radioTooltipUnavailable);
    }
    this.radioTooltipValue = config;
    return this;
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
  on_change(callback) { if((!disabledKinds.has(this.kind) && ![46,64,74,80].includes(this.kind) || [4,19,60].includes(this.kind)) || typeof callback!=='function') throw new TypeError('invalid change callback'); this.changeValue=callback; return this; }
  detail(text) { if(!detailKinds.has(this.kind) || typeof text!=='string') throw new TypeError('detail requires alert text');if(this.owner)native.setDetail(this.owner.handle,this[NODE],text);this.detailValue=text;return this; }
  open(value=true) {if(!([15,20,58,63,75].includes(this.kind)||surfaces.has(this.kind))||typeof value!=='boolean')throw new TypeError('invalid open state');if(this.owner)native.setOpen(this.owner.handle,this[NODE],value?1:0);this.openValue=value;this.openSet=true;return this;}
  select(item) {if(!selectedKinds.has(this.kind)||!this.childrenValue.includes(item))throw new TypeError('selection must belong to group');if(this.owner)native.setSelected(this.owner.handle,this[NODE],item[NODE]);this.selectedValue=item;return this;}
  selected() {if(!selectedKinds.has(this.kind)&&this.kind!==73)throw new Error('component has no selection');return this.owner ? this.owner.nodes.get(keyFor(native.getSelected(this.owner.handle,this[NODE]))) : this.selectedValue;}
  anchor(node) {if(![27,28,29].includes(this.kind)||!(node instanceof Node))throw new TypeError('invalid anchor');if(this.owner){if(node.owner!==this.owner)throw new Error('foreign anchor');native.setAnchor(this.owner.handle,this[NODE],node[NODE]);}this.anchorValue=node;return this;}
  is_open() {if(![15,20,24,25,26,27,28,29,58,62,63,75].includes(this.kind))throw new Error('component has no open state');if(!this.owner)return this.openValue;return native.getOpen(this.owner.handle,this[NODE]);}
  on_close(callback) {if(!surfaces.has(this.kind)||typeof callback!=='function')throw new TypeError('invalid close callback');this.closeValue=callback;return this;}
  carousel_axis(axis) {
    if (this.kind !== 65) throw new TypeError('carousel_axis requires a Carousel');
    if (!['horizontal', 'vertical'].includes(axis)) throw new TypeError('axis must be horizontal or vertical');
    if (this.owner) {
      this.owner._alive();
      if (axis === 'vertical' || this.carouselAxisValue === 'vertical') {
        if (!this.owner.supports_carousel_axis()) throw new Error('Carousel axis unavailable');
        native.setCarouselAxis(this.owner.handle, this[NODE], axis === 'vertical' ? 1 : 0);
      }
    }
    this.carouselAxisValue = axis;
    return this;
  }
  carousel_controls({arrows = true, pagination = true} = {}) {
    if (this.kind !== 65) throw new TypeError('carousel_controls requires a Carousel');
    if (typeof arrows !== 'boolean' || typeof pagination !== 'boolean')
      throw new TypeError('arrows and pagination must be booleans');
    const mask = Number(arrows) | (Number(pagination) << 1);
    if (this.owner) {
      this.owner._alive();
      if (mask !== this.carouselControlsValue) {
        if (!this.owner.supports_carousel_controls()) throw new Error('Carousel controls unavailable');
        native.setCarouselControls(this.owner.handle, this[NODE], mask);
      }
    }
    this.carouselControlsValue = mask;
    return this;
  }
  carousel_looping(enabled = true) {
    if (this.kind !== 65) throw new TypeError('carousel_looping requires a Carousel');
    if (typeof enabled !== 'boolean') throw new TypeError('enabled must be a boolean');
    if (this.owner) {
      this.owner._alive();
      if (enabled !== this.carouselLoopingValue) {
        if (!this.owner.supports_carousel_looping()) throw new Error('Carousel looping unavailable');
        native.setCarouselLooping(this.owner.handle, this[NODE], enabled);
      }
    }
    this.carouselLoopingValue = enabled;
    return this;
  }
  carousel_basis(fraction) {
    if (this.kind !== 20) throw new TypeError('carousel_basis requires an Item');
    if (!Number.isFinite(fraction) || fraction < 0 || fraction > 1)
      throw new TypeError('fraction must be finite and in [0, 1]');
    if (this.owner) {
      this.owner._alive();
      if (fraction || this.carouselBasisValue) {
        if (!this.owner.supports_carousel_basis()) throw new Error('Carousel basis unavailable');
        native.setCarouselBasis(this.owner.handle, this[NODE], fraction);
      }
    }
    this.carouselBasisValue = fraction;
    return this;
  }
  visual_size(width,height) {if(!visualKinds.has(this.kind)||![width,height].every(x=>Number.isFinite(x)&&x>=0&&x<=4096))throw new TypeError('invalid visual size');if(this.owner)native.setVisualSize(this.owner.handle,this[NODE],width,height);this.visualValue=[width,height];return this;}
  total(value) {if(![42,43,59].includes(this.kind)||!Number.isInteger(value)||value<1||value>(this.kind===43?10:this.kind===59?256:1000000))throw new TypeError('invalid total');if(this.owner)native.setTotal(this.owner.handle,this[NODE],value);this.totalValue=value;if(this.kind===59)return this;this.rangeValue=[this.kind===42?1:0,value,1];this.numericValue=Math.min(this.numericValue,value);return this;}
  step_status(value) {if(this.kind!==20||!Number.isInteger(value)||value<0||value>63)throw new TypeError('invalid step status/depth');if(this.owner)native.setStepStatus(this.owner.handle,this[NODE],value);this.statusValue=value;return this;}
  numeric_part_bounds(part) {if(!this.owner||!Number.isInteger(part))throw new TypeError('invalid numeric part');return native.numericBounds(this.owner.handle,this[NODE],part);}
  scroll_extent(width,height) {if(this.kind!==46||![width,height].every(x=>Number.isFinite(x)&&x>=0&&x<=32768))throw new TypeError('invalid scroll extent');if(this.owner)native.setScrollExtent(this.owner.handle,this[NODE],width,height);this.extentValue=[width,height];return this;}
  scroll_offset() {if(![46,64,74,80].includes(this.kind))throw new Error('component has no scroll offset');return this.owner?native.getValue(this.owner.handle,this[NODE]):0;}
  attachment_status(value) {
    const names=attachmentStatuses;
    if(this.kind!==52 || !Object.hasOwn(names,value))throw new TypeError('invalid attachment status');
    if(this.owner)native.setAttachmentStatus(this.owner.handle,this[NODE],names[value]);
    this.attachmentValue=names[value];return this;
  }
  stepper_visual(enabled=true,vertical=false,density='md') {
    const densities={md:0,xs:1,sm:2,lg:3};
    if(this.kind!==44 || typeof enabled!=='boolean' || typeof vertical!=='boolean' || !Object.hasOwn(densities,density))throw new TypeError('stepper_visual requires Stepper, bools and md/xs/sm/lg density');
    const value=[+enabled,+vertical,densities[density]];
    if(this.owner)native.setStepperVisual(this.owner.handle,this[NODE],...value);
    this.stepperVisualValue=value;this.stepperFillValue=null;return this;
  }
  stepper_fill(vertical=false,density='md',width=0,height=0) {
    const densities={md:0,xs:1,sm:2,lg:3};
    if(this.kind!==44 || typeof vertical!=='boolean' || typeof density!=='string' || !Object.hasOwn(densities,density) ||
       ![width,height].every(x=>Number.isFinite(x)&&x>=0&&x<=4096))
      throw new TypeError('stepper_fill requires Stepper, bool, md/xs/sm/lg and finite dimensions in [0,4096]');
    const value=[+vertical,densities[density],width,height];
    if(this.owner){
      if(!this.owner.supports_stepper_fill())throw new Error(stepperFillUnavailable);
      native.setStepperFill(this.owner.handle,this[NODE],...value);
    }
    this.stepperFillValue=value;this.stepperVisualValue=null;return this;
  }
  image_rgba(pixels,width,height,stride=width*4) {
    if(![53,20].includes(this.kind) || !(pixels instanceof Uint8Array) || ![width,height].every(x=>Number.isInteger(x)&&x>0&&x<=2048) ||
       !Number.isInteger(stride) || stride<width*4 || stride*height>16777216 || pixels.byteLength!==stride*height)
       throw new TypeError('invalid RGBA byte buffer/dimensions/stride');
    const copy=Uint8Array.from(pixels);
    if(this.owner)native.setImage(this.owner.handle,this[NODE],copy,width,height,stride);
    this.imageValue=[copy,width,height,stride];return this;
  }
  time_value(hour,minute,second=0) {
    if(this.kind!==55 || ![hour,minute,second].every(Number.isInteger) || hour<0 || hour>23 || minute<0 || minute>59 || second<0 || second>59)
      throw new TypeError('invalid time');
    if(this.owner)native.setTime(this.owner.handle,this[NODE],hour,minute,second);
    this.timeValue=[hour,minute,second];return this;
  }
  current_time() {if(this.kind!==55)throw new Error('component has no time');return this.owner?native.getTime(this.owner.handle,this[NODE]):this.timeValue.slice();}
  date_value(year,month,day) {
    validateDate(year,month,day);
    if(![57,58].includes(this.kind))throw new Error('component has no date');
    if(this.owner)native.setDate(this.owner.handle,this[NODE],year,month,day);
    this.dateValue=[year,month,day];return this;
  }
  current_date() {if(![57,58].includes(this.kind))throw new Error('component has no date');return this.owner?native.getDate(this.owner.handle,this[NODE]):this.dateValue.slice();}
  color_rgba(value) {if(this.kind!==56 || !Number.isInteger(value) || value<0 || value>0xffffffff)throw new TypeError('invalid packed RGBA');if(this.owner)native.setColor(this.owner.handle,this[NODE],value);this.colorValue=value;return this;}
  current_color() {if(this.kind!==56)throw new Error('component has no color');return this.owner?native.getColor(this.owner.handle,this[NODE]):this.colorValue;}
  color_part_bounds(part) {if(!this.owner || this.kind!==56 || ![1,2,3].includes(part))throw new TypeError('invalid color part');return native.colorBounds(this.owner.handle,this[NODE],part);}
  calendar_day_bounds(year,month,day) {validateDate(year,month,day);if(!this.owner || this.kind!==57)throw new Error('calendar must be mounted');return native.dayBounds(this.owner.handle,this[NODE],year,month,day);}
  _settings_child(child) {
    if(this.kind!==80 || !this.owner || !(child instanceof Node) || child.owner!==this.owner)
      throw new TypeError('mounted same-app Settings child required');
  }
  settings_query(query) {
    if(this.kind!==80 || !this.owner) throw new TypeError('mounted Settings required');
    if(typeof query!=='string' || query.includes('\0') || Buffer.byteLength(query)>256)
      throw new TypeError('query must contain at most 256 UTF-8 bytes without NUL');
    native.querySettings(this.owner.handle,this[NODE],query); return this;
  }
  settings_page() {
    if(this.kind!==80 || !this.owner) throw new TypeError('mounted Settings required');
    return this.owner.nodes.get(keyFor(native.pageSettings(this.owner.handle,this[NODE])));
  }
  settings_select(page) {this._settings_child(page); native.selectSettings(this.owner.handle,this[NODE],page[NODE]); return this;}
  settings_reset(page) {this._settings_child(page); native.resetSettings(this.owner.handle,this[NODE],page[NODE]); return this;}
  settings_field_bounds(field) {this._settings_child(field); return native.boundsSettings(this.owner.handle,this[NODE],field[NODE]);}
  _dock_panel(panel) {if(this.kind!==79||!this.owner||!(panel instanceof Node)||panel.owner!==this.owner||!this.childrenValue.includes(panel))throw new TypeError('mounted same-app Dock panel required');}
  dock_activate(panel) {this._dock_panel(panel);native.activateDock(this.owner.handle,this[NODE],panel[NODE]);return this;}
  dock_move_split(panel,reference,axis='horizontal',ratio=.5,before=false) {this._dock_panel(panel);this._dock_panel(reference);const axes={horizontal:0,vertical:1};if(!Object.hasOwn(axes,axis)||!Number.isFinite(ratio)||ratio<=0||ratio>=1||typeof before!=='boolean')throw new TypeError('invalid dock split');native.moveDock(this.owner.handle,this[NODE],panel[NODE],reference[NODE],axes[axis],ratio,before?1:0);return this;}
  dock_active() {if(this.kind!==79||!this.owner)throw new TypeError('mounted Dock required');return this.owner.nodes.get(keyFor(native.activeDock(this.owner.handle,this[NODE])));}
  dock_panel_bounds(panel) {this._dock_panel(panel);return native.boundsDock(this.owner.handle,this[NODE],panel[NODE]);}
  dock_serialize() {if(this.kind!==79||!this.owner)throw new TypeError('mounted Dock required');return native.serializeDock(this.owner.handle,this[NODE]);}
  question(required=false,multiple=false) {if(this.kind!==20||typeof required!=='boolean'||typeof multiple!=='boolean')throw new TypeError('question requires Item and bool flags');if(this.owner)native.questionConfig(this.owner.handle,this[NODE],required?1:0,multiple?1:0);this.questionFlags=[required,multiple];return this;}
  questionnaire_state() {if(this.kind!==78||!this.owner)throw new TypeError('mounted Questionnaire required');const state=native.questionState(this.owner.handle,this[NODE]);return {current:this.owner.nodes.get(keyFor(state)),complete:state.complete};}
  choice_selected() {if(this.kind!==20||!this.owner)throw new TypeError('mounted Questionnaire choice required');return native.choiceSelected(this.owner.handle,this[NODE]);}
  questionnaire_next() {if(this.kind!==78||!this.owner)throw new TypeError('mounted Questionnaire required');native.nextQuestion(this.owner.handle,this[NODE]);return this;}
  questionnaire_submit() {if(this.kind!==78||!this.owner)throw new TypeError("mounted Questionnaire required");native.submitQuestion(this.owner.handle,this[NODE]);return this;}
  questionnaire_previous() {if(this.kind!==78||!this.owner)throw new TypeError('mounted Questionnaire required');native.previousQuestion(this.owner.handle,this[NODE]);return this;}
  speech_start() {if(this.kind!==77||!this.owner)throw new TypeError('mounted Speech required');native.startSpeech(this.owner.handle,this[NODE]);return this;}
  speech_finish() {if(this.kind!==77||!this.owner)throw new TypeError('mounted Speech required');native.finishSpeech(this.owner.handle,this[NODE]);return this;}
  speech_report(text,final=false) {if(this.kind!==77||!this.owner||typeof text!=='string'||typeof final!=='boolean')throw new TypeError('mounted Speech and bool final required');native.reportSpeech(this.owner.handle,this[NODE],text,final?1:0);return this;}
  speech_transcript() {if(this.kind!==77||!this.owner)throw new TypeError('mounted Speech required');return native.getSpeech(this.owner.handle,this[NODE]);}
  speech_active() {if(this.kind!==77||!this.owner)throw new TypeError('mounted Speech required');return native.getChecked(this.owner.handle,this[NODE]);}
  padding(value) {if(this.kind!==76||!Number.isFinite(value)||value<0||value>256)throw new TypeError('root padding requires 0..256 pixels');if(this.owner)native.setRootPadding(this.owner.handle,this[NODE],value);this.rootPaddingValue=value;return this;}
  chart_data(data) {
    if(this.kind!==69 || !Array.isArray(data) || data.length>64 || data.some(p=>!Array.isArray(p)||p.length!==2||typeof p[0]!=='string'||p[0].includes('\0')||Buffer.byteLength(p[0])>128||!Number.isFinite(p[1])||Math.abs(p[1])>1e9||(this.chartKindValue===3&&p[1]<0)))throw new TypeError('invalid chart data');
    const copied=data.map(p=>[...p]);if(this.owner)native.setChart(this.owner.handle,this[NODE],copied);this.chartDataValue=copied;return this;
  }
  chart_kind(value) {const names={line:0,area:1,bar:2,pie:3};if(this.kind!==69 || !Object.hasOwn(names,value) || (value==='pie'&&this.chartDataValue.some(p=>p[1]<0)))throw new TypeError('invalid chart kind');if(this.owner)native.setChartKind(this.owner.handle,this[NODE],names[value]);this.chartKindValue=names[value];return this;}
  current_datum(index) {if(this.kind!==69 || !Number.isInteger(index)||index<0||index>=64)throw new TypeError('invalid datum index');if(this.owner)return native.getChart(this.owner.handle,this[NODE],index);if(!this.chartDataValue[index])throw new RangeError('missing datum');return [...this.chartDataValue[index]];}
  plot_points(points) {if(this.kind!==72 || !Array.isArray(points)||points.length>256||points.some(p=>!Array.isArray(p)||p.length!==2||p.some(v=>!Number.isFinite(v)||Math.abs(v)>16384)))throw new TypeError('invalid plot points');const copied=points.map(p=>[...p]);if(this.owner)native.setPlot(this.owner.handle,this[NODE],copied);this.plotPointsValue=copied;return this;}
  current_point(index) {if(this.kind!==72 || !Number.isInteger(index)||index<0||index>=256)throw new TypeError('invalid point index');if(this.owner)return native.getPlot(this.owner.handle,this[NODE],index);if(!this.plotPointsValue[index])throw new RangeError('missing point');return [...this.plotPointsValue[index]];}
  table_data(headers, rows, rowKeys, options) {
    if (this.kind !== 71 || !Array.isArray(headers) || !Array.isArray(rows))
      throw new TypeError('invalid table matrix');
    let rowCapacity;
    if (options !== undefined) {
      if (!options || typeof options !== 'object' || Array.isArray(options))
        throw new TypeError('DataTable capacity requires an options object');
      ({rowCapacity} = options);
      if (!Number.isInteger(rowCapacity) || rowCapacity < 1 || rowCapacity > maxTableRows)
        throw new RangeError(`rowCapacity must be an integer in 1..${maxTableRows}`);
      if (rowKeys === undefined) throw new TypeError('rowCapacity requires rowKeys');
      if (this.owner) requireDataTableCapacity(this.owner, rowCapacity);
    }
    const columnCount = headers.length, rowCount = rows.length;
    if (!Number.isInteger(columnCount) || columnCount < 1 || columnCount > 16 ||
        !Number.isInteger(rowCount) || rowCount < 0 || rowCount > (rowCapacity ?? legacyTableRows))
      throw new TypeError('invalid table matrix');
    const text = value => typeof value === 'string' && !value.includes('\0') &&
      Buffer.byteLength(value) <= 128;
    const copiedHeaders = Array.from({length: columnCount}, (_, index) => headers[index]);
    if (!copiedHeaders.every(text)) throw new TypeError('invalid table matrix');
    const copiedRows = Array.from({length: rowCount}, (_, index) => {
      const row = rows[index];
      if (!Array.isArray(row) || row.length !== copiedHeaders.length)
        throw new TypeError('invalid table matrix');
      const cells = Array.from({length: copiedHeaders.length}, (_, column) => row[column]);
      if (!cells.every(text)) throw new TypeError('invalid table matrix');
      return cells;
    });
    let keys;
    if (rowKeys !== undefined) {
      if (!Array.isArray(rowKeys) || rowKeys.length !== copiedRows.length)
        throw new TypeError('rowKeys must contain one distinct nonzero uint32 per row');
      keys = Array.from({length: copiedRows.length}, (_, index) => rowKeys[index]);
      if (keys.some(key => !Number.isInteger(key) || key < 1 || key > 0xffffffff) ||
          new Set(keys).size !== keys.length)
        throw new TypeError('rowKeys must contain one distinct nonzero uint32 per row');
    }
    const copied = [copiedHeaders, copiedRows, keys, rowCapacity];
    if (this.owner) applyTableData(this, copied);
    this.matrixValue = copied;
    return this;
  }
  configure_load_more(options = {}) {
    if (this.kind !== 71) throw new TypeError('configure_load_more requires DataTable');
    if (!options || typeof options !== 'object' || Array.isArray(options))
      throw new TypeError('DataTable paging requires an options object');
    const {enabled = true, threshold = 0} = options;
    if (typeof enabled !== 'boolean') throw new TypeError('enabled must be boolean');
    if (!Number.isInteger(threshold) || threshold < 0 || threshold > 32)
      throw new RangeError('threshold must be an integer in 0..32');
    if (this.owner) {
      if (!this.owner.supports_data_table_paging())
        throw new Error('DataTable paging is unavailable in this bridge');
      native.configureDataTableLoadMore(this.owner.handle, this[NODE], Number(enabled), threshold);
    }
    this.loadMoreThreshold = enabled ? threshold : undefined;
    return this;
  }
  poll_load_more() {
    if (this.kind !== 71 || !this.owner) throw new Error('poll_load_more requires a mounted DataTable');
    if (!this.owner.supports_data_table_paging())
      throw new Error('DataTable paging is unavailable in this bridge');
    const request = native.pollDataTableLoadMore(this.owner.handle, this[NODE]);
    if (request === undefined) return undefined;
    if (request.capacity !== (this.matrixValue?.[3] ?? legacyTableRows))
      throw new Error('malformed DataTable load-more record');
    return Object.freeze(request);
  }
  current_cell(row,column) {if(this.kind!==71||![row,column].every(v=>Number.isInteger(v)&&v>=0&&v<=0xffffffff))throw new TypeError('invalid cell');if(this.owner)return native.getMatrixCell(this.owner.handle,this[NODE],row,column);if(!this.matrixValue||!this.matrixValue[1][row]||column>=this.matrixValue[0].length)throw new RangeError('missing cell');return this.matrixValue[1][row][column];}
  selection_mode(value) {const modes={row:0,column:1,cell:2};if(this.kind!==71||!Object.hasOwn(modes,value))throw new TypeError('invalid table selection mode');if(this.owner)native.setSelectionMode(this.owner.handle,this[NODE],modes[value]);this.selectionModeValue=modes[value];return this;}
  table_state() {if(this.kind!==71||!this.owner)throw new TypeError('mounted DataTable required');return native.getMatrixState(this.owner.handle,this[NODE]);}
  table_cell_bounds(row,column) {if(this.kind!==71||!this.owner||![row,column].every(v=>Number.isInteger(v)&&v>=0&&v<=0xffffffff))throw new TypeError('invalid mounted cell');return native.matrixCellBounds(this.owner.handle,this[NODE],row,column);}
  table_header_bounds(column) {if(this.kind!==71||!this.owner||!Number.isInteger(column)||column<0||column>0xffffffff)throw new TypeError('invalid mounted header');return native.matrixHeaderBounds(this.owner.handle,this[NODE],0,column);}
  _liveVirtualList() {
    if (this.kind !== 74 || !this.owner) throw new TypeError('mounted VirtualList required');
    this.owner._alive();
    if (!this.owner.supportsVirtualListRows())
      throw new Error('VirtualList live rows require both V1 bridge symbols');
    return this.owner;
  }
  preserveVirtualListAnchor(enabled = true) {
    const app = this._liveVirtualList();
    if (typeof enabled !== 'boolean') throw new TypeError('enabled must be boolean');
    native.virtualRowsOptions(app.handle, 1, this[NODE], enabled ? 1 : 0);
    return this;
  }
  setVirtualListRows(rows) {
    const app = this._liveVirtualList();
    if (!Array.isArray(rows) || rows.length > 64 || rows.length !== this.childrenValue.length)
      throw new RangeError('projection requires all children, at most 64');
    const seen = new Set(), ordered = [], wire = [];
    for (const row of rows.slice(0, 65)) {
      if (!row) throw new TypeError('row required');
      // Capture user-controlled accessors once; commit exactly the validated snapshot.
      const item = row.item, extent = row.extent;
      if (!(item instanceof Node) || item.kind !== 20 || item.owner !== app ||
          item.parent !== this || !this.childrenValue.includes(item) || seen.has(item))
        throw new TypeError('unique live direct Items required');
      if (typeof extent !== 'number' || !Number.isFinite(extent) ||
          !(extent === 0 || (extent >= 1 && extent <= 4096)))
        throw new RangeError('extent must be zero or finite in 1..4096');
      seen.add(item);
      ordered.push(item);
      wire.push({item: item[NODE], extent});
    }
    native.virtualRowsOptions(app.handle, 2, this[NODE], wire);
    this.childrenValue = ordered;
    return this;
  }
  appendVirtualListItem(item) {
    const app = this._liveVirtualList();
    if (!(item instanceof Node) || item.kind !== 20 || item.owner || item.parent ||
        item.childrenValue.length || this.childrenValue.length >= 64)
      throw new TypeError('unattached leaf Item required; capacity is 64');
    if (item.carouselBasisValue) throw new TypeError('carousel_basis requires a direct Carousel Item');
    if (item.radioContentValue || item.radioTooltipValue || item.radioMotionValue ||
        item.radioTabIndexValue !== null || item.radioPaintValues)
      throw new TypeError('RadioGroup Item options unavailable here');
    if ([...app.nodes.values()].some(node => node.keyValue === item.keyValue))
      throw new Error('duplicate key');
    item[NODE] = native.makeNode(app.handle, this[NODE], 20, item.keyValue);
    try {
      native.setText(app.handle, item[NODE], item.textValue);
      if (item.detailValue) native.setDetail(app.handle, item[NODE], item.detailValue);
      if (item.disabledValue) native.setDisabled(app.handle, item[NODE], 1);
      if (item.checkedSet) native.setChecked(app.handle, item[NODE], item.checkedValue ? 1 : 0);
      if (item.openSet) native.setOpen(app.handle, item[NODE], item.openValue ? 1 : 0);
      if (item.visualValue) native.setVisualSize(app.handle, item[NODE], ...item.visualValue);
      if (item.imageValue) native.setImage(app.handle, item[NODE], ...item.imageValue);
      if (item.statusValue !== null) native.setStepStatus(app.handle, item[NODE], item.statusValue);
      if (item.panelValue !== null) native.setValue(app.handle, item[NODE], item.panelValue);
      if (item.questionFlags)
        native.questionConfig(app.handle, item[NODE], ...item.questionFlags.map(value => value ? 1 : 0));
      for (const [part, style] of item.partShadowStyles)
        native.setPartShadows(app.handle, item[NODE], part, style.wire());
    } catch (error) {
      native.removeNode(app.handle, item[NODE]);
      item[NODE] = null;
      throw error;
    }
    item.owner = app;
    item.parent = this;
    app.nodes.set(keyFor(native.nodeId(item[NODE])), item);
    this.childrenValue.push(item);
    return this;
  }
  removeVirtualListItem(item) {
    this._liveVirtualList();
    if (!(item instanceof Node) || item.owner !== this.owner || item.parent !== this ||
        !this.childrenValue.includes(item)) throw new TypeError('live direct Item required');
    item.remove();
    return this;
  }
  item_bounds(item) {if(![73,74].includes(this.kind)||!this.owner||!(item instanceof Node)||item.owner!==this.owner||!this.childrenValue.includes(item))throw new TypeError('item_bounds requires mounted same-app child');return this.kind===73?native.commandBounds(this.owner.handle,this[NODE],item[NODE]):native.virtualBounds(this.owner.handle,this[NODE],item[NODE]);}
  scroll_to(offset) {if(![64,74].includes(this.kind) || !this.owner || !Number.isFinite(offset) || offset<0 || offset>1000000)throw new TypeError('message scroll_to requires a mounted component after first frame and valid offset');native.setValue(this.owner.handle,this[NODE],offset);return this;}
  panel_size(size) {if(this.kind!==20 || !Number.isFinite(size) || size<100 || size>4096)throw new TypeError('Resizable panel requires 100..4096 pixels');if(this.owner)native.setValue(this.owner.handle,this[NODE],size);this.panelValue=size;return this;}
  current_panel_size() {if(this.kind!==20)throw new Error('panel_size requires Resizable Item');return this.owner?native.getValue(this.owner.handle,this[NODE]):this.panelValue??0;}
  resizable_handle_bounds(preceding) {if(this.kind!==66 || !this.owner || !(preceding instanceof Node) || preceding.owner!==this.owner)throw new TypeError('invalid Resizable Item');return native.resizableBounds(this.owner.handle,this[NODE],preceding[NODE]);}
  text_format(value) {const names={markdown:0,html:1,plain:2};if(this.kind!==67 || !Object.hasOwn(names,value))throw new TypeError('invalid text format');if(this.owner)native.setTextFormat(this.owner.handle,this[NODE],names[value]);this.textFormatValue=names[value];return this;}
  select_text(anchor,cursor) {if(this.kind!==67 || !this.owner || ![anchor,cursor].every(x=>Number.isInteger(x)&&x>=0&&x<=0xffffffff))throw new TypeError('selection requires mounted TextView and rendered UTF-8 byte offsets');native.selectText(this.owner.handle,this[NODE],anchor,cursor);return this;}
  selection_text() {if(this.kind!==67 || !this.owner)throw new Error('selection requires mounted TextView');return native.getSelection(this.owner.handle,this[NODE]);}
  hover_color(value) { this._button(); this.hoverValue = rgba(value); if (this.owner) native.setHoverRgba(this.owner.handle, this[NODE], ...this.hoverValue); return this; }
  on_click(callback) { if(![4,20,23,62].includes(this.kind))throw new Error("click requires button"); if (typeof callback !== 'function') throw new TypeError('callback must be a function'); this.clickValue = callback; return this; }
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

function dialog(key,text="") {const node=new Node(24,text);return key===undefined?node:node.key(key);}
function alert_dialog(key,text="") {const node=new Node(25,text);return key===undefined?node:node.key(key);}
function sheet(key,text="") {const node=new Node(26,text);return key===undefined?node:node.key(key);}
function popover(key,text="") {const node=new Node(27,text);return key===undefined?node:node.key(key);}
function tooltip(key,text="") {const node=new Node(28,text);return key===undefined?node:node.key(key);}
function hover_card(key,text="") {const node=new Node(29,text);return key===undefined?node:node.key(key);}
function avatar(key,text="") {const node=new Node(30,text);return key===undefined?node:node.key(key);}
function bubble(key,text="") {const node=new Node(31,text);return key===undefined?node:node.key(key);}
function empty_view(key,text="") {const node=new Node(32,text);return key===undefined?node:node.key(key);}
function group_box(key,text="") {const node=new Node(33,text);return key===undefined?node:node.key(key);}
function kbd(key,text="") {const node=new Node(34,text);return key===undefined?node:node.key(key);}
function skeleton(key,text="") {const node=new Node(35,text);return key===undefined?node:node.key(key);}
function spinner(key,text="") {const node=new Node(36,text);return key===undefined?node:node.key(key);}
function tag(key,text="") {const node=new Node(37,text);return key===undefined?node:node.key(key);}
function marker(key,text="") {const node=new Node(38,text);return key===undefined?node:node.key(key);}
function shimmer(key,text="") {const node=new Node(39,text);return key===undefined?node:node.key(key);}
function list_view(key,text="") {const node=new Node(40,text);return key===undefined?node:node.key(key);}
function menu(key,text="") {const node=new Node(41,text);return key===undefined?node:node.key(key);}
function pagination(key,text="") {const node=new Node(42,text);return key===undefined?node:node.key(key);}
function rating(key,text="") {const node=new Node(43,text);return key===undefined?node:node.key(key);}
function stepper(key,text="") {const node=new Node(44,text);return key===undefined?node:node.key(key);}
function description_list(key,text="") {const node=new Node(45,text);return key===undefined?node:node.key(key);}
function scrollable(key,text="") {const node=new Node(46,text);return key===undefined?node:node.key(key);}
function status_bar(key,text="") {const node=new Node(47,text);return key===undefined?node:node.key(key);}
function toolbar(key,text="") {const node=new Node(48,text);return key===undefined?node:node.key(key);}
function title_bar(key,text="") {const node=new Node(49,text);return key===undefined?node:node.key(key);}
function icon(key,text="") {const node=new Node(50,text);return key===undefined?node:node.key(key);}
function message(key,text="") {const node=new Node(51,text);return key===undefined?node:node.key(key);}
const attachmentSlotRoles = Object.freeze({media: 1, overlay: 2, metadata: 3, actions: 4});
function attachment_slot(key, role, preferredWidth = 0) {
  if (typeof role !== 'string' || !Object.hasOwn(attachmentSlotRoles, role))
    throw new TypeError('invalid Attachment slot role');
  const actions = role === 'actions';
  if (!Number.isFinite(preferredWidth) ||
      (actions ? Math.fround(preferredWidth) <= 0 || preferredWidth > 4096 : preferredWidth !== 0))
    throw new RangeError('invalid Attachment slot width');
  if (typeof key !== 'string' || !key || key.includes('\0') || Buffer.byteLength(key) > 4096)
    throw new TypeError('invalid Attachment slot key');
  const node = new Node(actions ? 1 : 2).key(key);
  node[ATTACHMENT_SLOT] = Object.freeze({role: attachmentSlotRoles[role], width: preferredWidth});
  return node;
}
function attachment(key,text="") {const node=new Node(52,text);return key===undefined?node:node.key(key);}
function image(key,text="") {const node=new Node(53,text);return key===undefined?node:node.key(key);}
function tree(key,text="") {const node=new Node(54,text);return key===undefined?node:node.key(key);}
function time_field(key,text="") {const node=new Node(55,text);return key===undefined?node:node.key(key);}
function color_picker(key,text="") {const node=new Node(56,text);return key===undefined?node:node.key(key);}
function calendar(key,text="") {const node=new Node(57,text);return key===undefined?node:node.key(key);}
function date_picker(key,text="") {const node=new Node(58,text);return key===undefined?node:node.key(key);}
function otp_input(key,text="") {const node=new Node(59,text);return key===undefined?node:node.key(key);}
function form(key,text="") {const node=new Node(60,text);return key===undefined?node:node.key(key);}
function input_group(key,text="") {const node=new Node(61,text);return key===undefined?node:node.key(key);}
function notification(key,text="") {const node=new Node(62,text);return key===undefined?node:node.key(key);}
function focus_trap(key,text="") {const node=new Node(63,text);return key===undefined?node:node.key(key);}
function message_scroller(key,text="") {const node=new Node(64,text);return key===undefined?node:node.key(key);}
function carousel(key,text="") {const node=new Node(65,text);return key===undefined?node:node.key(key);}
function resizable(key,text="") {const node=new Node(66,text);return key===undefined?node:node.key(key);}
function text_view(key,text="") {const node=new Node(67,text);return key===undefined?node:node.key(key);}
function editor(key,text="") {const node=new Node(68,text);return key===undefined?node:node.key(key);}
function chart(key,text=""){const node=new Node(69,text);return key===undefined?node:node.key(key);}
function table(key,text=""){const node=new Node(70,text);return key===undefined?node:node.key(key);}
function data_table(key,text=""){const node=new Node(71,text);return key===undefined?node:node.key(key);}
function plot(key,text=""){const node=new Node(72,text);return key===undefined?node:node.key(key);}
function command(key,text=""){const node=new Node(73,text);return key===undefined?node:node.key(key);}
function virtual_list(key,text=""){const node=new Node(74,text);return key===undefined?node:node.key(key);}
function sidebar(key,text=""){const node=new Node(75,text);return key===undefined?node:node.key(key);}
function root_view(key="root"){return new Node(76,"").key(key);}
function speech(key){return new Node(77,"").key(key);}
function settings(key,text=""){return new Node(80,text).key(key);}
function questionnaire(key,text=""){return new Node(78,text).key(key);}
function dock(key,text=""){return new Node(79,text).key(key);}
const sliderModels = require('./slider-model.cjs')(native, Node, NODE, keyFor);
const localizedTimes = require('./time-field-localized.cjs')(native, Node, NODE, keyFor);
const icons = require('./icon-resources.cjs')(native);
const iconBadges = require('./icon-badge.cjs')(native, Node, NODE, icons, designSizes);
containers.add(iconBadges.badgeKind);
const skeletons = require('./skeleton-presentation.cjs')(native, Node, NODE);
const paginationOptions = require('./pagination-options.cjs')(native, Node, NODE);
const ratingOptions = require('./rating-options.cjs')(native, Node, NODE, icons, designSizes);
const attachments = require('./attachment-options.cjs')(native, Node, NODE, attachmentStatuses);
const disclosures = require('./disclosure.cjs')(native, Node, NODE, designSizes);
class App {
  constructor(options = {}) {
    if (options.initiallyHidden !== undefined && typeof options.initiallyHidden !== "boolean") throw new TypeError("initiallyHidden must be boolean");
    this.disposed = false; this.closed = false; this.nodes = new Map(); this.root = null; this.timer = null;
    this.handle = native.create({width: positive(options.width ?? 640, 'width'), height: positive(options.height ?? 480, 'height'),
      backend: options.backend === 'sdl' ? 1 : options.backend === undefined || options.backend === 'headless' ? 0 : NaN,
      maxNodes: options.maxNodes ?? 1024, maxEvents: options.maxEvents ?? 1024,
      title: options.title, fontPath: options.fontPath, initiallyHidden: options.initiallyHidden ? 1 : 0});
  }
  supportsVirtualListRows() {
    this._alive();
    return native.virtualRowsOptions(this.handle, 0);
  }
  _alive() { if (this.disposed) throw new Error('app is disposed'); }
  font_fallbacks(paths) {
    this._alive();
    if(!Array.isArray(paths) || paths.length>32 || paths.some(p=>typeof p!=='string'||!p))throw new TypeError('fallbacks require at most 32 nonempty paths');
    native.setFontFallbacks(this.handle,paths);return this;
  }
  mount(root) {
    this._alive();
    if (this.root) throw new Error('app already has a root');
    if (!(root instanceof Node) || root.parent || root.owner || ![1,2,76].includes(root.kind)) throw new TypeError('root must be an unattached row, column or root_view');
    if (!this.supports_radio_motion() && requestsRadioMotion(root)) throw new Error(radioMotionUnavailable);
    preflightNodeOptions(this, root);
    preflightDataTables(this, root);
    skeletons.preflight(this, root);
    paginationOptions.preflight(this, root);
    ratingOptions.preflight(this, root);
    disclosures.preflight(this, root);
    attachments.preflight(this, root);
    sliderModels.preflight(this, root);
    localizedTimes.preflight(this, root);
    iconBadges.preflight(this, root);
    const attach = (node, parent) => {
      const parentHandle = parent && parent[NODE];
      const slot = node[ATTACHMENT_SLOT];
      if (slot) node[NODE] = native.attachmentSlot(this.handle, parentHandle, slot.role, node.keyValue, slot.width);
      else if (node.kind === 81) node[NODE] = sliderModels.mount(this, node, parentHandle);
      else if (node.kind === iconBadges.badgeKind) node[NODE] = iconBadges.createBadge(this, node, parentHandle);
      else node[NODE] = native.makeNode(this.handle, parentHandle, node.kind, node.keyValue);
      node.owner = this;
      const id = native.nodeId(node[NODE]); this.nodes.set(keyFor(id), node);
      if(node.kind===76)native.setRootPadding(this.handle,node[NODE],node.rootPaddingValue);
      if(node.kind===59)native.setTotal(this.handle,node[NODE],node.totalValue);
      if (node.textValue !== undefined) native.setText(this.handle, node[NODE], node.textValue);
      if (node.gapValue !== undefined) native.setGap(this.handle, node[NODE], node.gapValue);
      if ([5,6,7,8].includes(node.kind) || (node.kind===20 && node.checkedSet)) native.setChecked(this.handle,node[NODE],node.checkedValue ? 1 : 0);
      if (node.radioPaintValues) {
        for (const [layer, paint] of node.radioPaintValues)
          native.setRadioPaint(this.handle, node[NODE], layer, paint);
      }
      if (node.carouselAxisValue === 'vertical') native.setCarouselAxis(this.handle, node[NODE], 1);
      if (node.carouselControlsValue) native.setCarouselControls(this.handle, node[NODE], node.carouselControlsValue);
      if (node.carouselLoopingValue) native.setCarouselLooping(this.handle, node[NODE], true);
      if (node.carouselBasisValue) native.setCarouselBasis(this.handle, node[NODE], node.carouselBasisValue);
      if (node.radioMotionValue) native.setRadioMotion(this.handle, node[NODE], true);
      if (node.radioTabIndexValue !== null)
        native.setRadioTabIndex(this.handle, node[NODE], true, node.radioTabIndexValue);
      if (node.radioContentValue) native.setRadioItemContent(this.handle, node[NODE], true);
      if (node.radioTooltipValue) native.setRadioTooltip(this.handle, node[NODE], ...node.radioTooltipValue);
      if ([9,14].includes(node.kind)) native.setRange(this.handle,node[NODE],...node.rangeValue);
      if([42,43].includes(node.kind))native.setTotal(this.handle,node[NODE],node.totalValue);
      if (numerics.has(node.kind)) native.setValue(this.handle,node[NODE],node.numericValue);
      if ((node[ATTACHMENT_SLOT] || node.kind===81 || disabledKinds.has(node.kind)) && (node.kind!==20 || node.disabledValue)) native.setDisabled(this.handle,node[NODE],node.disabledValue ? 1 : 0);
      if(node.kind===15 || (node.kind===20 && node.openSet))native.setOpen(this.handle,node[NODE],node.openValue?1:0);
      if(detailKinds.has(node.kind))native.setDetail(this.handle,node[NODE],node.detailValue);
      if(node.kind===52)native.setAttachmentStatus(this.handle,node[NODE],node.attachmentValue);
      if(node.imageValue)native.setImage(this.handle,node[NODE],...node.imageValue);
      if(node.kind===55)native.setTime(this.handle,node[NODE],...node.timeValue);
      localizedTimes.mount(this,node);
      skeletons.mount(this, node);
      paginationOptions.mount(this, node);
      ratingOptions.mount(this, node);
      disclosures.mount(this, node);
      attachments.mount(this, node);
      iconBadges.mount(this, node);
      if([57,58].includes(node.kind))native.setDate(this.handle,node[NODE],...node.dateValue);
      if(node.kind===56)native.setColor(this.handle,node[NODE],node.colorValue);
      if(node.kind===69){native.setChartKind(this.handle,node[NODE],node.chartKindValue);native.setChart(this.handle,node[NODE],node.chartDataValue);}
      if(node.kind===72)native.setPlot(this.handle,node[NODE],node.plotPointsValue);
      if(node.kind===71)native.setSelectionMode(this.handle,node[NODE],node.selectionModeValue);
      if(node.kind===75 && node.openSet)native.setOpen(this.handle,node[NODE],node.openValue?1:0);
      if (node.kind === 71 && node.matrixValue) applyTableData(node, node.matrixValue);
      if (node.loadMoreThreshold !== undefined)
        node.configure_load_more({threshold: node.loadMoreThreshold});
      if(node.kind===67)native.setTextFormat(this.handle,node[NODE],node.textFormatValue);
      if(node.questionFlags)native.questionConfig(this.handle,node[NODE],...node.questionFlags.map(v=>v?1:0));
      if(node.panelValue!==null)native.setValue(this.handle,node[NODE],node.panelValue);
      if(node.extentValue)native.setScrollExtent(this.handle,node[NODE],...node.extentValue);
      if(node.visualValue)native.setVisualSize(this.handle,node[NODE],...node.visualValue);
      if(node.statusValue!==null)native.setStepStatus(this.handle,node[NODE],node.statusValue);
      if(node.stepperFillValue)native.setStepperFill(this.handle,node[NODE],...node.stepperFillValue);
      if(node.stepperVisualValue)native.setStepperVisual(this.handle,node[NODE],...node.stepperVisualValue);
      if(node.designSizeValue!==undefined)native.setDesignSize(this.handle,node[NODE],node.designSizeValue);
      if(node.shadowStyleValue)native.setShadows(this.handle,node[NODE],node.shadowStyleValue.wire());
      for(const [part,style] of node.partShadowStyles)native.setPartShadows(this.handle,node[NODE],part,style.wire());
      if(node.avatarIdentityValue)native.setAvatarIdentity(this.handle,node[NODE],...node.avatarIdentityValue);
      if(node.avatarPlaceholderValue)native.setAvatarPlaceholder(this.handle,node[NODE],...node.avatarPlaceholderValue);
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
    try { attach(root, null);
      for(const node of this.nodes.values())if(node.anchorValue){if(node.anchorValue.owner!==this)throw new Error('foreign/unmounted anchor');native.setAnchor(this.handle,node[NODE],node.anchorValue[NODE]);}
      for(const node of this.nodes.values()){if((surfaces.has(node.kind)||[58,63].includes(node.kind))&&node.openValue)native.setOpen(this.handle,node[NODE],1);if(node.selectedValue)native.setSelected(this.handle,node[NODE],node.selectedValue[NODE]);}
      this.root = root;
      for (const node of this.nodes.values()) attachments.finishMount(node);
    } catch (error) { this.dispose(); throw error; }
    return this;
  }
  _remove(node) {
    this._alive();
    native.removeNode(this.handle, node[NODE]);
    const clear = current => { localizedTimes.release(current); iconBadges.release(current); ratingOptions.release(current); if(current.kind===81)current.requestValue=null; this.nodes.delete(keyFor(native.nodeId(current[NODE]))); current.owner = null; for (const child of current.childrenValue) clear(child); };
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
  scroll(x,y,dx,dy) {this._alive();native.scroll(this.handle,x,y,dx,dy);return this;}
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
    if(this.sliderDispatching)throw new Error('step unavailable during controlled Slider callback');
    if(this.localizedTimeDispatching)throw new Error('step unavailable during localized TimeField callback');
    this._alive();
    if (this.running) throw new Error("step is unavailable during run");
    native.step(this.handle, dt);
    return this._dispatch();
  }
  step_only(dt = 1 / 60) {
    if(this.sliderDispatching)throw new Error('step_only unavailable during controlled Slider callback');
    if(this.localizedTimeDispatching)throw new Error('step_only unavailable during localized TimeField callback');
    this._alive();
    if (this.running) throw new Error("step is unavailable during run");
    native.step(this.handle, dt);
    return this;
  }
  poll_notification_v1() {
    this._alive();
    if (typeof native.pollNotificationV1 !== 'function') throw new Error('V1 notification addon unavailable');
    const raw = native.pollNotificationV1(this.handle);
    if (raw === undefined) return undefined;
    const handle = Object.freeze({slot: raw.slot, generation: raw.generation});
    const selectedHandle = Object.freeze({slot: raw.selectedSlot, generation: raw.selectedGeneration});
    const legacy = Object.freeze({type: raw.type, checkedRaw: raw.checkedRaw, value: raw.value,
      text: raw.text, selectedHandle});
    const common = {schemaVersion: 1, handle, node: this.nodes.get(keyFor(handle)),
      frame: raw.frame, hasFrame: raw.hasFrame, nodeKind: raw.nodeKind, legacy};
    switch (raw.payloadKind) {
      case 0: return Object.freeze({...common, kind: 'activation'});
      case 1: return Object.freeze({...common, kind: 'boolean-change', value: raw.booleanValue});
      case 5: return Object.freeze({...common, kind: 'table-change', reason: raw.reason === 1 ? 'selection' : 'sort',
        row: raw.row, column: raw.column, sortColumn: raw.sortColumn, sortDirection: raw.sortDirection});
      case 6: return Object.freeze({...common, kind: 'surface-close', result: raw.result});
      default: return Object.freeze({...common, kind: 'legacy', selectedNode: this.nodes.get(keyFor(selectedHandle))});
    }
  }
  supports_attachment_actions() { this._alive(); return typeof native.attachmentActions === "function" && native.attachmentActions(this.handle); }
  poll_component_notification_v1() {
    if (!this.supports_attachment_actions()) throw new Error("Attachment actions unavailable");
    return this._poll_value_notification_v1(true);
  }
  poll_value_notification_v1() { return this._poll_value_notification_v1(false); }
  _poll_value_notification_v1(actions) {
    this._alive();
    if (typeof native.pollValueNotificationV1 !== 'function') throw new Error('V1 notification addon unavailable');
    const raw = actions ? native.pollComponentNotificationV1(this.handle) : native.pollValueNotificationV1(this.handle);
    if (raw === undefined) return undefined;
    const handle = Object.freeze({slot: raw.slot, generation: raw.generation});
    const selectedHandle = Object.freeze({slot: raw.selectedSlot, generation: raw.selectedGeneration});
    const legacy = Object.freeze({type: raw.type, checkedRaw: raw.checkedRaw, value: raw.value,
      text: raw.text, selectedHandle});
    const common = {schemaVersion: 1, handle, node: this.nodes.get(keyFor(handle)),
      frame: raw.frame, hasFrame: raw.hasFrame, nodeKind: raw.nodeKind, legacy};
    switch (raw.payloadKind) {
      case 8: return Object.freeze({...common, kind: 'attachment-action',
        action: {1:'open',2:'remove',4:'retry'}[raw.action],
        part: ['unknown','card','remove','media-retry','description-retry'][raw.part],
        source: raw.actionFlags & 4 ? {2:'pointer',3:'keyboard',4:'accessibility'}[raw.source] : undefined,
        eventIndex: raw.actionFlags & 2 ? raw.eventIndex : undefined});
      case 0: return Object.freeze({...common, kind: 'activation'});
      case 1: return Object.freeze({...common, kind: 'boolean-change', value: raw.booleanValue});
      case 2: return Object.freeze({...common, kind: 'scalar-change', value: raw.value});
      case 3: return Object.freeze({...common, kind: 'text-change', text: raw.text});
      case 4: return Object.freeze({...common, kind: 'selection-change', selectedHandle,
        selectedNode: this.nodes.get(keyFor(selectedHandle))});
      case 5: return Object.freeze({...common, kind: 'table-change', reason: raw.reason === 1 ? 'selection' : 'sort',
        row: raw.row, column: raw.column, sortColumn: raw.sortColumn, sortDirection: raw.sortDirection});
      case 6: return Object.freeze({...common, kind: 'surface-close', result: raw.result});
      default: return Object.freeze({...common, kind: 'legacy', selectedNode: this.nodes.get(keyFor(selectedHandle))});
    }
  }
  _dispatch() {
    if(this.sliderDispatching)throw new Error('nested controlled Slider dispatch unavailable');
    if(this.localizedTimeDispatching)throw new Error('nested localized TimeField dispatch unavailable');
    let event; let delivered = 0;
    while ((event = native.poll(this.handle)) !== undefined) {
      const node = this.nodes.get(keyFor(event));
      if (event.type === 1 && node && node.clickValue) { node.clickValue({node, frame: event.frame}); delivered++; }
      if (event.type === 2 && node && node.changeValue) { node.changeValue({node,frame:event.frame,checked:event.checked,value:event.value,text:event.text,selectedHandle:{slot:event.selectedSlot,generation:event.selectedGeneration}}); delivered++; }
      if(event.type===3 && node && node.closeValue){node.closeValue({node,frame:event.frame,result:event.value});delivered++;}
      if (this.disposed || this.closed) break;
    }
    if(this.sliderEnabled && !this.closed && !this.disposed)delivered+=sliderModels.dispatch(this);
    if(this.localizedTimeEnabled && !this.closed && !this.disposed)delivered+=localizedTimes.dispatch(this);
    return delivered;
  }
  supports_icon_presentation() { return iconBadges.supported(this, 'iconNode'); }
  supports_configured_badge() { return iconBadges.supported(this, 'configuredBadge'); }
  supports_icon_resources() { return icons.supports(this); }
  icon_resource(source, limits) { return new icons.IconResource(this, source, limits); }
  default_icon_limits() { return icons.defaultLimits(this); }
  icon_catalog_count() { return icons.catalogCount(this); }
  icon_catalog_entry(index) { return icons.catalogEntry(this, index); }
  query_icon(name) { return icons.query(this, name); }
  copy_icon_asset(name) { return icons.copyAsset(this, name); }
  supports_keyed_data_table() {
    this._alive();
    return typeof native.supportsKeyedDataTable === 'function' && native.supportsKeyedDataTable();
  }
  data_table_max_row_capacity() {
    this._alive();
    if (typeof native.maxMatrixCapacity === 'function' && typeof native.setMatrixCapacity === 'function')
      return native.maxMatrixCapacity() || undefined;
    return undefined;
  }
  supports_data_table_paging() {
    this._alive();
    return typeof native.supportsDataTablePaging === 'function' &&
      typeof native.configureDataTableLoadMore === 'function' &&
      typeof native.pollDataTableLoadMore === 'function' && native.supportsDataTablePaging();
  }
  supports_pagination_custom_size() { return paginationOptions.supportsCustom(this); }
  supports_radio_paint() {
    this._alive();
    return typeof native.supportsRadioPaint === 'function' && typeof native.setRadioPaint === 'function' && native.supportsRadioPaint();
  }
  supports_stepper_fill() {
    this._alive();
    return typeof native.supportsStepperFill==='function' && typeof native.setStepperFill==='function' && native.supportsStepperFill();
  }
  supports_carousel_axis() {
    this._alive();
    return typeof native.supportsCarouselAxis === 'function' &&
      typeof native.setCarouselAxis === 'function' && native.supportsCarouselAxis();
  }
  supports_carousel_controls() {
    this._alive();
    return typeof native.supportsCarouselControls === 'function' &&
      typeof native.setCarouselControls === 'function' && native.supportsCarouselControls();
  }
  supports_carousel_looping() {
    this._alive();
    return typeof native.supportsCarouselLooping === 'function' &&
      typeof native.setCarouselLooping === 'function' && native.supportsCarouselLooping();
  }
  supports_carousel_basis() {
    this._alive();
    return typeof native.supportsCarouselBasis === 'function' &&
      typeof native.setCarouselBasis === 'function' && native.supportsCarouselBasis();
  }
  supports_radio_motion() {
    this._alive();
    return typeof native.supportsRadioMotion === 'function' &&
      typeof native.setRadioMotion === 'function' && native.supportsRadioMotion();
  }
  supports_radio_tab_index() {
    this._alive();
    return typeof native.supportsRadioTabIndex === 'function' &&
      typeof native.setRadioTabIndex === 'function' && native.supportsRadioTabIndex();
  }
  supports_radio_item_content() {
    this._alive();
    return typeof native.supportsRadioItemContent === 'function' &&
      typeof native.setRadioItemContent === 'function' && native.supportsRadioItemContent();
  }
  supports_radio_tooltip() {
    this._alive();
    return typeof native.supportsRadioTooltip === 'function' &&
      typeof native.setRadioTooltip === 'function' && native.supportsRadioTooltip();
  }
  supports_disclosure_title_state() { return disclosures.supportsTitle(this); }
  supports_disclosure_heading() { return disclosures.supportsHeading(this); }
  supports_disclosure_divider() { return disclosures.supportsDivider(this); }
  supports_disclosure_presentation() { return disclosures.supports(this); }
  supports_attachment_slots() {
    this._alive();
    return typeof native.attachmentSlots === 'function' && typeof native.attachmentSlot === 'function' &&
      native.attachmentSlots(this.handle);
  }
  supports_attachment_options() { return attachments.supports(this); }
  supports_attachment_preview() { return attachments.supports(this,true); }
  supports_rating_options() { return ratingOptions.supports(this); }
  supports_pagination_options() { return paginationOptions.supports(this); }
  supports_skeleton_presentation() { return skeletons.supports(this); }
  supports_localized_time_field(){return localizedTimes.supports(this);}
  enable_localized_time_fields(capacity=4096){return localizedTimes.enable(this,capacity);}
  poll_localized_time_change(){if(this.localizedTimeDispatching)throw new Error('poll unavailable during localized TimeField callback');return localizedTimes.poll(this);}
  dispatch_localized_time_changes(){return localizedTimes.dispatch(this);}
  supports_slider_model(){this._alive();return sliderModels.supports(this);}
  enable_slider_requests(capacity=4096){return sliderModels.enable(this,capacity);}
  poll_slider_request(){return sliderModels.poll(this);}
  accept_slider_request(receipt,value){sliderModels.accept(this,receipt,value);}
  dispatch_slider_requests(){if(this.localizedTimeDispatching)throw new Error('nested typed dispatch unavailable');return sliderModels.dispatch(this);}
  run(afterFrame) {
    if(this.sliderDispatching)throw new Error('run unavailable during controlled Slider callback');
    if(this.localizedTimeDispatching)throw new Error('run unavailable during localized TimeField callback');
    this._alive();
    if (afterFrame !== undefined && typeof afterFrame !== 'function')
      throw new TypeError('after-frame hook must be a function');
    if (this.running || this.timer) throw new Error('app is already running');
    this.running = true;
    try {
      native.run(this.handle, () => {
        this._dispatch();
        if (afterFrame && !this.disposed && !this.closed) afterFrame(this);
      });
    }
    finally { this.running = false; }
    return this;
  }
  start(fps = 60) {
    this._alive(); positive(fps, 'fps');
    if (this.timer || this.running) throw new Error('app is already running');
    const interval = 1000 / fps;
    this.timer = setInterval(() => { try { this.step(1 / fps); } catch (error) {
      this.stop();
      if (error.message === 'closed') { this.dispose(); return; }
      process.nextTick(() => { throw error; });
    } }, interval);
    return this;
  }
  set_window_visible(visible) {this._alive();if(typeof visible!=="boolean")throw new TypeError("visible must be boolean");native.setWindowVisible(this.handle,visible?1:0);return this;}
  window_visible() {this._alive();return native.getWindowVisible(this.handle);}
  clipboard_set(text) {this._alive();if(typeof text!=="string"||text.includes("\0")||Buffer.byteLength(text)>65536)throw new TypeError("clipboard accepts UTF-8 text up to 65536 bytes without NUL");native.setClipboard(this.handle,text);return this;}
  clipboard_get() {this._alive();return native.getClipboard(this.handle);}
  theme_presets() {this._alive();return native.themePresets();}
  theme_preset(index) {this._alive();if(!Number.isInteger(index)||index<0||index>0xffffffff)throw new TypeError("invalid theme index");native.setThemePreset(this.handle,index);return this;}
  current_theme_preset() {this._alive();return native.getThemePreset(this.handle);}
  stop() { if (this.timer) { clearInterval(this.timer); this.timer = null; } return this; }
  close() { this._alive(); this.stop(); native.close(this.handle); this.closed = true; for(const node of this.nodes.values()){localizedTimes.release(node);iconBadges.release(node);ratingOptions.release(node);if(node.kind===81)node.requestValue=null;} return this; }
  dispose() { if (!this.disposed) { this.stop(); native.dispose(this.handle); this.disposed = true; for(const node of this.nodes.values()){localizedTimes.release(node);iconBadges.release(node);ratingOptions.release(node);if(node.kind===81)node.requestValue=null;} this.nodes.clear(); } }
}
module.exports = {RadioPaintLayer,radio_item:item,ShadowPart,BoxShadow,ShadowStyle,ShadowKind,ShadowPreset,CornerRadii,App,row,column,label,button,checkbox,radio,switch: switch_control,switch_control,toggle,slider,progress,badge,input,textarea,number_input,collapsible,alert,radio_group,tabs,accordion,item,select,combobox,dropdown_button,dialog,alert_dialog,sheet,popover,tooltip,hover_card,avatar,bubble,empty_view,group_box,kbd,skeleton,spinner,tag,marker,shimmer,list_view,menu,pagination,rating,stepper,description_list,scrollable,status_bar,toolbar,title_bar,icon,message,attachment,attachment_slot,image,tree,time_field,color_picker,calendar,date_picker,otp_input,form,input_group,notification,focus_trap,message_scroller,carousel,resizable,text_view,editor,chart,table,data_table,plot,command,virtual_list,sidebar,root_view,speech,questionnaire,dock,settings};

Object.assign(module.exports, sliderModels.exports);

Object.assign(module.exports, localizedTimes.exports, skeletons.exports, icons.exports, iconBadges.exports);
