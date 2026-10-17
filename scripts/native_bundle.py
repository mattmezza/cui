#!/usr/bin/env python3
"""Generate a deterministic, offline native source distribution."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import tarfile

ROOT = Path(__file__).resolve().parents[1]


def bundle_files():
    metadata = json.loads((ROOT / 'cmake/sources.json').read_text())
    portable = list(dict.fromkeys(p for paths in metadata['modules'].values() for p in paths))
    groups = {'portable': portable, **metadata['native']}
    # Software's private helpers are used by SDL3; its renderer is optional.
    groups.pop('software')
    files = {}

    def add(path, destination=None):
        files[destination or ('vendor/' + path)] = (ROOT / path).read_bytes()

    for paths in groups.values():
        for path in paths:
            add(path)
            # Preserve private/generated headers alongside their source files.
            for sibling in sorted((ROOT / path).parent.iterdir()):
                if sibling.suffix in {'.h', '.inc', '.json'}:
                    add(sibling.relative_to(ROOT).as_posix())
    for path in sorted((ROOT / 'include/cui').glob('*.h')):
        add(path.relative_to(ROOT).as_posix())
    for path in sorted((ROOT / 'backends/software').glob('*.h')):
        add(path.relative_to(ROOT).as_posix())
    for directory in ('clay', 'nanosvg', 'sheenbidi', 'gpui_icons', 'gpui_themes'):
        for path in sorted((ROOT / 'third_party' / directory).rglob('*')):
            if path.is_file():
                add(path.relative_to(ROOT).as_posix())
    add('third_party/unicode/17.0.0/LICENSE.txt')
    add('third_party/unicode/17.0.0/manifest.json')
    for path in sorted((ROOT / 'third_party/inter').iterdir()):
        if path.is_file():
            add(path.relative_to(ROOT).as_posix(), 'assets/fonts/' + path.name)
    add('LICENSE', 'LICENSE')
    add('docs/gpui-parity.json', 'component-manifest.json')
    add('cmake/sources.json', 'vendor/sources.json')
    # Preserve upstream provenance and notices for every dependency used here.
    dependencies = []
    for record in json.loads((ROOT / 'third_party/dependencies.json').read_text()):
        selected = {p: digest for p, digest in record.get('sha256', {}).items()
                    if 'vendor/' + p in files or
                    (p.startswith('third_party/inter/') and 'assets/fonts/' + Path(p).name in files)}
        if selected:
            for path, expected in selected.items():
                if hashlib.sha256((ROOT / path).read_bytes()).hexdigest() != expected:
                    raise ValueError(f'Pinned dependency changed: {path}')
            add(record['license_file'])
            dependencies.append({**record, 'sha256': selected})
    files['upstream-dependencies.json'] = (json.dumps(dependencies, indent=2) + '\n').encode()
    for name in ('Makefile', 'README.md', 'main.c', 'app.c', 'mail.c'):
        add('examples/native_bundle/' + name, name)
    lines = ['# Generated from cmake/sources.json; do not edit.']
    for name, paths in groups.items():
        lines.append(name.upper() + '_SOURCES := ' + ' '.join('vendor/' + p for p in paths))
    files['sources.mk'] = ('\n'.join(lines) + '\n').encode()
    manifest = {'format': 1, 'files': {p: hashlib.sha256(data).hexdigest()
                                     for p, data in sorted(files.items())}}
    files['bundle-manifest.json'] = (json.dumps(manifest, indent=2) + '\n').encode()
    return files


def generate(output):
    files = bundle_files()
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('wb') as stream:
        # Fixed metadata, including gzip's filename and clock, makes bytes stable.
        with gzip.GzipFile(filename='', fileobj=stream, mode='wb', mtime=0) as compressed:
            with tarfile.open(fileobj=compressed, mode='w', format=tarfile.USTAR_FORMAT) as archive:
                for name, data in sorted(files.items()):
                    info = tarfile.TarInfo('cui-native/' + name)
                    info.size = len(data)
                    info.mode = 0o644
                    archive.addfile(info, io.BytesIO(data))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/cui-native.tar.gz')
    args = parser.parse_args()
    generate(args.output)
    print(args.output)
