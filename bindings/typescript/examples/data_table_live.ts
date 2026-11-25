import {App, button, column, data_table, label, root_view} from '../../node/index.cjs';
import type {DataTableState} from '../../node/index.cjs';

const headers = ['Subject', 'Sender'] as const;
const keys = Array.from({length: 12}, (_, i) => 100 + i);
const rows = keys.map(key => [`Project update ${key}`, 'Workspace'] as const);
const inbox = data_table('inbox').table_data(headers, rows, keys).visual_size(360, 184);
const status = label('12 messages');
inbox.on_change(() => {
  const selection: DataTableState = inbox.table_state();
  status.label(`Selected message ${selection.row}`);
});
const load = button('load').label('Load latest').on_click(() => {
  inbox.table_data(headers, [['Release notes', 'Build team'], ['Design review', 'Morgan'], ...rows],
    [203, 204, ...keys]);
  load.disabled();
  status.label('14 messages · Updated just now');
});

const app = new App({backend: 'headless', width: 480, height: 320});
try {
  if (!app.supports_keyed_data_table()) throw new Error('This example needs keyed DataTable support');
  app.mount(root_view().padding(16).children(column().gap(8).children(
    label('Inbox'), load, inbox, status,
  ))).step();
  const {x, y} = inbox.table_cell_bounds(1, 0);
  app.scroll(x + 10, y + 10, 0, -1).step();
  const offset = inbox.table_cell_bounds(1, 0).y;
  if (offset >= y) throw new Error('The inbox did not scroll');

  const bounds = load.bounds();
  app.pointer('down', bounds.x + bounds.width / 2, bounds.y + bounds.height / 2)
    .pointer('up', bounds.x + bounds.width / 2, bounds.y + bounds.height / 2).step();
  app.step();
  if (inbox.current_cell(0, 0) !== 'Release notes' ||
      inbox.table_cell_bounds(3, 0).y !== offset)
    throw new Error('The incoming messages changed the visible row position');
  console.log('TypeScript inbox updated while keeping the visible row in place.');
} finally {
  app.dispose();
}
