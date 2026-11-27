'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const c = require('..');
const native = require(process.env.CUI_NODE_ADDON);

test('notification accepts one unattached Button action', () => {
  for (const invalid of [c.label('text'), c.column(), null]) {
    assert.throws(() => c.notification('n').action(invalid), /Button/);
  }
  assert.throws(() => c.column().action(c.button('b')), /Notification owner/);

  const action = c.button('b');
  const notice = c.notification('n').action(action);
  assert.equal(action.parent, notice);
  assert.throws(() => notice.action(c.button('other')), /one action/);
  assert.throws(() => notice.children(c.button('other')), /accept children/);
  assert.throws(() => c.notification('other').action(action), /unattached/);
  assert.equal(notice.childrenValue.length, 1);
});

test('unsupported action preflight leaves the app reusable', () => {
  const app = new c.App();
  const saved = native.notificationActions;
  try {
    native.notificationActions = undefined;
    assert.equal(app.supports_notification_action(), false);
    const action = c.button('action');
    const notice = c.notification('n', 'Ready').open().action(action);
    assert.throws(() => app.mount(c.column().children(notice)), /action unavailable/);
    assert.equal(app.root, null);
    assert.equal(app.nodes.size, 0);
    assert.equal(action.owner, null);

    const plain = c.notification('plain', 'Ready').open();
    app.mount(c.column().children(plain)).step_only();
    assert.equal(plain.is_open(), true);
  } finally {
    native.notificationActions = saved;
    app.dispose();
  }
});

test('Button click leaves the notification open and removal invalidates the Button', () => {
  const app = new c.App();
  try {
    const supported = app.supports_notification_action();
    const expected = process.env.CUI_EXPECT_NOTIFICATION_ACTION;
    if (expected !== undefined) assert.equal(supported, expected === '1');
    if (!supported) return;

    const calls = [];
    const action = c.button('undo').label('Undo').on_click(event => calls.push(event.node));
    const notice = c.notification('notice', 'Saved').action(action).open();
    app.mount(c.column().children(notice)).step_only().step_only();
    const b = action.bounds();
    assert.ok(b.width > 0);
    assert.ok(b.height > 0);
    const x = b.x + b.width / 2;
    const y = b.y + b.height / 2;
    app.pointer('down', x, y).step_only();
    app.pointer('up', x, y).step();
    assert.deepEqual(calls, [action]);
    assert.equal(notice.is_open(), true);

    notice.remove();
    assert.throws(() => action.bounds());
  } finally {
    app.dispose();
  }
});
