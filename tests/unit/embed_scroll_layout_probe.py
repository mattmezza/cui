#!/usr/bin/env python3
"""Instrument only phase observations in a build-directory Core copy."""
from pathlib import Path
import sys

root, output = map(Path, sys.argv[1:])
source = (root / 'src/core/cui.c').read_text()
private = (root / 'src/core/cui_scroll_layout_internal.h').read_text()
for needle, replacement in (
    ('    uint64_t widths = cui_core_sl_widths(context);',
     '    probe_before(ui);\n    uint64_t widths = cui_core_sl_widths(context);'),
    ('    Clay__CalculateFinalLayout(0, false, true);',
     '    Clay__CalculateFinalLayout(0, false, true);\n    probe_after(ui);'),
):
    if private.count(needle) != 1:
        raise SystemExit('Scroll layout probe seam changed: review required')
    private = private.replace(needle, replacement)
needle = '#include "cui_scroll_layout_internal.h"'
if source.count(needle) != 1:
    raise SystemExit('Core scroll adapter include changed: review required')
output.write_text(source.replace(needle, private))
