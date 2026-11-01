#!/usr/bin/env python3
"""Offline fail-closed parser/field-bound tests; no compiler/dependency install."""
import unittest
from software_public_layout import NAMES, check_pair, parse_layout


class LayoutTests(unittest.TestCase):
    def assembly(self, word):
        values = [word] + list(range(1, len(NAMES)))
        directive = '.quad' if word == 8 else '.int32'
        return ('cui_sw_layout_probe:\n' + ''.join(f'{directive} {n}\n' for n in values) +
                f'.size cui_sw_layout_probe, {len(NAMES)*word}\n'), values

    def test_native_and_wasm(self):
        for word in (4, 8):
            text, values = self.assembly(word)
            self.assertEqual(parse_layout(text, word), dict(zip(NAMES, values)))

    def test_missing_truncated_relocation_wrong_word(self):
        text, _ = self.assembly(8)
        for bad in ('', text.replace('.quad 19\n', ''),
                    text.replace('.quad 4', '.quad external_symbol'),
                    text.replace('.quad', '.int32')):
            with self.assertRaises(ValueError):
                parse_layout(bad, 8)

    def test_zero_and_unsigned_sentinel(self):
        text, _ = self.assembly(8)
        text = text.replace('.quad 1\n', '.zero 8\n').replace('.quad 12\n', '.quad -1\n')
        result = parse_layout(text, 8)
        self.assertEqual(result['size'], 0)
        self.assertEqual(result['v2_busy'], (1 << 64) - 1)
        with self.assertRaises(ValueError):
            parse_layout(text.replace('.zero 8', '.zero 7'), 8)

    def test_growth_and_old_offsets(self):
        old = dict.fromkeys(NAMES, 0)
        old.update(size=64, alignment=8, text_paint=32, paint_size=32)
        candidate = dict(old, size=72, v2_busy=64)
        check_pair(old, candidate)
        for changed in (dict(candidate, size=80), dict(candidate, width=1),
                        dict(candidate, v2_busy=63), dict(candidate, size=63)):
            with self.assertRaises(ValueError):
                check_pair(old, changed)


if __name__ == '__main__':
    unittest.main()
