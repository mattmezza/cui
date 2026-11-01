#!/usr/bin/env python3
"""Frozen pre-relocation private source authority; no compiler/dependency required."""
from pathlib import Path
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'tests/fixtures/cpu_v2_common_v1'
RECEIPT = json.loads((FIXTURE / 'receipt.json').read_text())
ORIGINAL_COMMIT = '8671fa893e648fd08720fdc9ebb75d027c913f3a'
ORIGINAL_HASHES = {
    'cui_software_v2_internal.h': '95940b370c2fb09b475b7ca7869074865ef8c3351553a6fe47cb332aad36c28d',
    'cui_software_v2_paint_internal.h': 'af402b579238ff198110dcc1e531b92ed29fac6e2f1c8bf07896bae7b863d983',
    'cui_software_v2_text_internal.h': '4e451665f47b730e54d24dfff78545d85a74350856070e2f796338f56b21db16',
    'cui_software_v2_shadow_internal.h': '15740c42e5af62516b7b01e821ca36ce6bc2cd43790743b2223a62f0ae5f716b',
}

# Keep literals and C multi-character operators intact: removing all whitespace
# naively would incorrectly equate `x + +y` with `x++ + y` or altered strings.
LEXER = re.compile(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|'
                   r'[A-Za-z_][A-Za-z_0-9]*|(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)'
                   r'(?:[eEpP][+-]?[0-9]+)?[uUlLfF]*|'
                   r'>>=|<<=|\.\.\.|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||'
                   r'\*=|/=|%=|\+=|-=|&=|\^=|\|=|##|[^\s]')


def tokens(source):
    """Whitespace/comment formatting is inert; directive boundaries are not."""
    out = []
    logical = source.replace('\\\n', '')
    comments_and_literals = re.compile(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'
                                      r"'(?:\\.|[^'\\])*'")
    logical = comments_and_literals.sub(
        lambda m: ' ' + '\n' * m[0].count('\n') if m[0].startswith(('/*', '//')) else m[0],
        logical)
    for line in logical.splitlines():
        if re.match(r'^\s*#', line):
            function = re.match(r'^\s*#\s*define\s+[A-Za-z_]\w*\(', line)
            out.append(('directive', bool(function)))
        out.extend(x for x in LEXER.findall(line) if not x.startswith(('/*', '//')))
        if re.match(r'^\s*#', line):
            out.append(('directive_end',))
    return out


def renamed(source):
    result = source.replace('cui_software_v2_', 'cui_cpu_v2_')
    result = result.replace('CUI_SOFTWARE_V2_', 'CUI_CPU_V2_')
    for utility in ('cui_shadow.h', 'cui_mesh.h', 'cui_shape.h'):
        result = result.replace('#include "' + utility + '"',
                                '#include "../software/' + utility + '"')
    return result


def verify():
    if RECEIPT['source_commit'] != ORIGINAL_COMMIT or RECEIPT['headers'] != ORIGINAL_HASHES:
        raise ValueError('original signed source authority/receipt changed')
    for name, expected in RECEIPT['headers'].items():
        frozen = FIXTURE / name
        if hashlib.sha256(frozen.read_bytes()).hexdigest() != expected:
            raise ValueError('frozen original changed: ' + name)
        new = ROOT / 'backends/common' / name.replace('cui_software_v2_', 'cui_cpu_v2_')
        if tokens(renamed(frozen.read_text())) != tokens(new.read_text()):
            raise ValueError('nonmechanical implementation change: ' + name)
    for name, expected in RECEIPT['v1_unchanged'].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected:
            raise ValueError('legacy source/fixture changed: ' + name)
