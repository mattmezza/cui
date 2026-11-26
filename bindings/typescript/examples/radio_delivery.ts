// SPDX-License-Identifier: MIT
import {App, button, column, item, label, radio_group, tag} from '../../node/index.cjs';

const app = new App({backend: 'headless', title: 'Delivery options', width: 320, height: 200});
try {
  const estimate = {arrival: 'Friday'};
  const summary = label(`Estimated arrival: ${estimate.arrival}`);
  const standard = item('standard', 'Standard');
  const decorated = app.supports_radio_item_content();
  const recommendation = tag('recommended', 'Recommended');
  const priority = item('priority', 'Priority').detail('Tracked delivery. Recommended.')
    .children(recommendation).decorate_content(decorated);
  if (app.supports_radio_tooltip()) priority.radio_tooltip('Track your parcel online');
  const delivery = radio_group('delivery').children(standard, priority).select(standard)
    .animate_selection(app.supports_radio_motion())
    .on_change(() => {
      estimate.arrival = delivery.selected() === priority ? 'Wednesday' : 'Friday';
      summary.label(`Estimated arrival: ${estimate.arrival}`);
    });
  const ordered = app.supports_radio_tab_index();
  if (ordered) delivery.tab_index(-2);
  const reset = button('reset').label('Reset selection').on_click(() => {
    delivery.select(standard);
    estimate.arrival = 'Friday';
    summary.label('Estimated arrival: Friday');
  });
  app.mount(column().gap(12).children(label('Delivery service'), reset, delivery, summary)).step();
  const {x, y, width, height} = (decorated ? recommendation : priority).bounds();
  function verifyDelivery(events: number, item: typeof standard, arrival: string) {
    if (events !== 1 || delivery.selected() !== item || estimate.arrival !== arrival)
      throw new Error(`Expected one action and arrival on ${arrival}`);
  }
  const events = app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
  verifyDelivery(events, priority, 'Wednesday');
  if (ordered) {
    app.key('down', 'tab').key('up', 'tab').step();
    const resetEvents = app.key('down', 'enter').key('up', 'enter').step();
    verifyDelivery(resetEvents, standard, 'Friday');
  }
} finally {
  app.dispose();
}
