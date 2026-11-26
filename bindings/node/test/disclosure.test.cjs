'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const {App, accordion, collapsible, column, item, label} = require('../index.cjs');

test('typed disclosure preserves expansion and queued events across a presentation reset', () => {
  const app = new App();
  try {
    for (const options of [
      {width: NaN}, {joined: 1}, {clipCard: true}, {collapsedIcon: 'x'.repeat(64)}, {cardBorderRgba: -1},
    ]) assert.throws(() => accordion().disclosure_configure(options));
    assert.throws(() => collapsible().set_trigger_style({cursor: 'bad'}));

    const supported = process.env.CUI_TEST_LEGACY_DISCLOSURE !== '1';
    assert.equal(app.supports_disclosure_presentation(), supported);
    if (!supported) {
      const fold = collapsible('advanced', 'Advanced').children(label('Cache preferences'));
      const root = column().children(fold.disclosure_configure({}));
      assert.throws(() => app.mount(root), /both optional/);
      fold.disclosure_configure(null);
      app.mount(root).step();
      fold.open(true);
      app.step();
      assert.equal(fold.is_open(), true);
      return;
    }

    const changes = [];
    const leaf = item('leaf', 'Editor').children(label('Preferences')).on_change(event => changes.push(event));
    const group = accordion('settings').children(leaf);
    const config = {joined: true, bordered: true, allowCollapse: true, width: 320};
    group.disclosure_configure(config);
    config.width = NaN;
    leaf.set_trigger_style({hoverRgba: 0, expandedRgba: 0x224466ff, cursor: 'pointer'});
    app.mount(column().children(group)).step();
    assert.equal(group.bounds().width, 320);
    assert.throws(() => group.disclosure_configure({collapsedIcon: 'not-an-embedded-icon'}));

    const {x, y} = leaf.bounds();
    app.pointer('down', x + 20, y + 12).pointer('up', x + 20, y + 12).step_only();
    group.disclosure_configure(null);
    leaf.set_trigger_style(null);
    assert.deepEqual(changes, []);
    app.step();
    assert.equal(changes.length, 1);
    assert.equal(changes[0].checked, true);
    assert.equal(leaf.is_checked(), true);
    app.step();
    assert.equal(changes.length, 1);
  } finally {
    app.dispose();
  }
});
