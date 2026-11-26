// SPDX-License-Identifier: MIT
'use strict';
const assert = require('node:assert/strict');
const {App, column, label, pagination} = require('../index.cjs');
// Runs headlessly by default. Supply a font path for an interactive native window.
const fontPath = process.argv[2];
const status = label('Page 10 of 20');
const pages = pagination('results').total(20).value(10).design_size('lg').custom_size(52.5).configure({
  counter: true, chevrons: true, availableWidth: 420,
  group: 'Search results pages', previous: 'Previous page', next: 'Next page',
}).on_change(event => status.label(`Page ${event.value} of 20`));
const app = new App({backend: fontPath ? 'sdl' : 'headless', fontPath, width: 480, height: 200});
try {
  app.mount(column().gap(12).children(status, pages));
  if (fontPath) app.run();
  else {
    app.step();
    const {x, y, width, height} = pages.numeric_part_bounds(3);
    assert.ok(width > 0 && height > 0);
    app.pointer('down', x + width / 2, y + height / 2)
      .pointer('up', x + width / 2, y + height / 2).step();
    assert.equal(pages.current_value(), 11);
    assert.equal(status.textValue, 'Page 11 of 20');
    assert.equal(pages.get_config().next, 'Next page');
    pages.configure(null);
    assert.equal(pages.get_config(), undefined);
    console.log('Pagination options: page callback and retained reset verified');
  }
} finally { app.dispose(); }
