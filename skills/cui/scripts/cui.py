#!/usr/bin/env python3
"""CUI discovery, prerequisite checks and pinned project setup. Python 3.9+, stdlib only."""
import argparse
import hashlib
import io
import json
from pathlib import Path, PurePosixPath
import re
import shutil
import shlex
import subprocess
import sys
import tarfile
import tempfile
import urllib.request
from urllib.parse import quote
from agent_resources import collect

SKILL = Path(__file__).resolve().parents[1]
TEMPLATES = {
    'native': ('app.c', 'Standalone SDL3 source bundle; run make after doctor --profile native.'),
    'portable': ('examples/hello_world/main.c', 'cmake -S . -B build -DCUI_BUILD_NATIVE=OFF; cmake --build build'),
    'web': ('web/gallery.c', 'python scripts/build_web.py; see docs/WEB_GALLERY.md for the Canvas/WASM host.'),
    'python': ('bindings/bridge/examples/save_changes.py', 'Read docs/BINDING_BUILDERS.md; build bindings/bridge, then select headless or SDL explicitly.'),
    'node': ('bindings/node/examples/save.cjs', 'Read docs/BINDING_BUILDERS.md; build bindings/node and run the example with --headless first.'),
    'typescript': ('bindings/typescript/examples/save.ts', 'Read docs/BINDING_BUILDERS.md; uses the Node addon and Node type stripping.'),
    'go': ('bindings/go/examples/save_changes/main.go', 'Read docs/GO_BUILDERS.md for bridge build/link and runtime library paths.'),
    'rust': ('bindings/rust/examples/save_changes.rs', 'Read docs/RUST_BUILDERS.md for bridge build/link and runtime library paths.'),
    'embedding': ('examples/embedding/consumer.cpp', 'Read docs/INTERFACES.md and docs/PLATFORM_MATRIX.md before selecting adapters.'),
    'sdl3': ('backends/sdl3/README.md', 'Read the SDL3 adapter contract and docs/NATIVE_APP_STARTER.md.'),
    'sdl-gpu': ('backends/sdl_gpu/README.md', 'Read the SDL GPU adapter contract and docs/PLATFORM_MATRIX.md; runtime GPU checks are separate.'),
    'sokol': ('backends/sokol/README.md', 'Read the Sokol adapter contract and docs/PLATFORM_MATRIX.md.'),
    'software': ('backends/software/README.md', 'Read the software renderer contract; supply a host independently.'),
}


def resources(source=None):
    if source:
        return collect(source)
    return json.loads((SKILL / 'data/resources.json').read_text(encoding='utf-8'))


def emit(value, as_json=False):
    print(json.dumps(value, indent=2, ensure_ascii=False) if as_json else value)


def search(data, query, kind='all', limit=10):
    words = re.findall(r'\w+', query.casefold())
    if not words:
        return []
    entries = []
    for c in data['components']:
        entries.append(('component', c['name'], c['usage'], c['aliases'] + ' ' + c['category'], json.dumps(c)))
    for s in data['symbols']:
        entries.append(('api', s['name'], s['declaration'], s['header'], s['declaration']))
    for path, body in data['documents'].items():
        if path.endswith('.md'):
            title = next((line.lstrip('# ') for line in body.splitlines() if line.startswith('# ')), path)
            entries.append(('guide', path, title, path.replace('_', ' '), body))
    results = []
    for category, name, description, aliases, body in entries:
        if kind != 'all' and category != kind:
            continue
        fields = [name.casefold(), aliases.casefold(), description.casefold(), body.casefold()]
        matched = sum(any(word in field for field in fields) for word in words)
        if not matched:
            continue
        score = sum(max((weight for field, weight in zip(fields, [30, 20, 8, 1]) if word in field), default=0) for word in words)
        if query.casefold() == name.casefold():
            score += 1000
        results.append({'kind': category, 'name': name, 'description': description,
                        'score': score, 'matched_words': matched, 'partial': matched < len(words)})
    complete = [r for r in results if not r['partial']]
    return sorted(complete or results, key=lambda r: (-r['matched_words'], -r['score'], r['name']))[:limit]


