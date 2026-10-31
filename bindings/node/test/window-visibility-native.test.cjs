'use strict';
const assert = require('node:assert/strict');
const {App, row, button} = require('..');
assert.equal(process.env.CUI_TEST_VIRTUAL_DISPLAY, '1');
assert.ok(process.env.DISPLAY && process.env.DISPLAY.split('.')[0] !== ':0');
assert.ok(!process.env.WAYLAND_DISPLAY);
assert.throws(()=>new App({initiallyHidden:true}), /backend unavailable/);
assert.throws(()=>new App({initiallyHidden:1}), /boolean/);
for(const hidden of [true,false]) {
  const app=new App({backend:'sdl', fontPath:process.env.CUI_TEST_FONT, initiallyHidden:hidden});
  try {
    assert.equal(app.window_visible(), !hidden);
    let calls=0;
    const target=button('show').label('Show').on_click(()=>{
      assert.equal(app.window_visible(), !hidden);
      app.set_window_visible(true);assert.equal(app.window_visible(),true);
      app.set_window_visible(false);assert.equal(app.window_visible(),false);
      calls++;app.close();
    });
    app.mount(row().children(target));app.step();
    assert.equal(app.window_visible(),!hidden);
    const b=target.bounds();app.pointer('down',b.x+2,b.y+2).pointer('up',b.x+2,b.y+2);
    app.run();assert.equal(calls,1);
    assert.throws(()=>app.window_visible(),/closed/);
  } finally {app.dispose();}
}
console.log('JavaScript window visibility PASS');
