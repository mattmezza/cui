#!/usr/bin/env python3
"""Build pinned optional AccessKit into a private cache; never install globally."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tarfile
import urllib.request

ROOT = Path(__file__).resolve().parents[1]


def write_changed(path, data):
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cache', type=Path, default=Path.home() / '.cache/cui-accesskit')
    args = p.parse_args()
    pin = json.loads((ROOT / 'third_party/accesskit/pin.json').read_text())
    cache = args.cache.resolve()
    cache.mkdir(parents=True, exist_ok=True)
    archive = cache / (pin['commit'] + '.tar.gz')
    if not archive.exists():
        data = urllib.request.urlopen(pin['archive_url'], timeout=60).read()
        if hashlib.sha256(data).hexdigest() != pin['archive_sha256']:
            raise RuntimeError('AccessKit archive checksum mismatch')
        archive.write_bytes(data)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != pin['archive_sha256']:
        raise RuntimeError('Cached AccessKit archive checksum mismatch')
    source = cache / ('accesskit-c-' + pin['commit'])
    if not source.exists():
        with tarfile.open(archive) as package:
            package.extractall(cache, filter='data')
    lock = source / 'Cargo.lock'
    if hashlib.sha256(lock.read_bytes()).hexdigest() != pin['cargo_lock_sha256']:
        raise RuntimeError('AccessKit Cargo.lock checksum mismatch')
    # Verify every archived source file, not only the archive sitting beside it.
    # The one declared entry-point extension is the only permitted cache change.
    with tarfile.open(archive) as package:
        prefix = 'accesskit-c-' + pin['commit'] + '/'
        for member in package.getmembers():
            if not member.isfile():
                continue
            relative = member.name.removeprefix(prefix)
            expected = package.extractfile(member).read()
            actual = (source / relative).read_bytes()
            allowed = (expected, expected + b'\nmod cui_lifetime;\n') if relative == 'src/lib.rs' else (expected,)
            if actual not in allowed:
                raise RuntimeError('Cached AccessKit source differs: ' + relative)
    # Refresh the pristine entry point from the verified archive before applying
    # our one-module ownership extension. Never silently accept cache source drift.
    with tarfile.open(archive) as package:
        original = package.extractfile('accesskit-c-' + pin['commit'] + '/src/lib.rs').read()
    shim = (ROOT / 'backends/accesskit/cui_lifetime.rs').read_bytes()
    extension = json.loads((ROOT / 'backends/accesskit/extension.json').read_text())
    if hashlib.sha256(shim).hexdigest() != extension['shim_sha256']:
        raise RuntimeError('CUI ownership shim checksum mismatch')
    write_changed(source / 'src/lib.rs', original + b'\nmod cui_lifetime;\n')
    write_changed(source / 'src/cui_lifetime.rs', shim)
    (cache / 'cui-extension.json').write_text(json.dumps({
        'shim_sha256': hashlib.sha256(shim).hexdigest(),
        'lib_rs_sha256': hashlib.sha256(original + b'\nmod cui_lifetime;\n').hexdigest(),
        'entry_point_append': 'mod cui_lifetime;',
    }, indent=2) + '\n')
    env = dict(os.environ)
    env['CARGO_HOME'] = str(cache / 'cargo')
    subprocess.run(['cargo', 'build', '--release', '--locked', '--manifest-path',
                    str(source / 'Cargo.toml'), '--target-dir', str(cache / 'target')],
                   env=env, check=True)
    print(json.dumps({'header': str(source / 'include/accesskit.h'),
                      'static_library': str(cache / 'target/release/libaccesskit.a')}))


if __name__ == '__main__':
    main()
