#!/usr/bin/env python3
"""Offline consumer tests: installed skill, real metadata, pinned setup and hostile archives."""
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
SCRIPTS = ROOT / 'skills/cui/scripts'
sys.path.insert(0, str(SCRIPTS))
import cui
import agent_resources


class AgentSkillTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = cui.resources()

    def test_all_components_have_real_declarations_and_limits(self):
        self.assertEqual(len(self.data['components']), 78)
        for c in self.data['components']:
            result = cui.component(self.data, c['name'], 'all')
            self.assertEqual(len(result['api']), len(c['api_symbols']))
            self.assertEqual(result['header']['text'], (ROOT / c['public_header']).read_text(encoding='utf-8'))
            self.assertIn('limitations', result['status'])
            self.assertTrue(result['examples'])

    def test_search_exact_alias_multiterm_literal_and_partial(self):
        self.assertEqual(cui.search(self.data, 'Select', 'component')[0]['name'], 'Select')
        self.assertEqual(cui.search(self.data, 'searchable dropdown', 'component')[0]['name'], 'Select')
        self.assertEqual(cui.search(self.data, 'cui_begin_frame', 'api')[0]['name'], 'cui_begin_frame')
        self.assertTrue(cui.search(self.data, 'font fallback', 'guide'))
        partial = cui.search(self.data, 'dropdown zzzzzzzzzzz', 'component')
        self.assertTrue(partial)
        self.assertTrue(all(x['partial'] for x in partial))
        self.assertEqual(cui.search(self.data, '[.*]'), [])
        self.assertEqual(cui.search(self.data, 'zzzzzzzzzz'), [])
        self.assertEqual(cui.search(self.data, 'FONT fallback'), cui.search(self.data, 'font FALLBACK'))

    def test_skill_routes_and_template_entrypoints_exist(self):
        skill = (ROOT / 'skills/cui/SKILL.md').read_text()
        for path in re.findall(r'`((?:docs|examples)/[^`*]+\.(?:md|json))`', skill):
            self.assertIn(path, self.data['documents'])
        for name, (entry, _) in cui.TEMPLATES.items():
            self.assertTrue((ROOT / ('examples/native_bundle/' + entry if name == 'native' else entry)).is_file(), name)

    def test_installed_copy_works_without_checkout_or_site_dependencies(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            installed = root / 'installed/cui'
            shutil.copytree(ROOT / 'skills/cui', installed)
            command = [sys.executable, str(installed / 'scripts/cui.py')]
            for arguments in (['list', '--json'], ['search', 'searchable dropdown', '--json'],
                              ['component', 'Input', '--show', 'usage', '--json'],
                              ['api', 'cui_begin_frame'], ['read', 'docs/TYPOGRAPHY.md']):
                result = subprocess.run(command + arguments, cwd=root, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue(result.stdout)
            result = subprocess.run(command + ['api', 'cui_not_a_real_api'], cwd=root, capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)

    def test_doctor_reports_missing_tools_without_installing(self):
        with mock.patch.object(cui.shutil, 'which', return_value=None), mock.patch.object(cui.subprocess, 'run') as run:
            result = cui.doctor('native')
            self.assertTrue(all(not x['ok'] for x in result['checks']))
            run.assert_not_called()
        with mock.patch.object(cui.shutil, 'which', return_value='/tool'):
            self.assertTrue(all(x['ok'] for x in cui.doctor('portable')['checks']))
        with mock.patch.object(cui.shutil, 'which', return_value=None):
            names = [x['dependency'] for x in cui.doctor('sokol')['checks']]
            self.assertIn('gl', names)
            self.assertNotIn('sdl3 >= 3.2', names)

    def test_resource_order_is_case_sensitive_and_platform_independent(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            headers = root / 'include/cui'
            headers.mkdir(parents=True)
            (headers / 'Zebra.h').write_text('bool cui_zebra(void);\n')
            (headers / 'alpha.h').write_text('bool cui_alpha(void);\n')
            docs = root / 'docs'
            docs.mkdir()
            (docs / 'Zebra.md').write_text('# Zebra\n')
            (docs / 'alpha.md').write_text('# Alpha\n')
            names = [p.relative_to(root).as_posix() for p in agent_resources.source_files(root)]
            self.assertEqual(names, sorted(names))
            self.assertEqual([entry['name'] for entry in agent_resources.public_api(root)],
                             ['cui_zebra', 'cui_alpha'])

    def test_fingerprint_tracks_header_and_doc_changes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for path, text in self.data['documents'].items():
                target = root / path
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(text, encoding='utf-8')
            original = agent_resources.collect(root)
            self.assertEqual(original['source_digest'], self.data['source_digest'])
            header = root / 'include/cui/cui.h'
            header.write_text(header.read_text() + '\n/* new contract */\n')
            changed = agent_resources.collect(root)
            self.assertNotEqual(original['source_digest'], changed['source_digest'])
            self.assertIn('new contract', changed['documents']['include/cui/cui.h'])

    def archive(self, name, kind=tarfile.REGTYPE, link=''):
        stream = io.BytesIO()
        with tarfile.open(fileobj=stream, mode='w') as archive:
            member = tarfile.TarInfo(name)
            member.type = kind
            member.linkname = link
            member.size = 1 if kind == tarfile.REGTYPE else 0
            archive.addfile(member, io.BytesIO(b'x') if member.size else None)
        return stream.getvalue()

    def test_archive_rejects_traversal_links_and_special_files_before_writing(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name, kind in [('repo/../../escape', tarfile.REGTYPE), ('/escape', tarfile.REGTYPE),
                               ('repo/link', tarfile.SYMTYPE), ('repo/link', tarfile.LNKTYPE),
                               ('repo/fifo', tarfile.FIFOTYPE), ('repo/C:\\escape', tarfile.REGTYPE)]:
                with self.subTest(name=name, kind=kind), self.assertRaises(ValueError):
                    cui.unpack(self.archive(name, kind, '/tmp'), root)
                self.assertEqual(list(root.iterdir()), [])

    def test_pinned_vendor_and_native_init_from_offline_source(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            # A real repository archive exercises offline use and excludes uncommitted edits.
            installed = root / 'installed-skill'
            shutil.copytree(ROOT / 'skills/cui', installed)
            helper = installed / 'scripts/cui.py'
            def invoke(*args):
                return subprocess.run([sys.executable, str(helper), *map(str, args)], cwd=root, capture_output=True, text=True)
            dest = root / 'vendor'
            result = invoke('vendor', '--source', ROOT, '--dest', dest)
            self.assertEqual(result.returncode, 0, result.stderr)
            lock = json.loads((dest / '.cui-lock.json').read_text())
            expected = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
            self.assertEqual(lock['revision'], expected)
            self.assertEqual(invoke('verify', '--source', dest).returncode, 0)
            self.assertEqual(invoke('vendor', '--source', ROOT, '--dest', dest).returncode, 2)
            (dest / 'include/cui/cui.h').write_text('changed')
            self.assertEqual(invoke('verify', '--source', dest).returncode, 1)
            for template in ('native', 'web', 'python', 'node', 'typescript', 'go', 'rust', 'embedding', 'portable', 'sdl3', 'sdl-gpu', 'sokol', 'software'):
                target = root / template
                result = invoke('init', '--template', template, '--source', ROOT, '--dest', target)
                self.assertEqual(result.returncode, 0, template + ': ' + result.stderr)
                self.assertTrue((target / cui.TEMPLATES[template][0]).is_file(), template)
                self.assertTrue((target / '.cui-lock.json').is_file())
                lookup = invoke('component', 'Select', '--show', 'usage', '--source', target, '--json')
                self.assertEqual(lookup.returncode, 0, lookup.stderr)
                shutil.rmtree(target)  # Only this test's newly created TemporaryDirectory children.

    def test_resolve_ref_returns_only_valid_immutable_sha(self):
        response = mock.MagicMock()
        response.__enter__.return_value = io.StringIO(json.dumps({'sha': 'a' * 40}))
        with mock.patch.object(cui.urllib.request, 'urlopen', return_value=response) as request:
            self.assertEqual(cui.resolve('release/test'), 'a' * 40)
            self.assertIn('release%2Ftest', request.call_args.args[0].full_url)
        response.__enter__.return_value = io.StringIO(json.dumps({'sha': 'main'}))
        with mock.patch.object(cui.urllib.request, 'urlopen', return_value=response), self.assertRaises(ValueError):
            cui.resolve('main')

    def test_download_requires_immutable_revision_and_checksum_mismatch_fails(self):
        with self.assertRaises(ValueError):
            cui.snapshot(None, 'main')
        with tempfile.TemporaryDirectory() as directory:
            archive = Path(directory) / 'source.tar'
            archive.write_bytes(self.archive('repo/file'))
            with self.assertRaises(ValueError):
                cui.snapshot(None, 'a' * 40, archive, '0' * 64)
            with self.assertRaises(ValueError):
                cui.snapshot(None, 'a' * 40, archive)
            blob, lock = cui.snapshot(None, 'a' * 40, archive, hashlib.sha256(archive.read_bytes()).hexdigest())
            self.assertEqual(lock['archive_sha256'], hashlib.sha256(blob).hexdigest())


if __name__ == '__main__':
    unittest.main()
