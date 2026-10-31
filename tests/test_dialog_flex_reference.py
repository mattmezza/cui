#!/usr/bin/env python3
"""Offline authority/freshness tests; never download, compile or open a window."""
import hashlib
import csv
import math
import importlib.util
import io
import json
from pathlib import Path
import tarfile
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('dialog_flex_reference',
    ROOT / 'tests/reference/dialog_flex/reference.py')
REF = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(REF)


class ReferenceAuthority(unittest.TestCase):
    def test_frozen_engine_output_receipt(self):
        here = ROOT / 'tests/reference/dialog_flex'
        receipt = json.loads((here / 'golden-receipt.json').read_text())
        self.assertEqual(receipt['source_pin'], REF.SOURCE_PIN)
        self.assertEqual(receipt['features'], REF.FEATURES)
        self.assertEqual(receipt['output_sha256'], REF.digest((here / 'golden.csv').read_bytes()))
        self.assertEqual(receipt['driver_sha256'], REF.digest((here / 'main.rs').read_bytes()))
        self.assertEqual(receipt['helper_sha256'], REF.digest((here / 'reference.py').read_bytes()))
        for name, (version, sha) in REF.PINS.items():
            self.assertEqual(receipt['crates'][name]['version'], version)
            self.assertEqual(receipt['crates'][name]['archive_sha256'], sha)
        self.assertLessEqual(receipt['compiled_artifact_bytes'], 100 * 1024 * 1024)
        self.assertLessEqual(receipt['source_bytes'], 5 * 1024 * 1024)
        rows = list(csv.DictReader(io.StringIO((here / 'golden.csv').read_text())))
        frozen_prefix = '\n'.join((here / 'golden.csv').read_text().splitlines()[:19]) + '\n'
        self.assertEqual(REF.digest(frozen_prefix.encode()),
            '833669cc5a10d2e2ab48674fafce0b18b1d0c43cf66e7bfca2601c90cbdd33c1')
        self.assertEqual(len(rows), 28)
        self.assertEqual(len({r['case'] for r in rows}), 28)
        self.assertIn('content_minimum_overflow', {r['case'] for r in rows})
        keyed = {r['case']: r for r in rows}
        self.assertIn('source_footer_oversize_auto', keyed)
        # The independently executed engine must distinguish absent from an
        # explicit empty body wrapper; never silently rewrite golden rows.
        self.assertEqual(float(keyed['source_body_omitted']['body']), 0)
        self.assertGreater(float(keyed['source_body_empty']['body']), 0)

        for row in rows:
            self.assertTrue(all(math.isfinite(float(v)) and float(v) >= 0
                                for k, v in row.items() if k != 'case'))

    def fixture(self, base):
        cache = base / 'cache'
        cache.mkdir()
        here = base / 'helper'
        here.mkdir()
        (here / 'main.rs').write_text('fn main() {}\n')
        buffer = io.BytesIO()
        with tarfile.open(fileobj=buffer, mode='w:gz') as archive:
            for name, content in [('Cargo.toml', b'[package]\nname="taffy"\nversion="0.13.0"\n'),
                                  ('src/lib.rs', b'pub fn source_truth() {}\n')]:
                info = tarfile.TarInfo('taffy-0.13.0/' + name)
                info.size = len(content)
                archive.addfile(info, io.BytesIO(content))
        data = buffer.getvalue()
        (cache / 'taffy-0.13.0.crate').write_bytes(data)
        pins = {'taffy': ('0.13.0', hashlib.sha256(data).hexdigest())}
        return cache, here, pins

    def test_offline_prepare_and_exact_source_manifest(self):
        with tempfile.TemporaryDirectory() as temp:
            cache, here, pins = self.fixture(Path(temp))
            with patch.object(REF, 'PINS', pins), patch.object(REF, 'HERE', here), \
                    patch.object(REF.urllib.request, 'urlopen') as network:
                REF.prepare(cache, False)
                REF.verify(cache)
                network.assert_not_called()
                delta = cache / 'source/taffy-0.13.0/MANIFEST_DELTA.patch'
                self.assertIn('+taffy_tree=["dep:slotmap"]', delta.read_text())
                source = cache / 'source/taffy-0.13.0/src/lib.rs'
                source.write_text('pub fn poisoned() {}\n')
                # Even editing the claimed source digest cannot bypass archive authority.
                receipt = cache / 'provenance.json'
                value = json.loads(receipt.read_text())
                value['crates']['taffy']['rust_source_sha256'] = REF.source_digest(source.parent.parent)
                receipt.write_text(json.dumps(value))
                with self.assertRaisesRegex(SystemExit, 'source/manifest authority'):
                    REF.verify(cache)

    def test_manifest_and_driver_tamper(self):
        with tempfile.TemporaryDirectory() as temp:
            cache, here, pins = self.fixture(Path(temp))
            with patch.object(REF, 'PINS', pins), patch.object(REF, 'HERE', here):
                REF.prepare(cache, False)
                manifest = cache / 'source/taffy-0.13.0/Cargo.toml'
                manifest.write_text(manifest.read_text() + '\n# changed\n')
                with self.assertRaisesRegex(SystemExit, 'source/manifest authority'):
                    REF.verify(cache)
                REF.prepare(cache, False)
                (here / 'main.rs').write_text('fn main() { panic!("changed"); }\n')
                with self.assertRaisesRegex(SystemExit, 'driver changed'):
                    REF.verify(cache)

    def test_bad_archive_rejected_without_extraction(self):
        with tempfile.TemporaryDirectory() as temp:
            cache, here, pins = self.fixture(Path(temp))
            (cache / 'taffy-0.13.0.crate').write_bytes(b'wrong archive')
            with patch.object(REF, 'PINS', pins), patch.object(REF, 'HERE', here):
                with self.assertRaisesRegex(SystemExit, 'Archive checksum mismatch'):
                    REF.prepare(cache, False)
                self.assertFalse((cache / 'source').exists())

    def test_missing_archive_never_fetches_implicitly(self):
        with tempfile.TemporaryDirectory() as temp:
            cache = Path(temp) / 'cache'
            with patch.object(REF.Path, 'home', return_value=Path(temp)), \
                    patch.object(REF.urllib.request, 'urlopen') as network:
                with self.assertRaisesRegex(SystemExit, 'separate prepare --network'):
                    REF.prepare(cache, False)
                network.assert_not_called()


