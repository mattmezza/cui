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

const gradient = {stops:[{position:0,color:0xff0000ff},{position:1,color:0x0000ffff}]};
const styled = kbd('styled', 'Ctrl+K').setKbdStyle({paddingX:0,textGradient:gradient});
gradient.stops[0].color=0;
assert.equal(styled.kbdStyleValue.length,144);
assert.equal(styled.kbdStyleValue.readUInt32LE(84),0xff0000ff);
assert.equal(styled.kbdStyleValue.readFloatLE(40),2);
assert.equal(kbd('all').setKbdStyle({maxWidth:200,fill:true,wrap:true,paddingX:0,paddingY:0,
  radius:0,background:0,borderWidth:0,borderColor:0,fontSize:18,lineHeight:0,
  weight:'bold',italic:true,monospace:true,textColor:0}).kbdStyleValue.readUInt32LE(8),8191);
const prior = styled.kbdStyleValue;
for (const style of [{unknown:1},{maxWidth:0},{fontSize:NaN},{fill:1},{borderWidth:1},
  {textColor:-1},{textGradient:{stops:[]}},{textColor:0,textGradient:gradient},
  {textGradient:{...gradient,x1:0}}]) {
  assert.throws(()=>styled.setKbdStyle(style));assert.equal(styled.kbdStyleValue,prior);
}
styled.setKbdStyle();assert.equal(styled.kbdStyleValue,null);
assert.throws(()=>column().setKbdStyle({}));
const styleApp = new App({backend: 'headless'});
try {
  const hint = kbd('style-live', 'Ctrl+K').setKbdStyle({paddingX: 14, paddingY: 8});
  const root = column().children(hint);
  const support = styleApp.supportsKbdStyle.bind(styleApp);
  styleApp.supportsKbdStyle = () => false;
  assert.throws(() => styleApp.mount(root), /cui_bridge_set_kbd_style_v1/);
  assert.equal(hint.owner, null);
  styleApp.supportsKbdStyle = support;
  const supported = support();
  if (process.env.CUI_EXPECT_KBD_STYLE !== undefined)
    assert.equal(supported, process.env.CUI_EXPECT_KBD_STYLE === '1');
  if (!supported) hint.setKbdStyle({});
  styleApp.mount(root);
  styleApp.step();
  const before = hint.bounds();
  if (supported) {
    hint.setKbdStyle({paddingX: 34, paddingY: 18});
    styleApp.step();
    const after = hint.bounds();
    assert(after.width > before.width);
    assert(after.height > before.height);
    hint.setKbdStyle(null);
    styleApp.step();
    assert(hint.bounds().width < after.width);
  } else {
    assert(before.width > 0);
    hint.setKbdStyle(null);
  }
} finally {
  styleApp.dispose();
}