def component(data, name, section):
    c = next((x for x in data['components'] if x['name'].casefold() == name.casefold()), None)
    if c is None:
        raise ValueError('Unknown component; use search or list.')
    result = {'name': c['name'], 'source_digest': data['source_digest']}
    if section in {'all', 'usage'}:
        result['usage'] = {k: c[k] for k in ('scope', 'composition', 'state_ownership', 'interaction', 'usage', 'state_types', 'callback_symbols')}
    if section in {'all', 'api'}:
        result['api'] = [s for s in data['symbols'] if s['name'] in c['api_symbols']]
        # Full header retains enums, callbacks, structs, defaults and comments.
        result['header'] = {'path': c['public_header'], 'text': data['documents'][c['public_header']]}
    if section in {'all', 'limitations'}:
        result['status'] = {k: c.get(k) for k in ('status', 'platforms', 'limitations', 'features')}
    if section in {'all', 'examples'}:
        result['examples'] = [{'path': p, 'text': data['documents'][p]} for p in c['docs']]
        result['sample'] = c.get('sample')
        result['tests'] = c['tests']
    return result


def read_command(path, source=None):
    words = [sys.executable, str(Path(__file__).resolve()), 'read', path]
    if source:
        words += ['--source', str(source)]
    return shlex.join(words)


def resource_index(data, paths, source=None):
    for path in dict.fromkeys(paths):
        body = data['documents'].get(path, '')
        title = next((line[2:] for line in body.splitlines() if line.startswith('# ')), path)
        print(path + ' — ' + title)
        print('  ' + read_command(path, source))


def declaration_index(data, matches, paths, source=None):
    print('Source digest: ' + data['source_digest'])
    print('Declaration/index only: incomplete contract. Read authoritative headers and relevant guides before building.')
    for match in matches:
        print(match['declaration'])
    if not matches:
        print('No indexed function declaration; types/macros require the exact resources below.')
    resource_index(data, paths, source)


def markdown_headings(body):
    """ATX headings outside fenced/indented code; not a complete CommonMark parser."""
    headings = []
    fence = None
    lines = body.splitlines(keepends=True)
    for index, line in enumerate(lines):
        marker = re.match(r'^ {0,3}(`{3,}|~{3,})(.*)$', line.rstrip('\r\n'))
        if fence:
            if marker and marker[1][0] == fence[0] and len(marker[1]) >= fence[1] and not marker[2].strip():
                fence = None
            continue
        if marker and (marker[1][0] != '`' or '`' not in marker[2]):
            fence = (marker[1][0], len(marker[1]))
            continue
        match = re.match(r'^ {0,3}(#{1,6})(?:[ \t]+(.*?)|[ \t]*)$', line.rstrip('\r\n'))
        if match:
            title = re.sub(r'[ \t]+#+[ \t]*$', '', match[2] or '').strip()
            headings.append({'title': title, 'level': len(match[1]), 'start': index + 1})
    if fence:
        raise ValueError('Unterminated Markdown code fence; use the full read instead of a partial view.')
    for index, heading in enumerate(headings):
        heading['end'] = next((h['start'] - 1 for h in headings[index + 1:] if h['level'] <= heading['level']), len(lines))
    return headings


