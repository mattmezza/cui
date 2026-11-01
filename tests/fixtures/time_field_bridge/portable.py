"""Bounded pinned-WASM compile-only probe; no engine build, install or window.

SOURCE ONLY until invoked under a compiler lease. Runtime support still needs
native/explicit-unshared tests; compile-only success does not certify final memory.
"""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zig', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, required=True)
    args = parser.parse_args()
    version = subprocess.check_output([str(args.zig), 'version'], text=True).strip()
    if version != '0.16.0':
        raise SystemExit('Expected pinned Zig0.16.0; no tool installation is attempted')
    output = args.build_dir / 'time-field-portable'
    output.mkdir(parents=True, exist_ok=True)
    common = [str(args.zig), 'cc', '-target', 'wasm32-wasi', '-std=c11', '-O2',
              '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-I', str(ROOT / 'include')]
    jobs = [('abi', ROOT / 'tests/fixtures/time_field_bridge/abi.c', []),
            ('bridge-default', ROOT / 'src/bridge/cui_bridge.c', []),
            ('bridge-unshared', ROOT / 'src/bridge/cui_bridge.c', ['-DCUI_BRIDGE_SLIDER_WASM_UNSHARED=1']),
            ('bridge-shared-unsupported', ROOT / 'src/bridge/cui_bridge.c',
             ['-DCUI_BRIDGE_SLIDER_WASM_UNSHARED=1', '-matomics', '-mbulk-memory'])]
    for name, source, defines in jobs:
        command = common + defines + ['-c', str(source), '-o', str(output / (name + '.o'))]
        print(command, flush=True)
        subprocess.run(command, check=True)
    print('Compile-only ABI/default/explicit-unshared/shared-unsupported objects; no runtime or memory-support claim')


if __name__ == '__main__':
    main()
