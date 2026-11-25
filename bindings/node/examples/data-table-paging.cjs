'use strict';
const assert = require('node:assert/strict');
const {performance} = require('node:perf_hooks');
const {App, column, data_table, label, root_view} = require('../index.cjs');

const args = process.argv.slice(2);
const larger = args.includes('--larger');
const smoke = args.includes('--smoke');
const fontPath = args.find(value => !value.startsWith('--'));
const headers = ['Subject', 'Sender'];
const keys = Array.from({length: larger ? 40 : 12}, (_, i) => 101 + i);
const rows = keys.map(key => [`Project update ${key}`, 'Workspace']);
const initialCount = 8;
const table = data_table('inbox').visual_size(360, 160).configure_load_more();
const status = label('8 messages');
let matrixOptions;

// The app owns the pending ticket and the transport. Only the UI thread touches CUI.
let generation = 1;
let pending;
function startPage() {
  if (pending) return undefined;
  const demand = table.poll_load_more();
  if (!demand) return undefined;
  table.configure_load_more({enabled: false});
  pending = {generation, frame: demand.frameSequence};
  status.label('Loading the next page…');
  return pending;
}
function retry() {
  generation++;
  pending = undefined;
  table.configure_load_more();
}
function completePage(ticket) {
  if (!pending || pending.generation !== ticket.generation || pending.frame !== ticket.frame)
    return false;
  table.table_data(headers, rows, keys, matrixOptions);
  pending = undefined;
  status.label(`${rows.length} messages · End of inbox`);
  return true;
}

function click(app, {x, y, width, height}) {
  app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2);
}

function verify(app, smoke) {
  const changes = [];
  table.on_change(event => changes.push(event.value));
  app.step();
  assert.equal(startPage(), undefined);
  const {x, y} = table.table_cell_bounds(1, 0);
  app.scroll(x + 10, y + 10, 0, -20).step();
  const cancelled = startPage();
  assert.ok(cancelled);
  assert.equal(startPage(), undefined);
  const selected = table.table_cell_bounds(6, 0);
  click(app, selected);
  app.step();
  assert.deepEqual(changes, [keys[6]]);

  retry();
  app.step();
  const fresh = startPage();
  assert.ok(fresh && fresh.generation !== cancelled.generation);
  assert.equal(completePage(cancelled), false);
  assert.equal(completePage(fresh), true);
  assert.equal(app.step(), 0);
  assert.equal(table.table_state().row, keys[6]);
  assert.deepEqual(table.table_cell_bounds(6, 0), selected);
  const last = rows.length - 1;
  assert.equal(table.current_cell(last, 0), rows[last][0]);
  app.scroll(x + 10, y + 10, 0, -(last + 1)).step();
  assert.equal(startPage(), undefined);
  if (smoke) {
    click(app, table.table_cell_bounds(last - 1, 0));
    let ran = false;
    app.run(active => {
      assert.equal(active, app);
      assert.deepEqual(changes, [keys[6], keys[last - 1]]);
      assert.equal(startPage(), undefined);
      ran = true;
      app.close();
    });
    assert.equal(ran, true);
  }
  console.log('Appended the final page, kept the selected row, and discarded a late response.');
}

if (smoke && (!fontPath || process.env.CUI_TEST_VIRTUAL_DISPLAY !== '1'))
  throw new Error('--smoke requires a font and the private display from scripts/test_local.py');
const app = new App({backend: fontPath ? 'sdl' : 'headless', fontPath,
  initiallyHidden: smoke, width: 480, height: 320, title: 'Paged inbox'});
try {
  if (!app.supports_data_table_paging())
    throw new Error('The inbox requires a bridge with paging');
  if (larger) {
    const maximum = app.data_table_max_row_capacity();
    if (maximum === undefined || maximum < rows.length)
      throw new Error('--larger requires an opt-in capacity of at least 40 rows');
    matrixOptions = {rowCapacity: Math.min(128, maximum)};
  } else if (!app.supports_keyed_data_table()) {
    throw new Error('The inbox requires a bridge with paging and keyed replacement');
  }
  table.table_data(headers, rows.slice(0, initialCount), keys.slice(0, initialCount), matrixOptions);
  app.mount(root_view().padding(16).children(column().gap(8).children(label('Inbox'), table, status)));
  if (!fontPath || smoke) {
    verify(app, smoke);
  } else {
    let readyAt = 0;
    app.run(() => {
      // Simulate a local page delay without blocking the UI or using network I/O.
      if (pending && performance.now() >= readyAt) completePage(pending);
      else if (startPage()) readyAt = performance.now() + 500;
    });
  }
} finally {
  app.dispose();
}
