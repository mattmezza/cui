// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, checkbox, column, item, radio, radio_group, tabs} = require('../index.cjs');

function press(app, node) {
  const {x, y, width, height} = node.bounds();
  assert.ok(width > 0 && height > 0);
  return app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
}

const app = new App({backend: 'headless', maxNodes: 5});
try {
  const legacy = process.env.CUI_EXPECT_RADIO_LEGACY === '1';
  assert.equal(app.supports_radio_motion(), !legacy);
  assert.throws(() => checkbox().animate_selection(), TypeError);
  assert.throws(() => tabs().animate_selection(), TypeError);

  const changes = [];
  const standalone = radio('updates', 'Receive updates').on_change(event => changes.push(event.checked));
  const standard = item('standard', 'Standard');
  const priority = item('priority', 'Priority');
  const group = radio_group('delivery').children(standard, priority).select(standard)
    .on_change(event => {
      assert.ok(event.selectedHandle.generation > 0);
      changes.push(group.selected());
    });
  assert.throws(() => standalone.animate_selection(1), TypeError);
  const root = column().children(standalone, group.animate_selection());

  if (legacy) {
    assert.throws(() => app.mount(root), /Radio motion is unavailable/);
    assert.equal(app.root, null);
    group.animate_selection(false);
  }
  app.mount(root);
  assert.equal(app.step(), 0);
  assert.equal(group.selected(), standard);
  assert.equal(standalone.is_checked(), false);

  if (legacy) {
    assert.throws(() => standalone.animate_selection(true), /Radio motion is unavailable/);
    assert.equal(standalone.animate_selection(false), standalone);
  } else {
    assert.equal(standalone.animate_selection(), standalone);
  }
  assert.equal(app.step(), 0);
  assert.equal(press(app, priority), 1);
  assert.equal(press(app, standalone), 1);
  assert.equal(standalone.is_checked(), true);
  group.animate_selection(false);
  standalone.animate_selection(false);
  assert.equal(app.step(0.25), 0);
  assert.deepEqual(changes, [priority, true]);
} finally {
  app.dispose();
}
