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
    float reference_rem; /* zero defaults to 16; otherwise finite positive */
    float offset;       /* signed logical pixels; used when custom_offset */
    bool custom_offset, arrow;
} cui_popover_position;
```

Zero mode must preserve current placement byte-for-byte and ignore new fields.
Anchor mode defaults to TopLeft, offset reference_rem*0.25 and arrow off.
Zero reference_rem selects 16 logical pixels; active nonzero values must be finite
positive. Arrow depth is reference_rem*0.375. Applications configure reference_rem
from their host's root sizing configuration, never by inferring theme font size.
Explicit offset 0 is distinct from default; finite negative offsets are supported.

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
completion: use context-owned frame storage, never retained stack arrays. Current concrete
path: `cui_set_prepaint_parts` copies payload plus tail into Core frame storage
(`cui_core_frame_copy_parts`, `src/core/cui.c:2811`); a prepaint callback reads
its copied payload/tail and emits a borrowed mesh pointing into those copied
vertices through `cui_prepaint_emit`. This allocation survives until next begin
frame, as does draw data. Tail alignment must be proved (payload size multiple
of vertex alignment) and callback budget/capacity rollback tested before Styled
arrow work. No new Core allocation API is currently needed. Render arrow
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

Integrator decisions: accept separate opt-in mode; accept explicit reference_rem with zero default16;
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

## Accepted sequencing after integrator review

The first implementation commit contains standalone public types/helpers and pure base geometry
plus headless and WASI coverage only. It does not append the proposed options
field or connect helpers to surface placement yet. Styled arrow paint is deferred until the
prepaint copied-storage path above is tested. Canonical/generated metadata updates
are deferred to integrator coordination; no shipped behavior parity claim yet.

## Styled consumption contract (second slice)

Keep `cui_surface_options` and `cui_surface_state` unchanged. Add
`bool cui_popover_begin(cui_context *, const cui_theme *, cui_surface_state *,
cui_str title, cui_str description, cui_surface_options, cui_popover_position)`.
It requires a Popover state; callers balance successful begins with existing
`cui_surface_end`. Mode LEGACY delegates the existing surface begin exactly and
ignores every other position field. No other surface kind is affected. Mode
ANCHOR resolves placement afresh from the supplied trigger every submitted frame;
width/height <=0 use420/260, finite positive dimensions retain their size even
when larger than usable viewport. Invalid active geometry rejects before overlay
update/begin. Pure `cui_popover_surface_geometry(options,position,out)` shares
those defaults and legacy delegation, with untouched output on failure. There is
no binary struct ABI change; clients link the new symbols only when using them.

Implementation will factor the existing Styled begin into an internal routine
with optional prevalidated explicit bounds and arrow payload. Existing public
begin takes the identical legacy path; Base's geometry remains unchanged for all
existing callers. No focus/event/semantic code is replaced. Decoration has no ID,
hover hitbox, semantic entry or layout effect, and outside dismissal continues to
use only the panel rectangle.

Build fixed vertex arrays before submission, embedded directly in one copied
prepaint payload struct. In its callback reconstruct mesh pointers into that
copied struct, never retain pointers to caller stack data. Register on the panel
portal and use deferred stream at the panel layer, after its border/children and
before later layers. The stream inherits panel opacity and Core's host viewport
clip. Clip arrow triangles to `options.viewport` in portable geometry before
emission (fixed polygon buffers, no caller clip commands); intersect the join
rectangle too. Panel placement retains existing portal clipping behavior. Frame arena failure rolls back registration
and Core suppresses the final draw list; balance any already-opened internal
boxes before returning false. Public successful begins retain their usual end
contract, including a later frame-level capacity failure.

Arrow fill uses resolved refined background except appearance NONE without an
explicit background refinement uses theme popover. Outline is present for
LEGACY/SOURCE appearance, absent for NONE, uses theme text with alpha26/255,
and stays1px independent of refined border. Triangle and join follow the pure
source geometry; arrow corner inset uses theme radius. This slice does not claim
pixel equivalence with GPUI's external PathBuilder stroke tessellator/AA.
Portable outline uses two joined 1px slope strips, clamping miter displacement
to2px; no base stroke. Triangle-vs-path raster/AA remains a documented gap. Any
portable outline mesh topology difference must remain documented, and native GPU
hardware evidence unavailable under Xvfb stays deferred.

## Input-independent visual paint correction

The original deferred stream incorrectly conflated modal input eligibility with
visual visibility. Core intentionally denies deferred emission outside the modal
input scope; inert boxes explicitly retain painting. Arrow paint now owns a
detached, inert decoration portal (surface part8) covering the supplied viewport
at the panel layer. Local prepaint clips to that viewport and host bounds and
remains visible when inert or behind another modal. The portal has no semantics,
hover hitbox or input ownership. Floating-root order places its complete local
stream after panel border/content and before higher layers. Copied payload and
capacity rollback rules are unchanged. Existing order/clip/nested/failure tests
plus inert/modal pixel and input-suppression regressions prove the correction.
