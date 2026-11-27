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

    def test_style_authoring_copies_and_validates(self):
        gradient = {"stops": [{"position": 0, "color": 0xff0000ff},
                               {"position": 1, "color": 0x0000ffff}]}
        node = kbd('literal', 'Ctrl+K').kbd_style({"padding_x": 0, "text_gradient": gradient})
        gradient["stops"][0]["color"] = 0
        self.assertEqual(node._kbd_style.stops[0].color, 0xff0000ff)
        self.assertEqual(node._kbd_style.padding_y, 2)
        self.assertEqual(node._kbd_style.struct_size, 144)
        all_fields = kbd('all').kbd_style({"max_width": 200, "fill": True, "wrap": True,
            "padding_x": 0, "padding_y": 0, "radius": 0, "background": 0,
            "border_width": 0, "border_color": 0, "font_size": 18, "line_height": 0,
            "weight": "bold", "italic": True, "monospace": True, "text_color": 0})
        self.assertEqual(all_fields._kbd_style.fields, 8191)
        old = node._kbd_style
        for style in ({"unknown": 1}, {"max_width": 0}, {"font_size": float('nan')},
                      {"fill": 1}, {"border_width": 1}, {"text_color": -1},
                      {"text_gradient": {"stops": []}},
                      {"text_color": 0, "text_gradient": gradient},
                      {"text_gradient": {**gradient, "x1": 0}}):
            with self.assertRaises((ValueError, TypeError)):
                node.kbd_style(style)
            self.assertIs(node._kbd_style, old)
        node.kbd_style()
        self.assertIsNone(node._kbd_style)
        with self.assertRaises(ValueError): column().kbd_style({})

    def test_style_provider_and_live_clear(self):
        with App(backend='none') as app:
            node = kbd('styled', 'Ctrl+K').kbd_style({"padding_x": 14, "padding_y": 8})
            root = column().children(node)
            with patch.object(app._api, 'set_kbd_style', None):
                self.assertFalse(app.supports_kbd_style())
                with self.assertRaisesRegex(RuntimeError, 'cui_bridge_set_kbd_style_v1'):
                    app.mount(root)
                self.assertIsNone(node._app)
            supported = app.supports_kbd_style()
            expected = os.getenv("CUI_EXPECT_KBD_STYLE")
            if expected is not None:
                self.assertEqual(supported, expected == "1")
            if not supported:
                node.kbd_style({})
            app.mount(root)
            app.step()
            before = app.bounds(node)
            if not supported:
                self.assertGreater(before[2], 0)
                node.kbd_style(None)
                return
            node.kbd_style({"padding_x": 34, "padding_y": 18}); app.step()
            after = app.bounds(node)
            self.assertGreater(after[2], before[2])
            self.assertGreater(after[3], before[3])
            node.kbd_style(None); app.step()
            self.assertLess(app.bounds(node)[2], after[2])

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
