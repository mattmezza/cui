// SPDX-License-Identifier: MIT
'use strict';

const ICON = {PROBE: 0, CONFIGURE: 1, CONFIG: 2, RESET: 3, RESULT: 4};
const BADGE = {PROBE: 0, CREATE: 1, CONFIGURE: 2, CONFIG: 3, RESULT: 4, BOUNDS: 5};
const ICON_KIND = 50;
const BADGE_KIND = 82;
const REMOVED = Symbol('disposed or removed');
const CONFIG = Symbol('retained configuration');
const FALLBACK = Symbol('embedded fallback');
const ICON_SIZES = ['inherited', 'xsmall', 'small', 'medium', 'large', 'custom'];
const BADGE_SIZES = ['medium', 'small', 'large', 'xsmall'];
const BADGE_VARIANTS = ['count', 'dot', 'icon'];
const ALIGN = ['start', 'center', 'end'];
const STYLE_BOOL = [
  ['row', 0], ['clip', 1], ['fixedWidth', 4], ['fixedHeight', 5], ['layoutOnly', 6],
  ['inert', 7], ['widthPercent', 9], ['heightPercent', 10],
];
const STYLE_NUM = [
  ['width', 16], ['height', 20], ['padding', 24, 0], ['gap', 28, 0], ['radius', 32, 0],
  ['borderWidth', 44, 0], ['offsetX', 56], ['offsetY', 60], ['maxWidth', 96, 0], ['maxHeight', 100, 0],
];
const STYLE_EDGE = [['corners', 64, 2], ['borderEdges', 80, 3], ['paddingEdges', 108, 11]];
const CONSTRAINTS = ['minWidth', 'minHeight', 'maxWidth', 'maxHeight'];

function object(value, name) {
  if (!value || typeof value !== 'object' || Array.isArray(value))
    throw new TypeError(`expected ${name} options`);
  return value;
}

function optionIndex(value, choices, name) {
  const index = choices.indexOf(value);
  if (index < 0) throw new TypeError(`invalid ${name}`);
  return index;
}

function boolean(value, name) {
  if (typeof value !== 'boolean') throw new TypeError(`${name} must be boolean`);
  return value;
}

function uint64(value, name) {
  if (typeof value === 'number' && Number.isSafeInteger(value)) value = BigInt(value);
  if (typeof value !== 'bigint' || value < 0n || value > 0xffffffffffffffffn)
    throw new RangeError(`${name} requires a nonnegative bigint or safe integer`);
  return value;
}

function record(size) {
  const wire = Buffer.alloc(size);
  wire.writeUInt32LE(size, 0);
  wire.writeUInt32LE(1, 4);
  return wire;
}

function icon_rotation(radians) {
  if (!Number.isFinite(radians)) throw new TypeError('radians must be finite');
  const cos = Math.cos(radians);
  const sin = Math.sin(radians);
  return Object.freeze([cos, sin, -sin, cos, 0, 0]);
}

