'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const c = require('../index.cjs');
test('copied heading and divider geometry, reset and queued change', () => {
  const app = new c.App();
  try {
    const supported = process.env.CUI_TEST_LEGACY_DISCLOSURE !== '1';
    assert.equal(app.supports_disclosure_heading(), supported);
    assert.equal(app.supports_disclosure_divider(), supported);
    for (const v
             of [{unknown: 1}, {box: {height: NaN}}, {box: {height: true}}, {box: {width: -2}},
                 {typography: {}}, {typography: {fontSize: 1e-50}}, {typography: {fontSize: 0}},
                 {typography: {fontSize: 14, wrap: 1}}, {titleRgba: -1}])
      assert.throws(() => c.accordion().set_disclosure_heading('normal', v));
    for (const v of [{thickness: Infinity}, {insetLeft: -1}, {colorRgba: true}, {extra: 0}])
      assert.throws(() => c.accordion().set_disclosure_divider(v));
    assert.throws(() => c.label('x').set_disclosure_heading('normal', {}));
    const changes = [];
    const leaf =
        c.item('leaf', 'Editor').children(c.label('Content')).on_change(e => changes.push(e));
    const second = c.item('second', 'Account').children(c.label('Details'));
    const group = c.accordion('group').children(leaf, second);
    const style = {box: {width: 300, height: 60, padding: 8}, titleRgba: 0};
    group.set_disclosure_heading('normal', style);
    style.box.height = NaN;
    const root = c.column().children(group);
    if (!supported) {
      assert.throws(() => app.mount(root), /optional/);
      group.set_disclosure_heading('normal', null);
      app.mount(root).step();
      return;
    }
    group.disclosure_configure({joined: true, width: 300});
    group.set_disclosure_divider({thickness: 0, colorRgba: 0});
    app.mount(root).step();
    assert.equal(leaf.bounds().height, 60);
    const secondY = second.bounds().y;
    const groupHeight = group.bounds().height;
    group.set_disclosure_divider({thickness: 7, insetLeft: 8, insetRight: 12, colorRgba: 0});
    app.step();
    assert.equal(second.bounds().y, secondY + 7);
    assert.equal(group.bounds().height, groupHeight + 7);
    group.set_disclosure_divider({thickness: 0});
    app.step();
    assert.equal(second.bounds().y, secondY);
    const {x, y} = leaf.bounds();
    app.pointer('down', x + 20, y + 20).pointer('up', x + 20, y + 20).step_only();
    group.set_disclosure_heading('normal', null).set_disclosure_divider(null);
    assert.equal(changes.length, 0);
    app.step();
    assert.equal(changes.length, 1);
    assert.equal(changes[0].checked, true);
    assert.throws(() => leaf.set_disclosure_heading('normal', {box: {height: -1}}));
  } finally {
    app.dispose();
  }
});
