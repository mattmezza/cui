// SPDX-License-Identifier: MIT
const assert = require('node:assert/strict');
const { run } = require('../build/cui.node');
let observed = 0;
const callback = count => { observed = count; console.log(`Node count: ${count}`); };
const native = process.argv[2];
assert.equal(native ? run(callback, native, Number(process.argv[3] || 0)) : run(callback), 0);
if (!native) {
  assert.equal(observed, 2);
  assert.throws(() => run(), /callback/);
  assert.throws(() => run(() => { throw new Error('callback sentinel'); }), /callback sentinel/);
  assert.equal(run(callback), 0);
}
