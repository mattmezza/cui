# Bounded current scrollbar track geometry V1

Proposal only, based on `6c69bb1d` plus Dialog checkpoints `a90319fb`,
`e0942271` and `a605a5ea`. No production, public header or vendor edits.
Coordinate any implementation base with root and Stepper's current Core V2 work;
this document does not authorize cherry-picking old Core implementation files.

## Decision and source seams

Recommend staging real fixed no-flow **track and thumb nodes** before the existing second
calculation, rather than introducing semantic-only geometry. Register a bar
semantic with the SAME ID as its real track box, associated with one already
registered Y clamp. Core owns current track/thumb allocation and range-derived
semantic eligibility; Base continues input reduction; components own track/thumb
paint and skin. No new calculation, observer, input dispatcher or callback pass.

Actual Core paths: `src/core/cui_scroll_layout_internal.h` stages heights/offsets,
resets scratch, calculates once more, validates all plans, then commits offsets
and current viewport scroll semantics. `cui_core_prepaint_prepare` builds geometry
from real Clay boxes and uses named semantic/layout matching for occlusion.
End-frame tracking and semantic publication separately query real Clay bounds.
A real correctly staged track therefore feeds all three existing consumers without
three parallel override paths. `cui_bounds` during submission still means previous
semantic bounds; no new current query promise is attached to it.

Semantic-only alternative: a record would map bar semantic to owner geometry, but
prepaint occlusion currently requires the semantic ID's own Clay layout element;
final semantic bounds/clipping and parent/order also use the layout node. Updating
only semantic.bounds is insufficient. A sidecar would need coordinated overrides
in prepaint, final bounds, previous_hits, parent/layer/order, tracking and shadow
ownership. This is wider and risks presenting a fake bar layout/paint owner.
Reject it for V1. No general semantic rectangle setter is proposed.

Fixed-decoration alternative avoids those overrides. Existing admission already
permits fixed-width/fixed-height parent-attached, inherited-clip zero-flow children.
The missing capability is narrowly staging submitted track/thumb fixed geometry
from a registered viewport's allocated dimensions before final positioning.
The box stays fixed during both calculations; it is never an unadmitted GROW
floating child or a flow child that corrupts intrinsic content height.

## Exact API and shared geometry primitive

```c
#define CUI_SCROLLBAR_TRACK_LAYOUT_VERSION_1 1u
#define CUI_SCROLLBAR_TRACK_LAYOUT_V1_BYTES 48u
#define CUI_SCROLLBAR_TRACK_VISIBLE 1u
#define CUI_SCROLLBAR_TRACK_INTERACTIVE 2u
typedef struct {
    uint32_t struct_size, version, flags, reserved;
    cui_id viewport, bar, thumb;
    float thickness, minimum_thumb;
} cui_scrollbar_track_layout_v1;
bool cui_layout_scrollbar_track_v1(cui_context *,
                                  const cui_scrollbar_track_layout_v1 *);
/* Pure Core geometry, callable outside frames; legacy normalization preserved. */
cui_rect cui_scrollbar_thumb_geometry(cui_rect track, float scroll,
    float viewport, float content, bool vertical, float minimum);
```

Exact record48, IDs16/24/32, floats40/44; unknown flags/reserved/version fail.
INTERACTIVE requires VISIBLE; permitted states are0, VISIBLE, or
VISIBLE|INTERACTIVE. These are authoritative authored paint/input policies,
not inferred from the placeholder semantic's prior-range disabled field.
Both floats finite positive. Core primitive adopts the existing complete pure
body of Base cui_scrollbar_thumb_ex, including finite/positive fallback12,
viewport/content fallback, clamp, degenerate-length zero rectangle and operation
order. Base keeps its existing symbols/signatures; cui_scrollbar_thumb_ex becomes
a delegating wrapper, and cui_scrollbar_thumb still passes12. Move the formula,
do not copy it in two layers. Core has no Base dependency; both Core layout and
existing Base drag math call the one Core primitive. Horizontal legacy calls stay
supported by that pure primitive although the new layout record is Y-only.
Strict layout descriptor floats are validated separately; legacy normalization
behavior is not changed to satisfy the new descriptor's stricter admission.

