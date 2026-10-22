#!/usr/bin/env python3
"""Offline Pages cache contracts; no tools installed and no browsers launched."""
from pathlib import Path
import re
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / '.github/workflows/pages.yml'
CACHE = 'actions/cache@0057852bfaa89a56745cba8c7296529d2fc39830'


def job(text, name):
    section = text.split('  '+name+':\n', 1)[1]
    return re.split(r'\n  [\w-]+:\n', section, maxsplit=1)[0]


def steps(section):
    return re.split(r'(?m)^      - ', section)[1:]


def named(section, name):
    return next(step for step in steps(section) if step.startswith('name: '+name+'\n'))


def validate(text):
    errors = []
    build, browser = job(text, 'build'), job(text, 'browser-checks')
    for section in (build, browser):
        setup = next(step for step in steps(section) if step.startswith('uses: actions/setup-python@'))
        if "python-version: '3.12'\n          cache: pip\n          cache-dependency-path: .github/workflows/pages.yml" not in setup:
            errors.append('pip cache must follow pinned Python/workflow inputs')
        install = next(step for step in steps(section) if step.startswith('name: Install pinned '))
        if 'if:' in install or 'python -m pip install uv==0.10.8 ziglang==0.16.0' not in install:
            errors.append('pinned installation must always run')
    if 'uv run --with playwright==1.58.0 playwright install --with-deps chromium' not in browser:
        errors.append('browser and OS dependency installation must always run')
    if 'cache-hit' in text or 'continue-on-error' in text:
        errors.append('cache must not bypass installation or checks')
    caches = [step for section in (build, browser) for step in steps(section) if 'uses: actions/cache@' in step]
    if len(caches) != 3 or any('uses: '+CACHE not in step for step in caches):
        errors.append('all three cache actions must use the reviewed immutable revision')
    expected = {
        'Cache pinned regex toolchain archives': [
            'build/regex-toolchain/channel.toml', 'build/regex-toolchain/host.tar.xz',
            'build/regex-toolchain/wasi.tar.xz'],
        'Cache pinned browser Python dependencies': ['~/.cache/uv'],
        'Cache pinned Chromium downloads': ['~/.cache/ms-playwright'],
    }
    for step in caches:
        name = step.splitlines()[0].removeprefix('name: ')
        paths = []
        for line in step.splitlines():
            if line.startswith('          path: ') and line.strip() != 'path: |':
                paths.append(line.split('path: ', 1)[1])
            elif line.startswith('            ') and not line.lstrip().startswith('pages-'):
                paths.append(line.strip())
        if paths != expected.get(name) or 'if:' in step:
            errors.append('cache paths must contain only the approved inputs: '+name)
        if '${{ runner.os }}-${{ runner.arch }}' not in step:
            errors.append('cache must isolate host OS and architecture')
        if name == 'Cache pinned regex toolchain archives':
            if "hashFiles('backends/regex/pin.json')" not in step or 'restore-keys:' in step:
                errors.append('Rust archive key must match checksum pins exactly')
        if name == 'Cache pinned Chromium downloads':
            if 'playwright1.58.0' not in step or 'restore-keys:' in step:
                errors.append('Chromium cache must match Playwright version exactly')
        if name == 'Cache pinned browser Python dependencies':
            prefix = 'pages-uv-${{ runner.os }}-${{ runner.arch }}-py3.12-uv0.10.8-pw1.58.0-shard${{ matrix.shard }}-'
            if prefix+"${{ hashFiles('tests/**/*.py') }}" not in step or '            '+prefix+'\n' not in step:
                errors.append('uv cache must retain per-shard dependency keys and scoped fallback')
    prepare = named(build, 'Prepare checksum-verified private optional regex toolchain')
    if 'if:' in prepare or 'run: python scripts/prepare_regex_toolchain.py --cache build/regex-toolchain' not in prepare:
        errors.append('cached Rust inputs must always be checksum-verified and installed')
    for section, cache_name, consumer_name in (
        (build, 'Cache pinned regex toolchain archives', 'Prepare checksum-verified private optional regex toolchain'),
        (browser, 'Cache pinned browser Python dependencies', 'Install pinned build and browser tools'),
        (browser, 'Cache pinned Chromium downloads', 'Install pinned build and browser tools'),
    ):
        if section.index('name: '+cache_name) > section.index('name: '+consumer_name):
            errors.append('cache restored after its consumer')
    for required in ('shard: [1, 2, 3]', 'fail-fast: false', 'needs: [build, browser-checks]',
                     'contents: read', 'python tests/test_pages_tool_cache.py'):
        if required not in text:
            errors.append('missing existing gate or cache contract: '+required)
    return errors


class CacheTests(unittest.TestCase):
    def test_actual_workflow(self):
        self.assertEqual(validate(WORKFLOW.read_text()), [])

    def test_rejects_broad_or_unpinned_caches(self):
        source = WORKFLOW.read_text()
        for before, after in (
            (CACHE, 'actions/cache@v4'),
            ('build/regex-toolchain/host.tar.xz', 'build/regex-toolchain'),
            ('path: ~/.cache/uv', 'path: ~/.cache'),
            ('playwright1.58.0', 'playwright-latest'),
            ("hashFiles('backends/regex/pin.json')", "hashFiles('README.md')"),
            ('-py3.12-uv0.10.8-pw1.58.0-shard${{ matrix.shard }}-', '-shared-'),
        ):
            with self.subTest(after=after):
                self.assertTrue(validate(source.replace(before, after)))

    def test_cache_hits_never_skip_checks_or_integrity(self):
        source = WORKFLOW.read_text()
        for before, after in (
            ('run: python scripts/prepare_regex_toolchain.py', "if: steps.cache.outputs.cache-hit != 'true'\n        run: python scripts/prepare_regex_toolchain.py"),
            ('python -m pip install uv==0.10.8', 'python -m pip install uv==latest'),
            ('playwright install --with-deps chromium', 'playwright install chromium'),
            ('needs: [build, browser-checks]', 'needs: build'),
        ):
            with self.subTest(after=after):
                self.assertTrue(validate(source.replace(before, after)))

    def test_literal_shell_blocks_parse(self):
        lines = WORKFLOW.read_text().splitlines()
        for i, line in enumerate(lines):
            if line.strip() != 'run: |':
                continue
            indent = len(line)-len(line.lstrip())
            body = []
            for following in lines[i+1:]:
                if following.strip() and len(following)-len(following.lstrip()) <= indent:
                    break
                body.append(following[indent+2:])
            result = subprocess.run(['bash', '-n'], input='\n'.join(body), text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == '__main__':
    unittest.main()
