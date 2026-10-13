#!/usr/bin/env python3
"""Verify every crawler-visible share page, image, canonical, scope and sitemap."""
from html.parser import HTMLParser
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from urllib.parse import urlsplit
from xml.etree import ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
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
        output = ROOT / 'build/web'
        routes = json.loads((output / 'routes.json').read_text())
        documents = json.loads((output / 'docs.json').read_text())
        self.assertEqual(len(routes), 80+len(documents))
        self.assertEqual(len({r['url'] for r in routes}), len(routes))
        self.assertEqual(sum(r['path'].startswith('components/') for r in routes), 78)
        for route in routes:
            markup = (output / route['path'] / 'index.html').read_text()
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
            if route['path'].startswith('components/'):
                self.assertIn('coverage remains in progress', markup)
        image = (output / 'assets/cui-social.png').read_bytes()
        self.assertEqual(image[:8], b'\x89PNG\r\n\x1a\n')
        sitemap = ET.parse(output / 'sitemap.xml')
        urls = [node.text for node in sitemap.findall('.//{*}loc')]
        self.assertEqual(set(urls), {r['url'] for r in routes})
        self.assertEqual(len(urls), len(routes))

    def test_rebuild_is_deterministic(self):
        output = ROOT / 'build/web'
        before = (output / 'components/checkbox/index.html').read_bytes()
        routes = SHARING.build(output)
        self.assertEqual(before, (output / 'components/checkbox/index.html').read_bytes())
        self.assertEqual(len(routes), len(json.loads((output / 'routes.json').read_text())))

    def test_invalid_origins_are_rejected_before_writes(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory)
            for site in ['http://example.com/', '/cui/', 'https://example.com/cui', 'https://example.com/?q=x', 'https://example.com/#x']:
                with self.assertRaises(ValueError):
                    SHARING.build(output, site)
            self.assertEqual(list(output.iterdir()), [])


if __name__ == '__main__':
    unittest.main()
