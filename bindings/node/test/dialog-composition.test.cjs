'use strict';
const test = require('node:test');
const assert = require('node:assert/strict');
const { App, column, label, button, dialog, alert_dialog, dialog_slot } = require('..');

function click(app, control) {
  const { x, y, width, height } = control.bounds();
  assert.ok(width > 0 && height > 0);
  app.pointer('down', x + width / 2, y + height / 2)
    .pointer('up', x + width / 2, y + height / 2).step();
}

test('Rejects invalid options, duplicate slots and misplaced slots', () => {
  for (const options of [{ width: NaN }, { height: 4097 }, { margin: -1 },
    { centered: 1 }, { bogus: true }, { show_close: true }]) {
    assert.throws(() => dialog('dialog').dialog_options(options));
  }
  assert.throws(() => column().children(dialog_slot('slot', 'body')), /direct Dialog owner/);
  assert.throws(() => alert_dialog('alert').children(label('Details')), /must be dialog slots/);
  assert.throws(() => dialog('dialog').children(dialog_slot('one', 'body'), dialog_slot('two', 'body')),
    /duplicate Dialog slot role/);
  const app = new App();
  try {
    assert.throws(() => app.mount(column().children(dialog('dialog').children(dialog_slot('slot', 'body')))),
      /configured direct Dialog/);
  } finally {
    app.dispose();
  }
});

test('All slots and actions preserve click-before-close order and honor a disabled Footer', () => {
  const actions = [['confirm', 1], ['cancel', 2], ['close', 3]];
  for (const factory of [dialog, alert_dialog]) {
    for (const [action, result] of actions) {
      const app = new App({ width: 640, height: 480 });
      try {
        assert.equal(app.supports_dialog_composition(), true);
        const events = [];
        const accept = button('Go').dialog_action(action).on_click(() => events.push('click'));
        const footer = dialog_slot('footer', 'footer').disabled().children(accept);
        const slots = ['header', 'title', 'description', 'content', 'body']
          .map(role => dialog_slot(role, role).children(label(role)));
        const owner = factory('dialog', 'Workspace').dialog_options({ width: 360, centered: true })
          .children(...slots, footer).on_close(event => events.push(event.result));
        app.mount(column().children(owner)).step();
        owner.open();
        app.step();
        click(app, accept);
        assert.deepEqual(events, []);
        assert.equal(owner.is_open(), true);
        footer.disabled(false);
        app.step();
        click(app, accept);
        assert.deepEqual(events, ['click', result]);
        assert.equal(owner.is_open(), false);
      } finally {
        app.dispose();
      }
    }
  }
});

test('Missing optional addon functions permit legacy dialogs', () => {
  const factory = require('../dialog-composition.cjs');
  class LegacyNode {
    constructor(kind) {
      this.kind = kind;
      this.childrenValue = [];
    }
  }
  const api = factory({}, LegacyNode, Symbol('handle'));
  const app = { _alive() {} };
  const owner = new LegacyNode(24).dialog_options({ width: 300 });
  assert.equal(api.supports(app), false);
  assert.throws(() => api.preflight(app, owner), /compatible bridge/);
  owner.dialog_options(null);
  assert.doesNotThrow(() => api.preflight(app, owner));
});
