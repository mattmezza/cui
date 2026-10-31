# Core intrinsic scroll-body layout

Development source only: these APIs were added after v0.1.0-alpha.3 and are
absent from its published packages. Use matching headers and implementation.

`cui_layout_scroll_body_v1` and `cui_layout_scroll_clamp_v1` are opt-in C Core
constraints. Read their exact declarations in `include/cui/cui.h`. They do not
change the existing Scrollable/Base/Dialog APIs or expose foreign-builder slots.
Python, JavaScript, TypeScript, Go and Rust builder composition support remains
future work; the C API does not establish full Dialog or component parity.

## Supported relationship

A body registration names this exact current-frame chain, after all six boxes close:

```text
Panel (fixed positive current width)
  Outer viewport (clip)
    Column (ordinary vertical flow, gaps and fixed siblings)
      Body frame (ordinary flow)
        Body viewport (absolute attachment, clip inherited from frame)
          Content (intrinsic flow or explicit content height)
```

Panel/Outer/Viewport each have exactly the named single flow child. Frame has no
flow child; its attached viewport grows from the frame origin. Outer, Frame and
Viewport have zero padding; Outer/Viewport align content from the top. Column is
vertical and may contain headers, descriptions and footers alongside Frame. Compose these and
all content once. The relationship allocates current intrinsic content height to
the Body, capped by the panel's available height. No previous-frame extent or
minimum readable spacer is used. AUTO chooses the smaller of natural demand and
max_height, respecting min_height. EXPLICIT uses the validated requested_height.
When fixed regions exceed the available height, the Body exposes its natural
height and the whole column scrolls inside Outer. Both Y offsets clamp to the
completed frame's ranges before final drawing, hits, semantics and caret geometry.

Widths must be determined independently of the new Y allocation. Ordinary GROW
children under a fixed/constrained ancestor are supported; every child need not
have a literal fixed width. Hidden/overlay scrollbars preserve that width.
Reserved-gutter feedback is outside V1. Core cannot infer an application's gutter
policy; the caller must satisfy the width-independent contract. The adapter rejects
aspect-ratio/percentage-height dependencies and foreign floating roots in the
registered subtree, checks the stored-tree width fingerprint and final predicted
ranges, and fails instead of iterating. Width independence comes from these structural
constraints and stable provider/X inputs; equality of a 64-bit diagnostic fingerprint
is not a proof of arbitrary constraint closure. This is not a general solver.
Fixed-size absolute decorations, including caret boxes, may attach to their actual
parent with inherited clipping. Canonical explicitly fixed-zero axes stay zero.
Nested registered bodies solve inside-out. No portal/root fallback is inferred.
Ordinary Core portals detach from their lexical parent and remain independent
roots; they do not contribute intrinsic demand or inherit this clip relationship.
The constraint does not certify or change those separate roots.

A standalone clamp names a directly parented clipped viewport and its content,
with a fixed positive viewport height. It changes Y translation only, preserves
sizes and cannot contain a registered body. Standalone and body-owned identities
cannot overlap. X/mixed axes are rejected, not ignored. The APIs create no semantic
role or input handler: applications retain their existing behavior/scroll owners.
For an existing matching scroll semantic, Core updates its completed range/value.
Input routing continues to use the existing dispatch and preceding-frame geometry.

## Registration and ownership

The 80-byte `cui_scroll_body_layout_v1` and 32-byte `cui_scroll_clamp_v1` records
contain no pointers. Set struct_size to sizeof(record), version to
CUI_SCROLL_LAYOUT_VERSION_1 and all reserved fields to zero. Heights and initial
offsets must be finite; 0 <= min_height <= max_height and max_height > 0.
AUTO requires requested_height = 0. EXPLICIT requires a positive request between
min_height and max_height. Clamp axes must equal CUI_SCROLL_CLAMP_Y.

