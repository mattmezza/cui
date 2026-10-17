#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# dependencies = ["markdown-it-py==4.0.0", "mdurl==0.1.2"]
# ///
"""Render repository documentation for the static website, without runtime Markdown."""
import argparse
import hashlib
from functools import lru_cache
import subprocess
import json
from pathlib import Path
import re
import shutil
from urllib.parse import quote, unquote, urlsplit

from markdown_it import MarkdownIt
from markdown_it.token import Token

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = 'https://github.com/mattmezza/cui/blob/main/'
GUIDES = {'API', 'ARCHITECTURE', 'CORE', 'INTERFACES', 'THEME_GUIDE',
          'COMPONENT_AUTHORING', 'ACCESSIBILITY', 'PLATFORM_MATRIX', 'DEPENDENCIES',
          'WEB_GALLERY', 'WASI', 'EDITING', 'PERFORMANCE', 'IMMEDIATE_AND_STATE',
          'LANGUAGE_BINDINGS', 'BACKGROUNDS', 'DESIGN_SYSTEM', 'COMPONENT_USAGE', 'COMPOSED_APPS', 'FORM_DESIGN_SCALE', 'TYPOGRAPHY', 'V1_READINESS', 'INTERACTION', 'SOFTWARE_MESH', 'BIDI', 'BIDI_SHAPING', 'IME', 'TOUCH_COMPONENT_COVERAGE', 'TOUCH_FOLLOWUP', 'IME_ADDITIONAL_AUDIT', 'TOUCH_PAGE_NAVIGATION', 'CONTROLLED_SIBLING_AUDIT', 'ACCESSIBILITY_ACTIONS', 'NATIVE_APP_STARTER', 'RESIZABLE_DRAG_AUDIT'}
DEVELOPMENT = {'NIGHTLY_REPORT', 'swarm-status', 'ROADMAP', 'DECISIONS',
               'MAINTAINER_ACTIONS', 'upstream-baseline'}
GUIDES.update({'DESIGN_SIZES', 'DATE_LOCALE_YEAR', 'SIDEBAR_CLIPPING',
               'INPUT_VISUAL_GEOMETRY', 'NATIVE_ACCESSIBILITY', 'EXAMPLE_POLISH',
               'PLOT_INTERACTIONS', 'CONTENT_STYLING', 'AVATAR_MESSAGE_EXAMPLES', 'SCROLLBAR_APPEARANCE', 'RADIO_GROUP', 'CHECKBOX_BEHAVIOR', 'CONTEXTUAL_NATIVE_TEXT', 'ALERT_BUBBLE_GUIDE', 'STATUS_MOTION_GUIDE', 'BUTTON_BEHAVIOR', 'TABLE_BEHAVIOR', 'CHART_APPEARANCE_HOVER', 'THEME_REGISTRY_GUIDE', 'SDL_SYSTEM_APPEARANCE', 'PLOT_CHART_OWNER_MOTION', 'INPUT_REFERENCE_CONTRACTS', 'CORE_PREPAINT', 'PLOT_CHART_CURRENT_FRAME', 'INPUT_ACCEPTANCE', 'CHART_SOURCE_POINT_AXIS', 'INPUT_NORMALIZATION', 'TABS_SCROLL_CHAINING', 'CHART_RADAR_TOOLTIP_TITLE', 'CHART_TYPED_TOOLTIP', 'BUTTON_ACTIVATION_BLOCKING', 'PASSWORD_ENGINE_STAGE1', 'PASSWORD_INPUT_STAGE2', 'INPUT_PATTERN_FORMATTING', 'INPUT_NUMBER_FORMATTING', 'INPUT_POINTER_SELECTION', 'BUTTON_GROUP', 'DROP_SHADOW', 'INSET_SHADOW', 'SHADOW_PRESETS', 'CHART_TOOLTIP_APPEARANCE'})


GUIDES.update({"BINDING_BUILDERS", "GO_BUILDERS", "RUST_BUILDERS", "BRIDGE_COMPONENTS",
               "LAYOUT_GUIDE", "IDS_AND_LIFETIMES", "DEBUGGING", "APP_RECIPES", "ONBOARDING_BENCHMARK",
               "PACKAGING", "SDK_TEXT_PARITY"})

