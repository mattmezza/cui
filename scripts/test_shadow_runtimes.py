#!/usr/bin/env python3
"""Focused Linux shadow facade checks, reusing one already-built bridge directory.

Run under nice/taskset. No dependencies are fetched and no CUI sources are rebuilt.
All native executions go through test_local.py's private Xvfb session.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def run(command, **kwargs):
    subprocess.run([str(x) for x in command], check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', required=True, type=Path)
    parser.add_argument('--old-library', type=Path)
    parser.add_argument('--parts', action='store_true', help='Check semantic part facades instead of legacy root slice')
    args = parser.parse_args()
    build = args.build_dir.resolve()
    library = build / 'libcui_bridge.so'
    if not library.is_file() or not (build / 'CMakeCache.txt').is_file():
        parser.error('build-dir must contain the already-built portable shared bridge')
    for tool in ('cc', 'node', 'go', 'cargo'):
        if not shutil.which(tool):
            parser.error(f'{tool} must already be installed')
    checks = build / 'shadow-runtime-checks'
    checks.mkdir(exist_ok=True)
    addon = checks / 'cui.node'
    # Match bindings/node/CMakeLists.txt: C11, PIC module, strict warnings,
    # shared bridge dependency, Linux math/dynamic-symbol libraries.
    run(['cc', '-std=c11', '-shared', '-fPIC', '-Wall', '-Wextra', '-Wpedantic',
         '-Werror', '-I/usr/include/node', '-I' + str(ROOT / 'include'),
         ROOT / 'bindings/node/addon.c', '-L' + str(build), '-lcui_bridge',
         '-Wl,-rpath,' + str(build), '-lm', '-ldl', '-o', addon])
    entries = []

    def test(name, command, cwd, env):
        quote = lambda value: json.dumps(str(value))
        entries.append('add_test(' + ' '.join(map(quote, [name, 'cmake', '-E', 'env',
                       *[f'{k}={v}' for k, v in env.items()], *command])) + ')\n')
        entries.append(f'set_tests_properties({name} PROPERTIES WORKING_DIRECTORY {quote(cwd)})\n')

    test('shadow_node', ['node', ROOT / ('bindings/node/test/part-shadows.test.cjs' if args.parts else 'bindings/node/test/shadows.test.cjs')], ROOT,
         {'CUI_NODE_ADDON': addon})
    test('shadow_go', ['go', 'test', '-tags', 'cui_dynamic', '-p', '1', '-count=1', '-run', '^TestPartShadowAuthoring$' if args.parts else '^TestShadowAuthoring$', './cui'],
         ROOT / 'bindings/go', {'CUI_BRIDGE_LIBRARY': library, 'GOMAXPROCS': 1, 'GOPROXY': 'off'})
    rust = ['cargo', 'test', '--offline', '--locked', '-j1', '--test', 'part_shadows' if args.parts else 'shadows']
    rust_env = {'CUI_BRIDGE_LIB_DIR': build, 'CARGO_TARGET_DIR': checks / 'rust',
                'LD_LIBRARY_PATH': build}
    test('shadow_rust', [*rust, 'semantic_parts' if args.parts else 'owned_root_shadows_clear_reset_geometry_and_input', '--', '--test-threads=1'],
         ROOT / 'bindings/rust', rust_env)
    if args.old_library:
        old = args.old_library.resolve()
        if not old.is_file():
            parser.error('old-library must exist')
        test('shadow_go_old', ['go', 'test', '-tags', 'cui_dynamic', '-p', '1', '-count=1', '-run', '^TestPartShadowOldLibraryDefaults$' if args.parts else '^TestShadowOldLibraryDefaults$', './cui'],
             ROOT / 'bindings/go', {'CUI_BRIDGE_LIBRARY': old, 'CUI_TEST_OLD_SHADOW_BRIDGE': 1,
                                    'GOMAXPROCS': 1, 'GOPROXY': 'off'})
        test('shadow_rust_old', [*rust, 'optional_old_library_defaults', '--', '--test-threads=1'],
             ROOT / 'bindings/rust', {**rust_env, 'CUI_BRIDGE_LIB_DIR': old.parent,
                                     'LD_LIBRARY_PATH': old.parent, 'CUI_TEST_OLD_SHADOW_BRIDGE': 1})
    (checks / 'CTestTestfile.cmake').write_text(''.join(entries))
    run([sys.executable, ROOT / 'scripts/test_local.py', '--build-dir', build,
         '--test-dir', checks, '-j1', '-V'])


if __name__ == '__main__':
    main()