## Registration, failure and compatibility

Ordinary frame submission only: after both boxes and the viewport/bar semantics
are submitted and closed, AFTER the associated Y clamp registration. Record is
copied immediately, no borrowed new pointer. Enclosing inert/layout-only may
legitimately suppress current semantic submission under existing Core behavior;
real boxes and correct topology remain mandatory, but absent semantics there are
normal. Active ancestry requires the exact owner/bar semantics. Never synthesize
input eligibility for inert or measurement-only trees. Existing Core80/32 record ABI and
old public structs remain unchanged. Full/folded nonzero ID checks apply at
registration AND end_frame, including cross-record and late duplicates. One bar
per viewport, one record per bar;64 records maximum, no dedup/replacement.
Wrong role, missing node/semantic/clamp, portal/wrong-parent topology or alias
is an error. Zero-range/hidden is normal final omission, never missing-node
fallback. Each bar semantic controls/parent=viewport, orientation=VERTICAL and
role=SCROLLBAR. Passive thumb has no semantic of its own. Core accepts explicit
IDs; components use existing public cui_scrollbar_id, no copied derivation.

Allocation/capacity/topology/geometry failure opts an ordinary frame into landed
Core failure semantics BEFORE scalar commit: non-OK end_frame even if caller
clears its first error, empty draw, no valid prepaint/geometry/snapshot promise;
no partial model commit. Wrong-phase calls retain existing lifecycle behavior.
Shared failure sentinel stays immutable. Next clear/begin/rebuild recovers, with
no input or observer rollback. Later downstream prepaint failure does not roll
back an already successful scalar commit. References live through end_frame
under existing Core lifetimes; no retained descriptor or global registration.
Zero opt-in frames and legacy bars without this record keep existing behavior.

## Fixed widths and authored topology

Width cannot be min(thickness,W/2,H/2) staged after first calculation, because H
may depend on allocated Y and width fingerprints include all decorations. Record thickness is the already-resolved explicit fixed width
of BOTH track and thumb, equal at registration and unchanged across calculations.
It must fit the current independently constrained viewport width. Components may
resolve thickness from the explicit horizontal constraint before composition;
no allocated-height width cap. Extremely short viewports can have a track narrower
or wider than their height; thumb length clamps to track height. This is an
explicit new-route overlay policy, not a claim to preserve old height-based
bar-thickness capping. Old bars without a record keep their current policy.

bar has one direct zero-flow thumb child, no flow children or other descendants.
Both boxes are FIXED in both axes, use fixed-positive1 height placeholders, inherit
parent clips/layer, have no padding/aspect/percentage/portals or application
prepaint callbacks. bar attaches to viewport; thumb attaches to bar. Existing
public absolute-box authoring supplies the actual IDs and topology. thumb has no
semantic/input identity of its own: the full track is the existing Base SCROLLBAR
control. Core validates every full/folded ID and all cross-record/structural
aliases; real nodes remain actual paint owners. An authored empty/disabled track
still supplies both nodes/semantic so omission can be resolved from CURRENT range.

Box background/border/radius and existing named box shadow APIs can paint/style
each real track/thumb node. They are actual Clay fixed boxes, so ordinary culling,
geometry, shadow bounds, own clipping and inherited opacity all consume the final
actual dimensions. No viewport-owned paint is advertised as thumb-owned shadow.
There is no production bar prepaint hook required for ordinary skins. Style is
resolved before authoring these two boxes, including fade on the common track
ancestor exactly once; thumb inherits it. Custom prepaint callbacks are excluded
from this narrow V1 topology rather than silently invoking them with synthetic
geometry or adding another observer. Existing box shadow capacity/error rules
apply; they are not a second shadow ownership architecture.

## Staging and omission