TASKS = {
    'native-app': ('Build a native application', ['START_HERE', 'NATIVE_APP_STARTER', 'IMMEDIATE_AND_STATE', 'IDS_AND_LIFETIMES']),
    'layout': ('Understand and repair layouts', ['LAYOUT_GUIDE', 'APP_RECIPES', 'BUTTON_BEHAVIOR', 'CORE']),
    'debugging': ('Diagnose errors and missing output', ['DEBUGGING', 'IDS_AND_LIFETIMES', 'ICON', 'LAYOUT_GUIDE']),
}
SEARCH_TERMS = {
    'LAYOUT_GUIDE': 'star disappeared grow fit fixed min width max_width clipping nowrap overflow',
    'DEBUGGING': 'capacity error unclosed box unbalanced missing end blank icon diagnostic',
    'ICON': 'blank icon missing icon embedded catalog loader not found',
    'IDS_AND_LIFETIMES': 'zero id duplicate id anonymous stable row identity lifetime borrowed',
    'NATIVE_APP_STARTER': 'bundle tarball custom app app.c main.c Makefile build',
}


@lru_cache(maxsize=16)
def source_revision(root):
    try:
        # Fixtures and unpacked source trees need no Git installation/history.
        if not (root / '.git').exists():
            return 'main'
        return subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'],
                                       text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return 'main'


def description_for(path):
    tokens = MarkdownIt('commonmark', {'html': False}).parse(path.read_text(encoding='utf-8'))
    for index, token in enumerate(tokens):
        if token.type == 'paragraph_open' and index + 1 < len(tokens):
            return re.sub(r'\s+', ' ', tokens[index + 1].content).strip()[:220]
    return path.stem.replace('_', ' ').title()



def slugify(value):
    return re.sub(r'[^\w-]+', '-', value.lower(), flags=re.UNICODE).strip('-') or 'section'


def discover(root):
    paths = [root / 'README.md']
    paths += sorted((root / 'docs').rglob('*.md'))
    paths += sorted((root / 'backends').rglob('README.md'))
    paths += sorted((root / 'examples').rglob('README.md'))
    # Packaged SDKs carry implementation snapshots, including backend READMEs.
    # Keep the canonical binding guides while excluding those vendored copies.
    sdk_sources = [root / 'bindings' / language / 'sdk' / 'source'
                   for language in ('go', 'rust')]
    paths += sorted(path for path in (root / 'bindings').rglob('README.md')
                    if not any(path.is_relative_to(source) for source in sdk_sources))
    return [p for p in paths if p.is_file() and p.resolve().is_relative_to(root.resolve())]


def document_slug(path, root):
    relative = path.relative_to(root)
    if relative.as_posix() == 'README.md':
        return 'getting-started'
    guide_slugs = {'docs/START_HERE.md': 'start-here',
                   'docs/NATIVE_APP_STARTER.md': 'native-app-starter',
                   'docs/AI_AGENTS.md': 'ai-agents',
                   'docs/IMMEDIATE_AND_STATE.md': 'immediate-and-state',
                   'docs/LANGUAGE_BINDINGS.md': 'language-bindings',
                   'docs/DESIGN_SYSTEM.md': 'design-system'}
    if relative.as_posix() in guide_slugs:
        return guide_slugs[relative.as_posix()]
    if relative.parts[0] == 'docs':
        relative = Path(*relative.parts[1:])
    return slugify(relative.with_suffix('').as_posix().replace('/', '-'))


def category(path, root):
    if path == root / 'README.md' or path.stem in {'START_HERE', 'AI_AGENTS', 'NATIVE_APP_STARTER', 'RESIZABLE_DRAG_AUDIT'}:
        return 'Getting started'
    if path.stem in DEVELOPMENT or 'validation' in path.relative_to(root).parts:
        return 'Development'
    if path.stem in GUIDES or path.relative_to(root).parts[0] in {'backends', 'examples'}:
        return 'Guides'
    return 'Components'


def walk(tokens):
    for token in tokens:
        yield token
        if token.children:
            yield from walk(token.children)


def website_text(value):
    import importlib.util
    spec = importlib.util.spec_from_file_location('site_language', Path(__file__).with_name('site_language.py'))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module.website_text(value)


