# Scroll observer checkpoint

Original design checkpoint `6251b9d6`; the Base and Styled pairs are now implemented,
as recorded at the end. Virtual/message event proposals below remain unimplemented. This is a
bounded follow-up to the [remaining audit](COMPONENT_EVENTS_REMAINING.md) and
[shared contract](COMPONENT_EVENTS_CONTRACT.md), not a new manifest family or a
claim of universal scroll events. Command Styled and TitleBar are separate work.

## Existing seams and meanings

| API/source | Actual behavior and boundary |
| --- | --- |
| [`cui_base_scrollable`](../../src/base/cui_scroll.c) | Captures entry offsets, clamps nonfinite/out-of-range/disabled-axis values, reduces ordered targeted wheel events, registers owner, reduces horizontal then vertical scrollbar, updates semantic details. Bool is entry-versus-final offset inequality, including normalization; it is not submission success or input count. Hidden bars clear drag state but leave wheel support. Disabled suppresses wheel/scrollbar interaction but does not bypass normalization. |
| `cui_base_scrollable_finish` in the same source | Resolves geometry, reduces still-unconsumed targeted wheel events now deferred by children and updates owner semantic details. No repeated owner registration or scrollbar pointer/key reduction. Bool compares offsets immediately before/after this finish only; it omits earlier changes. A consumed event does not replay on another finish. |
| [`cui_scrollable_begin`](../../src/components/cui_scroll.c) | Returns false on null context/theme/state or zero ID, otherwise calls Base once and opens root/body/viewport/content boxes. True means caller must pair end, not that offset changed or geometry succeeded. Theme resolution and optional previous-frame content measurement precede Base. First auto-measured range may be zero. |
| `cui_scrollable_end` | Const-state paint/box closure; no deferred wheel reduction. Cannot truthfully promise a final offset that includes children. |
| `cui_scrollable_end_ex` | Mutable preferred end: re-resolves measured geometry, calls finish, updates current clipped viewport translation, then legacy end paints bars/closes boxes. Returns void. It must pair a successful begin with matching owner/state/options. |
| Scrollbar overlay split/combined APIs | Context-owned bounded drag state, caller offsets, Base reduction followed by paint; distinct owner geometry and validation. These are not automatically covered by a Scrollable owner facade. |

The [public scroll API](../../include/cui/cui_scroll.h) retains style pointers only
for the documented call/frame, not as listener registrations. There are no legacy
Scrollable change callbacks to chain. Rendering callbacks belong to composed
children. Do not retrofit handlers into `cui_scroll_state` or options layouts.

Wheel reduction consumes a changed offset, or an explicit horizontal trap; at an
edge it can defer routing to an ancestor. Precise deltas use logical units;
nonprecise deltas use wheel step. Axis/gesture locking are existing routing policy
(gesture lock is disabled on WASM), not new event phases. Scrollbar Home/End/page/
arrow keys act on the scrollbar; basic Scrollable is not the virtual owner's
focusable-keyboard API. Do not add that keyboard behavior through observation.

[`test_scroll.c`](../../tests/unit/test_scroll.c) already contains nested deferred
routing, no replay, consumed-wheel and current-frame translation assertions.
These are source/test evidence, not tests rerun for this documentation checkpoint.

## Recommended first slice: one paired final-offset notification

Add a new `cui_scroll_events.h`, narrow Base/Styled common seams and a dedicated
unit test only after review. Keep every legacy entrypoint and layout unchanged.
Proposed declarations (names/layout subject to review):

```c
typedef struct { float x, y; } cui_scroll_offset;
typedef struct {
    cui_component_event_metadata event; /* CHANGE, UNKNOWN source */
    cui_scroll_offset previous, value;
    bool has_previous; /* false when entry contained a nonfinite coordinate */
} cui_scroll_change;
typedef void (*cui_scroll_change_fn)(void *, const cui_scroll_change *);
typedef struct { cui_scroll_change_fn on_change; void *user; } cui_scroll_handler;
typedef struct {
    bool valid, changed, count_overflow;
    size_t change_count; /* actual observer calls: zero or one */
} cui_scroll_event_result;

/* Caller-owned single-use pairing token; private implementation fields.
 * No handler retained in token/context. Identity, frame and entry offsets only. */
typedef struct {
    cui_context *private_context;
    cui_scroll_state *private_state;
    cui_id private_owner;
    uint64_t private_frame;
    cui_scroll_offset private_entry;
    bool private_active, private_styled;
} cui_scroll_event_scope;

/* Same bool meaning as cui_base_scrollable; scope records admission separately. */
bool cui_base_scrollable_events(cui_context *, cui_id, cui_scroll_state *,
                               cui_scroll_options, cui_scroll_event_scope *);
cui_scroll_event_result cui_base_scrollable_finish_events(
    cui_context *, cui_id, cui_scroll_state *, cui_scroll_options,
    cui_scroll_event_scope *, cui_scroll_handler);
/* Same begin/pair contract as legacy Styled begin. */
bool cui_scrollable_begin_events(cui_context *, cui_id, const cui_theme *,
    cui_scroll_state *, cui_scroll_options, cui_scroll_event_scope *);
cui_scroll_event_result cui_scrollable_end_events(cui_context *, cui_id,
    const cui_theme *, cui_scroll_state *, cui_scroll_options,
    cui_scroll_event_scope *, cui_scroll_handler);
```

