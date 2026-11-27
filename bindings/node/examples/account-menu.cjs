'use strict';
const { App, button, column, popover } = require('..');

const app = new App();
try {
  const trigger = button('account').label('Account');
  const close = button('close').label('Close account menu');
  const menu = popover('account-menu').anchor(trigger).children(close);
  if (app.supports_popover_options()) {
    menu.popover_options({ anchor: 0, width: 220, height: 96, margin: 8,
                           hide_close: true, focus_content: true });
  }
  trigger.on_click(() => menu.open());
  close.on_click(() => menu.open(false));
  app.mount(column().children(trigger, menu)).step();

  const { x, y, width, height } = trigger.bounds();
  app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
  app.step();
  if (!menu.is_open()) throw new Error('The Account button should open the menu');

  app.key('down', 'escape').key('up', 'escape').step();
  if (menu.is_open()) throw new Error('Escape should close the account menu');
} finally {
  app.dispose();
}
