#!/usr/bin/env python3
"""Immutable SDL V1 source authority and compact guard-layout source preparation."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'tests/fixtures/sdl_v1_reference'
FROZEN = {
    'cui_sdl3.c': '62e1ab7726116506e80dade53c1934f3d65ee91b7e9d19ba3beec8ccba01125f',
    'cui_sdl3_mesh.h': '41ece3b72bb3ecfdfc7f0540f066873133d05123e1525ea551bd1aeeb1852883',
    'cui_sdl3.h': 'c59d7e1c9ee300b710d8c389ec3314f42db3572aef5f1f7f5c136d17485bad28',
}


def verify():
    receipt = json.loads((FIXTURE / 'receipt.json').read_text())
    for name, expected in FROZEN.items():
        if hashlib.sha256((FIXTURE / name).read_bytes()).hexdigest() != expected:
            raise ValueError('frozen SDL V1 source changed: ' + name)
        matches = [v for p, v in receipt['source_sha256'].items() if Path(p).name == name]
        if matches != [expected]:
            raise ValueError('SDL V1 source receipt changed: ' + name)
    for name, expected in receipt['immutable_dependencies'].items():
        if hashlib.sha256((ROOT / name).read_bytes()).hexdigest() != expected:
            raise ValueError('SDL V1 dependency changed: ' + name)


def layout_source(candidate=False):
    """Extract exact old fields; no production source/header is modified.

    The mesh implementation include is unused by the object declaration. Removing
    it avoids compiling/printing renderer functions merely to measure constants.
    Keep actual bitmap/shadow type definitions, with byte-frozen dependencies.
    """
    verify()
    source = (FIXTURE / 'cui_sdl3.c').read_text()
    prefix, boundary, _ = source.partition('static cui_system_appearance system_appearance(void)')
    if not boundary or not prefix.endswith('};\n'):
        raise ValueError('SDL private layout boundary changed')
    prefix = prefix.replace('#include "cui_sdl3_mesh.h"\n', '')
    if candidate:
        tail = '    SDL_Cursor *cursors[CUI_CURSOR_HELP + 1];\n};\n'
        if prefix.count(tail) != 1:
            raise ValueError('SDL private tail changed')
        prefix = prefix.replace(tail, tail[:-3] + '    bool v2_busy, v2_faulted;\n};\n')
    fields = ['shadows', 'shadow_frame', 'appearance', 'window', 'renderer', 'font', 'ink',
              'text_paint', 'density_x', 'density_y', 'text_density', 'quit', 'glyph_ok',
              'ime_composing', 'ime_commit_pending', 'input_suspended',
              'ime_focus', 'input_state', 'input_active', 'glyphs', 'bitmaps', 'bitmap_bytes',
              'shadow_bytes', 'glyph_bytes', 'clock', 'hits', 'misses', 'frame', 'frame_ticks',
              'frame_clock_started', 'event_pending', 'callback_mode', 'motion_pending',
              'motion', 'pending_event', 'clipboard', 'cursors']
    if candidate:
        fields += ['v2_busy', 'v2_faulted']
    constants = ['const unsigned sdl_v1_size = sizeof(cui_sdl3);',
                 'const unsigned sdl_v1_align = _Alignof(cui_sdl3);',
                 'const unsigned sdl_v1_constructor_bytes = sizeof(cui_sdl3);']
    constants += [f'const unsigned sdl_v1_offset_{f} = offsetof(cui_sdl3, {f});' for f in fields]
    keep = ('void sdl_layout_keep_bitmap_declarations(void) {\n'
            '    (void)cui_bitmap_key_make; (void)cui_bitmap_key_equal; (void)cui_bitmap_raster;\n}\n')
    return prefix + '\n' + '\n'.join(constants) + '\n' + keep
