import {App, button, column, type ButtonFocus} from '../../node/index.cjs';

const app = new App({backend: 'headless'});
try {
  const save = button('save').label('Save').on_click(() => console.log('Saved'));
  const treatment: ButtonFocus = 'outer-ring';
  if (app.supports_button_focus()) save.focus_treatment(treatment);
  app.mount(column().children(save)).step();
} finally {
  app.dispose();
}
