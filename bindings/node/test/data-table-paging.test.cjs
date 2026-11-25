'use strict';
const assert = require('node:assert/strict');
const {test} = require('node:test');
const {App, column, data_table, label} = require('../index.cjs');

const headers = ['Subject'];
const rows = Array.from({length: 8}, (_, i) => [`Message ${i}`]);
const keys = [11, 22, 33, 44, 55, 66, 77, 0xffffffff];
const fixture = process.env.CUI_DATA_TABLE_PAGING_FIXTURE;

function fallback() {
  const app = new App({maxNodes: 2});
  try {
    assert.equal(app.supports_data_table_paging(), false);
    const table = data_table().table_data(headers, rows).configure_load_more();
    const root = column().children(table);
    assert.throws(() => app.mount(root), /paging is unavailable/);
    table.configure_load_more({enabled: false});
    app.mount(root).step();
    assert.throws(() => table.configure_load_more(), /paging is unavailable/);
    assert.throws(() => table.poll_load_more(), /paging is unavailable/);
    const {x, y} = table.table_cell_bounds(0, 0);
    app.pointer('down', x + 10, y + 10).pointer('up', x + 10, y + 10).step();
    assert.equal(table.table_state().row, 1);
  } finally {
    app.dispose();
  }
}

if (['legacy', 'partial', 'size'].includes(fixture)) {
  test(`ordinary DataTable works with a ${fixture} provider`, fallback);
} else if (fixture) {
  test(`N-API validates the ${fixture} provider record`, () => {
    const app = new App();
    try {
      assert.equal(app.supports_data_table_paging(), true);
      const table = data_table().table_data(headers, rows).configure_load_more();
      app.mount(column().children(table)).step();
      if (fixture === 'exact') {
        assert.deepEqual(table.poll_load_more(), {frameSequence: 0xfffffffffffffffen,
          rowCount: 8, visibleBegin: 4, visibleEnd: 8, lastRowKey: 0xffffffff, capacity: 32});
      } else {
        assert.throws(() => table.poll_load_more(), /malformed DataTable load-more record/);
      }
    } finally {
      app.dispose();
    }
  });
} else {
  test('a missing addon method keeps ordinary DataTable available', () => {
    const addon = require(process.env.CUI_NODE_ADDON);
    const configure = addon.configureDataTableLoadMore;
    try {
      addon.configureDataTableLoadMore = undefined;
      fallback();
    } finally {
      addon.configureDataTableLoadMore = configure;
    }
  });

  test('paging opt-in and native frame hook validate input before attaching', () => {
    const app = new App({maxNodes: 2});
    try {
      assert.equal(app.supports_data_table_paging(), true);
      assert.throws(() => app.run(null), /hook must be a function/);
      assert.throws(() => label('other').configure_load_more(), /requires DataTable/);
      const table = data_table();
      assert.throws(() => table.poll_load_more(), /mounted DataTable/);
      for (const options of [null, 2, [], false])
        assert.throws(() => table.configure_load_more(options), /options object/);
      assert.throws(() => table.configure_load_more({enabled: 1}), /enabled must be boolean/);
      for (const threshold of [-1, 33, 1.5, NaN, true, '0', 0n])
        assert.throws(() => table.configure_load_more({threshold}), /threshold/);
      const root = column().children(table.configure_load_more());
      assert.throws(() => app.mount(root), /requires column headers/);
      table.table_data(headers, [], []);
      app.mount(root).step();
      const firstPage = table.poll_load_more();
      assert.ok(firstPage.frameSequence > 0n);
      assert.deepEqual([firstPage.visibleBegin, firstPage.visibleEnd, firstPage.rowCount,
        firstPage.lastRowKey], [0, 0, 0, 0]);
    } finally {
      app.dispose();
    }
  });

  test('a regular callback requesting close suppresses the native frame hook', () => {
    const addon = require(process.env.CUI_NODE_ADDON);
    const run = addon.run;
    const app = new App();
    try {
      // Drive a successful native frame on the headless host, then its run callback.
      addon.run = (_handle, callback) => callback();
      let closedInHandler = false;
      const table = data_table().table_data(headers, rows).on_change(() => {
        closedInHandler = true;
        app.close();
      });
      app.mount(column().children(table)).step();
      const {x, y} = table.table_cell_bounds(0, 0);
      app.pointer('down', x + 10, y + 10).pointer('up', x + 10, y + 10).step_only();
      app.run(() => assert.fail('the hook ran after close'));
      assert.equal(closedInHandler, true);
    } finally {
      app.dispose();
      addon.run = run;
    }
  });

  test('one copied edge survives later frames and shutdown without consuming events', () => {
    const app = new App({width: 400, height: 300});
    const table = data_table().table_data(headers, rows, keys).visual_size(320, 160);
    let request;
    try {
      app.mount(column().children(table)).step();
      const {x, y} = table.table_cell_bounds(1, 0);
      app.scroll(x + 10, y + 10, 0, -20).step_only();
      assert.equal(table.poll_load_more(), undefined);
      table.configure_load_more();
      app.step_only();
      const cell = table.table_cell_bounds(6, 0);
      app.pointer('down', cell.x + 10, cell.y + 10).pointer('up', cell.x + 10, cell.y + 10)
        .step_only();
      request = table.poll_load_more();
      assert.ok(Object.isFrozen(request) && request.frameSequence > 0n);
      assert.equal(request.rowCount, 8);
      assert.equal(request.visibleEnd, 8);
      assert.ok(request.visibleBegin < request.visibleEnd);
      assert.equal(request.lastRowKey, 0xffffffff);
      assert.equal(request.capacity, 32);
      assert.equal(table.poll_load_more(), undefined);
      const event = app.poll_value_notification_v1();
      assert.equal(event.kind, 'table-change');
      assert.equal(event.row, keys[6]);
      table.table_data(headers, [...rows, ['New']], [...keys, 120]);
      app.step_only();
      assert.equal(table.table_cell_bounds(6, 0).y, cell.y);
      assert.equal(table.table_state().row, keys[6]);
      assert.equal(app.poll_value_notification_v1(), undefined);
      table.remove();
      assert.throws(() => table.poll_load_more(), /mounted DataTable/);
    } finally {
      app.dispose();
    }
    assert.equal(request.lastRowKey, 0xffffffff);
    assert.throws(() => { request.capacity = 1; }, TypeError);
  });
}
