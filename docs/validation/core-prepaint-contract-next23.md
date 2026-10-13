# Current-frame prepaint contract audit

Audit source: CUI `3f4e108`; reference GPUI component v0.7.1
`87d10ae5e1299d1be18670a37570c5402f952e34` in
`/tmp/cui-gpui-reference`. This is a design proposal, not an implemented API or
Chart/Plot feature promotion. No production files were changed.

## Observed phases and the actual gap

1. `cui_begin_frame` processes queued input against the previous completed
   semantic hit graph. The live global pointer is updated here and retained
   on idle frames. This capture/activation behavior must remain unchanged.
2. Components submit Clay layout and CUI semantics/paint payloads.
   `cui_bounds` first returns previous semantic bounds; it is not a current
   layout query. `cui_interact` also uses those bounds for its returned hover.
3. `cui_end_frame` calls `Clay_EndLayout` once. Clay now has current bounds and
   ordered render commands, including custom commands carrying CUI node data.
4. Core immediately converts that array to the portable draw list.
5. Only after conversion does Core update tracked bounds, bind semantics to
   actual layout nodes, calculate ancestor clips/layer/order, retire obsolete
   focus/capture, and publish the next input graph.

Evidence: `src/core/cui.c` functions `cui_bounds`, `cui_track_bounds`,
`cui_interact`, `cui_end_frame`, `cui_core_ancestral_clip`, `cui_core_above`;
`third_party/clay/clay.h` custom-command emission before child traversal.
Existing solid/bitmap underlays already resolve grow/alignment against current
bounds during conversion. They are a useful insertion seam, but have no public
callback that can update their content after layout.

Chart registers `cui_track_bounds(ui, id, &state->bounds)` after rendering its
bitmap. Tooltip origin still uses that retained state. Plot also calls
`cui_bounds` before its bitmap/tooltip paint. Neither a live-pointer getter nor
an extra invalidated frame makes the current submission's origin correct while
scrolling, resizing or opening an overlay. Calling `cui_bounds` from a late
callback without a separate snapshot would still prefer previous semantics.

The pinned reference differs deliberately: `crates/base/src/plot/element.rs`
prepaint receives the actual bounds, inserts a Normal hitbox, uses the live
window pointer minus current origin, and prepaints its tooltip as a separate
root. Its paint phase rechecks hitbox visibility after all hitboxes exist,
including idle relayout under a stationary pointer. Styled tooltip crosshair
and dots stay in the plot layer; only the box is deferred
(`crates/component/src/plot/tooltip.rs`). Reference Plot implementations may
also lay out arbitrary children in prepaint. The minimal proposal below does
not provide that latter capability.

## Proposed portable surface

Names are illustrative; approval is required before adding declarations.

```c
typedef struct cui_prepaint_writer cui_prepaint_writer;
typedef struct {
    cui_id owner;
    cui_rect bounds;             /* Actual current global layout rectangle. */
    cui_rect clip;               /* Viewport intersected with ancestor clips. */
    cui_rect visible_bounds;     /* bounds intersected with clip. */
    float pointer_x, pointer_y;  /* Global, last processed host position. */
    bool pointer_valid;
    bool pointer_inside;         /* Inside visible_bounds. */
    bool pointer_visible;        /* Inside, modal allowed, unoccluded hitbox. */
    bool modal_allowed, inert;
    cui_id pointer_hit, capture; /* Capture is informational, never rerouted. */
    float opacity;              /* Cumulative owner opacity. */
    double frame_time;
    float frame_delta;
} cui_prepaint_info;
typedef bool (*cui_prepaint_fn)(const cui_prepaint_info *,
                              const void *copied_payload,
                              cui_prepaint_writer *);
typedef struct {
    cui_prepaint_fn callback;
    const void *payload;
    size_t payload_bytes, command_budget;
    bool hover_hitbox;
    int16_t deferred_layer;
} cui_prepaint_options;
bool cui_set_prepaint(cui_context *, cui_prepaint_options);
bool cui_prepaint_emit(cui_prepaint_writer *, const cui_draw_command *, bool deferred);
void cui_prepaint_request_frame(cui_prepaint_writer *);
```

Registration attaches to the currently open, successful, explicitly identified
box, with its node, parent, layer, paint root and focus scope captured there.
It does not create another layout box, semantic or Tab stop. At most one
callback may attach to a node per frame; no persistent registration survives
`end_frame`. A descriptor supplies callback, payload bytes, output-command
budget, optional deferred layer, and explicit hover-hitbox participation.
Registration copies the payload into `max_align_t`-aligned frame storage;
pointers inside the payload remain borrowed through renderer completion.
Chart may copy its options and a pointer to caller-owned state, rather than
retain the address of its temporary local options. Payload copies, padding,
and retained text count against `frame_bytes`; no per-frame heap allocation.

