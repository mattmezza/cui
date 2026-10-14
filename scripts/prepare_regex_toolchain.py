#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify and install pinned optional Rust tooling into a caller-owned private cache."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import urllib.request
ROOT = Path(__file__).resolve().parents[1]
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cache', type=Path, required=True)
    p.add_argument('--archive-dir', type=Path)
    a = p.parse_args()
    cache = a.cache.resolve()
    cache.mkdir(parents=True, exist_ok=True)
    pin = json.loads((ROOT / 'backends/regex/pin.json').read_text())['toolchain']
    items = [('channel.toml', f"https://static.rust-lang.org/dist/{pin['date']}/channel-rust-1.85.1.toml", pin['manifest_sha256'])]
    items += [(name + '.tar.xz', spec['url'], spec['sha256']) for name, spec in pin['artifacts'].items()]
    # Validate every input before extracting or executing any installer.
    for filename, url, digest in items:
        path = cache / filename
        if a.archive_dir:
            data = (a.archive_dir / filename).read_bytes()
        elif path.exists():
            data = path.read_bytes()
        else:
            data = urllib.request.urlopen(url, timeout=60).read()
        if hashlib.sha256(data).hexdigest() != digest:
            raise RuntimeError('Pinned Rust checksum mismatch: ' + filename)
        if not path.exists() or path.read_bytes() != data:
            path.write_bytes(data)
    prefix = cache / 'rust-1.85.1'
    for name in ['host', 'wasi']:
        extract = cache / (name + '-verified')
        extract.mkdir(exist_ok=True)
        with tarfile.open(cache / (name + '.tar.xz')) as archive:
            entries = archive.getmembers()
            roots = set()
            for entry in entries:
                path = Path(entry.name)
                if path.is_absolute() or '..' in path.parts or not path.parts:
                    raise RuntimeError('Unsafe Rust archive path')
                roots.add(path.parts[0])
            if len(roots) != 1:
                raise RuntimeError('Unexpected Rust archive roots')
            archive.extractall(extract, filter='data')
        installer = extract / roots.pop() / 'install.sh'
        command = ['sh', str(installer), '--prefix=' + str(prefix), '--disable-ldconfig']
        if name == 'host':
            command += ['--components=rustc,cargo,rust-std-x86_64-unknown-linux-gnu']
        subprocess.run(command, check=True)
    version = subprocess.check_output([prefix / 'bin/rustc', '--version'], text=True)
    if version.split()[:2] != ['rustc', '1.85.1']:
        raise RuntimeError('Installed Rust version mismatch')
    (cache / 'receipt.json').write_text(json.dumps({'pin': pin, 'prefix': str(prefix), 'version': version.strip()}, indent=2) + '\n')
    print(prefix)
if __name__ == '__main__': main()
