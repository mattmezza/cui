#!/usr/bin/env python3
"""Offline integrity of the independent pre-extraction surface oracle."""
import hashlib
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class DialogSurfaceReferenceTests(unittest.TestCase):
    def test_frozen_source_receipt(self):
        folder = ROOT / "tests/reference/dialog_surface"
        receipt = json.loads((folder / "provenance.json").read_text())
        source = (folder / "legacy_surface.c").read_bytes()
        self.assertEqual(hashlib.sha256(source).hexdigest(), receipt["sha256"])
        self.assertEqual(receipt["source_revision"], "d4a1d8479a5af5a81265e37eba1cbb3b465609cc")
        self.assertEqual(receipt["source_path"], "src/base/cui_surface.c")

    def test_probe_only_renames_original_symbols(self):
        source = (ROOT / "tests/reference/dialog_surface/legacy_surface.c").read_text()
        names = re.findall(r"^(?!static)(?:bool|void|cui_surface_result|cui_id|cui_rect) (cui_\w+)\(",
                           source, re.M)
        probe = (ROOT / "tests/unit/dialog_surface_legacy_probe.c").read_text()
        renames = re.findall(r"^#define (cui_\w+) (legacy_cui_\w+)$", probe, re.M)
        self.assertEqual(renames, [(name, "legacy_" + name) for name in names])
        self.assertEqual(len(names), 18)
        self.assertIn('#include "../reference/dialog_surface/legacy_surface.c"', probe)
        self.assertNotIn("cui_base_surface_update(", probe)


if __name__ == "__main__":
    unittest.main()