def read_partial(data, path, section, source=None):
    if section == '':
        raise ValueError('--section requires a nonempty exact heading title')
    body = data['documents'][path]
    headings = markdown_headings(body)
    total = len(body.splitlines())
    selected = [h for h in headings if h['title'] == section] if section is not None else []
    if section is not None and len(selected) != 1:
        locations = ', '.join(str(h['start']) for h in selected)
        raise ValueError('Section must match one exact heading; ' + ('duplicate lines: ' + locations if selected else 'not found') + '. Use --headings or the full read.')
    print('Partial Markdown view (ATX headings outside code); not a complete contract or standalone program.')
    print('Resource: ' + path + '\nSource digest: ' + data['source_digest'])
    print('Full read: ' + read_command(path, source))
    if section is None:
        for h in headings:
            print(f"{h['level']}: {h['title']} [lines {h['start']}–{h['end']} of {total}]")
        return
    h = selected[0]
    print(f"Lines {h['start']}–{h['end']} of {total}; omitted before: {h['start'] - 1}, after: {total - h['end']}.")
    print(''.join(body.splitlines(keepends=True)[h['start'] - 1:h['end']]), end='')


def doctor(profile, font=None):
    """Read-only probes. CMake remains authority for actual link/header availability."""
    checks = []
    def executable(name, alternatives=()):
        found = next((shutil.which(n) for n in (name, *alternatives) if shutil.which(n)), None)
        checks.append({'dependency': name, 'ok': bool(found), 'detail': found or 'not found on PATH'})
    executable('cc', ('clang', 'gcc', 'cl'))
    if profile == 'native':
        executable('make')
    else:
        executable('cmake')
        executable('ninja', ('make',))
    if profile in {'native', 'sdl3', 'sdl-gpu', 'sokol'}:
        executable('pkg-config')
        packages = ('gl', 'freetype2', 'harfbuzz') if profile == 'sokol' else ('sdl3 >= 3.2', 'freetype2', 'harfbuzz')
        for package in packages:
            ok = bool(shutil.which('pkg-config')) and subprocess.run(['pkg-config', '--exists', package], capture_output=True).returncode == 0
            checks.append({'dependency': package, 'ok': ok, 'detail': 'pkg-config discovery; configure/build must verify compatibility'})
        checks.append({'dependency': 'font', 'ok': bool(font and font.is_file()), 'detail': str(font) if font else 'Pass --font with a readable TTF/OTF; configure fallback families for your scripts.'})
    if profile == 'web':
        executable('zig')
        if shutil.which('zig'):
            version = subprocess.run(['zig', 'version'], capture_output=True, text=True).stdout.strip()
            checks.append({'dependency': 'Zig 0.16.0', 'ok': version == '0.16.0', 'detail': version})
    if profile in {'node', 'typescript'}:
        executable('node')
        checks.append({'dependency': 'Node development headers', 'ok': None, 'detail': 'CMake must locate node_api.h; TypeScript also needs a suitable Node type-stripping runtime or project compiler.'})
    if profile == 'go':
        executable('go')
    if profile == 'rust':
        executable('cargo')
        executable('rustc')
    return {'profile': profile, 'checks': checks,
            'note': 'No packages installed. Binding profiles check headless build tools; run native checks too for SDL windows. CMake and a real build verify adapter/version support. Sokol checks the independent GL renderer; its demo host additionally needs SDL3. GPU availability is untested.'}


def unpack(blob, destination, strip_root=True):
    """Accept only regular files/directories; reject traversal, links and special files."""
    with tarfile.open(fileobj=io.BytesIO(blob), mode='r:*') as archive:
        members = archive.getmembers()
        roots = {PurePosixPath(m.name).parts[0] for m in members if PurePosixPath(m.name).parts}
        if strip_root and len(roots) != 1:
            raise ValueError('Archive must have exactly one top-level directory')
        for member in members:
            path = PurePosixPath(member.name)
            if path.is_absolute() or '..' in path.parts or '\\' in member.name or ':' in member.name:
                raise ValueError('Unsafe archive path: ' + member.name)
            if not member.isfile() and not member.isdir():
                raise ValueError('Archive links/special files are unsupported: ' + member.name)
        for member in members:
            parts = PurePosixPath(member.name).parts[1:] if strip_root else PurePosixPath(member.name).parts
            if not parts:
                continue
            target = destination.joinpath(*parts)
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            else:
                target.parent.mkdir(parents=True, exist_ok=True)
                with archive.extractfile(member) as source, target.open('wb') as output:
                    shutil.copyfileobj(source, output)
                target.chmod(0o755 if member.mode & 0o111 else 0o644)


