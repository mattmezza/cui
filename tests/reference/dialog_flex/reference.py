#!/usr/bin/env python3
"""Prepare a bounded pinned flex oracle, then build/run OFFLINE by explicit command.

No CUI runtime dependency. The prepare action alone may use the network, and only when
--network is supplied. Cargo uses local paths, unchanged verified Rust sources,
and transparent minimal manifests; no registry resolver/network is needed.
"""
import argparse
import difflib
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import tarfile
import urllib.request

PINS = {
    'taffy': ('0.13.0', 'c034e05f6ee85a12daa63863c2245797715075c70649947aa0da54f3f2ab1d0f'),
    'arrayvec': ('0.7.8', 'd3fb67a6e08acf24fdeccbac2cb6ac4305825bd1f117462e0e6f2f193345ad56'),
    'slotmap': ('1.1.1', 'bdd58c3c93c3d278ca835519292445cb4b0d4dc59ccfdf7ceadaab3f8aeb4038'),
    'version_check': ('0.9.5', '0b928f33d975fc6ad9f86c8f283853ad26bdd5b10b7f1542aa2fa15e2289105a'),
}
FEATURES = ['std', 'flexbox', 'taffy_tree', 'content_size']
HERE = Path(__file__).resolve().parent
SOURCE_PIN = '87d10ae5e1299d1be18670a37570c5402f952e34'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def source_digest(directory):
    result = hashlib.sha256()
    for file in sorted(directory.rglob('*.rs'), key=lambda p: p.relative_to(directory).as_posix()):
        result.update(file.relative_to(directory).as_posix().encode() + b'\0' + file.read_bytes())
    return result.hexdigest()


def archive_source_digest(data):
    result = hashlib.sha256()
    with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as archive:
        members = sorted((m for m in archive.getmembers() if m.isfile() and m.name.endswith('.rs')),
                         key=lambda m: m.name)
        for member in members:
            name = member.name.split('/', 1)[1]
            result.update(name.encode() + b'\0' + archive.extractfile(member).read())
    return result.hexdigest()


def manifest(name, version):
    edition = '2018' if name in ('slotmap', 'arrayvec') else '2021' if name == 'taffy' else '2015'
    text = f'[package]\nname="{name}"\nversion="{version}"\nedition="{edition}"\n'
    text += '[lib]\npath="src/lib.rs"\n'
    if name == 'taffy':
        text += '[dependencies]\narrayvec={path="../arrayvec-0.7.8",default-features=false}\n'
        text += 'slotmap={path="../slotmap-1.1.1",default-features=false,optional=true}\n'
        text += '[features]\nstd=["slotmap?/std"]\nflexbox=[]\ncontent_size=[]\ntaffy_tree=["dep:slotmap"]\n'
        for feature in ['default', 'document-features', 'alloc', 'grid', 'block_layout', 'float_layout', 'calc', 'detailed_layout_info',
                        'strict_provenance', 'serde', 'parse', 'parse_faster', 'debug', 'profile']:
            text += feature + '=[]\n'
    elif name == 'slotmap':
        text += '[build-dependencies]\nversion_check={path="../version_check-0.9.5"}\n'
        text += '[features]\ndefault=["std"]\nstd=[]\nunstable=[]\nserde=[]\n'
    elif name == 'arrayvec':
        text += '[features]\ndefault=["std"]\nstd=[]\nserde=[]\nzeroize=[]\nborsh=[]\n'
    return text


