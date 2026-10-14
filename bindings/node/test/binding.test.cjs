// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, row, column, label, button} = require('..');

async function main() {
  let clicks = 0;
  const status = label('Ready');
  const save = button('save').primary().rounded_md().medium().hover_color('#4466cc')
    .label('Save changes').on_click(() => { clicks++; status.label(`Saved ${clicks}`); });
  const defaultSize = button('default-size').label('Equal');
  const explicitBase = button('explicit-base').base().label('Equal');
  const sizeApp = new App({width: 480, height: 240});
  sizeApp.mount(row().children(defaultSize, explicitBase));
  sizeApp.step();
  const defaultBounds = defaultSize.bounds();
  const baseBounds = explicitBase.bounds();
  assert.equal(defaultBounds.width, baseBounds.width, 'default width equals explicit BASE');
  assert.equal(defaultBounds.height, baseBounds.height, 'default height equals explicit BASE');
  sizeApp.dispose();
  const app = new App({width: 480, height: 240});
  app.mount(column().gap_units(2).children(row().gap_px(8).children(status, save)));
  app.step();
  const b = save.bounds();
  assert.ok(b.width > 50 && b.height > 20 && b.x > 0 && b.y > 0, 'CUI computed button geometry');
  app.pointer('move', b.x + 3, b.y + 3).pointer('down', b.x + 3, b.y + 3).pointer('up', b.x + 3, b.y + 3);
  assert.equal(app.step(), 1);
  assert.equal(clicks, 1);
  assert.ok(status.bounds().width > 0, 'callback mutation is accepted by native scene');
  app.step();
  app.key('down', 'tab').key('up', 'tab').key('down', 'enter').key('up', 'enter');
  assert.equal(app.step(), 1, 'keyboard activation reaches native button');
  assert.equal(clicks, 2);
  save.disabled();
  app.pointer('move', b.x + 3, b.y + 3).pointer('down', b.x + 3, b.y + 3).pointer('up', b.x + 3, b.y + 3);
  assert.equal(app.step(), 0, 'disabled button does not click');
  assert.equal(clicks, 2);
  assert.throws(() => app.mount(row()), /already has a root/);
  assert.throws(() => save.children(label('x')), /only row and column/);
  assert.throws(() => save.hover_color('bad'), /color/);
  const other = new App();
  assert.throws(() => other.mount(row().children(save)), /unattached/);
  other.dispose();
  app.dispose();
  assert.throws(() => app.step(), /disposed/);
  assert.throws(() => save.bounds(), /disposed/);
  assert.throws(() => save.label('late'), /disposed/);

  const staleApp = new App();
  const staleButton = button('old').label('Old');
  staleApp.mount(row().children(staleButton)); staleApp.step();
  staleButton.remove();
  assert.throws(() => staleButton.bounds(), /not mounted/);
  staleApp.dispose();

  const timerApp = new App(); timerApp.mount(row().children(button('timer').label('Timer')));
  let timerRan = false;
  timerApp.start(120);
  await new Promise(resolve => setTimeout(() => { timerRan = true; resolve(); }, 40));
  assert.equal(timerRan, true, 'native frame scheduling leaves Node event loop responsive');
  timerApp.stop().dispose();
  if (global.gc) { global.gc(); await new Promise(resolve => setImmediate(resolve)); }
  console.log('Node binding tests passed');
}
main().catch(error => { console.error(error); process.exitCode = 1; });
