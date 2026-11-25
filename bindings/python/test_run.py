import math
import unittest
from unittest.mock import Mock, patch

from cui_bridge import App, BridgeError, row


class RunTests(unittest.TestCase):
    def test_portable_and_invalid_timing_leave_manual_loop_available(self):
        with App() as app:
            app.mount(row())
            for dt in (0, -1, math.inf, math.nan):
                with self.assertRaises(ValueError): app.run(dt)
            with self.assertRaises(TypeError):
                app.run(after_frame=17)
            with self.assertRaises(BridgeError) as unavailable: app.run()
            self.assertEqual(unavailable.exception.code, 9)
            app.step()
            self.assertEqual(app.dispatch(), 0)

    def test_after_frame_preserves_dispatch_order_and_exception(self):
        def native_runner(_app, callback, _user):
            return callback(None) or 2

        order = []
        marker = ValueError("after-frame exception")
        with App() as app:
            app.mount(row())

            def hook(active_app):
                self.assertIs(active_app, app)
                order.append("hook")
                raise marker

            with patch.object(app._api.lib, "cui_bridge_run", native_runner), patch.object(
                    app, "dispatch", side_effect=lambda: order.append("dispatch")):
                with self.assertRaises(ValueError) as error:
                    app.run(after_frame=hook)
                self.assertIs(error.exception, marker)
                self.assertEqual(order, ["dispatch", "hook"])
            app.step()

    def test_after_frame_is_skipped_when_dispatch_requests_close(self):
        def native_runner(_app, callback, _user):
            return callback(None) or 2

        with App() as app:
            app.mount(row())
            hook = Mock()
            with patch.object(app._api.lib, "cui_bridge_run", native_runner), patch.object(
                    app, "dispatch", side_effect=app.request_close):
                app.run(after_frame=hook)
            hook.assert_not_called()


if __name__ == '__main__': unittest.main()
