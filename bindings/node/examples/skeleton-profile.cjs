'use strict';
const assert = require('node:assert/strict');
const {App, button, column, label, responsive_skeleton} = require('../index.cjs');

const args = process.argv.slice(2);
const font = args.find(arg => !arg.startsWith('--'));
const smoke = args.includes('--smoke');
if (smoke && (!font || process.env.CUI_TEST_VIRTUAL_DISPLAY !== '1'))
  throw new Error('--smoke requires a font and the private display from scripts/test_local.py');

const avatar = responsive_skeleton('avatar', {widthMode: 'fixed', width: 40, height: 40, radius: 20});
const name = responsive_skeleton('name', {width: .65, radius: 6});
const role = responsive_skeleton('role', {width: .4, height: 12, radius: 6, secondary: true});
const description = responsive_skeleton('description', {height: 36, radius: 8});
const placeholders = [avatar, name, role, description];
const title = label('Workspace profile');
const status = label('Loading profile…');
const finish = button('finish').primary().label('Finish loading');
let loaded = false;

finish.on_click(() => {
  for (const placeholder of placeholders) placeholder.remove();
  title.label('Alex Morgan');
  status.label('Workspace admin · alex@example.com');
  finish.disabled().label('Profile loaded');
  loaded = true;
});

const app = new App({width: 480, height: 320, title: 'JavaScript Skeleton profile',
  backend: font ? 'sdl' : 'headless', fontPath: font, initiallyHidden: smoke});
try {
  app.mount(column().gap(10).children(title, placeholders, status, finish));
  if (font && !smoke) {
    app.run();
  } else {
    app.step(.25);
    assert.equal(avatar.bounds().width, 40);
    assert.ok(Math.abs(name.bounds().width - 448 * .65) < .001);
    assert.equal(description.bounds().width, 448);
    const sample = name.presentation();
    assert.ok(sample && sample.nextWakeSeconds > sample.coreTime);
    const before = status.bounds().y;
    const {x, y, width, height} = finish.bounds();
    app.pointer('down', x + width / 2, y + height / 2)
      .pointer('up', x + width / 2, y + height / 2).step();
    app.step();
    assert.ok(loaded && status.bounds().y < before);
    console.log('Responsive Skeleton profile: loaded and placeholders released');
  }
} finally {
  app.dispose();
}
