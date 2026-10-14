const {App, button, column, label} = require('@cui/native');
let count = 0;
const status = label('Clicks: 0');
const click = button('increment').label('Click me').on_click(() => {
  status.label(`Clicks: ${++count}`);
});
const app = new App({backend: 'sdl', title: 'CUI Counter', fontPath: process.env.CUI_FONT});
app.mount(column().children(status, click));
if (process.env.CUI_EXAMPLE_SMOKE === '1') {
  app.step();
  const {x, y, width, height} = click.bounds();
  app.pointer('down', x + width / 2, y + height / 2);
  app.pointer('up', x + width / 2, y + height / 2);
  app.step();
  if (count !== 1) throw new Error('counter callback failed');
  app.step();
  app.dispose();
} else {
  app.start();
}
