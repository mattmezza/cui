// SPDX-License-Identifier: MIT
const {App, row, column, label, button} = require('..');
const headless = process.argv.includes('--headless');
const count = label('Changes: 0');
let changes = 0;
const save = button('save').primary().rounded_md().label('Save changes')
  .on_click(() => count.label(`Changes: ${++changes}`));
const app = new App({backend: headless ? 'headless' : 'sdl', width: 480, height: 240,
  title: 'CUI Node example', fontPath: process.env.CUI_FONT_PATH});
app.mount(column().gap_units(2).children(label('Native CUI'), row().gap_units(2).children(save, count)));
if (headless) {
  app.step();
  const b = save.bounds();
  app.pointer('move', b.x + 2, b.y + 2).pointer('down', b.x + 2, b.y + 2).pointer('up', b.x + 2, b.y + 2);
  app.step();
  console.log(`Saved ${changes} time(s)`);
  app.dispose();
} else {
  app.start();
  process.on('SIGINT', () => { app.stop(); app.dispose(); });
}
