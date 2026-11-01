#!/usr/bin/env python3
"""Measure one appended software guard from actual frozen/candidate C source.

Test-only; never rewrites canonical production or frozen expected resources.
Compiler use requires the maintainer's separately coordinated bounded lease.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[1]
FROZEN = ROOT / 'tests/fixtures/text_prepare_v1/source/backends/software/cui_software.c'
CANONICAL = ROOT / 'backends/software/cui_software.c'
BASE_SHA = 'f2975fc0873a6c100f47aea8c2e5da05b39f00ac850ed77cc938a33ec34b74cb'
SHADOW_SHA = 'cc4786f94d509d185639d680eb0d3023207d6b700821111d4faf8e12420207d8'
NAMES = ['word', 'size', 'alignment', 'shadows', 'width', 'height', 'pixels', 'font',
         'clip', 'ink', 'text_paint', 'paint_size', 'v2_busy', 'renderer_size',
         'renderer_alignment', 'renderer_user', 'renderer_name', 'renderer_capabilities',
         'renderer_render', 'renderer_destroy']


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def parse_layout(assembly, word):
    """Read only the named constant array; reject relocations/partial metadata."""
    symbol = 'cui_sw_layout_probe'
    match = re.search(r'^' + symbol + r':\s*(?:#.*)?$', assembly, re.M)
    if not match:
        raise ValueError('missing layout symbol')
    declared = re.search(r'\.size\s+' + symbol + r'\s*,\s*(\d+)', assembly)
    if not declared or int(declared[1]) != len(NAMES) * word:
        raise ValueError('wrong declared layout byte size')
    values = []
    ended = False
    for raw in assembly[match.end():].splitlines():
        line = raw.split('#', 1)[0].strip()
        if re.match(r'\.size\s+' + symbol + r'\s*,', line):
            ended = True
            break
        if not line:
            continue
        if line.startswith(('.ident', '.section')) and len(values) == len(NAMES):
            ended = True
            break
        item = re.fullmatch(r'\.(quad|long|int32|int64|zero|skip)\s+(.+)', line)
        if not item:
            raise ValueError('unexpected layout directive: ' + line)
        kind, data = item.groups()
        if kind in ('zero', 'skip'):
            count = int(data, 0)
            if count % word:
                raise ValueError('partial zero word')
            values.extend([0] * (count // word))
        else:
            width = 8 if kind in ('quad', 'int64') else 4
            if width != word:
                raise ValueError('wrong metadata word width')
            for number in data.split(','):
                values.append(int(number.strip(), 0) & ((1 << (word * 8)) - 1))
    if not ended or len(values) != len(NAMES) or values[0] != word:
        raise ValueError('incomplete layout array')
    return dict(zip(NAMES, values))


def rewritten(text):
    # The copied backend lives in the existing build directory. Only relative
    # include spelling changes; every included canonical dependency is pinned.
    return re.sub(r'#include "(\.\.?/[^"\n]+)"',
                  lambda m: '#include "' + str((CANONICAL.parent / m[1]).resolve()) + '"', text)


def prepare(build, actual=False):
    if digest(FROZEN) != BASE_SHA or digest(ROOT / 'backends/software/cui_shadow.h') != SHADOW_SHA:
        raise ValueError('frozen software/dependent cache authority changed')
    baseline = FROZEN.read_text()
    current = CANONICAL.read_text()
    if not actual:
        if digest(CANONICAL) != BASE_SHA:
            raise ValueError('preimplementation canonical source no longer matches baseline')
        needle = '    cui_text_paint text_paint;\n};'
        if current.count(needle) != 1:
            raise ValueError('ambiguous guard insertion')
        current = current.replace(needle, '    cui_text_paint text_paint;\n    bool v2_busy;\n};')
    elif current.count('bool v2_busy;') != 1:
        raise ValueError('actual backend must contain the reviewed single guard')
    paths = {}
    for kind, text in [('old', baseline), ('candidate', current)]:
        source = build / ('software-public-' + kind + '.c')
        source.write_text(rewritten(text))
        probe = build / ('software-public-' + kind + '-probe.c')
        values = ['sizeof(size_t)', 'sizeof(cui_software)', '_Alignof(cui_software)']
        values += ['offsetof(cui_software,' + f + ')' for f in NAMES[3:11]]
        values += ['sizeof(cui_text_paint)',
                   'SIZE_MAX' if kind == 'old' else 'offsetof(cui_software,v2_busy)',
                   'sizeof(cui_renderer)', '_Alignof(cui_renderer)']
        values += ['offsetof(cui_renderer,' + f + ')' for f in ['user', 'name', 'capabilities', 'render', 'destroy']]
        records = re.findall(r'struct cui_software\s*\{.*?\n\};', text, re.S)
        if len(records) != 1:
            raise ValueError('ambiguous actual renderer record')
        # Exact source record, not a manually maintained mirror. Unused renderer
        # functions are excluded so compiler output is compact constants only.
        probe.write_text('#include <stddef.h>\n#include "cui/cui_software.h"\n'
                         '#include "cui_shadow.h"\n' + records[0] + '\n'
                         'const size_t cui_sw_layout_probe[]={' + ','.join(values) + '};\n')
        paths[kind] = {'source': source, 'probe': probe}
    return paths


def check_pair(old, candidate):
    if candidate['size'] < old['size'] or candidate['size'] - old['size'] > old['alignment']:
        raise ValueError('private guard growth exceeds approved one-alignment-unit bound')
    for name in NAMES:
        if name not in ('size', 'v2_busy') and old[name] != candidate[name]:
            raise ValueError('old field/ABI changed: ' + name)
    if candidate['v2_busy'] < old['text_paint'] + old['paint_size']:
        raise ValueError('guard overlaps old fields')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--zig', type=Path, required=True)
    parser.add_argument('--actual', action='store_true')
    args = parser.parse_args()
    build = args.build_dir.resolve()
    if not build.is_dir():
        raise ValueError('reuse an existing build directory')
    sources = prepare(build, args.actual)
    receipt = {'canonical_source_sha256': digest(CANONICAL), 'frozen_source_sha256': digest(FROZEN),
               'shadow_header_sha256': digest(ROOT / 'backends/software/cui_shadow.h'),
               'actual': args.actual, 'targets': {}}
    zig_lib = args.zig.resolve().parent / 'lib'
    wasm = [str(args.zig), 'clang', '--no-default-config', '-target', 'wasm32-unknown-wasi', '-nostdinc']
    for include in ['include', 'libc/include/wasm-wasi-musl', 'libc/include/generic-musl']:
        directory = zig_lib / include
        if not directory.is_dir():
            raise ValueError('missing pinned WASI header root: ' + str(directory))
        wasm += ['-isystem', str(directory)]
    for target, compiler, word in [('native', [args.cc], 8), ('wasm32', wasm, 4)]:
        data = {}
        for kind in ('old', 'candidate'):
            assembly = build / ('software-public-' + target + '-' + kind + '.s')
            command = compiler + ['-std=c11', '-O0', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                                  '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'backends/software'),
                                  '-S', str(sources[kind]['probe']), '-o', str(assembly)]
            print(shlex.join(command), flush=True)
            subprocess.run(command, check=True, cwd=ROOT)
            data[kind] = parse_layout(assembly.read_text(), word)
            data[kind + '_source_sha256'] = digest(sources[kind]['source'])
            data[kind + '_assembly_sha256'] = digest(assembly)
        check_pair(data['old'], data['candidate'])
        receipt['targets'][target] = data
        print(target, json.dumps({'old': data['old'], 'candidate': data['candidate']}), flush=True)
    if args.actual:
        expected = json.loads((ROOT / 'tests/fixtures/software_v2_public_v1/layout.json').read_text())
        for target, data in receipt['targets'].items():
            for kind in ('old', 'candidate'):
                if data[kind] != expected['targets'][target][kind]:
                    raise ValueError('actual layout differs from frozen preimplementation proof: ' + target + '/' + kind)
    path = build / ('software-public-layout-actual.json' if args.actual else 'software-public-layout-baseline.json')
    path.write_text(json.dumps(receipt, indent=2) + '\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
