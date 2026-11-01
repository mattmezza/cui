"""Offline independent Skeleton source-token oracle; optional actual C dumper."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('skeleton_generator', ROOT / 'scripts/generate_skeleton_palettes.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)
DUMPER = None


def expected():
    # Independent normalized extraction, not generator's raw input/parser/sort.
    presets = json.loads((ROOT / 'third_party/gpui_themes/presets.json').read_text())
    result = []
    for preset in presets:
        value = preset['colors'].get('skeleton.background', preset['colors']['secondary.background'])
        result.append(tuple(int(value[i:i + 2], 16) for i in (1, 3, 5, 7)))
    return result


class SkeletonPalette(unittest.TestCase):
    def test_independent_all38(self):
        table = (ROOT / 'src/theme/cui_skeleton_palette_presets.inc').read_text()
        rows = [tuple(map(int, match)) for match in re.findall(r'\{\{(\d+), (\d+), (\d+), (\d+)\}\}', table)]
        self.assertEqual(len(rows), 38)
        self.assertEqual(rows, expected())
        self.assertEqual(rows[0], (245, 245, 245, 255))
        self.assertEqual(rows[1], (241, 245, 249, 255))
        names = json.loads((ROOT / 'third_party/gpui_themes/presets.json').read_text())
        dark = next(i for i, p in enumerate(names) if p['name'] == 'Default Dark')
        self.assertEqual(rows[dark], (23, 23, 23, 255))

    def test_check_missing_changed_and_idempotence(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src/theme').mkdir(parents=True)
            (root / 'third_party/gpui_themes').mkdir(parents=True)
            for name in ('palettes.json', 'presets.json'):
                (root / 'third_party/gpui_themes' / name).write_bytes((ROOT / 'third_party/gpui_themes' / name).read_bytes())
            old = generator.ROOT
            generator.ROOT = root
            try:
                target = root / 'src/theme/cui_skeleton_palette_presets.inc'
                output = generator.generate().encode()
                # Actual CLI main/check path with isolated root and argv, no mirror helper.
                original = sys.argv
                try:
                    sys.argv = ['generator', '--check']
                    with self.assertRaises(SystemExit): generator.main()
                    sys.argv = ['generator']; generator.main()
                    self.assertEqual(target.read_bytes(), output)
                    stamp = target.stat().st_mtime_ns
                    generator.main(); self.assertEqual(target.stat().st_mtime_ns, stamp)
                    target.write_bytes(b'corrupt')
                    sys.argv = ['generator', '--check']
                    with self.assertRaises(SystemExit): generator.main()
                    sys.argv = ['generator']; generator.main()
                    self.assertEqual(target.read_bytes(), output)
                    sys.argv = ['generator', '--check']; generator.main()
                finally: sys.argv = original
                raw = root / 'third_party/gpui_themes/palettes.json'
                raw.write_bytes(raw.read_bytes() + b' ')
                with self.assertRaises(ValueError): generator.generate()
            finally: generator.ROOT = old

    def test_explicit_token_does_not_eagerly_require_fallback(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            folder = root / 'third_party/gpui_themes'
            folder.mkdir(parents=True)
            source = json.loads((ROOT / 'third_party/gpui_themes/palettes.json').read_text())
            explicit = next(t for t in source['themes'] if 'skeleton.background' in t['colors'])
            del explicit['colors']['secondary.background']
            raw = json.dumps(source).encode()
            (folder / 'palettes.json').write_bytes(raw)
            (folder / 'presets.json').write_bytes((ROOT / 'third_party/gpui_themes/presets.json').read_bytes())
            old_root, old_pin = generator.ROOT, generator.PIN
            generator.ROOT, generator.PIN = root, hashlib.sha256(raw).hexdigest()
            try:
                self.assertEqual(generator.generate(), (ROOT / 'src/theme/cui_skeleton_palette_presets.inc').read_text())
                fallback = next(t for t in source['themes'] if 'skeleton.background' not in t['colors'])
                del fallback['colors']['secondary.background']
                raw = json.dumps(source).encode()
                (folder / 'palettes.json').write_bytes(raw)
                generator.PIN = hashlib.sha256(raw).hexdigest()
                with self.assertRaises(KeyError): generator.generate()
            finally:
                generator.ROOT, generator.PIN = old_root, old_pin

    @unittest.skipUnless(DUMPER, 'actual C dumper path not supplied')
    def test_actual_c_records(self):
        rows = []
        for line in subprocess.check_output([DUMPER], text=True).splitlines():
            numbers = tuple(map(int, line.split()))
            self.assertEqual(numbers[0], len(rows))
            rows.append(numbers[1:])
        self.assertEqual(rows, expected())


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--dumper')
    args, rest = parser.parse_known_args()
    DUMPER = args.dumper
    unittest.main(argv=[sys.argv[0], *rest])
