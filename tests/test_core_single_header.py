#!/usr/bin/env python3
"""Offline guard for the legacy file-only Core implementation prerequisite."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]


class CoreSingleHeaderTests(unittest.TestCase):
    def test_no_new_local_header_prerequisites(self):
        core = (ROOT / 'dist/cui.h').read_text()
        core = re.sub(r'/\*.*?\*/|//[^\n]*', '', core, flags=re.S)
        self.assertEqual(re.findall(r'^\s*#\s*include\s*"([^"]+)"', core, re.M), [])
        for name in ('CUI_BACKGROUND', 'CUI_DRAW_V2'):
            self.assertTrue('#define ' + name + '_DECLARATIONS_READY' in core, name + ' readiness missing')
            self.assertFalse('#define ' + name + '_IMPLEMENTATION_ONCE' in core, name + ' implementation emitted eagerly')


if __name__ == '__main__':
    unittest.main()
