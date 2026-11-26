// SPDX-License-Identifier: MIT
import {App, column, item, label, radio_group} from '../../node/index.cjs';

const app = new App({backend: 'headless', title: 'Delivery options', width: 320, height: 200});
try {
  const estimate = {arrival: 'Friday'};
  const summary = label(`Estimated arrival: ${estimate.arrival}`);
  const standard = item('standard', 'Standard');
  const priority = item('priority', 'Priority').detail('Tracked delivery');
  const delivery = radio_group('delivery').children(standard, priority).select(standard)
    .animate_selection(app.supports_radio_motion())
    .on_change(() => {
      estimate.arrival = delivery.selected() === priority ? 'Wednesday' : 'Friday';
      summary.label(`Estimated arrival: ${estimate.arrival}`);
    });
  app.mount(column().gap(12).children(label('Delivery service'), delivery, summary)).step();
  const {x, y, width, height} = priority.bounds();
  const events = app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
  if (events !== 1 || delivery.selected() !== priority || estimate.arrival !== 'Wednesday')
    throw new Error('Expected a single delivery callback and an updated arrival estimate');
} finally {
  app.dispose();
}