def render_document(path, root, slugs, output):
    parser = MarkdownIt('commonmark', {'html': False}).enable(['table', 'strikethrough'])
    tokens = parser.parse(path.read_text(encoding='utf-8'))
    slug = slugs[path.resolve()]
    headings = {}
    title = path.stem.replace('_', ' ').title()
    have_title = False
    title_index = None
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
            title, have_title = website_text(content), True
            title_index = index
        elif token.tag == 'h1':
            # The document shell owns the page's main heading.
            token.tag = tokens[index + 2].tag = 'h2'

    if title_index is not None:
        # Keep source fragment links without repeating the shell's visible title.
        anchor = Token('html_block', '', 0)
        anchor.content = '<span id="' + tokens[title_index].attrGet('id') + '"></span>\n'
        tokens[title_index:title_index + 3] = [anchor]

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
        result = REPOSITORY.replace('/main/', '/' + source_revision(root) + '/') + quote(relative, safe='/')
        if url.fragment:
            result += '#' + quote(unquote(url.fragment), safe='-')
        return result

    for token in walk(tokens):
        if token.type in {'text', 'code_inline', 'fence', 'code_block'}:
            token.content = website_text(token.content)
        if token.type == 'link_open':
            token.attrSet('href', rewrite(token.attrGet('href') or ''))
        elif token.type == 'image':
            token.attrSet('src', rewrite(token.attrGet('src') or '', image=True))
            token.attrSet('loading', 'lazy')
    relative = path.relative_to(root).as_posix()
    historical = category(path, root) == 'Development'
    return {'slug': slug, 'title': title, 'html': parser.renderer.render(tokens, parser.options, {}),
            'source': REPOSITORY.replace('/main/', '/' + source_revision(root) + '/') + quote(relative, safe='/'),
            'markdown': './markdown/' + quote(relative, safe='/'),
            'description': description_for(path), 'keywords': SEARCH_TERMS.get(path.stem, ''),
            'symbols': sorted(set(re.findall(r'\bcui_[A-Za-z0-9_]+', path.read_text(encoding='utf-8')))),
            'historical': historical, 'category': category(path, root)}


def markdown_for_agents(path, root):
    """Keep code fences exact; make repository-relative prose links fetchable."""
    text = website_text(path.read_text(encoding='utf-8'))
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
        destination = 'https://raw.githubusercontent.com/mattmezza/cui/' + source_revision(root) + '/'
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


