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
    def test_title_is_owned_by_shell_and_source_anchors_survive(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'README.md').write_text('# Start\n\n[Title](#start)\n\n# Start\n\n## Details\n\nBody.\n')
            page = DOCS.build(root, root / 'output')[0]
            self.assertEqual(page['title'], 'Start')
            self.assertNotIn('<h1', page['html'])
            self.assertIn('<span id="doc-start"></span>', page['html'])
            self.assertIn('href="#docs/getting-started/start"', page['html'])
            self.assertIn('<h2 id="doc-start-1">Start</h2>', page['html'])
            self.assertIn('<h2 id="doc-details">Details</h2>', page['html'])
            self.assertIn('Body.', page['html'])
            # Agent-facing Markdown remains an exact source-title document.
            self.assertTrue((root / 'output/markdown/README.md').read_text().startswith('# Start\n'))

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

    def test_versioned_exports_headers_tasks_and_history(self):
        import hashlib
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'docs').mkdir()
            (root / 'include/cui').mkdir(parents=True)
            (root / 'README.md').write_text('# Start\n\nBuild a C11 application.\n')
            (root / 'docs/LAYOUT_GUIDE.md').write_text('# Layout\n\nConstrain long text beside a fixed star.\n')
            (root / 'docs/NIGHTLY_REPORT.md').write_text('# Historical nightly\n\nObsolete experiment.\n')
            header = root / 'include/cui/cui.h'
            header.write_text('typedef void (*cui_callback)(int);\n/* description */\nbool cui_begin_frame(cui_context *ui, float w, float h, float dt);\n')
            output = root / 'output'
            pages = DOCS.build(root, output, 'web-default-icons')
            info = json.loads((output / 'build-info.json').read_text())
            self.assertEqual(info['build_profile'], 'web-default-icons')
            self.assertEqual(info['headers_sha256']['include/cui/cui.h'], hashlib.sha256(header.read_bytes()).hexdigest())
            self.assertEqual((output / 'headers/cui.h').read_bytes(), header.read_bytes())
            symbols = json.loads((output / 'api-index.json').read_text())['symbols']
            self.assertEqual([symbol['name'] for symbol in symbols], ['cui_begin_frame'])
            self.assertIn('cui_context *ui', symbols[0]['declaration'])
            self.assertIn('fixed star', (output / 'tasks/layout.md').read_text())
            self.assertNotIn('Obsolete experiment', (output / 'llms-full.txt').read_text())
            self.assertNotIn('Historical nightly', (output / 'llms.txt').read_text())
            self.assertIn('Historical nightly', (output / 'docs-index.md').read_text())
            self.assertTrue(next(page for page in pages if page['slug'] == 'nightly_report')['historical'])
            layout = next(page for page in pages if page['title'] == 'Layout')
            self.assertIn('star disappeared', layout['keywords'])
            self.assertEqual(layout['markdown'], './markdown/docs/LAYOUT_GUIDE.md')

    def test_canonical_application_snippets_compile(self):
        import os
        import re
        import shlex
        import shutil
        import subprocess
        compiler = shlex.split(os.environ.get('CC', 'cc'))
        if not compiler or not shutil.which(compiler[0]):
            self.skipTest('A C11 compiler is required for documentation compilation')
        layout = (ROOT / 'docs/LAYOUT_GUIDE.md').read_text()
        recipes = (ROOT / 'docs/APP_RECIPES.md').read_text()
        snippets = re.findall(r'```c\n(.*?)```', layout, re.S)
        self.assertGreaterEqual(len(snippets), 2)
        sidebar = re.findall(r'```c\n(.*?)```', recipes, re.S)[0]
        prefix = '#include "cui/cui_components.h"\n#include "cui/cui_sidebar.h"\n'
        sources = [prefix + 'void doc_example(cui_context *ui, const cui_theme *theme, cui_str subject) {\n(void)theme; (void)subject;\n' + snippet + '\n}\n' for snippet in snippets]
        sources.append(prefix + sidebar + '\nvoid callback_contract(void) { cui_sidebar_content_fn fn = folder_label; (void)fn; }\n')
        with tempfile.TemporaryDirectory() as directory:
            for index, source in enumerate(sources):
                path = Path(directory) / f'example_{index}.c'
                path.write_text(source)
                result = subprocess.run([*compiler, '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-fsyntax-only', '-I' + str(ROOT / 'include'), str(path)], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)

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

    def test_packaging_guide_and_canonical_binding_docs(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory) / 'repo'
            output = Path(directory) / 'site'
            canonical = ['bindings/README.md', 'bindings/go/README.md',
                         'bindings/rust/README.md', 'bindings/go/sdk/README.md',
                         'bindings/rust/sdk/README.md']
            copies = ['bindings/go/sdk/source/README.md',
                      'bindings/go/sdk/source/backends/software/README.md',
                      'bindings/rust/sdk/source/README.md',
                      'bindings/rust/sdk/source/backends/software/README.md']
            for relative in canonical + copies + ['docs/PACKAGING.md']:
                path = root / relative
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(f'# {relative}\n\nFixture documentation.\n')
            discovered = {path.relative_to(root).as_posix() for path in DOCS.discover(root)}
            self.assertEqual(discovered, set(canonical + ['docs/PACKAGING.md']))
            pages = DOCS.build(root, output)
            sources = {page['source'] for page in pages}
            self.assertEqual(sources, {DOCS.REPOSITORY + relative for relative in discovered})
            packaging = next(page for page in pages if page['slug'] == 'packaging')
            self.assertEqual(packaging['category'], 'Guides')
            index = (output / 'llms.txt').read_text()
            full = (output / 'llms-full.txt').read_text()
            self.assertIn('./markdown/docs/PACKAGING.md', index)
            for relative in canonical:
                self.assertIn('./markdown/' + relative, index)
                self.assertTrue((output / 'markdown' / relative).is_file())
            for relative in copies:
                self.assertNotIn(relative, index)
                self.assertNotIn(relative, full)
                self.assertFalse((output / 'markdown' / relative).exists())

    def test_complete_repository_docs(self):
        with tempfile.TemporaryDirectory() as directory:
            pages = DOCS.build(ROOT, Path(directory))
            sources = {p['source'] for p in pages}
            for path in (ROOT / 'docs').rglob('*.md'):
                self.assertIn(DOCS.REPOSITORY.replace('/main/', '/' + DOCS.source_revision(ROOT) + '/') + path.relative_to(ROOT).as_posix(), sources)
            self.assertIn('getting-started', {p['slug'] for p in pages})
            self.assertTrue({'start-here', 'ai-agents', 'immediate-and-state'} <= {p['slug'] for p in pages})
            self.assertEqual(len(pages), len({p['slug'] for p in pages}))
            readiness = next(p for p in pages if p['slug'] == 'v1_readiness')
            self.assertEqual(readiness['category'], 'Guides')
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
