'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const c = require('..');
const native = require(process.env.CUI_NODE_ADDON);

test('Button focus validates named modes', () => {
  for (const mode of ['legacy', 'variant', 'outer-ring']) {
    const b = c.button(); assert.equal(b.focus_treatment(mode), b);
  }
  for (const mode of [null, true, 0, 'outer', '__proto__', [], {}])
    assert.throws(() => c.button().focus_treatment(mode), TypeError);
  assert.throws(() => c.column().focus_treatment('legacy'), TypeError);
});
test('optional Button focus preserves old-provider defaults atomically', () => {
  const app = new c.App({backend:'headless'}), saved = native.supportsButtonFocus;
  try {
    native.supportsButtonFocus = undefined;
    assert.equal(app.supports_button_focus(), false);
    assert.throws(() => app.mount(c.column().children(c.button().focus_treatment('variant'))), /set_button_focus_v1/);
    assert.equal(app.root, null);
    const b = c.button().focus_treatment('legacy');
    app.mount(c.column().children(b)).step();
    assert.equal(b.focus_treatment('legacy'), b);
    assert.throws(() => b.focus_treatment('outer-ring'), /set_button_focus_v1/);
    assert.equal(b.buttonFocusValue, 'legacy');
  } finally { native.supportsButtonFocus = saved; app.dispose(); }
});
test('live Button focus preserves layout and click callback', () => {
  const app = new c.App({backend:'headless'});
  try {
    if (process.env.CUI_EXPECT_BUTTON_FOCUS !== undefined)
      assert.equal(app.supports_button_focus(), process.env.CUI_EXPECT_BUTTON_FOCUS === '1');
    if (!app.supports_button_focus()) return;
    let clicks = 0;
    const b = c.button('save').label('Save').focus_treatment('variant').on_click(() => ++clicks);
    app.mount(c.column().children(b)).step();
    const bounds = b.bounds();
    for (const mode of ['outer-ring', 'legacy', 'variant']) {
      assert.equal(b.focus_treatment(mode), b); app.step(); assert.deepEqual(b.bounds(), bounds);
    }
    app.pointer('down', bounds.x+2, bounds.y+2).pointer('up', bounds.x+2, bounds.y+2);
    app.step(); app.step(); assert.equal(clicks, 1);
  } finally { app.dispose(); }
});
