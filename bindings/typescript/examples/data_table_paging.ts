import {App, column, data_table, label, root_view} from '../../node/index.cjs';
import type {DataTableLoadMore} from '../../node/index.cjs';

const headers = ['Subject', 'Sender'] as const;
const keys = Array.from({length: 12}, (_, i) => 101 + i);
const rows = keys.map(key => [`Project update ${key}`, 'Workspace'] as const);
const inbox = data_table('inbox').table_data(headers, rows.slice(0, 8), keys.slice(0, 8))
  .visual_size(360, 160).configure_load_more({threshold: 0});
const status = label('8 messages');
let pending: DataTableLoadMore | undefined;

function pollPage(): DataTableLoadMore | undefined {
  if (pending) return undefined;
  const request = inbox.poll_load_more();
  if (request) {
    inbox.configure_load_more({enabled: false});
    status.label('Loading the next page…');
    pending = request;
  }
  return request;
}

// The local source delivers one final page. Apply it on the UI thread.
function completePage(request: DataTableLoadMore) {
  if (pending !== request) return;
  inbox.table_data(headers, rows, keys);
  pending = undefined;
  status.label('12 messages · End of inbox');
}

const app = new App({backend: 'headless', width: 480, height: 320});
try {
  if (!app.supports_data_table_paging() || !app.supports_keyed_data_table())
    throw new Error('The inbox requires a bridge with paging and keyed replacement');
  app.mount(root_view().padding(16).children(column().gap(8).children(
    label('Inbox'), inbox, status,
  ))).step();
  if (pollPage()) throw new Error('The first viewport should not require another page');
  const {x, y} = inbox.table_cell_bounds(1, 0);
  app.scroll(x + 10, y + 10, 0, -20).step();
  const request = pollPage();
  if (!request || request.frameSequence <= 0n || request.rowCount !== 8 || pollPage())
    throw new Error('Expected exactly one page request for the eight bound rows');

  const anchor = inbox.table_cell_bounds(request.visibleBegin, 0).y;
  completePage(request);
  app.step();
  if (inbox.table_cell_bounds(request.visibleBegin, 0).y !== anchor ||
      inbox.current_cell(11, 0) !== rows[11][0] || pollPage())
    throw new Error('Expected the final page in place with no more requests');
  console.log('TypeScript inbox appended a page and preserved the viewport.');
} finally {
  app.dispose();
}