Null callback, malformed payload, zero owner, unsupported phase or unsuccessful
box rejects registration without partially installing a callback. Duplicate
registration reports `CUI_ERROR_DUPLICATE_ID`; exceeding bounded registration,
frame storage or command budget reports `CUI_ERROR_CAPACITY`. Lifecycle misuse
reports `CUI_ERROR_LIFECYCLE`. Invalid paint data returns false, matching
existing bitmap/mesh validation. A callback returning false discards its entire
recorded output and reports a documented `CUI_ERROR_LAYOUT` failure; it must
not silently leave a half-tooltip in the list.

Attach an internal callback index to the existing CUI node and retain its
customData type; replacing Clay customData with a function pointer would break
existing underlay/outline/mesh conversion. Preserve those existing painters
and skip-background logic when adding the ordered callback output.

Registration count is bounded by `max_elements`; no new config field is needed
for the first implementation. Each registration reserves an explicit positive
command budget. Sum with overflow checks is bounded by `max_commands`, including
Core-owned clip commands. Existing ordinary layout conversion also uses that
same final budget. If final combined output exceeds capacity, return an empty
draw list with `CUI_ERROR_CAPACITY`, as existing conversion does. Do not run
callbacks again or silently truncate a clip pair.

## Execution and ordering

After the single `Clay_EndLayout`, build/read current-node geometry and bind
current semantics before running any callback. This can reuse/refactor the
existing post-conversion node binding and clip calculation, but must keep
previous dispatch arrays intact until the existing publication point.
A separate current snapshot, or bounded on-demand traversal, is required.
Do not simply move writes to `previous_hits` earlier: public event queries
would then see a different hit graph halfway through the same frame.

Freeze all current hitboxes before the first callback. Execute callbacks once
in ordered custom-command/node paint order, including owners with an empty
visible region (so retained hover/fade state can clear). Do not execute
layout-only owners; inert owners receive `inert=true` and cannot participate in
hover. Current viewport, nodes and opacity are available even on the first
frame. Callback order cannot alter visibility seen by a later callback.

Callbacks receive no mutable `cui_context`; they may measure with an explicitly
read-only measurement helper and update caller-owned presentation state, but
must not submit layout, semantics, events, focus, capture, portals, or another
callback. Every existing mutation entry point needs a phase guard if a context
pointer is nevertheless retained by caller code. Calling `begin_frame`,
`end_frame` or layout APIs recursively is lifecycle failure, not another
layout pass. Requesting the next frame is permitted through a writer helper;
it has no effect on this frame's layout or hit graph.

Writer output is an ordered list of validated RECT/BORDER/TEXT/IMAGE/MESH
commands with global coordinates. No raw CLIP_PUSH/POP is exposed: Core owns
balanced clip envelopes. Text is copied into frame storage, including contextual
paragraph/color-span data where supplied. Bitmaps and vertices remain borrowed
through the existing renderer-completion/next-begin lifetime. Existing renderer
interfaces and draw-command types remain unchanged.

For local paint, insert output above the owner's background and below its
already submitted children at that owner's custom-command position. Core wraps
it with `visible_bounds` and cumulative opacity (local paint cannot escape the
owner rectangle). Ancestor floating-root
clips must use the complete ancestry, not Clay's nearest-scissor approximation.
Do not append all callback output to the end of the frame: that would put plot
crosshairs above sibling controls and change ordered paint semantics.

A second, explicitly requested writer stream can defer the tooltip box only.
Merge that stream at a bounded explicit layer using the same layer/paint-root
ordering as ordinary portals; it must not be unconditionally appended above
modals. It clips to viewport instead of the plot rectangle, carries the owner
fade/opacity once, and introduces no input hitbox. No tooltip output is allowed
for a modal-blocked owner. The local stream still holds crosshair/dots. Clip
state must be balanced before and after every inserted/deferred stream.

A lean first implementation may support local paint only, but that does not
complete the source tooltip overlay behavior; deferred fixed-paint output is a
separate acceptance requirement for Chart/Plot migration.

## Occlusion is a policy, not a rectangle subtraction

`clip` is the geometric ancestor clip, independent of sibling occlusion.
`visible_bounds` is its intersection with the owner. Neither represents the
possibly disconnected region left by overlapping siblings. `pointer_visible`
is a point test against the frozen current hover hit graph.

Core's existing input graph contains eligible semantic controls/scroll owners.
Chart/Plot have passive IMAGE semantics; GPUI gives plots Normal hitboxes
without making them keyboard buttons. Therefore callback owners need explicit
hover-hitbox participation independent of Tab, pointer activation and capture.
That participation must not opt a plot into click activation or focus.
Compare current layer/paint-root/node order, ancestor clips, active modal scopes,
disabled/inert state and eligible control hitboxes. A covering control or a
newly opened modal must suppress the plot tooltip on that same frame.

