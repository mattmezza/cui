'use strict';

const {endianness} = require('node:os');
const nativeOrder = endianness() === 'BE' ? 'BE' : 'LE';
const CONFIG = Symbol('disclosure config');
const STYLE = Symbol('disclosure style');
const flags = ['joined', 'bordered', 'single', 'allowCollapse', 'customCard', 'clipCard', 'borderless'];
const dimensions = ['width', 'cardRadius', 'cardBorderWidth', 'cardPadding', 'cardGap'];
const colors = ['hoverRgba', 'pressedRgba', 'expandedRgba', 'disabledRgba', 'focusRgba'];
const cursors = ['default', 'pointer', 'text', 'resize_horizontal', 'resize_vertical', 'move',
  'not_allowed', 'crosshair', 'grab', 'grabbing', 'zoom_in', 'zoom_out', 'wait', 'help'];

function writeUInt(record, value, offset) {
  record[`writeUInt32${nativeOrder}`](value, offset);
}

function color(value) {
  if (!Number.isInteger(value) || value < 0 || value > 0xffffffff)
    throw new RangeError('color must be uint32 RGBA');
  return value;
}

function wire(value, size) {
  if (!value || typeof value !== 'object' || Array.isArray(value))
    throw new TypeError('expected disclosure record or null');
  const record = Buffer.alloc(size);
  writeUInt(record, size, 0);
  writeUInt(record, 1, 4);
  return record;
}

function encodeConfig(value, sizes) {
  if (value === null) return null;
  const record = wire(value, 172);
  let mask = 0;
  flags.forEach((name, index) => {
    const enabled = value[name];
    if (enabled !== undefined && typeof enabled !== 'boolean')
      throw new TypeError('disclosure flags must be boolean');
    if (enabled) mask |= 1 << index;
  });
  if (value.clipCard && !value.customCard) throw new RangeError('clipCard requires customCard');
  writeUInt(record, mask, 8);
  if (value.designSize != null && !Object.hasOwn(sizes, value.designSize))
    throw new RangeError('invalid disclosure size');
  writeUInt(record, value.designSize == null ? 0 : sizes[value.designSize], 12);

  dimensions.forEach((name, index) => {
    const dimension = value[name] ?? 0;
    if (!Number.isFinite(dimension) ||
        !((name === 'width' && dimension === -1) || (dimension >= 0 && dimension <= 4096)))
      throw new RangeError('invalid disclosure dimension');
    record[`writeFloat${nativeOrder}`](dimension, 16 + index * 4);
  });
  writeUInt(record, color(value.cardBackgroundRgba ?? 0), 36);
  writeUInt(record, color(value.cardBorderRgba ?? 0), 40);
  ['collapsedIcon', 'expandedIcon'].forEach((name, index) => {
    const icon = value[name] ?? '';
    if (typeof icon !== 'string' || icon.includes('\0') || Buffer.byteLength(icon) > 63)
      throw new RangeError('icon names must contain at most 63 UTF-8 bytes without NUL');
    record.write(icon, 44 + index * 64, 63, 'utf8');
  });
  return record;
}

function encodeStyle(value) {
  if (value === null) return null;
  const record = wire(value, 40);
  let mask = 0;
  colors.forEach((name, index) => {
    if (value[name] == null) return;
    mask |= 1 << index;
    writeUInt(record, color(value[name]), 16 + index * 4);
  });
  if (value.cursor != null) {
    const cursor = cursors.indexOf(value.cursor);
    if (cursor < 0) throw new RangeError('invalid disclosure cursor');
    mask |= 1 << 5;
    writeUInt(record, cursor, 36);
  }
  writeUInt(record, mask, 8);
  return mask ? record : null;
}

module.exports = (native, Node, handle, sizes) => {
  function supports(app) {
    app._alive();
    return typeof native.disclosureOptions === 'function' && native.disclosureOptions(app.handle, 0);
  }

  function requireSupport(app) {
    if (!supports(app)) throw new Error('disclosure presentation requires both optional bridge symbols');
  }

  function apply(app, node, operation, record) {
    requireSupport(app);
    native.disclosureOptions(app.handle, operation, node[handle], record);
  }

  Node.prototype.disclosure_configure = function (config) {
    if (this.kind !== 15 && this.kind !== 19)
      throw new TypeError('disclosure configuration requires Accordion or Collapsible');
    const record = encodeConfig(config, sizes);
    if (config && this.kind === 15 && flags.slice(0, 4).some(name => config[name]))
      throw new RangeError('Accordion flags require Accordion');
    if (config?.borderless && this.kind === 19)
      throw new RangeError('borderless requires Collapsible');
    if (this.owner) apply(this.owner, this, 1, record);
    this[CONFIG] = record;
    return this;
  };

  Node.prototype.set_trigger_style = function (style) {
    if (this.kind !== 15 && this.kind !== 20)
      throw new TypeError('trigger style requires Collapsible or direct Accordion Item');
    const record = encodeStyle(style);
    if (this.owner) apply(this.owner, this, 2, record);
    this[STYLE] = record;
    return this;
  };

  return {
    supports,
    preflight(app, root) {
      const pending = [root];
      while (pending.length) {
        const node = pending.pop();
        if (node[CONFIG] || node[STYLE]) requireSupport(app);
        if (node[STYLE] && node.kind === 20 && node.parent?.kind !== 19)
          throw new TypeError('styled Item must be a direct Accordion child');
        for (const child of node.childrenValue) pending.push(child);
      }
    },
    mount(app, node) {
      if (node[CONFIG]) apply(app, node, 1, node[CONFIG]);
      if (node[STYLE]) apply(app, node, 2, node[STYLE]);
    },
  };
};
