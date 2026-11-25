// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, IconError, IconSource, button, column, configured_badge, configured_icon,
  label, row} = require('../index.cjs');

const font = process.argv.slice(2).find(arg => !arg.startsWith('--'));
const smoke = process.argv.includes('--smoke');
if (smoke && (!font || process.env.CUI_TEST_VIRTUAL_DISPLAY !== '1'))
  throw new Error('--smoke requires a font and the private display from scripts/test_local.py');

const check = '<svg width="20" height="20"><path d="M3 10l4 4 10-10" fill="none" stroke="white" stroke-width="2"/></svg>';
const app = new App({width: 480, height: 260, title: 'CUI inbox', backend: font ? 'sdl' : 'headless',
  fontPath: font, initiallyHidden: smoke});
try {
  const artwork = app.icon_resource(IconSource.svg(check));
  const headerIcon = configured_icon('header-status', artwork, {width: 24, height: 24, colorRgba: 0x34a170ff},
    {name: 'Synced'});
  const status = label('8 unread messages');
  const unread = configured_badge('unread-count', {count: 8n, name: 'Unread'});
  const synced = configured_badge('synced', {variant: 'icon', artwork, name: 'Synced',
    artworkOptions: {width: 12, height: 12}});
  const mark = button('mark').primary().label('Mark all as read').on_click(() => {
    unread.configure({...unread.get_config(), count: 0n});
    status.label('Inbox is up to date');
  });
  app.mount(column().gap(16).children(row().gap(12).children(headerIcon, label('Workspace inbox')),
    status, row().gap(20).children(unread.children(mark),
      synced.children(button('outbox').secondary().label('Outbox')))));
  if (font && !smoke) {
    app.run();
  } else {
    app.step();
    const bounds = mark.bounds();
    assert.equal(artwork.info().references, 2);
    assert.equal(synced.result().iconError, IconError.OK);
    assert.ok(unread.indicator_bounds());
    const x = bounds.x + bounds.width / 2;
    const y = bounds.y + bounds.height / 2;
    assert.equal(app.pointer('down', x, y).pointer('up', x, y).step(), 1);
    app.step();
    assert.equal(unread.indicator_bounds(), undefined);
    assert.deepEqual(mark.bounds(), bounds);
    console.log('Inbox: shared SVG, child activation and stable layout');
  }
} finally {
  app.dispose();
}
