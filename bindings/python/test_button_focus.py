"""Button focus authoring and optional provider compatibility (headless)."""
import os
import unittest
from cui_bridge import App, button, column


class ButtonFocusTests(unittest.TestCase):
    def test_authoring(self):
        for mode in ('legacy', 'variant', 'outer-ring'):
            node = button('save')
            self.assertIs(node.focus_treatment(mode), node)
            self.assertEqual(node._button_focus, mode)
        for value in (None, True, 0, 'outer', [], {}):
            with self.assertRaises(ValueError):
                button('save').focus_treatment(value)
        with self.assertRaises(ValueError):
            column().focus_treatment('legacy')

    def test_live_wire_without_provider(self):
        from types import SimpleNamespace
        calls = []
        api = SimpleNamespace(set_button_focus=lambda *args: calls.append(args) or 0,
                              check=lambda code: self.assertEqual(code, 0))
        app = SimpleNamespace(_api=api, _ptr=123, _ensure_open=lambda: None,
                              supports_button_focus=lambda: True)
        node = button('save')
        node._app, node._handle = app, 456
        for mode, wire in [('variant', 1), ('outer-ring', 2), ('legacy', 0)]:
            self.assertIs(node.focus_treatment(mode), node)
            self.assertEqual(calls[-1], (123, 456, wire))

    def test_old_provider(self):
        with App() as app:
            app._api.set_button_focus = None
            self.assertFalse(app.supports_button_focus())
            root = column().children(button('save').focus_treatment('variant'))
            with self.assertRaisesRegex(RuntimeError, 'set_button_focus_v1'):
                app.mount(root)
            self.assertIsNone(root._handle)
            plain = button('save').focus_treatment('legacy')
            app.mount(column().children(plain))
            self.assertIs(plain.focus_treatment('legacy'), plain)
            with self.assertRaises(RuntimeError):
                plain.focus_treatment('outer-ring')
            self.assertEqual(plain._button_focus, 'legacy')
            app.step()

    def test_live(self):
        with App() as app:
            expected = os.environ.get('CUI_EXPECT_BUTTON_FOCUS')
            if expected is not None:
                self.assertEqual(app.supports_button_focus(), expected == '1')
            if not app.supports_button_focus():
                self.skipTest('provider has no Button focus extension')
            node = button('save').label('Save').focus_treatment('variant')
            app.mount(column().children(node))
            app.step()
            clicks = []
            node.on_click(clicks.append)
            before = app.bounds(node)
            for mode in ('outer-ring', 'legacy', 'variant'):
                self.assertIs(node.focus_treatment(mode), node)
                app.step()
                self.assertEqual(app.bounds(node), before)
            x, y, width, height = before
            app.queue_pointer('down', x + width / 2, y + height / 2)
            app.queue_pointer('up', x + width / 2, y + height / 2)
            app.step()
            app.step()
            app.dispatch()
            self.assertEqual(len(clicks), 1)


if __name__ == '__main__':
    unittest.main()
