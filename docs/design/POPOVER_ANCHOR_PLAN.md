# Popover anchor, offset and arrow slice (proposal)

Design only; integrator review required before implementation. Baseline CUI
`4eb9aab8`; frozen GPUI reference `87d10ae5e1299d1be18670a37570c5402f952e34`.
This proposal makes no present parity claim. Transitions, right-click activation,
focus and controlled state are outside this slice.

## Source authority and present gap

Reference paths below are relative to `/tmp/cui-gpui-reference` at that commit:

* `crates/component/src/popover.rs`: `new`, `anchor`, `offset`, `arrow`, `render`
  (lines 145–438), `arrow_anchor`, `arrow_points`, `arrow_join_bounds`
  (445–498), tests 596–891. Component default anchor is TopLeft, gap is
  rem*0.25, arrow depth is rem*0.375; arrow depth is added to the surface gap.
* `crates/base/src/popover.rs`: options and render forwarding (175–223, 320–326).
* `crates/base/src/popup.rs`: live `anchor_position` (103–113), measured trigger
  tracking and corner positioner (133–183). `resolved_corner` is an older helper;
  it is NOT the live trigger geometry contract.
* `crates/base/src/positioner.rs`: corner strategy (98–107), `resolve`
  (211–239), `clamp` (303–325), native client-frame inset integration (396–420).
  Corner mode clamps without flipping, retains popup size and defaults to 4px
  margins plus native client-frame insets. Final painted displacement is rounded.

CUI `src/base/cui_surface.c:cui_surface_geometry` calls `cui_popup_place`
with fixed gap 6; `src/base/cui_models.c` tries an opposite side, selects lower
overflow, shrinks to viewport and clamps. Surface options expose only trigger
rectangle and side. CUI has appearance refinement but no anchor selector, offset
or arrow. Changing shared `cui_popup_place` would change other consumers.

## Proposed portable public contract

Append to `cui_surface_options` a copied `cui_popover_position position` field;
consume it only for Popover. Add types to `include/cui/cui_surface.h`:

```c
typedef enum {
    CUI_POPOVER_POSITION_LEGACY = 0,
    CUI_POPOVER_POSITION_ANCHOR
} cui_popover_position_mode;
typedef enum {
    CUI_POPOVER_TOP_LEFT = 0, CUI_POPOVER_TOP_CENTER, CUI_POPOVER_TOP_RIGHT,
    CUI_POPOVER_BOTTOM_LEFT, CUI_POPOVER_BOTTOM_CENTER, CUI_POPOVER_BOTTOM_RIGHT,
    CUI_POPOVER_LEFT_CENTER, CUI_POPOVER_RIGHT_CENTER
} cui_popover_anchor;
typedef struct {
    cui_popover_position_mode mode;
    cui_popover_anchor anchor;
    float offset;       /* signed logical pixels; used when custom_offset */
    bool custom_offset, arrow;
} cui_popover_position;
```

Zero mode must preserve current placement byte-for-byte and ignore new fields.
Anchor mode defaults to TopLeft, offset 4 and arrow off at the established 16px
reference rem. Explicit offset 0 is distinct from default; finite negative offsets
are supported as upstream accepts signed Pixels. Arrow depth is 6px. Document
this fixed-rem limitation (source is rem-relative); do not silently infer rem from
theme font size. A future host rem parameter is a separate API decision.

TopLeft/Center/Right put popup top left/center/right against trigger bottom
left/center/right. Bottom variants put popup bottom against trigger top.
LeftCenter puts popup left-center against trigger right-center; RightCenter does
the reverse. Offset moves outward on that side; arrow adds depth before clamp.
Arrow points at the corresponding trigger point after surface clamping, never
at trigger center unconditionally. Do not flip or reuse side placement in anchor
mode. Existing `side` is ignored there; existing rectangle `anchor` stays the
trigger bounds and must not be renamed.

