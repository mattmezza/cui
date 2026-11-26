import {App, column, label, pagination} from '../../node/index.cjs';
import type {PaginationConfig} from '../../node/index.cjs';
const options: PaginationConfig = {
  counter: true, chevrons: true, availableWidth: 420,
  group: 'Search results pages', previous: 'Previous page', next: 'Next page',
};
const status = label('Page 10 of 20');
let changedPage = 10;
const pages = pagination('results').total(20).value(10).design_size('lg').custom_size(52.5).configure(options)
  .on_change(event => { changedPage = event.value; status.label(`Page ${event.value} of 20`); });
const app = new App({backend: 'headless', width: 480, height: 200});
try {
  app.mount(column().gap(12).children(status, pages)).step();
  const {x, y, width, height} = pages.numeric_part_bounds(3);
  if (width <= 0 || height <= 0) throw new Error('Expected Next page bounds');
  app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
  if (pages.current_value() !== 11 || changedPage !== 11 || pages.get_config()?.next !== 'Next page')
    throw new Error('Expected page callback and copied labels');
  pages.configure(null);
  if (pages.get_config() !== undefined) throw new Error('Expected cleared configuration');
  console.log('TypeScript Pagination options: page callback and retained reset verified');
} finally { app.dispose(); }