The complete token is caller-stack allocatable and initialized to zero before
first use. Its private-prefixed fields must not be read or modified by callers.
The proposed frame field uses the actual public frame-sequence width (uint64_t).
It contains borrowed context/state identities, no retained handler or callback
user pointer, and must not be copied while active. Independent nested owners use
distinct tokens. No global listener registry, heap allocation or hidden event log.
Begin must reject an already-active token rather than overwrite its live pair.

Capture entry offsets before the one existing reducer. Base finish invokes the
legacy finish once, freezes final offsets/metadata, consumes the token, then
invokes one optional observer. Styled end does the existing finish/translation/
paint/closure once, consumes token, then invokes observer after owner closure.
Do not call the public Base observer from Styled end and then notify again.
Handler/user borrowing lasts through end only. This explicit final-after-paint
seam keeps child/paint timing intact; it is not a generic frame-end dispatch.

Token checks require zero initialization before first use, correct context,
owner, state address and Base/Styled layer, and single-use pairing. These identity
mismatches and an already-consumed token reject before reducer/box/observer effects.
An admitted token records `cui_frame_sequence(ui)`, which is zero in inert or
layout-only scopes. **The current public API cannot prove same-frame identity for
frame-zero work.** Do not add a Core counter or claim a runtime guarantee there.
When both recorded and current frame sequences are nonzero, require equality;
a zero/nonzero transition is also rejected as a changed submission scope. If both
are zero, same-frame/same-scope pairing remains a caller obligation: carrying such
a token across frames is unsupported misuse that cannot be detected reliably.
No diagnostic promise covers that case. The same passive ancestry must surround
the pair; the facade must not temporarily escape it to obtain evidence.

Base begin returns the exact legacy reducer bool: false may mean valid no-change.
Admission is separately recorded in the token, so callers must not use that bool
to decide whether Base finish is allowed. Base null arguments/zero ID reject and
leave no admitted pair; a later finish returns a zero invalid result. For Styled
begin, preserve the legacy argument preflight and boolean composition contract:
false means no pair was opened and end must not be called; true admits the token
and requires end even when offsets did not change. An already-active token is
rejected before calling legacy begin. A matching finish result is `valid=true`
even when `changed=false`; invalid/mismatched finish is `valid=false` and has
zero callback count. Layout/capacity errors after admitted Styled begin still
require balanced box closure; `valid` is adapter admission, not transactional
rendering success. Existing Core diagnostics are not erased.

Passive Styled begin/end still execute the existing normalization, layout and
paint policy exactly once; frame-zero is not an argument failure or permission
to omit end. Base also preserves its existing maintenance behavior. A final
result can report `changed=true` with `change_count=0` in these scopes, because
no observer is dispatched without nonzero frame evidence. Admission and change
are therefore independent of callback eligibility.

The caller must use the same effective dimensions/style/options across its pair,
including disabled state, enabled axes, geometry, auto-measure and style lifetime.
Identity checks do not prove descriptor equality: mismatched options are unsupported
caller misuse, not silently reconciled. Preserve Styled's existing remeasurement
at end; the same options do not promise identical measured geometry. No hidden
option snapshot or retained stack-style pointer. Do not hide a second finish in a
passive getter.

`changed` means the finite final offset differs from a finite entry, or a
nonfinite entry was repaired to a finite result. `has_previous=false` leaves
`previous` zeroed rather than presenting NaN as a usable coordinate. A malformed
final nonfinite state is an adapter failure, not a fabricated CHANGE. Count is
actual callbacks, missing handler zero; no callbacks for net-zero changes or
pure drag-state/opacity changes. One aggregate may represent several wheel/key/
drag operations, including a return to entry that is intentionally silent.

