#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["markdown-it-py==4.0.0", "mdurl==0.1.2"]
# ///
"""Render repository documentation for the static website, without runtime Markdown."""
import argparse
import json
from pathlib import Path
import re
import shutil
from urllib.parse import quote, unquote, urlsplit

from markdown_it import MarkdownIt

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = 'https://github.com/mattmezza/cui/blob/main/'
GUIDES = {'API', 'ARCHITECTURE', 'CORE', 'INTERFACES', 'THEME_GUIDE',
          'COMPONENT_AUTHORING', 'ACCESSIBILITY', 'PLATFORM_MATRIX', 'DEPENDENCIES',
          'WEB_GALLERY', 'WASI', 'EDITING', 'PERFORMANCE', 'IMMEDIATE_AND_STATE'}
DEVELOPMENT = {'NIGHTLY_REPORT', 'swarm-status', 'ROADMAP', 'DECISIONS',
               'MAINTAINER_ACTIONS', 'upstream-baseline'}


def slugify(value):
    return re.sub(r'[^\w-]+', '-', value.lower(), flags=re.UNICODE).strip('-') or 'section'


def discover(root):
    paths = [root / 'README.md']
    paths += sorted((root / 'docs').rglob('*.md'))
    paths += sorted((root / 'backends').rglob('README.md'))
    paths += sorted((root / 'examples').rglob('README.md'))
    return [p for p in paths if p.is_file() and p.resolve().is_relative_to(root.resolve())]


def document_slug(path, root):
    relative = path.relative_to(root)
    if relative.as_posix() == 'README.md':
        return 'getting-started'
    guide_slugs = {'docs/START_HERE.md': 'start-here',
                   'docs/AI_AGENTS.md': 'ai-agents',
                   'docs/IMMEDIATE_AND_STATE.md': 'immediate-and-state'}
    if relative.as_posix() in guide_slugs:
        return guide_slugs[relative.as_posix()]
    if relative.parts[0] == 'docs':
        relative = Path(*relative.parts[1:])
    return slugify(relative.with_suffix('').as_posix().replace('/', '-'))


def category(path, root):
    if path == root / 'README.md' or path.stem in {'START_HERE', 'AI_AGENTS'}:
        return 'Getting started'
    if path.stem in DEVELOPMENT:
        return 'Development'
    if path.stem in GUIDES or path.relative_to(root).parts[0] in {'backends', 'examples'}:
        return 'Guides'
    return 'Components'


def walk(tokens):
    for token in tokens:
        yield token
        if token.children:
            yield from walk(token.children)


def render_document(path, root, slugs, output):
    parser = MarkdownIt('commonmark', {'html': False}).enable(['table', 'strikethrough'])
    tokens = parser.parse(path.read_text(encoding='utf-8'))
    slug = slugs[path.resolve()]
    headings = {}
    title = path.stem.replace('_', ' ').title()
    have_title = False
    for index, token in enumerate(tokens):
        if token.type != 'heading_open':
            continue
        content = tokens[index + 1].content
        base = slugify(content)
        occurrence = headings.get(base, 0)
        headings[base] = occurrence + 1
        anchor = base + (f'-{occurrence}' if occurrence else '')
        token.attrSet('id', 'doc-' + anchor)
        if not have_title and token.tag == 'h1':
            title, have_title = content, True

    def rewrite(value, image=False):
        url = urlsplit(value)
        if url.scheme or url.netloc:
            return value if url.scheme.lower() in {'https', 'http', 'mailto'} else ''
        if not url.path:
            return '#docs/' + slug + ('/' + quote(unquote(url.fragment), safe='-') if url.fragment else '')
        target = (path.parent / unquote(url.path)).resolve()
        if not target.is_relative_to(root.resolve()):
            return ''
        relative = target.relative_to(root.resolve()).as_posix()
        if target in slugs and not image:
            result = '#docs/' + slugs[target]
            return result + ('/' + quote(unquote(url.fragment), safe='-') if url.fragment else '')
        if image and target.is_file() and target.suffix.lower() in {'.png', '.jpg', '.jpeg', '.gif', '.webp', '.svg'}:
            asset = Path('docs-assets') / relative
            destination = output / asset
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(target, destination)
            return quote(asset.as_posix(), safe='/')
        result = REPOSITORY + quote(relative, safe='/')
        if url.fragment:
            result += '#' + quote(unquote(url.fragment), safe='-')
        return result

    for token in walk(tokens):
        if token.type == 'link_open':
            token.attrSet('href', rewrite(token.attrGet('href') or ''))
        elif token.type == 'image':
            token.attrSet('src', rewrite(token.attrGet('src') or '', image=True))
            token.attrSet('loading', 'lazy')
    return {'slug': slug, 'title': title, 'html': parser.renderer.render(tokens, parser.options, {}),
            'source': REPOSITORY + quote(path.relative_to(root).as_posix(), safe='/'),
            'category': category(path, root)}


