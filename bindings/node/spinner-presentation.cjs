// SPDX-License-Identifier: MIT
'use strict';

const sizes = ['custom', 'xsmall', 'small', 'medium', 'large'];
const easings = ['quadratic', 'linear'];
const CONFIG = Symbol('Spinner configuration');

function bridgeFloat(value, limit, name) {
  if (!Number.isFinite(value) || value < 0 || value > limit)
    throw new RangeError(`${name} must be a number in [0, ${limit}]`);
  const rounded = Math.fround(value);
  if (value !== 0 && rounded === 0)
    throw new RangeError(`${name} is too small for a bridge float`);
  return rounded;
}

function configValue(options) {
  if (!options || typeof options !== 'object' || Array.isArray(options))
    throw new TypeError('expected a Spinner configuration object');
  const {size = 'medium', pixels = 0, remSize = 0, duration = 0, rgba = null,
    easing = 'quadratic', icon = '', reducedMotion = false} = options;
  if (!sizes.includes(size) || !easings.includes(easing)) throw new TypeError('invalid Spinner size or easing');
  if (typeof reducedMotion !== 'boolean') throw new TypeError('reducedMotion must be boolean');
  if (rgba !== null && (!Number.isInteger(rgba) || rgba < 0 || rgba > 0xFFFFFFFF))
    throw new RangeError('rgba must be null or numeric 0xRRGGBBAA');
  if (typeof icon !== 'string' || icon.includes('\0') || Buffer.byteLength(icon, 'utf8') > 63)
    throw new TypeError('icon must be an embedded catalog key of at most 63 UTF-8 bytes without NUL');
  const config = {size, pixels: bridgeFloat(pixels, 4096, 'pixels'),
    remSize: bridgeFloat(remSize, 4096, 'remSize'), duration: bridgeFloat(duration, 60, 'duration'),
    rgba, easing, icon, reducedMotion};
  if (size === 'custom' ? !config.pixels || config.remSize !== 0 :
      config.pixels !== 0 || Math.fround((config.remSize || 16) * [0, .75, .875, 1, 1.5][sizes.indexOf(size)]) > 4096)
    throw new RangeError('custom requires positive pixels and remSize=0; named size requires pixels=0 and resolved size <=4096');
  return Object.freeze(config);
}

module.exports = function spinnerModule(native, Node, NODE, icons) {
  function supports(app) {
    app._alive();
    return typeof native.spinnerPresentation === 'function' &&
      native.spinnerPresentation(app.handle, 0) === true;
  }

  function call(app, op, node, ...args) {
    app._alive();
    if (typeof native.spinnerPresentation !== 'function')
      throw new Error('rotating Spinner V1 requires a complete, compatible bridge symbol group');
    return native.spinnerPresentation(app.handle, op, node[NODE], ...args);
  }

  function configure(app, node, c) {
    const flags = (c.rgba !== null ? 1 : 0) | (c.reducedMotion ? 2 : 0);
    call(app, 1, node, flags, sizes.indexOf(c.size), c.pixels, c.remSize,
      c.duration, c.rgba ?? 0, easings.indexOf(c.easing), c.icon);
  }

  class RotatingSpinner extends Node {
    constructor(key, config = {}) {
      super(36);
      if (key !== undefined) this.key(key);
      this[CONFIG] = configValue(config);
    }

    configure(options) {
      const config = configValue(options);
      if (this.owner) configure(this.owner, this, config);
      this[CONFIG] = config;
      return this;
    }

    set_icon_resource(resource) {
      if (!this.owner) throw new Error('rotating Spinner is not mounted');
      if (!supportsResource(this.owner))
        throw new Error('Spinner Icon resource V1 requires a complete, compatible bridge symbol group');
      const handle = resource === null ? null : icons.liveHandle(resource, this.owner);
      native.spinnerIconResource(this.owner.handle, 1, this[NODE], handle);
      return this;
    }

    get_config() {
      if (!this.owner) return this[CONFIG];
      const raw = call(this.owner, 2, this);
      if (!sizes[raw.size] || !easings[raw.easing] || !Number.isInteger(raw.flags) || raw.flags < 0 || raw.flags > 3 ||
          (!(raw.flags & 1) && raw.rgba !== 0)) throw new TypeError('invalid copied Spinner configuration');
      return configValue({...raw, size: sizes[raw.size], easing: easings[raw.easing],
        rgba: raw.flags & 1 ? raw.rgba : null, reducedMotion: Boolean(raw.flags & 2)});
    }

    presentation() {
      if (!this.owner) throw new Error('rotating Spinner is not mounted');
      const result = call(this.owner, 3, this);
      if (result === undefined) return undefined;
      if (typeof result.frameSequence !== 'bigint' || result.frameSequence <= 0n ||
          result.frameSequence > 0xFFFFFFFFFFFFFFFFn || !Number.isFinite(result.coreTime) ||
          result.coreTime < 0 || !Number.isFinite(result.nextWakeSeconds) ||
          (result.nextWakeSeconds !== 0 && result.nextWakeSeconds <= result.coreTime) ||
          !(result.phase >= 0 && result.phase <= 1))
        throw new TypeError('invalid copied Spinner presentation');
      return Object.freeze(result);
    }

    visual_size() {
      throw new TypeError('use Spinner configuration to size a rotating Spinner');
    }
  }

  function rotating_spinner(key, config) {
    return new RotatingSpinner(key, config);
  }

  function preflight(app, root) {
    const pending = [root];
    while (pending.length > 0) {
      const node = pending.pop();
      if (node instanceof RotatingSpinner) {
        if (!supports(app))
          throw new Error('rotating Spinner V1 requires a complete, compatible bridge symbol group');
        return;
      }
      for (const child of node.childrenValue) pending.push(child);
    }
  }

  function mount(app, node) {
    if (node instanceof RotatingSpinner) configure(app, node, node[CONFIG]);
  }

  function supportsResource(app) {
    app._alive();
    return supports(app) && icons.supports(app) &&
      typeof native.spinnerIconResource === 'function' && native.spinnerIconResource(app.handle, 0);
  }

  return {supportsResource, exports: {RotatingSpinner, rotating_spinner}, supports, preflight, mount};
};
