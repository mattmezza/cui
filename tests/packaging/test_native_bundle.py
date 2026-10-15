#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Verify offline native source bundles, relocation, incremental builds and rendering."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]


def run(command, *, cwd=None, env=None, success=True, timeout=180):
    result = subprocess.run([str(item) for item in command], cwd=cwd, env=env,
                            text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, timeout=timeout)
    assert (result.returncode == 0) == success, f'{command}: exit {result.returncode}\n{result.stdout}'
    return result.stdout


def extract_and_check(archive, destination):
    with tarfile.open(archive, 'r:gz') as package:
        members = package.getmembers()
        assert members
        for member in members:
            path = Path(member.name)
            assert not path.is_absolute() and '..' not in path.parts, member.name
            assert path.parts[0] == 'cui-native', member.name
            assert member.isfile() or member.isdir(), member.name
        # Only validated relative regular files/directories reach extraction.
        # Avoid the filter argument added in newer Python security releases.
        package.extractall(destination)
    bundle = destination / 'cui-native'
    for name in ('Makefile', 'main.c', 'README.md', 'sources.mk',
                 'assets/fonts/Inter-Regular.otf', 'assets/fonts/Inter-Bold.otf'):
        assert (bundle / name).is_file(), name
    for name in ('include', 'src', 'backends', 'third_party'):
        assert (bundle / 'vendor' / name).is_dir(), name
    manifest = json.loads((bundle / 'bundle-manifest.json').read_text())
    files = manifest['files']
    actual = {p.relative_to(bundle).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in bundle.rglob('*') if p.is_file() and p.name != 'bundle-manifest.json'}
    assert files == actual, 'manifest must cover every payload file with its SHA-256'
    return bundle


def build_environment():
    env = dict(os.environ)
    # Preserve external SDL/FreeType/HarfBuzz visibility, removing CUI-specific paths.
    for variable in ('PKG_CONFIG_PATH', 'CMAKE_PREFIX_PATH', 'CPATH', 'C_INCLUDE_PATH',
                     'LIBRARY_PATH', 'LD_LIBRARY_PATH'):
        if variable in env:
            env[variable] = os.pathsep.join(p for p in env[variable].split(os.pathsep)
                                          if p and 'cui' not in p.lower())
    env.pop('PKG_CONFIG_LIBDIR', None)
    env['SDL_VIDEODRIVER'] = 'x11'
    env['SDL_RENDER_DRIVER'] = 'software'
    env['LIBGL_ALWAYS_SOFTWARE'] = '1'
    env.pop('WAYLAND_DISPLAY', None)
    return env


def snapshot(paths):
    return {p: p.stat().st_mtime_ns for p in paths}


def useful_bmp(path):
    data = path.read_bytes()
    assert len(data) > 54 and data[:2] == b'BM', 'capture is not a BMP'
    offset = struct.unpack_from('<I', data, 10)[0]
    width, height = struct.unpack_from('<ii', data, 18)
    assert width >= 100 and abs(height) >= 100
    pixels = data[offset:]
    assert len(pixels) >= width * abs(height) * 3
    assert len(set(pixels)) > 16, 'capture has no useful rendered variation'


