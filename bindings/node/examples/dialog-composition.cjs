'use strict';
// Pass --font /absolute/font.ttf to open an SDL window with a native-enabled addon.
const { App, alert_dialog, button, column, dialog_slot, label } = require('..');
const argv = process.argv.slice(2);
if (argv.length && (argv.length !== 2 || argv[0] !== '--font')) {
  throw new Error('Usage: dialog-composition.cjs [--font /absolute/font.ttf]');
}
const font = argv[1];
const app = new App({ width: 560, height: 360, title: 'Account settings',
  backend: font ? 'sdl' : 'headless', fontPath: font });
try {
  if (!app.supports_dialog_composition()) throw new Error('Retained dialog composition requires a current bridge');
  const status = label('Local account preferences are active.');
  const confirm = button('Remove account').dialog_action('confirm');
  const cancel = button('Keep account').outline().dialog_action('cancel');
  let removed = false;
  const alert = alert_dialog('remove', 'Remove this account?').detail('Only this device is affected.')
    .dialog_options({ centered: true }).children(
      dialog_slot('details', 'body').children(label('Remove saved preferences and account information.')),
      dialog_slot('actions', 'footer').children(cancel, confirm),
    ).on_close(event => {
      if (event.result === 1) {
        status.label('Local account preferences removed.');
        removed = true;
      }
    });
  const trigger = button('Manage account').on_click(() => alert.open());
  app.mount(column().gap(12).children(label('Account settings'), status, trigger, alert));
  if (font) {
    app.run();
  } else {
    const click = control => {
      const { x, y, width, height } = control.bounds();
      app.pointer('down', x + width / 2, y + height / 2)
        .pointer('up', x + width / 2, y + height / 2).step();
    };
    app.step();
    click(trigger);
    app.step();
    click(confirm);
    if (!removed || alert.is_open()) throw new Error('The typed confirmation did not close the dialog');
    console.log('Local account preferences removed.');
  }
} finally {
  app.dispose();
}