Add a pure bool geometry resolver with output untouched on invalid input:
`cui_popover_position_resolve(cui_rect trigger, cui_size size, cui_rect viewport,
float margin, cui_popover_position, cui_rect *out)` for ANCHOR mode only.
Retain existing surface width/height defaults, but in ANCHOR mode do not shrink
explicit popup dimensions before clamp: oversized surfaces settle at viewport
minimum plus margin, matching corner clamp. A viewport represents usable client
bounds; source native non-client/tiled-edge inset handling is host responsibility
and not claimed here. Caller margin stays explicit (zero means zero); source-like
examples set 4. Do not introduce platform or Clay public types.

Add `typedef struct { float x, y; } cui_popover_point;` and pure arrow geometry
output (three `cui_popover_point` points), join rectangle,
and source-facing edge, computed from final surface, trigger, anchor, depth and
resolved radius. Port source half=min(depth,edge/2),
inset=min(radius+half,edge/2), target clamp exactly. Join uses 1px stroke and
min(stroke,half-base) inset. Resolve fill from effective refined background;
NONE without background refinement falls back to theme popover. Outline uses
theme popover ring only when appearance is enabled, independent of border
refinement. Match source theme-radius arrow inset (not refined radius) unless
review explicitly chooses a documented departure.

## Architecture and review decisions

Geometry and validation belong to base; component owns themed arrow drawing and
join ordering. Core's portable mesh paint can draw triangles/slopes without new
renderer protocol, but borrowed mesh vertices must remain alive through renderer
completion: use context-owned frame storage, never stack arrays. Render arrow
after panel border/shadow and before/after children as dictated by source overlay
canvas; its join must cover the panel border on both sides. Verify portal clip
and ordering with draw-list assertions before choosing storage/layout changes.
Keep arrow decoration outside layout sizing, hit targets and accessibility tree;
source popup dismissal tracks surface bounds, not triangle tip. Arrow must not
create a focusable element or alter existing event routing.

Adding fields is source-compatible for zero/designated initializers but changes
the public struct ABI; rebuild C consumers. Bridge/binding exposure requires
separate review of versioned fixed structs: never append blindly to bridge ABI.
Until implemented and tested, bindings must explicitly report these options as
C-only. Reject invalid enums/nonfinite active geometry before begin mutates state;
preserve outputs and balanced begin/end behavior on rejection. Define overflow
validation and degenerate zero-size trigger acceptance explicitly in headers.

Integrator decisions: accept separate opt-in mode; accept fixed 16px-rem values;
accept oversized/no-shrink source mode; confirm source theme-radius inset and
appearance NONE fill fallback. These are review decisions, not implemented facts.

## Required evidence before parity metadata changes

1. Table-driven headless pure tests: all eight anchors, offsets default/0/positive/
   negative, arrow off/on, nonzero viewport origins, clamping every edge/corner,
   oversized panels, tiny arrow bases/radius, trigger movement and invalid finite/
   enum cases. Golden values derive independently from frozen source tests and
   equations. Assert no flipping even where legacy would flip.
2. Regression geometry tests lock old zero-mode gap 6, side flip/shrink behavior,
   legacy appearance, and non-Popover behavior with garbage inactive new fields.
3. Software draw-list/pixel tests verify each arrow direction, anchor-aligned tip,
   seam coverage, effective fill/NONE fallback, outline toggle, tiny surfaces,
   edge clipping, z-order and frame-storage lifetime across nested popovers.
   Exercise actual begin/end and moving/resized trigger, not only helper math.
4. Reuse appearance native fixture/browser probe pattern for actual rendered
   arrows: native via private Xvfb wrapper only, browser headless; verify software
   and supported native draw adapters. GPU hardware behavior unsupported by Xvfb
   remains explicitly deferred. No desktop launch, dependency installation or
   runtime-network tests.
5. Update canonical component feature/limitation metadata, relevant consumer
   guidance and example; preserve all 78 manifest entries and exact authoritative
   header indexing. Run `python scripts/generate_agent_skill.py` and
   `python tests/test_agent_skill.py`; keep binding coverage truthful.
6. Configure/build with warnings and run
   `TMPDIR=/home/matteo/.cache/cui-validation/video-tests-tmp python scripts/test_local.py --build-dir build`.
   Record native/browser/backend results separately. Only then describe anchor,
   offset and arrow slice coverage; do not claim complete Popover parity.

This design-only commit changes no behavior and requires no native launch.
