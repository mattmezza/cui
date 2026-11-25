"""Headless tests for the Python-owned SVG and configured Badge APIs."""
import copy
import ctypes as C
import os
import threading
import unittest
from dataclasses import replace
from types import SimpleNamespace
from unittest.mock import patch

from cui_bridge import (
    App, BadgeConfig, BadgeSize, BadgeVariant, BridgeError, IconAlign, IconAvailability,
    IconConstraints, IconError, IconLimits, IconOptions, IconSize, IconSource, IconStyle,
    badge, button, collapsible, column, configured_badge, configured_icon, icon, icon_rotation,
)
from cui_bridge.icons import _group, _Options


SVG = "<svg width='16' height='16'><rect x='2' y='3' width='10' height='9'/></svg>"
ASSET = "custom/approved.svg"


def queue_click(app, node):
    x, y, w, h = app.bounds(node)
    app.queue_pointer("down", x + w / 2, y + h / 2)
    app.queue_pointer("up", x + w / 2, y + h / 2)
    app.step()


class IconResourceTests(unittest.TestCase):
    def test_catalog_and_published_bounds(self):
        with App() as app:
            self.assertTrue(app.supports_icon_resources())
            count = app.icon_catalog_count()
            self.assertGreater(count, 100)
            entry = app.icon_catalog_entry(count - 1)
            self.assertTrue(entry.name)
            self.assertIsNotNone(entry.path)
            self.assertIs(app.query_icon("check"), IconAvailability.EMBEDDED)
            self.assertIn("<svg", app.copy_icon_asset("check"))
            self.assertIs(app.query_icon("invalid-catalog-name"), IconAvailability.UNKNOWN)
            self.assertIsNone(app.copy_icon_asset("invalid-catalog-name"))
            with self.assertRaises(BridgeError):
                app.icon_catalog_entry(count)

    def test_copied_path_sources_raster_and_collection(self):
        with App() as app:
            art = app.icon_resource(IconSource.path(ASSET))
            self.assertEqual(art.limits(), app.default_icon_limits())
            self.assertEqual(art.source(), IconSource.path(ASSET))
            self.assertEqual(art.bitmap().rgba, b"")
            self.assertIs(art.prepare(24, 24), IconError.NOT_FOUND)
            revision = art.info().source_revision
            self.assertIs(art.put_asset(ASSET, SVG.encode()).reload().prepare(24, 24), IconError.OK)
            bitmap = art.bitmap()
            self.assertEqual(bitmap.stride, bitmap.width * 4)
            self.assertEqual(len(bitmap.rgba), bitmap.stride * bitmap.height)
            self.assertGreater(max(bitmap.rgba[3::4]), 0)
            self.assertGreater(art.info().source_revision, revision)
            self.assertEqual(art.info().parse_count, 1)
            self.assertIs(art.prepare(30, 27, transform=icon_rotation(.5)), IconError.OK)
            self.assertEqual(art.info().cache_versions, 2)
            art.collect()
            self.assertEqual(art.info().cache_versions, 1)
            art.remove_asset(ASSET)
            self.assertIs(art.prepare(25, 25), IconError.NOT_FOUND)
            art.put_asset(ASSET, SVG).clear_assets()
            self.assertIs(art.prepare(25, 25), IconError.NOT_FOUND)
            art.set_source(IconSource.svg(SVG))
            self.assertEqual(art.source(), IconSource.svg(SVG))
            self.assertIs(art.prepare(16, 16), IconError.OK)
            self.assertNotEqual(art.bitmap().rgba, bitmap.rgba)
            art.set_source(IconSource())
            self.assertEqual(art.source(), IconSource())
            art.close()
            art.close()
            self.assertEqual(len(bitmap.rgba), bitmap.stride * bitmap.height)

    def test_limit_error_preserves_source_and_constructor_rolls_back(self):
        with App() as app:
            art = app.icon_resource(IconSource.named("check"))
            with self.assertRaises(BridgeError) as err:
                art.set_source(IconSource.named("not-in-catalog"))
            self.assertEqual(err.exception.code, 1)
            self.assertEqual(art.info().error, IconError.NOT_FOUND)
            self.assertEqual(art.source(), IconSource.named("check"))
            art.set_limits(IconLimits(max_svg_bytes=16, max_pixels=144))
            self.assertEqual(art.limits().max_svg_bytes, 16)
            with self.assertRaises(BridgeError):
                art.set_source(IconSource.svg(SVG))
            self.assertEqual(art.source(), IconSource.named("check"))
            art.close()
            limits = IconLimits(max_cache_bytes=64 * 1024 * 1024)
            with self.assertRaises(BridgeError):
                app.icon_resource(IconSource.named("not-in-catalog"), limits=limits)
            art = app.icon_resource(limits=limits)
            art.close()

    def test_owned_lifetime_and_thread_affinity(self):
        with App() as app:
            art = app.icon_resource()
            with self.assertRaises(TypeError):
                copy.copy(art)
            errors = []

            def worker():
                for fn in (art.info, art.close, app.close):
                    try:
                        fn()
                    except RuntimeError as error:
                        errors.append(str(error))

            thread = threading.Thread(target=worker)
            thread.start()
            thread.join()
            self.assertEqual(len(errors), 3)
            self.assertTrue(all("owner thread" in error for error in errors))
            self.assertEqual(art.source(), IconSource())
        with self.assertRaisesRegex(RuntimeError, "disposed"):
            art.bitmap()
        art.close()


