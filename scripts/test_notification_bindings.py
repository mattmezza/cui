#!/usr/bin/env python3
"""Offline Linux Go/Rust notification contracts through private Xvfb CTest.

Requires existing CMake/C11, Go, Cargo and xvfb-run. No downloads, SDK generation,
production changes or publication. The supplied shared bridge must implement V1.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MODES = ('edges', 'capacity', 'limit', 'zero', 'repeat', 'size', 'unavailable')


def quote(value):
    return '"' + str(value).replace('\\', '\\\\').replace('"', '\\"').replace(';', '\\;').replace('$', '\\$') + '"'


def run(command, cwd, env):
    print('+ ' + ' '.join(map(str, command)), flush=True)
    subprocess.run(list(map(str, command)), cwd=cwd, env=env, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--older-library', type=Path, help='Optional genuine pre-V1 bridge')
    args = parser.parse_args()
    if not sys.platform.startswith('linux'):
        parser.error('This isolated runner validates Linux only')
    source = args.source_root.resolve(strict=True)
    build = args.build_dir.resolve()
    if build == source or source in build.parents:
        parser.error('--build-dir must be outside the source checkout')
    library = args.library.resolve(strict=True)
    if library.name != 'libcui_bridge.so':
        parser.error('--library must be named libcui_bridge.so for Rust linking')
    older = args.older_library.resolve(strict=True) if args.older_library else None
    if older and older.name != 'libcui_bridge.so':
        parser.error('--older-library must be named libcui_bridge.so')
    build.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'MIR_SOCKET', 'SDL_VIDEODRIVER',
                'XDG_RUNTIME_DIR', 'CUI_TEST_VIRTUAL_DISPLAY', 'CUI_BRIDGE_LIBRARY',
                'CUI_BRIDGE_LIB_DIR', 'LD_PRELOAD', 'LD_LIBRARY_PATH', 'GOFLAGS',
                'RUSTFLAGS', 'CARGO_ENCODED_RUSTFLAGS', 'CUI_NOTIFICATION_FIXTURE',
                'CUI_NOTIFICATION_UNAVAILABLE'):
        env.pop(key, None)
    env.update({'CGO_ENABLED': '1', 'GOPROXY': 'off', 'GOSUMDB': 'off',
                'GOTOOLCHAIN': 'local', 'GOWORK': 'off', 'GO111MODULE': 'on',
                'CARGO_NET_OFFLINE': 'true', 'GOCACHE': str(build / 'go-cache'),
                'GOMODCACHE': str(build / 'go-modules')})
    stage = Path(tempfile.mkdtemp(prefix='staged-', dir=build))
    # Preserve staged inputs and CTest logs for inspection, including failed runs.
    for language in ('go', 'rust'):
        dest = stage / language
        shutil.copytree(source / 'bindings' / language, dest,
                        ignore=shutil.ignore_patterns('target', 'build', '__pycache__', '.git'))
        for name in ('include/cui/cui_bridge.h', 'src/bridge/cui_bridge.c'):
            if (dest / 'sdk/source' / name).read_bytes() != (source / name).read_bytes():
                raise RuntimeError(f'{language} SDK {name} is stale; integrator must regenerate it')
    print('Source revision: ' + subprocess.check_output(
        ['git', 'rev-parse', 'HEAD'], cwd=source, text=True).strip(), flush=True)
    print('Shared bridge SHA256: ' + hashlib.sha256(library.read_bytes()).hexdigest(), flush=True)
    fixture = build / 'libcui_notification_fixture.so'
    run([env.get('CC', 'cc'), '-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic',
         '-fPIC', '-shared', '-I' + str(source / 'include'),
         source / 'bindings/tests/notification_v1_fixture.c', '-Wl,--no-as-needed',
         library, '-Wl,-rpath,' + str(library.parent), '-o', fixture], source, env)
    tests = []

    def add(name, command, extra=None):
        command = ['cmake', '-E', 'env', 'CUI_TEST_VIRTUAL_DISPLAY=0',
                   *[key + '=' + str(value) for key, value in (extra or {}).items()], *command]
        tests.append('add_test(NAME ' + quote(name) + ' COMMAND ' +
                     ' '.join(map(quote, command)) + ')\nset_tests_properties(' +
                     quote(name) + ' PROPERTIES TIMEOUT 300)')

    go = ['cmake', '-E', 'chdir', stage / 'go', 'go', 'test']
    rust = ['cargo', 'test', '--offline', '--manifest-path', stage / 'rust/Cargo.toml']
    # Full existing portable tests also prove legacy behavior; native lifecycle
    # tests are explicitly disabled, since these contracts need no visible UI.
    add('go_static_legacy_and_v1', go + ['./cui', '-count=1'])
    add('rust_static_legacy_and_v1', rust + ['--', '--test-threads=1'],
        {'CARGO_TARGET_DIR': build / 'rust-static' / stage.name})
    dynamic = {'CUI_BRIDGE_LIB_DIR': library.parent, 'CARGO_TARGET_DIR': build / 'rust-dynamic'}
    add('go_dynamic_legacy_and_v1', go + ['-tags', 'cui_dynamic', './cui', '-count=1'],
        {'CUI_BRIDGE_LIBRARY': library})
    add('rust_dynamic_legacy_and_v1', rust + ['--', '--test-threads=1'], dynamic)
    for mode in MODES:
        add('go_protocol_' + mode,
            go + ['-tags', 'cui_dynamic', './cui', '-run', '^TestNotificationV1ProtocolFixture$', '-count=1'],
            {'CUI_BRIDGE_LIBRARY': fixture, 'CUI_NOTIFICATION_FIXTURE': mode})
        add('rust_protocol_' + mode,
            rust + ['--test', 'notifications_v1', 'injected_protocol', '--', '--exact'],
            {**dynamic, 'LD_PRELOAD': fixture, 'CUI_NOTIFICATION_FIXTURE': mode})
    if older:
        add('go_older_library', go + ['-tags', 'cui_dynamic', './cui', '-run', '^TestNotificationV1Unavailable$', '-count=1'],
            {'CUI_BRIDGE_LIBRARY': older, 'CUI_NOTIFICATION_UNAVAILABLE': '1'})
        add('rust_older_library', rust + ['--test', 'notifications_v1', 'unavailable_old_library', '--', '--exact'],
            {'CUI_BRIDGE_LIB_DIR': older.parent, 'CARGO_TARGET_DIR': build / 'rust-old', 'CUI_NOTIFICATION_UNAVAILABLE': '1'})
    harness = build / 'harness'
    harness.mkdir(exist_ok=True)
    (harness / 'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.20)\n'
        'project(NotificationBindings NONE)\nenable_testing()\n' + '\n'.join(tests) + '\n')
    run(['cmake', '-S', harness, '-B', build / 'ctest'], source, env)
    run([sys.executable, source / 'scripts/test_local.py', '--build-dir', build / 'ctest'], source, env)
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
