import {App, pagination, rating, label} from '../index.cjs';
import type {PaginationConfig} from '../index.cjs';
const config: PaginationConfig = {counter: true, iconOnly: false, chevrons: true, jumpMenu: true,
  visiblePages: 7, availableWidth: 200, group: 'Pages', earlierPages: 'Earlier', laterPages: 'Later'};
const pages = pagination().configure(config).design_size('lg');
pages.on_change(event => { const page: number = event.value; pages.value(page); });
const copied: Readonly<Required<PaginationConfig>> | undefined = pages.get_config();
if (copied) {
  const next: string = copied.next;
  const counter: boolean = copied.counter;
  void next; void counter;
  // @ts-expect-error copied configuration is immutable
  copied.counter = false;
}
pages.configure(null).reset_config();
new App().supports_pagination_options();
// @ts-expect-error booleans are strict
pages.configure({counter: 1});
// @ts-expect-error width is numeric
pages.configure({availableWidth: 'wide'});
// @ts-expect-error labels are strings
pages.configure({laterPages: false});
// @ts-expect-error design size is a named enum
pages.design_size(3);
// @ts-expect-error Rating has no Pagination configuration
rating().configure(config);
// @ts-expect-error Rating has no Pagination configuration getter
rating().get_config();
// @ts-expect-error plain Label has no Pagination configuration
label('status').configure(config);
