// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, button, column, item, label, radio_group, tag} = require('../index.cjs');

const [fontPath, mode] = process.argv.slice(2);
const app = new App({
  backend: fontPath ? 'sdl' : 'headless', fontPath,
  title: 'Delivery options', width: 320, height: 200,
});
try {
  let arrival = 'Friday';
  const summary = label(`Estimated arrival: ${arrival}`);
  const standard = item('standard', 'Standard');
  const decorated = app.supports_radio_item_content();
  const recommendation = tag('recommended', 'Recommended');
  const priority = item('priority', 'Priority').detail('Tracked delivery. Recommended.')
    .children(recommendation).decorate_content(decorated);
  if (app.supports_radio_tooltip()) priority.radio_tooltip('Track your parcel online');
  const delivery = radio_group('delivery').children(standard, priority).select(standard)
    .animate_selection(app.supports_radio_motion())
    .on_change(() => {
      arrival = delivery.selected() === priority ? 'Wednesday' : 'Friday';
      summary.label(`Estimated arrival: ${arrival}`);
    });
  const ordered = app.supports_radio_tab_index();
  if (ordered) delivery.tab_index(-2);
  const reset = button('reset').label('Reset selection').on_click(() => {
    delivery.select(standard);
    arrival = 'Friday';
    summary.label('Estimated arrival: Friday');
  });
  app.mount(column().gap(12).children(label('Delivery service'), reset, delivery, summary));

  if (fontPath) {
    if (mode === '--smoke') app.run(window => window.close());
    else app.run();
  } else {
    app.step();
    const {x, y, width, height} = (decorated ? recommendation : priority).bounds();
    const events = app.pointer('down', x + width / 2, y + height / 2)
      .pointer('up', x + width / 2, y + height / 2).step();
    assert.equal(events, 1);
    assert.equal(delivery.selected(), priority);
    assert.equal(arrival, 'Wednesday');
    if (ordered) {
      app.key('down', 'tab').key('up', 'tab').step();
      const resetEvents = app.key('down', 'enter').key('up', 'enter').step();
      assert.equal(resetEvents, 1);
      assert.equal(delivery.selected(), standard);
      assert.equal(arrival, 'Friday');
    }
  }
} finally {
  app.dispose();
}
