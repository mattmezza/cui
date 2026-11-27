'use strict';
const assert = require('node:assert/strict');
const {App, button, column, row, label, rotating_spinner} = require('../index.cjs');
// Run without arguments for a headless interaction check; pass a font for an interactive window.
const font = process.argv[2];
const loading = rotating_spinner('loading');
const status = label('Syncing workspace…');
const finish = button('finish').primary().label('Finish sync');
let completed = false;
finish.on_click(() => {
  loading.remove(); status.label('Workspace is up to date');
  finish.disabled().label('Sync complete'); completed = true;
});
const app = new App({width:480,height:180,backend:font?'sdl':'headless',fontPath:font});
try {
  app.mount(column().gap(12).children(row().gap(8).children(loading,status),finish));
  if (font) app.run();
  else {
    app.step(.1); assert.ok(loading.presentation().nextWakeSeconds > 0);
    const {x,y,width,height} = finish.bounds();
    app.pointer('down',x+width/2,y+height/2).pointer('up',x+width/2,y+height/2).step();
    app.step(); assert.ok(completed);
    console.log('Spinner status: sync completed through pointer interaction');
  }
} finally {app.dispose();}
