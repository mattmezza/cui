'use strict';
const assert = require('node:assert/strict');
const {test} = require('node:test');
const {App, column, data_table} = require('../index.cjs');
const addon = require(process.env.CUI_NODE_ADDON);

const headers = ['Subject'];
const rows = Array.from({length: 40}, (_, i) => [`Message ${i}`]);
const keys = Array.from({length: 40}, (_, i) => i === 39 ? 0xffffffff : i + 100);
const fixture = process.env.CUI_DATA_TABLE_CAPACITY_FIXTURE;

function checkProvider(maximum) {
  const app = new App({maxNodes: 2});
  const requested = 96;
  const table = data_table().table_data(headers, [], [], {rowCapacity: requested});
  const root = column().children(table);
  let creates = 0;
  const makeNode = addon.makeNode;
  try {
    assert.equal(app.data_table_max_row_capacity(), maximum);
    addon.makeNode = (...args) => { creates++; return makeNode(...args); };
    assert.throws(() => app.mount(root), /capacity is unavailable|provider's 48-row limit/);
    assert.equal(creates, 0);
    table.table_data(headers, [['Ordinary']]);
    app.mount(root).step();
    const bomb = [];
    Object.defineProperty(bomb, 0, {get() { assert.fail('rows copied before provider preflight'); }});
    assert.throws(() => table.table_data(headers, bomb, [8], {rowCapacity: requested}),
      /capacity is unavailable|provider's 48-row limit/);
    assert.equal(table.current_cell(0, 0), 'Ordinary');
    if (maximum) {
      table.table_data(headers, rows, keys, {rowCapacity: maximum});
      assert.equal(table.current_cell(39, 0), 'Message 39');
    }
  } finally {
    addon.makeNode = makeNode;
    app.dispose();
  }
}

if (fixture === 'large') {
  test('a larger future provider is bounded by the addon limit', () => {
    const app = new App();
    try {
      assert.equal(app.data_table_max_row_capacity(), 10000);
    } finally {
      app.dispose();
    }
  });
} else if (fixture) {
  test(`ordinary tables survive the ${fixture} capacity provider`, () => {
    checkProvider(fixture === 'limited' ? 48 : undefined);
  });
} else {
  test('both addon methods are required before opting into capacity', () => {
    for (const name of ['setMatrixCapacity', 'maxMatrixCapacity']) {
      const saved = addon[name];
      try {
        addon[name] = undefined;
        checkProvider(undefined);
      } finally {
        addon[name] = saved;
      }
    }
  });

  test('paging past 32 rows preserves selection, position and copied state', () => {
    const app = new App({width: 420, height: 320});
    const selected = [];
    const table = data_table().table_data(headers, rows.slice(0, 32), keys.slice(0, 32),
      {rowCapacity: 96}).visual_size(300, 160).configure_load_more()
      .on_change(event => selected.push(event.value));
    try {
      assert.equal(app.data_table_max_row_capacity(), 10000);
      app.mount(column().children(table)).step();
      const {x, y} = table.table_cell_bounds(1, 0);
      app.scroll(x + 10, y + 10, 0, -100).step();
      const cell = table.table_cell_bounds(30, 0);
      const px = cell.x + 10, py = cell.y + 10;
      app.pointer('down', px, py).pointer('up', px, py).step();
      assert.deepEqual(selected, [keys[30]]);

      const bind = addon.setMatrixCapacity;
      try {
        addon.setMatrixCapacity = () => { throw new Error('rejected matrix'); };
        assert.throws(() => table.table_data(headers, rows, keys, {rowCapacity: 64}), /rejected matrix/);
      } finally {
        addon.setMatrixCapacity = bind;
      }
      assert.equal(table.current_cell(31, 0), 'Message 31');
      const first = table.poll_load_more();
      assert.equal(first.rowCount, 32);
      assert.equal(first.capacity, 96);
      assert.equal(table.table_state().row, keys[30]);

      const copied = rows.map(row => [...row]);
      table.table_data(headers, copied, keys, {rowCapacity: 64});
      copied[39][0] = 'Changed after submission';
      assert.equal(app.step(), 0);
      assert.deepEqual(table.table_cell_bounds(30, 0), cell);
      assert.equal(table.table_state().row, keys[30]);
      assert.equal(table.current_cell(39, 0), 'Message 39');
      assert.equal(table.poll_load_more(), undefined);
      app.scroll(x + 10, y + 10, 0, -100).step();
      const next = table.poll_load_more();
      assert.ok(Object.isFrozen(next) && next.frameSequence > first.frameSequence);
      assert.deepEqual([next.rowCount, next.visibleEnd, next.lastRowKey, next.capacity],
        [40, 40, 0xffffffff, 64]);
      assert.equal(table.poll_load_more(), undefined);

      table.table_data(headers, rows, keys, {rowCapacity: 40});
      app.step();
      assert.equal(table.poll_load_more(), undefined);
      table.table_data(headers, [], []);
      app.step();
      assert.equal(table.poll_load_more().capacity, 32);
    } finally {
      app.dispose();
    }
  });

  test('capacity inputs are bounded and a mismatched wire capacity is rejected', () => {
    const table = data_table();
    for (const rowCapacity of [0, -1, 1.25, true, '40', 10001, NaN, 1n])
      assert.throws(() => table.table_data(headers, [], [], {rowCapacity}), /rowCapacity/);
    for (const options of [null, false, 40, []])
      assert.throws(() => table.table_data(headers, [], [], options), /options object/);
    assert.throws(() => table.table_data(headers, [], undefined, {rowCapacity: 48}), /rowKeys/);
    assert.throws(() => table.table_data(headers, rows, keys), /invalid table matrix/);
    assert.throws(() => table.table_data(headers, rows, keys, {rowCapacity: 39}), /invalid table matrix/);

    // A JavaScript Proxy can return different lengths on successive reads.
    let lengthReads = 0;
    const oneRow = new Proxy([['Bounded']], {get(target, name) {
      if (name === 'length') return ++lengthReads === 1 ? 1 : 10001;
      return Reflect.get(target, name);
    }});
    table.table_data(headers, oneRow, [1], {rowCapacity: 48});
    assert.equal(lengthReads, 1);
    assert.equal(table.current_cell(0, 0), 'Bounded');

    const app = new App();
    const poll = addon.pollDataTableLoadMore;
    try {
      const maximum = app.data_table_max_row_capacity();
      const allRows = Array.from({length: maximum}, (_, index) => [String(index)]);
      table.table_data(headers, allRows, allRows.map((_, i) => i + 1), {rowCapacity: maximum});
      app.mount(column().children(table)).step();
      assert.equal(table.current_cell(maximum - 1, 0), String(maximum - 1));
      addon.pollDataTableLoadMore = () => ({frameSequence: 1n, rowCount: 1,
        visibleBegin: 0, visibleEnd: 1, lastRowKey: 1, capacity: maximum - 1});
      assert.throws(() => table.poll_load_more(), /malformed DataTable load-more record/);
    } finally {
      addon.pollDataTableLoadMore = poll;
      app.dispose();
    }
  });

  test('raw N-API validates UTF-8 lengths and preserves a JavaScript getter error', () => {
    const app = addon.create({backend: 0, width: 400, height: 320, maxNodes: 2, maxEvents: 4});
    try {
      const root = addon.makeNode(app, null, 2, 'root');
      const table = addon.makeNode(app, root, 71, 'inbox');
      const set = (data, ids, capacity = 48) => addon.setMatrixCapacity(app, table, headers, data, ids, capacity);
      set([['Stored']], [8]);
      assert.throws(() => set([['é'.repeat(65)]], [8]), /128 UTF-8 bytes/);
      assert.throws(() => set([['nul\0text']], [8]), /embedded NUL/);
      assert.throws(() => set(rows, keys, 1), /array/);
      const marker = new Error('getter threw');
      const broken = [['First']];
      Object.defineProperty(broken, 1, {get() { throw marker; }});
      assert.throws(() => set(broken, [8, 9]), thrown => thrown === marker);
      assert.equal(addon.getMatrixCell(app, table, 0, 0), 'Stored');
      set([['Accepted']], [8]);
      assert.equal(addon.getMatrixCell(app, table, 0, 0), 'Accepted');
    } finally {
      addon.dispose(app);
    }
  });
}