```c
/* After composing/closing the six boxes, using stable application-owned IDs. */
cui_scroll_body_layout_v1 layout = {
    .struct_size = sizeof layout,
    .version = CUI_SCROLL_LAYOUT_VERSION_1,
    .mode = CUI_SCROLL_BODY_AUTO,
    .panel = panel_id, .outer_viewport = outer_id, .column = column_id,
    .body_frame = frame_id, .viewport = viewport_id, .content = content_id,
    .max_height = available_panel_height
};
bool accepted = cui_layout_scroll_body_v1(ui, &layout, &model.outer_y, &model.body_y);
```

Records copy during registration and can leave scope. Borrowed writable floats
must remain aligned, valid and stable through end_frame, including inside child
callbacks. Distinct destinations cannot overlap each other, another constraint's
destinations or retained tracked-bounds rectangles. Alias checks repeat after
composition so later bounds registrations cannot evade the rule. Record storage
may overlap a destination because its values have already copied; the destination
itself must still outlive end_frame. Core cannot validate arbitrary C pointer lifetime.
Register every frame; there are no persistent links or replacement semantics.
Inert/layout-only ancestry receives effective geometry clamping without writing
borrowed model scalars or publishing synthetic change events.

At most 32 body links and 64 total clamps are accepted. Each body reserves two
clamp slots atomically; standalone clamps use remaining slots. Repeated links,
missing/ambiguous IDs, incorrect ancestry, open referenced boxes, conflicting
identities and aliases reject the frame. No last-writer-wins or silent fallback.

## Errors and phase behavior

Ordinary-composition registration opts the frame in even if it fails. Invalid
records/topology report INVALID_ACTION; identity ambiguity reports DUPLICATE_ID;
resource/link/clamp limits report CAPACITY; unresolved/unsupported final geometry
reports LAYOUT. Wrong-phase calls retain existing lifecycle behavior and do not
retroactively opt a frame in. The first sticky error remains until explicitly
cleared. Clearing within a failed opt-in frame cannot salvage it: end_frame then
reports LAYOUT if no earlier error remains, and returns an empty draw list.

An erroneous opt-in frame has no valid draw, prepaint, tracked-bounds or semantic
snapshot promise. Discarded trial geometry never reaches prepaint. All active
borrowed offsets commit together only after every final constraint validates;
failures before that commit preserve them. This transaction does not roll back
application/input mutations or a successful commit followed by a later prepaint
error. Clear the sticky error and rebuild on the next frame to recover. A frame
with no opt-in attempt keeps the original layout, callbacks and error behavior.

The adapter calls real Clay_EndLayout once, then one additional stored-tree
calculation before Core prepaint/conversion. Public composition, input dispatch
and prepaint do not repeat. Text measurement/shaping can repeat under their
existing provider contracts. Vendored Clay is unchanged; the private adapter is
pinned to the repository version and must be reviewed when that dependency moves.

Storage is lazy within existing Core resource budgets: one 12856-byte payload on
the measured LP64 build, plus an 8-byte opaque-context pointer even when idle and
one shared immutable 16-byte failure header. The track extension adds 5640 bytes
to the former 7216-byte layout payload, including for body/clamp-only opt-in users.
Applications with exact custom resource-byte budgets must allow this increase.
No public config or existing public
struct grows. Other ABIs may have less padding; a compile-time 16 KiB payload bound
and the same 32/64 capacities apply. No resource scan or extra calculation runs
on a zero-opt-in legacy frame. Resource omission/collection follows the existing
Core lifetime contract. Tests cover public APIs, fixed/narrow/wide/current-font,
nested/empty/oversized regions, once-only callbacks, scroll/caret coherence,
capacity/allocation failure, late aliases, context isolation and legacy behavior.


## Current-frame overlay track and thumb

`cui_layout_scrollbar_track_v1` optionally attaches a real track and passive thumb
to an already registered Y clamp. This C Core API does not install Base input
handlers or add Dialog/binding composition. `cui_scrollbar_thumb_geometry` is the
pure Core rectangle helper; the existing Base `cui_scrollbar_thumb_ex` forwards to
it with the same ABI, normalization and arithmetic order.