After all body/standalone heights and offsets are staged, take planned viewport
W,H, range M and clamped offset Y. Fixed bar width B=record.thickness. For VISIBLE
and M>0 and H>0, stage bar height H and attachment offset W-B,0. Compute a local
thumb rectangle through the SAME Core primitive with track=(0,0,B,H), scroll=Y,
viewport=H, content=H+M and minimum=minimum_thumb. Stage thumb height and attachment
Y from that rectangle, fixed width B and X0. Validate finite H+M/intermediates.
No width changes, new flow child, measurement callback or additional calculation.

For hidden/zero-range omit both heights (zero), keeping widths unchanged. Mark
bar semantic for omission before prepaint hit testing and final publication.
Additionally suppress effective paint opacity on BOTH real decoration nodes for
this frame, including their named shadows, so a shadow on a zero-size box cannot
leave a phantom halo. This is opt-in range-derived paint eligibility, not mutation
of the authored skin or persistent shadow resource. Next frame reauthors nodes
and recomputes effective opacity normally. Inert preserves existing inert paint
semantics (visible but noninteractive), layout-only suppresses all paint/input as
usual; do not conflate inert with hidden. VISIBLE without INTERACTIVE preserves
full track/thumb paint, shadows and a disabled SCROLLBAR semantic with actual
bounds/range/value; it accepts no capture, focus, keyboard, pointer or touch drag.
VISIBLE|INTERACTIVE permits the ordinary role behavior under existing modal,
clip and enclosing inert/layout-only restrictions. flags0 hides paint and omits
the semantic. Zero range/height omits both regardless of bits. Existing Core
disabled-role filtering retires capture/focus through its normal publication
rules; no blanket range-derived enabling may override authored noninteractive
policy. Enclosing inert suppresses semantics/input but retains the same paint;
layout-only suppresses both as it already does.

Final actual bar/thumbnail rectangles must match the staged relation after the
single second calculation. Expected clipping is the actual inherited viewport
clip and ancestor clips, not a synthesized same-size viewport root. Compare
nonzero scroll on owner and ancestors explicitly: owner scroll must not move its
own bar; ancestor scrolling must move owner/bar together. If the pinned floating
clip-childOffset behavior fails that relation, reject/prototype the narrow
anchoring fix before approval. Do not compensate from old bounds or silently
allow a third layout. All validation precedes all scalar model commits.

Metadata updates use real nodes only: semantic value=Y, min0/maxM, controls/parent
viewport, actual bar bounds for both current occlusion and completed input; thumb
is passive paint. Parent/layer/order, tracking, painted bar culling and shadows
remain ordinary Core paths. No per-consumer rectangle override is necessary.
Projected records do not grant style access to nonexistent child parts elsewhere.

Phase order is strict: ordinary input/composition once -> existing first layout
-> validate every record and derive ALL planned body/clamp/track/thumb scalars ->
reset existing render/wrap scratch as already audited -> existing second layout
-> validate actual rectangles, topology, aliases, finite destinations and all
ranges -> atomic active offset commit and current range/value metadata -> omit
ineligible track semantics/effective paint -> ordinary prepaint conversion ->
ordinary tracking/final semantic publication. No allocation, event consumption,
application callback or model write occurs in the scalar-staging/validation steps.
If final track geometry does not match, fail BEFORE any offset commit; don't use
an equal width hash as a substitute for independent width admission. Current
prepaint occlusion and next input must see the same eligible named track.

## Bounded storage and proof gate

Private candidate per record: copied48 + staged bar rect16 + staged thumb rect16
+ clamp_index4 + flags4 =88 bytes.64 entries5632; existing7216 + count/padding8
=>12856 candidate resource bytes, <=16KiB. Actual sizeof/offsets remain to measure.
No new context field. Disclose +5640 lazy payload growth for layout users even
without bars; no new resource allocation for zero opt-in legacy frames. Separate
nullable extension allocation remains an alternative if root rejects this shared
payload growth; it needs its own failure contract and cannot be added invisibly.
Two Dialog owners use2 records,4 actual decoration nodes (existing max_elements),
and one semantic per track. They need no extra prepaint payload or paint command
reservation; ordinary background/border/shadow commands consume existing configured
render capacities. Shadow layers still incur their normal bounded costs.

