"use strict";
const assert=require('node:assert/strict'),c=require('../index.cjs');
if(process.env.CUI_TEST_VIRTUAL_DISPLAY!=='1'||process.env.WAYLAND_DISPLAY||!process.env.DISPLAY||process.env.DISPLAY===':0')throw new Error('Requires isolated virtual display');
const app=new c.App({backend:'sdl',fontPath:process.env.CUI_TEST_FONT});
try {app.mount(c.root_view().children(c.label('Clipboard round trip'))).step();for(const text of ['Unicode ✓ é','é'.repeat(32768),'']){app.clipboard_set(text);assert.equal(app.clipboard_get(),text);}}finally{app.dispose();}
console.log('PASS Node SDL clipboard Unicode/64KiB/empty round trips');
