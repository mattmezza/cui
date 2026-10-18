"""Standard-library exporter shared by the website and the distributable CUI skill."""
import hashlib
import json
from pathlib import Path
import re

FUNCTION = re.compile(r'^(?:[A-Za-z_]\w*[ \t*\r\n]+)+(cui_[A-Za-z0-9_]+)\s*\([^;{}]*\)\s*[;{]', re.M)
ALIASES = {
    'Select': 'dropdown searchable combobox autocomplete picker options selection',
    'DropdownButton': 'split button actions menu',
    'Input': 'textbox textfield single line edit',
    'Textarea': 'multiline textbox text editor',
    'Dialog': 'modal popup overlay',
    'VirtualList': 'virtualized large list recycling',
    'DataTable': 'grid spreadsheet rows columns sorting',
    'Sidebar': 'navigation side panel',
    'Theme': 'colors appearance dark light styling',
}


def public_api(root):
    """Index function declarations; complete headers remain authority for types/macros."""
    symbols = []
    for header in sorted((root / 'include/cui').glob('*.h'), key=lambda p: p.as_posix()):
        clean = re.sub(r'/\*.*?\*/|//[^\n]*', '', header.read_text(encoding='utf-8'), flags=re.S)
        for match in FUNCTION.finditer(clean):
            symbols.append({'name': match[1], 'declaration': re.sub(r'\s+', ' ', match[0].rstrip().removesuffix('{').rstrip().removesuffix(';') + ';').strip(),
                            'header': header.relative_to(root).as_posix()})
    return symbols


def source_files(root):
    """Consumer docs and examples, excluding checkpoint reports and upstream references."""
    paths = {root / 'README.md', root / 'LICENSE', root / 'docs/component-usage.json',
             root / 'docs/gpui-parity.json', root / 'docs/binding-coverage.json'}
    historical = {'NIGHTLY_REPORT', 'swarm-status', 'ROADMAP', 'DECISIONS',
                  'MAINTAINER_ACTIONS', 'upstream-baseline'}
    paths.update(p for p in (root / 'docs').glob('*.md') if p.stem not in historical)
    paths.update((root / 'include/cui').glob('*.h'))
    for directory in ('examples', 'bindings', 'backends'):
        paths.update((root / directory).rglob('README.md'))
    # Runnable examples and language API definitions, not binary assets or test reports.
    for directory in ('examples', 'bindings'):
        for p in (root / directory).rglob('*'):
            if (p.is_file() and not any(x in {'build', 'target', 'node_modules', '__pycache__'} for x in p.parts)
                    and p.suffix in {'.c', '.h', '.py', '.js', '.cjs', '.ts', '.go', '.rs', '.toml'}
                    and ('examples' in p.parts or p.name in {'index.d.ts', '__init__.py', 'lib.rs'})):
                paths.add(p)
    for path in ('web/gallery.c', 'web/gallery.js', 'cmake/sources.json', 'CMakeLists.txt',
                 'scripts/build_web.py', 'scripts/native_bundle.py', 'scripts/test_local.py',
                 'third_party/dependencies.json'):
        paths.add(root / path)
    return sorted((p for p in paths if p.is_file()), key=lambda p: p.as_posix())


def collect(root):
    root = Path(root).resolve()
    documents = {p.relative_to(root).as_posix(): p.read_text(encoding='utf-8') for p in source_files(root)}
    hashes = {p: hashlib.sha256(text.encode()).hexdigest() for p, text in documents.items()}
    fingerprint = hashlib.sha256(json.dumps(hashes, sort_keys=True).encode()).hexdigest()
    usage = json.loads(documents['docs/component-usage.json'])
    manifest = json.loads(documents['docs/gpui-parity.json'])['components']
    if len(manifest) != 78 or [x['name'] for x in manifest] != [x['name'] for x in usage['components']]:
        raise ValueError('Usage metadata must cover all 78 manifest entries in order')
    components = []
    api = public_api(root)
    names = {s['name'] for s in api}
    for entry, status in zip(usage['components'], manifest):
        for path in [entry['public_header'], *entry['docs']]:
            if path not in documents:
                raise ValueError('Missing component resource: ' + path)
        for symbol in entry['api_symbols']:
            if symbol not in names:
                raise ValueError('Missing public function: ' + symbol)
        components.append({**status, **entry, 'aliases': ALIASES.get(entry['name'], '')})
    return {'schema': 1, 'source_digest': fingerprint, 'source_hashes': hashes,
            'lifecycle': usage['lifecycle'], 'components': components,
            'symbols': api, 'documents': documents}
