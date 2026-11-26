// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, column, kbd, kbdStroke} = require('..');
const node = kbdStroke('save', 'é', {modifiers:10, plain:true, outline:true, macSymbols:true, accessibleName:'Save'});
assert.deepEqual(node.kbdStrokeValue, [10, 7, 'é', 'Save']);
kbdStroke('limit', 'é'.repeat(64), {accessibleName:'é'.repeat(512)});
for (const key of ['', 'a\nb', 'a\rb', 'a\tb', '\0', '\u2028', 'é'.repeat(65), '\ud800'])
  assert.throws(() => kbdStroke('bad', key));
for (const modifiers of [-1, 16, true, 1.5]) assert.throws(() => kbdStroke('bad', 'K', {modifiers}));
assert.throws(() => kbdStroke('bad', 'K', {plain:1}));
assert.throws(() => kbdStroke('bad', 'K', {accessibleName:'x'.repeat(1025)}));
const missing = new App({backend:'headless'});
missing.supportsKbdStroke = () => false;
assert.throws(() => missing.mount(column().children(node)), /cui_bridge_set_kbd_stroke_v1/);
assert(!node.owner);
missing.mount(column().children(kbd('legacy', 'Ctrl+K')));
missing.step(); missing.dispose();
const app = new App({backend:'headless'});
try {
  const supported = app.supportsKbdStroke();
  if (process.env.CUI_EXPECT_KBD_STROKE !== undefined)
    assert.equal(supported, process.env.CUI_EXPECT_KBD_STROKE === '1');
  const hint = kbdStroke('typed', 'K', {modifiers:10});
  if (!supported)
    assert.throws(() => app.mount(column().children(hint)), /cui_bridge_set_kbd_stroke_v1/);
  const mounted = supported ? hint : kbd('legacy', 'Ctrl+K');
  app.mount(column().children(mounted));
  app.step();
  assert(mounted.bounds().width > 0);
} finally {
  app.dispose();
}