def snapshot(source, revision, archive=None, checksum=None):
    if source:
        revision = subprocess.check_output(['git', '-C', str(source), 'rev-parse', '--verify', (revision or 'HEAD') + '^{commit}'], text=True).strip()
        blob = subprocess.check_output(['git', '-C', str(source), 'archive', '--format=tar', '--prefix=cui/', revision])
        origin = 'local git archive (committed files only)'
    else:
        if not revision or not re.fullmatch(r'[0-9a-fA-F]{40}', revision):
            raise ValueError('Downloads require --revision with a full 40-character commit SHA')
        origin = 'https://codeload.github.com/mattmezza/cui/tar.gz/' + revision
        if archive:
            if not checksum:
                raise ValueError('Local archives require --sha256')
            blob = archive.read_bytes()
        else:
            with urllib.request.urlopen(origin, timeout=60) as response:
                blob = response.read()
    digest = hashlib.sha256(blob).hexdigest()
    if checksum and digest != checksum.lower():
        raise ValueError('Archive SHA-256 mismatch')
    return blob, {'schema': 1, 'revision': revision, 'archive_sha256': digest, 'origin': origin}


def resolve(ref):
    url = 'https://api.github.com/repos/mattmezza/cui/commits/' + quote(ref, safe='')
    request = urllib.request.Request(url, headers={'Accept': 'application/vnd.github+json', 'User-Agent': 'cui-skill'})
    with urllib.request.urlopen(request, timeout=30) as response:
        revision = json.load(response)['sha']
    if not re.fullmatch(r'[0-9a-fA-F]{40}', revision):
        raise ValueError('Upstream returned an invalid commit SHA')
    return revision


def create(args):
    destination = args.dest.absolute()
    if destination.exists():
        raise ValueError('Destination already exists; refusing to overwrite it')
    blob, lock = snapshot(args.source, args.revision, args.archive, args.sha256)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.cui-stage-', dir=destination.parent) as stage:
        staging = Path(stage) / 'project'
        staging.mkdir()
        unpack(blob, staging)
        # Validate the source snapshot before making it visible to the consumer.
        data = collect(staging)
        lock['source_digest'] = data['source_digest']
        if args.command == 'init' and args.template == 'native':
            bundle = Path(stage) / 'native.tar.gz'
            subprocess.run([sys.executable, str(staging / 'scripts/native_bundle.py'), '--output', str(bundle)], check=True, capture_output=True)
            native = Path(stage) / 'native'
            native.mkdir()
            unpack(bundle.read_bytes(), native)
            staging = native
        # Retain and hash matching lookup data even in the standalone native bundle.
        (staging / '.cui-resources.json').write_text(json.dumps(data, ensure_ascii=False) + '\n', encoding='utf-8')
        lock['files'] = {p.relative_to(staging).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                         for p in sorted(staging.rglob('*')) if p.is_file()}
        (staging / '.cui-lock.json').write_text(json.dumps(lock, indent=2) + '\n', encoding='utf-8')
        staging.rename(destination)
    print('Created ' + str(destination) + '\nPinned revision: ' + lock['revision'])
    if args.command == 'init':
        entry, guidance = TEMPLATES[args.template]
        print('Entry point: ' + entry + '\n' + guidance)
        if args.template != 'native':
            print('This is a pinned source workspace containing the existing example and its build context; adapt that example for your app.')


