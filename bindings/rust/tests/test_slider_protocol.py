#!/usr/bin/env python3
"""Linux Rust optional Slider protocol proof with existing tools, no downloads.

Build only the Rust protocol test against the supplied actual old bridge. Tiny
LD_PRELOAD providers supply the optional group; each case is a distinct process.
Synthetic records prove copy/validation, not actual UI or physical accessibility.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cargo', default='cargo', help='existing Cargo; select a retained toolchain through RUSTUP_TOOLCHAIN')
    p.add_argument('--cc', default='cc', help='existing C11 compiler')
    p.add_argument('--build-dir', type=Path, required=True)
    p.add_argument('--target-dir', type=Path, required=True, help='existing reused Cargo target directory')
    p.add_argument('--old-library', type=Path, required=True)
    a = p.parse_args()
    if sys.platform != 'linux':
        p.error('these preload fixtures currently validate Linux only')
    root = Path(__file__).resolve().parents[3]
    build, target, old = a.build_dir.resolve(), a.target_dir.resolve(), a.old_library.resolve()
    if not build.is_dir() or not target.is_dir() or not old.is_file() or old.name != 'libcui_bridge.so':
        p.error('existing build/target directories and actual old libcui_bridge.so are required')
    out = build / 'rust-slider-protocol'
    out.mkdir(exist_ok=True)
    env = dict(os.environ, CARGO_BUILD_JOBS='1', CARGO_NET_OFFLINE='true', CUI_BRIDGE_LIB_DIR=str(old.parent))
    env.pop('LD_PRELOAD', None)

    def run(command, capture=False):
        print('RUN', json.dumps([str(x) for x in command]), flush=True)
        return subprocess.run([str(x) for x in command], cwd=root, env=env, check=True,
                              text=True, stdout=subprocess.PIPE if capture else None)

    result = run([a.cargo, 'test', '--offline', '--locked', '-j1', '--manifest-path',
                  root / 'bindings/rust/Cargo.toml', '--target-dir', target, '--test',
                  'slider_model_protocol', '--no-run', '--message-format=json'], capture=True)
    binary = None
    for line in result.stdout.splitlines():
        try:
            item = json.loads(line)
        except json.JSONDecodeError:
            print(line, flush=True)
            continue
        if item.get('reason') == 'compiler-artifact' and item.get('target', {}).get('name') == 'slider_model_protocol' and item.get('executable'):
            binary = Path(item['executable'])
        if item.get('reason') == 'compiler-message':
            print(item.get('message', {}).get('rendered', ''), end='', flush=True)
    if binary is None:
        raise RuntimeError('Cargo did not report the protocol executable')
    for missing in range(17):
        run([a.cc, '-std=c11', '-O2', '-shared', '-fPIC', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
             '-I' + str(root / 'include'), '-DCUI_SLIDER_OMIT=' + str(missing),
             root / 'bindings/rust/tests/fixtures/slider_model_provider.c', '-o', out / f'optional-{missing}.so'])
    entries = []

    def case(name, mode, provider=None):
        variables = [f'LD_LIBRARY_PATH={old.parent}', f'CUI_RUST_SLIDER_PROTOCOL={mode}', f'CUI_SLIDER_FIXTURE={mode}']
        # Explicitly clear inherited preload in the actual-old case.
        variables.append('LD_PRELOAD=' + (str(provider) if provider else ''))
        command = ['cmake', '-E', 'env', *variables, binary, '--exact', 'optional_protocol', '--nocapture']
        entries.append('add_test(rust_slider_' + name + ' ' + ' '.join(json.dumps(str(x)) for x in command) + ')\n')

    case('actual_old', 'old')
    for missing in range(16):
        case('missing_' + str(missing), 'missing', out / f'optional-{missing}.so')
    modes = ['edges', 'unavailable', 'config_size', 'value_size', 'receipt_size', 'request_size',
             'presentation_size', 'receipt_size_bad', 'receipt_schema', 'phase', 'reason', 'source',
             'flags', 'absent_source', 'absent_frame', 'absent_index', 'absent_input', 'input_flags',
             'modifiers', 'key', 'input_type', 'numeric', 'inactive_numeric', 'value_reserved', 'domain_reserved', 'config_parent']
    for mode in modes:
        case(mode, mode, out / 'optional-16.so')
    (out / 'CTestTestfile.cmake').write_text(''.join(entries))
    run([sys.executable, root / 'scripts/test_local.py', '--build-dir', build, '--test-dir', out, '-j1', '-V'])


if __name__ == '__main__':
    main()
