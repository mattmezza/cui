# Typed Slider Base interaction/presentation checkpoint

Design-only addendum to [the reviewed model ABI](SLIDER_MODEL_ABI_V1.md), following
its frozen-current/evolving-intention resolution. Pure math/init is implemented
in `d4506197`; the reducer/presenter below is **not implemented or approved by this
document**. The earlier contract omitted layout and an in-frame authority query.
These decisions require root review before production. No Styled, host, bridge or
language API is introduced here; whole-family completion remains open.

## Concrete bounded Base composition

Keep `cui_slider_model.h` pure. Put new UI records/functions in
`include/cui/cui_base_slider_model.h`, including exact Core/model/component-event
headers. Retain f15/b24 option/request/result records, phase tags and ownership.
Add one separate exact-version/exact-size layout record to the new unshipped Base
entry point; no existing record grows:

```c
typedef struct {
    uint32_t version, struct_size; /* exactly1,sizeof(v1) */
    float length, cross_extent, track_extent, thumb_extent, axis_inset;
    uint32_t reserved; /*0*/
    cui_box root, track, fill, start_thumb, end_thumb; /* appearance only */
} cui_base_slider_model_layout_v1;
cui_slider_model_result_v1 cui_base_slider_model_events_v1(
    cui_context *, cui_id owner, cui_slider_model_value_v1 confirmed,
    cui_slider_model_state_v1 *, const cui_slider_model_options_v1 *,
    const cui_base_slider_model_layout_v1 *, cui_slider_model_handler_v1);
```

The function owns opening/closing its root; callers do not open a same-ID box.
All dimensions are logical pixels, finite positive; `axis_inset>=0`,
`axis_inset<length/2`, `cross_extent>=max(track_extent,thumb_extent)`.
Root main/cross dimensions are fixed `length`/`cross_extent`, transposed vertically.
Track centers run from `axis_inset` to `length-axis_inset`; cross center is
`cross_extent/2`. Thus center travel is `length-2*axis_inset`.
Caller may select inset0 for source-style edge-centered thumbs extending outside
the root, or thumb_extent/2 to keep them inside. This choice is explicit, never
inferred from theme. Defaults are not silently installed: callers author the
record. Source-size examples are horizontal length240 or vertical length120,
cross24, track6, thumb16; theme/token-derived presentation is a later Styled task.

Only appearance fields of the five boxes participate: background, border,
border_width, radius/custom_corners/corner_radii,
custom_border_edges/border_edges and custom_opacity/opacity. Validate active
finite nonnegative widths/radii/edges and active opacity[0,1]. Ignore all other
box fields (including their inactive NaNs); geometry, padding, clipping, layout,
inertness and child translation come from this composition/ancestry, never from
appearance records. No theme or glyph is invented. Zero appearance draws nothing. Named parts can use the existing
`cui_element_set_box_shadows` after the facade returns and before end_frame;
no new shadow storage/hook is needed. Getter identity is never a presence check.

Use fixed parent-attached absolute children for track/fill/thumbs, inheriting
ancestor clip/layer/opacity. Horizontal value fraction u places center
`(axis_inset+u*travel,cross/2)`; vertical center is
`(cross/2,length-axis_inset-u*travel)`. A thumb is a square of thumb_extent centered
there; track thickness is track_extent. Single fill is min-center→end-center,
or end-center→max-center when REVERSE_FILL; Range fill is start-center→end-center,
ignoring reverse. Reverse changes no target, axis, value or keyboard rule.
Single never emits START thumb. Range thumb draw/hit order is START then END,
so coincident thumbs select END. Root/track/fill/START_THUMB/END_THUMB getters are
pure typed IDs, not presence tests. Proposed helper:
`cui_id cui_base_slider_model_part_id_v1(cui_id owner, uint32_t part)` with fixed
ROOT/TRACK/FILL/START_THUMB/END_THUMB tags, ROOT=owner. Internal salts are shared with composition;
zero owner/unknown part reject and no zero ID is silently substituted. Validate
logical generated IDs are nonzero/distinct before admission; Core remains the
sole authority for collisions with other boxes and its private folded IDs.
Do not copy its hash implementation or claim a full Core-capacity preflight.

