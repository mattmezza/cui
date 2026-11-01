"""Compare real bridge output with the immutable pre-resource capture, offline."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
EXECUTABLE = None


class IconLegacy(unittest.TestCase):
    def test_frozen_commands_pixels_events_and_allocations(self):
        if EXECUTABLE is None:
            self.skipTest("legacy trace executable not supplied")
        frozen = ROOT / "tests/fixtures/bridge_icon_resources_v1"
        with tempfile.TemporaryDirectory(prefix="cui-icon-legacy-") as directory:
            trace = Path(directory) / "legacy.trace"
            allocations = Path(directory) / "constructor.alloc"
            subprocess.run([EXECUTABLE, str(trace), str(allocations)],
                           check=True, timeout=30)
            self.assertEqual(trace.read_bytes(), (frozen / "legacy.trace").read_bytes())
            before = (frozen / "constructor.alloc").read_text().splitlines()
            after = allocations.read_text().splitlines()
            # The sole permitted legacy allocation change is the appended app
            # resource-pool pointer. No allocation call is added or reordered.
            self.assertEqual(before, ["calloc 1 1304", "calloc 33 73136",
                                      "calloc 32 24", "calloc 32 72"])
            self.assertEqual(after, ["calloc 1 1312", *before[1:]])


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", required=True)
    arguments, rest = parser.parse_known_args()
    EXECUTABLE = arguments.executable
    unittest.main(argv=[__file__, *rest])
