// SPDX-License-Identifier: MIT
'use strict';
const CUSTOM = Symbol('Pagination custom size');
const CONFIG = Symbol('Pagination configuration');
const labels = ['group', 'first', 'previous', 'next', 'last', 'earlierPages', 'laterPages'];
const booleans = ['counter', 'iconOnly', 'chevrons', 'jumpMenu'];
function configValue(options) {
  if (!options || typeof options !== 'object' || Array.isArray(options))
    throw new TypeError('expected a Pagination configuration object');
  const config = {};
  for (const name of booleans) {
    config[name] = options[name] === undefined ? false : options[name];
    if (typeof config[name] !== 'boolean') throw new TypeError(`${name} must be boolean`);
  }
  config.visiblePages = options.visiblePages === undefined ? 0 : options.visiblePages;
  if (!Number.isInteger(config.visiblePages) ||
      (config.visiblePages !== 0 && (config.visiblePages < 5 || config.visiblePages > 32)))
    throw new RangeError('visiblePages must be zero or 5..32');
  const width = options.availableWidth === undefined ? 0 : options.availableWidth;
  if (!Number.isFinite(width) || width < 0 || width > 32768 || (width !== 0 && Math.fround(width) === 0))
    throw new RangeError('availableWidth must be zero or a positive bridge float up to 32768');
  config.availableWidth = Math.fround(width);
  for (const name of labels) {
    const value = options[name] === undefined ? '' : options[name];
    if (typeof value !== 'string' || Buffer.byteLength(value, 'utf8') > 127 ||
        /[\0-\x1f\x7f\u0085\u2028\u2029]/u.test(value) ||
        /[\uD800-\uDBFF](?![\uDC00-\uDFFF])|(?<![\uD800-\uDBFF])[\uDC00-\uDFFF]/u.test(value))
      throw new TypeError(`${name} must be single-line UTF-8 up to 127 bytes`);
    config[name] = value;
  }
  return Object.freeze(config);
}
module.exports = function paginationModule(native, Node, NODE) {
  function supportsCustom(app) {
    app._alive();
    return typeof native.paginationCustomSize === 'function' && native.paginationCustomSize(app.handle, 0) === true;
  }
  function customCall(app, op, node, ...args) {
    app._alive();
    if (typeof native.paginationCustomSize !== 'function')
      throw new Error('Pagination custom size requires a complete, compatible bridge symbol group');
    return native.paginationCustomSize(app.handle, op, node[NODE], ...args);
  }
  function supports(app) {
    app._alive();
    return typeof native.paginationOptions === 'function' && native.paginationOptions(app.handle, 0) === true;
  }
  function call(app, op, node, ...args) {
    app._alive();
    if (typeof native.paginationOptions !== 'function')
      throw new Error('Pagination V1 requires a complete, compatible bridge symbol group');
    return native.paginationOptions(app.handle, op, node[NODE], ...args);
  }
  function configure(app, node, config) {
    call(app, 1, node, booleans.reduce((flags, name, i) => flags | (config[name] ? 1 << i : 0), 0),
      config.visiblePages, config.availableWidth, ...labels.map(name => config[name]));
  }
  function paginationNode(node) {
    if (node.kind !== 42) throw new TypeError('Pagination configuration requires Pagination');
  }
  Node.prototype.custom_size = function(value) {
    paginationNode(this);
    if (typeof value !== 'number' || !Number.isFinite(value) || !(value === 0 || value >= 1 && value <= 512))
      throw new RangeError('custom_size must be zero or a number in [1,512]');
    value = Math.fround(value);
    if (this.owner) customCall(this.owner, 1, this, value);
    this[CUSTOM] = value;
    return this;
  };
  Node.prototype.get_custom_size = function() {
    paginationNode(this);
    return this.owner ? customCall(this.owner, 2, this) : this[CUSTOM] ?? 0;
  };
  Node.prototype.configure = function(options) {
    paginationNode(this);
    if (options === null) return this.reset_config();
    const config = configValue(options);
    if (this.owner) configure(this.owner, this, config);
    this[CONFIG] = config;
    return this;
  };
  Node.prototype.reset_config = function() {
    paginationNode(this);
    if (this.owner) call(this.owner, 3, this);
    delete this[CONFIG];
    return this;
  };
  Node.prototype.get_config = function() {
    paginationNode(this);
    if (!this.owner) return this[CONFIG];
    if (this[CONFIG] === undefined && !supports(this.owner)) return undefined;
    const raw = call(this.owner, 2, this);
    if (raw === undefined) return undefined;
    if (!Number.isInteger(raw.flags) || raw.flags < 0 || raw.flags > 15)
      throw new TypeError('invalid copied Pagination flags');
    return configValue({...raw, ...Object.fromEntries(booleans.map((name, i) => [name, Boolean(raw.flags & (1 << i))]))});
  };
  function preflight(app, root) {
    const pending = [root];
    while (pending.length) {
      const node = pending.pop();
      if (node.kind === 42 && (node[CONFIG] !== undefined || node.designSizeValue !== undefined) && !supports(app))
        throw new Error('Pagination V1 requires a complete, compatible bridge symbol group');
      if (node.kind === 42 && node[CUSTOM] && !supportsCustom(app))
        throw new Error('Pagination custom size requires a complete, compatible bridge symbol group');
      pending.push(...node.childrenValue);
    }
  }
  function mount(app, node) {
    if (node[CUSTOM]) customCall(app, 1, node, node[CUSTOM]);
    if (node[CONFIG] !== undefined) configure(app, node, node[CONFIG]);
  }
  return {supports, supportsCustom, preflight, mount};
};
