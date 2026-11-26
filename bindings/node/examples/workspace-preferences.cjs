'use strict';

// Pass a TrueType font path to open a native SDL window.
const assert = require('node:assert/strict');
const {App, accordion, collapsible, column, item, label} = require('../index.cjs');
const fontPath = process.argv[2];
const app = new App({
  backend: fontPath ? 'sdl' : 'headless', fontPath,
  width: 480, height: 320, title: 'Workspace preferences',
});
try {
  const updates = [];
  const status = label('Open a section to edit your preferences.');
  const editor = item('editor', 'Editor').children(label('Auto-save is enabled.')).on_change(event => {
    if (event.checked) {
      const text = 'Editing workspace preferences.';
      status.label(text);
      updates.push(text);
    }
  });
  const account = item('account', 'Account').children(label('Personal details'));
  const sections = accordion('sections').children(account, editor);
  const advanced = collapsible('advanced', 'Advanced').children(label('Keep local cache'));
  if (app.supports_disclosure_presentation()) {
    sections.disclosure_configure({
      width: 360, designSize: 'lg', joined: true, bordered: true,
      single: true, allowCollapse: true, collapsedIcon: 'plus', expandedIcon: 'minus',
    });
    editor.set_trigger_style({hoverRgba: 0x4879e02b, cursor: 'pointer'});
    advanced.disclosure_configure({width: 360, borderless: true});
  }
  if (app.supports_disclosure_title_state()) {
    sections.set_disclosure_title_state('hovered', {titleRgba: 0x275dccff});
    advanced.set_disclosure_title_state('focus-visible', {typography: {fontSize: 17, weight: 2}});
  }
  if (app.supports_disclosure_heading()) {
    sections.set_disclosure_heading('normal', {
      box: {width: -1, height: 44, padding: 12, gap: 8},
      typography: {fontSize: 16, weight: 2}, titleRgba: 0x23354aff,
    });
    editor.set_disclosure_heading('expanded', {titleRgba: 0x275dccff});
  }
  if (app.supports_disclosure_divider()) {
    sections.set_disclosure_divider({thickness: 1, insetLeft: 12, insetRight: 12, colorRgba: 0xc8d0daff});
  }
  app.mount(column().key('workspace').gap(12).children(label('Workspace preferences'), sections, advanced, status));
  if (fontPath) {
    app.run();
  } else {
    app.step();
    const {x, y} = editor.bounds();
    app.pointer('down', x + 20, y + 12).pointer('up', x + 20, y + 12).step();
    assert.equal(editor.is_checked(), true);
    assert.deepEqual(updates, ['Editing workspace preferences.']);
    console.log(updates[0]);
  }
} finally {
  app.dispose();
}
