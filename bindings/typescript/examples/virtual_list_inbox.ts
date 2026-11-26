// SPDX-License-Identifier: MIT
import {App, button, column, item, label, virtual_list} from '../../node/index.cjs';
import type {VirtualListRow} from '../../node/index.cjs';

declare const process: {argv: string[]};
const fontPath = process.argv[2];
const app = new App({backend: fontPath ? 'sdl' : 'headless', fontPath, title: 'Inbox', width: 360, height: 320});
try {
  const rows: VirtualListRow[] = Array.from({length: 20}, (_, i) => ({
    item: item(`message-${i}`, `Message ${i}: project update`), extent: 32,
  }));
  const inbox = virtual_list('inbox').visual_size(320, 160).children(...rows.map(row => row.item));
  const summary = label('Scroll the inbox, then prepend or expand a message.');
  const supported = app.supportsVirtualListRows();
  let sequence = 0;
  const add = button('prepend').label('Prepend message').disabled(!supported).on_click(() => {
    if (rows.length === 64) {summary.label('Inbox capacity reached (64 messages).'); return;}
    const message = item(`new-${++sequence}`, `New message ${sequence}`);
    inbox.appendVirtualListItem(message);
    rows.unshift({item: message, extent: 48});
    inbox.setVirtualListRows(rows);
  });
  const expand = button('resize').label('Expand first message').disabled(!supported).on_click(() => {
    rows[0].extent = rows[0].extent === 80 ? 32 : 80;
    inbox.setVirtualListRows(rows);
  });
  app.mount(column().gap(8).children(add, expand, inbox, summary));
  if (supported) inbox.preserveVirtualListAnchor().setVirtualListRows(rows);
  if (fontPath) app.run();
  else {
    app.step();
    if (supported) {
      inbox.scroll_to(96.5); app.step();
      const anchor = rows[3].item, before = inbox.item_bounds(anchor).y;
      for (const control of [add, expand]) {
        const {x, y, width, height} = control.bounds();
        app.pointer('down', x+width/2, y+height/2).pointer('up', x+width/2, y+height/2).step();
        app.step();
        if (inbox.item_bounds(anchor).y !== before) throw new Error('anchor moved');
      }
      if (rows.length !== 21 || rows[0].extent !== 80) throw new Error('callbacks did not update rows');
    }
  }
} finally {app.dispose();}
