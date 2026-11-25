// SPDX-License-Identifier: MIT
import {App, IconError, IconSource, button, column, configured_badge,
  configured_icon, label, row} from '../../node/index.cjs';
import type {BadgeConfigInput, IconOptionsInput} from '../../node/index.cjs';

const check = '<svg width="20" height="20"><path d="M3 10l4 4 10-10" fill="none" stroke="white" stroke-width="2"/></svg>';
const app = new App({width: 480, height: 260});
try {
  const artwork = app.icon_resource(IconSource.svg(check));
  const titleStyle: IconOptionsInput = {width: 24, height: 24, colorRgba: 0x34a170ff};
  const glyph = configured_icon('header', artwork, titleStyle, {name: 'Synced'});
  const badgeStyle: BadgeConfigInput = {variant: 'icon', artwork, artworkOptions: {width: 12, height: 12}};
  const sync = configured_badge('sync', badgeStyle).children(button('outbox').label('Outbox'));
  const unread = configured_badge('unread', {count: 8n, name: 'Unread'});
  const clear = button('clear').primary().label('Mark all as read').on_click(() => {
    unread.configure({...unread.get_config(), count: 0n});
  });
  app.mount(column().gap(16).children(row().gap(12).children(glyph, label('Workspace inbox')),
    row().gap(20).children(unread.children(clear), sync))).step();
  if (artwork.info().references !== 2 || sync.result()?.iconError !== IconError.OK ||
      unread.indicator_bounds() === undefined)
    throw new Error('Expected shared SVG and unread indicator');
  const {x, y, width, height} = clear.bounds();
  app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
  app.step();
  if (unread.indicator_bounds() !== undefined || clear.bounds().x !== x || clear.bounds().y !== y)
    throw new Error('Clearing the badge must leave the child button in place');
  console.log('TypeScript inbox: shared SVG, child activation and stable layout');
} finally {
  app.dispose();
}
