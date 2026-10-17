import math
import unittest
from cui_bridge import App, BridgeError, row

class RunTests(unittest.TestCase):
    def test_portable_and_invalid_timing_leave_manual_loop_available(self):
        with App() as app:
            app.mount(row())
            for dt in (0, -1, math.inf, math.nan):
                with self.assertRaises(ValueError): app.run(dt)
            with self.assertRaises(BridgeError) as unavailable: app.run()
            self.assertEqual(unavailable.exception.code, 9)
            app.step()
            self.assertEqual(app.dispatch(), 0)

if __name__ == '__main__': unittest.main()
