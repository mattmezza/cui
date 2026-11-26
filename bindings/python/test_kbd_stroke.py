"""Typed Kbd authoring and optional-provider checks; always headless."""
import os
import unittest
from unittest.mock import patch
from cui_bridge import App, column, kbd, kbd_stroke


class KbdStrokeTests(unittest.TestCase):
    def test_authoring_and_utf8_limits(self):
        node = kbd_stroke('save', 'é', modifiers=10, plain=True, outline=True,
                          mac_symbols=True, accessible_name='Save')
        self.assertEqual(node._kbd_stroke, (10, 7, b'\xc3\xa9', 2, b'Save', 4))
        kbd_stroke('limit', 'é' * 64, accessible_name='é' * 512)
        for key in ('', 'a\nb', 'a\rb', 'a\tb', '\0', '\u2028', 'é' * 65, '\ud800'):
            with self.subTest(key=repr(key)), self.assertRaises((ValueError, UnicodeError)):
                kbd_stroke('bad', key)
        for modifiers in (-1, 16, True, 1.5):
            with self.assertRaises(ValueError):
                kbd_stroke('bad', 'K', modifiers=modifiers)
        with self.assertRaises(TypeError):
            kbd_stroke('bad', 'K', plain=1)
        with self.assertRaises(ValueError):
            kbd_stroke('bad', 'K', accessible_name='x' * 1025)
        self.assertIsNone(kbd('legacy', 'Ctrl+K')._kbd_stroke)

    def test_missing_symbol_keeps_legacy_usable(self):
        with App(backend='none') as app:
            with patch.object(app._api, 'set_kbd_stroke', None):
                self.assertFalse(app.supports_kbd_stroke())
                typed = kbd_stroke('typed', 'K')
                with self.assertRaisesRegex(RuntimeError, 'cui_bridge_set_kbd_stroke_v1'):
                    app.mount(column().children(typed))
                self.assertIsNone(typed._app)
                app.mount(column().children(kbd('legacy', 'Ctrl+K')))
                app.step()

    def test_provider(self):
        with App(backend='none') as app:
            expected = os.getenv('CUI_EXPECT_KBD_STROKE')
            if expected is not None:
                self.assertEqual(app.supports_kbd_stroke(), expected == '1')
            if not app.supports_kbd_stroke():
                with self.assertRaisesRegex(RuntimeError, 'cui_bridge_set_kbd_stroke_v1'):
                    app.mount(column().children(kbd_stroke('typed', 'K')))
                node = kbd('legacy', 'Ctrl+K')
            else:
                node = kbd_stroke('typed', 'K', modifiers=10, accessible_name='Save')
            app.mount(column().children(node))
            app.step()
            self.assertGreater(app.bounds(node)[2], 0)


if __name__ == '__main__':
    unittest.main()
