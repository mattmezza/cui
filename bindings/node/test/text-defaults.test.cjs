'use strict';
// Pure authoring and legacy addon compatibility; no native dependencies or windows.
const assert = require('node:assert/strict');
const Module = require('node:module');
const original = Module._load;
Module._load = function(name, ...args) {
  if (name.endsWith('cui.node')) return {};
  return original.call(this, name, ...args);
};
const {App, row, label} = require('../index.cjs');
Module._load = original;
const node = row().textDefaults(24, 0);
assert.deepEqual(node.textDefaultsValue, [24, 0, 1]);
assert.equal(node.textDefaults(null, 0x11223344), node);
for (const size of [NaN, Infinity, -1, 4097, true, 1e-100]) {
  assert.throws(() => node.textDefaults(size), /size/);
  assert.deepEqual(node.textDefaultsValue, [0, 0x11223344, 1]);
}
assert.throws(() => label('no').textDefaults(20), /Row or Column/);
const app = Object.create(App.prototype);
app._alive = () => {};
assert.equal(app.supports_text_defaults(), false);
node.owner = app;
assert.throws(() => node.textDefaults(), /unavailable/);
assert.deepEqual(node.textDefaultsValue, [0, 0x11223344, 1]);
node.owner = null;
assert.equal(node.textDefaults(), node);
assert.deepEqual(node.textDefaultsValue, [0, 0, 0]);
console.log('text defaults offline tests passed');
