#!/usr/bin/env python3
"""Validate an installed macOS starter and capture it using SDL dummy/software."""
import argparse
import os
from pathlib import Path
import plistlib
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('app', type=Path)
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('execution requires macOS; use test_macos_bundle.py for portable contract checks')
    app = args.app.resolve()
    with (app / 'Contents/Info.plist').open('rb') as stream:
        info = plistlib.load(stream)
    executable = app / 'Contents/MacOS' / info['CFBundleExecutable']
    for asset in ('Inter-Regular.otf', 'Inter-Bold.otf', 'LICENSE.txt', 'manifest.json'):
        if not (app / 'Contents/Resources/inter' / asset).is_file():
            raise RuntimeError(f'missing bundle resource: {asset}')
    # Reject absolute build-prefix references even when that prefix still exists.
    for binary in {executable, *app.rglob('*.dylib')}:
        listing = subprocess.check_output(['otool', '-L', str(binary)], text=True)
        for line in listing.splitlines()[1:]:
            dependency = line.strip().split(' (', 1)[0]
            if dependency.startswith('/') and not dependency.startswith(('/System/Library/', '/usr/lib/')):
                raise RuntimeError(f'external dependency in {binary}: {dependency}')
    environment = os.environ.copy()
    # Prove the deployed app does not rely on loader overrides from the build.
    for key in list(environment):
        if key.startswith('DYLD_'):
            environment.pop(key)
    environment.update(SDL_VIDEODRIVER='dummy', SDL_RENDER_DRIVER='software')
    with tempfile.TemporaryDirectory(prefix='cui-macos-app-') as directory:
        capture = Path(directory) / 'starter.bmp'
        subprocess.run([str(executable), '--frames', '3', '--capture', str(capture)],
                       cwd=directory, env=environment, check=True, timeout=30)
        if not capture.is_file() or capture.read_bytes()[:2] != b'BM':
            raise RuntimeError('starter did not produce a BMP capture')
    print('Installed bundle resources and headless starter capture passed; desktop/GPU checks remain separate.')


if __name__ == '__main__':
    main()
