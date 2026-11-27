'use strict';
module.exports = (native, Node, handle) => {
  const roles = ['header', 'title', 'description', 'content', 'body', 'footer'];
  const actions = ['none', 'confirm', 'cancel', 'close'];
  const flags = ['centered', 'no_escape_dismiss', 'no_outside_dismiss', 'hide_close', 'show_close', 'outside_dismiss'];
  const dimensions = ['width', 'height', 'margin'];
  const optionNames = new Set([...flags, ...dimensions]);
  const isDialog = node => node.kind === 24 || node.kind === 25;
  const footerRole = roles.indexOf('footer') + 1;

  function supports(app) {
    app._alive();
    return typeof native.dialogOptions === 'function' && native.dialogOptions(app.handle, 0);
  }
  function requireSupport(app) {
    if (!supports(app)) throw new Error('Dialog composition requires a compatible bridge');
  }
  function apply(app, node, config) {
    requireSupport(app);
    if (config) native.dialogOptions(app.handle, 1, node[handle], ...config);
    else native.dialogOptions(app.handle, 2, node[handle]);
  }
  function wireConfig(node, options) {
    if (!options || typeof options !== 'object' || Array.isArray(options)) {
      throw new TypeError('expected Dialog options');
    }
    for (const key of Object.keys(options)) {
      if (!optionNames.has(key)) throw new TypeError(`unknown Dialog option ${key}`);
    }
    let mask = 0;
    for (const [bit, key] of flags.entries()) {
      const enabled = options[key] === undefined ? false : options[key];
      if (typeof enabled !== 'boolean') throw new TypeError(`${key} must be boolean`);
      if (enabled) mask |= 1 << bit;
    }
    if (node.kind === 24 && (options.show_close || options.outside_dismiss)) {
      throw new RangeError('show_close and outside_dismiss require AlertDialog');
    }
    const values = dimensions.map(key => {
      const value = options[key] === undefined ? 0 : options[key];
      if (typeof value !== 'number' || !(value >= 0 && value <= 4096)) {
        throw new RangeError(`${key} out of range`);
      }
      return value;
    });
    return [mask, ...values];
  }
  Node.prototype.dialog_options = function (options) {
    if (!isDialog(this)) throw new TypeError('dialog_options requires Dialog or AlertDialog');
    const config = options === null ? null : wireConfig(this, options);
    if (this.owner) apply(this.owner, this, config);
    this.dialogConfig = config;
    return this;
  };
  Node.prototype.dialog_action = function (value) {
    const action = actions.indexOf(value);
    if (this.kind !== 4 || action < 0) throw new TypeError('dialog_action requires Button and known action');
    if (this.owner) {
      requireSupport(this.owner);
      native.dialogOptions(this.owner.handle, 3, this[handle], action);
    }
    this.dialogAction = action;
    return this;
  };
  function slot(key, role) {
    const index = roles.indexOf(role);
    if (index < 0) throw new TypeError('unknown Dialog slot role');
    const node = new Node(role === 'footer' ? 1 : 2).key(key);
    node.dialogSlot = index + 1;
    return node;
  }
  function validateChildren(owner, children) {
    const all = [...owner.childrenValue, ...children];
    const slots = all.filter(node => node?.dialogSlot).map(node => node.dialogSlot);
    if (slots.length && !isDialog(owner)) throw new TypeError('Dialog slots require a direct Dialog owner');
    if (owner.kind === 25 && slots.length !== all.length) throw new TypeError('AlertDialog children must be dialog slots');
    if (new Set(slots).size !== slots.length) throw new TypeError('duplicate Dialog slot role');
  }
  function preflight(app, node, parent = null) {
    if (node.dialogSlot && (!parent?.dialogConfig || !isDialog(parent))) {
      throw new TypeError('Dialog slots require a configured direct Dialog or AlertDialog parent');
    }
    if (node.dialogAction && parent?.dialogSlot !== footerRole) {
      throw new TypeError('Dialog actions require a direct Footer Button');
    }
    if (node.dialogConfig) requireSupport(app);
    for (const child of node.childrenValue) preflight(app, child, node);
  }
  function mount(app, node) {
    if (node.dialogConfig) apply(app, node, node.dialogConfig);
    if (node.dialogAction) native.dialogOptions(app.handle, 3, node[handle], node.dialogAction);
  }
  return { supports, slot, validateChildren, preflight, mount };
};
