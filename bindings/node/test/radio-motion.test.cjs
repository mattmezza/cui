// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, button, checkbox, column, item, radio, radio_group, tabs, tag} = require('../index.cjs');

function press(app, node) {
  const {x, y, width, height} = node.bounds();
  assert.ok(width > 0 && height > 0);
  return app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
}

const app = new App({backend: 'headless', maxNodes: 8});
try {
  const legacy = process.env.CUI_EXPECT_RADIO_LEGACY === '1';
  assert.equal(app.supports_radio_motion(), !legacy);
  assert.equal(app.supports_radio_item_content(), !legacy);
  assert.throws(() => checkbox().animate_selection(), TypeError);
  assert.throws(() => tabs().animate_selection(), TypeError);
  assert.throws(() => checkbox().decorate_content(), TypeError);
  assert.throws(() => item('invalid').decorate_content(1), TypeError);
  const invalidParent = tabs().children(item('a').decorate_content());
  assert.throws(() => app.mount(column().children(invalidParent)), /direct RadioGroup Item/);
  if (!legacy) {
    const invalidChild = item('a').children(column().children(button())).decorate_content();
    const invalidTree = column().children(radio_group().children(invalidChild));
    assert.throws(() => app.mount(invalidTree), /requires passive children/);
  }
  assert.equal(app.root, null);

  const changes = [];
  const standalone = radio('updates', 'Receive updates').on_change(event => changes.push(event.checked));
  const standard = item('standard', 'Standard');
  const recommendation = tag('recommended', 'Recommended');
  const priority = item('priority', 'Priority').detail('Tracked delivery')
    .children(column().children(recommendation)).decorate_content();
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
    assert.throws(() => app.mount(root), /Radio decoration is unavailable/);
    assert.equal(app.root, null);
    priority.decorate_content(false);
  }
  app.mount(root);
  assert.equal(app.step(), 0);
  assert.equal(group.selected(), standard);
  assert.equal(standalone.is_checked(), false);

  if (legacy) {
    assert.throws(() => standalone.animate_selection(true), /Radio motion is unavailable/);
    assert.equal(standalone.animate_selection(false), standalone);
    assert.throws(() => priority.decorate_content(), /Radio decoration is unavailable/);
    assert.throws(() => recommendation.bounds());
  } else {
    assert.equal(standalone.animate_selection(), standalone);
  }
  assert.equal(app.step(), 0);
  assert.equal(press(app, legacy ? priority : recommendation), 1);
  if (!legacy) {
    priority.decorate_content(false);
    assert.equal(app.step(), 0);
    assert.throws(() => recommendation.bounds());
    assert.equal(group.selected(), priority);
    priority.decorate_content();
    assert.equal(app.step(), 0);
    assert.ok(recommendation.bounds().width > 0);
  }
  assert.equal(press(app, standalone), 1);
  assert.equal(standalone.is_checked(), true);
  group.animate_selection(false);
  standalone.animate_selection(false);
  assert.equal(app.step(0.25), 0);
  assert.deepEqual(changes, [priority, true]);
} finally {
  app.dispose();
}
