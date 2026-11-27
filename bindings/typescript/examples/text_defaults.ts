import {App, column, row, icon, label, spinner} from '../../node/index.cjs';
const nested = row().textDefaults(null, 0xe09020ff).children(
  icon('status', 'check'), label('Inherited size, amber tint'), spinner('busy'));
const app = new App({backend: 'headless'});
try {
  app.mount(column().textDefaults(24, 0x3060b0ff).children(
    row().children(icon('outer', 'check'), label('Blue, 24 pixels')), nested,
    row().children(label('Sibling remains blue'))));
  app.step();
  nested.textDefaults(18, 0);
  app.step();
  nested.textDefaults();
  app.step();
} finally {
  app.dispose();
}
