#!/usr/bin/env python3
"""Frozen original source and byte oracle integrity; no compiler/display/deps."""
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests/fixtures/text_prepare_v1"

def matches(path, expected):
    return hashlib.sha256(path.read_bytes()).hexdigest() == expected

class Reference(unittest.TestCase):
    def test_original_sources_and_bytes(self):
        receipt = json.loads((FIXTURE / "receipt.json").read_text())
        self.assertEqual(receipt["reference_source_commit"], "f4e078610264674ed9b7e8bbb86f3a27d315e857")
        for name, digest in receipt["original_sources"].items():
            self.assertTrue(matches(FIXTURE / "source" / name, digest), name)
        self.assertEqual(receipt["source_sha256"]["src/text/cui_text.c"], receipt["original_sources"]["src/text/cui_text.c"])
        self.assertEqual(receipt["source_sha256"]["src/text/cui_bidi.c"], receipt["original_sources"]["src/text/cui_bidi.c"])
        self.assertTrue(matches(FIXTURE / "records_pixels.golden", receipt["golden_sha256"]))
        self.assertTrue(matches(FIXTURE / "records_pixels_nobidi.golden", receipt["nobidi_golden_sha256"]))
        for name, digest in receipt["fonts"].items():
            self.assertTrue(matches(ROOT / name, digest), name)

    def test_changed_payload_is_rejected(self):
        receipt = json.loads((FIXTURE / "receipt.json").read_text())
        data = bytearray((FIXTURE / "records_pixels.golden").read_bytes())
        data[-1] ^= 1
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "changed.golden"
            path.write_bytes(data)
            self.assertFalse(matches(path, receipt["golden_sha256"]))

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--compare-reject":
        # Optional captured-toolchain native check; invoke under test_local/Xvfb.
        binary, regular, bold, golden = sys.argv[2:]
        with tempfile.TemporaryDirectory() as directory:
            changed = Path(directory) / "changed.golden"
            data = bytearray(Path(golden).read_bytes())
            data[-1] ^= 1
            changed.write_bytes(data)
            env = dict(os.environ, CUI_TEXT_FROZEN_EXACT="1")
            result = subprocess.run([binary, regular, bold, str(changed)], env=env,
                                    capture_output=True, text=True, check=False)
            if result.returncode != 2 or "Captured font fixture mismatch." not in result.stderr:
                raise SystemExit("native fixture mutation was not rejected normally")
    else:
        unittest.main()
