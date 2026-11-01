#!/usr/bin/env python3
"""SDL baseline source-only checks: no compile, native window or dependencies."""
import unittest
from sdl_v1_reference import FIXTURE, FROZEN, ROOT, layout_source, verify


class SDLV1SourceTests(unittest.TestCase):
    def test_original_and_dependencies_frozen(self):
        verify()

    def test_production_preserves_original_private_fields_and_constructor_count(self):
        original = (FIXTURE / 'cui_sdl3.c').read_text()
        current = (ROOT / 'backends/sdl3/cui_sdl3.c').read_text()
        def fields(source):
            return source.split('struct cui_sdl3 {', 1)[1].split('};', 1)[0].split()
        self.assertEqual(fields(current.replace('    bool v2_busy, v2_faulted;\n', '')), fields(original))
        self.assertEqual(current.count('cui_sdl3 *s = calloc(1, sizeof(*s));'), 1)
        self.assertEqual((ROOT / 'backends/sdl3/cui_sdl3_mesh.h').read_bytes(),
                         (FIXTURE / 'cui_sdl3_mesh.h').read_bytes())

    def test_guard_layout_candidate_is_only_two_appended_bools(self):
        old, candidate = layout_source(), layout_source(True)
        self.assertNotIn('bool v2_busy, v2_faulted;', old)
        self.assertEqual(candidate.replace('    bool v2_busy, v2_faulted;\n', '').replace(
            'const unsigned sdl_v1_offset_v2_busy = offsetof(cui_sdl3, v2_busy);\n', '').replace(
            'const unsigned sdl_v1_offset_v2_faulted = offsetof(cui_sdl3, v2_faulted);\n', ''), old)
        self.assertNotIn('SDL_CreateRenderer(', old)
        self.assertNotIn('SDL_RenderPresent(', old)
        self.assertIn('const unsigned sdl_v1_offset_density_x', old)
        self.assertEqual(len(FROZEN), 3)

    def test_measurement_includes_every_old_field_and_constructor_request(self):
        source = layout_source()
        fields = ['ime_composing', 'ime_commit_pending', 'input_suspended', 'hits',
                  'misses', 'frame_ticks', 'event_pending', 'callback_mode', 'motion_pending']
        for field in fields:
            self.assertIn('sdl_v1_offset_' + field + ' = offsetof(cui_sdl3, ' + field + ')', source)
        self.assertIn('sdl_v1_constructor_bytes = sizeof(cui_sdl3)', source)
        original = (FIXTURE / 'cui_sdl3.c').read_text()
        self.assertEqual(original.count('cui_sdl3 *s = calloc(1, sizeof(*s));'), 1)


if __name__ == '__main__':
    unittest.main()
