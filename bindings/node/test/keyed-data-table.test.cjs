'use strict';
const assert = require('node:assert/strict');
const {test} = require('node:test');
const {App, column, data_table} = require('../index.cjs');

function press(app, {x, y, width, height}) {
  app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
}

if (process.env.CUI_KEYED_DATA_TABLE_LEGACY === '1') {
  test('an older provider keeps ordinary tables usable after a keyed request fails', () => {
    const app = new App({maxNodes: 2});
    try {
      assert.equal(app.supports_keyed_data_table(), false);
      const table = data_table('inbox').table_data(['Subject'], [['Keyed']], [7]);
      const root = column().children(table);
      assert.throws(() => app.mount(root), /keyed DataTable.*unavailable/);
      assert.equal(table.current_cell(0, 0), 'Keyed');

      table.table_data(['Subject'], [['Legacy']]);
      app.mount(root).step();
      assert.throws(() => table.table_data(['Subject'], [['Rejected']], [7]),
        /keyed DataTable.*unavailable/);
      assert.equal(table.current_cell(0, 0), 'Legacy');
      press(app, table.table_cell_bounds(0, 0));
      assert.equal(table.table_state().row, 1);
    } finally {
      app.dispose();
    }
  });
} else {
  test('preserves the visible row, selected key and keyboard focus through replacement', () => {
    const headers = ['Subject', 'Sender'];
    const rows = Array.from({length: 8}, (_, i) => [`Message ${i}`, 'Team']);
    const keys = [11, 22, 0xffffffff, 44, 55, 66, 77, 88];
    const changes = [];
    const initialKeys = keys.slice();
    const table = data_table('inbox').table_data(headers, rows, keys)
      .visual_size(320, 160).selection_mode('cell').on_change(e => changes.push(JSON.parse(e.text)));
    keys[2] = 999;
    const app = new App();
    try {
      assert.equal(app.supports_keyed_data_table(), true);
      app.mount(column().children(table)).step();
      const {x, y} = table.table_cell_bounds(1, 0);
      app.scroll(x + 10, y + 10, 0, -1).step();
      const anchoredY = table.table_cell_bounds(1, 0).y;
      assert.equal(anchoredY, y - 48);

      const selected = table.table_cell_bounds(2, 1);
      press(app, selected);
      assert.equal(table.table_state().row, 0xffffffff);
      assert.equal(changes.pop().row, 0xffffffff);
      for (const invalid of [null, [false], [0], [-1], [2 ** 32], [1.5], [], Array(1)])
        assert.throws(() => table.table_data(headers, rows.slice(0, 1), invalid), /rowKeys/);
      assert.throws(() => table.table_data(headers, rows.slice(0, 2), [44, 44]), /rowKeys/);
      assert.throws(() => table.table_data(headers, Array(1), [44]), /invalid table matrix/);
      assert.throws(() => table.table_data(headers, [['x'.repeat(129), 'Team']], [44]),
        /invalid table matrix/);
      assert.equal(table.current_cell(2, 0), 'Message 2');
      assert.equal(table.table_state().row, 0xffffffff);

      table.table_data(headers, [['New message', 'Owner'], ...rows], [9, ...initialKeys]);
      app.step();
      assert.equal(table.table_cell_bounds(2, 0).y, anchoredY);
      assert.deepEqual(table.table_cell_bounds(3, 1), selected);
      assert.equal(table.table_state().row, 0xffffffff);
      assert.deepEqual(changes, []);

      app.key('down', 'down').step();
      assert.equal(table.table_state().row, 44);
      assert.equal(changes.pop().row, 44);

      table.table_data(headers, [], []);
      app.step();
      assert.equal(table.table_state().row, 0);
      table.table_data(headers, rows);
      app.step();
      assert.equal(table.table_cell_bounds(1, 0).y, y);
      press(app, table.table_cell_bounds(1, 1));
      assert.equal(table.table_state().row, 2);
    } finally {
      app.dispose();
    }
  });
}
