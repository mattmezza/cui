#!/usr/bin/env python3
"""Build focused Dialog TUs against an existing matching native library cache.

Does not run native code: invoke scripts/test_local.py on the emitted CTest file.
No dependency download, Core rebuild or window creation.
"""
import argparse
import json
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir', type=Path, required=True)
    p.add_argument('--libraries', type=Path, required=True)
    args = p.parse_args()
    out = args.build_dir.resolve()
    libs = args.libraries.resolve()
    assert out.is_dir() and libs.is_dir()
    prefix = out / 'dialog-six-slot-current'
    prefix.mkdir(exist_ok=True)
    commands = []
    def run(command):
        command = [str(x) for x in command]
        commands.append(command)
        print(shlex.join(command), flush=True)
        subprocess.run(command, cwd=ROOT, check=True)
    flags = ['cc', '-std=c11', '-O2', '-g0', '-UNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Iinclude']
    sources = ['src/base/cui_dialog_composition.c', 'src/components/cui_dialog_composition.c',
               'tests/unit/dialog_composition_helpers.c']
    objects = []
    # ABI compile is first; assertions encode expectations, not a substitute for
    # reporting the actual exported sizeof/offsetof constants below.
    run([*flags, '-c', 'tests/unit/test_dialog_composition_abi.c', '-o', prefix / 'abi.o'])
    abi = prefix / 'abi.c'
    abi.write_text('#include "cui/cui_dialog_composition.h"\n#include <stddef.h>\n#include <stdio.h>\nint main(void) { printf("style=%zu refinement=%zu sizing=%zu slot=%zu composition=%zu layout=%zu completed_offset=%zu sequence_offset=%zu\\n", sizeof(cui_dialog_slot_style_v1),sizeof(cui_dialog_refinement_v1),sizeof(cui_dialog_sizing_v1),sizeof(cui_dialog_slot),sizeof(cui_dialog_composition_v1),sizeof(cui_dialog_layout_v1),offsetof(cui_dialog_layout_v1,completed),offsetof(cui_dialog_layout_v1,private_submit_frame)); return 0; }\n')
    run([*flags, abi, '-o', prefix / 'abi'])
    for index, source in enumerate(sources):
        obj = prefix / f'part{index}.o'
        run([*flags, '-c', source, '-o', obj])
        objects.append(obj)
    link = [libs / 'libcui_software.a', libs / 'libcui_text.a', libs / 'libcui.a', '-lm',
            '/usr/lib/libharfbuzz.so', '/usr/lib/libfreetype.so', libs / 'libcui_bidi.a']
    tests = [('dialog_composition_abi', prefix / 'abi', [])]
    for case in ['contracts', 'geometry', 'actions', 'intrinsic']:
        binary = prefix / case
        run([*flags, f'tests/unit/test_dialog_composition_{case}.c', *objects, *link, '-o', binary])
        fonts = [ROOT / 'third_party/inter/Inter-Regular.otf', ROOT / 'third_party/inter/Inter-Bold.otf'] if case in ['geometry', 'intrinsic'] else []
        tests.append((f'dialog_composition_{case}', binary, fonts))
    guide = (ROOT / 'docs/DIALOG_COMPOSITION.md').read_text()
    example = guide.split('```c\n', 1)[1].split('\n```', 1)[0]
    example_file = prefix / 'guide.c'
    example_file.write_text(example + '\n')
    run([*flags, example_file, *objects[:2], *link, '-o', prefix / 'guide'])
    tests.append(('dialog_composition_guide', prefix / 'guide', [ROOT / 'third_party/inter/Inter-Regular.otf', ROOT / 'third_party/inter/Inter-Bold.otf']))
    lines = []
    for name, binary, tail in tests:
        lines.append('add_test(' + ' '.join('"' + str(x).replace('\\', '/') + '"' for x in [name, binary, *tail]) + ')')
    (prefix / 'CTestTestfile.cmake').write_text('\n'.join(lines) + '\n')
    (prefix / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
    print('Build complete. Run private wrapper with --test-dir ' + str(prefix), flush=True)

if __name__ == '__main__':
    main()