def markdown_for_agents(path, root):
    """Keep code fences exact; make repository-relative prose links fetchable."""
    text = path.read_text(encoding='utf-8')
    lines = text.splitlines(keepends=True)
    parser = MarkdownIt('commonmark', {'html': False})
    protected = set()
    for token in parser.parse(text):
        if token.type in {'fence', 'code_block'} and token.map:
            protected.update(range(*token.map))

    def rewrite(match):
        prefix, value, suffix = match.groups()
        url = urlsplit(value)
        if url.scheme or url.netloc or not url.path:
            return match.group(0)
        target = (path.parent / unquote(url.path)).resolve()
        if not target.is_relative_to(root.resolve()):
            return match.group(0)
        destination = 'https://raw.githubusercontent.com/mattmezza/cui/main/'
        destination += quote(target.relative_to(root.resolve()).as_posix(), safe='/')
        if url.fragment:
            destination += '#' + url.fragment
        return prefix + destination + suffix

    # Markdown source remains downloadable text; HTML rendering uses the separate
    # safe token-based renderer above. Do not rewrite literal code examples.
    pattern = re.compile(r'(\]\()([^\s)]+)(\))')
    for index, line in enumerate(lines):
        if index not in protected:
            lines[index] = pattern.sub(rewrite, line)
    return ''.join(lines)


def publish_agent_docs(root, output, paths, documents):
    introduction = ('# CUI\n\n'
        '> Experimental portable C11 UI: immediate frame composition, persistent '
        'application models, themed components and independent host/renderer adapters.\n\n'
        'All 78 comparison entries remain in scope; gallery presence is not complete '
        'GPUI Kit parity. Use catalog.json for feature status and limitations. '
        'Public headers define the API; examples must state their tested host.\n')
    index = introduction
    bundle = introduction
    for group in ('Getting started', 'Guides', 'Components', 'Development'):
        entries = [(path, doc) for path, doc in zip(paths, documents) if doc['category'] == group]
        if not entries:
            continue
        index += '\n## ' + ('Optional' if group == 'Development' else group) + '\n\n'
        for path, doc in entries:
            relative = path.relative_to(root).as_posix()
            url = './markdown/' + quote(relative, safe='/')
            label = doc['title'].replace('[', '\\[').replace(']', '\\]')
            index += f'- [{label}]({url}): {group} documentation.\n'
            markdown = markdown_for_agents(path, root)
            destination = output / 'markdown' / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(markdown, encoding='utf-8')
            bundle += f"\n\n---\n\n## {doc['title']}\n\nSource: {doc['source']}\n\n{markdown}"
    index += ('\n## API and status\n\n'
              '- [Public headers](https://github.com/mattmezza/cui/tree/main/include/cui): API authority.\n'
              '- [Feature manifest](./catalog.json): All 78 entries and incomplete behavior.\n'
              '- [Full documentation bundle](./llms-full.txt): All documentation and catalog data.\n')
    manifest = root / 'docs/gpui-parity.json'
    if manifest.is_file():
        shutil.copyfile(manifest, output / 'catalog.json')
        bundle += '\n\n## Current component feature manifest\n\n```json\n' + manifest.read_text(encoding='utf-8') + '\n```\n'
    for name in ('llms.txt', 'llm.txt'):
        (output / name).write_text(index, encoding='utf-8')
    for name in ('llms-full.txt', 'llm-full.txt'):
        (output / name).write_text(bundle, encoding='utf-8')


def build(root, output):
    root, output = root.resolve(), output.resolve()
    paths = discover(root)
    slugs = {p.resolve(): document_slug(p, root) for p in paths}
    if len(set(slugs.values())) != len(slugs):
        raise ValueError('Documentation slugs must be unique')
    output.mkdir(parents=True, exist_ok=True)
    documents = [render_document(p, root, slugs, output) for p in paths]
    (output / 'docs.json').write_text(json.dumps(documents, ensure_ascii=False, indent=2) + '\n',
                                     encoding='utf-8')
    publish_agent_docs(root, output, paths, documents)
    return documents


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/web')
    args = parser.parse_args()
    documents = build(args.root, args.output)
    print(f'Rendered {len(documents)} documentation pages into {args.output / "docs.json"}')


if __name__ == '__main__':
    main()