class IconBadgeTests(unittest.TestCase):
    def test_full_style_copy_and_atomic_configure(self):
        with App() as app:
            self.assertTrue(app.supports_icon_presentation())
            art = app.icon_resource(IconSource.svg(SVG))
            edges = [1, 2, 3, 4]
            style = IconStyle(width=-1, height=40, padding=7, gap=3, radius=8,
                              background_rgba=0x402233ff, border_rgba=0x102838fa, border_width=2,
                              align_x=IconAlign.END, align_y=IconAlign.CENTER, offset_x=-1.5,
                              offset_y=2, corners=edges, border_edges=(1, 2, 1, 2), max_width=30,
                              max_height=33, opacity=0, padding_edges=(1, 1, 1, 1), row=True,
                              clip=True, fixed_width=True, fixed_height=True, inert=True,
                              width_percent=True, height_percent=True)
            opts = IconOptions(width=24, height=18, inherited_text_size=30, rem_size=16,
                               color_rgba=0, inherited_color_rgba=0x609defff, opacity=0,
                               transform=icon_rotation(.5), decorative=True,
                               constraints=IconConstraints(max_width=22, min_height=19),
                               debug_placeholder=True, style=style)
            node = configured_icon("star", art, opts, name="Étoile", fallback="check")
            app.mount(column("root").children(node))
            edges[0] = 99
            self.assertEqual(node.get_config().options.style.corners, (1, 2, 3, 4))
            self.assertEqual(node.get_config().name, "Étoile")
            self.assertIsNone(node.result())
            app.step()
            self.assertEqual(app.bounds(node)[2:], (22, 19))
            previous = node.result()
            self.assertIs(previous.error, IconError.OK)
            with self.assertRaises(BridgeError):
                node.configure(art, IconOptions(style=IconStyle(padding=3000)))
            self.assertEqual(node.result(), previous)
            self.assertEqual(node.get_config().options, opts)
            node.reset()
            self.assertIsNone(node.get_config())
            self.assertIsNone(node.result())
            self.assertEqual(art.info().references, 0)
            app.step()
            node.configure(art, IconOptions(size=IconSize.CUSTOM, custom_size=33))
            app.step()
            self.assertEqual(app.bounds(node)[2:], (33, 33))
            node.remove()
            art.close()

    def test_badge_child_callback_replaces_config_and_removes_subtree(self):
        with App() as app:
            self.assertTrue(app.supports_configured_badge())
            art = app.icon_resource(IconSource.svg(SVG))
            large = min(2**63 + 7, (1 << (8 * C.sizeof(C.c_size_t))) - 1)
            status = configured_badge("status", BadgeConfig(
                variant=BadgeVariant.ICON, size=BadgeSize.XSMALL, artwork=art, design_size="xl"))
            unread = configured_badge("unread", BadgeConfig(count=large, maximum=0))
            image = configured_icon("image", art, IconOptions(width=30, height=24))
            calls = []

            def read(event):
                calls.append(event.type)
                unread.configure(replace(unread.get_config(), count=0))
                if len(calls) == 2:
                    status.remove()

            child = button("open").label("Open inbox").on_click(read)
            app.mount(column("root").children(image, status.children(unread.children(child))))
            self.assertEqual(art.info().references, 2)
            with self.assertRaises(BridgeError) as err:
                art.close()
            self.assertEqual(err.exception.code, 8)
            app.step()
            self.assertEqual(unread.get_config().count, large)
            self.assertEqual(unread.get_config().maximum, 0)
            self.assertIsNone(unread.result().icon_error)
            self.assertIs(status.result().icon_error, IconError.OK)
            self.assertIs(image.result().error, IconError.OK)
            before = app.bounds(child)
            self.assertIsNotNone(unread.indicator_bounds())
            queue_click(app, child)
            self.assertEqual(calls, [])
            self.assertEqual(app.dispatch(), 1)
            self.assertIsNone(unread.result())
            app.step()
            self.assertEqual(app.bounds(child), before)
            self.assertIsNone(unread.indicator_bounds())
            self.assertEqual(app.dispatch(), 0)
            status.disabled()
            app.step()
            queue_click(app, child)
            self.assertEqual(app.dispatch(), 0)
            status.disabled(False)
            app.step()
            queue_click(app, child)
            self.assertEqual(app.dispatch(), 1)
            self.assertEqual(calls, ["click", "click"])
            self.assertEqual(art.info().references, 1)
            with self.assertRaisesRegex(RuntimeError, "disposed"):
                child.label("cannot edit")
            image.remove()
            art.close()

    def test_named_artwork_in_badge_errors_omission_and_recovery(self):
        with App() as app:
            art = app.icon_resource(IconSource.path(ASSET))
            image = configured_icon("image", art, name="Status")
            marker = configured_badge("badge", BadgeConfig(
                variant=BadgeVariant.ICON, artwork=art, name="Download status"))
            panel = collapsible("panel", "Panel").open().children(image, marker.children(
                button("open").label("Open")))
            app.mount(column("root").children(panel))
            app.step()
            self.assertIs(image.result().error, IconError.NOT_FOUND)
            self.assertIs(marker.result().icon_error, IconError.NOT_FOUND)
            self.assertTrue(marker.result().submitted)
            self.assertEqual(marker.result().frame_status, 0)
            self.assertIsNotNone(marker.indicator_bounds())
            panel.open(False)
            app.step()
            self.assertIsNone(image.result())
            self.assertIsNone(marker.result())
            self.assertIsNone(marker.indicator_bounds())
            panel.open()
            art.put_asset(ASSET, SVG)
            app.step()
            self.assertIs(image.result().error, IconError.OK)
            marker.configure(BadgeConfig(variant=BadgeVariant.ICON, named_icon="check"))
            self.assertEqual(marker.get_config().named_icon, "check")
            app.step()
            self.assertIsNone(marker.result().icon_error)
            marker.configure(BadgeConfig(variant=BadgeVariant.DOT, color_rgba=0))
            self.assertEqual(marker.get_config().color_rgba, 0)
            app.step()
            self.assertIsNotNone(marker.indicator_bounds())

    def test_foreign_app_preflight_and_partial_mount_rollback(self):
        with App() as origin, App() as target:
            art = origin.icon_resource(IconSource.svg(SVG))
            modern = configured_icon("image", art)
            with patch.object(target._api, "node_create", wraps=target._api.node_create) as create:
                with self.assertRaisesRegex(ValueError, "different App"):
                    target.mount(column("foreign").children(modern))
                create.assert_not_called()
            root = column("root").children(
                modern, configured_badge("marker").children(badge("plain"), button("nested")))
            # The ordinary C capacity failure can occur after an Icon retains
            # its resource, and App.mount must unwind that reference.
            with App(max_nodes=2) as limited:
                owned = limited.icon_resource(IconSource.svg(SVG))
                view = configured_icon("art", owned)
                bad_tree = column("small").children(view, configured_badge("exceeds-limit"))
                with self.assertRaises(BridgeError) as err:
                    limited.mount(bad_tree)
                self.assertEqual(err.exception.code, 5)
                self.assertEqual(owned.info().references, 0)
                limited.mount(column("retry").children(view))
                limited.step()
                self.assertIs(view.result().error, IconError.OK)
            origin.mount(root)
            origin.step()
            self.assertIs(modern.result().error, IconError.OK)

    def test_argument_validation_and_malformed_result(self):
        with self.assertRaises(ValueError):
            IconOptions(width=float("inf"))
        with self.assertRaises(ValueError):
            IconStyle(padding=None)
        with self.assertRaises(ValueError):
            IconSource.svg("nul\0svg")
        with self.assertRaises(ValueError):
            IconOptions(transform=(1, 0, 0, 1, 0, 1000001))
        with self.assertRaises(ValueError):
            BadgeConfig(count=True)
        with self.assertRaises(ValueError):
            BadgeConfig(variant=BadgeVariant.ICON)
        with App() as app:
            art = app.icon_resource(IconSource.svg(SVG))
            node = configured_icon("icon", art)
            app.mount(column("root").children(node))
            app.step()
            with patch.object(app._api.icon_presentation, "get_node_result_v1", return_value=0):
                with self.assertRaisesRegex(BridgeError, "malformed Icon result"):
                    node.result()


