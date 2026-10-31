#!/usr/bin/env python3
"""Offline V1 enum authority and frozen lossless oracle integrity."""
import hashlib
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'tests/fixtures/background_v1'


def enum_body(text, name):
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    match = re.search(r'typedef\s+enum\s*\{([^{}]*)\}\s*' + re.escape(name) + r'\s*;', text)
    if not match:
        raise ValueError('Missing enum ' + name)
    return re.sub(r'\s+', '', match[1])


class FrozenV1Tests(unittest.TestCase):
    def test_public_enums_match_frozen_authority(self):
        public = (ROOT / 'include/cui/cui.h').read_text()
        frozen = (FIXTURE / 'frozen_abi.h').read_text()
        names = re.findall(r'}\s*v1_frozen_(cui_\w+)\s*;', frozen)
        checked = 0
        for name in names:
            try:
                expected = enum_body(frozen, 'v1_frozen_' + name)
            except ValueError:
                continue
            expected = expected.replace('V1_FROZEN_CUI_', 'CUI_')
            self.assertEqual(enum_body(public, name), expected, name + ' V1 enum changed')
            checked += 1
        self.assertGreaterEqual(checked, 6)

    def test_extra_v1_kind_is_detectable(self):
        public = (ROOT / 'include/cui/cui.h').read_text()
        expected = enum_body(public, 'cui_draw_type')
        changed = public.replace('    CUI_DRAW_INSET_SHADOW\n',
                                 '    CUI_DRAW_INSET_SHADOW, CUI_DRAW_NOT_V1\n')
        self.assertNotEqual(enum_body(changed, 'cui_draw_type'), expected)

    def test_receipt_owns_test_inputs_and_golden(self):
        receipt = json.loads((FIXTURE / 'provenance.json').read_text())
        for name, digest in receipt['sha256'].items():
            if name.startswith('tests/'):
                self.assertEqual(hashlib.sha256((ROOT / name).read_bytes()).hexdigest(),
                                 digest, name + ' changed since capture')
        self.assertEqual(receipt['capture_status'], 'PASS')

    def test_lossless_pixel_runs_and_all_v1_kinds(self):
        cases, commands, pixels, kinds = [], 0, 0, set()
        expected_commands = None
        for line in (FIXTURE / 'commands_pixels.golden').read_text().splitlines():
            words = line.split()
            if words[0] == 'S':
                if expected_commands is not None:
                    self.assertEqual(commands, expected_commands)
                    self.assertEqual(pixels, 96 * 72)
                cases.append((int(words[1]), int(words[2])))
                expected_commands = int(words[3])
                self.assertEqual(int(words[4]), int(int(words[1]) == 4))
                commands, pixels = 0, 0
            elif words[0] == 'C':
                commands += 1
                kinds.add(int(words[1]))
            elif words[0] == 'P':
                self.assertEqual(len(words), 6)
                run, *rgba = map(int, words[1:])
                self.assertGreater(run, 0)
                self.assertTrue(all(0 <= value <= 255 for value in rgba))
                pixels += run
            else:
                self.fail('Unknown frozen record')
        self.assertEqual(commands, expected_commands)
        self.assertEqual(pixels, 96 * 72)
        self.assertEqual(cases, [(scene, stage) for scene in range(8) for stage in range(3)])
        self.assertEqual(kinds, set(range(9)))


if __name__ == '__main__':
    unittest.main()
