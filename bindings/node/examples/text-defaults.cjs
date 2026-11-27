'use strict';
// Headless by default; pass a font path to open the SDL example.
const {App, column, row, icon, label, spinner} = require('../index.cjs');
const font = process.argv[2];
const inner = row().textDefaults(null, 0xe09020ff).children(
  icon('nested-icon', 'check'), label('Inherited size, amber tint'), spinner('busy'));
const root = column().textDefaults(24, 0x3060b0ff).children(
  row().children(icon('outer-icon', 'check'), label('Blue, 24 pixels')), inner,
  row().children(label('Sibling remains blue')));
const app = new App({backend: font ? 'sdl' : 'headless', fontPath: font});
try {
  app.mount(root);
  if (font) {
    app.run();
  } else {
    app.step();
    inner.textDefaults(18, 0);
    app.step();
    inner.textDefaults();
    app.step();
    console.log('Nested text defaults mounted, updated and reset');
  }
} finally {
  app.dispose();
}
