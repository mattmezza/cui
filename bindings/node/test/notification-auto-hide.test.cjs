'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const c = require('..');
const native = require(process.env.CUI_NODE_ADDON);

test('auto_hide accepts Notification and strict booleans', () => {
  assert.equal(c.notification('n').auto_hide().notificationAutoHideValue, true);
  for (const value of [0, 1, null, 'true']) assert.throws(() => c.notification('n').auto_hide(value), TypeError);
  assert.throws(() => c.column().auto_hide(), TypeError);
});

test('missing optional timer fails atomically and plain cards persist', () => {
  const app = new c.App();
  const saved = native.notificationAutoHide;
  try {
    native.notificationAutoHide = undefined;
    assert.equal(app.supports_notification_auto_hide(), false);
    assert.throws(() => app.mount(c.column().children(c.notification('n').auto_hide(false).open())), /auto-hide unavailable/);
    assert.equal(app.root, null);
    assert.equal(app.nodes.size, 0);
    const plain = c.notification('plain').open();
    app.mount(c.column().children(plain));
    for (let i = 0; i < 10; ++i) app.step_only(1);
    assert.equal(plain.is_open(), true);
  } finally { native.notificationAutoHide = saved; app.dispose(); }
});

test('timer expires once and changes apply on next opening', () => {
  const app = new c.App();
  try {
    const supported = app.supports_notification_auto_hide();
    if (process.env.CUI_EXPECT_NOTIFICATION_AUTO_HIDE !== undefined)
      assert.equal(supported, process.env.CUI_EXPECT_NOTIFICATION_AUTO_HIDE === '1');
    if (!supported) return;
    let closed = 0;
    const notice = c.notification('timed').auto_hide().open().on_close(() => ++closed);
    const persistent = c.notification('persistent').auto_hide(false).open();
    app.mount(c.column().children(notice, persistent)).step(1);
    notice.auto_hide(false);
    for (let i = 0; i < 8; ++i) app.step(1);
    assert.equal(notice.is_open(), false);
    assert.equal(persistent.is_open(), true);
    assert.equal(closed, 1);
    notice.open();
    for (let i = 0; i < 10; ++i) app.step(1);
    assert.equal(notice.is_open(), true);
    assert.equal(closed, 1);
  } finally { app.dispose(); }
});
