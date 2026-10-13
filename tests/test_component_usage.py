#!/usr/bin/env python3
"""Headless contract checks for per-component website documentation."""
import json
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def repository_key(path, root=ROOT):
    return path.relative_to(root).as_posix()


class ComponentUsageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest = json.loads((ROOT / 'docs/gpui-parity.json').read_text(encoding='utf-8'))['components']
        cls.data = json.loads((ROOT / 'docs/component-usage.json').read_text(encoding='utf-8'))
        cls.entries = cls.data['components']
        cls.headers = {repository_key(p): p.read_text(encoding='utf-8') for p in (ROOT / 'include/cui').glob('*.h')}
        cls.all_headers = '\n'.join(cls.headers.values())

    def test_repository_keys_are_portable(self):
        for kind, root in [(pathlib.PureWindowsPath, 'C:/work/cui'),
                           (pathlib.PurePosixPath, '/work/cui')]:
            base = kind(root)
            self.assertEqual(repository_key(base / 'include/cui/cui.h', base),
                             'include/cui/cui.h')

    def test_exact_manifest_coverage_and_headers(self):
        self.assertEqual(len(self.entries), 78)
        self.assertEqual([x['name'] for x in self.entries], [x['name'] for x in self.manifest])
        for entry, manifest in zip(self.entries, self.manifest):
            self.assertEqual(entry['public_header'], manifest['public_header'])
            self.assertIn(entry['public_header'], self.headers)

    def test_public_api_types_callbacks_and_references(self):
        for entry in self.entries:
            with self.subTest(component=entry['name']):
                header = self.headers[entry['public_header']]
                self.assertTrue(entry['api_symbols'])
                for api in entry['api_symbols']:
                    self.assertRegex(header, r'\b' + re.escape(api) + r'\s*\(')
                for typename in entry['state_types']:
                    # A real typedef or named struct, not a guessed component name.
                    self.assertRegex(self.all_headers, r'(?:}\s*|\bstruct\s+)' + re.escape(typename) + r'\b')
                for callback in entry['callback_symbols']:
                    self.assertRegex(header, r'\b' + re.escape(callback) + r'\b')
                    if callback.startswith('cui_'):
                        self.assertRegex(header, r'\(\*' + re.escape(callback) + r'\)')
                    else:
                        self.assertRegex(header, r'\(\*' + re.escape(callback) + r'\)')
                for doc in entry['docs']:
                    self.assertTrue((ROOT / doc).is_file(), doc)
                # Every C identifier cited in prose must exist in a public header.
                for symbol in re.findall(r'\bcui_[a-z0-9_]+\b', ' '.join(entry[k] for k in ['state_ownership','interaction','usage'])):
                    self.assertRegex(self.all_headers, r'\b' + re.escape(symbol) + r'\b')
                for field in ['state_ownership', 'interaction', 'usage', 'scope']:
                    self.assertGreater(len(entry[field]), 20)

    def test_immediate_engine_and_model_only_boundaries(self):
        for entry in self.entries:
            self.assertIn(entry['composition'], ['immediate frame submission', 'application model + frame composition'])
        index = {x['name']: x for x in self.entries}
        for name in ['Dock', 'VirtualList', 'Editor', 'Plot']:
            self.assertTrue(index[name]['scope'].startswith('styled frame adapter'))
            self.assertEqual(index[name]['composition'], 'application model + frame composition')
        self.assertIn('host adapter', index['Clipboard']['scope'])
        self.assertIn('not an application widget tree', self.data['lifecycle'])

    def test_guide_covers_metadata_and_lifetimes(self):
        guide = (ROOT / 'docs/COMPONENT_USAGE.md').read_text(encoding='utf-8')
        for entry in self.entries:
            self.assertIn('\n## ' + entry['name'] + '\n', guide)
            for field in ['state_ownership','interaction','usage']:
                self.assertIn(entry[field], guide)
        for phrase in ['synchronously', 'cui_end_frame', 'copying', 'paint-only', 'save_document(app)', 'persist_preferences(app)']:
            self.assertIn(phrase, guide)


if __name__ == '__main__':
    unittest.main()
