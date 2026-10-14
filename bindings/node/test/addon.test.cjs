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
  const chart=native.makeNode(app,root,69,'chart');
  native.setChart(app,chart,[['valid',5]]);
  assert.throws(()=>native.setChart(app,chart,[['allocated',2],['bad\u0000label',3]]),/NUL/);
  assert.throws(()=>native.setChart(app,chart,Array(65).fill(['many',1])),/array length/);
  assert.throws(()=>native.setChart(app,chart,[['wrong']]),/label,value/);
  assert.throws(()=>native.setChartKind(app,chart,0xffffffff),/invalid argument/);
  assert.deepEqual(native.getChart(app,chart,0),['valid',5]);
  assert.throws(()=>native.getChart(other,chart,0),/another app/);
  const plot=native.makeNode(app,root,72,'plot');
  native.setPlot(app,plot,[[20,30]]);
  assert.throws(()=>native.setPlot(app,plot,[[0,0],[Infinity,5]]),/finite number/);
  assert.deepEqual(native.getPlot(app,plot,0),[20,30]);
  const table=native.makeNode(app,root,71,'data-table');
  native.setMatrix(app,table,['Name'],[['Alex']]);
  assert.throws(()=>native.setMatrix(app,table,['Name'],[['allocated'],['bad\u0000cell']]),/NUL/);
  assert.throws(()=>native.setMatrix(app,table,['Name'],[['extra','cell']]),/match headers/);
  assert.throws(()=>native.setMatrix(app,table,Array(17).fill('header'),[]),/array length/);
  assert.equal(native.getMatrixCell(app,table,0,0),'Alex');
  assert.throws(()=>native.setSelectionMode(app,table,0xffffffff),/invalid argument/);
  native.removeNode(app,chart);
  assert.throws(()=>native.getChart(app,chart,0),/stale handle/);
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
