// SPDX-License-Identifier: MIT
'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const {App, column, pagination, rating, label} = require('..');
const native = require(process.env.CUI_NODE_ADDON);
function withApp(work) { const app = new App(); try { work(app); } finally { app.dispose(); } }
function press(app, bounds) {
  assert.ok(bounds.width > 0 && bounds.height > 0);
  const x = bounds.x + bounds.width / 2, y = bounds.y + bounds.height / 2;
  app.pointer('down', x, y).pointer('up', x, y).step();
}
const unsupported = process.env.CUI_PAGINATION_UNSUPPORTED === '1';
test('ordinary paging and CHANGE callbacks remain available', () => withApp(app => {
  const events = [], pages = pagination('pages').total(20).value(10).on_change(event => events.push(event));
  assert.equal(app.supports_pagination_options(), !unsupported);
  app.mount(column().children(pages)).step();
  press(app, pages.numeric_part_bounds(3));
  assert.equal(pages.current_value(), 11);
  assert.equal(events.length, 1);
  assert.equal(events[0].value, 11);
}));
if (unsupported) {
  test('unsupported opt-in fails before mount mutation and leaves legacy mounting available', () => {
    for (const node of [pagination('options').configure({counter: true}), pagination('size').design_size('lg')]) {
      withApp(app => {
        assert.throws(() => app.mount(column().children(node)), /complete, compatible bridge/);
        assert.equal(app.root, null);
        assert.equal(app.nodes.size, 0);
        const ordinary = pagination('ordinary');
        app.mount(column().children(ordinary)).step();
        assert.throws(() => ordinary.configure({}), /complete, compatible bridge/);
        assert.throws(() => ordinary.design_size('sm'), /complete, compatible bridge/);
        assert.equal(ordinary.get_config(), undefined);
      });
    }
    withApp(app => app.mount(column().children(pagination().configure({}).configure(null))));
  });
} else {
  test('copied configuration, independent page state, replacement and clearing', () => withApp(app => {
    const options = {counter: true, iconOnly: true, chevrons: true, jumpMenu: true,
      visiblePages: 9, availableWidth: 317.3, group: 'Seiten', first: 'Erste', previous: 'Zurück',
      next: 'Weiter', last: 'Letzte', earlierPages: 'Früher', laterPages: 'Später'};
    const events = [], pages = pagination('pages').total(20).value(10).configure(options).design_size('lg')
      .on_change(event => events.push(event));
    options.group = 'changed';
    const snapshot = pages.get_config();
    assert.equal(snapshot.group, 'Seiten');
    assert.equal(snapshot.availableWidth, Math.fround(317.3));
    assert.ok(Object.isFrozen(snapshot));
    app.mount(column().children(pages)).step();
    assert.deepEqual(pages.get_config(), snapshot);
    assert.equal(pages.current_value(), 10);
    assert.equal(events.length, 0);
    press(app, pages.numeric_part_bounds(3));
    assert.equal(pages.current_value(), 11);
    assert.equal(events.length, 1);
    pages.configure({counter: true, next: 'Next page'});
    assert.equal(pages.get_config().group, '');
    assert.equal(pages.get_config().visiblePages, 0);
    assert.equal(pages.current_value(), 11);
    pages.configure(null);
    assert.equal(pages.get_config(), undefined);
    pages.configure({visiblePages: 5}).reset_config();
    assert.equal(pages.get_config(), undefined);
    app.step();
    assert.equal(events.length, 1);
  }));
  test('numbered layout wraps at copied width and design size stays independent of reset', () => withApp(app => {
    const changes = [];
    const pages = pagination('wrapped').total(400).value(11).design_size('2xl')
      .configure({chevrons: true, jumpMenu: true, visiblePages: 5, availableWidth: 224})
      .on_change(event => changes.push(event.value));
    app.mount(column().children(pages)).step();
    const previous = pages.numeric_part_bounds(2), next = pages.numeric_part_bounds(3);
    assert.ok(next.y > previous.y);
    assert.equal(next.height, 56);
    assert.equal(previous.height, 56);
    press(app, pages.numeric_part_bounds(6));
    assert.deepEqual(changes, []);
    app.key('down', 'down').step();
    app.key('down', 'enter').step();
    assert.deepEqual(changes, [14]);
    pages.reset_config();
    app.step();
    assert.equal(pages.numeric_part_bounds(3).height, 56);
    pages.design_size('sm');
    app.step();
    assert.ok(pages.numeric_part_bounds(3).height < 56);
    assert.equal(pages.current_value(), 14);
  }));
  test('direct addon validates fields, ownership, wrong kinds and stale handles', () => withApp(app => {
    const handle = native.makeNode(app.handle, null, 2, 'root');
    const pages = native.makeNode(app.handle, handle, 42, 'pages');
    const other = native.makeNode(app.handle, handle, 43, 'rating');
    const configure = (...args) => native.paginationOptions(app.handle, 1, pages, ...args);
    assert.throws(() => configure(16, 0, 0, '', '', '', '', '', '', ''), /invalid/i);
    assert.throws(() => configure(0, 4, 0, '', '', '', '', '', '', ''), /invalid/i);
    assert.throws(() => configure(0, 0, Number.MIN_VALUE, '', '', '', '', '', '', ''), /underflows/);
    assert.throws(() => configure(0, 0, 32768.0001, '', '', '', '', '', '', ''), /width/);
    assert.throws(() => configure(0, 0, 0, 'a'.repeat(128), '', '', '', '', '', ''), /127/);
    assert.throws(() => configure(0, 0, 0, '\n', '', '', '', '', '', ''), /invalid/i);
    assert.throws(() => native.paginationOptions(app.handle, 2, other), /invalid/i);
    withApp(second => assert.throws(() => native.paginationOptions(second.handle, 2, pages), /another app/));
    assert.equal(native.paginationOptions(app.handle, 2, pages), undefined);
    configure(0, 5, 128, '', '', '', '次へ', '', '', '');
    assert.equal(native.paginationOptions(app.handle, 2, pages).next, '次へ');
    native.removeNode(app.handle, pages);
    assert.throws(() => native.paginationOptions(app.handle, 2, pages), /stale handle/);
  }));
}
test('invalid options fail before mutating retained configuration', () => {
  const node = pagination().configure({group: 'valid'}), original = node.get_config();
  for (const options of [undefined, [], {counter: 1}, {visiblePages: 4}, {visiblePages: 33},
    {visiblePages: 5.5}, {availableWidth: NaN}, {availableWidth: -1}, {availableWidth: 32769},
    {availableWidth: Number.MIN_VALUE}, {next: 'a'.repeat(128)}, {next: 'é'.repeat(64)},
    {next: '\0'}, {next: '\n'}, {next: '\u0085'}, {next: '\u2028'}, {next: '\ud800'}]) {
    assert.throws(() => node.configure(options));
    assert.equal(node.get_config(), original);
  }
  assert.throws(() => label().configure({}), /requires Pagination/);
  assert.equal(pagination().configure({next: 'é'.repeat(63)}).get_config().next.length, 63);
});

