// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, column, item, label, radio_group} = require('../index.cjs');

const [fontPath, mode] = process.argv.slice(2);
const app = new App({
  backend: fontPath ? 'sdl' : 'headless', fontPath,
  title: 'Delivery options', width: 320, height: 200,
});
try {
  let arrival = 'Friday';
  const summary = label(`Estimated arrival: ${arrival}`);
  const standard = item('standard', 'Standard');
  const priority = item('priority', 'Priority').detail('Tracked delivery');
  const delivery = radio_group('delivery').children(standard, priority).select(standard)
    .animate_selection(app.supports_radio_motion())
    .on_change(() => {
      arrival = delivery.selected() === priority ? 'Wednesday' : 'Friday';
      summary.label(`Estimated arrival: ${arrival}`);
    });
  app.mount(column().gap(12).children(label('Delivery service'), delivery, summary));

  if (fontPath) {
    if (mode === '--smoke') app.run(window => window.close());
    else app.run();
  } else {
    app.step();
    const {x, y, width, height} = priority.bounds();
    const events = app.pointer('down', x + width / 2, y + height / 2)
      .pointer('up', x + width / 2, y + height / 2).step();
    assert.equal(events, 1);
    assert.equal(delivery.selected(), priority);
    assert.equal(arrival, 'Wednesday');
  }
} finally {
  app.dispose();
}
