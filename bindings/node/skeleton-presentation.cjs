// SPDX-License-Identifier: MIT
'use strict';

const widthModes = ['fixed', 'fraction', 'grow'];
const CONFIG = Symbol('Skeleton configuration');

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
    throw new TypeError('expected a Skeleton configuration object');
  const {widthMode = 'fraction', width = 1, height = 16, maxWidth = 0, radius = 0,
    duration = 0, backgroundRgba = null, secondary = false, reducedMotion = false} = options;
  const mode = widthModes.indexOf(widthMode);
  if (mode < 0) throw new TypeError('widthMode must be fixed, fraction or grow');
  if (typeof secondary !== 'boolean' || typeof reducedMotion !== 'boolean')
    throw new TypeError('secondary and reducedMotion must be boolean');
  if (backgroundRgba !== null &&
      (!Number.isInteger(backgroundRgba) || backgroundRgba < 0 || backgroundRgba > 0xFFFFFFFF))
    throw new RangeError('backgroundRgba must be null or a numeric 0xRRGGBBAA color');

  const config = {
    widthMode, width: bridgeFloat(width, [16384, 1, 0][mode], 'width'),
    height: bridgeFloat(height, 16384, 'height'), maxWidth: bridgeFloat(maxWidth, 16384, 'maxWidth'),
    radius: bridgeFloat(radius, 16384, 'radius'), duration: bridgeFloat(duration, 60, 'duration'),
    backgroundRgba, secondary, reducedMotion,
  };
  if (!config.height || (mode !== 2 && !config.width))
    throw new RangeError('height and fixed/fractional widths must be positive');
  if (mode === 1 && config.maxWidth !== 0)
    throw new RangeError('fractional width requires maxWidth=0');
  return Object.freeze(config);
}

module.exports = function skeletonModule(native, Node, NODE) {
  function supports(app) {
    app._alive();
    return typeof native.skeletonPresentation === 'function' &&
      native.skeletonPresentation(app.handle, 0) === true;
  }

  function call(app, op, node, ...args) {
    app._alive();
    if (typeof native.skeletonPresentation !== 'function')
      throw new Error('responsive Skeleton V1 requires a complete, compatible bridge symbol group');
    return native.skeletonPresentation(app.handle, op, node[NODE], ...args);
  }

  function configure(app, node, c) {
    const flags = (c.secondary ? 1 : 0) | (c.backgroundRgba !== null ? 2 : 0) |
      (c.reducedMotion ? 4 : 0);
    call(app, 1, node, flags, widthModes.indexOf(c.widthMode), c.width, c.height, c.maxWidth,
      c.radius, c.duration, c.backgroundRgba ?? 0);
  }

  class ResponsiveSkeleton extends Node {
    constructor(key, config = {}) {
      super(35);
      if (key !== undefined) this.key(key);
      this[CONFIG] = configValue(config);
    }

    configure(options) {
      const config = configValue(options);
      if (this.owner) configure(this.owner, this, config);
      this[CONFIG] = config;
      return this;
    }

    get_config() {
      if (!this.owner) return this[CONFIG];
      const raw = call(this.owner, 2, this);
      const widthMode = widthModes[raw.widthMode];
      if (!widthMode || !Number.isInteger(raw.flags) || raw.flags < 0 || raw.flags > 7 ||
          (!(raw.flags & 2) && raw.backgroundRgba !== 0))
        throw new TypeError('invalid copied Skeleton configuration');
      return configValue({...raw, widthMode, backgroundRgba: raw.flags & 2 ? raw.backgroundRgba : null,
        secondary: Boolean(raw.flags & 1), reducedMotion: Boolean(raw.flags & 4)});
    }

    presentation() {
      if (!this.owner) throw new Error('responsive Skeleton is not mounted');
      const result = call(this.owner, 3, this);
      if (result === undefined) return undefined;
      if (typeof result.frameSequence !== 'bigint' || result.frameSequence <= 0n ||
          result.frameSequence > 0xFFFFFFFFFFFFFFFFn || !Number.isFinite(result.coreTime) ||
          result.coreTime < 0 || !Number.isFinite(result.nextWakeSeconds) ||
          (result.nextWakeSeconds !== 0 && result.nextWakeSeconds <= result.coreTime) ||
          !(result.phase >= 0 && result.phase <= 1) || !(result.opacity >= .5 && result.opacity <= 1))
        throw new TypeError('invalid copied Skeleton presentation');
      return Object.freeze(result);
    }

    visual_size() {
      throw new TypeError('use Skeleton configuration to size a responsive Skeleton');
    }
  }

  function responsive_skeleton(key, config) {
    return new ResponsiveSkeleton(key, config);
  }

  function preflight(app, root) {
    const pending = [root];
    while (pending.length > 0) {
      const node = pending.pop();
      if (node instanceof ResponsiveSkeleton) {
        if (!supports(app))
          throw new Error('responsive Skeleton V1 requires a complete, compatible bridge symbol group');
        return;
      }
      for (const child of node.childrenValue) pending.push(child);
    }
  }

  function mount(app, node) {
    if (node instanceof ResponsiveSkeleton) configure(app, node, node[CONFIG]);
  }

  return {exports: {ResponsiveSkeleton, responsive_skeleton}, supports, preflight, mount};
};
