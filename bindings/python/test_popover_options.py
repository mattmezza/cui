"""Headless Popover options, legacy preflight and keyboard interaction."""
import ctypes as C
import os
import unittest
from unittest.mock import patch

from cui_bridge import App, button, column, popover
from cui_bridge.popover_options import _Config


class PopoverTests(unittest.TestCase):
    def test_validation(self):
        self.assertEqual(C.sizeof(_Config), 32)
        for config in (
            {"anchor": 8}, {"anchor": True}, {"width": float("nan")},
            {"width": 10**400}, {"height": 4097}, {"margin": -1},
            {"offset": 4097}, {"reference_rem": 257}, {"arrow": 1}, {"secondary_trigger": 1}, {"bogus": True},
        ):
            with self.subTest(config=config), self.assertRaises((ValueError, TypeError)):
                popover("p").popover_options(config)
        self.assertEqual(popover("p").popover_options(offset=0)._popover_config.flags, 64)
        self.assertIsNone(popover("p").popover_options(width=180).popover_options(None)._popover_config)

    def test_preflight_and_legacy(self):
        with App() as app:
            trigger = button("trigger")
            panel = popover("p").anchor(trigger).popover_options(width=180)
            root = column().children(trigger, panel)
            with patch.object(app._api, "popover_options", None):
                self.assertFalse(app.supports_popover_options())
                with self.assertRaisesRegex(RuntimeError, "compatible bridge"):
                    app.mount(root)
                self.assertIsNone(panel._app)
                panel.popover_options(None)
                app.mount(root).step()
                panel.open()
                app.step()
                self.assertTrue(panel.is_open())
                panel.open(False)
                app.step()
                self.assertFalse(panel.is_open())

    def test_secondary_trigger(self):
        with App() as app:
            trigger = button("trigger")
            clicks, closes = [], []
            trigger.on_click(clicks.append)
            panel = popover("menu").anchor(trigger).on_close(closes.append)
            panel.popover_options(secondary_trigger=True)
            root = column().children(panel, trigger)
            expected = not (os.environ.get("CUI_POPOVER_UNSUPPORTED") == "1" or
                            os.environ.get("CUI_POPOVER_SECONDARY_UNSUPPORTED") == "1")
            self.assertEqual(app.supports_popover_secondary_trigger(), expected)
            if not expected:
                with self.assertRaisesRegex(RuntimeError, "compatible bridge"):
                    app.mount(root)
                panel.popover_options(None)
                app.mount(root).step()
                return
            app.mount(root).step()
            x, y, w, h = app.bounds(trigger)
            for open_after in (True, False):
                for phase in ("down", "up"):
                    app.queue_pointer(phase, x + w / 2, y + h / 2, button="secondary")
                app.step()
                app.step()
                app.dispatch()
                self.assertEqual(panel.is_open(), open_after)
            self.assertEqual(clicks, [])
            self.assertEqual(len(closes), 1)

    def test_placement_focus_escape(self):
        with App(width=640, height=480) as app:
            if os.environ.get("CUI_POPOVER_UNSUPPORTED") == "1":
                self.assertFalse(app.supports_popover_options())
                return
            self.assertTrue(app.supports_popover_options())
            closed, activated = [], []
            trigger = button("Account")
            action = button("Profile").on_click(activated.append)
            panel = (
                popover("menu").anchor(trigger).children(action).on_close(closed.append)
                .popover_options(anchor=0, width=180, height=120,
                                 hide_close=True, focus_content=True)
            )
            trigger.on_click(lambda event: panel.open())
            app.mount(column().children(trigger, panel)).step()

            x, y, width, height = app.bounds(trigger)
            app.queue_pointer("down", x + width / 2, y + height / 2)
            app.queue_pointer("up", x + width / 2, y + height / 2)
            app.step()
            app.dispatch()
            app.step()
            self.assertTrue(panel.is_open())
            _, panel_y, panel_width, _ = app.bounds(panel)
            self.assertAlmostEqual(panel_width, 180, delta=1)
            self.assertGreaterEqual(panel_y, y + height)

            app.queue_key("down", "enter")
            app.queue_key("up", "enter")
            app.step()
            app.dispatch()
            self.assertEqual(len(activated), 1)
            app.queue_key("down", "escape")
            app.queue_key("up", "escape")
            app.step()
            app.dispatch()
            self.assertFalse(panel.is_open())
            self.assertEqual(len(closed), 1)
            panel.popover_options(None)


if __name__ == "__main__":
    unittest.main()
