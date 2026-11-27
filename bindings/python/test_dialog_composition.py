import unittest
from unittest.mock import patch

from cui_bridge import App, alert_dialog, button, column, dialog, dialog_slot, label


def click(app, control):
    x, y, width, height = app.bounds(control)
    for phase in ('down', 'up'):
        app.queue_pointer(phase, x + width / 2, y + height / 2)
    app.step()
    app.dispatch()


class DialogTests(unittest.TestCase):
    def test_validation_and_optional_provider(self):
        for config in ({'width': float('nan')}, {'height': 4097}, {'margin': -1},
                       {'centered': 1}, {'bogus': True}, {'show_close': True}):
            with self.assertRaises((ValueError, TypeError)):
                dialog('dialog').dialog_options(config)
        with self.assertRaisesRegex(ValueError, 'direct Dialog owner'):
            column().children(dialog_slot('slot', 'body'))
        with self.assertRaisesRegex(ValueError, 'duplicate Dialog slot'):
            dialog('dialog').children(dialog_slot('one', 'body'), dialog_slot('two', 'body'))
        with self.assertRaisesRegex(ValueError, 'must be dialog slots'):
            alert_dialog('alert').children(label('Details'))
        with App() as app:
            owner = dialog('dialog').dialog_options(width=300)
            with patch.object(app._api, 'dialog_configure', None):
                self.assertFalse(app.supports_dialog_composition())
                with self.assertRaisesRegex(RuntimeError, 'compatible bridge'):
                    app.mount(column().children(owner))
                self.assertIsNone(owner._app)
                owner.dialog_options(None)
                app.mount(column().children(owner)).step()

    def test_failed_configuration_preserves_previous_options(self):
        with App() as app:
            owner = dialog('dialog').dialog_options(width=300).children(dialog_slot('body', 'body'))
            app.mount(column().children(owner)).step()
            owner.open()
            app.step()
            with self.assertRaises(RuntimeError):
                owner.dialog_options(None)
            self.assertEqual(app.bounds(owner)[2], 300)
            owner.dialog_options(width=350)
            app.step()
            self.assertEqual(app.bounds(owner)[2], 350)

    def test_slots_actions_and_close_order(self):
        for factory in (dialog, alert_dialog):
            for action, result in (('confirm', 1), ('cancel', 2), ('close', 3)):
                with self.subTest(factory=factory, action=action), App(width=640, height=480) as app:
                    events = []
                    accept = button('Go').dialog_action(action).on_click(lambda _: events.append('click'))
                    footer = dialog_slot('footer', 'footer').disabled().children(accept)
                    slots = [dialog_slot(role, role).children(label(role)) for role in
                             ('header', 'title', 'description', 'content', 'body')]
                    owner = (factory('dialog', 'Workspace').dialog_options(width=360, centered=True)
                             .children(*slots, footer).on_close(lambda event: events.append(int(event.value))))
                    app.mount(column().children(owner)).step()
                    owner.open()
                    app.step()
                    click(app, accept)
                    self.assertEqual(events, [])
                    self.assertTrue(owner.is_open())
                    footer.disabled(False)
                    app.step()
                    click(app, accept)
                    self.assertEqual(events, ['click', result])
                    self.assertFalse(owner.is_open())


if __name__ == '__main__':
    unittest.main()