def native_checks(bundle, work):
    env = build_environment()
    make = ['make', '-j2', 'CFLAGS=-O0 -g -std=c11 -Wall -Wextra -Wpedantic -Werror']
    fake = work / 'missing-pkg-config'
    fake.write_text('#!/bin/sh\necho "test: external prerequisite unavailable" >&2\nexit 1\n')
    fake.chmod(0o755)
    output = run([*make, f'PKG_CONFIG={fake}'], cwd=bundle, env=env, success=False)
    assert all(name in output for name in ('pkg-config', 'SDL3', 'FreeType', 'HarfBuzz')), output
    assert not list((bundle / 'build').rglob('*.o')), 'prerequisites should fail before compiling'
    run(make, cwd=bundle, env=env)
    executable = bundle / 'build/native_starter'
    app = bundle / 'build/main.o'
    vendor = list((bundle / 'build/vendor').rglob('*.o')) + list((bundle / 'build/vendor').rglob('*.a'))
    assert executable.is_file() and app.is_file() and vendor
    before = snapshot([app, executable, *vendor])
    run(make, cwd=bundle, env=env)
    assert snapshot(before) == before, 'no-op make rebuilt files'
    time.sleep(1.1)
    with (bundle / 'main.c').open('a') as stream:
        stream.write('\n/* incremental application rebuild test */\n')
    run(make, cwd=bundle, env=env)
    after = snapshot(before)
    assert after[app] > before[app] and after[executable] > before[executable]
    assert all(after[p] == before[p] for p in vendor), 'application edit rebuilt vendored CUI'
    # Exercise compiler-produced dependency files, including transitive public headers.
    dependencies = list((bundle / 'build/vendor').rglob('*.d'))
    assert dependencies, 'vendor compilations must emit dependency files'
    dependent = None
    for dep in dependencies:
        words = dep.read_text().replace('\\\n', ' ').split()
        headers = [bundle / word for word in words[1:] if word.endswith('.h')]
        headers = [p for p in headers if p.is_file() and 'vendor' in p.parts]
        obj = dep.with_suffix('.o')
        if headers and obj.is_file():
            dependent = obj, headers[0]
            break
    assert dependent, 'no vendor header dependency found'
    obj, header = dependent
    before_header = obj.stat().st_mtime_ns
    time.sleep(1.1)
    with header.open('a') as stream:
        stream.write('\n/* dependency rebuild test */\n')
    run(make, cwd=bundle, env=env)
    assert obj.stat().st_mtime_ns > before_header, 'header edit did not rebuild dependent object'
    capture = work / 'frame.bmp'
    run([executable, '--frames', '3', '--capture', capture], cwd=work, env=env, timeout=30)
    useful_bmp(capture)
    for option in ('--font', '--bold-font'):
        missing = work / ('missing-' + option[2:] + '.otf')
        output = run([executable, '--frames', '3', option, missing], cwd=work,
                     env=env, success=False, timeout=30)
        assert str(missing) in output and 'font' in output.lower(), output
    # Default-path errors must identify the missing bundled face as well.
    for name in ('Inter-Regular.otf', 'Inter-Bold.otf'):
        asset = bundle / 'assets/fonts' / name
        backup = asset.with_suffix('.saved')
        asset.rename(backup)
        try:
            output = run([executable, '--frames', '1'], cwd=work, env=env,
                         success=False, timeout=30)
            assert name in output and 'font' in output.lower(), output
        finally:
            backup.rename(asset)
    for value in ('0', '-1', '3x', '4294967296'):
        output = run([executable, '--frames', value], cwd=work, env=env,
                     success=False, timeout=30)
        assert '--frames' in output, output
    run([executable, '--capture', capture], cwd=work, env=env, success=False, timeout=30)
    run([executable, '--frames', '1', '--capture', work / 'absent/frame.bmp'],
        cwd=work, env=env, success=False, timeout=30)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--portable', action='store_true')
    mode.add_argument('--native', action='store_true')
    args = parser.parse_args()
    if args.native and (not sys.platform.startswith('linux') or
                        os.environ.get('CUI_TEST_VIRTUAL_DISPLAY') != '1'):
        parser.error('native tests require scripts/test_local.py and CUI_TEST_VIRTUAL_DISPLAY=1; desktop fallback is forbidden')
    with tempfile.TemporaryDirectory(prefix='native-bundle-') as directory:
        work = Path(directory)
        archives = [work / 'first.tar.gz', work / 'second.tar.gz']
        for archive in archives:
            run([sys.executable, ROOT / 'scripts/native_bundle.py', '--output', archive])
        assert archives[0].read_bytes() == archives[1].read_bytes(), 'bundle is not reproducible'
        bundle = extract_and_check(archives[0], work / 'extracted')
        assert not bundle.is_relative_to(ROOT), 'consumer must build outside source checkout'
        if args.native:
            native_checks(bundle, work)
    print('PASS native source bundle' + (' build, dependencies, relocation and rendering' if args.native else ' reproducibility and manifest'))


if __name__ == '__main__':
    main()
