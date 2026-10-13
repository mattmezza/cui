# Sidebar ancestral clipping

Scrolled sidebar SVG icons could overlap the fixed header/footer even though the
body viewport already clipped ordinary rows and text. The pinned Clay renderer
paints absolute descendants as separate floating roots. Its synthetic scissor
command has `userData == NULL` and the bounds of only the nearest clipping
ancestor (often a menu row). The body scissor belongs to another root and is no
longer active when that icon is painted. Intersecting the renderer's current clip
stack alone therefore cannot recover the missing body constraint.

Core draw conversion now resolves that synthetic command through the pinned
Clay floating-root metadata and its existing derived command ID. It intersects
the nearest clip bounds with all clipping ancestors recorded in the CUI node
chain. Ordinary scissor commands receive the same intersection. Clip push/pop
structure, draw order and icon bitmap geometry remain unchanged; a completely
hidden descendant receives a zero-area clip. No backend, Sidebar layout, public
API or vendored Clay changes are required.

Explicit detached portals retain their escape behavior: their CUI parent chain
ends at the portal root, so descendants inherit clips inside the portal but do
not inherit the original sidebar/body clip. Absolute children remain attached
and inherit the full chain. This follows the same ancestry already used for CUI
hit bounds, bringing icon paint clipping into agreement with hit testing.

## Regression evidence

`test_sidebar` renders actual registered folder SVG bitmaps through a real
Sidebar with fixed header/footer, ten menu rows and an expanded nested folder.
It samples nonzero source alpha at destination pixel centers under the emitted
clip stack and rejects any visible icon pixel outside the body viewport. It also
checks visible text bounds and clicks hidden rows above/below the viewport.
Expanded and icon-collapsed sidebars are tested at two scroll positions. The
final test fails against the original `ddd0ae3` Core at the assertion that a
visible folder pixel lies within the viewport's vertical extent, and passes with
the conversion fix. A rect-only regression would miss this separate-root case.

`test_core` independently checks an absolute child nested under an oversized
clipping row: its effective clip must equal the outer viewport intersection.
The same scene opens a detached portal and an absolute child inside it outside
the original viewport, and verifies that its own clip remains visible.

The browser regression builds the actual WASM gallery and tracks the Files
folder bitmap's nonzero alpha through its emitted clip stack while scrolling in
expanded/collapsed modes. It checks top/bottom leakage without treating the
fixed header toggle or footer settings icon as body content:

```sh
python3 scripts/build_web.py --output build/web
uv run tests/web/test_sidebar_clipping.py --directory build/web --chromium /path/to/chrome
```

Validation on this branch: strict-warning native build; 93/94 CTests pass (the
only failure is the expected stale generated `dist/cui.h`, owned by the
integrator); ASAN/UBSAN Core and Sidebar tests pass; the browser regression
passes against pinned Zig 0.16.0 output and Chromium. The integrator must
regenerate the distribution header before publication. The test requires
Playwright 1.58.0 and an installed Chromium fixture; it does not install a browser.

The metadata lookup intentionally targets the pinned Clay implementation already
used privately by Core. A Clay version update must retain or revise the
synthetic-scissor lookup and rerun these paint-stack/pixel and detached-portal
regressions. This fix does not change viewport sizing, virtualization estimates,
row layout, portal focus behavior, or scrolling policy.
