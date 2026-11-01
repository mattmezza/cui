#!/usr/bin/env python3
"""Prepare compact compile-only SDL guard measurements; never compile/init SDL."""
import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from sdl_v1_reference import ROOT, layout_source


def prepare(candidate=False):
    source = layout_source(candidate)
    # Only include spellings change; actual frozen declarations remain authoritative.
    source = source.replace('"../../src/', '"' + str(ROOT / 'src') + '/')
    source = source.replace('"../software/', '"' + str(ROOT / 'backends/software') + '/')
    source += '\n/* Exact calloc(1,sizeof(*s)) request; no native calls in this TU. */\n'
    source += '_Static_assert(sizeof(cui_sdl3) <= 0xffffffffu, "compact measurement");\n'
    return source


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', action='store_true')
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_text(prepare(args.candidate))