def prepare(cache, network):
    cache.mkdir(parents=True, exist_ok=True)
    provenance = {'source_pin': SOURCE_PIN, 'features': FEATURES, 'crates': {}}
    for name, (version, sha) in PINS.items():
        archive_path = cache / f'{name}-{version}.crate'
        if not archive_path.exists():
            cached = list((Path.home() / '.cargo/registry/cache').glob('*/' + archive_path.name))
            if cached:
                data = cached[0].read_bytes()
            elif network:
                url = f'https://static.crates.io/crates/{name}/{name}-{version}.crate'
                data = urllib.request.urlopen(url, timeout=30).read(5 * 1024 * 1024 + 1)
            else:
                raise SystemExit('Missing verified archive; use separate prepare --network step')
            if len(data) > 5 * 1024 * 1024 or digest(data) != sha:
                raise SystemExit('Archive cap/checksum mismatch: ' + name)
            archive_path.write_bytes(data)
        data = archive_path.read_bytes()
        if digest(data) != sha:
            raise SystemExit('Archive checksum mismatch: ' + name)
        directory = cache / 'source' / f'{name}-{version}'
        with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as archive:
            archive.extractall(cache / 'source', filter='data')
        before = source_digest(directory)
        original = (directory / 'Cargo.toml').read_text()
        reduced = manifest(name, version)
        (directory / 'Cargo.toml').write_text(reduced)
        if source_digest(directory) != before:
            raise SystemExit('Engine Rust source changed')
        delta = ''.join(difflib.unified_diff(original.splitlines(True), reduced.splitlines(True),
                                           fromfile='upstream/Cargo.toml', tofile='local/Cargo.toml'))
        # Hash and write identical UTF-8/LF bytes on Windows as well as POSIX.
        (directory / 'MANIFEST_DELTA.patch').write_bytes(delta.encode('utf-8'))
        provenance['crates'][name] = {'version': version, 'archive_sha256': sha,
                                      'rust_source_sha256': before,
                                      'manifest_delta_sha256': digest(delta.encode())}
    driver = cache / 'driver'
    (driver / 'src').mkdir(parents=True, exist_ok=True)
    (driver / 'src/main.rs').write_bytes((HERE / 'main.rs').read_bytes())
    (driver / 'Cargo.toml').write_text('''[package]
name="dialog-flex-reference"
version="0.0.0"
edition="2021"
[dependencies]
taffy={path="../source/taffy-0.13.0",default-features=false,features=["std","flexbox","taffy_tree","content_size"]}
[profile.release]
debug=0
opt-level=1
incremental=false
codegen-units=1
''')
    provenance['driver_sha256'] = digest((HERE / 'main.rs').read_bytes())
    provenance['helper_sha256'] = digest(Path(__file__).read_bytes())
    source_bytes = sum(p.stat().st_size for p in (cache / 'source').rglob('*') if p.is_file())
    if source_bytes > 5 * 1024 * 1024:
        raise SystemExit('Reference source budget exceeds5MiB')
    provenance['source_bytes'] = source_bytes
    (cache / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')


def verify(cache):
    p = json.loads((cache / 'provenance.json').read_text())
    if p['source_pin'] != SOURCE_PIN or p['features'] != FEATURES:
        raise SystemExit('Reference feature/source pin mismatch')
    if p['helper_sha256'] != digest(Path(__file__).read_bytes()):
        raise SystemExit('Reference helper changed; prepare again before build')
    if p['driver_sha256'] != digest((HERE / 'main.rs').read_bytes()):
        raise SystemExit('Reference driver changed; prepare again before build')
    for name, (version, sha) in PINS.items():
        entry = p['crates'][name]
        source = cache / 'source' / f'{name}-{version}'
        if (entry['version'] != version or entry['archive_sha256'] != sha
                or digest((cache / f'{name}-{version}.crate').read_bytes()) != sha
                or source_digest(source) != entry['rust_source_sha256']
                or source_digest(source) != archive_source_digest(
                    (cache / f'{name}-{version}.crate').read_bytes())
                or digest((source / 'MANIFEST_DELTA.patch').read_bytes())
                    != entry['manifest_delta_sha256']
                or (source / 'Cargo.toml').read_text() != manifest(name, version)):
            raise SystemExit('Reference source/manifest authority changed: ' + name)
    if (cache / 'driver/src/main.rs').read_bytes() != (HERE / 'main.rs').read_bytes():
        raise SystemExit('Copied driver stale')
    return p


def artifact_size(cache):
    return sum(p.stat().st_size for p in (cache / 'target').rglob('*') if p.is_file())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', required=True, type=Path)
    parser.add_argument('--network', action='store_true')
    parser.add_argument('action', choices=['prepare', 'build', 'capture', 'verify'])
    args = parser.parse_args()
    cache = args.cache.resolve()
    if args.action == 'prepare':
        prepare(cache, args.network)
        return
    if args.network:
        parser.error('--network is only valid for prepare')
    provenance = verify(cache)
    if args.action == 'build':
        env = dict(os.environ, CARGO_TARGET_DIR=str(cache / 'target'), CARGO_INCREMENTAL='0')
        subprocess.run(['cargo', 'build', '--manifest-path', str(cache / 'driver/Cargo.toml'),
                        '--offline', '--release', '-j1'], env=env, check=True)
        if artifact_size(cache) > 100 * 1024 * 1024:
            raise SystemExit('Reference compiled-artifact budget exceeds100MiB')
        (cache / 'build-receipt.json').write_text(json.dumps({
            'provenance_sha256': digest((cache / 'provenance.json').read_bytes()),
            'binary_sha256': digest((cache / 'target/release/dialog-flex-reference').read_bytes()),
        }, indent=2) + '\n')
    elif args.action == 'capture':
        built = json.loads((cache / 'build-receipt.json').read_text())
        if (built['provenance_sha256'] != digest((cache / 'provenance.json').read_bytes())
                or built['binary_sha256'] != digest(
                    (cache / 'target/release/dialog-flex-reference').read_bytes())):
            raise SystemExit('Reference build stale or binary changed')
        output = subprocess.check_output([str(cache / 'target/release/dialog-flex-reference')])
        (cache / 'reference.csv').write_bytes(output)
        provenance['output_sha256'] = digest(output)
        provenance['compiled_artifact_bytes'] = artifact_size(cache)
        provenance['rustc'] = subprocess.check_output(['rustc', '--version'], text=True).strip()
        (cache / 'receipt.json').write_text(json.dumps(provenance, indent=2) + '\n')
        print(output.decode(), end='')


if __name__ == '__main__':
    main()
