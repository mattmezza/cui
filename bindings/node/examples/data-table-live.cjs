'use strict';
const assert = require('node:assert/strict');
const {App, button, column, data_table, label, root_view} = require('../index.cjs');

const args = process.argv.slice(2);
const font = args.find(arg => !arg.startsWith('--'));
const smoke = args.includes('--smoke');
if (smoke && (!font || process.env.CUI_TEST_VIRTUAL_DISPLAY !== '1'))
  throw new Error('--smoke requires a font and the private display from scripts/test_local.py');

const headers = ['Subject', 'Sender'];
const keys = Array.from({length: 12}, (_, i) => 100 + i);
const rows = keys.map(key => [`Project update ${key}`, 'Workspace']);
const inbox = data_table('inbox').table_data(headers, rows, keys).visual_size(360, 184);
const status = label('12 messages');
const load = button('load').label('Load latest').on_click(() => {
  inbox.table_data(headers, [['Release notes', 'Build team'], ['Design review', 'Morgan'], ...rows],
    [203, 204, ...keys]);
  load.disabled();
  status.label('14 messages · Updated just now');
});

const app = new App({width: 480, height: 320, title: 'Live inbox',
  backend: font ? 'sdl' : 'headless', fontPath: font, initiallyHidden: smoke});
try {
  app.mount(root_view().padding(16).children(column().gap(8).children(
    label('Inbox'), load, inbox, status,
  )));
  if (font && !smoke) {
    app.run();
  } else {
    app.step();
    const {x, y} = inbox.table_cell_bounds(1, 0);
    app.scroll(x + 10, y + 10, 0, -1).step();
    const offset = inbox.table_cell_bounds(1, 0).y;
    assert.ok(offset < y);
    const bounds = load.bounds();
    app.pointer('down', bounds.x + bounds.width / 2, bounds.y + bounds.height / 2)
      .pointer('up', bounds.x + bounds.width / 2, bounds.y + bounds.height / 2).step();
    app.step();
    assert.equal(inbox.current_cell(0, 0), 'Release notes');
    assert.equal(inbox.current_cell(3, 0), 'Project update 101');
    assert.equal(inbox.table_cell_bounds(3, 0).y, offset);
    console.log('Live inbox updated and kept the visible row in place.');
  }
} finally {
  app.dispose();
}
