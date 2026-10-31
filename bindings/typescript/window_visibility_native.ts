import {App, row, button} from './index';
declare const process: {env: {[key: string]: string | undefined}};
function check(value: boolean): void {if (!value) throw new Error('visibility proof failed');}
check(process.env.CUI_TEST_VIRTUAL_DISPLAY === '1');
check(!!process.env.DISPLAY && process.env.DISPLAY.split('.')[0] !== ':0');
check(!process.env.WAYLAND_DISPLAY);
for (const hidden of [true, false]) {
  const app = new App({backend:'sdl', fontPath:process.env.CUI_TEST_FONT, initiallyHidden:hidden});
  try {
    check(app.window_visible() === !hidden);
    let calls = 0;
    const target = button('show').label('Show').on_click(() => {
      check(app.window_visible() === !hidden);
      app.set_window_visible(true);check(app.window_visible());
      app.set_window_visible(false);check(!app.window_visible());
      calls++;app.close();
    });
    app.mount(row().children(target));app.step();
    check(app.window_visible() === !hidden);
    const b = target.bounds();
    app.pointer('down', b.x+2,b.y+2).pointer('up',b.x+2,b.y+2);
    app.run();check(calls === 1);
  } finally {app.dispose();}
}
console.log('TypeScript compiled native window visibility PASS');
