'use strict';
const c = require('../index.cjs');
const headless = process.argv.includes('--headless');
const status = c.label('Ready');
const name = c.input('name', 'Alex Morgan');
const email = c.checkbox('email', 'Email notifications').checked();
const save = c.button('save').label('Save changes').primary().rounded_md()
  .on_click(() => status.label('Saved account: ' + name.current_text()));
const account = c.item('account-page', 'Account').children(
  c.column().gap_units(3).children(c.label('Account details'), name, save, status));
const preferences = c.item('preferences-page', 'Preferences').children(
  c.column().gap_units(3).children(c.label('Notifications'), email));
const pages = c.tabs('pages').children(account, preferences).select(account);
const accountAction = c.item('account-action', 'Account')
  .on_click(() => pages.select(account));
const preferencesAction = c.item('preferences-action', 'Preferences')
  .on_click(() => pages.select(preferences));
const navigation = c.sidebar('navigation').visual_size(180, 280)
  .children(accountAction, preferencesAction);
const root = c.root_view().padding(24).children(
  c.column().gap_units(4).children(c.label('CUI workspace'),
    c.row().gap_units(6).children(navigation, pages)));
const app = new c.App({
  width: 820, height: 460, title: 'CUI sidebar workspace',
  backend: headless ? 'headless' : 'sdl',
  fontPath: headless ? undefined : process.env.CUI_FONT_PATH,
});
app.mount(root);
if (headless) {
  const assert = require('node:assert/strict');
  function press(node) {
    const {x, y, width, height} = node.bounds();
    assert.ok(width > 0 && height > 0);
    app.pointer('down', x + width/2, y + height/2)
      .pointer('up', x + width/2, y + height/2).step();
    app.step();
  }
  app.step();
  press(preferencesAction);
  assert.equal(pages.selected(), preferences);
  assert.ok(email.bounds().width > 0);
  press(accountAction);
  press(name);
  app.text_input('!').step();
  press(save);
  assert.equal(status.textValue, 'Saved account: ' + name.current_text());
  assert.ok(name.current_text().endsWith('!'));
  app.dispose();
  console.log('PASS sidebar workspace: navigation, native input and save callback');
} else {
  app.start();
}
