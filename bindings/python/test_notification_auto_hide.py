"""Optional next-open Notification lifetime policy."""
import os
import unittest
from cui_bridge import App, notification, column


class NotificationAutoHide(unittest.TestCase):
    def test_authoring(self):
        self.assertIs(notification('n').auto_hide()._notification_auto_hide, True)
        for value in (0, 1, None, 'true'):
            with self.assertRaises(TypeError):
                notification('n').auto_hide(value)
        with self.assertRaises(ValueError):
            column().auto_hide()

    def test_unsupported_is_atomic_and_plain_is_persistent(self):
        with App() as app:
            app._api.notification_set_auto_hide = None
            self.assertFalse(app.supports_notification_auto_hide())
            root = column().children(notification('configured').auto_hide(False).open())
            with self.assertRaisesRegex(RuntimeError, 'auto-hide unavailable'):
                app.mount(root)
            self.assertIsNone(root._handle)
            plain = notification('plain').open()
            app.mount(column().children(plain))
            for _ in range(10):
                app.step(1)
            self.assertTrue(plain.is_open())

    def test_expiry_and_next_open_policy(self):
        with App() as app:
            supported = app.supports_notification_auto_hide()
            expected = os.environ.get('CUI_EXPECT_NOTIFICATION_AUTO_HIDE')
            if expected is not None:
                self.assertEqual(supported, expected == '1')
            if not supported:
                self.skipTest('provider has no Notification auto-hide')
            closed = []
            notice = notification('timed').auto_hide().open().on_close(closed.append)
            persistent = notification('persistent').auto_hide(False).open()
            app.mount(column().children(notice, persistent))
            app.step(1)
            notice.auto_hide(False)  # Does not retime the already open card.
            for _ in range(8):
                app.step(1)
                app.dispatch()
            self.assertFalse(notice.is_open())
            self.assertTrue(persistent.is_open())
            self.assertEqual(len(closed), 1)
            notice.open()
            for _ in range(10):
                app.step(1)
                app.dispatch()
            self.assertTrue(notice.is_open())
            self.assertEqual(len(closed), 1)


if __name__ == '__main__':
    unittest.main()
