#!/usr/bin/env python3
"""Focused Linux hidden-window proof using one existing native bridge build.

No downloads or dependency installs. Compiles only the addon, fixture and dynamic
Go/Rust tests; TypeScript is compiled then executed separately. All runtime tests
run through test_local.py/private Xvfb. Pass the existing --build-dir; this helper
does not configure or build another CUI tree. The native proof requires installed
SDL3/X11 development headers and the X11 runtime already used by private Xvfb. Run under nice/taskset as desired.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def run(command, cwd, env, capture=False):
    command = [str(item) for item in command]
    print('+ ' + ' '.join(command), flush=True)
    return subprocess.run(command, cwd=cwd, env=env, check=True, text=True,
                          stdout=subprocess.PIPE if capture else None)

def quote(value):
    return '"' + str(value).replace('\\', '\\\\').replace('"', '\\"').replace(';', '\\;') + '"'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--font', type=Path, required=True)
    parser.add_argument('--tsc', type=Path, help='Existing TypeScript compiler JS entrypoint; never installs packages')
    args = parser.parse_args()
    if not sys.platform.startswith('linux'):
        parser.error('Linux private-Xvfb validation only')
    build = args.build_dir.resolve(strict=True)
    library = (build / 'libcui_bridge.so').resolve(strict=True)
    font = args.font.resolve(strict=True)
    env = dict(os.environ)
    for name in ('DISPLAY', 'WAYLAND_DISPLAY', 'MIR_SOCKET', 'SDL_VIDEODRIVER',
                 'XDG_RUNTIME_DIR', 'CUI_TEST_VIRTUAL_DISPLAY', 'LD_PRELOAD',
                 'CUI_BRIDGE_LIBRARY', 'CUI_BRIDGE_LIB_DIR', 'CUI_NODE_ADDON'):
        env.pop(name, None)
    env.update(GOMAXPROCS='1', GOFLAGS='-p=1', CGO_ENABLED='1', GOPROXY='off', GOSUMDB='off',
               GOTOOLCHAIN='local', GOWORK='off', CARGO_BUILD_JOBS='1', CARGO_NET_OFFLINE='true',
               CUI_TEST_FONT=str(font), CARGO_TARGET_DIR=str(build / 'visibility-rust-target'),
               CUI_BRIDGE_LIB_DIR=str(build), LD_LIBRARY_PATH=str(build))
    node = Path(shutil.which('node') or '').resolve()
    include = next((p for p in (node.parent.parent / 'include/node', Path('/usr/include/node'))
                    if (p / 'node_api.h').is_file()), None)
    if include is None:
        parser.error('Installed Node headers are required')
    addon = build / 'visibility.node'
    run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-fPIC', '-shared',
         '-DNAPI_VERSION=8', '-I' + str(ROOT / 'include'), '-I' + str(include),
         ROOT / 'bindings/node/addon.c', library, '-Wl,-rpath,' + str(build), '-lm', '-o', addon], ROOT, env)
    fixture = build / 'visibility-fixture.so'
    flags = subprocess.check_output(['pkg-config', '--cflags', '--libs', 'sdl3'], text=True).split()
    run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-UNDEBUG', '-fPIC', '-shared',
         ROOT / 'bindings/tests/window_visibility_fixture.c', *flags, '-ldl', '-o', fixture], ROOT, env)
    go_binary = build / 'visibility-go'
    run(['go', 'test', '-c', '-p', '1', '-tags', 'cui_dynamic', '-o', go_binary, './cui'], ROOT / 'bindings/go', env)
    result = run(['cargo', 'test', '--offline', '-j', '1', '--test', 'window_visibility_native',
                  '--no-run', '--message-format=json'], ROOT / 'bindings/rust', env, capture=True)
    binaries = [Path(obj['executable']) for line in result.stdout.splitlines()
                if (obj := json.loads(line)).get('reason') == 'compiler-artifact' and obj.get('executable')]
    if len(binaries) != 1:
        raise RuntimeError('Expected one Rust visibility test executable')
    tsout = build / 'visibility-typescript'
    tsout.mkdir(exist_ok=True)
    compiler = ['node', args.tsc.resolve(strict=True)] if args.tsc else ['tsc']
    run([*compiler, '--strict', '--target', 'ES2020', '--module', 'commonjs', '--outDir', tsout,
         ROOT / 'bindings/typescript/window_visibility_native.ts'], ROOT, env)
    (tsout / 'index.js').write_text('module.exports = require(' + json.dumps(str(ROOT / 'bindings/node')) + ');\n')
    # Relink the same verified objects with only new exports hidden. This models
    # an older ABI1 dynamic library without rebuilding the engine or packages.
    version_script = build / 'visibility-legacy.map'
    version_script.write_text('{ global: cui_bridge_*; local: cui_bridge_create_with_window_options; '
                              'cui_bridge_window_options_size; cui_bridge_set_window_visible; '
                              'cui_bridge_get_window_visible; *; };\n')
    legacy = build / 'visibility-legacy.so'
    dependencies = subprocess.check_output(['pkg-config', '--libs', 'sdl3', 'freetype2', 'harfbuzz'], text=True).split()
    run(['cc', '-shared', '-Wl,--version-script=' + str(version_script), '-o', legacy,
         build / 'CMakeFiles/cui_bridge.dir/src/bridge/cui_bridge.c.o',
         build / 'CMakeFiles/cui_bridge.dir/src/bridge/cui_bridge_sdl.c.o',
         build / 'libcui.a', build / 'libcui_sdl3.a', build / 'libcui_text.a',
         build / 'libcui.a', build / 'libcui_bidi.a', *dependencies, '-lm'], ROOT, env)
    tests = []
    common = {'CUI_TEST_FONT': font, 'LD_PRELOAD': fixture, 'LD_LIBRARY_PATH': build}
    def add(name, command, extra, native=True):
        values = dict(common if native else {}, **extra)
        cmd = ['cmake', '-E', 'env', *[key + '=' + str(value) for key, value in values.items()], *command]
        tests.append('add_test(' + quote(name) + ' ' + ' '.join(quote(x) for x in cmd) + ')\n'
                     + 'set_tests_properties(' + quote(name) + ' PROPERTIES TIMEOUT 30)\n')
    add('hidden_python', [sys.executable, ROOT / 'bindings/python/test_window_visibility_native_virtual.py'],
        {'PYTHONPATH': ROOT / 'bindings/python', 'CUI_BRIDGE_LIBRARY': library})
    add('hidden_javascript', ['node', ROOT / 'bindings/node/test/window-visibility-native.test.cjs'], {'CUI_NODE_ADDON': addon})
    add('hidden_typescript', ['node', tsout / 'window_visibility_native.js'], {'CUI_NODE_ADDON': addon})
    add('hidden_go', [go_binary, '-test.run=^TestWindowVisibilityNative$', '-test.v'], {'CUI_BRIDGE_LIBRARY': library})
    add('hidden_rust', [binaries[0], '--test-threads=1'], {})
    add('hidden_python_legacy', [sys.executable, ROOT / 'bindings/python/test_window_visibility_legacy.py'],
        {'PYTHONPATH': ROOT / 'bindings/python', 'CUI_BRIDGE_LIBRARY': legacy}, native=False)
    add('hidden_go_legacy', [go_binary, '-test.run=^TestWindowVisibilityLegacyLibrary$', '-test.v'],
        {'CUI_BRIDGE_LIBRARY': legacy, 'CUI_WINDOW_LEGACY_LIBRARY': '1'}, native=False)
    registration = build / 'visibility-tests' 
    registration.mkdir(exist_ok=True)
    (registration / 'CTestTestfile.cmake').write_text(''.join(tests))
    run([sys.executable, ROOT / 'scripts/test_local.py', '--build-dir', registration], ROOT, env)

if __name__ == '__main__':
    main()