The two-owner wheel/touch/new-route bypass contract is specified below.
Existing Base overlay reducer uses previous completed track bounds, previous
owner viewport/range and stable model, hence the same thumb formula used to paint
that completed frame. Current staging does not replay those events or dispatch
newly appeared controls. The placeholder semantic rule handles previous range0;
Core final visibility/interactivity are explicit, and old helper loads/defaults
remain unchanged.

Required prototype adds actual box pixel/draw/shadow and current hit oracles to
the earlier plan: track/thumb retained bounds, published bar bounds and next-frame
pointer/touch targets agree after first-overflow/resize/shrink; actual viewport
scroll childOffset does not displace its own bar; nested ancestor scrolling does;
track/thumb independently painted/shadowed ownership matches the named node and
clips/culling; zero/hidden leaves neither semantic nor halo; alpha under ancestor
×fade applied once. Verify the moved Core primitive against a frozen legacy
Base oracle over degenerate/nonfinite/normal/both-axis cases and exact operation
order, plus track page/drag and touch interactions on BOTH owners. No narrow
wheel-only or decoration-only result is claimed as full visible control parity.

## Dialog input ownership and Base compatibility

New Dialog composes each owner's cui_base_scrollable(hidden own bars) before its
children, finish afterward, sharing one stable modelY via the local-state copies
specified in a605. Inner changes consume whole scroll events; unchanged inner
may defer to ancestor; outer finish handles only still-unconsumed newly targeted
events once. Core touch pan produces precise scroll events through the same path;
bar-role touch drag keeps its existing priority. Existing previous-frame modal,
clip/parent/directional routing remains authoritative. No residual-delta replay.

Call cui_base_scrollbar_overlay once per owner during ordinary input reduction,
using that owner's last completed positive geometry and copied resolved skin.
Its existing context-owned drag/grab resource handles capture, track click,
keyboard and CANCEL. Its model update is copied back to the local wheel state
before finish. This helper conditionally omits the semantic when the PREVIOUS
range is0; therefore new composition privately ensures exactly one current bar
semantic afterward (submit a disabled SCROLLBAR placeholder with controls/parent
when absent, not a second reducer). Registered Core projection owns final
range-derived omission, including first overflow. Core may clear a placeholder
disabled field ONLY for explicit authored VISIBLE|INTERACTIVE with positive range
and eligible ancestry. VISIBLE alone always publishes disabled=true. It never
replays previous input on newly appeared controls. Components pass disabled=true
to Base overlay reduction for authored noninteractive bars and preserve existing
disabled skin/math/timing; hidden/zero-fade policy clears VISIBLE (and INTERACTIVE)
without disabling the viewport wheel/touch owner. Core final disappearance
withdraws old controls. Existing legacy Base bars do not read these new flags.

New-route private surface reducer disables legacy reserved1/2/3 activation AND
legacy Body6 wheel reduction; those are owned by the new action/scroll helpers.
Old public surface update/begin executes its original statements/timing unchanged.
Pending request_close, Escape/outside and observer/focus statements keep order.
Do not execute old Body wheel then restore model: an observer could see it.
This is a required explicit route selection, not approval to alter old behavior.

## Validation gate and remaining scope

No runtime evidence accompanies this proposal. Prior e094 proves current public
semantic-range timing and viewport-owned bar paint only. This actual fixed-node
representation needs a TEST-ONLY proof before production approval. Frozen legacy
thumb comparator must retain the old pre-move function body in the test oracle,
compare exact float bits (with equivalent NaNs categorized explicitly), cover
both axes, nonfinite/degenerate/default cases and bounded deterministic randomized
inputs. Test ordinary Base wrappers as well as new Core primitive; no default or
ABI drift is accepted merely because new Dialog geometry passes.

