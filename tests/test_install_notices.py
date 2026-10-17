#!/usr/bin/env python3
"""Check actual CMake installation includes Core notices and its pin receipt.

Run after building installable targets; no native application is launched.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
CORE_NOTICES = {
    'CUI-LICENSE': 'LICENSE',
    'CLAY-LICENSE': 'third_party/clay/LICENSE',
    'NANOSVG-LICENSE': 'third_party/nanosvg/LICENSE',
    'GPUI-ICONS-LICENSE': 'third_party/gpui_icons/LICENSE',
    'GPUI-THEMES-LICENSE': 'third_party/gpui_themes/LICENSE',
    'UNICODE-LICENSE': 'third_party/unicode/17.0.0/LICENSE.txt',
}


def verify(prefix, source):
    notices = prefix / 'share/cui/licenses'
    receipt = source / 'third_party/dependencies.json'
    installed_receipt = notices / 'DEPENDENCIES.json'
    if not installed_receipt.is_file() or installed_receipt.read_bytes() != receipt.read_bytes():
        raise RuntimeError('installed dependency receipt is missing or differs from canonical pins')
    pins = {}
    for dependency in json.loads(receipt.read_text()):
        pins.update(dependency['sha256'])
    expected = dict(CORE_NOTICES)
    for library, installed, canonical in (
        ('cui_bidi', 'SHEENBIDI-LICENSE', 'third_party/sheenbidi/LICENSE'),
        ('cui_sokol', 'SOKOL-LICENSE', 'third_party/sokol/LICENSE'),
    ):
        if any((prefix / 'lib' / name).is_file() for name in ('lib' + library + '.a', library + '.lib')):
            expected[installed] = canonical
    for installed, canonical in expected.items():
        file = notices / installed
        source_bytes = (source / canonical).read_bytes()
        if not file.is_file() or file.read_bytes() != source_bytes:
            raise RuntimeError('installed notice is missing or differs: ' + installed)
        if canonical != 'LICENSE' and hashlib.sha256(file.read_bytes()).hexdigest() != pins.get(canonical):
            raise RuntimeError('installed notice does not match pinned license hash: ' + installed)
    return len(expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--config', help='built configuration for multi-config generators')
    args = parser.parse_args()
    source = args.source_root.resolve(strict=True)
    build = args.build_dir.resolve(strict=True)
    environment = dict(os.environ)
    environment.pop('DESTDIR', None)
    with tempfile.TemporaryDirectory(prefix='cui-installed-notices-') as temporary:
        prefix = Path(temporary) / 'prefix'
        command = ['cmake', '--install', str(build), '--prefix', str(prefix)]
        if args.config:
            command += ['--config', args.config]
        result = subprocess.run(command, env=environment, text=True, capture_output=True)
        if result.returncode:
            raise RuntimeError('CMake installation failed:\n' + result.stdout + result.stderr)
        count = verify(prefix, source)
        # Prove missing/corrupt installed bytes are rejected, using only our
        # disposable install tree, never canonical source or the build tree.
        file = prefix / 'share/cui/licenses/NANOSVG-LICENSE'
        original = file.read_bytes()
        file.write_bytes(original + b'corrupt')
        try:
            verify(prefix, source)
        except RuntimeError:
            pass
        else:
            raise RuntimeError('notice verification accepted corrupted installed bytes')
        file.write_bytes(original)
        receipt = prefix / 'share/cui/licenses/DEPENDENCIES.json'
        receipt.unlink()
        try:
            verify(prefix, source)
        except RuntimeError:
            pass
        else:
            raise RuntimeError('notice verification accepted an omitted dependency receipt')
    print(f'PASS: actual temporary CMake install, {count} notices match canonical bytes/pins; receipt and corruption guards passed')


if __name__ == '__main__':
    main()
