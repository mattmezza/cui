"""Notification actions retain ordinary Button identity and preflight atomically."""
import os
import unittest

from cui_bridge import App, notification, button, label, column


class NotificationAction(unittest.TestCase):
    def test_authoring(self):
        for invalid in (label("text"), column(), None):
            with self.assertRaises(TypeError):
                notification("n").action(invalid)
        with self.assertRaises(ValueError):
            column().action(button("b"))
        notice = notification("n").action(button("b"))
        with self.assertRaises(ValueError):
            notice.action(button("other"))
        with self.assertRaises(ValueError):
            notice.children(button("other"))
        self.assertEqual(len(notice._children), 1)

    def test_unsupported_preflight_and_ordinary_notification(self):
        with App() as app:
            app._api.create_notification_action = None
            self.assertFalse(app.supports_notification_action())
            action = button("action")
            notice = notification("notice", "Ready").open().action(action)
            root = column().children(notice)
            with self.assertRaisesRegex(RuntimeError, "action unavailable"):
                app.mount(root)
            self.assertIsNone(root._handle)
            self.assertIsNone(action._handle)
            plain = notification("plain", "Ready").open()
            app.mount(column().children(plain))
            app.step()
            self.assertTrue(plain.is_open())

    def test_button_action_click_keeps_notification_open(self):
        with App() as app:
            supported = app.supports_notification_action()
            expected = os.environ.get("CUI_EXPECT_NOTIFICATION_ACTION")
            if expected is not None:
                self.assertEqual(supported, expected == "1")
            if not supported:
                self.skipTest("provider has no Notification action")

            calls = []
            action = button("action").label("Undo").on_click(lambda event: calls.append(event.node))
            notice = notification("notice", "Saved").action(action).open()
            app.mount(column().children(notice))
            app.step()
            app.step()
            x, y, width, height = app.bounds(action)
            self.assertGreater(width, 0)
            self.assertGreater(height, 0)
            app.queue_pointer("down", x + width / 2, y + height / 2)
            app.step()
            app.queue_pointer("up", x + width / 2, y + height / 2)
            app.step()
            self.assertEqual(app.dispatch(), 1)
            self.assertEqual(calls, [action])
            self.assertTrue(notice.is_open())


if __name__ == "__main__":
    unittest.main()