class ProductionAuthority(unittest.TestCase):
    def test_small_viewport_independent_receipt(self):
        here = ROOT / 'tests/reference/dialog_flex'
        receipt = json.loads((here / 'small-viewport-receipt.json').read_text())
        for filename, key in [('small_viewport.rs', 'driver_sha256'),
                              ('small-viewport.csv', 'output_sha256'),
                              ('golden.csv', 'frozen28_output_sha256'),
                              ('golden-receipt.json', 'engine_receipt_sha256')]:
            self.assertEqual(REF.digest((here / filename).read_bytes()), receipt[key])
        self.assertEqual(receipt['source_pin'], REF.SOURCE_PIN)
        self.assertEqual(receipt['features'], REF.FEATURES)
        lines = (here / 'small-viewport.csv').read_text().splitlines()
        self.assertEqual(len(lines), 5)
        self.assertEqual([float(row.split(',')[-1]) for row in lines[1:]], [96.] * 4)

    def test_geometry_probe_uses_unmodified_production_implementation(self):
        # A copied allocator would invalidate the independent runtime witness.
        # The C tests exercise geometry; this guard protects their authority.
        probe = (ROOT / 'tests/unit/dialog_flex_probe.c').read_text()
        self.assertIn('#include "../../src/core/cui.c"', probe)
        self.assertIn('#define CUI_IMPLEMENTATION', probe)
        self.assertIn('#include "cui.h"', probe)
        self.assertNotIn('CalculateFinalLayout', probe)
        self.assertNotIn('region_solve', probe)
        fixture = (ROOT / 'tests/unit/test_dialog_flex_regions.c').read_text()
        self.assertIn('cui_layout_scroll_body_v2(', fixture)
        self.assertNotIn('prototype_layout_regions', fixture)
        cmake = (ROOT / 'CMakeLists.txt').read_text()
        self.assertNotIn('embed_dialog_flex_probe.py', cmake)


if __name__ == '__main__':
    unittest.main()