Real queued wheel/touch/drag tests must cover BOTH owners: child consumes, edge
defers once, direct Outer routing, hidden bars retain wheel/pan, primary track
page click, thumb grab/move/outside release/CANCEL, right/middle ignore, keyboard
Home/End/Page/arrow, resize/shrink while held, next snapshot has no stale bar,
visible-disabled paint+semantic/no input, invalid INTERACTIVE-only flags,
placeholder override only with explicit interactive policy, focus/capture
retirement on omission/disable, owner removal/reuse, inert/measurement,
record64/65 capacity, byte failure, malformed topology/folds/late duplicates and
all-or-nothing model commit/recovery/two contexts. Layout/application/input runs
once; resource/string/text scratch invariants remain checked through second calc.
No bitmap/text raster or GPU claim follows from draw-command geometry tests.

Full Base/Styled C composition stays gated until this architecture and its proof
are reviewed. Root/native host, global Enter/source placement and all five typed
foreign slot/action plans remain separately open. Foreign synchronous veto has
its own lifetime/exception/reentrancy gate. Coordinate any source base with root
and Stepper V2; do not edit frozen root sources or merge/push from this worker.

### First runtime admission counterexample

The first test-only frame failed LAYOUT with public fixed-zero absolute track/thumb
placeholders: Core maps fixed-zero to Clay PERCENT0, while csl_floating requires
nonregistered floating descendants to be actual FIXED. Its ordinary-flow canonical
zero exception does not extend that floating guard. The proposed representation
therefore uses fixed-positive1 height placeholders, excluded from intrinsic flow;
staging later writes actual current/zero heights as fixed values before second
calculation. No production admission relaxation or stale geometry fallback is
proposed. This correction still requires the revised runtime proof below.

### Publication eligibility review and exact input boundary

INTERACTIVE does not override a submitted owner's or actual layout ancestor's
`disabled` / `details.readonly`, the bar's authored readonly metadata, or inert /
layout-only ancestry. The test adapter traverses bounded actual node parents and
matches their current submitted semantics; eligible flag3 may clear only the
bar's range-placeholder disabled state. Visible disabled owners retain ordinary
track/thumb paint and disabled bar semantics. Fixed sizing admission is actual
Clay FIXED with positive finite height at registration/trial; only the already
staged fixed height may be zero. AUTO/PERCENT/GROW are rejected explicitly.

There is no new generic disabled-scope dispatcher: a component must pass its
current effective disabled/readonly policy to Base input reducers BEFORE they
run. The public Base helpers do not discover arbitrary current semantic ancestors
(the public parent query is completed-frame only). Publication can withdraw a
future hit/capture snapshot; it cannot undo model changes already delivered by a
caller that ran a reducer before learning/changing an ancestor policy. Same-frame
late policy changes have the existing no-rollback boundary. If a product requires
that stronger guarantee for arbitrary external semantic ancestry, this proposal
does not silently provide it: use existing inert/layout-only scope policy before
composition, or bring the stronger ownership rule for a separate review.
The descriptor does not introduce a logical-parent graph traversal or reinterpret
arbitrary unrelated semantic parent links as layout ownership.

Tests isolate readonly owner with disabled=false and disabled actual ancestor
with owner disabled=false, so publication tests cannot pass solely through an
already-disabled owner. Separate queued Base tests supply current owner disabled,
readonly or ancestor-disabled policy before reduction and assert unchanged model,
retired focus/capture, retained disabled paint/semantics. Existing Base disabled
math, normal wheel consumption and legacy observer timing remain unchanged.

## Test-only fixed-node proof on Core V2

