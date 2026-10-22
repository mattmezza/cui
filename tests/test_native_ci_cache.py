#!/usr/bin/env python3
"""Offline policy for raw optional-toolchain caching; never installs tools."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
CACHE = 'actions/cache@0057852bfaa89a56745cba8c7296529d2fc39830'
KEY = "pages-regex-archives-${{ runner.os }}-${{ runner.arch }}-${{ hashFiles('backends/regex/pin.json') }}"
PATHS = ['build/regex-toolchain/channel.toml', 'build/regex-toolchain/host.tar.xz',
         'build/regex-toolchain/wasi.tar.xz']


def validate(text):
    section = text.split('  optional-regex:\n', 1)[1].split('\n  go-rust-builders:', 1)[0]
    steps = re.split(r'(?m)^      - ', section)[1:]
    caches = [s for s in steps if 'uses: actions/cache@' in s]
    errors = []
    if len(caches) != 1:
        return ['exactly one raw input cache required']
    cache = caches[0]
    paths = [l.strip() for l in cache.splitlines() if l.startswith('            ')]
    if CACHE not in cache or paths != PATHS or 'key: '+KEY+'\n' not in cache:
        errors.append('cache must use immutable action, exact inputs and checksum pin key')
    if any(x in cache for x in ('restore-keys:', 'if:', 'continue-on-error:')):
        errors.append('cache must not fall back or condition execution')
    if any(x in section for x in ('cache-hit', 'continue-on-error:', '        if:')):
        errors.append('all preparation and checks remain unconditional')
    required = [
        'python tests/test_native_ci_cache.py',
        'python scripts/prepare_regex_toolchain.py --cache build/regex-toolchain',
        'python scripts/build_regex.py --cache build/regex-native --toolchain build/regex-toolchain/rust-1.85.1',
        'cargo test --offline --locked --release',
        'cmake --build build/native',
        'ctest --test-dir build/native --output-on-failure',
    ]
    for command in required:
        if command not in section:
            errors.append('missing unconditional check: '+command)
    if section.index('name: Cache pinned') > section.index('name: Prepare checksum'):
        errors.append('restore must precede checksum verification')
    return errors


class NativeCacheTests(unittest.TestCase):
    def test_actual_workflow(self):
        self.assertEqual(validate((ROOT / '.github/workflows/ci.yml').read_text()), [])

    def test_rejects_unsafe_cache_and_bypass(self):
        source = (ROOT / '.github/workflows/ci.yml').read_text()
        for before, after in (
            (CACHE, 'actions/cache@v4'),
            (PATHS[0], 'build/regex-toolchain'),
            (KEY, 'unkeyed-toolchain'),
            ('          key: '+KEY, '          restore-keys: broad\n          key: '+KEY),
            ('      - name: Prepare checksum-verified private optional toolchain',
             '      - if: steps.cache.outputs.cache-hit != true\n        name: Prepare checksum-verified private optional toolchain'),
            ('ctest --test-dir build/native --output-on-failure', 'true'),
            ('cargo test --offline --locked --release', 'cargo test'),
        ):
            with self.subTest(after=after):
                self.assertNotEqual(source, source.replace(before, after))
                self.assertTrue(validate(source.replace(before, after)))

    def test_matches_pages_input_cache(self):
        pages = (ROOT / '.github/workflows/pages.yml').read_text()
        block = pages.split('      - name: Cache pinned regex toolchain archives\n', 1)[1].split('      - name:', 1)[0]
        self.assertIn(CACHE, block)
        self.assertIn('key: '+KEY, block)
        self.assertEqual([l.strip() for l in block.splitlines() if l.startswith('            ')], PATHS)


if __name__ == '__main__':
    unittest.main()