This is an explicit-size Base slice, not full source adaptive sizing. Grow/percent
length, arbitrary child content, animation/rings, palette/gradient styling and
state-style facade hooks remain separate presentation work. Do not call this
whole Slider family completion. A future grow facade must use current-layout
geometry for both paint and semantics; cached bounds or a paint-only prepaint
thumb cannot substitute for current hit/semantic layout.

### Current paint versus prior input geometry

Composition order is concrete: open root; register root and thumb semantics,
details, Tab stops and current numeric capabilities; reduce original-order input;
then emit track/fill/thumb boxes from the resulting paint pair and close root.
Core explicitly supports behavior before its own layout box: end_frame rebinds
semantic ID to its actual painted node (`cui_core_commands_done`), so preregistered
thumbs acquire final thumb bounds/clip/parent/order, not root-sized hit regions.
The same binding exists in final CoreV2 `3e18bbba` (statically inspected). This
resolves numeric claim's need for current semantic registration without freezing
thumb paint before input. No provisional or duplicate semantics are emitted.

`src/core/cui.c:cui_bounds` first returns previous semantic bounds. Reducers must
not mistake that result for the just-composed rectangle. Before writing state,
snapshot prior root/track/thumb geometry and all current entry options/values.
Read the named nonsemantic track through public cui_bounds before opening current
parts; its fallback reads the retained completed layout. Never retain a pointer
to that backend data, and require prior owner submission/readiness/continuity.
Tests must prove changed inset/length uses actual prior track bounds, not current
option values reconstructed over the prior root.
A valid first frame submits layout/semantics and establishes ownership, but starts
no pointer gesture without positive prior axis geometry. Input mapping uses the
prior completed track axis associated with the actual routed target, then pure
mapping/constrain helpers; it never reads newly guessed screen coordinates.
Layout emits current parts from the chosen paint value: ongoing preview only
while dragging, otherwise confirmed entry, including immediately after terminal
input. Current absolute child rectangles use the explicit authored extents,
relative to the current root; moving/resizing the ancestor therefore updates
paint/hits/semantics together at end_frame, without a one-frame paint workaround.
Changing physical length/inset does not change the value domain or orientation;
subsequent pointer samples use that frame's prior resolved track. This follows
normal previous-layout routing and must be tested with parent motion/resizing.

## Existing dispatch: sufficient local ownership, explicit focus policy

Use one root semantic GROUP with `focusable=true`, `tab_stop=false` and
`touch_drag_priority=true`. It has no pointer_activatable flag and no numeric
capability: Core can capture/focus its pointer down because it is focusable,
without advertising an ignored accessibility ACTIVATE action. Each real thumb
is a SLIDER semantic, its own enabled Tab stop, precise description and normalized
value-domain semantic value. Give each thumb touch_drag_priority too. Root label
is the group name; thumb names/descriptions follow f15 fallback rules.

Actual mechanics are already in Core:
`cui_core_hit` / `cui_core_pointer_focus_target`, `cui_begin_frame` pointer-down
branch (capture=hit when focus target==hit), pointer move/up event targets,
`cui_event_target`, `cui_consume_event`, `cui_pressed`, `cui_focus` and
`cui_set_tab_stop`. No public capture setter, retarget/requeue, fabricated key,
new global dispatcher or host change is needed.

For a direct-thumb down, select that thumb. For a root-track down, select nearest
**prior visual** thumb, strict distance comparison with exact midpoint END.
Capture identity remains the original Core target (root or thumb), copied into
state; never pretend it captured the chosen thumb. After all current thumb
semantics are registered, call existing `cui_focus(chosen_thumb)` for track down.
**Reviewable policy:** this public focus call makes focus_visible=true, unlike
Core's initial pointer focus which sets it false. This slice must document/test
that result, not privately change Core focus flags or claim identical source
focus-ring behavior. Future Styled ring behavior needs an explicit decision.

