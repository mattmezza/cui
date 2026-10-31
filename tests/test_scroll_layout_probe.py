#!/usr/bin/env python3
"""Offline exact-seam tests; never compile, install dependencies or open windows."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'tests/unit/embed_scroll_layout_probe.py'
INCLUDE = '#include "cui_scroll_layout_internal.h"'
BEFORE = '    uint64_t widths = cui_core_sl_widths(context);'
AFTER = '    Clay__CalculateFinalLayout(0, false, true);'


class ProbeTests(unittest.TestCase):
    def test_exact_observation_only_substitutions(self):
        source = (ROOT / 'src/core/cui.c').read_text()
        private = (ROOT / 'src/core/cui_scroll_layout_internal.h').read_text()
        expected = source.replace(INCLUDE, private.replace(
            BEFORE, '    probe_before(ui);\n' + BEFORE).replace(
            AFTER, AFTER + '\n    probe_after(ui);'))
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'core.inc'
            subprocess.run([sys.executable, str(SCRIPT), str(ROOT), str(output)], check=True)
            self.assertEqual(output.read_text(), expected)
        self.assertEqual((ROOT / 'src/core/cui.c').read_text(), source)
        self.assertEqual((ROOT / 'src/core/cui_scroll_layout_internal.h').read_text(), private)

    def test_missing_duplicate_seams_preserve_output(self):
        source = (ROOT / 'src/core/cui.c').read_text()
        private = (ROOT / 'src/core/cui_scroll_layout_internal.h').read_text()
        for needle in (INCLUDE, BEFORE, AFTER):
            for count in (0, 2):
                with self.subTest(needle=needle, count=count), tempfile.TemporaryDirectory() as d:
                    root = Path(d)
                    (root / 'src/core').mkdir(parents=True)
                    (root / 'src/core/cui.c').write_text(source.replace(needle, needle * count))
                    (root / 'src/core/cui_scroll_layout_internal.h').write_text(
                        private.replace(needle, needle * count))
                    output = root / 'output.inc'
                    output.write_text('keep prior output')
                    result = subprocess.run([sys.executable, str(SCRIPT), str(root), str(output)],
                                            capture_output=True, text=True)
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn('review required', result.stderr)
                    self.assertEqual(output.read_text(), 'keep prior output')


if __name__ == '__main__':
    unittest.main()
