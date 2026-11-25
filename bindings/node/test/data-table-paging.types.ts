import {App, data_table, label} from '../index.cjs';
import type {DataTableCapacityOptions, DataTableLoadMore} from '../index.cjs';

declare const app: App;
declare const request: DataTableLoadMore;
const table = data_table('inbox').configure_load_more({enabled: true, threshold: 1});
const supported: boolean = app.supports_data_table_paging();
const frame: bigint | undefined = table.poll_load_more()?.frameSequence;
const maximum: number | undefined = app.data_table_max_row_capacity();
const capacity: DataTableCapacityOptions = {rowCapacity: Math.min(maximum ?? 64, 64)};
table.table_data(['Subject'], [['Report']], [1], capacity);
void [supported, frame, maximum];
// @ts-expect-error opting into larger capacity requires stable row keys
table.table_data(['Subject'], [], undefined, capacity);
// @ts-expect-error capacity must be a number
table.table_data(['Subject'], [], [], {rowCapacity: '64'});
app.run(active => { if (active === app) active.close(); });
app.run();

// @ts-expect-error a page request is a read-only copy
request.rowCount = 0;
// @ts-expect-error the frame sequence is precisely represented by bigint
const lossy: number = request.frameSequence;
void lossy;
// @ts-expect-error there may be no request to consume
const required: DataTableLoadMore = table.poll_load_more();
void required;
// @ts-expect-error polling is only available for DataTable
label('other').poll_load_more();
// @ts-expect-error threshold is numeric
table.configure_load_more({threshold: '1'});
// @ts-expect-error enabled is boolean
table.configure_load_more({enabled: 1});
// @ts-expect-error the hook is callable
app.run(true);