Core resolves the whole input batch before reducers run. A following key may
therefore still target ROOT even after local focus was changed to a thumb.
Handle ROOT-targeted keys as local delegation to the active chosen thumb (END
before any choice); preserve actual ROOT event.target and put the chosen endpoint
in request.thumb. A thumb-targeted key always names that actual thumb. Do not
rewrite target/index/evidence to imply Core already focused another target.
While dragging, ignore non-Escape keys and revoke both thumbs' current numeric
capabilities, as b24 specifies; refresh before any numeric claim. After terminal
input restore future eligibility. Local delegation is limited to these own IDs;
never handle foreign targets or already consumed input.

Lock actual pointer type/device/contact, primary button and captured target.
Second contacts never update another contact's preview; if Core replaces capture,
finish scanning valid original-order terminal/cancel inputs before diagnosing
capture loss. Otherwise a normal same-batch up (Core has already cleared final
capture) would be misreported as an early synthetic cancel. Escape can be routed
to an enclosing focus scope by Core: do not steal that scope's event. If it causes
capture loss without a widget-targeted cancel, emit synthetic CAPTURE_LOST with
no invented Escape input/index. Explicit cancellation targeted to this root or
active thumb can retain its actual copied input. Core touch handoff cancel/scroll
insertion stays authoritative; no custom touch dispatcher is added.

## Narrow missing readiness query: production gate

`cui_accessibility_numeric_snapshot_ready` deliberately returns false in-frame
(`src/core/cui.c:cui_accessibility_numeric_snapshot_ready`). Base reduction runs
in-frame, so it cannot gate pointer/keyboard requests on this outside-frame
predicate. `cui_bounds` and `cui_event_target` do not certify prior validity after
an error was cleared. Numeric claims are already safe, but capability queries
cannot be used as a general proxy: a dragging widget revokes capabilities while
its valid captured pointer samples still need reduction.

Propose this **new narrow observer**, pending root approval, not a dispatcher:

```c
bool cui_frame_input_snapshot_ready(const cui_context *);
```

True only during ordinary admitted composition when the most recent completed
Core semantic/geometry snapshot was valid and no known invalidating current-frame
error has occurred. Reuse e5's authoritative private validity/invalid-frame flags
and error gate; no new allocation, context flag, old public struct growth or
legacy routing change. It certifies neither current final geometry nor renderer,
native host or physical-AT success. Outside-frame callers keep using the existing
snapshot_ready API. In-frame observer is false initially, after failed begin/end,
after diagnostic→clear and throughout a failed frame; only a later successful
ordinary end establishes authority for the next frame. Clear alone cannot restore
it. Layout-only/inert calls separately have frame_sequence0 and never reduce.

Initial healthy frame may bind/init owned model state and submit confirmed
geometry without input. After an invalid completed snapshot, do not consume input,
advance sequence or emit new callbacks while this query is false. Submit safe
confirmed geometry for recovery; preserve an existing drag and its last-admitted
frame marker. On the next ready submission, the admission gap produces one
OMITTED cancel under the existing priority order; it means no admitted painted
model submission, not a fabricated physical omission. That cancel offers the
captured start and must not replay invalid-frame input. If the **current** frame
fails after callbacks already happened, stop further requests; no rollback.
Return CORE_FAILURE when current Core error is known; no new callbacks occur
after that diagnosis and every opened box is balanced. Otherwise a recovery-only
geometry call returns VALID|SUBMITTED, counts0, leaving existing state unchanged.
The healthy first-frame ownership initialization is the sole non-input admission
exception; it must not silently revive a previously active gesture.

Modal eligibility additionally gates reduction with the existing
`cui_pointer_scope_allowed(ui,0)` **scope-policy implementation**, which checks
current and previous modal ancestry independently of a coordinate. Reusing it
for this component's pointer/key admission is an explicit review point because
its public documentation calls it a coordinate-opener helper. Numeric claim also
rechecks previous/current modal scopes. A later-composed modal cannot retroactively
undo already delivered callbacks; tests must distinguish known modal blocking
from later-frame effects. No private modal state is read in Base.

## Ordering, inactive modes and acceptance