const customUnsupported = process.env.CUI_PAGINATION_CUSTOM_UNSUPPORTED === '1';
test('custom height validates detached values atomically', () => {
  const pages = pagination().custom_size(52.5);
  for (const value of [NaN, Infinity, -1, .5, 512.001, true, '52.5', null, undefined]) {
    assert.throws(() => pages.custom_size(value));
    assert.equal(pages.get_custom_size(), 52.5);
  }
  assert.equal(pages.custom_size(0).get_custom_size(), 0);
  assert.throws(() => rating().custom_size(52.5), /requires Pagination/);
});
test('custom height feature and legacy mounting are independent', () => withApp(app => {
  assert.equal(app.supports_pagination_custom_size(), !customUnsupported);
  if (customUnsupported) {
    assert.throws(() => app.mount(column().children(pagination().custom_size(52.5))), /compatible bridge/);
    assert.equal(app.nodes.size, 0);
    const pages = pagination().custom_size(52.5).custom_size(0);
    app.mount(column().children(pages)).step();
    assert.throws(() => pages.custom_size(52.5), /compatible bridge/);
    assert.throws(() => pages.custom_size(0), /compatible bridge/);
    assert.throws(() => pages.get_custom_size(), /compatible bridge/);
    if (!unsupported) {
      pages.design_size('lg').configure({counter: true});
      app.step();
      assert.equal(pages.numeric_part_bounds(3).height, 40);
      pages.design_size('inherit').reset_config();
    }
    return;
  }
  const changes = [], pages = pagination().total(20).value(10)
    .custom_size(52.5).on_change(event => changes.push(event.value));
  const ordinary = pagination('ordinary');
  app.mount(column().children(pages, ordinary)).step();
  assert.equal(pages.get_custom_size(), 52.5);
  assert.equal(pages.numeric_part_bounds(3).height, 52.5);
  press(app, pages.numeric_part_bounds(3));
  assert.deepEqual(changes, [11]);
  app.step();
  assert.deepEqual(changes, [11]);
  assert.throws(() => pages.custom_size(513));
  assert.equal(pages.get_custom_size(), 52.5);
  if (!unsupported) pages.design_size('sm');
  app.step();
  assert.equal(pages.numeric_part_bounds(3).height, 52.5);
  pages.custom_size(0);
  app.step();
  assert.equal(pages.get_custom_size(), 0);
  assert.equal(pages.numeric_part_bounds(3).height,
    unsupported ? ordinary.numeric_part_bounds(3).height : 24);
  app.dispose();
  assert.throws(() => pages.get_custom_size(), /disposed/);
  assert.throws(() => pages.custom_size(0), /disposed/);
}));
if (!customUnsupported) test('direct custom height rejects stale handles, foreign owners and wrong kinds', () => withApp(app => {
  const root = native.makeNode(app.handle, null, 2, 'root');
  const pages = native.makeNode(app.handle, root, 42, 'pages');
  const other = native.makeNode(app.handle, root, 43, 'rating');
  native.paginationCustomSize(app.handle, 1, pages, 52.5);
  for (const value of [-1, .5, 512.001, NaN, Infinity])
    assert.throws(() => native.paginationCustomSize(app.handle, 1, pages, value));
  assert.equal(native.paginationCustomSize(app.handle, 2, pages), 52.5);
  assert.throws(() => native.paginationCustomSize(app.handle, 2, other), /invalid/i);
  withApp(second => assert.throws(() => native.paginationCustomSize(second.handle, 2, pages), /another app/));
  native.removeNode(app.handle, pages);
  assert.throws(() => native.paginationCustomSize(app.handle, 2, pages), /stale handle/);
}));