def verify(source):
    lock = json.loads((source / '.cui-lock.json').read_text(encoding='utf-8'))
    changed = [p for p, digest in lock['files'].items()
               if not (source / p).is_file() or hashlib.sha256((source / p).read_bytes()).hexdigest() != digest]
    return {'revision': lock['revision'], 'changed_or_missing': changed,
            'note': 'Tracks original snapshot files; application edits are reported and never overwritten.'}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    for command in ('search', 'component', 'api', 'read', 'list', 'info'):
        p = sub.add_parser(command)
        p.add_argument('--source', type=Path, help='Use matching resources from a CUI source tree or generated project')
        if command in {'search', 'component', 'api', 'read'}:
            p.add_argument('query')
        if command == 'search':
            p.add_argument('--kind', choices=['all', 'component', 'api', 'guide'], default='all')
            p.add_argument('--limit', type=int)
        if command == 'component':
            p.add_argument('--show', choices=['summary', 'all', 'api', 'usage', 'examples', 'limitations'])
        if command in {'component', 'api'}:
            p.add_argument('--full', action='store_true', help='Restore complete legacy human output')
        if command == 'api':
            p.add_argument('--declaration', action='store_true')
        if command == 'read':
            group = p.add_mutually_exclusive_group()
            group.add_argument('--headings', action='store_true')
            group.add_argument('--section', help='One exact, unique ATX heading title')
        p.add_argument('--json', action='store_true')
    p = sub.add_parser('doctor')
    p.add_argument('--profile', choices=sorted(TEMPLATES), default='portable')
    p.add_argument('--font', type=Path)
    p.add_argument('--json', action='store_true')
    for command in ('vendor', 'init'):
        p = sub.add_parser(command)
        p.add_argument('--dest', type=Path, required=True)
        p.add_argument('--revision')
        group = p.add_mutually_exclusive_group()
        group.add_argument('--source', type=Path, help='Offline Git source; archives committed revision only')
        group.add_argument('--archive', type=Path, help='Offline upstream tarball; requires --revision and --sha256')
        p.add_argument('--sha256')
        if command == 'init':
            p.add_argument('--template', choices=sorted(TEMPLATES), required=True)
    p = sub.add_parser('resolve')
    p.add_argument('--ref', default='main', help='Resolve a branch/tag to an immutable SHA for new setup')
    p = sub.add_parser('verify')
    p.add_argument('--source', type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        if args.command == 'resolve':
            print(resolve(args.ref))
            return 0
        if args.command in {'vendor', 'init'}:
            create(args)
            return 0
        if args.command == 'verify':
            result = verify(args.source)
            emit(result, True)
            return int(bool(result['changed_or_missing']))
        if args.command == 'doctor':
            result = doctor(args.profile, args.font)
            emit(result, True)
            return int(any(c['ok'] is False for c in result['checks']))
        if args.source and (args.source / '.cui-resources.json').is_file():
            # Prefer live sources for full checkouts, packaged data for native bundles.
            data = resources(args.source) if (args.source / 'docs/component-usage.json').is_file() else json.loads((args.source / '.cui-resources.json').read_text(encoding='utf-8'))
        else:
            data = resources(args.source)
        if args.command == 'search':
            limit = args.limit if args.limit is not None else (10 if args.json else 5)
            if limit < 1:
                raise ValueError('--limit must be positive')
            all_results = search(data, args.query, args.kind, sys.maxsize)
            result = all_results[:limit]
            emit(result if args.json else '\n'.join(f"{r['kind']}: {r['name']}{' (partial match)' if r['partial'] else ''}\n  {r['description']}" for r in result), args.json)
            if not args.json:
                print(f"Showing {len(result)} of {len(all_results)} {'partial' if result and result[0]['partial'] else 'complete'} matches; use --limit N or refine the query.")
            return 0 if result else 1
        if args.command == 'component':
            if args.full and args.show not in {None, 'all'}:
                raise ValueError('--full conflicts with --show other than all')
            if args.json and args.show == 'summary':
                raise ValueError('--show summary is human-only; omit it for legacy JSON')
            section = args.show or ('all' if args.json or args.full else 'summary')
            result = component(data, args.query, 'all' if section == 'summary' else section)
            if args.json:
                emit(result, True)
            elif section == 'summary':
                print('# ' + result['name'] + '\nSource digest: ' + data['source_digest'])
                emit(result['usage'], True)
                print('Canonical status, platforms, limitations and features (unabridged):')
                emit(result['status'], True)
                c = next(c for c in data['components'] if c['name'] == result['name'])
                handlers = c.get('handler_types')
                print('Indexed handler types (not exhaustive): ' +
                      (json.dumps(handlers, ensure_ascii=False) if handlers else
                       'not indexed; inspect API/owning headers'))
                resource_index(data, [c['public_header']] + c.get('additional_public_headers', []), args.source)
                print('Incomplete contract; use --show api, --show examples and --full for progressive detail.')
            elif section == 'examples':
                print('Example/guide index; read selected resources in full. Shared guides are not standalone programs.')
                resource_index(data, [e['path'] for e in result['examples']], args.source)
                print('Sample reference: ' + str(result['sample']))
                print('Test references: ' + json.dumps(result['tests'], ensure_ascii=False))
            elif section == 'api':
                c = next(c for c in data['components'] if c['name'] == result['name'])
                declaration_index(data, result['api'], [result['header']['path']] + c.get('additional_public_headers', []) + [m['header'] for m in result['api']] + c['docs'], args.source)
            else:
                print('# ' + result['name'] + '\nSource digest: ' + data['source_digest'])
                for key, value in result.items():
                    if key in {'name', 'source_digest'}:
                        continue
                    print('\n## ' + key)
                    if key == 'header':
                        print(value['path'] + '\n\n' + value['text'])
                    elif key == 'examples':
                        for example in value:
                            print('\n### ' + example['path'] + '\n\n' + example['text'])
                    else:
                        print(json.dumps(value, indent=2, ensure_ascii=False))
        elif args.command == 'api':
            if args.full and args.declaration:
                raise ValueError('--full conflicts with --declaration')
            if args.json and args.declaration:
                raise ValueError('--declaration is human-only')
            matches = [s for s in data['symbols'] if s['name'] == args.query]
            # Types, enums and macros are also discoverable without inventing declarations.
            headers = {p: text for p, text in data['documents'].items() if p.startswith('include/') and re.search(r'\b' + re.escape(args.query) + r'\b', text)}
            if not headers:
                raise ValueError('Unknown public symbol; use search.')
            if args.json:
                emit({'functions': matches, 'matching_headers': headers}, True)
            elif not args.full:
                guides = [p for c in data['components'] if args.query in c['api_symbols'] for p in c['docs']]
                declaration_index(data, matches, list(headers) + guides, args.source)
            else:
                for match in matches:
                    print(match['declaration'])
                for path, text in headers.items():
                    print('\n# ' + path + '\n\n' + text)
        elif args.command == 'read':
            if args.query not in data['documents']:
                raise ValueError('Unknown resource path; use search or info --json.')
            if args.headings or args.section is not None:
                if args.json:
                    raise ValueError('--headings/--section are human-only; JSON read remains the full document')
                if not args.query.endswith('.md'):
                    raise ValueError('--headings/--section require a Markdown resource')
                read_partial(data, args.query, args.section, args.source)
            else:
                emit(data['documents'][args.query], args.json)
        elif args.command == 'list':
            result = [{'name': c['name'], 'category': c['category'], 'status': c['status']} for c in data['components']]
            emit(result if args.json else '\n'.join(f"{r['name']} [{r['category']}; {r['status']}]" for r in result), args.json)
        elif args.command == 'info':
            emit({k: v for k, v in data.items() if k in {'schema', 'source_digest', 'source_hashes', 'lifecycle'}}, True)
        return 0
    except (ValueError, OSError, KeyError, tarfile.TarError, subprocess.SubprocessError) as error:
        print('cui: ' + str(error), file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
