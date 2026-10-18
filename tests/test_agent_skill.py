#!/usr/bin/env python3
"""Offline consumer tests: installed skill, real metadata, pinned setup and hostile archives."""
import hashlib
import io
import contextlib
import shlex
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

    def invoke(self, arguments, data=None):
        out, err = io.StringIO(), io.StringIO()
        with mock.patch.object(cui, 'resources', return_value=data or self.data), contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            code = cui.main(arguments)
        return code, out.getvalue(), err.getvalue()

    def test_presentation_preserves_json_full_and_read(self):
        for name in ('Select', 'Input'):
            full = cui.component(self.data, name, 'all')
            code, out, _ = self.invoke(['component', name, '--json'])
            self.assertEqual(code, 0)
            self.assertEqual(json.loads(out), full)
            self.assertEqual(self.invoke(['component', name, '--full'])[1],
                             self.invoke(['component', name, '--show', 'all'])[1])
            summary = self.invoke(['component', name])[1]
            self.assertIn(json.dumps(full['status'], indent=2, ensure_ascii=False), summary)
            self.assertNotIn(full['header']['text'], summary)
            canonical = next(c for c in self.data['components'] if c['name'] == name)
            self.assertIn(json.dumps(canonical.get('handler_types', []), ensure_ascii=False), summary)
            for header in [canonical['public_header']] + canonical.get('additional_public_headers', []):
                self.assertIn(header, summary)
            index = self.invoke(['component', name, '--show', 'examples'])[1]
            for example in full['examples']:
                self.assertIn(example['path'], index)
                self.assertNotIn(example['text'], index)
            for section in ('usage', 'api', 'limitations', 'examples'):
                self.assertEqual(json.loads(self.invoke(['component', name, '--show', section, '--json'])[1]),
                                 cui.component(self.data, name, section))
        path = 'docs/START_HERE.md'
        self.assertEqual(self.invoke(['read', path])[1], self.data['documents'][path] + '\n')
        self.assertEqual(json.loads(self.invoke(['read', path, '--json'])[1]), self.data['documents'][path])
        symbol = 'cui_begin_frame'
        matches = [s for s in self.data['symbols'] if s['name'] == symbol]
        headers = {p: text for p, text in self.data['documents'].items()
                   if p.startswith('include/') and re.search(r'\b' + symbol + r'\b', text)}
        expected = ''.join(m['declaration'] + '\n' for m in matches)
        expected += ''.join('\n# ' + p + '\n\n' + text + '\n' for p, text in headers.items())
        self.assertEqual(self.invoke(['api', symbol, '--full'])[1], expected)
        self.assertEqual(json.loads(self.invoke(['api', symbol, '--json'])[1]),
                         {'functions': matches, 'matching_headers': headers})
        concise = self.invoke(['api', symbol])[1]
        self.assertIn('incomplete contract', concise)
        self.assertIn(self.data['source_digest'], concise)
        for c in self.data['components']:
            api_index = self.invoke(['component', c['name'], '--show', 'api'])[1]
            for match in cui.component(self.data, c['name'], 'api')['api']:
                self.assertIn(match['header'], api_index)
        self.assertIn(matches[0]['declaration'], concise)
        self.assertLess(len(concise), len(expected))

    def test_search_defaults_ranking_and_flag_errors(self):
        expected = cui.search(self.data, 'cui', limit=10)
        self.assertEqual(json.loads(self.invoke(['search', 'cui', '--json'])[1]), expected)
        human = self.invoke(['search', 'cui'])[1]
        self.assertIn('Showing 5 of ', human)
        for match in expected[:5]:
            self.assertIn(match['name'], human)
        self.assertIn('Showing 2 of ', self.invoke(['search', 'cui', '--limit', '2'])[1])
        for args in (['component', 'Input', '--full', '--show', 'usage'],
                     ['component', 'Input', '--show', 'summary', '--json'],
                     ['api', 'cui_begin_frame', '--full', '--declaration'],
                     ['api', 'cui_begin_frame', '--declaration', '--json'],
                     ['read', 'docs/START_HERE.md', '--headings', '--json'],
                     ['read', 'docs/START_HERE.md', '--section', 'x', '--json'],
                     ['search', 'cui', '--limit', '0']):
            code, _, error = self.invoke(args)
            self.assertEqual(code, 2, args)
            self.assertTrue(error, args)

    def test_exact_markdown_sections_fences_duplicates_unicode(self):
        body = '# Guide\nintro\n## Résumé *exact* ##\nkeep\n### Child\nchild\n```md\n## Fake\n```\n~~~\n# Also fake\n~~~\n    ## Indented\n## Next\nnext\n## Duplicate\na\n## Duplicate\nb\n'
        headings = cui.markdown_headings(body)
        self.assertEqual([h['title'] for h in headings],
                         ['Guide', 'Résumé *exact*', 'Child', 'Next', 'Duplicate', 'Duplicate'])
        data = {'documents': {'docs/é.md': body}, 'source_digest': 'fixture'}
        code, out, _ = self.invoke(['read', 'docs/é.md', '--section', 'Résumé *exact*'], data)
        self.assertEqual(code, 0)
        self.assertIn('Partial Markdown', out)
        self.assertIn('omitted before: 2', out)
        self.assertTrue(out.endswith(''.join(body.splitlines(keepends=True)[2:13])))
        self.assertNotIn('next\n', out)
        for title in ('Duplicate', 'Missing', 'résumé *exact*', 'Fake', ''):
            self.assertEqual(self.invoke(['read', 'docs/é.md', '--section', title], data)[0], 2)
        self.assertIn('lines 3–13', self.invoke(['read', 'docs/é.md', '--headings'], data)[1])
        source = Path("/tmp/source with spaces/é's checkout")
        self.assertEqual(shlex.split(cui.read_command('docs/é.md', source))[-2:], ['--source', str(source)])
        # Types/macros use resource paths, never fabricated declarations.
        data = {'symbols': [], 'components': [], 'documents': {'include/cui/a.h': '#define CUI_TEST 1'}, 'source_digest': 'fixture'}
        code, out, _ = self.invoke(['api', 'CUI_TEST'], data)
        self.assertEqual(code, 0)
        self.assertIn('No indexed function declaration', out)
        self.assertNotIn('#define', out)

    def test_markdown_fence_failure_and_exact_boundaries(self):
        for body in ('# Heading\n```\n## hidden\n', '# Heading\n````\n```\n',
                     '# Heading\n~~~\n```\n', '# Heading\n```\n``` trailing\n'):
            data = {'documents': {'docs/a.md': body}, 'source_digest': 'fixture'}
            for selector in (['--headings'], ['--section', 'Heading']):
                code, out, error = self.invoke(['read', 'docs/a.md'] + selector, data)
                self.assertEqual(code, 2)
                self.assertEqual(out, '')
                self.assertIn('Unterminated', error)
            self.assertEqual(self.invoke(['read', 'docs/a.md'], data)[1], body + '\n')
        body = '# Heading\r\n```` lang\r\n## hidden\r\n`````\r\n## Last\r\nno final newline'
        data = {'documents': {'docs/a.md': body}, 'source_digest': 'fixture'}
        self.assertEqual([h['title'] for h in cui.markdown_headings(body)], ['Heading', 'Last'])
        code, out, _ = self.invoke(['read', 'docs/a.md', '--section', 'Last'], data)
        self.assertEqual(code, 0)
        self.assertTrue(out.endswith('## Last\r\nno final newline'))
        empty = {'documents': {'docs/a.md': ''}, 'source_digest': 'fixture'}
        self.assertEqual(self.invoke(['read', 'docs/a.md', '--headings'], empty)[0], 0)
        self.assertEqual(self.invoke(['read', 'docs/a.md', '--section', 'Missing'], empty)[0], 2)

    def test_live_packaged_and_installed_presentation(self):
        # Live resources and packaged native-bundle data take different loading paths.
        command = [sys.executable, str(SCRIPTS / 'cui.py')]
        live = subprocess.run(command + ['component', 'Select', '--source', str(ROOT)], capture_output=True, text=True)
        self.assertEqual(live.returncode, 0, live.stderr)
        live_data = agent_resources.collect(ROOT)
        self.assertIn(json.dumps(cui.component(live_data, 'Select', 'limitations')['status'], indent=2, ensure_ascii=False), live.stdout)
        with tempfile.TemporaryDirectory(prefix="cui skill é '") as directory:
            target = Path(directory)
            installed = target / 'installed'
            shutil.copytree(ROOT / 'skills/cui', installed)
            (target / '.cui-resources.json').write_text(json.dumps(self.data), encoding='utf-8')
            installed_command = [sys.executable, str(installed / 'scripts/cui.py')]
            for args in (['component', 'Select'], ['component', 'Input', '--show', 'examples'],
                         ['api', 'cui_begin_frame'], ['read', 'docs/START_HERE.md', '--headings']):
                result = subprocess.run(installed_command + args + ['--source', str(target)], cwd=target, capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue(result.stdout)
            result = subprocess.run(installed_command + ['api', 'cui_begin_frame', '--source', str(target)], capture_output=True, text=True)
            read_line = next(line.strip() for line in result.stdout.splitlines() if ' read ' in line)
            followup = subprocess.run(shlex.split(read_line), cwd=target, capture_output=True, text=True)
            self.assertEqual(followup.returncode, 0, followup.stderr)
            self.assertIn('cui_begin_frame', followup.stdout)

    def test_all_components_have_real_declarations_and_limits(self):
        self.assertEqual(len(self.data['components']), 78)
        for c in self.data['components']:
            result = cui.component(self.data, c['name'], 'all')
            self.assertEqual(len(result['api']), len(c['api_symbols']))
            self.assertEqual(result['header']['text'], (ROOT / c['public_header']).read_text(encoding='utf-8'))
            self.assertIn('limitations', result['status'])
            self.assertIn(json.dumps(result['status'], indent=2, ensure_ascii=False),
                          self.invoke(['component', c['name']])[1])
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