class CompatibilityTests(unittest.TestCase):
    def test_published_alpha4_resource_only_provider(self):
        library = os.environ.get("CUI_ICON_BADGE_LEGACY_LIBRARY")
        if not library:
            self.skipTest("published alpha.4 provider not configured")
        with App(library=library) as app:
            self.assertTrue(app.supports_icon_resources())
            self.assertFalse(app.supports_icon_presentation())
            self.assertFalse(app.supports_configured_badge())
            art = app.icon_resource(IconSource.named("check"))
            self.assertIs(art.prepare(20, 20), IconError.OK)
            for new in (configured_badge("badge"), configured_icon("icon", art)):
                with patch.object(app._api, "node_create", wraps=app._api.node_create) as create:
                    with self.assertRaisesRegex(RuntimeError, "complete, compatible bridge"):
                        app.mount(column("preflight").children(new))
                    create.assert_not_called()
            legacy = icon("legacy", "check").visual_size(22, 18)
            app.mount(column("root").children(legacy, badge("pill", "New")))
            app.step()
            self.assertEqual(app.bounds(legacy)[2:], (22, 18))
            art.close()

    def test_real_partial_provider_keeps_complete_icon_group(self):
        library = os.environ.get("CUI_ICON_BADGE_PARTIAL_LIBRARY")
        if not library:
            self.skipTest("partial Badge provider not configured")
        with App(library=library) as app:
            self.assertTrue(hasattr(app._api.lib, "cui_bridge_badge_versions"))
            self.assertFalse(hasattr(app._api.lib, "cui_bridge_badge_indicator_bounds_v1"))
            self.assertTrue(app.supports_icon_presentation())
            self.assertFalse(app.supports_configured_badge())
            with self.assertRaisesRegex(RuntimeError, "complete, compatible bridge"):
                app.mount(column("bad").children(configured_badge("needs-bounds")))
            node = configured_icon("image", app.icon_resource(IconSource.svg(SVG)))
            app.mount(column("good").children(node, badge("legacy", "New")))
            app.step()
            self.assertIs(node.result().error, IconError.OK)

    def test_optional_group_rejects_wrong_sizes_and_version_without_calls(self):
        specs = {"versions": (), "record_size_v1": ()}
        actual = C.sizeof(_Options)
        for version, size in ((0, actual), (1, actual - 4)):
            functions = {"probe_versions": lambda: version, "probe_record_size_v1": lambda _: size}
            library = SimpleNamespace(**{"cui_bridge_" + name: fn for name, fn in functions.items()})
            group = _group(SimpleNamespace(lib=library), lambda name, *_: functions[name],
                           "probe_", specs, ((2, _Options, actual),))
            self.assertIsNone(group)


if __name__ == "__main__":
    unittest.main()
