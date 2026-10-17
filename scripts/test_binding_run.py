#!/usr/bin/env python3
"""Build and validate binding callback loops using only an invisible local display.

Linux only. Requires installed Go, Cargo/CMake/C11 tools, SDL3/FreeType/HarfBuzz,
Node development headers, Xvfb and xdotool. Never downloads or publishes packages.
Source modules (including their SDKs) are staged without modifying the checkout;
all builds and generated CTest files live under --build-dir.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def run(command, cwd, env):
    print('+ ' + ' '.join(str(item) for item in command), flush=True)
    subprocess.run([str(item) for item in command], cwd=cwd, env=env, check=True)


def require_static_cui(binary, env):
    dependencies = subprocess.check_output(['ldd', str(binary)], env=env, text=True)
    if 'libcui_bridge' in dependencies:
        raise RuntimeError(f'{binary.name} unexpectedly requires a shared CUI bridge')
    print(binary.name + ': no shared CUI bridge dependency', flush=True)


def rust_binary(command, cwd, env):
    command = [str(item) for item in command] + ['--message-format=json']
    print('+ ' + ' '.join(command), flush=True)
    result = subprocess.run(command, cwd=cwd, env=env, check=True, stdout=subprocess.PIPE, text=True)
    executables = []
    for line in result.stdout.splitlines():
        message = json.loads(line)
        if message.get('reason') == 'compiler-message':
            print(message['message'].get('rendered', ''), end='', flush=True)
        if message.get('reason') == 'compiler-artifact' and message.get('target', {}).get('name') == 'run_native':
            if message.get('executable'):
                executables.append(Path(message['executable']))
    if len(executables) != 1:
        raise RuntimeError('Expected exactly one compiled Rust runner test')
    return executables[0]


def quote(value):
    return '"' + str(value).replace('\\', '\\\\').replace('"', '\\"').replace(';', '\\;') + '"'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--library', type=Path, help='Canonical native shared bridge; required for dynamic mode')
    parser.add_argument('--font', type=Path, required=True)
    parser.add_argument('--node-include', type=Path, help='Directory containing node_api.h')
    parser.add_argument('--mode', choices=('all', 'static', 'dynamic'), default='all')
    args = parser.parse_args()
    if not sys.platform.startswith('linux'):
        parser.error('This runner validates Linux; other platforms need an isolated native host runner')
    source = args.source_root.resolve(strict=True)
    build = args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    font = args.font.resolve(strict=True)
    if args.mode != 'static' and not args.library:
        parser.error('--library is required for dynamic validation')
    library = args.library.resolve(strict=True) if args.library else None
    env = dict(os.environ)
    for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'MIR_SOCKET', 'SDL_VIDEODRIVER', 'XDG_RUNTIME_DIR',
                'CUI_TEST_VIRTUAL_DISPLAY', 'CUI_BRIDGE_LIBRARY', 'CUI_BRIDGE_LIB_DIR',
                'CUI_NODE_ADDON', 'LD_LIBRARY_PATH', 'LD_PRELOAD', 'PYTHONPATH', 'GOFLAGS',
                'RUSTFLAGS', 'CARGO_ENCODED_RUSTFLAGS'):
        env.pop(key, None)
    env.update(PYTHONDONTWRITEBYTECODE='1', CGO_ENABLED='1', GOPROXY='off', GOSUMDB='off',
               CARGO_NET_OFFLINE='true', GOWORK='off', GOTOOLCHAIN='local', GO111MODULE='on', GOCACHE=str(build / 'go-cache'),
               GOMODCACHE=str(build / 'go-modules'), CARGO_TARGET_DIR=str(build / 'rust-target'),
               CUI_TEST_FONT=str(font))
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=source, text=True).strip()
    print('Source revision: ' + revision, flush=True)
    print('Font SHA256: ' + hashlib.sha256(font.read_bytes()).hexdigest(), flush=True)
    if library:
        print('Native bridge SHA256: ' + hashlib.sha256(library.read_bytes()).hexdigest(), flush=True)
    tests = []
    def add(name, command, extra=None):
        command = ['cmake', '-E', 'env', '--unset=CUI_BRIDGE_LIBRARY', '--unset=CUI_BRIDGE_LIB_DIR',
                   *[key + '=' + str(value) for key, value in (extra or {}).items()], *command]
        tests.append('add_test(' + quote(name) + ' ' + ' '.join(quote(item) for item in command) + ')\n'
                     + 'set_tests_properties(' + quote(name) + ' PROPERTIES TIMEOUT 60)\n')
    with tempfile.TemporaryDirectory(prefix='binding-run-source-', dir=build) as temporary:
        stage = Path(temporary)
        for language in ('go', 'rust'):
            shutil.copytree(source / 'bindings' / language, stage / language,
                            ignore=shutil.ignore_patterns('target', 'build', '__pycache__', '.git'))
            sdk = stage / language / 'sdk/source'
            manifest = sdk / 'SOURCE_FILES.sha256'
            if manifest.is_file():
                print(language + ' SDK manifest SHA256: ' + hashlib.sha256(manifest.read_bytes()).hexdigest(), flush=True)
            if not sdk.is_dir():
                raise RuntimeError(f'{language} canonical SDK snapshot is missing')
            for name in ('include/cui/cui_bridge.h', 'src/bridge/cui_bridge.c'):
                actual = (sdk / name).read_bytes()
                if actual != (source / name).read_bytes():
                    raise RuntimeError(f'{language} SDK {name} differs from canonical source')
        go = stage / 'go'
        rust = stage / 'rust'
        # Build binaries first; no compiler runs inside native callbacks/CTest.
        if args.mode != 'dynamic':
            for kind, tags in (('portable', ''), ('native', 'cui_native')):
                binary = build / ('go-static-' + kind)
                run(['go', 'test', '-c', '-tags', tags, '-o', binary, './cui'], go, env)
                require_static_cui(binary, env)
                pattern = ('^TestPortableRunLeavesManualLoopAvailable$' if kind == 'portable' else
                           '^Test(NativeRunLifecycle|PortableRunLeavesManualLoopAvailable)$')
                add('go_static_' + kind, [binary, '-test.run=' + pattern, '-test.v'])
            for kind, features in (('portable', []), ('native', ['--features', 'native'])):
                # CMake caches absolute staged source paths; isolate each snapshot
                # instead of reusing a cache whose source directory was removed.
                target = build / ('rust-static-' + kind) / stage.name
                rust_env = dict(env, CARGO_TARGET_DIR=str(target))
                binary = rust_binary(['cargo', 'test', '--offline', *features, '--test', 'run_native', '--no-run'], rust, rust_env)
                require_static_cui(binary, env)
                command = [binary, '--test-threads=1']
                if kind == 'portable':
                    command.append('portable_runner_reports_unavailable')
                add('rust_static_' + kind, command)
        if args.mode != 'static':
            include = args.node_include
            if include is None:
                node = Path(shutil.which('node') or '').resolve()
                candidates = (node.parent.parent / 'include/node', Path('/usr/include/node'))
                include = next((path for path in candidates if (path / 'node_api.h').is_file()), None)
            if include is None or not (include / 'node_api.h').is_file():
                raise RuntimeError('Pass --node-include with installed node_api.h headers')
            addon = build / 'cui.node'
            run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-fPIC', '-shared',
                 '-DNAPI_VERSION=8', '-I' + str(source / 'include'), '-I' + str(include),
                 source / 'bindings/node/addon.c', library, '-Wl,-rpath,' + str(library.parent),
                 '-ldl', '-lm', '-o', addon], source, env)
            binary = build / 'go-dynamic'
            run(['go', 'test', '-c', '-tags', 'cui_dynamic', '-o', binary, './cui'], go, env)
            target = build / 'rust-dynamic'
            rust_env = dict(env, CARGO_TARGET_DIR=str(target), CUI_BRIDGE_LIB_DIR=str(library.parent))
            rust_test = rust_binary(['cargo', 'test', '--offline', '--test', 'run_native', '--no-run'], rust, rust_env)
            for kind, filename in (('native', 'test_run_native_virtual.py'), ('portable', 'test_run.py')):
                add('python_' + kind, [sys.executable, source / 'bindings/python' / filename],
                    {'PYTHONPATH': source / 'bindings/python', 'CUI_BRIDGE_LIBRARY': library})
            for kind, filename in (('native', 'run-native.test.cjs'), ('portable', 'run.test.cjs')):
                add('node_' + kind, ['node', source / 'bindings/node/test' / filename], {'CUI_NODE_ADDON': addon})
            add('go_dynamic', [binary, '-test.run=^Test(NativeRunLifecycle|PortableRunLeavesManualLoopAvailable)$', '-test.v'],
                {'CUI_BRIDGE_LIBRARY': library})
            add('rust_dynamic', [rust_test, '--test-threads=1'])
        ctest = build / 'runner-tests'
        ctest.mkdir(exist_ok=True)
        (ctest / 'CTestTestfile.cmake').write_text(''.join(tests))
        run([sys.executable, source / 'scripts/test_local.py', '--build-dir', ctest], source, env)
    print('Binding callback runner validation PASS', flush=True)


if __name__ == '__main__':
    main()
