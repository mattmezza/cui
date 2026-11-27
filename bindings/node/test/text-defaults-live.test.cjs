'use strict';
const assert = require('node:assert/strict');
const {App, column, row, label, icon, spinner} = require('../index.cjs');
const app = new App({backend:'headless'});
try {
  if (process.env.CUI_EXPECT_TEXT_DEFAULTS_UNSUPPORTED === '1') {
    assert.equal(app.supports_text_defaults(), false);
    assert.throws(() => app.mount(column().textDefaults(24)), /unavailable/);
    app.mount(column().children(label('Legacy still works'))); app.step();
  } else {
    assert.equal(app.supports_text_defaults(), true);
    const inherited = icon('inherited', 'check'), sibling = icon('sibling', 'check');
    const nested = row().textDefaults(30, 0).children(inherited, label('Nested'), spinner('busy'));
    app.mount(column().textDefaults(20, 0x336699ff).children(nested, row().children(sibling)));
    app.step(); assert.equal(inherited.bounds().width, 30); assert.equal(sibling.bounds().width, 20);
    nested.textDefaults(); app.step();
    assert.equal(inherited.bounds().width, 20); assert.equal(sibling.bounds().width, 20);
    nested.textDefaults(16); app.step(); assert.equal(inherited.bounds().width, 16);
  }
} finally {app.dispose();}
console.log('text defaults live tests passed');