Arbitrary passive paint currently has no explicit pointer-occlusion registration.
For GPUI-equivalent passive popup/overlay occlusion, add a small bounded
`hover hitbox/barrier` registration on an existing node, processed before all
callbacks. Its purpose is only to block lower hover hitboxes; it introduces no
semantic action or capture. Reuse existing clipping/order and offer an explicit
pass-through policy. Do not infer occlusion from background alpha, text bounds,
bitmap alpha or all layout containers: transparent ancestors and tooltip paint
would otherwise incorrectly block plots. Semantic-only occlusion is a valid
narrow first milestone but must be named as incomplete, not full source parity.

The global pointer is the last processed Move/Down/Up position, initially0,0;
Core does not poll an OS cursor at `end_frame`. Preserve that public contract,
with `pointer_valid` following the existing finite-position getter, including
its initial0,0; it does not invent an OS pointer-presence signal. Idle layout changes
must recompute hover using those retained coordinates. Existing event targets,
consumed bits, press/Space capture, touch contacts/gesture ownership, activations
and keyboard focus remain exactly the begin-frame dispatch results.

## Migration and limits

Chart can automatically render/rasterize/update its tooltip model inside the
callback using current bounds and global pointer, then emit the bitmap and
fixed tooltip paint. Plot can do the same for its current-data selection and
crosshair. The host must not manually provide prior bounds as an interim fix.
UI resources/caller-owned state continue owning bitmap allocation and cleanup;
Core owns only registration/output metadata.

Existing `tooltip_content` callbacks submit layout. They cannot be invoked from
this restricted paint callback. Preserving those callbacks while claiming
current-frame arbitrary tooltip layout would require a separate bounded
independent overlay-layout context or a new measured paint-content API.
Calling Clay_EndLayout again, submitting another root into the ended main tree,
or permitting callbacks to resubmit the whole scene is not the minimal seam.
Keep this custom-content limitation explicit until that capability is designed.
Likewise this proposal does not alter previous-frame event routing or guarantee
same-frame activation for a newly appeared/moved control.

## Regression evidence and required implementation gates

New `tests/unit/test_core_prepaint_contract_fixture.c` links Core alone. It
proves the current gap rather than pretending the proposed callback exists:
first submission has zero bounds; on the next submission live pointer50,35 and
hover still match old y20/width200 while the actual underlay and tracked bounds
become y60/width100. The underlay uses the current origin/size without another
layout pass. The fixture passes with strict C11 warnings. This is timing/seam
evidence only, not a callback acceptance test.

Implementation tests must additionally cover:

- First frame, responsive width/height, sibling/wrapped-text shifts, scrollbar
  offset and stationary-pointer relayout: callback and actual emitted paint
  match the same current node and choose the current datum/anchor.
- Nested clipped owners, absolute children, detached portals and partial clips:
  correct snapshot clip and balanced renderer clips, including floating roots.
- Overlapping control/nonmodal popup/passive barrier/new modal: same-frame hover
  denial with current paint ordering; pass-through overlay allows lower hover.
- Two callback owners see one frozen hit graph; local crosshair stays below a
  later sibling, deferred tooltip escapes plot clip but remains below a modal.
- Capture/keyboard/touch cancellation and consumed/activation queries are
  unchanged; paint visibility never retargets already dispatched input.
- Missing/layout-only/inert owner, duplicate registration, callback failure,
  invalid finite/bitmap/mesh/text data, exact capacity and overflow: deterministic
  errors, zero partial output and no clip corruption, no repeated callbacks.
- Payload stack lifetime/copy/alignment, caller-state mutation, owned-resource
  retirement, text-span lifetime, renderer completion and next begin-frame.
- Callback attempts nested layout/frame/semantic/event registration fail with
  lifecycle error; headless, ASAN/UBSAN and WASI validate the same contract.

No production callback, tooltip migration, browser or physical-device behavior
was implemented or certified by this audit. Root should register the diagnostic
fixture only if it is useful as permanent evidence; it is not a feature test.

Validation commands/results for this audit:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build build -j6
ctest --test-dir build --output-on-failure
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude -Ithird_party/clay \
  tests/unit/test_core_prepaint_contract_fixture.c src/core/cui.c -lm -o fixture
```

The warning-enabled portable build and CTest passed100/100. The standalone
fixture passed both the command above and ASAN/UBSAN with leak detection.
Optional native adapters were excluded from this portable audit build; no
native GPU or SDL coverage is inferred. Logs are under
`/home/matteo/.cache/cui-validation/tmp-storage/core-prepaint`.
