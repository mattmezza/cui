'use strict';
const assert = require('node:assert/strict');
const {App, button, column, row, label, rotating_spinner, IconSource} = require('../index.cjs');
// Run without arguments for a headless interaction check; pass a font for an interactive window.
const font = process.argv[2];
const loading = rotating_spinner('loading');
const status = label('Syncing workspace…');
const finish = button('finish').primary().label('Finish sync');
let completed = false;
let artwork;
finish.on_click(() => {
  if (artwork) { loading.set_icon_resource(null); artwork.close(); }
  loading.remove(); status.label('Workspace is up to date');
  finish.disabled().label('Sync complete'); completed = true;
});
const app = new App({width:480,height:180,backend:font?'sdl':'headless',fontPath:font});
try {
  app.mount(column().gap(12).children(row().gap(8).children(loading,status),finish));
  // Shared SVG artwork is an optional mounted-only extension; the catalog Loader
  // remains available with bridge providers that predate this extension.
  if (app.supports_spinner_icon_resource()) {
    artwork = app.icon_resource(IconSource.svg('<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><path d="M8 1a7 7 0 0 1 7 7h-3a4 4 0 0 0-4-4z"/></svg>'));
    loading.set_icon_resource(artwork);
  }
  if (font) app.run();
  else {
    app.step(.1); assert.ok(loading.presentation().nextWakeSeconds > 0);
    const {x,y,width,height} = finish.bounds();
    app.pointer('down',x+width/2,y+height/2).pointer('up',x+width/2,y+height/2).step();
    app.step(); assert.ok(completed);
    console.log('Spinner status: sync completed through pointer interaction');
  }
} finally {app.dispose();}
