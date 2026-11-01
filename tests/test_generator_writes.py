#!/usr/bin/env python3
"""Offline generator entrypoint tests; no SDK build or canonical regeneration."""
import contextlib
import importlib.util
import io
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))


def load(name):
    spec = importlib.util.spec_from_file_location(name, ROOT / 'scripts' / (name + '.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class GeneratorWrites(unittest.TestCase):
    def invoke(self, module, check=False):
        with patch.object(sys, 'argv', ['generator'] + (['--check'] if check else [])), \
                contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            try:
                return module.main() or 0
            except SystemExit as error:
                return error.code

    def exercise(self, module, root, representatives):
        self.assertEqual(self.invoke(module), 0)
        files = [root / name for name in representatives]
        expected = {p: p.read_bytes() for p in files}
        # Fixed old timestamps make this deterministic even on coarse filesystems.
        for p in files:
            os.utime(p, ns=(1_000_000_000, 1_000_000_000))
        before = {p: p.stat().st_mtime_ns for p in files}
        self.assertEqual(self.invoke(module), 0)
        self.assertEqual({p: p.stat().st_mtime_ns for p in files}, before)
        self.assertEqual(self.invoke(module, True), 0)
        self.assertEqual({p: p.stat().st_mtime_ns for p in files}, before)
        files[0].write_bytes(b'broken\r\n')
        files[1].unlink()
        self.assertNotEqual(self.invoke(module, True), 0)
        self.assertEqual(files[0].read_bytes(), b'broken\r\n')
        self.assertFalse(files[1].exists())
        self.assertEqual(self.invoke(module), 0)
        for p, data in expected.items():
            self.assertEqual(p.read_bytes(), data)
        self.assertEqual(self.invoke(module, True), 0)

    def test_sdk_outputs_and_stale_refusal(self):
        m = load('generate_go_sdk')
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'bindings/go/cui').mkdir(parents=True)
            m.ROOT = root
            m.SDKS = [root / 'bindings/go/sdk/source', root / 'bindings/rust/sdk/source']
            snapshot = {'CUI_SOURCES.cmake': b'# fixture\nset(CUI_SOURCES\n src/a.c\n)\n',
                        'src/a.c': b'/* caf\xc3\xa9 */\n', 'SOURCE_FILES.sha256': b'fixture\n'}
            with patch.object(m, 'snapshot', return_value=snapshot), \
                    patch.object(m.subprocess, 'check_output', side_effect=lambda argv, **kw:
                                 'fixture-revision\n' if kw.get('text') else b''):
                self.exercise(m, root, ['bindings/go/cui/z_cui_src_a.c',
                    'bindings/go/sdk/source/src/a.c', 'bindings/go/sdk/SOURCE_PROVENANCE.json',
                    'bindings/rust/sdk/source/SOURCE_FILES.sha256', 'bindings/rust/sdk/.gitattributes'])
                snapshot['src/a.c'] = b'/* changed */\n'
                self.assertNotEqual(self.invoke(m, True), 0)
                self.assertEqual(self.invoke(m), 0)
                self.assertEqual((m.SDKS[1] / 'src/a.c').read_bytes(), snapshot['src/a.c'])
                extra = m.SDKS[0] / 'obsolete.c'
                extra.write_bytes(b'preserve')
                self.assertNotEqual(self.invoke(m), 0)
                self.assertEqual(extra.read_bytes(), b'preserve')
                self.assertNotEqual(self.invoke(m, True), 0)
                extra.unlink()
                wrapper = root / 'bindings/go/cui/z_cui_obsolete.c'
                wrapper.write_bytes(b'preserve')
                self.assertNotEqual(self.invoke(m), 0)
                self.assertEqual(wrapper.read_bytes(), b'preserve')

    def test_amalgamation_outputs(self):
        m = load('amalgamate')
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            headers = root / 'include/cui'
            headers.mkdir(parents=True)
            for name in ('cui_fixture', 'cui_renderer', 'cui_platform'):
                (headers / (name + '.h')).write_text('#ifndef FIXTURE\n#define FIXTURE\n/* café */\n#endif\n', encoding='utf-8')
            m.ROOT, m.MODULES = root, {'cui_fixture': []}
            self.exercise(m, root, ['dist/cui_fixture.h', 'dist/cui_renderer.h',
                                   'dist/cui_all.h', 'dist/implementations.json'])
            (headers / 'cui_fixture.h').write_text('#ifndef FIXTURE\n#define FIXTURE\n/* changed */\n#endif\n')
            self.assertNotEqual(self.invoke(m, True), 0)
            self.assertEqual(self.invoke(m), 0)
            self.assertIn(b'changed', (root / 'dist/cui_fixture.h').read_bytes())
            extra = root / 'dist/unmanaged.h'
            extra.write_bytes(b'preserve')
            self.assertEqual(self.invoke(m), 0)
            self.assertEqual(extra.read_bytes(), b'preserve')

    def test_skill_outputs(self):
        m = load('generate_agent_skill')
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'scripts').mkdir()
            (root / 'scripts/agent_resources.py').write_bytes(b'# caf\xc3\xa9\n')
            m.ROOT = root
            with patch.object(m, 'collect', return_value={'fixture': 'café'}):
                self.exercise(m, root, ['skills/cui/data/resources.json',
                                       'skills/cui/scripts/agent_resources.py'])
                with patch.object(m, 'collect', return_value={'fixture': 'changed'}):
                    self.assertNotEqual(self.invoke(m, True), 0)
                    self.assertEqual(self.invoke(m), 0)
                    self.assertIn(b'changed', (root / 'skills/cui/data/resources.json').read_bytes())


if __name__ == '__main__':
    unittest.main()
