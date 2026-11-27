"""Optional responsive in-app notifications: portable headless regression."""
import os
import unittest
from unittest.mock import patch

from cui_bridge import App, column, notification


class ResponsiveNotifications(unittest.TestCase):
    def test_optional_provider_and_validation(self):
        with App(backend='none') as app:
            expected = os.getenv('CUI_EXPECT_RESPONSIVE_NOTIFICATIONS')
            if expected is not None:
                self.assertEqual(app.supports_responsive_notifications(), expected == '1')
            for value in (0, 1, None, 'true'):
                with self.assertRaises(TypeError):
                    app.set_responsive_notifications(value)
            with patch.object(app._api, 'set_responsive_notifications', None):
                self.assertFalse(app.supports_responsive_notifications())
                for enabled in (True, False):
                    with self.assertRaisesRegex(RuntimeError, 'unavailable'):
                        app.set_responsive_notifications(enabled)
                node = notification('legacy', 'Legacy').open()
                app.mount(column().children(node))
                app.step(.4)
                self.assertTrue(node.is_open())
        with self.assertRaises(RuntimeError):
            app.supports_responsive_notifications()

    def test_wrapping_reversible_and_independent_close(self):
        with App(backend='none', width=220, height=320) as app:
            if not app.supports_responsive_notifications():
                self.skipTest('provider lacks responsive notifications')
            events = []
            node = notification('notice', 'Saved').detail(
                'Your workspace changes have been saved and are ready for everyone on your team.'
            ).open().on_click(events.append).on_close(events.append)
            app.mount(column().children(node))
            app.step(.4)
            app.step(.3)
            legacy_height = app.bounds(node)[3]
            self.assertIs(app.set_responsive_notifications(True), app)
            app.step(.4)
            app.step(.3)
            x, y, width, height = app.bounds(node)
            self.assertGreater(height, legacy_height)
            app.set_responsive_notifications(False)
            app.step(.4)
            app.step(.3)
            self.assertAlmostEqual(app.bounds(node)[3], legacy_height, places=2)
            app.set_responsive_notifications(True)
            app.step(.4)
            app.step(.3)
            x, y, width, _ = app.bounds(node)
            # No hover frame: the independent Close target must already exist.
            app.queue_pointer('down', x + width - 18, y + 18)
            app.queue_pointer('up', x + width - 18, y + 18)
            app.step(.4)
            app.step(.3)
            app.dispatch()
            self.assertFalse(node.is_open())
            self.assertEqual([event.type for event in events], ['close'])


if __name__ == '__main__':
    unittest.main()
