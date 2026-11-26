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
  assert.equal(app.supports_radio_tooltip(), !legacy);
  assert.throws(() => checkbox().animate_selection(), TypeError);
  assert.throws(() => tabs().animate_selection(), TypeError);
  assert.throws(() => checkbox().decorate_content(), TypeError);
  assert.throws(() => item('invalid').decorate_content(1), TypeError);
  const invalidParent = tabs().children(item('a').decorate_content());
  assert.throws(() => app.mount(column().children(invalidParent)), /direct RadioGroup Item/);
  const misplacedHint = tabs().children(item('hint').radio_tooltip('Wrong parent'));
  assert.throws(() => app.mount(column().children(misplacedHint)), /direct RadioGroup Item/);
  assert.throws(() => checkbox().radio_tooltip('Wrong control'), TypeError);
  assert.throws(() => radio().radio_tooltip(true), TypeError);
  assert.throws(() => radio().radio_tooltip('é'.repeat(129)), RangeError);
  assert.throws(() => radio().radio_tooltip('Bad\0text'), RangeError);
  for (const delay of [-1, 10001, 350.5, true])
    assert.throws(() => radio().radio_tooltip('Hint', delay), RangeError);
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
    .children(column().children(recommendation)).decorate_content()
    .radio_tooltip('Suivre le colis', 0);
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
    assert.throws(() => app.mount(root), /Radio tooltips are unavailable/);
    assert.equal(app.root, null);
    priority.radio_tooltip(null);
  }
  app.mount(root);
  assert.equal(app.step(), 0);
  assert.equal(group.selected(), standard);
  assert.equal(standalone.is_checked(), false);

  if (legacy) {
    assert.throws(() => standalone.animate_selection(true), /Radio motion is unavailable/);
    assert.equal(standalone.animate_selection(false), standalone);
    assert.throws(() => priority.decorate_content(), /Radio decoration is unavailable/);
    assert.throws(() => priority.radio_tooltip('Track shipment'), /Radio tooltips are unavailable/);
    assert.equal(priority.radio_tooltip(''), priority);
    assert.throws(() => recommendation.bounds());
  } else {
    assert.equal(standalone.animate_selection(), standalone);
    assert.equal(standalone.radio_tooltip('Email receipt', 0), standalone);
    const {x, y, width, height} = priority.bounds();
    assert.equal(app.pointer('move', x + width / 2, y + height / 2).step(), 0);
    assert.equal(priority.radio_tooltip('Track shipment', 0), priority);
    assert.equal(standalone.radio_tooltip(null), standalone);
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


function checkRadioTabOrder(legacy) {
  const app = new App({backend: 'headless', maxNodes: 6});
  try {
    assert.equal(app.supports_radio_tab_index(), !legacy);
    assert.throws(() => checkbox('invalid').tab_index(0), TypeError);
    assert.throws(() => item('invalid').tab_index(-2), TypeError);
    for (const value of [true, '0', undefined])
      assert.throws(() => radio().tab_index(value), TypeError);
    for (const value of [NaN, Infinity, 0.5, -0x80000001, 0x80000000])
      assert.throws(() => radio_group().tab_index(value), RangeError);

    const events = [];
    const review = button('review').label('Review').on_click(() => events.push('review'));
    const single = radio('alternate', 'Alternate').on_change(event => events.push(event.checked));
    const standard = item('standard', 'Standard');
    const priority = item('priority', 'Priority');
    const group = radio_group('delivery').children(standard, priority).select(standard)
      .on_change(event => {
        assert.ok(event.selectedHandle.generation > 0);
        events.push(group.selected());
      });
    const root = column().children(review, single, group);
    const pressKey = (key, mods = 0) => app.key('down', key, mods).key('up', key, mods).step();

    if (legacy) {
      group.tab_index(0);
      assert.throws(() => app.mount(root), /Radio tab order is unavailable/);
      assert.equal(app.root, null);
      group.tab_index(null);
      app.mount(root).step();
      assert.throws(() => single.tab_index(-2), /Radio tab order is unavailable/);
      single.tab_index(null);
      assert.equal(pressKey('tab'), 0);
      assert.equal(pressKey('enter'), 1);
      assert.equal(pressKey('tab'), 0);
      assert.equal(pressKey('tab'), 0);
      assert.equal(pressKey('down'), 1);
      assert.deepEqual(events, ['review', priority]);
      return;
    }

    group.tab_index(-0x80000000);
    single.tab_index(0x7fffffff);
    assert.equal(app.mount(root).step(), 0);
    assert.equal(pressKey('tab'), 0);
    assert.equal(pressKey('down'), 1);
    assert.equal(pressKey('tab'), 0);
    assert.equal(pressKey('enter'), 1);
    assert.equal(pressKey('tab', 1), 0);
    assert.equal(pressKey('up'), 1);
    assert.equal(pressKey('tab'), 0);
    assert.equal(pressKey('tab'), 0);
    assert.equal(pressKey('enter'), 1);
    assert.deepEqual(events, [priority, 'review', standard, true]);

    assert.equal(group.tab_index(null), group);
    assert.equal(single.tab_index(0), single);
    assert.equal(app.step(), 0);
    assert.equal(pressKey('tab'), 0);
    assert.equal(pressKey('down'), 1);
    assert.deepEqual(events, [priority, 'review', standard, true, priority]);
  } finally {
    app.dispose();
  }
}

checkRadioTabOrder(process.env.CUI_EXPECT_RADIO_LEGACY === '1');

function checkRadioPaint(legacy) {
  const {RadioPaintLayer: L} = require('../index.cjs');
  const app = new App({backend:'headless', maxNodes:6});
  try {
    assert.equal(app.supports_radio_paint(), !legacy);
    assert.throws(() => checkbox().radio_paint(L.BASE, {}), TypeError);
    for (const paint of [{unknown:0},{descriptionColor:0},{labelWeight:4},{indicatorExtent:3},{rootPadding:NaN},{markColor:-1}])
      assert.throws(() => radio().radio_paint(L.BASE, paint));
    assert.throws(() => radio().radio_paint(L.CHECKED,{rootPadding:0}), TypeError);
    assert.throws(() => radio_group().radio_paint(L.CHECKED,{}), RangeError);
    assert.throws(() => radio_group().radio_paint(L.BASE,{indicatorExtent:20}), TypeError);
    const wrong = item('wrong').radio_paint(L.BASE,{labelColor:0});
    assert.throws(() => app.mount(column().children(tabs().children(wrong))), /direct RadioGroup Item/);
    assert.equal(app.root,null);
    const changes = [];
    const single = radio('single', 'Single').on_change(event => changes.push(event.checked));
    const first = item('first', 'First'), second = item('second', 'Second').detail('Details');
    const group = radio_group().children(first, second).select(first)
      .on_change(() => changes.push(group.selected()));
    const root = column().children(single,group);
    if (legacy) {
      single.radio_paint(L.BASE,{rootPadding:0});
      assert.throws(() => app.mount(root), /Radio paint is unavailable/);
      assert.equal(app.root,null);
      single.radio_paint(L.BASE,null);
      app.mount(root).step();
      assert.throws(() => single.radio_paint(L.BASE,{labelColor:0}), /Radio paint is unavailable/);
      assert.equal(single.radio_paint(L.BASE,null),single);
      assert.equal(single.radio_paint(L.BASE,{}),single);
      return;
    }
    app.mount(root).step();
    const before = single.bounds();
    single.radio_paint(L.BASE,{rootPadding:20,indicatorExtent:32,labelSize:18,labelWeight:2,markColor:0});
    group.radio_paint(L.BASE,{rootBorderWidth:0,rootPadding:8});
    second.radio_paint(L.BASE,{descriptionColor:0,descriptionSize:12,descriptionWeight:1});
    for (const layer of [L.CHECKED,L.HOVERED,L.PRESSED,L.FOCUSED,L.DISABLED])
      single.radio_paint(layer,{rootBackground:0,indicatorBorderWidth:0,labelColor:0});
    assert.equal(app.step(),0);
    const painted=single.bounds();
    assert.ok(painted.height > before.height);
    assert.equal(single.is_checked(),false);
    assert.equal(group.selected(),first);
    assert.equal(press(app,single),1);
    assert.equal(single.is_checked(),true);
    single.disabled();
    assert.equal(app.step(),0);
    assert.equal(press(app,single),0);
    single.disabled(false).radio_paint(L.BASE,null);
    for (const layer of [L.CHECKED,L.HOVERED,L.PRESSED,L.FOCUSED,L.DISABLED]) single.radio_paint(layer,{});
    assert.equal(app.step(),0);
    assert.equal(single.bounds().height,before.height);
    assert.equal(press(app,second),1);
    assert.equal(group.selected(),second);
    assert.deepEqual(changes, [true, second]);
  } finally { app.dispose(); }
}
checkRadioPaint(process.env.CUI_EXPECT_RADIO_LEGACY === '1');