**Normalization decision for review:** recommend including observed entry-to-final
clamping/axis-disable/range-shrink repair in this aggregate, because it is an
actual applied offset change and matches Base's old bool comparison. It is
explicitly not a user-input notification. Disabled ordinary submission may
normalize caller state and yield a final-offset CHANGE with genuine nonzero frame
and UNKNOWN source. Inert/layout-only frame-zero work may normalize state and
report maintenance `changed`, but dispatches no observer. Neither implies a user
activation/request. Preserve legacy reducer behavior; do not gate away its
normalization to satisfy a blanket disabled-event slogan. If user-only events
are required, that is a separate instrumented reducer design, not an inference
from this aggregate. External assignment before begin is the baseline and silent;
arbitrary external scroll-state mutation between begin/end is unsupported.

Metadata target is the public scroll owner, kind CHANGE, source UNKNOWN. Dispatch
only when the pair has matching genuine nonzero frame evidence; retain that frame,
omit event index/HAS_ACTIVATION. Frame-zero pairs emit no callback; several
inputs, range reconciliation or children can contribute. Do not select a last
wheel event and pretend it caused the final offset. Payload is pointer-free and
copyable; callback cannot mutate active scroll state/options/input, recursively
compose, destroy context or free active owner resources. Defer teardown until the
outer call/frame obligations finish. This is notification, never controlled veto,
scroll acceptance, gesture BEGIN/COMMIT/CANCEL or an animation-complete signal.

## VirtualScroll/VirtualList: preserve result-only window state

The [Base virtual adapter](../../src/base/cui_virtual_scroll.c) resolves finite
model extents, calls Base Scrollable, then processes focusable-owner main-axis
keys, updates semantics/last viewport and computes its typed view. Its
`scroll_changed` includes initial normalization and final Base/key offset changes.
It does not execute child render callbacks or finish after them.

The [Styled adapter](../../src/components/cui_virtual.c) may first invoke intrinsic
measurement, runs Base, then renders visible/overscan items from that computed
view. After callbacks it calls scroll finish, adjusts viewport translation, marks
`scroll_changed`, and sets `window_pending` only if deferred main-axis scrolling
changed the rendered window's offset. It invalidates for a subsequent frame.
The returned `view` remains the one used for row callbacks, not a recomputed claim
that newly visible rows were rendered. Cross-axis finish can change offsets
without main-axis `window_pending`. [Existing deferred-window assertions](../../tests/unit/test_virtual_styled.c)
prove pending on the first call and no pending on the next stable call.

Recommend **no window-completion observer**. `window_pending=false` means no new
pending deferred change in this call; it does not prove rendering/GPU presentation,
async data arrival or completion of a previously retained request. There is no
retained request generation to complete. A callback would duplicate a useful
result and encourage side effects during rendering. Applications can inspect
results and invalidate as already implemented. Never rerender the new window in
the same call or fire a callback for each row as a proxy for completion.

A later virtual final-offset facade, if requested, can wrap the entire existing
Base or Styled call with one entry/final comparison and preserve the original
`cui_virtual_widget_result` separately. It must not reuse the internal Base event
pair in a way that misses owner keys or double-delivers after finish. Measurement
and row callback users/models are borrowed through their existing composition
boundary; keys/heights must remain unchanged during row callbacks. No new retained
subscription or item selection ownership is appropriate.

## MessageScroller is a distinct double-precision model

[Public contracts](../../include/cui/cui_message.h),
[Base reducer](../../src/base/cui_message.c),
[Styled composition](../../src/components/cui_message.c), and
[tests](../../tests/unit/test_message.c) establish opaque caller-owned bookkeeping,
explicit append/prepend/splice/measure/navigation operations, double offsets and
stable row tracking through `cui_end_frame`. State/collection mutation during row
rendering is prohibited until end_frame, stronger than callback-only borrowing.

Base first applies previous rendered-row bounds as measurements, performs layout
(which can anchor/follow/clamp offsets), then captures its local `old` scroll.
Wheel/owner keyboard/scrollbar updates and jump action follow. Its `changed` also
includes measurement and jump/fade opacity changes; `jumped` is an action result.
Neither is equivalent to entry-versus-final scroll change. Styled width changes
can remeasure before Base; rows render after Base, and tracked bounds become
measurement input on a later call. There is no Styled Scrollable finish call here
and no virtual `window_pending` contract to forward.

