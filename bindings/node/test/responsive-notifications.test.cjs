// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, column, notification} = require('..');
const app = new App({backend:'headless', width:220, height:320});
try {
  const supported = app.supports_responsive_notifications();
  if (process.env.CUI_EXPECT_RESPONSIVE_NOTIFICATIONS !== undefined)
    assert.equal(supported, process.env.CUI_EXPECT_RESPONSIVE_NOTIFICATIONS === '1');
  for (const value of [0, 1, null, 'true'])
    assert.throws(() => app.set_responsive_notifications(value), TypeError);
  const capability = app.supports_responsive_notifications;
  app.supports_responsive_notifications = () => false;
  for (const enabled of [true, false])
    assert.throws(() => app.set_responsive_notifications(enabled), /unavailable/);
  app.supports_responsive_notifications = capability;
  const events = [];
  const node = notification('notice', 'Saved').detail(
    'Your workspace changes have been saved and are ready for everyone on your team.'
  ).open().on_click(() => events.push('click')).on_close(() => events.push('close'));
  app.mount(column().children(node));
  app.step(.4);
  app.step(.3);
  assert.equal(node.is_open(), true);
  const legacyHeight = node.bounds().height;
  if (supported) {
    assert.equal(app.set_responsive_notifications(true), app);
    app.step(.4);
    app.step(.3);
    assert.ok(node.bounds().height > legacyHeight);
    app.set_responsive_notifications(false);
    app.step(.4);
    app.step(.3);
    assert.ok(Math.abs(node.bounds().height - legacyHeight) < .01);
    app.set_responsive_notifications(true);
    app.step(.4);
    app.step(.3);
    const {x, y, width} = node.bounds();
    // No hover frame: Close must be independently actionable already.
    app.pointer('down', x + width - 18, y + 18).pointer('up', x + width - 18, y + 18);
    app.step(.4);
    app.step(.3);
    assert.equal(node.is_open(), false);
    assert.deepEqual(events, ['close']);
  } else {
    assert.throws(() => app.set_responsive_notifications(true), /unavailable/);
  }
} finally {
  app.dispose();
}
assert.throws(() => app.supports_responsive_notifications(), /disposed/);
