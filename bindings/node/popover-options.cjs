// SPDX-License-Identifier: MIT
'use strict';

const CONFIG = Symbol('Popover options');
const booleanOptions = [
  'arrow', 'hide_close', 'keep_pointer_focus', 'focus_content',
  'no_escape_dismiss', 'no_outside_dismiss',
];
const numberOptions = ['width', 'height', 'margin', 'offset', 'reference_rem'];
const allowedOptions = new Set(['anchor', ...booleanOptions, ...numberOptions]);

module.exports = function (native, Node, handle) {
  function supports(app) {
    app._alive();
    return typeof native.popoverOptions === 'function' && native.popoverOptions(app.handle, 0);
  }

  function requireSupport(app) {
    if (!supports(app)) throw new Error('Popover options require a compatible bridge');
  }

  function apply(app, node, config) {
    requireSupport(app);
    if (config === null) {
      native.popoverOptions(app.handle, 2, node[handle]);
    } else {
      native.popoverOptions(app.handle, 1, node[handle], ...config);
    }
  }

  Node.prototype.popover_options = function (options) {
    if (this.kind !== 27) throw new TypeError('popover_options requires Popover');
    let config = null;
    if (options !== null) {
      if (!options || typeof options !== 'object' || Array.isArray(options)) {
        throw new TypeError('expected Popover options object');
      }
      for (const key of Object.keys(options)) {
        if (!allowedOptions.has(key)) throw new TypeError('unknown Popover option');
      }
      const { anchor = 0 } = options;
      if (!Number.isInteger(anchor) || anchor < 0 || anchor > 7) {
        throw new RangeError('anchor must be an integer in 0..7');
      }
      let flags = Object.hasOwn(options, 'offset') ? 1 << 6 : 0;
      for (const [bit, name] of booleanOptions.entries()) {
        const value = options[name] === undefined ? false : options[name];
        if (typeof value !== 'boolean') throw new TypeError(`${name} must be boolean`);
        if (value) flags |= 1 << bit;
      }
      const dimensions = numberOptions.map(name => {
        const value = options[name] === undefined ? 0 : options[name];
        const lower = name === 'offset' ? -4096 : 0;
        const upper = name === 'reference_rem' ? 256 : 4096;
        if (typeof value !== 'number' || !(value >= lower && value <= upper)) {
          throw new RangeError(`${name} out of range`);
        }
        return value;
      });
      config = [anchor, flags, ...dimensions];
    }
    if (this.owner) apply(this.owner, this, config);
    this[CONFIG] = config;
    return this;
  };

  function preflight(app, node) {
    if (node[CONFIG]) requireSupport(app);
    for (const child of node.childrenValue) preflight(app, child);
  }

  function mount(app, node) {
    if (node[CONFIG]) apply(app, node, node[CONFIG]);
  }

  return { supports, preflight, mount };
};
