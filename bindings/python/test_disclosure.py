import ctypes as C
import os
import unittest
from types import SimpleNamespace
from unittest.mock import Mock
from cui_bridge import (
    App, BridgeError, DisclosureConfig, DisclosureTriggerStyle,
    accordion, collapsible, column, item, label,
)
from cui_bridge.disclosure import Config, Style, bind_api


from test_disclosure_heading import HeadingTests


class DisclosureTests(unittest.TestCase):
    def test_validation_and_probe(self):
        self.assertEqual(C.sizeof(Config), 172)
        self.assertEqual(C.sizeof(Style), 40)
        for args in (
            {"width": float("nan")}, {"joined": 1}, {"clip_card": True},
            {"collapsed_icon": "x" * 64}, {"card_border_rgba": -1},
        ):
            with self.subTest(args=args), self.assertRaises((ValueError, TypeError)):
                DisclosureConfig(**args)
        for available in ("disclosure_configure_v1", "disclosure_set_trigger_style_v1"):
            api = SimpleNamespace(lib=SimpleNamespace(**{"cui_bridge_" + available: Mock()}))
            bind_api(api, Mock())
            self.assertIsNone(api.disclosure)

    def test_incomplete_bridge_preserves_ordinary_disclosure(self):
        with App() as app:
            app._api.disclosure = None
            self.assertFalse(app.supports_disclosure_presentation())
            fold = collapsible("advanced", "Advanced").children(label("Cache preferences"))
            fold.disclosure_configure(DisclosureConfig())
            root = column().children(fold)
            with self.assertRaisesRegex(RuntimeError, "both optional"):
                app.mount(root)
            fold.disclosure_configure(None)
            app.mount(root)
            app.step()
            fold.open(True)
            app.step()
            self.assertTrue(fold.is_open())
            with self.assertRaisesRegex(RuntimeError, "both optional"):
                fold.set_trigger_style(DisclosureTriggerStyle(hover_rgba=0))

    def test_layout_reset_and_events(self):
        with App() as app:
            supported = os.environ.get("CUI_TEST_LEGACY_DISCLOSURE") != "1"
            self.assertEqual(app.supports_disclosure_presentation(), supported)
            if not supported:
                return
            events = []
            leaf = item("leaf", "Editor").children(label("Preferences")).on_change(events.append)
            group = accordion("settings").children(leaf).disclosure_configure(
                DisclosureConfig(joined=True, bordered=True, allow_collapse=True, width=320),
            )
            leaf.set_trigger_style(DisclosureTriggerStyle(
                hover_rgba=0, expanded_rgba=0x224466ff, cursor="pointer",
            ))
            app.mount(column().children(group))
            app.step()
            self.assertEqual(app.bounds(group)[2], 320)
            with self.assertRaises(BridgeError):
                group.disclosure_configure(DisclosureConfig(collapsed_icon="not-an-embedded-icon"))
            self.assertEqual(group._disclosure_config.width, 320)
            x, y, _, _ = app.bounds(leaf)
            app.queue_pointer("down", x + 20, y + 12)
            app.queue_pointer("up", x + 20, y + 12)
            app.step()
            group.disclosure_configure(None)
            leaf.set_trigger_style(None)
            self.assertEqual(events, [])
            app.dispatch()
            self.assertEqual(len(events), 1)
            self.assertTrue(events[0].checked)
            self.assertTrue(leaf.is_open())
            app.step()
            app.dispatch()
            self.assertEqual(len(events), 1)


if __name__ == "__main__":
    unittest.main()
