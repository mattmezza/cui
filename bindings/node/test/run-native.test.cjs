'use strict';
const assert = require('node:assert/strict');
const {App, row, button} = require('..');
assert.equal(process.env.CUI_TEST_VIRTUAL_DISPLAY, '1');
assert.ok(process.env.DISPLAY && process.env.DISPLAY.split('.')[0] !== ':0');
assert.ok(!process.env.WAYLAND_DISPLAY);
for (const mode of ['close','destroy','error','nested']) {
  const app = new App({backend:'sdl',fontPath:process.env.CUI_TEST_FONT});
  let calls=0; const marker=new Error('runner callback marker');
  const target=button('go').label('Go').on_click(()=>{
    calls++;
    if(mode==='error')throw marker;
    if(mode==='nested'){
      assert.throws(()=>app.run(),/running/);
      assert.throws(()=>app.step(),/during run/);
      assert.throws(()=>app.start(),/running/);
    }
    if(mode==='destroy')app.dispose();else app.close();
  });
  try {
    app.mount(row().children(target));app.step();
    const b=target.bounds();app.pointer('down',b.x+2,b.y+2).pointer('up',b.x+2,b.y+2);
    if(mode==='error')assert.throws(()=>app.run(),error=>error===marker);else app.run();
    assert.equal(calls,1);
    if(mode!=='destroy')assert.throws(()=>app.step(),/closed/);
  } finally {app.dispose();}
}
console.log('Node native callback run lifecycle PASS');
