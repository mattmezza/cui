// SPDX-License-Identifier: MIT
'use strict';
module.exports = function(native, Node, key, statuses) {
  const actions = Symbol('Attachment actions');
  const config = Symbol('Attachment configuration');
  const preview = Symbol('Attachment preview');
  const little = require('node:os').endianness() === 'LE';
  const uint = (value, max = 0xffffffff) => Number.isInteger(value) && value >= 0 && value <= max;
  const num = (value, lo, hi) => typeof value === 'number' && Number.isFinite(value) && value >= lo && value <= hi;

  function target(node) {
    if (node.kind !== 52) throw new TypeError('requires Attachment');
    if (node.owner) node.owner._alive();
    if (node.disposed) throw new Error('Widget belongs to disposed App');
  }

  function supports(app, pixels = false) {
    app._alive();
    return typeof native.attachmentOptions === 'function' &&
      native.attachmentOptions(app.handle, pixels ? 1 : 0);
  }

  function apply(node, value, pixels = false) {
    if (!supports(node.owner, pixels)) throw new Error('Attachment requires optional bridge symbol');
    const operation = pixels ? (value ? 4 : 5) : (value ? 2 : 3);
    const args = value ? (pixels ? value : [value]) : [];
    native.attachmentOptions(node.owner.handle, operation, node[key], ...args);
  }

  function wire(options) {
    if (!options || typeof options !== 'object' || Array.isArray(options)) {
      throw new TypeError('expected AttachmentOptions');
    }
    const buffer = Buffer.alloc(80);
    const uint32 = (value, offset) => little
      ? buffer.writeUInt32LE(value, offset) : buffer.writeUInt32BE(value, offset);
    const float32 = (value, offset) => little
      ? buffer.writeFloatLE(value, offset) : buffer.writeFloatBE(value, offset);
    uint32(80, 0);
    uint32(1, 4);
    let flags = 0;
    ['vertical', 'media', 'indeterminate'].forEach((name, index) => {
      const value = options[name] ?? false;
      if (typeof value !== 'boolean') throw new TypeError('Attachment flags must be boolean');
      if (value) flags |= 1 << index;
    });
    const aspect = options.mediaAspect ?? 0;
    if (!num(aspect, 0, 16) || (aspect !== 0 && aspect < 1 / 16)) {
      throw new RangeError('invalid media aspect');
    }
    float32(aspect, 16);
    ['titleStatus', 'descriptionStatus'].forEach((name, index) => {
      const value = options[name];
      if (value != null) {
        if (typeof value !== 'string' || !Object.hasOwn(statuses, value)) {
          throw new RangeError('invalid Attachment status');
        }
        flags |= 1 << (index + 3);
        uint32(statuses[value], 20 + index * 4);
      }
    });
    uint32(flags, 8);
    ['title', 'description'].forEach((name, index) => {
      const text = options[name];
      if (text == null) return;
      if (typeof text !== 'object' || Array.isArray(text)) {
        throw new TypeError('expected AttachmentTextStyle');
      }
      const offset = 32 + index * 24;
      const size = text.fontSize ?? 0;
      const tracking = text.letterSpacing ?? 0;
      const weight = text.weight ?? 0;
      if (!num(size, 0, 4096) || !num(tracking, -4096, 4096) || !uint(weight, 3)) {
        throw new RangeError('invalid Attachment typography');
      }
      for (const name of ['italic', 'monospace']) {
        if (text[name] != null && typeof text[name] !== 'boolean') {
          throw new TypeError('text flags must be boolean');
        }
      }
      let flags = text.weight != null || tracking !== 0 || text.italic || text.monospace ? 1 : 0;
      if (text.colorRgba != null) {
        if (!uint(text.colorRgba)) throw new RangeError('invalid RGBA');
        flags |= 2;
        uint32(text.colorRgba, offset + 16);
      }
      if (text.italic) flags |= 4;
      if (text.monospace) flags |= 8;
      uint32(flags, offset);
      uint32(weight, offset + 4);
      float32(size, offset + 8);
      float32(tracking, offset + 12);
    });
    return buffer;
  }

  function singleLine(text) {
    if (typeof text !== 'string' || /[\r\n]/.test(text)) {
      throw new TypeError('configured Attachment labels must be single-line');
    }
  }

  for (const name of ['label', 'detail']) {
    const original = Node.prototype[name];
    Node.prototype[name] = function(text) {
      if (this.kind === 52 && this[config]) singleLine(text);
      return original.call(this, text);
    };
  }

  Node.prototype.attachment_options = function(options) {
    target(this);
    const value = options === null ? null : wire(options);
    if (value) {
      singleLine(this.textValue);
      singleLine(this.detailValue);
    }
    if (this.owner) apply(this, value);
    this[config] = value;
    return this;
  };

  Node.prototype.attachment_preview_rgba = function(pixels, width, height, stride = width * 4) {
    target(this);
    if (!(pixels instanceof Uint8Array || pixels instanceof Uint8ClampedArray)) {
      throw new TypeError('preview pixels require Uint8Array');
    }
    if (!uint(width, 2048) || !width || !uint(height, 2048) || !height || !uint(stride) ||
        stride < width * 4 || stride * height > 16777216 || pixels.byteLength !== stride * height) {
      throw new RangeError('invalid preview dimensions/byte count');
    }
    if (this.owner) {
      // Copy shared bytes on the JS side before the bridge reads native memory.
      const source = pixels.buffer instanceof SharedArrayBuffer ? Buffer.from(pixels) : pixels;
      apply(this, [source, width, height, stride], true);
      this[preview] = null;
    } else {
      this[preview] = [Buffer.from(pixels), width, height, stride];
    }
    return this;
  };

  Node.prototype.clear_attachment_preview = function() {
    target(this);
    if (this.owner) apply(this, null, true);
    this[preview] = null;
    return this;
  };

  Node.prototype.attachment_actions = function(options = {}) {
    target(this);
    if (!options || typeof options !== 'object' || Array.isArray(options) ||
        Object.keys(options).some(key => !['open','remove','retry'].includes(key))) throw new TypeError('invalid Attachment actions');
    let mask = 0;
    ['open','remove','retry'].forEach((name, index) => {
      const value = Object.hasOwn(options,name) ? options[name] : false;
      if (typeof value !== 'boolean') throw new TypeError('Attachment choices must be boolean');
      if (value) mask |= 1 << index;
    });
    if (this.owner) {
      if (!this.owner.supports_attachment_actions()) throw new Error('Attachment actions unavailable');
      native.attachmentActions(this.owner.handle, this[key], mask);
    }
    this[actions] = mask || undefined;
    return this;
  };
  return {
    supports,
    preflight(app, node) {
      if (node[actions] !== undefined && !app.supports_attachment_actions()) throw new Error("Attachment actions unavailable");
      for (const [field, pixels] of [[config, false], [preview, true]]) {
        if (node[field] && !supports(app, pixels)) {
          throw new Error('Attachment requires optional bridge symbol');
        }
      }
      for (const child of node.childrenValue) this.preflight(app, child);
    },
    mount(app, node) {
      if (node[actions] !== undefined) native.attachmentActions(app.handle,node[key],node[actions]);
      if (node[config]) apply(node, node[config]);
      if (node[preview]) apply(node, node[preview], true);
    },
    finishMount(node) {
      // Keep pending pixels intact until the entire mount transaction succeeds.
      if (node.kind === 52) node[preview] = null;
    },
  };
};
