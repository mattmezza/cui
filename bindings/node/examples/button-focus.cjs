'use strict';
const {App, button, column} = require('..');
const app = new App({backend: 'headless'});
try {
  const save = button('save').label('Save').on_click(() => console.log('Saved'));
  if (app.supports_button_focus()) save.focus_treatment('outer-ring');
  app.mount(column().children(save)).step();
  // Mounted Buttons can switch to 'variant' or clear with 'legacy'.
} finally { app.dispose(); }
