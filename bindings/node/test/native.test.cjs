// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, row, button} = require('..');
if (process.env.CUI_TEST_VIRTUAL_DISPLAY !== '1' || process.env.WAYLAND_DISPLAY ||
    !process.env.DISPLAY || process.env.DISPLAY === ':0')
  throw new Error('Native proof requires the isolated local test runner');
if (!process.env.CUI_FONT_PATH) throw new Error('CUI_FONT_PATH must name a readable font file');
let clicks = 0;
const action = button('native-proof').primary().label('Native button').on_click(() => clicks++);
const app = new App({backend: 'sdl', width: 400, height: 200,
  title: 'CUI Node native smoke', fontPath: process.env.CUI_FONT_PATH});
try {
  app.mount(row().children(action));
  app.step();
  const b = action.bounds();
  assert.ok(b.width > 0 && b.height > 0, 'SDL frame laid out CUI button');
  app.pointer('move', b.x + 2, b.y + 2)
    .pointer('down', b.x + 2, b.y + 2)
    .pointer('up', b.x + 2, b.y + 2);
  assert.equal(app.step(), 1);
  assert.equal(clicks, 1);
  app.close();
  assert.throws(() => app.step(), /closed/);
} finally {
  app.dispose();
}
console.log('Native SDL smoke passed');