The copied pointer-free `cui_scrollbar_track_layout_v1` is 48 bytes: four uint32
fields (struct_size, version, flags, reserved), viewport/bar/thumb IDs at offsets
16/24/32 and thickness/minimum_thumb floats at 40/44. Use version 1, reserved zero,
finite positive thickness/minimum_thumb and at most one track per clamp (64 total).
The record may leave scope after registration. No new pointer lifetime is added;
the clamp's distinct borrowed model float still lives through end_frame.

Compose and close both named boxes before registering. Track must attach directly
to viewport; thumb directly to track. Both are no-flow absolute boxes with inherited
clipping, the parent's layer/focus scope, fixed width equal to thickness and fixed
positive placeholder height (1 is sufficient). No padding, aspect ratio, maximum
size caps or custom prepaint hook is admitted. Track has exactly one floating child
(the passive thumb); neither has flow children. Their actual IDs must be distinct
from registered structural identities, including folded collisions. No portal or
row fallback is inferred. The active track semantic must be SCROLLBAR, vertical,
with parent and controls equal to viewport; the thumb has no semantic. Under
inert/layout-only ancestry the normally omitted semantics are accepted.

Flags independently author VISIBLE (1) and INTERACTIVE (2); interactivity requires
visibility. Visible disabled bars retain ordinary paint/shadows and a disabled
semantic, with no capture/focus/input eligibility. INTERACTIVE is the authoritative
bar enabled policy and may replace the submitted bar.disabled field (including
a disabled range placeholder). To author a disabled bar, the caller MUST clear
INTERACTIVE: bar.disabled alone does not override an explicit interactive request.
Core cannot distinguish an arbitrary disabled bar field from a range placeholder.
The track readonly flag, disabled/readonly owner or actual
layout ancestors, inert and layout-only still exclude input. Callers must apply
current eligibility before invoking their existing reducers; publication cannot
retroactively undo earlier application/input mutations. Core does not provide a
new dispatcher or replay events. Existing input uses the preceding completed hit
snapshot; final current-frame paint/prepaint hits and published semantic geometry
use the actual final boxes. Visible style and input policy remain caller owned.

Hidden or zero-range tracks have zero effective height and no visible paint,
shadow halo or published semantic. Alpha-zero commands may remain in the draw
list. Inert ancestry keeps visible paint while suppressing semantics/input;
layout-only suppresses paint and leaves model scalars untouched. Independent
portals and existing modal eligibility retain their normal Core behavior.

Before the existing second calculation, Core derives local Y-only track/thumb
geometry from planned viewport height, range and staged offset. Width remains
independent of Y; overlay bars reserve no gutter. Final real boxes, clipping,
culling, paint/shadow identity and hit/semantic bounds then share one layout.
All tracks validate before any borrowed offset commits; malformed records,
aliases, unsupported topology, capacity and final failures invalidate the entire
opt-in frame under the rules above. Register afresh next frame; omission/reuse
retains no record or caller pointer. This adds no second resource, public config
field or context pointer beyond the existing opt-in layout allocation.

Tests compare 80000 direct-Core/Base results with a frozen independent legacy
math oracle, and exercise current resize/shrink, shadows/clip/alpha, queued wheel,
touch/drag, nested consumption, inert/layout-only/disabled/modal policies,
record copying, aliases, resource/capacity failure and next-frame recovery.
These are software/headless C oracles, not GPU, text raster or full component
parity claims. Base page/key/CANCEL and full Dialog/Root/all-five facade composition
remain separate followups.

## V2 source-style Content/Body peers and separate Footer

`cui_layout_scroll_body_v2` is additive C Core, not Dialog/Base/facade composition.
Its layout-record version2 is independent of draw ABI2/background negotiation.
It works in ordinary `cui_begin_frame`/`cui_end_frame` frames with V1 draw lists,
as the source/software oracles exercise. It requires no new renderer support
and does not claim that a renderer supports draw ABI2.
Read the144-byte `cui_scroll_body_layout_v2` and24-byte
`cui_scroll_region_sizing_v1` in the same authoritative public header. The embedded
base header is size144/version2; its height fields are the sole Panel sizing
authority. Passing that record to the old V1 function is invalid. Existing V1
geometry, input, callbacks, memory allocation and resource slot budget remain.

