#!/usr/bin/env python3
"""TEST ONLY: stage fixed scrollbar nodes in an exact-seam Core copy.

Production inputs are read-only. Refuse unknown/multiple seams before replacing
output. This prototype changes only a build-directory copy, not vendor/Core APIs.
"""
from pathlib import Path
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/core/cui.c').read_text()
private = (root / 'src/core/cui_scroll_layout_internal.h').read_text()
patches = (
    ('    uint8_t order[CUI_CORE_SL_BODY_MAX];',
     '    uint8_t order[CUI_CORE_SL_BODY_MAX];\n'
     '    uint32_t track_count, track_reserved;\n'
     '    prototype_track_entry tracks[64];'),
    ('    uint64_t widths = cui_core_sl_widths(context);',
     '    if (!prototype_track_stage(ui, state))\n'
     '        return cui_core_sl_abort(ui);\n'
     '    uint64_t widths = cui_core_sl_widths(context);'),
    ('    Clay__CalculateFinalLayout(0, false, true);',
     '    Clay__CalculateFinalLayout(0, false, true);\n'
     '    if (!prototype_track_validate(ui, state))\n'
     '        return cui_core_sl_abort(ui);'),
    ('    state->header.flags = CUI_CORE_SL_READY;',
     '    prototype_track_publish(ui, state);\n'
     '    state->header.flags = CUI_CORE_SL_READY;'),
)
for needle, replacement in patches:
    if private.count(needle) != 1:
        raise SystemExit('Scrollbar track prototype seam changed: review required')
    private = private.replace(needle, replacement)
needle = '#include "cui_scroll_layout_internal.h"'
if source.count(needle) != 1:
    raise SystemExit('Core scroll adapter include changed: review required')
output.write_text(source.replace(needle, private))
