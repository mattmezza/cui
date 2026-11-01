#!/usr/bin/env python3
"""Linux optional Go Slider protocol proof; existing tools/old binary, no downloads.

The provider's legacy dependency is the supplied actual old bridge. Synthetic
records are protocol-only evidence. All runtime cases use the private wrapper.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--go', default='go', help='existing Go executable; no installation')
    p.add_argument('--cc', default='cc', help='existing C11 compiler')
    p.add_argument('--build-dir', type=Path, required=True)
    p.add_argument('--library', type=Path, required=True)
    p.add_argument('--old-library', type=Path, required=True)
    p.add_argument('--legacy', action='store_true', help='also run the full legacy suite, including SDL fixtures')
    p.add_argument('--font', type=Path, help='explicit font for --legacy (default: pinned bundled Inter)')
    a = p.parse_args()
    if sys.platform != 'linux':
        p.error('this dependency-provider fixture currently validates Linux only')
    root = Path(__file__).resolve().parents[3]
    build, current, old = a.build_dir.resolve(), a.library.resolve(), a.old_library.resolve()
    if not build.is_dir() or not current.is_file() or not old.is_file():
        p.error('build directory and both actual shared libraries must already exist')
    out = build / 'go-slider-protocol'
    out.mkdir(exist_ok=True)
    env = dict(os.environ, GOMAXPROCS='1', GOPROXY='off', GOTOOLCHAIN='local')

    def run(command, cwd=root):
        print('RUN', json.dumps([str(x) for x in command]), flush=True)
        subprocess.run([str(x) for x in command], cwd=cwd, env=env, check=True)

    binary = out / 'slider-tests'
    run([a.go, 'test', '-tags', 'cui_dynamic', '-p', '1', '-c', '-o', binary, './cui'], root / 'bindings/go')
    for missing in range(17):
        run([a.cc, '-std=c11', '-O2', '-shared', '-fPIC', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
             '-I' + str(root / 'include'), '-DCUI_SLIDER_OMIT=' + str(missing),
             root / 'bindings/go/tests/slider-model-provider.c', '-Wl,--no-as-needed', old,
             '-Wl,-rpath,' + str(old.parent), '-o', out / f'optional-{missing}.so'])
    entries = []

    def case(name, variables, arguments):
        command = ['cmake', '-E', 'env', 'GOMAXPROCS=1',
                   *[f'{k}={v}' for k, v in variables.items()], binary, *arguments]
        entries.append('add_test(go_slider_' + name + ' ' + ' '.join(json.dumps(str(x)) for x in command) + ')\n')

    case('typed', {'CUI_BRIDGE_LIBRARY': current}, ['-test.run', '^TestSliderControlled', '-test.v'])
    case('actual_old', {'CUI_BRIDGE_LIBRARY': old, 'CUI_TEST_OLD_SLIDER': '1'},
         ['-test.run', '^TestSliderOldLibraryFallback$', '-test.v'])
    for missing in range(16):
        case('missing_' + str(missing), {'CUI_BRIDGE_LIBRARY': out / f'optional-{missing}.so',
             'CUI_SLIDER_PROTOCOL': 'missing', 'CUI_SLIDER_FIXTURE': 'missing'},
             ['-test.run', '^TestSliderOptionalProtocol$', '-test.v'])
    modes = ['edges', 'numeric_set', 'inactive_numeric_zero', 'unavailable', 'config_size', 'value_size', 'receipt_size', 'request_size',
             'presentation_size', 'receipt_size_bad', 'receipt_schema', 'phase', 'reason', 'source',
             'flags', 'absent_source', 'absent_frame', 'absent_index', 'absent_input', 'input_flags',
             'modifiers', 'key', 'input_type', 'numeric', 'inactive_numeric', 'inactive_numeric_key', 'value_reserved', 'domain_reserved']
    for mode in modes:
        case(mode, {'CUI_BRIDGE_LIBRARY': out / 'optional-16.so', 'CUI_SLIDER_PROTOCOL': mode,
                    'CUI_SLIDER_FIXTURE': mode}, ['-test.run', '^TestSliderOptionalProtocol$', '-test.v'])
    if a.legacy:
        font = (a.font or root / 'third_party/inter/Inter-Regular.otf').resolve()
        if not font.is_file():
            p.error('--legacy requires an existing explicit font')
        case('legacy', {'CUI_BRIDGE_LIBRARY': current, 'CUI_TEST_FONT': font},
             ['-test.skip', '^TestSlider', '-test.v'])
    (out / 'CTestTestfile.cmake').write_text(''.join(entries))
    run([sys.executable, root / 'scripts/test_local.py', '--build-dir', build,
         '--test-dir', out, '-j1', '-V'])


if __name__ == '__main__':
    main()