Do not adapt MessageScroller by float-casting `view.scroll`, treating `changed` as
an offset event, or retaining row callbacks until a fictional completion. Keep its
existing typed result for now. If demanded later, propose a separate double
`previous/value` observation around the whole call, with pre-layout entry access
inside the model and exact normalization/anchor/tail semantics; use distinct
payload names or double offsets rather than weakening this model's precision.
Append/splice and measure operations are not message-received/read notifications.

## Verification and alternatives

First implementation ownership should be new events header, narrow Base/Styled
scroll files, focused test and guide only; root owns build/export/binding wiring.
Before implementation review the proposed complete token and normalization policy.
Required headless assertions:

- Exact legacy state, consumption, semantics, layout/paint and bool meanings for
  old/new paths; no-op, repeated input and net-zero aggregate; handler absent.
- Child-deferred nested wheel delivery at finish, correct current translation,
  one observer after child/paint closure, no replay and no duplicate dispatch.
- Horizontal/vertical/two-axis ranges, precise/nonprecise wheel, edge trapping,
  axis locking, scrollbar pointer drag/page/key and hidden/auto-hidden bars.
- Range shrink, disabled-axis/disabled-owner, nonfinite entry repair, prior-frame
  auto-measure normalization; distinguish these from user input explicitly.
- Inert/layout-only normalization/layout preserved with frame-zero maintenance
  results and no callbacks; omitted owner has no callback. Token identity/layer/
  reuse errors and differing nonzero frames reject atomically. A frame-zero
  cross-frame test cannot assert a diagnostic that the API cannot establish;
  document that misuse and test suppression instead.
- Base false/no-change still admits finish; Base invalid begin does not. Styled
  false begin requires no end; successful no-change/passive begin requires end.
  Check `valid`, `changed` and callback count independently, including normalized
  disabled ordinary work versus frame-zero maintenance.
- Nested tokens, frozen copied payload after context destruction, callback timing
  and unsupported mutation documented; strict C11/C++ header construction.
- Preserve virtual deferred-window/result tests and MessageScroller double/tail/
  measurement regressions without adding their production facades in this slice.

An alternative first API could take a whole-call content callback instead of a
public begin/end token. It would execute begin, invoke borrowed content, then
mutable end and notify in one stack frame, avoiding unprovable cross-frame token
identity. Costs: a new callback composition convention, user lifetime through the
whole call, inability to interleave arbitrary caller begin/end control flow, and
an additional failure/early-return contract for content. It still needs passive
callback suppression and normalization rules; it does not create input evidence.
Recommend the compatible paired form with the explicitly limited checks above,
not a hidden Core architecture change. A whole-call convenience wrapper can be
reviewed independently if applications prefer that tradeoff.

Callbacks at begin miss deferred work; callbacks at both phases double-deliver;
per-input notifications need internal mutation instrumentation and different
semantics. A context-retained listener unnecessarily extends userdata lifetime.
Frame-end hooks retime side effects. Recomputing virtual rows in finish changes
render callbacks and layout. None is part of the recommended bounded slice.

Run warnings-enabled isolated tests through `python scripts/test_local.py
--build-dir <build>` (private Xvfb), never a desktop session. Browser tests remain
headless. No runtime tests/builds were run for this documentation-only proposal.
Bindings retain existing notification allowlists; no shared queue producer,
synchronous foreign veto or foreign scroll callback is proposed here.

## Implemented Base slice

The first implementation exposes only the Base pair and shared types described
in [SCROLL_EVENTS](../SCROLL_EVENTS.md). The complete Base-only scope has no Styled
layer field because there is no Styled facade yet. Finish consumes the token before
observer dispatch; typed count is bounded to one and overflow remains false.
Frame-zero maintenance and normalization policy follow the approved refinement.
The focused fixture plus legacy scroll/virtual/message and strict C++ header checks
verify this slice independently. Styled declarations above remain proposals, not
public symbols. No virtual/window/MessageScroller event implementation is included.

## Implemented Styled slice

`cui_scrollable_begin_events` / `cui_scrollable_end_events` now use the same
aggregate policy after exactly one legacy mutable end/translation/paint/closure.
The unreleased token appends `private_styled`, checked by both finalizers before
any work; consumers must rebuild. Each layer freezes final payload and consumes
its token without calling the other layer's observer/finalizer. Callback borrowing
is through the matching end only. There is still no virtual-window completion or
MessageScroller event facade. See the guide for begin admission versus change and
frame-zero limitations. Native strict Release and ASan/UBSan bounded regressions
include old/new Styled paint/geometry and layer rejection tests.