Retain f15/b24 exactly: immutable current/domain/options snapshot; local requested
intention evolves in original order; each key/a11y command has BEGIN(before),
UPDATE(after), COMMIT(after), and two keys produce +1 then +2. Pointer down emits
BEGIN+UPDATE; move UPDATE; up UPDATE+COMMIT. Preflight sequence budget and every
public version/size/tag/string/value/state/owner/layout argument before UI/state
mutation; preserve atomic admission failure. Known later Core failure is not a
whole-frame transaction. Copy metadata before any synchronous callback. Callback
may assign external owner/userdata and enqueue work, but not mutate/destroy active
UI/state/options/layout/borrowed strings. No auto-accept, CHANGE or owner reread.

Disabled/read-only/domain-zero/source/mode/domain/orientation/omission/capture-loss
cancels follow the reviewed priority. Missing handler is read-only cleanup, no
callback/count. Inert/layout-only ancestry is recognized without a new observer:
`cui_frame_sequence` returns0 there; ordinary composition validation and
`cui_frame_paint_enabled` distinguish invalid phase/layout-only/inert paint. Submit
layout where allowed, paint inert content, but leave state/sequence unchanged and
emit no callbacks/input claims. Omission has no immediate callback; next admitted
painted frame cancels once. Disabled thumb boxes keep disabled semantics but their
visible appearance is suppressed; the source's disabled thumb omission remains
explicit rather than painting an invented thumb. Precise descriptions survive.

## CoreV2 compatibility and required independent proof

Root's target composition base must include e5+d450 plus CoreV2 final `3e18bbba`;
this worktree has not silently substituted an older Core. Coordinate before any
production Core observer edit. Hover's track/scrollbar prototype is test-only,
not an available Slider API. Fixed dimensions/absolute decorations fit the current
scroll-layout contract: no percentage-height, fill-GROW floating child or foreign
portal is introduced. Grow/percent alternatives require a separate admission proof,
not an assertion that CoreV2 background paint solved layout.

Proposed files: new Base UI header/source and shared private part/geometry helpers;
extend pure model header only with independently approved phase/options/record
vocabulary if needed (no math changes); new focused interaction/geometry tests;
usage guide/metadata and generated SDK/dist/skill updates. No legacy reducer,
Core dispatcher, Styled facade, host or bridge implementation edits. The observer
would be a separately reviewable Core header/source/test change after approval.

Required headless evidence before a Base implementation handoff:

- Strict selective/umbrella/source C11, record sizes/versions/inactive fields,
  unchanged-output/state/callback admission, owner/context/duplicate/frame/sequence
  failure and capacity errors. Keep old .375/.375 two-key fixture beside new
  .375/.5, and compare legacy ID/commands/callbacks against its existing baseline.
- Horizontal/vertical, Single/Range, linear/log, reverse Single and ignored Range
  reverse; actual final paint/semantic bounds and next-frame hit routing under
  translated/resized/clipped parents. Root/thumb/direct-hit/overlap/midpoint,
  first-frame no hit, source-size and custom extents, exact endpoints/neighbors.
- Actual mouse/touch down/move/up, same-batch click, repeats/Home/End/arrows,
  two keys and mixed absolute/relative actions; frozen current/evolving intention;
  accept all/none/second and external userdata mutation. Original target/index,
  pointer identity, no activation evidence invented; copied requests outlive UI.
- Root capture plus child focus/Tab and explicit focus-visible policy; root-key
  local delegation; second contact/capture replacement, touch pan handoff,
  ancestor Escape ownership, current/previous modal gates and no foreign consume.
- Disabled/read-only/no handler, inert/layout-only, omitted/resumed, source/domain/
  mode/axis replacement, busy numeric revocation and cancellation order. Current
  typed SET raw double stays intact and never directly writes owner.
- Actual CoreV2 registered-scroll composition and real early/late layout failures:
  initial/failed begin/current diagnostic→clear/end failure/clear-only/fresh-good
  recovery; readiness does not depend on numeric allocation/capability and no
  invalid-frame input replays. Already copied requests are not rolled back.

Signed: `/root/hidden_windows`, 2026-10-31. Design only; no production, generator,
compiler, dependency, merge or push in this checkpoint. Await root review of
layout signature, focus/delegation policy, modal helper reuse and narrow observer.
