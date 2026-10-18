#!/usr/bin/env python3
"""Verify every crawler-visible share page, image, canonical, scope and sitemap."""
from html.parser import HTMLParser
import importlib.util
import json
import os
import shutil
from pathlib import Path
import tempfile
import unittest
from urllib.parse import urlsplit
from xml.etree import ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = Path(os.environ.get('CUI_WEB_TEST_DIRECTORY', str(ROOT/'build/web')))
SPEC = importlib.util.spec_from_file_location('sharing', ROOT / 'scripts/build_sharing.py')
SHARING = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(SHARING)


class Head(HTMLParser):
    def __init__(self):
        super().__init__()
        self.metadata = {}
        self.canonical = []
        self.fallback = False
    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if tag == 'meta':
            key = attrs.get('property', attrs.get('name'))
            if key:
                if key in self.metadata:
                    raise AssertionError('Duplicate metadata '+key)
                self.metadata[key] = attrs.get('content')
        if tag == 'link' and attrs.get('rel') == 'canonical':
            self.canonical.append(attrs['href'])
        if tag == 'noscript':
            self.fallback = True


class SharingTests(unittest.TestCase):
    def test_complete_pages_and_metadata(self):
        output = OUTPUT
        routes = json.loads((output / 'routes.json').read_text())
        documents = json.loads((output / 'docs.json').read_text())
        extensions=json.loads((output/'catalog.json').read_text()).get('extensions',[])
        self.assertEqual(len(routes), 80+len(documents)+len(extensions))
        self.assertEqual(len({r['url'] for r in routes}), len(routes))
        self.assertEqual(sum(r['path'].startswith('components/') for r in routes), 78+len(extensions))
        for route in routes:
            markup = (output / route['path'] / 'index.html').read_text()
            self.assertNotIn('__CUI_VERSION__', markup)
            head = Head()
            head.feed(markup)
            self.assertEqual(head.canonical, [route['url']])
            for key in ['og:title','og:type','og:image','og:url','og:description','og:image:alt','twitter:card']:
                self.assertTrue(head.metadata[key], (route['path'], key))
            self.assertEqual(head.metadata['og:title'], route['title'])
            self.assertEqual(head.metadata['og:url'], route['url'])
            self.assertEqual(head.metadata['og:image:width'], '1200')
            self.assertEqual(head.metadata['og:image:height'], '630')
            self.assertEqual(head.metadata['twitter:card'], 'summary_large_image')
            self.assertTrue(urlsplit(head.metadata['og:image']).scheme == 'https')
            self.assertTrue(head.fallback)
            if route['path']:
                self.assertIn('<base href="../../">', markup)
            if route['path'].startswith('docs/'):
                self.assertIn('rel="alternate" type="text/markdown"', markup)
            if route['path'].startswith('components/'):
                self.assertIn('coverage remains in progress', markup)
        image = (output / 'assets/cui-social.png').read_bytes()
        self.assertEqual(image[:8], b'\x89PNG\r\n\x1a\n')
        sitemap = ET.parse(output / 'sitemap.xml')
        urls = [node.text for node in sitemap.findall('.//{*}loc')]
        self.assertEqual(set(urls), {r['url'] for r in routes})
        self.assertEqual(len(urls), len(routes))

    def test_rebuild_is_deterministic(self):
        output = OUTPUT
        before = (output / 'components/checkbox/index.html').read_bytes()
        routes = SHARING.build(output)
        self.assertEqual(before, (output / 'components/checkbox/index.html').read_bytes())
        self.assertEqual(len(routes), len(json.loads((output / 'routes.json').read_text())))

    def test_literal_regex_backslashes_survive_metadata(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            (output / 'assets').mkdir()
            shutil.copyfile(OUTPUT / 'catalog.json', output / 'catalog.json')
            shutil.copyfile(OUTPUT / 'index.html', output / 'index.html')
            shutil.copyfile(ROOT / 'web/assets/cui-social.png', output / 'assets/cui-social.png')
            title = r'Regex \p{L} <docs>'
            description = r'Match \p{L} and \1 literally & safely.'
            documents = [{'slug': 'regex-probe', 'title': title,
                          'html': r'<p>Match \p{L} and \1 literally &amp; safely.</p>'}]
            (output / 'docs.json').write_text(json.dumps(documents))
            SHARING.build(output)
            head = Head()
            head.feed((output / 'docs/regex-probe/index.html').read_text())
            self.assertEqual(head.metadata['og:title'], title + ' · CUI')
            self.assertEqual(head.metadata['og:description'], description)
            self.assertEqual(head.metadata['description'], description)

    def test_extension_routes_and_version_and_rejection(self):
        with tempfile.TemporaryDirectory() as directory:
            output=Path(directory)
            (output/'assets').mkdir()
            shutil.copyfile(ROOT/'web/assets/cui-social.png',output/'assets/cui-social.png')
            template=(ROOT/'web/index.html').read_text().replace('__CUI_VERSION__','fixture-version')
            (output/'index.html').write_text(template)
            (output/'docs.json').write_text('[]')
            baseline=json.loads((ROOT/'docs/gpui-parity.json').read_text())
            baseline['extensions']=[{'name':'Video','limitations':['Silent extension outside baseline.']}]
            (output/'catalog.json').write_text(json.dumps(baseline))
            SHARING.build(output)
            for path in ['components/video/index.html','components/button/index.html','index.html']:
                markup=(output/path).read_text()
                self.assertIn('fixture-version',markup)
                self.assertNotIn('__CUI_VERSION__',markup)
                head=Head();head.feed(markup)
                self.assertEqual(len(head.canonical),1)
            before=(output/'index.html').read_text()
            SHARING.build(output)
            self.assertEqual(before,(output/'index.html').read_text())
            self.assertIn('components/video/',(output/'sitemap.xml').read_text())
            for extensions in [[{'name':'Button'}],[{'name':'Video'},{'name':'video'}],[{'name':''}],{}]:
                baseline['extensions']=extensions
                (output/'catalog.json').write_text(json.dumps(baseline))
                with self.assertRaises(ValueError): SHARING.build(output)
            baseline['components'].pop()
            baseline['extensions']=[]
            (output/'catalog.json').write_text(json.dumps(baseline))
            with self.assertRaises(ValueError): SHARING.build(output)

    def test_invalid_origins_are_rejected_before_writes(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            for site in ['http://example.com/', '/cui/', 'https://example.com/cui', 'https://example.com/?q=x', 'https://example.com/#x']:
                with self.assertRaises(ValueError):
                    SHARING.build(output, site)
            self.assertEqual(list(output.iterdir()), [])


if __name__ == '__main__':
    unittest.main()
