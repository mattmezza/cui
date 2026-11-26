import {App, column, item, virtual_list} from '../index.cjs';
import type {VirtualListRow} from '../index.cjs';
const app = new App({backend: 'headless'});
const message = item('message');
const inbox = virtual_list('inbox').children(message);
app.mount(column().children(inbox));
const rows: readonly VirtualListRow[] = [{item: message, extent: 32}];
if (app.supportsVirtualListRows()) {
  inbox.preserveVirtualListAnchor().setVirtualListRows(rows)
    .appendVirtualListItem(item('new')).removeVirtualListItem(message);
  // @ts-expect-error extents are numeric
  inbox.setVirtualListRows([{item: message, extent: '32'}]);
  // @ts-expect-error row items must be Item nodes
  inbox.appendVirtualListItem(column());
  // @ts-expect-error anchor flag must be boolean
  inbox.preserveVirtualListAnchor(1);
}
app.dispose();
