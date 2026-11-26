// SPDX-License-Identifier: MIT
'use strict';

const CONFIG = Symbol('Rating configuration');
const RATING = 43;
const FLAGS = ['halfSteps', 'hoverPreview', 'stepDownFilled', 'clearable', 'readonly'];
const COLORS = ['fillRgba', 'outlineRgba', 'previewRgba'];
const MASKS = ['outlineIcon', 'fillIcon'];
const UNAVAILABLE = 'Rating options require a complete, compatible bridge symbol group';

module.exports = function ratingOptions(native, Node, handleKey, icons, sizes) {
  const namedSizes = Object.fromEntries(Object.entries(sizes).map(([key, value]) => [value, key]));
  function normalize(options) {
    if (!options || typeof options !== 'object' || Array.isArray(options)) {
      throw new TypeError('expected Rating configuration');
    }
    const result = {};
    for (const name of FLAGS) {
      result[name] = options[name] ?? false;
      if (typeof result[name] !== 'boolean') throw new TypeError('Rating flags must be boolean');
    }
    result.designSize = options.designSize ?? null;
    if (result.designSize !== null &&
        (typeof result.designSize !== 'string' || !Object.hasOwn(sizes, result.designSize))) {
      throw new RangeError('invalid Rating design size');
    }
    if (result.designSize === 'inherit') result.designSize = null;
    const width = options.width ?? 0;
    if (!Number.isFinite(width) || !(width === 0 || width >= 1 && width <= 4096)) {
      throw new RangeError('invalid Rating width');
    }
    result.width = Math.fround(width);
    for (const name of COLORS) {
      const color = options[name] ?? null;
      if (color !== null && (!Number.isInteger(color) || color < 0 || color > 0xffffffff)) {
        throw new RangeError('Rating colors must be uint32');
      }
      result[name] = color;
    }
    for (const name of MASKS) {
      const resource = options[name] ?? null;
      if (resource !== null && !(resource instanceof icons.IconResource)) {
        throw new TypeError('Rating masks must be IconResource');
      }
      result[name] = resource;
    }
    return Object.freeze(result);
  }

  function supports(app) {
    app._alive();
    return typeof native.ratingOptions === 'function' && native.ratingOptions(app.handle, 0);
  }

  function call(app, op, node, ...parameters) {
    app._alive();
    if (typeof native.ratingOptions !== 'function') throw new Error(UNAVAILABLE);
    return native.ratingOptions(app.handle, op, node[handleKey], ...parameters);
  }

  function apply(app, node, config) {
    let bits = 0;
    for (const [index, name] of FLAGS.entries()) {
      if (config[name]) bits |= 1 << index;
    }
    for (const [index, name] of COLORS.entries()) {
      if (config[name] !== null) bits |= 1 << (index + FLAGS.length);
    }
    const designSize = config.designSize === null ? 0 : sizes[config.designSize];
    const colors = COLORS.map(name => config[name] ?? 0);
    const handles = MASKS.map(name => config[name] === null ? null : icons.liveHandle(config[name], app));
    call(app, 1, node, bits, designSize, config.width, ...colors, ...handles);
  }

  const prior = {
    configure: Node.prototype.configure,
    reset: Node.prototype.reset_config,
    get: Node.prototype.get_config,
  };
  Node.prototype.configure = function configure(options) {
    if (this.kind !== RATING) return prior.configure.call(this, options);
    if (options === null) return this.reset_config();
    const config = normalize(options);
    if (this.owner) apply(this.owner, this, config);
    this[CONFIG] = config;
    return this;
  };
  Node.prototype.reset_config = function resetConfig() {
    if (this.kind !== RATING) return prior.reset.call(this);
    if (this.owner) call(this.owner, 3, this);
    delete this[CONFIG];
    return this;
  };
  Node.prototype.get_config = function getConfig() {
    if (this.kind !== RATING) return prior.get.call(this);
    if (!this.owner) return this[CONFIG];
    if (!supports(this.owner)) return undefined;
    const raw = call(this.owner, 2, this);
    if (raw === undefined) return undefined;
    const config = {...this[CONFIG], width: raw.width, designSize: namedSizes[raw.designSize]};
    for (const [index, name] of FLAGS.entries()) {
      config[name] = Boolean(raw.flags & (1 << index));
    }
    for (const [index, name] of COLORS.entries()) {
      config[name] = raw.flags & (1 << (index + FLAGS.length)) ? raw[name] : null;
    }
    return normalize(config);
  };

  return {
    supports,
    preflight(app, root) {
      const pending = [root];
      while (pending.length) {
        const node = pending.pop();
        const config = node[CONFIG];
        if (node.kind === RATING && config) {
          if (!supports(app)) throw new Error(UNAVAILABLE);
          for (const name of MASKS) {
            if (config[name]) icons.liveHandle(config[name], app);
          }
        }
        for (const child of node.childrenValue) pending.push(child);
      }
    },
    mount(app, node) {
      if (node.kind === RATING && node[CONFIG]) apply(app, node, node[CONFIG]);
    },
    release(node) {
      delete node[CONFIG];
    },
  };
};
