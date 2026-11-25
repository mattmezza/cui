import {App, data_table} from '../index.cjs';

const headers = ['Subject', 'Sender'] as const;
const rows = [['New message', 'Workspace']] as const;
const table = data_table('inbox').table_data(headers, rows, [7] as const).selection_mode('cell');
const supported: boolean = new App().supports_keyed_data_table();
void supported;

// @ts-expect-error keys use numbers, not strings
table.table_data(headers, rows, ['7']);
// @ts-expect-error row keys are supplied as an array
table.table_data(headers, rows, 7);
// @ts-expect-error selection mode names are closed
table.selection_mode('multiple');

table.table_data(headers, rows);
