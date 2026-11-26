'use strict';

const {endianness} = require('node:os');
const nativeOrder = endianness() === 'BE' ? 'BE' : 'LE';
const CONFIG = Symbol('disclosure config');
const STYLE = Symbol('disclosure style');
const TITLES = Symbol('disclosure title states');
const HEADINGS = Symbol('disclosure headings');
const DIVIDER = Symbol('disclosure divider');
const flags =
    ['joined', 'bordered', 'single', 'allowCollapse', 'customCard', 'clipCard', 'borderless'];
const dimensions = ['width', 'cardRadius', 'cardBorderWidth', 'cardPadding', 'cardGap'];
const colors = ['hoverRgba', 'pressedRgba', 'expandedRgba', 'disabledRgba', 'focusRgba'];
const cursors = [
  'default', 'pointer', 'text', 'resize_horizontal', 'resize_vertical', 'move', 'not_allowed',
  'crosshair', 'grab', 'grabbing', 'zoom_in', 'zoom_out', 'wait', 'help'
];

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

function exact(value, fields) {
  if (!value || typeof value !== 'object' ||
      ![Object.prototype, null].includes(Object.getPrototypeOf(value)) ||
      Reflect.ownKeys(value).some(k => !fields.includes(k)))
    throw new TypeError('expected exact disclosure record');
}
function dimension(v, minimum = 0) {
  if (typeof v !== 'number' || !Number.isFinite(v) || v < minimum || v > 4096)
    throw new RangeError('invalid disclosure dimension');
  return v;
}
function encodeHeading(value) {
  if (value === null) return null;
  exact(value, ['box', 'typography', 'titleRgba']);
  const r = wire(value, 76);
  let mask = 0;
  if (value.box != null) {
    const names = [
      'width', 'height', 'padding', 'gap', 'radius', 'borderWidth', 'backgroundRgba', 'borderRgba'
    ];
    exact(value.box, names);
    mask |= 1;
    names.forEach((n, i) => {
      const v = value.box[n] === undefined ? 0 : value.box[n];
      if (i < 6) {
        if (!(n === 'width' && v === -1)) dimension(v);
        r[`writeFloat${nativeOrder}`](v, 44 + i * 4);
      } else
        writeUInt(r, color(v), 44 + i * 4);
    });
  }
  if (value.typography != null) {
    const t = value.typography;
    exact(t, [
      'fontSize', 'lineHeight', 'letterSpacing', 'weight', 'align', 'wrap', 'italic', 'monospace'
    ]);
    mask |= 2;
    ['fontSize', 'lineHeight', 'letterSpacing'].forEach((n, i) => {
      const v = t[n] === undefined && i !== 0 ? 0 : t[n];
      dimension(v, i === 2 ? -4096 : 0);
      if (i === 0 && Math.fround(v) <= 0) throw new RangeError('fontSize must be positive');
      r[`writeFloat${nativeOrder}`](v, 16 + i * 4);
    });
    ['weight', 'align'].forEach((n, i) => {
      const v = t[n] === undefined ? 0 : t[n];
      if (!Number.isInteger(v) || v < 0 || v > (i === 0 ? 3 : 2))
        throw new RangeError('invalid typography enum');
      writeUInt(r, v, 28 + i * 4);
    });
    let flags = 0;
    ['wrap', 'italic', 'monospace'].forEach((n, i) => {
      if (t[n] !== undefined && typeof t[n] !== 'boolean')
        throw new TypeError('typography flags must be boolean');
      if (t[n]) flags |= 1 << i;
    });
    writeUInt(r, flags, 36);
  }
  if (value.titleRgba != null) {
    mask |= 4;
    writeUInt(r, color(value.titleRgba), 40);
  }
  writeUInt(r, mask, 8);
  return mask ? r : null;
}
function encodeDivider(value) {
  if (value === null) return null;
  exact(value, ['thickness', 'insetLeft', 'insetRight', 'colorRgba']);
  const r = wire(value, 32);
  ['thickness', 'insetLeft', 'insetRight'].forEach(
      (n, i) => r[`writeFloat${nativeOrder}`](
          dimension(value[n] === undefined ? 0 : value[n]), 16 + i * 4));
  writeUInt(r, color(value.colorRgba === undefined ? 0 : value.colorRgba), 28);
  return r;
}
module.exports = (native, Node, handle, sizes) => {
  function supports(app) {
    app._alive();
    return typeof native.disclosureOptions === 'function' &&
        native.disclosureOptions(app.handle, 0);
  }

  function requireSupport(app) {
    if (!supports(app))
      throw new Error('disclosure presentation requires both optional bridge symbols');
  }

  function apply(app, node, operation, record) {
    requireSupport(app);
    native.disclosureOptions(app.handle, operation, node[handle], record);
  }

  Node.prototype.disclosure_configure = function(config) {
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

  Node.prototype.set_trigger_style = function(style) {
    if (this.kind !== 15 && this.kind !== 20)
      throw new TypeError('trigger style requires Collapsible or direct Accordion Item');
    const record = encodeStyle(style);
    if (this.owner) apply(this.owner, this, 2, record);
    this[STYLE] = record;
    return this;
  };

  function supportsOptional(app, divider = false) {
    app._alive();
    return typeof native.disclosureHeadingOptions === 'function' &&
        native.disclosureHeadingOptions(app.handle, divider ? 2 : 0);
  }
  function applyOptional(app, node, record, layer = 0, divider = false) {
    if (!supportsOptional(app, divider))
      throw new Error('disclosure requires optional bridge symbol');
    native.disclosureHeadingOptions(app.handle, divider ? 3 : 1, node[handle], layer, record);
  }
  function supportsTitle(app) {
    app._alive();
    return typeof native.disclosureHeadingOptions === 'function' &&
        native.disclosureHeadingOptions(app.handle, 4);
  }
  function applyTitle(app, node, state, record) {
    if (!supportsTitle(app)) throw new Error('title state requires optional bridge symbol');
    native.disclosureHeadingOptions(app.handle, 5, node[handle], state, record);
  }
  Node.prototype.set_disclosure_title_state = function(state, value) {
    const index = ['hovered', 'focus-visible', 'pressed'].indexOf(state);
    if (index < 0) throw new RangeError('invalid disclosure title state');
    if (![15, 19, 20].includes(this.kind)) throw new TypeError('invalid title state target');
    if (value !== null) exact(value, ['typography', 'titleRgba']);
    const r = encodeHeading(value);
    if (this.owner) applyTitle(this.owner, this, index, r);
    if (r === null) this[TITLES]?.delete(index);
    else (this[TITLES] ??= new Map()).set(index, r);
    return this;
  };
  Node.prototype.set_disclosure_heading = function(layer, value) {
    if (!['normal', 'expanded', 'disabled'].includes(layer))
      throw new RangeError('invalid heading layer');
    if (![15, 19, 20].includes(this.kind)) throw new TypeError('invalid heading target');
    const r = encodeHeading(value);
    const index = ['normal', 'expanded', 'disabled'].indexOf(layer);
    if (this.owner) applyOptional(this.owner, this, r, index);
    if (r === null) {
      this[HEADINGS]?.delete(index);
    } else {
      (this[HEADINGS] ??= new Map()).set(index, r);
    }
    return this;
  };
  Node.prototype.set_disclosure_divider = function(value) {
    if (this.kind !== 19) throw new TypeError('divider requires Accordion');
    const r = encodeDivider(value);
    if (this.owner) applyOptional(this.owner, this, r, 0, true);
    this[DIVIDER] = r;
    return this;
  };
  return {
    supports,
    supportsTitle,
    supportsHeading: app => supportsOptional(app),
    supportsDivider: app => supportsOptional(app, true),
    preflight(app, root) {
      const pending = [root];
      while (pending.length) {
        const node = pending.pop();
        if (node[TITLES]?.size) {
          if (!supportsTitle(app)) throw new Error('title state requires optional bridge symbol');
          if (node.kind === 20 && node.parent?.kind !== 19)
            throw new TypeError('title state Item must be direct Accordion child');
        }
        if (node[HEADINGS]?.size) {
          if (!supportsOptional(app)) throw new Error('heading requires optional bridge symbol');
          if (node.kind === 20 && node.parent?.kind !== 19)
            throw new TypeError('heading Item must be direct Accordion child');
        }
        if (node[DIVIDER] && !supportsOptional(app, true))
          throw new Error('divider requires optional bridge symbol');
        if (node[CONFIG] || node[STYLE]) requireSupport(app);
        if (node[STYLE] && node.kind === 20 && node.parent?.kind !== 19)
          throw new TypeError('styled Item must be a direct Accordion child');
        for (const child of node.childrenValue) pending.push(child);
      }
    },
    mount(app, node) {
      for (const [state, r] of node[TITLES] ?? []) applyTitle(app, node, state, r);
      for (const [layer, r] of node[HEADINGS] ?? [])
        if (r) applyOptional(app, node, r, layer);
      if (node[DIVIDER]) applyOptional(app, node, node[DIVIDER], 0, true);
      if (node[CONFIG]) apply(app, node, 1, node[CONFIG]);
      if (node[STYLE]) apply(app, node, 2, node[STYLE]);
    },
  };
};
