import {App, data_table, label} from '../index.cjs';
import type {DataTableLoadMore} from '../index.cjs';

declare const app: App;
declare const request: DataTableLoadMore;
const table = data_table('inbox').configure_load_more({enabled: true, threshold: 1});
const supported: boolean = app.supports_data_table_paging();
const frame: bigint | undefined = table.poll_load_more()?.frameSequence;
void [supported, frame];
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