def publish_agent_docs(root, output, paths, documents, profile):
    introduction = ('# CUI\n\n'
        '> Experimental portable C11 UI: immediate frame composition, persistent '
        'application models, themed components and independent host/renderer adapters.\n\n'
        'All 78 comparison entries remain in scope; gallery presence is not complete '
        'component parity. Use catalog.json for feature status and limitations. '
        'Public headers define the API; examples must state their tested host.\n')
    revision = source_revision(root)
    headers = sorted((root / 'include/cui').glob('*.h'))
    hashes = {}
    symbols = []
    declaration = re.compile(r'^(?:[A-Za-z_]\w*[ \t*]+)+(cui_[A-Za-z0-9_]+)\s*\([^;{}]*\)\s*;', re.M)
    for header in headers:
        relative = header.relative_to(root).as_posix()
        target = output / 'headers' / header.name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(header, target)
        hashes[relative] = hashlib.sha256(header.read_bytes()).hexdigest()
        clean = re.sub(r'/\*.*?\*/|//[^\n]*', '', header.read_text(), flags=re.S)
        for match in declaration.finditer(clean):
            symbols.append({'name': match[1], 'declaration': re.sub(r'\s+', ' ', match[0]).strip(),
                            'header': './headers/' + header.name})
    documentation_hashes = {path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in paths}
    metadata = {'source_revision': revision, 'build_profile': profile, 'headers_sha256': hashes,
                'documentation_sha256': documentation_hashes,
                'api_authority': 'Matching public headers; declarations do not establish feature completeness.'}
    (output / 'build-info.json').write_text(json.dumps(metadata, indent=2) + '\n')
    (output / 'api-index.json').write_text(json.dumps({'source_revision': revision, 'symbols': symbols}, indent=2) + '\n')
    introduction += ('\nSource revision: `' + revision + '`. Build profile: `' + profile + '`. '
                     '[Exact input hashes and build metadata](./build-info.json).\n')
    index = introduction
    bundle = introduction
    index += '\n## Choose a task\n\n'
    for task, (title, stems) in TASKS.items():
        chosen = [(path, doc) for path, doc in zip(paths, documents) if path.stem in stems]
        content = introduction + '\n# ' + title + '\n'
        for path, doc in chosen:
            content += '\n## ' + doc['title'] + '\n\n' + markdown_for_agents(path, root)
        target = output / 'tasks' / (task + '.md')
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content, encoding='utf-8')
        index += f'- [{title}](./tasks/{task}.md): A focused reading set for this task.\n'

    complete_index = introduction + '\n## All documentation\n\n'
    curated = {'API', 'CORE', 'INTERFACES', 'ICON', 'AI_AGENTS', 'LAYOUT_GUIDE', 'IDS_AND_LIFETIMES', 'DEBUGGING', 'APP_RECIPES', 'THEME_GUIDE', 'PLATFORM_MATRIX', 'IMMEDIATE_AND_STATE', 'LANGUAGE_BINDINGS'}
    for group in ('Getting started', 'Guides', 'Components', 'Development'):
        entries = [(path, doc) for path, doc in zip(paths, documents) if doc['category'] == group]
        if not entries:
            continue
        if group in {'Getting started', 'Guides'}:
            index += '\n## ' + group + '\n\n'
        for path, doc in entries:
            relative = path.relative_to(root).as_posix()
            url = './markdown/' + quote(relative, safe='/')
            label = doc['title'].replace('[', '\\[').replace(']', '\\]')
            entry = f'- [{label}]({url}): {doc["description"]}\n'
            complete_index += entry
            if group == 'Getting started' or (path.stem in curated and not doc['historical']):
                index += entry
            markdown = markdown_for_agents(path, root)
            destination = output / 'markdown' / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(markdown, encoding='utf-8')
            if not doc['historical']:
                bundle += f"\n\n---\n\n## {doc['title']}\n\nSource: {doc['source']}\n\n{markdown}"
    (output / 'docs-index.md').write_text(complete_index, encoding='utf-8')
    index += ('\n## API and status\n\n'
              '- [Complete documentation index](./docs-index.md): All guides, component references and historical reports.\n'
              '- [API symbol index](./api-index.json): Header-derived declarations and matching downloadable headers.\n'
              '- [Feature manifest](./catalog.json): All 78 entries and incomplete behavior.\n'
              '- [Full documentation bundle](./llms-full.txt): Current documentation and catalog data; historical reports are linked separately.\n')
    manifest = root / 'docs/gpui-parity.json'
    if manifest.is_file():
        shutil.copyfile(manifest, output / 'catalog.json')
        bundle += '\n\n## Current component feature manifest\n\n```json\n' + website_text(manifest.read_text(encoding='utf-8')) + '\n```\n'
    for name in ('llms.txt', 'llm.txt'):
        (output / name).write_text(index, encoding='utf-8')
    for name in ('llms-full.txt', 'llm-full.txt'):
        (output / name).write_text(bundle, encoding='utf-8')


def build(root, output, profile='source-defaults'):
    root, output = root.resolve(), output.resolve()
    paths = discover(root)
    slugs = {p.resolve(): document_slug(p, root) for p in paths}
    if len(set(slugs.values())) != len(slugs):
        raise ValueError('Documentation slugs must be unique')
    output.mkdir(parents=True, exist_ok=True)
    documents = [render_document(p, root, slugs, output) for p in paths]
    (output / 'docs.json').write_text(json.dumps(documents, ensure_ascii=False, indent=2) + '\n',
                                     encoding='utf-8')
    publish_agent_docs(root, output, paths, documents, profile)
    return documents


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/web')
    parser.add_argument('--profile', default='source-defaults', help='Actual artifact build configuration')
    args = parser.parse_args()
    documents = build(args.root, args.output, args.profile)
    print(f'Rendered {len(documents)} documentation pages into {args.output / "docs.json"}')


if __name__ == '__main__':
    main()