Prototype source base: exact preserved Stepper `3e18bbba` (includes numeric Core
`e5b64e4e`); separate branch `test/dialog-track-layout`. Prior signed contracts
and the two-viewport proof stay preserved on `design/dialog-slots-v1`. No public
production header, Core/Base implementation or vendor file is modified here.
`embed_scrollbar_track_probe.py` creates a build-directory Core copy with four
exact substitutions: bounded writable plan extension and stage/validate/publish
seams. Offline tests verify every other source byte unchanged and missing/duplicate
seams preserve prior output. The adapter is `scrollbar_track_probe.c`; executable
fixture is `test_scrollbar_track_geometry.c`.

Measured LP64 resource12856, record48, entry88, matching the budget; existing
production resource remains7216. Actual fixed track/thumb bounds match their
post-end tracked bounds and the bar's final semantic bounds/range/value. Core
prepaint pointer_hit agrees with the next completed input target inside an
ancestor clip and excludes the same bar outside it. Software RGBA tests (no fonts)
produce alpha50 for one original200×ancestor.5×fade.5 layer, alpha90 for overlapping
track/thumb layers, and no pixels/halo beyond ancestor clip, including named
track/thumb shadows. Fully clipped track has no visible pixels. Shadow caster
rectangles match the actual named paint boxes. Hidden/zero nodes may produce
degenerate alpha0 commands; omission means no semantic or visible paint, not a
promise of zero command records.

Queued real input passes on BOTH owners: wheel, touch pan, thumb grab/move/release
and resize while held. Nested child consumes48 without moving parent, then a
current child edge defers once to parent finish (child240, parent48); nested touch
pan changes only child. Visible-disabled paint/semantic remain while pointer/focus/
capture/model changes are refused; hidden bars preserve wheel. Isolated owner
readonly with disabled=false and disabled actual ancestor with ownerdisabled=false
publish a disabled bar and reject next pointer target. Separate Base-policy cases
prove queued input refusal when caller supplies those current disabled policies.
Inert geometry still paints while a model seed10000 remains unchanged. New-route
input helpers are skipped when public frame_sequence is0; no inert setter call.

All64 records commit current clamps; a65th clamp, one-byte-short resource budget,
invalid flags and aliased bar/thumb fail atomically, empty draw and preserve model,
including an attempted error clear within the failed frame; next plain frame
recovers. Fixed-zero/PERCENT0 and GROW placeholders are rejected.40,000 exact
comparisons (both axes, degenerate/nonfinite/default and deterministic random
cases) match the prototype moved formula against the unchanged real Base helper.
This is not a claim that production Core already exports the new pure helper or
that an actual Base forwarding change has been validated.

Focused warnings-as-errors Release build plus2/2 private-Xvfb CTests passed:

```sh
nice -n 19 taskset -c 0 cmake --build BUILD --target test_scrollbar_track_geometry --parallel 1
nice -n 19 taskset -c 0 python scripts/test_local.py --build-dir BUILD \
  -R '^(scrollbar_track_geometry_prototype|scrollbar_track_probe_generator)$' -V
```

Runtime evidence supports further review of this narrow representation; it is
not production approval or full Dialog parity. Still unproved in this fixture:
all late/folded alias variants, layout-only model/lifecycle cases, interactive
track page-click/keyboard/right-button/CANCEL permutations and arbitrary portal/
modal/layer interactions. Those remain production test requirements above.
Existing upstream Core proof covers related cases but is not substituted as
new-record proof. Windows/WASM/GPU and text raster are not run or claimed.

Disabled/interactivity policy blocks bar input; it does not mark an otherwise
active viewport's Core clamp inert. Ordinary current-range layout clamping on
content shrink can still change that viewport's offset. The disabled-input tests
use a model already valid in the established completed geometry; they do not
assert that a disabled bar preserves an out-of-range offset across resize.

Final checkpoint checks: both focused CTests2/2 passed after final configure;
Ninja dry-run reported no work for the exact target. Offline installed-skill
suite18/18, canonical skill freshness, changed C/header clang-format23.1.2 and
staged diff checks passed. The skill suite shared CPU0 with root's suite and
finished in128.258s; future heavy skill suites must coordinate like compile slots.
No full library rebuild or second build directory was used.
