#!/usr/bin/env python3
"""Lease-only, compile-only native/WASM SDL guard microgate; no SDL initialization."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sdl_v2_guard_layout import ROOT, prepare
sys.path.insert(0, str(ROOT / 'tests'))
from sdl_v1_reference import FROZEN, verify


def constants(assembly):
    result = {}
    label = None
    for line in assembly.splitlines():
        match = re.match(r'^(sdl_v1_\w+):', line)
        if match:
            label = match[1]
        value = re.match(r'\s*\.(?:long|int32)\s+(\d+)', line)
        if label and value:
            result[label.removeprefix('sdl_v1_')] = int(value[1])
            label = None
    if 'size' not in result or 'align' not in result or 'constructor_bytes' not in result:
        raise ValueError('missing compact layout constants; preserve assembly for diagnosis')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--zig', type=Path, required=True)
    parser.add_argument('--cc', default='cc')
    parser.add_argument('--cpu', type=int, default=1)
    args = parser.parse_args()
    verify()
    if args.cpu not in os.sched_getaffinity(0):
        raise ValueError('requested CPU unavailable; coordinate rather than silently repin')
    os.sched_setaffinity(0, {args.cpu})
    os.nice(19)
    out = args.output_dir.resolve()
    out.mkdir(parents=True, exist_ok=True)
    zig = args.zig.resolve()
    lib = zig.parent / 'lib'
    roots = [lib / 'include', lib / 'libc/include/wasm-wasi-musl',
             lib / 'libc/include/generic-musl']
    if not all(path.is_dir() for path in roots):
        raise ValueError('pinned Zig WASI header roots unavailable')
    common = ['-std=c11', '-O1', '-UNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
              '-I' + str(ROOT / 'include')]
    wasm = [str(zig), 'clang', '--no-default-config', '-target', 'wasm32-unknown-wasi', '-nostdinc']
    for path in roots:
        wasm += ['-isystem', str(path)]
    # SDL's architecture-selecting public headers only, never host libc fallback.
    # wasm32 is a layout diagnostic, NOT an SDL/WASI adapter support claim.
    wasm += ['-idirafter', '/usr/include']
    receipt = {'frozen_sha256': FROZEN, 'scope': 'compile-only guard proposal; no SDL calls',
               'layouts': {}, 'commands': [], 'generated_sha256': {}}
    with (out / 'commands.log').open('w') as log:
        for tool in [[args.cc, '--version'], [str(zig), 'version']]:
            log.write(subprocess.check_output(tool, text=True))
        for target, compiler in [('native', [args.cc]), ('wasm32', wasm)]:
            layouts = {}
            for candidate in (False, True):
                name = target + ('-candidate' if candidate else '-original')
                source, assembly = out / (name + '.c'), out / (name + '.s')
                source.write_text(prepare(candidate))
                receipt['generated_sha256'][name] = hashlib.sha256(source.read_bytes()).hexdigest()
                command = compiler + common + ['-S', str(source), '-o', str(assembly)]
                receipt['commands'].append(command)
                log.write(shlex.join(command) + '\n')
                log.flush()
                subprocess.run(command, stdout=log, stderr=log, check=True)
                layouts['candidate' if candidate else 'original'] = constants(assembly.read_text())
            old, new = layouts['original'], layouts['candidate']
            offsets = {k for k in old if k.startswith('offset_')}
            if len(offsets) != 37 or any(old[k] != new[k] for k in offsets):
                raise ValueError('missing/changed original offsets')
            if old['align'] != new['align'] or old['constructor_bytes'] != old['size'] or new['constructor_bytes'] != new['size']:
                raise ValueError('alignment/constructor request mismatch')
            if not (old['tail_end'] <= new['offset_v2_busy'] < new['offset_v2_faulted'] < new['size']):
                raise ValueError('guards not appended beyond old final field offset')
            layouts['constructor_byte_delta'] = new['size'] - old['size']
            receipt['layouts'][target] = layouts
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt['layouts'], indent=2))


if __name__ == '__main__':
    main()
