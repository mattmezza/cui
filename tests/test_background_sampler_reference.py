#!/usr/bin/env python3
"""Frozen pre-refactor sampler authority, never refreshed from the candidate."""
import hashlib
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class SamplerReferenceTests(unittest.TestCase):
    def test_original_source_receipt(self):
        receipt = json.loads((ROOT / 'tests/fixtures/background_sampler_v1.json').read_text())
        source = (ROOT / 'tests/fixtures/background_sampler_v1.c').read_bytes()
        self.assertEqual(hashlib.sha256(source).hexdigest(), receipt['sha256'])
        self.assertEqual(receipt['source_commit'], 'e5b64e4ef847047a028abf47766171976ce593d1')


if __name__ == '__main__':
    unittest.main()
