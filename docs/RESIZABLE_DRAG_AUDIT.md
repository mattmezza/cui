# Resizable visible-divider regression

The reported failure reproduced on the actual HTTPS gallery revision `44819c1`
at a 1920 × 1080 viewport. Normal desktop mouse and actual Chromium CDP touch
contacts began on the visible divider. Both pane bounds stayed at Explorer
300 px and Editor 600 px, with zero width change after dragging. Before/after
screenshots and pane rectangles were captured in `/tmp/cui-resizable-wide` during
the audit. At 1280 px, the same gestures worked (212 → 260.5 → 300 px), explaining
why previous narrower browser tests missed the problem.

The gallery configured hard maxima of 300 and 600 px while its responsive group
could be wider than their combined 900 px. Both panes reached their maxima. A
split conserves space by transferring it between panels; neither pane had any
capacity to accept the transfer. The model correctly returned `INFEASIBLE` and
preserved the declared constraints. This was gallery configuration, not a mouse
capture or Core touch routing defect.

The Explorer keeps its 80–300 px range. The Editor now has maximum capacity equal
to the responsive group extent, so it can fill and accept transfers at large
widths. Vertical mode starts with a balanced 105/105 px split in its 210 px group;
changing orientation resets that story's split to the appropriate initial sizes.
Resizing within the same orientation retains user state. Both minima remain 80 px,
and normal movement stops at a bound. The underlying `INFEASIBLE` policy, callback
semantics, capture and touch drag priority are unchanged.

## Strict browser checks

Run against a locally built gallery:

```sh
uv run tests/web/test_resizable_drag.py --directory build/web \
  --chromium /path/to/chromium
```

Without `--directory`, the script uses the actual HTTPS gallery. It fails on any
geometry assertion. It uses DOM controls only to configure the story, real mouse
input for desktop cases and `Input.dispatchTouchEvent` for touch cases; it never
calls reducer exports to resize. The touch cases focus the keyboard fallback
with a real touch tap.

The matrix contains 24 independent cases: 390/1280/1920 px viewport widths ×
horizontal/vertical × comfortable/compact density × mouse/touch. Each asserts
actual Explorer growth, Editor shrinkage and boundary movement, enabled content
following horizontal pane widths, disabled geometry remaining fixed, and arrow-key
resizing. The wide cases save actual screenshots before/after. The JSON report
records pane rectangles. Resizable does not expose a size variant or rounded
option in its API/story; these were not fabricated as tested selectors. The
integrator is handling separate gallery option presentation.

## Headless and sanitizer checks

`tests/unit/test_resizable.c` adds eight actual styled split scenes: narrow/wide
extent × both orientations × mouse/touch. They assert changed panel bounds,
content bounds following the panel, conserved total extent, and exactly one
completion callback on release. Existing disabled, keyboard, overlays, clipping,
nested geometry and model constraint tests remain intact. These verify the
responsive configuration contract; the pre-fix production failure is the strict
HTTPS wide browser reproduction, not a claim that valid Core constraint behavior
needed changing.

Warning-enabled native build passed. Full CTest passed 93/94, with only the
root-owned generated-distribution check stale after the public header comment.
Resizable and base-only Resizable passed ASAN/UBSAN (2/2). The fixed local strict
browser matrix passed 24/24; the existing component touch suite passed 42/42
and rendered all 78 stories. Headless Chromium/CDP does not establish physical
mobile device coverage or every consumer's custom appearance/constraint setup.
