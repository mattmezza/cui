#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["markdown-it-py==4.0.0", "mdurl==0.1.2"]
# ///
"""Exercise safe Markdown, links, assets, and complete repository doc discovery."""
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('build_docs', ROOT / 'scripts/build_docs.py')
DOCS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DOCS)


class DocumentationTests(unittest.TestCase):
    def test_website_wording_preserves_heading_links(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'README.md').write_text('# GPUI comparison\n\n[Jump](#gpui-comparison)\n\nCUI usage.\n')
            pages = DOCS.build(root, root / 'output')
            self.assertEqual(pages[0]['title'], 'upstream comparison')
            self.assertIn('id="doc-gpui-comparison"', pages[0]['html'])
            self.assertIn('href="#docs/getting-started/gpui-comparison"', pages[0]['html'])
            self.assertNotIn('>GPUI', pages[0]['html'])

    def test_website_language_keeps_source_urls(self):
        text = DOCS.website_text('GPUI Kit parity; GPUI. https://gpui-kit.com/base/')
        self.assertEqual(text, 'upstream kit parity; upstream. https://gpui-kit.com/base/')
        self.assertEqual(DOCS.website_text('[Manifest](gpui-parity.json)'), '[Manifest](gpui-parity.json)')
        self.assertEqual(DOCS.website_text('cui_begin_frame(ui, width, height, dt);'),
                         'cui_begin_frame(ui, width, height, dt);')

    def test_render_links_and_assets(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'repo'
            output = Path(directory) / 'site'
            (root / 'docs/screenshots').mkdir(parents=True)
            (root / 'README.md').write_text('# Start\n\n[Guide](docs/API.md#ownership)\n')
            (root / 'docs/screenshots/test.png').write_bytes(b'PNG fixture')
            (root / 'docs/API.md').write_text('''# API

## Ownership

## Ownership

[Start](../README.md) [Here](#ownership) [Code](../include/cui/cui.h#L12)

![Preview](screenshots/test.png)

| Name | Meaning |
| --- | --- |
| CUI | C UI |

```c
if (x < 2) puts("<script>");
```

<script>alert(1)</script>

[Unsafe](javascript:alert(1))
''')
            pages = DOCS.build(root, output)
            self.assertEqual({p['slug'] for p in pages}, {'getting-started', 'api'})
            start, api = pages
            self.assertIn('href="#docs/api/ownership"', start['html'])
            self.assertIn('href="#docs/getting-started"', api['html'])
            self.assertIn('href="#docs/api/ownership"', api['html'])
            self.assertIn('id="doc-ownership"', api['html'])
            self.assertIn('id="doc-ownership-1"', api['html'])
            self.assertIn('https://github.com/mattmezza/cui/blob/main/include/cui/cui.h#L12', api['html'])
            self.assertIn('<table>', api['html'])
            self.assertIn('class="language-c"', api['html'])
            self.assertIn('&lt;script&gt;', api['html'])
            self.assertNotIn('<script>', api['html'])
            self.assertNotIn('href="javascript:', api['html'])
            self.assertEqual((output / 'docs-assets/docs/screenshots/test.png').read_bytes(), b'PNG fixture')
            self.assertEqual(json.loads((output / 'docs.json').read_text()), pages)
            index = (output / 'llms.txt').read_text()
            full = (output / 'llms-full.txt').read_text()
            self.assertTrue(index.startswith('# CUI\n'))
            self.assertIn('./markdown/README.md', index)
            self.assertIn('./markdown/docs/API.md', index)
            self.assertEqual(index, (output / 'llm.txt').read_text())
            self.assertEqual(full, (output / 'llm-full.txt').read_text())
            exported = (output / 'markdown/docs/API.md').read_text()
            self.assertIn('https://raw.githubusercontent.com/mattmezza/cui/main/README.md', exported)
            self.assertIn('if (x < 2) puts("<script>");', exported)
            self.assertIn(exported, full)

    def test_complete_repository_docs(self):
        with tempfile.TemporaryDirectory() as directory:
            pages = DOCS.build(ROOT, Path(directory))
            sources = {p['source'] for p in pages}
            for path in (ROOT / 'docs').rglob('*.md'):
                self.assertIn(DOCS.REPOSITORY + path.relative_to(ROOT).as_posix(), sources)
            self.assertIn('getting-started', {p['slug'] for p in pages})
            self.assertTrue({'start-here', 'ai-agents', 'immediate-and-state'} <= {p['slug'] for p in pages})
            self.assertEqual(len(pages), len({p['slug'] for p in pages}))
            self.assertTrue(all(p['title'] and p['html'] for p in pages))
            index = (Path(directory) / 'llms.txt').read_text()
            full = (Path(directory) / 'llms-full.txt').read_text()
            self.assertIn('./markdown/docs/AI_AGENTS.md', index)
            self.assertIn('## Current component feature manifest', full)
            self.assertIn('"name": "Questionnaire"', full)
            self.assertIn('"name": "TextView"', full)

    def test_collisions_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'docs').mkdir()
            (root / 'docs/A B.md').write_text('# One')
            (root / 'docs/A-B.md').write_text('# Two')
            with self.assertRaises(ValueError):
                DOCS.build(root, root / 'out')


if __name__ == '__main__':
    unittest.main()
