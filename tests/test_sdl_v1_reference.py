#!/usr/bin/env python3
"""SDL baseline source-only checks: no compile, native window or dependencies."""
import unittest
from sdl_v1_reference import FIXTURE, FROZEN, ROOT, layout_source, verify


class SDLV1SourceTests(unittest.TestCase):
    def test_original_and_dependencies_frozen(self):
        verify()

    def test_baseline_matches_preproduction_source(self):
        # Remove this equality only when reviewed production SDL V2 begins;
        # immutable frozen authority and paired-runtime gate must remain.
        for path in ('backends/sdl3/cui_sdl3.c', 'backends/sdl3/cui_sdl3_mesh.h',
                     'include/cui/cui_sdl3.h'):
            self.assertEqual((ROOT / path).read_bytes(), (FIXTURE / path.split('/')[-1]).read_bytes())

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


if __name__ == '__main__':
    unittest.main()
