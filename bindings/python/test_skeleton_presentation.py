"""Runtime and compatibility checks for Python's responsive Skeleton facade."""
import math
import os
import unittest
from dataclasses import replace
from unittest.mock import patch

from cui_bridge import App, BridgeError, SkeletonConfig, SkeletonWidth, collapsible, column
from cui_bridge import responsive_skeleton, skeleton


class SkeletonTests(unittest.TestCase):
    def test_geometry_and_copied_style(self):
        for width in (320, 480):
            with self.subTest(width=width), App(width=width, height=180) as app:
                self.assertTrue(app.supports_skeleton_presentation())
                style = SkeletonConfig(width=.65, radius=6, secondary=True, background_rgba=0)
                line = responsive_skeleton("line", style)
                circle = responsive_skeleton("circle", SkeletonConfig(
                    width_mode=SkeletonWidth.FIXED, width=40, height=40, radius=20))
                capped = responsive_skeleton("capped", SkeletonConfig(
                    width_mode=SkeletonWidth.GROW, width=0, max_width=128))
                legacy = skeleton("legacy", "Original").visual_size(72, 12)
                self.assertEqual(line.get_config(), style)
                with self.assertRaisesRegex(TypeError, "SkeletonConfig"):
                    line.visual_size(1, 1)
                app.mount(column("root").children(line, circle, capped, legacy))
                self.assertEqual(line.get_config(), style)
                self.assertIsNone(line.presentation())
                app.step(.25)
                self.assertAlmostEqual(app.bounds(line)[2], (width - 32) * .65, places=4)
                self.assertEqual(app.bounds(circle)[2:], (40, 40))
                self.assertEqual(app.bounds(capped)[2:], (128, 16))
                self.assertEqual(app.bounds(legacy)[2:], (72, 12))
                self.assertEqual(app.dispatch(), 0)

    def test_read_only_timing_and_restylings(self):
        line = responsive_skeleton("line")
        with App() as app:
            app.mount(column("root").children(line))
            app.step(.25)
            first = line.presentation()
            self.assertEqual(line.presentation(), first)
            self.assertEqual(first.phase, 0)
            self.assertAlmostEqual(first.next_wake_seconds - first.core_time, 1 / 60)
            line.configure(replace(line.get_config(), secondary=True, width=.5))
            self.assertIsNone(line.presentation())
            app.step(.5)
            updated = line.presentation()
            self.assertGreater(updated.frame_sequence, first.frame_sequence)
            self.assertEqual((updated.phase, updated.opacity), (.5, .75))
            line.configure(replace(line.get_config(), reduced_motion=True))
            app.step(.25)
            stable = line.presentation()
            self.assertEqual((stable.phase, stable.opacity, stable.next_wake_seconds), (0, 1, 0))
            line.configure(replace(line.get_config(), reduced_motion=False))
            app.step(.25)
            self.assertEqual(line.presentation().phase, 0)
            self.assertGreater(line.presentation().next_wake_seconds, 0)
            self.assertEqual(first.phase, 0)

    def test_omission_and_removed_lifecycle(self):
        line = responsive_skeleton("line")
        panel = collapsible("panel", "Profile").open().children(line)
        with App() as app:
            app.mount(column("root").children(panel))
            app.step(.25)
            self.assertEqual(line.presentation().phase, 0)
            panel.open(False)
            app.step(.25)
            self.assertIsNone(line.presentation())
            panel.open(True)
            app.step(.25)
            self.assertEqual(line.presentation().phase, 0)
            line.remove()
            with self.assertRaises(ValueError):
                app.bounds(line)
            with self.assertRaises(RuntimeError):
                line.presentation()
            with self.assertRaises(RuntimeError):
                line.get_config()
            app.step(.25)

    def test_invalid_configuration_and_atomic_validation(self):
        for fields in (dict(width=0), dict(width=math.nan), dict(width=10**1000), dict(width=1.1),
                       dict(height=0), dict(radius=-1), dict(duration=math.inf),
                       dict(duration=1e-100), dict(max_width=1), dict(background_rgba=-1),
                       dict(background_rgba=True)):
            with self.subTest(fields=fields), self.assertRaises(ValueError):
                SkeletonConfig(**fields)
        with self.assertRaises(TypeError):
            SkeletonConfig(width_mode=0)
        with self.assertRaises(TypeError):
            SkeletonConfig(secondary=1)
        line = responsive_skeleton("line")
        with App() as app:
            app.mount(column("root").children(line))
            app.step()
            before = line.presentation()
            with self.assertRaises(TypeError):
                line.configure({})
            self.assertEqual(line.get_config(), SkeletonConfig())
            self.assertEqual(line.presentation(), before)

    def test_rejected_configuration_retains_mounted_style(self):
        line = responsive_skeleton("line")
        with App() as app:
            app.mount(column("root").children(line))
            app.step()
            before = line.presentation()
            with patch.object(app._api.skeleton_presentation, "configure_v1", return_value=6):
                with self.assertRaises(BridgeError) as failure:
                    line.configure(SkeletonConfig(width=.5))
            self.assertEqual(failure.exception.code, 6)
            self.assertEqual(line.get_config(), SkeletonConfig())
            self.assertEqual(line.presentation(), before)

    def test_partial_mount_rolls_back_configured_nodes(self):
        avatar = responsive_skeleton("avatar")
        line = responsive_skeleton("line")
        tree = column("root").children(avatar, line)
        with App(max_nodes=3) as app:
            configure = app._api.skeleton_presentation.configure_v1
            attempts = 0

            def configure_first(*args):
                nonlocal attempts
                attempts += 1
                return configure(*args) if attempts == 1 else 6  # OUT_OF_MEMORY

            with patch.object(app._api.skeleton_presentation, "configure_v1",
                              side_effect=configure_first):
                with self.assertRaises(BridgeError):
                    app.mount(tree)
            self.assertEqual(attempts, 2)
            self.assertIsNone(tree._handle)
            self.assertIsNone(avatar._handle)
            self.assertIsNone(line._handle)
            app.mount(tree)
            app.step(.25)
            self.assertEqual(line.presentation().phase, 0)

    def _check_missing_provider(self, app):
        modern = responsive_skeleton("modern")
        tree = column("root").children(modern)
        self.assertFalse(app.supports_skeleton_presentation())
        with self.assertRaisesRegex(RuntimeError, "compatible bridge symbol group"):
            app.mount(tree)
        self.assertIsNone(tree._handle)
        self.assertIsNone(modern._handle)
        legacy = skeleton("legacy").visual_size(96, 20)
        app.mount(column("legacy-root").children(legacy))
        self.assertEqual(legacy._handle.generation, 1)  # No native mount was attempted.
        app.step()
        self.assertEqual(app.bounds(legacy)[2:], (96, 20))

    def test_incompatible_abi_probe(self):
        for function, value in (("versions", 0), ("config_size_v1", 36), ("presentation_size_v1", 32)):
            with self.subTest(function=function), App() as app, patch.object(
                    app._api.skeleton_presentation, function, return_value=value):
                self._check_missing_provider(app)

    def test_published_legacy_provider(self):
        library = os.environ.get("CUI_SKELETON_LEGACY_LIBRARY")
        if not library:
            self.skipTest("published legacy provider not configured")
        with App(library=library) as app:
            self._check_missing_provider(app)

    def test_partial_optional_symbol_group(self):
        library = os.environ.get("CUI_SKELETON_PARTIAL_LIBRARY")
        if not library:
            self.skipTest("partial provider not configured")
        with App(library=library) as app:
            self.assertTrue(hasattr(app._api.lib, "cui_bridge_skeleton_versions"))
            self.assertFalse(hasattr(app._api.lib, "cui_bridge_skeleton_get_presentation_v1"))
            self._check_missing_provider(app)


if __name__ == "__main__":
    unittest.main()