```text
Panel (fixed positive width; optional vertical padding/gap)
  Outer (clip, zero padding)
    Column (vertical, automatic gap0)
      fixed authored siblings / conditional fixed spacers
      Content region (optional, ordinary direct Column child)
      conditional fixed spacer only when Body is authored present
      BodyFrame -> attached clipped BodyViewport -> intrinsic BodyContent
  Footer (optional ordinary direct Panel child after Outer, fixed/FIT height)
```

All six internal base boxes remain, even for Body OMIT, and all referenced boxes
must close before registration. Content ID0 iff its policy is OMIT. Footer0
means absent. Content must precede Frame; ordinary parent links, full/folded ID
distinctness, inherited-clip floating rules and independent width restrictions
remain. No row/root/portal fallback is inferred. Footer is the only optional
second flow child of Panel. Column automatic gap must be0: explicit fixed spacer
boxes between authored present regions preserve conditional omission exactly.
Horizontal padding belongs in the content subtree; viewport zero-padding remains.

FLEX1 has zero basis and shares available Y with the other FLEX1 peer, freezing
against explicit/intrinsic minimum and maximum. It is not always equal halves.
FIT takes clamped current intrinsic demand. FIXED takes finite positive requested
height within min/max; FLEX1/FIT requested_height0. Non-OMIT min finite>=0,
max finite>0, min<=max; use FLT_MAX for uncapped. Reserved0. OMIT is all-zero policy
and receives0 allocation. For source Body omission, author an empty zero-height
internal content subtree and no Body callback/semantic/input owner. An explicit
empty FLEX1 Body remains a participating region and can receive positive height.
Core does not infer omission from callback effects or remove caller semantics.

AUTO is current intrinsic Panel demand, subject to its min/max; EXPLICIT is its
validated height. Footer demand includes its actual children and retains that
height outside Inner. Content minimum overflow is clipped by Inner; Outer range
stays0. Oversized Footer can extend beyond Panel and is subject to ordinary
ancestor/window clips, not a promise of full visibility or wheel reachability.
There is no automatic whole-Outer scrolling exception. For example, the pinned
independent engine/CUI software fixture with Panel260, padding16, gap16 and
Footer400 gives Inner0 and FooterY32/bottom432. AUTO max600 with the short-body
fixture fits Panel562 instead. This is geometry/software evidence, not GPUI GPU
raster or full Dialog input parity. Existing V1 reachable-Outer policy is unchanged.

Public record/policies copy on registration; no caller record pointer is kept.
Distinct aligned writable offsets and tracked bounds storage remain stable
through end_frame. All shared32 bodies/64 clamps/64 tracks, late-alias checks,
no-partial-commit and failed-frame recovery rules above apply. No third layout,
application callback replay or new dispatcher. Text measurement may repeat.

V1-only uses the EXACT existing base resource size/callback/key and one slot
(LP6412856 bytes). V2 adds one2560-byte sidecar and one slot, shared across all
V2 registrations: LP6415416 total, below16KiB. Other pointer-size targets retain
their existing V1 base size plus the same2560 sidecar. No new opaque-context field,
zero-opt-in allocation or resource scan. Cached slot access is checked and O(1);
wrong/stale/missing V2 cache fails the opt-in frame, never falls back to V1.
Sidecar/base collect independently at next begin. After V2 omission the sidecar
retains the ordinary one-complete-frame resource history before destruction;
this is explicit V2 history cost. Exact V1 byte/one-slot budgets still admit V1.
V2 with one slot or one byte short fails ordinary CAPACITY, retains models, and
can recover to V1 next frame after clearing the error. No public config grows.

The28-row pinned Taffy reference is test-only/offline, not a CUI dependency.
`dialog_flex_regions_native` and `_distribution` exercise public production APIs,
software clip/overflow pixels, current prepaint/semantic/caret geometry, same-UI
resize/omission and resource/alias/failure controls. Core V2 is a foundation;
full Dialog slots/actions, Root/native-host and all five builders remain open.
