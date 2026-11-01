#!/usr/bin/env python3
"""Immutable validator authority and distribution closure, without compilation."""
import hashlib
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SHA = 'dbd240c716dd3ff11e035f18a8827a0d3c3630ee51351bc1a19c8b639c52087b'


class RangeReferenceTests(unittest.TestCase):
    def test_frozen_authority_and_mutation(self):
        fixture = ROOT / 'tests/fixtures/cpu_v2_range_v1'
        data = (fixture / 'cui_cpu_v2_internal.h').read_bytes()
        receipt = json.loads((fixture / 'receipt.json').read_text())
        self.assertEqual(receipt['source_commit'], 'b386ec55b907b8431339ba1897a33d497200a87d')
        self.assertEqual(receipt['sha256'], SHA)
        self.assertEqual(hashlib.sha256(data).hexdigest(), SHA)
        self.assertNotEqual(hashlib.sha256(data + b'\n').hexdigest(), SHA)

    def test_sdk_carries_new_transitive_header(self):
        spec = importlib.util.spec_from_file_location('range_sdk', ROOT / 'scripts/generate_go_sdk.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        path = 'backends/common/cui_cpu_v2_sources_internal.h'
        self.assertEqual(module.snapshot()[path], (ROOT / path).read_bytes())


if __name__ == '__main__':
    unittest.main()
