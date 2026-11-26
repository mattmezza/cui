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

test('independent title state lifecycle and copied draft', () => {
  const app = new c.App();
  try {
    for (const value of [{box: {}}, {titleRgba: -1}, {typography: {}}, {extra: 0}])
      assert.throws(() => c.accordion().set_disclosure_title_state('hovered', value));
    assert.throws(() => c.accordion().set_disclosure_title_state('hover', {}));
    assert.throws(() => c.label('x').set_disclosure_title_state('pressed', {}));
    const events = [];
    const leaf = c.item('title', 'Title').children(c.label('Body')).on_change(e => events.push(e));
    const group = c.accordion().children(leaf);
    const style = {typography: {fontSize: 18}, titleRgba: 0};
    group.set_disclosure_title_state('hovered', style);
    style.typography.fontSize = NaN;
    const root = c.column().children(group);
    const supported = app.supports_disclosure_title_state();
    const unavailable = process.env.CUI_TEST_LEGACY_DISCLOSURE === '1' ||
      process.env.CUI_TEST_TITLE_STATE_UNAVAILABLE === '1';
    assert.equal(supported, !unavailable);
    if (!supported) {
      assert.throws(() => app.mount(root), /optional/);
      group.set_disclosure_title_state('hovered', null);
      app.mount(root).step();
      return;
    }
    app.mount(root).step();
    leaf.set_disclosure_title_state('focus-visible', {titleRgba: 0});
    const {x, y} = leaf.bounds();
    app.pointer('down', x + 10, y + 10).pointer('up', x + 10, y + 10).step_only();
    leaf.set_disclosure_title_state('focus-visible', null);
    group.set_disclosure_title_state('hovered', {});
    assert.equal(events.length, 0);
    app.step();
    assert.equal(events.length, 1);
  } finally { app.dispose(); }
});
test('heading capability does not imply title state capability', () => {
  class Node {
    constructor() { this.kind = 19; this.childrenValue = []; }
  }
  const app = {_alive() {}, handle: {}};
  const api = require('../disclosure.cjs')({
    disclosureHeadingOptions(_handle, operation) { return operation === 0; },
  }, Node, Symbol('handle'), {});
  assert.equal(api.supportsHeading(app), true);
  assert.equal(api.supportsTitle(app), false);
  const node = new Node().set_disclosure_title_state('hovered', {titleRgba: 0});
  assert.throws(() => api.preflight(app, node), /optional/);
  node.set_disclosure_title_state('hovered', null);
  api.preflight(app, node);
});
