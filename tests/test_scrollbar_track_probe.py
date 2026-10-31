#!/usr/bin/env python3
"""Offline exact-seam protection for the TEST-ONLY mutating adapter copy."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'tests/unit/embed_scrollbar_track_probe.py'
INCLUDE = '#include "cui_scroll_layout_internal.h"'
SEAMS = (
    '    uint8_t order[CUI_CORE_SL_BODY_MAX];',
    '    uint64_t widths = cui_core_sl_widths(context);',
    '    Clay__CalculateFinalLayout(0, false, true);',
    '    state->header.flags = CUI_CORE_SL_READY;',
)

class TrackProbeTests(unittest.TestCase):
    def test_exact_copy_and_authority(self):
        source = (ROOT / 'src/core/cui.c').read_text()
        private = (ROOT / 'src/core/cui_scroll_layout_internal.h').read_text()
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'copy.inc'
            subprocess.run([sys.executable, str(SCRIPT), str(ROOT), str(output)], check=True)
            result = output.read_text()
            self.assertNotIn(INCLUDE, result)
            self.assertEqual(result.count('    prototype_track_entry tracks[64];'), 1)
            for call in ('prototype_track_stage(ui, state)',
                         'prototype_track_validate(ui, state)',
                         'prototype_track_publish(ui, state)'):
                self.assertEqual(result.count(call), 1)
            # Four specified private substitutions; all other Core bytes exact.
            edited = private
            edited = edited.replace(SEAMS[0], SEAMS[0] + '\n'
                '    uint32_t track_count, track_reserved;\n'
                '    prototype_track_entry tracks[64];')
            edited = edited.replace(SEAMS[1],
                '    if (!prototype_track_stage(ui, state))\n'
                '        return cui_core_sl_abort(ui);\n' + SEAMS[1])
            edited = edited.replace(SEAMS[2], SEAMS[2] + '\n'
                '    if (!prototype_track_validate(ui, state))\n'
                '        return cui_core_sl_abort(ui);')
            edited = edited.replace(SEAMS[3],
                '    prototype_track_publish(ui, state);\n' + SEAMS[3])
            self.assertEqual(result, source.replace(INCLUDE, edited))
        self.assertEqual((ROOT / 'src/core/cui.c').read_text(), source)
        self.assertEqual((ROOT / 'src/core/cui_scroll_layout_internal.h').read_text(), private)

    def test_bad_seams_preserve_prior_output(self):
        source = (ROOT / 'src/core/cui.c').read_text()
        private = (ROOT / 'src/core/cui_scroll_layout_internal.h').read_text()
        for needle in (INCLUDE, *SEAMS):
            for count in (0, 2):
                with self.subTest(needle=needle, count=count), tempfile.TemporaryDirectory() as d:
                    root = Path(d)
                    (root / 'src/core').mkdir(parents=True)
                    (root / 'src/core/cui.c').write_text(source.replace(needle, needle * count))
                    (root / 'src/core/cui_scroll_layout_internal.h').write_text(
                        private.replace(needle, needle * count))
                    output = root / 'copy.inc'
                    output.write_text('preserve')
                    done = subprocess.run([sys.executable, str(SCRIPT), str(root), str(output)],
                                          capture_output=True, text=True)
                    self.assertNotEqual(done.returncode, 0)
                    self.assertIn('review required', done.stderr)
                    self.assertEqual(output.read_text(), 'preserve')

if __name__ == '__main__':
    unittest.main()
