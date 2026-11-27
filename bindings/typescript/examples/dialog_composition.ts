import { App, alert_dialog, button, column, dialog_slot, label } from '../../node/index.cjs';
import type { Button, DialogOptions } from '../../node/index.cjs';

const app = new App({ width: 560, height: 360 });
try {
  if (!app.supports_dialog_composition()) throw new Error('Retained dialog composition requires a current bridge');
  const options: DialogOptions = { centered: true, width: 380 };
  const status = label('Local preferences are active.');
  const confirm = button('Remove account').dialog_action('confirm');
  const cancel = button('Keep account').outline().dialog_action('cancel');
  let accepted = false;
  const alert = alert_dialog('remove', 'Remove this account?').dialog_options(options).children(
    dialog_slot('details', 'body').children(label('Only this device is affected.')),
    dialog_slot('actions', 'footer').children(cancel, confirm),
  ).on_close(event => {
    if (event.result === 1) {
      accepted = true;
      status.label('Local account preferences removed.');
    }
  });
  const trigger = button('Manage account').on_click(() => alert.open());
  app.mount(column().gap(12).children(label('Account settings'), status, trigger, alert)).step();
  const click = (control: Button) => {
    const { x, y, width, height } = control.bounds();
    app.pointer('down', x + width / 2, y + height / 2)
      .pointer('up', x + width / 2, y + height / 2).step();
  };
  click(trigger);
  app.step();
  click(confirm);
  if (!accepted || alert.is_open()) throw new Error('The typed confirmation did not close the dialog');
} finally {
  app.dispose();
}
