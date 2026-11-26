"""Retained Pagination compatibility and routed input through the public Python API."""
import os
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch

from cui_bridge import App, BridgeError, PaginationConfig, column, pagination
from cui_bridge.pagination_options import bind_api


def click(app, pager, part):
    x, y, w, h = app.numeric_part_bounds(pager, part)
    app.queue_pointer("down", x + w / 2, y + h / 2)
    app.queue_pointer("up", x + w / 2, y + h / 2)
    app.step()


class PaginationTests(unittest.TestCase):
    def test_wrapping_navigation_menu_and_reset(self):
        changes = []
        config = PaginationConfig(
            chevrons=True, jump_menu=True, visible_pages=5, available_width=210,
            group="Pages de résultats", previous="Précédente", next="Suivante",
            later_pages="Pages suivantes",
        )
        pager = pagination("pager").configure(config).design_size("xl")
        pager.total(20).value(3).on_change(changes.append)
        self.assertEqual(pager.get_config(), config)
        with App(width=320, height=480) as app:
            self.assertTrue(app.supports_pagination_options())
            app.mount(column().children(pager))
            app.step()
            self.assertEqual(pager.get_config(), config)
            previous = app.numeric_part_bounds(pager, 2)
            following = app.numeric_part_bounds(pager, 3)
            self.assertEqual((previous[3], following[3]), (48, 48))
            self.assertLess(previous[1], following[1])
            with self.assertRaises(BridgeError):
                app.numeric_part_bounds(pager, 1)
            self.assertEqual(app.dispatch(), 0)
            click(app, pager, 3)
            self.assertEqual(app.dispatch(), 1)
            self.assertEqual((changes[0].type, changes[0].node, changes[0].value),
                             ("change", pager, 4))
            self.assertEqual(app.dispatch(), 0)

            click(app, pager, 6)  # Later pages.
            self.assertEqual(app.dispatch(), 0)
            app.queue_key("down", "down")
            app.step()
            app.queue_key("down", "enter")
            app.step()
            self.assertEqual(app.dispatch(), 1)
            self.assertEqual([event.value for event in changes], [4, 7])

            pager.configure(PaginationConfig(chevrons=True, counter=True, available_width=181))
            app.step()
            self.assertTrue(pager.get_config().counter)
            self.assertEqual(app.dispatch(), 0)
            pager.configure(None).design_size("inherit")
            app.step()
            self.assertIsNone(pager.get_config())
            self.assertGreater(app.numeric_part_bounds(pager, 1)[2], 0)
            self.assertEqual(app.dispatch(), 0)
        with self.assertRaises(RuntimeError):
            pager.get_config()

    def test_validation_and_atomic_rejection(self):
        for values in ({"counter": 1}, {"visible_pages": True}, {"visible_pages": 4},
                       {"visible_pages": 33}, {"available_width": float("nan")},
                       {"available_width": 1e-100}, {"first": "First\0Page"},
                       {"next": 3}, {"next": "Page\u2028Next"}, {"next": "é" * 64}):
            with self.subTest(values=values), self.assertRaises((ValueError, TypeError)):
                PaginationConfig(**values)
        accepted = PaginationConfig(first="a" * 127, visible_pages=32)
        pager = pagination("pager").configure(accepted)
        with App() as app:
            app.mount(column().children(pager))
            with patch.object(app._api.pagination_options, "configure_v1", return_value=6):
                with self.assertRaises(BridgeError):
                    pager.configure(PaginationConfig(visible_pages=5))
            self.assertEqual(pager.get_config(), accepted)

    def check_unavailable(self, app):
        self.assertFalse(app.supports_pagination_options())
        configured = pagination("configured").configure(PaginationConfig())
        sized = pagination("sized").design_size("xl")
        for pager in (configured, sized):
            tree = column().children(pager)
            with self.assertRaisesRegex(RuntimeError, "compatible bridge symbol group"):
                app.mount(tree)
            self.assertIsNone(pager._handle)
            self.assertIsNone(tree._handle)
        ordinary = pagination("ordinary")
        app.mount(column().children(ordinary))
        self.assertEqual(ordinary._handle.generation, 1)
        app.step()
        self.assertIsNone(ordinary.get_config())
        click(app, ordinary, 3)
        event, = app.poll_events()
        self.assertEqual((event.type, event.node, event.value), ("change", ordinary, 2))
        with self.assertRaisesRegex(RuntimeError, "compatible bridge symbol group"):
            ordinary.configure(PaginationConfig())
        self.assertIsNone(ordinary.get_config())

    def test_incompatible_provider(self):
        for method, value in (("versions", 2), ("config_size_v1", 920)):
            with self.subTest(method=method), App() as app, patch.object(
                    app._api.pagination_options, method, return_value=value):
                self.check_unavailable(app)

    def test_partial_provider_discovery(self):
        api = SimpleNamespace(lib=SimpleNamespace(
            cui_bridge_pagination_versions=Mock(return_value=1),
            cui_bridge_pagination_config_size_v1=Mock(return_value=916),
            cui_bridge_pagination_configure_v1=Mock(),
        ))
        bind = Mock()
        bind_api(api, bind)
        self.assertIsNone(api.pagination_options)
        bind.assert_not_called()

    def test_published_legacy_bridge(self):
        library = os.environ.get("CUI_PAGINATION_LEGACY_LIBRARY")
        if not library:
            self.skipTest("published legacy library was not supplied")
        with App(library=library) as app:
            self.check_unavailable(app)


if __name__ == "__main__":
    unittest.main()
