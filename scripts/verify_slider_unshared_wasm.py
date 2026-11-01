#!/usr/bin/env python3
"""Fail closed unless an opted-in FINAL wasm32 module has exactly one unshared memory."""
import argparse
import hashlib
import json
from pathlib import Path


def verify(data):
    if data[:8] != b'\0asm\x01\0\0\0':
        raise ValueError('expected final WebAssembly v1 module')
    cursor = 8
    memories = []
    seen = set()

    def integer(end):
        nonlocal cursor
        value = 0
        for shift in range(0, 35, 7):
            if cursor >= end:
                raise ValueError('truncated integer')
            byte = data[cursor]; cursor += 1
            value |= (byte & 127) << shift
            if not byte & 128:
                if value > 0xffffffff:
                    raise ValueError('integer overflow')
                return value
        raise ValueError('oversized integer')

    def byte(end):
        nonlocal cursor
        if cursor >= end:
            raise ValueError('truncated descriptor')
        value = data[cursor]; cursor += 1
        return value

    def name(end):
        nonlocal cursor
        size = integer(end)
        if size > end - cursor:
            raise ValueError('truncated name')
        value = data[cursor:cursor+size].decode('utf-8', errors='strict'); cursor += size
        return value

    def limits(end):
        flags = integer(end)
        if flags & 2:
            raise ValueError('shared memory/table is unsupported')
        if flags not in (0, 1):
            raise ValueError('unsupported memory64/limits encoding')
        minimum = integer(end)
        maximum = integer(end) if flags else None
        if maximum is not None and maximum < minimum:
            raise ValueError('invalid limits')
        return {'minimum': minimum, 'maximum': maximum}

    while cursor < len(data):
        section = byte(len(data))
        size = integer(len(data))
        end = cursor + size
        if end > len(data):
            raise ValueError('truncated section')
        if section in (2, 5):
            if section in seen:
                raise ValueError('duplicate import/memory section')
            seen.add(section)
            count = integer(end)
            if count > end - cursor:
                raise ValueError('impossible vector count')
            for _ in range(count):
                if section == 5:
                    memories.append({'imported': False, **limits(end)})
                    continue
                module, field = name(end), name(end)
                kind = byte(end)
                if kind == 0:
                    integer(end)
                elif kind == 1:
                    if byte(end) not in (0x70, 0x6f):
                        raise ValueError('unsupported table reference encoding')
                    limits(end)
                elif kind == 2:
                    memories.append({'imported': True, 'module': module, 'name': field, **limits(end)})
                elif kind == 3:
                    if byte(end) not in (0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x70, 0x6f) or byte(end) > 1:
                        raise ValueError('unsupported global encoding')
                elif kind == 4:
                    if byte(end) != 0:
                        raise ValueError('unsupported tag encoding')
                    integer(end)
                else:
                    raise ValueError('unknown import kind')
            if cursor != end:
                raise ValueError('trailing import/memory bytes')
        cursor = end
    if len(memories) != 1:
        raise ValueError('expected exactly one actual final memory')
    # An unshared import descriptor prevents a conforming runtime substituting
    # shared memory; actual host runtime tests independently inspect its buffer.
    return {'memory': memories[0], 'shared': False, 'sha256': hashlib.sha256(data).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('module', type=Path)
    args = parser.parse_args()
    try:
        result = verify(args.module.read_bytes())
    except (OSError, ValueError, UnicodeError) as error:
        parser.exit(1, f'Unshared Slider verification failed: {error}\n')
    print(json.dumps(result, sort_keys=True))


if __name__ == '__main__':
    main()
