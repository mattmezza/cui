import ctypes as C
import os
import unittest
from types import SimpleNamespace
from unittest.mock import Mock

from cui_bridge import (
    App, DisclosureConfig, DisclosureHeadingLayer as Layer, DisclosureHeading as Heading,
    DisclosureHeadingBox as Box, DisclosureTypography as Typography, DisclosureDivider as Divider,
    accordion, item, column, label,
)
from cui_bridge.disclosure import bind_api, Heading as WireHeading, Divider as WireDivider


class HeadingTests(unittest.TestCase):
    def test_validation_and_independent_probes(self):
        self.assertEqual(C.sizeof(WireHeading), 76)
        self.assertEqual(C.sizeof(WireDivider), 32)
        invalid_records = (
            lambda: Box(height=True),
            lambda: Box(width=-2),
            lambda: Typography(),
            lambda: Typography(font_size=1e-50),
            lambda: Typography(font_size=0),
            lambda: Typography(font_size=14, letter_spacing=float('nan')),
            lambda: Typography(font_size=14, wrap=1),
            lambda: Heading(title_rgba=True),
            lambda: Heading(box={}),
            lambda: Divider(thickness=-1),
        )
        for create in invalid_records:
            with self.assertRaises((ValueError, TypeError)):
                create()
        for family in ('heading', 'divider'):
            symbol = 'cui_bridge_disclosure_set_' + family + '_v1'
            api = SimpleNamespace(lib=SimpleNamespace(**{symbol: Mock()}))
            bind_api(api, Mock())
            other_family = 'divider' if family == 'heading' else 'heading'
            self.assertIsNotNone(getattr(api, 'disclosure_' + family))
            self.assertIsNone(getattr(api, 'disclosure_' + other_family))
            self.assertIsNone(api.disclosure)

    def test_geometry_reset_and_events(self):
        with App() as app:
            supported = os.environ.get('CUI_TEST_LEGACY_DISCLOSURE') != '1'
            self.assertEqual(app.supports_disclosure_heading(), supported)
            self.assertEqual(app.supports_disclosure_divider(), supported)
            events = []
            leaf = item('leaf', 'Editor').children(label('Content')).on_change(events.append)
            second = item('second', 'Account').children(label('Details'))
            group = accordion('group').children(leaf, second)
            group.set_disclosure_heading(Layer.NORMAL, Heading(
                box=Box(width=300, height=60, padding=8), title_rgba=0,
            ))
            root = column().children(group)
            if not supported:
                with self.assertRaisesRegex(RuntimeError, 'optional'):
                    app.mount(root)
                group.set_disclosure_heading(Layer.NORMAL, None)
                self.assertEqual(group._disclosure_headings, {})
                app.mount(root)
                app.step()
                return
            group.disclosure_configure(DisclosureConfig(joined=True, width=300))
            group.set_disclosure_divider(Divider(thickness=0, color_rgba=0))
            app.mount(root)
            app.step()
            self.assertEqual(app.bounds(leaf)[3], 60)
            second_y = app.bounds(second)[1]
            group_height = app.bounds(group)[3]
            # The new setters remain independent of the older presentation bundle.
            app._api.disclosure = None
            group.set_disclosure_divider(Divider(
                thickness=7, inset_left=8, inset_right=12, color_rgba=0,
            ))
            app.step()
            self.assertEqual(app.bounds(second)[1], second_y + 7)
            self.assertEqual(app.bounds(group)[3], group_height + 7)
            group.set_disclosure_divider(Divider(thickness=0))
            app.step()
            self.assertEqual(app.bounds(second)[1], second_y)
            x, y, _, _ = app.bounds(leaf)
            app.queue_pointer('down', x + 20, y + 20)
            app.queue_pointer('up', x + 20, y + 20)
            app.step()
            group.set_disclosure_heading(Layer.NORMAL, None).set_disclosure_divider(None)
            self.assertEqual(group._disclosure_headings, {})
            self.assertEqual(events, [])
            app.dispatch()
            self.assertEqual(len(events), 1)
            self.assertTrue(events[0].checked)


if __name__ == '__main__':
    unittest.main()