module.exports = function iconBadgeModule(native, Node, NODE, resources, designSizes) {
  const {textBytes, bridgeFloat, liveHandle, uint, IconResource} = resources;
  const {IconAvailability} = resources.exports;

  function text(value) { return textBytes(value, 4096); }
  function rgba(value, name) { return uint(value, 0xffffffff, name); }
  function designIndex(value) {
    if (typeof value !== 'string' || !Object.hasOwn(designSizes, value))
      throw new TypeError('invalid designSize');
    return designSizes[value];
  }

  function array(value, size, name, range) {
    if (!Array.isArray(value) || value.length !== size)
      throw new TypeError(`${name} requires ${size} numbers`);
    return Object.freeze(Array.from(value, (number, index) =>
      bridgeFloat(number, ...range(index), `${name}[${index}]`)));
  }

  function floatArray(wire, at, arrayValue) {
    arrayValue.forEach((value, index) => wire.writeFloatLE(value, at + index * 4));
  }

  function styleInput(input) {
    object(input, 'Icon style');
    const wire = record(128);
    const result = {};
    let flags = 0;
    for (const [name, offset, min = -Infinity] of STYLE_NUM) {
      result[name] = bridgeFloat(input[name] ?? 0, min, Infinity, name);
      wire.writeFloatLE(result[name], offset);
    }
    for (const [name, bit] of STYLE_BOOL) {
      result[name] = boolean(input[name] ?? false, name);
      if (result[name]) flags |= 1 << bit;
    }
    for (const [name, offset, bit] of STYLE_EDGE) {
      const edges = input[name] ?? null;
      result[name] = edges === null ? null : array(edges, 4, name, () => [0, Infinity]);
      if (result[name] !== null) {
        flags |= 1 << bit;
        floatArray(wire, offset, result[name]);
      }
    }
    const {alignX = 'start', alignY = 'start', backgroundRgba = 0, borderRgba = 0,
      opacity = null} = input;
    wire.writeUInt32LE(optionIndex(alignX, ALIGN, 'alignX'), 48);
    wire.writeUInt32LE(optionIndex(alignY, ALIGN, 'alignY'), 52);
    wire.writeUInt32LE(rgba(backgroundRgba, 'backgroundRgba'), 36);
    wire.writeUInt32LE(rgba(borderRgba, 'borderRgba'), 40);
    Object.assign(result, {alignX, alignY, backgroundRgba, borderRgba, opacity: null});
    if (opacity !== null) {
      flags |= 1 << 8;
      result.opacity = bridgeFloat(opacity, -Infinity, Infinity, 'style opacity');
      wire.writeFloatLE(result.opacity, 104);
    }
    wire.writeUInt32LE(flags, 8);
    return {wire, value: Object.freeze(result)};
  }

  function constraintInput(input) {
    object(input, 'Icon constraint');
    const wire = record(32);
    const result = {};
    let flags = 0;
    for (const [index, name] of CONSTRAINTS.entries()) {
      const value = input[name] ?? null;
      result[name] = value === null ? null : bridgeFloat(value, 0, 4096, name);
      if (result[name] !== null) {
        flags |= 1 << index;
        wire.writeFloatLE(result[name], 16 + 4 * index);
      }
    }
    wire.writeUInt32LE(flags, 8);
    return {wire, value: Object.freeze(result)};
  }

  function optionsInput(input) {
    const {size = 'inherited', designSize = 'inherit', customSize = 0, inheritedTextSize = 0,
      width = null, height = null, opacity = null, remSize = 0, colorRgba = null,
      inheritedColorRgba = null, transform = null, decorative = false, style = null,
      constraints = null, debugPlaceholder = false} = object(input, 'Icon');
    const wire = record(256);
    wire.writeUInt32LE(optionIndex(size, ICON_SIZES, 'Icon size'), 12);
    wire.writeUInt32LE(designIndex(designSize), 16);
    const result = {size, designSize};
    const dimensions = [['customSize', customSize, 32], ['inheritedTextSize', inheritedTextSize, 36],
      ['remSize', remSize, 52]];
    for (const [name, value, offset] of dimensions) {
      result[name] = bridgeFloat(value, 0, 4096, name);
      wire.writeFloatLE(result[name], offset);
    }
    let flags = 0;
    for (const [name, value, offset, bit] of [
      ['width', width, 40, 0], ['height', height, 44, 1], ['opacity', opacity, 48, 4],
    ]) {
      const isExtent = name !== 'opacity';
      result[name] = value === null ? null : bridgeFloat(value, isExtent ? 0 : -Infinity,
        isExtent ? 4096 : Infinity, name);
      if (result[name] !== null) {
        flags |= 1 << bit;
        wire.writeFloatLE(result[name], offset);
      }
    }
    for (const [name, value, offset, bit] of [
      ['colorRgba', colorRgba, 56, 2], ['inheritedColorRgba', inheritedColorRgba, 60, 3],
    ]) {
      result[name] = value === null ? null : rgba(value, name);
      if (result[name] !== null) {
        flags |= 1 << bit;
        wire.writeUInt32LE(value, offset);
      }
    }
    result.transform = transform === null ? null : array(transform, 6, 'transform',
      index => index < 4 ? [-64, 64] : [-1000000, 1000000]);
    if (result.transform !== null) {
      flags |= 1 << 5;
      floatArray(wire, 64, result.transform);
    }
    result.decorative = boolean(decorative, 'decorative');
    result.debugPlaceholder = boolean(debugPlaceholder, 'debugPlaceholder');
    if (decorative) flags |= 1 << 6;
    if (debugPlaceholder) flags |= 1 << 9;
    for (const [name, value, convert, offset, bit] of [
      ['style', style, styleInput, 88, 7], ['constraints', constraints, constraintInput, 216, 8],
    ]) {
      result[name] = null;
      if (value !== null) {
        const nested = convert(value);
        flags |= 1 << bit;
        nested.wire.copy(wire, offset);
        result[name] = nested.value;
      }
    }
    wire.writeUInt32LE(flags, 8);
    return {wire, value: Object.freeze(result)};
  }

  function iconConfig(resource, options, name) {
    if (!(resource instanceof IconResource)) throw new TypeError('expected an IconResource');
    const named = text(name);
    const config = optionsInput(options);
    return {iconWire: config.wire, name: named, value: Object.freeze({resource, name, options: config.value})};
  }

  function badgeConfig(input) {
    const {variant = 'count', count = 0n, maximum = 99n, size = 'medium', designSize = 'inherit',
      colorRgba = null, name = '', namedIcon = '', artwork = null,
      artworkOptions = null} = object(input, 'Badge');
    const wire = record(64);
    const sourceCount = Number(namedIcon !== '') + Number(artwork !== null);
    if (variant === 'icon' ? sourceCount !== 1 : sourceCount !== 0)
      throw new TypeError('an Icon Badge requires exactly one named or owned Icon source');
    if (artwork === null && artworkOptions !== null)
      throw new TypeError('artworkOptions requires an owned IconResource');
    if (artwork !== null && !(artwork instanceof IconResource))
      throw new TypeError('expected an IconResource for Badge artwork');
    wire.writeUInt32LE(optionIndex(variant, BADGE_VARIANTS, 'Badge variant'), 16);
    wire.writeUInt32LE(optionIndex(size, BADGE_SIZES, 'Badge size'), 20);
    wire.writeUInt32LE(designIndex(designSize), 24);
    if (colorRgba !== null) {
      wire.writeUInt32LE(1, 28);
      wire.writeUInt32LE(rgba(colorRgba, 'colorRgba'), 48);
    }
    const countValue = uint64(count, 'count');
    const maximumValue = uint64(maximum, 'maximum');
    wire.writeBigUInt64LE(countValue, 32);
    wire.writeBigUInt64LE(maximumValue, 40);
    const options = artwork === null ? null : optionsInput(artworkOptions ?? {});
    return {
      wire, iconWire: options?.wire ?? null, name: text(name), namedIcon: text(namedIcon),
      value: Object.freeze({variant, count: countValue, maximum: maximumValue, size, designSize,
        colorRgba, name, namedIcon, artwork, artworkOptions: options?.value ?? null}),
    };
  }

  function appAlive(app) { resources.appAlive(app); }

  function supported(app, entry) {
    appAlive(app);
    return typeof native[entry] === 'function' && native[entry](app.handle, 0) === true;
  }

  function requireEntry(entry) {
    const route = entry === 'iconNode' ? 'Icon presentation' : 'configured Badge';
    return new Error(`${route} V1 requires a complete, compatible bridge`);
  }

  function call(app, entry, op, ...args) {
    appAlive(app);
    if (typeof native[entry] !== 'function') throw requireEntry(entry);
    return native[entry](app.handle, op, ...args);
  }

  function checkNode(node) {
    if (node[CONFIG] === REMOVED) throw new Error('node was removed or belongs to a closed app');
  }

  function attached(node) {
    checkNode(node);
    if (!node.owner) throw new Error('node is not mounted');
    return node.owner;
  }

  function setIcon(app, node, config) {
    call(app, 'iconNode', ICON.CONFIGURE, node[NODE], liveHandle(config.value.resource, app),
      config.iconWire, config.name);
  }

  function badgeHandle(app, config) {
    return config.value.artwork === null ? null : liveHandle(config.value.artwork, app);
  }

  function badgeArguments(app, config) {
    return [badgeHandle(app, config), config.wire, config.iconWire, config.name, config.namedIcon];
  }

  class ConfiguredIcon extends Node {
    constructor(key, resource, options = {}, {name = '', fallback = null} = {}) {
      if (fallback !== null && !text(fallback).length)
        throw new TypeError('fallback must be a nonempty embedded Icon name');
      super(ICON_KIND, fallback ?? '');
      if (key !== undefined) this.key(key);
      this[CONFIG] = iconConfig(resource, options, name);
      this[FALLBACK] = fallback;
    }

    configure(resource, options = {}, {name = ''} = {}) {
      checkNode(this);
      const next = iconConfig(resource, options, name);
      if (this.owner) setIcon(this.owner, this, next);
      this[CONFIG] = next;
      return this;
    }

    get_config() {
      checkNode(this);
      const config = this[CONFIG];
      if (this.owner) {
        const raw = call(this.owner, 'iconNode', ICON.CONFIG, this[NODE],
          config === null ? null : liveHandle(config.value.resource, this.owner));
        if (config === null && raw === undefined) return undefined;
        if (!config || !raw || !config.iconWire.equals(raw.options) || raw.name !== config.value.name)
          throw new Error('invalid copied Icon configuration');
      }
      return config?.value;
    }

    result() {
      const out = call(attached(this), 'iconNode', ICON.RESULT, this[NODE]);
      return out === undefined ? undefined : Object.freeze(out);
    }

    reset() {
      const app = attached(this);
      if (this[FALLBACK] === null) throw new Error('provide an embedded fallback to reset an Icon');
      call(app, 'iconNode', ICON.RESET, this[NODE]);
      this[CONFIG] = null;
      return this;
    }

    label() { throw new TypeError('set the accessible name in Icon configuration'); }
    visual_size() { throw new TypeError('set the size in Icon options'); }
  }

  class ConfiguredBadge extends Node {
    constructor(key, config = {}) {
      super(BADGE_KIND);
      if (key !== undefined) this.key(key);
      this[CONFIG] = badgeConfig(config);
    }

    configure(config) {
      checkNode(this);
      const next = badgeConfig(config);
      if (this.owner)
        call(this.owner, 'configuredBadge', BADGE.CONFIGURE, this[NODE], ...badgeArguments(this.owner, next));
      this[CONFIG] = next;
      return this;
    }

    get_config() {
      checkNode(this);
      const config = this[CONFIG];
      if (this.owner) {
        const raw = call(this.owner, 'configuredBadge', BADGE.CONFIG, this[NODE], badgeHandle(this.owner, config));
        if (!raw || !config.wire.equals(raw.config) ||
            !(config.iconWire ?? record(256)).equals(raw.options) ||
            raw.name !== config.value.name || raw.namedIcon !== config.value.namedIcon)
          throw new Error('invalid copied Badge configuration');
      }
      return config.value;
    }

    result() {
      const out = call(attached(this), 'configuredBadge', BADGE.RESULT, this[NODE]);
      return out === undefined ? undefined : Object.freeze(out);
    }

    indicator_bounds() {
      const out = call(attached(this), 'configuredBadge', BADGE.BOUNDS, this[NODE]);
      return out === undefined ? undefined : Object.freeze(out);
    }

    disabled(value = true) {
      checkNode(this);
      boolean(value, 'disabled');
      if (this.owner) native.setDisabled(attached(this).handle, this[NODE], Number(value));
      this.disabledValue = value;
      return this;
    }

    label() { throw new TypeError('set the indicator name in Badge configuration'); }
  }

  function preflight(app, root) {
    const pending = [root];
    while (pending.length) {
      const node = pending.pop();
      if (node instanceof ConfiguredIcon) {
        checkNode(node);
        if (!supported(app, 'iconNode')) throw requireEntry('iconNode');
        liveHandle(node[CONFIG].value.resource, app);
        if (node[FALLBACK] !== null && resources.query(app, node[FALLBACK]) !== IconAvailability.EMBEDDED)
          throw new TypeError('fallback must be embedded in the Icon catalog');
      }
      if (node instanceof ConfiguredBadge) {
        checkNode(node);
        if (!supported(app, 'configuredBadge')) throw requireEntry('configuredBadge');
        badgeHandle(app, node[CONFIG]);
      }
      for (const child of node.childrenValue) pending.push(child);
    }
  }

  function createBadge(app, node, parent) {
    return call(app, 'configuredBadge', BADGE.CREATE, parent, node.keyValue, ...badgeArguments(app, node[CONFIG]));
  }

  function mount(app, node) {
    if (node instanceof ConfiguredIcon) setIcon(app, node, node[CONFIG]);
    if (node instanceof ConfiguredBadge && node.disabledValue) node.disabled(true);
  }

  function release(node) {
    if (node instanceof ConfiguredIcon || node instanceof ConfiguredBadge) node[CONFIG] = REMOVED;
  }

  const configured_icon = (...args) => new ConfiguredIcon(...args);
  const configured_badge = (...args) => new ConfiguredBadge(...args);
  return {
    exports: {ConfiguredIcon, ConfiguredBadge, configured_icon, configured_badge, icon_rotation},
    preflight, createBadge, mount, release, supported, badgeKind: BADGE_KIND,
  };
};
