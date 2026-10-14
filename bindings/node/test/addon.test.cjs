// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const native = require(process.env.CUI_NODE_ADDON);
function makeApp() {
  return native.create({backend: 0, width: 320, height: 200, maxNodes: 32, maxEvents: 32});
}
async function main() {
  const app = makeApp();
  const root = native.makeNode(app, null, 1, 'root');
  const label = native.makeNode(app, root, 3, 'label');
  assert.throws(() => native.setGap(app, root, 1e300), /finite number/);
  assert.throws(() => native.create({backend: 0, width: 1e300, height: 200,
    maxNodes: 32, maxEvents: 32}), /finite number/);
  const button = native.makeNode(app, root, 4, 'button');
  assert.throws(() => native.setVariant(app, button, 0xffffffff), /supported range/);
  assert.throws(() => native.setSize(app, button, 0xffffffff), /supported range/);
  assert.throws(() => native.setRounding(app, button, 0xffffffff), /supported range/);
  native.setText(app, label, 'Safe');
  native.step(app, 1 / 60);
  assert.ok(native.bounds(app, label).width > 0);
  assert.throws(() => native.step(root, 1 / 60), /app is disposed or invalid/);
  assert.throws(() => native.dispose(root), /invalid app/);
  assert.throws(() => native.nodeId(app), /invalid node/);
  assert.throws(() => native.bounds(app, app), /node belongs/);
  assert.throws(() => native.makeNode(app, app, 3, 'bad'), /node belongs/);
  const other = makeApp();
  assert.throws(() => native.bounds(other, label), /another app/);
  assert.throws(() => native.makeNode(other, root, 3, 'bad'), /another app/);
  assert.throws(() => native.setText(app, label, 'before\u0000after'), /embedded NUL/);
  assert.throws(() => native.makeNode(app, root, 3, 'before\u0000after'), /embedded NUL/);
  assert.throws(() => native.makeNode(app, root, 3, 'x'.repeat(4097)), /4096/);
  assert.throws(() => native.create({backend: 0, width: 320, height: 200, maxNodes: 32, maxEvents: 32, title: 'bad\u0000title'}), /embedded NUL/);
  native.dispose(other);
  native.dispose(app);
  assert.throws(() => native.bounds(app, label), /disposed/);

  let retained = makeApp();
  const retainedNode = native.makeNode(retained, null, 1, 'retained');
  const weak = new WeakRef(retained);
  retained = null;
  if (global.gc) { global.gc(); await new Promise(resolve => setImmediate(resolve)); global.gc(); }
  const stillLive = weak.deref();
  assert.ok(stillLive, 'native node keeps its app external alive');
  native.step(stillLive, 1 / 60);
  native.dispose(stillLive);
  assert.deepEqual(native.nodeId(retainedNode), {slot: 1, generation: 1});
  console.log('N-API safety tests passed');
}
main().catch(error => { console.error(error); process.exitCode = 1; });
