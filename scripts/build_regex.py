#!/usr/bin/env python3
"""Build the optional source-pinned regex C ABI in a private cache (no global install)."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--archive-dir', type=Path, help='Use only local verified crate archives')
    parser.add_argument('--toolchain', type=Path, required=True,
                        help='Existing complete official Rust1.85.1 prefix; never installed by this script')
    parser.add_argument('--target', choices=('native', 'wasm32-wasip1'), default='native')
    args = parser.parse_args()
    cache = args.cache.resolve()
    cache.mkdir(parents=True, exist_ok=True)
    spec = ROOT / 'backends/regex'
    pin = json.loads((spec / 'pin.json').read_text())
    if hashlib.sha256((spec / 'cache.patch').read_bytes()).hexdigest() != pin['cache_patch_sha256']:
        raise RuntimeError('Private cache patch checksum mismatch')
    if hashlib.sha256((spec / 'Cargo.lock').read_bytes()).hexdigest() != pin['cargo_lock_sha256']:
        raise RuntimeError('Pinned Cargo.lock checksum mismatch')
    rustc = args.toolchain.resolve() / 'bin/rustc'
    cargo = args.toolchain.resolve() / 'bin/cargo'
    if subprocess.check_output([rustc, '--version'], text=True).split()[:2] != ['rustc', '1.85.1']:
        raise RuntimeError('Exact Rust1.85.1 is required')
    if ('commit-hash: ' + pin['rustc_commit']) not in subprocess.check_output([rustc, '-Vv'], text=True):
        raise RuntimeError('Exact official Rust1.85.1 compiler commit is required')
    # Validate every archive before extracting any dependency.
    archives = []
    for crate in pin['crates']:
        filename = f"{crate['name']}-{crate['version']}.crate"
        archive = cache / filename
        if args.archive_dir:
            data = (args.archive_dir / filename).read_bytes()
        elif archive.exists():
            data = archive.read_bytes()
        else:
            data = urllib.request.urlopen(crate['url'], timeout=60).read()
        if hashlib.sha256(data).hexdigest() != crate['sha256']:
            raise RuntimeError('Crate checksum mismatch: ' + filename)
        if not archive.exists() or archive.read_bytes() != data:
            archive.write_bytes(data)
        archives.append((crate, archive))
    source = cache / 'source'
    vendor = source / 'vendor'
    vendor.mkdir(parents=True, exist_ok=True)
    for crate, archive in archives:
        prefix = f"{crate['name']}-{crate['version']}"
        with tarfile.open(archive) as package:
            for member in package.getmembers():
                path = Path(member.name)
                if path.is_absolute() or '..' in path.parts or path.parts[0] != prefix:
                    raise RuntimeError('Unsafe archive path')
                if not member.isfile() and not member.isdir():
                    raise RuntimeError('Unsupported archive entry')
            # Restore this private cache from verified pristine sources before patching.
            package.extractall(vendor, filter='data')
    subprocess.run(['patch', '--batch', '-p1', '-i', str(spec / 'cache.patch')],
                   cwd=vendor / 'regex-automata-0.4.18', check=True)
    (source / 'src').mkdir(exist_ok=True)
    for filename in ('Cargo.toml', 'Cargo.lock'):
        shutil.copyfile(spec / filename, source / filename)
    for filename in ('lib.rs', 'tests.rs'):
        shutil.copyfile(spec / 'src' / filename, source / 'src' / filename)
    env = dict(os.environ)
    env['RUSTC'] = str(rustc)
    env['CARGO_HOME'] = str(cache / 'cargo')
    command = [str(cargo), 'rustc', '--offline', '--locked', '--release', '--lib',
               '--manifest-path', str(source / 'Cargo.toml'), '--target-dir', str(cache / 'target')]
    if args.target != 'native':
        command += ['--target', args.target]
    command += ['--', '--print', 'native-static-libs']
    subprocess.run(command, env=env, check=True)
    directory = cache / 'target'
    if args.target != 'native':
        directory /= args.target
    library = directory / 'release/libcui_regex.a'
    print(json.dumps({'library': str(library), 'header': str(ROOT / 'include/cui/cui_regex.h'),
                      'sha256': hashlib.sha256(library.read_bytes()).hexdigest(),
                      'target': args.target, 'rust': '1.85.1'}))


if __name__ == '__main__':
    main()
