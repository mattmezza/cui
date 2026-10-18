# VirtualList and MessageScroller precise observation checkpoint

Status: design only, audited against `9c61c158`. No production facade is
implemented by this checkpoint. This follows the [shared contract](COMPONENT_EVENTS_CONTRACT.md),
[Scrollable slice](SCROLL_EVENTS_SLICE.md), [implemented Scrollable guide](../SCROLL_EVENTS.md)
and [lifecycle/result checkpoint](LIFECYCLE_RESULT_EVENTS_SLICE.md).
All 78 manifest families remain; this neither adds a family nor closes their
binding or lifecycle omissions.

## Recommendation and boundary

Implement additive, call-local aggregate final-state observers in two independently
reviewed slices: VirtualList Base/Styled, then MessageScroller Base/Styled.
Existing architecture supports these without changing legacy callback timing,
render counts, reducers, result layouts or ownership. Keep virtual window completion
result-only. Keep MessageScroller explicit model mutators result-only initially.
Neither scroll observation requires renderer/data lifecycle events or a retained
listener. These are actual remaining notification gaps, not implemented parity.

A VirtualList wrapper can snapshot public state and invoke the corresponding
legacy operation once. A MessageScroller wrapper needs a private state snapshot
before any reconciliation; it must not call layout merely to obtain an entry
value. Its opaque double scroll has no public read accessor. This is a bounded
implementation requirement, not a reason to expose internal state or float-cast
it. Before implementation, approve the call-local policy below: operations between
submissions are outside observation, and observer callbacks cannot mutate active
model/state. A requirement to observe every explicit append/scroll/reset operation
would require a separate explicitly targeted operation facade or retained ownership
design; these wrappers cannot supply it retrospectively.

## Audited state and execution

| Surface | Actual owner, result and seam |
| --- | --- |
| [`cui_virtual_scroll.h`](../../include/cui/cui_virtual_scroll.h) | Caller owns widget state (`cui_scroll_state`, last viewport) and model. Model carries double extents; widget offsets are floats. `view` uses model coordinates; `scroll_changed` and `window_pending` have distinct meanings. Measurement and row callbacks are composition services, not notifications. |
| [Virtual Base](../../src/base/cui_virtual_scroll.c) | Validates/resolves model scroll options, snapshots x/y, runs Scrollable reduction, then owner Home/End/Page/axis keys and semantics. Keys reject consumed events and Ctrl/Meta/Alt. Returns resolved view and entry/final float change. No children/finish pass. |
| [Virtual Styled](../../src/components/cui_virtual.c) | Resolves theme and optional intrinsic measure (which can reject before reduction), calls Base, renders its view once, closes window/viewport, runs `cui_base_scrollable_finish`, translates existing boxes, paints bars and closes root. Deferred main-axis movement invalidates and sets pending; cross-axis movement can change offsets without pending. Returned `view` still describes rows rendered before finish. |
| [`cui_message.h`](../../include/cui/cui_message.h) | Application owns records; opaque state owns keys/heights, double offset, anchor/tail policy and animation/layout bookkeeping. Collection and state must agree and remain stable until `cui_end_frame` completes. Renderer runs visible rows plus 400px overdraw. Result separates valid/view, changed/jumped, opacity/disabled and wake time. |
| [Message model/Base](../../src/base/cui_message.c) | Reset/splice/append/prepend/measure/remeasure/to-item/to-end/scroll/layout can alter offsets or following policy immediately. Layout clamps and restores anchors; following tracks maximum. Base first reconciles previous tracked row bounds, then lays out, then takes its local old offset, reduces wheel/keys/bar, updates semantics/layout, animates and accepts jump action. `changed` includes measurement and opacity changes and misses some pre-local-snapshot offset movement. It cannot gate an exact observer. |
| [Message Styled](../../src/components/cui_message.c) | Width change may remeasure before Base; then row callbacks and tracked bounds, scrollbar/fade/jump composition run. No VirtualList-style finish or pending window exists. Tracked bounds write into live state at frame completion; their reconciliation belongs to a later call. State must remain alive through that write. |

Preserve exact legacy filtering, including Message Base's target scan without an
explicit consumed-event guard and focused-owner key handling without VirtualList's
modifier filter. Do not silently repair it in this event slice. Wheel consumes only
when offset changes; a key or jump need not imply a net offset change. The float
scrollbar adapter and semantic coordinates do not define MessageScroller's double
model precision. Neither row rendering nor `jump_content` may become a new action
callback. No existing scroll-change observer must be chained at these seams.

Existing tests provide evidence, not missing observer coverage:
[`test_virtual_styled.c`](../../tests/unit/test_virtual_styled.c) covers wheel,
keys/bars, disabled, deferred-pending then stable, cross-axis/large coordinates and
intrinsic sizing; [`test_virtual.c`](../../tests/unit/test_virtual.c) covers model
operations; [`test_message.c`](../../tests/unit/test_message.c) covers anchoring,
tail, rendering/interactions, animation and top-padding measurement preservation.
None proves typed callback counts, copies, suppression or legacy/observer parity.

## Proposed typed contract

Use separate new portable headers; do not append listeners to old state/options.
Names below are proposals. Add `cui_base_virtual_list_events` and
`cui_virtual_list_draw_events`, with the old arguments plus a borrowed handler.
Return a new wrapper result containing the exact legacy `control_result` plus
`valid`, `changed`, `change_count`, `count_overflow`. `valid` requires a successful
legacy virtual result; `changed` independently compares entry/final x/y, even when
no handler runs. Reuse `cui_scroll_change`/`cui_scroll_handler` and their float
representation, including `has_previous=false` and zero previous pair if either
entry coordinate is nonfinite. Do not promote the model's double `view.scroll`
to an offset the float widget never owned. Exact finite inequality gates change;
net-zero is silent. Styled observation occurs after finish/translation/bar paint/
box closure, not after Base and not once per rendered row.

Add `cui_base_message_scroller_events` and `cui_message_scroller_events` with a
borrowed handler and wrapper result containing unchanged legacy `control_result`,
`valid`, independent `offset_changed` and `tail_changed`, separate zero/one delivered
counts and `count_overflow`. Proposed pointer-free payloads:

```c
/* Schematic fields, not installed declarations. */
struct message_offset_change {
    cui_component_event_metadata event;
    double previous, value;
};
struct message_tail_change {
    cui_component_event_metadata event;
    bool previous, value;
};
```

`tail` means `following_tail`, not `scrolled_up`, proximity, visibility or a new
message. Compare both independently: tail policy can change with identical offset.
Do not use epsilon for double offset notification; preserve the reducer's existing
1e-6 tail policy without using that tolerance to erase real offset changes.
Private state normally guarantees finite offset; unexpected nonfinite state is not
a supported mutation path and must not be converted to fabricated previous data.
Snapshot offset and following before Styled width remeasure or Base row-bound/
layout work. Freeze final payloads before any observer, deliver offset then tail,
after the entire old function once. No nested Base observer inside Styled. Both
payloads describe the same completed operation even if a callback modifies
unrelated application data. Animation-only `changed`, wake updates and total/window
changes with identical offset/tail produce no notification.

Callbacks are CHANGE, UNKNOWN source, no input index or activation. Aggregate
normalization/anchor/keys/wheel/measurement/jump cannot identify one input cause.
Include only a genuine nonzero context frame; never manufacture source, index,
activation or programmatic attribution because an app may have edited state earlier.
Counter index is not an input index. Follow existing Scrollable submission/protected
phase policy: reject unsupported protected entry before work; ordinary admitted
frame-zero maintenance preserves legacy work/results but suppresses callbacks.
Disabled VirtualList normalization can notify in an ordinary frame. Notification
counts track actual callbacks, zero with missing handler or suppression; change
flags still describe applied state. Bounded zero/one delivery makes overflow false.

Payload and metadata are plain values with no borrowed text, rows, keys arrays,
model pointer, context or theme pointer. Copy by assignment into caller-owned
storage; copies survive callback/frame/context lifetime. Handler/user are borrowed
only until return and never retained for pending windows, animation or bounds
reconciliation. The copy guarantee does not extend the state/renderer's lifetime.

## Mutation, omission and compatibility

Observer runs after legacy composition callbacks and before return. It may copy
payloads and update unrelated state. Mutating/freeing active model, record collection,
state, options/theme, input, or tracked bounds storage, recursive frame/component
submission and context destruction are unsupported. Message state/records remain
stable through end_frame even after observer returns. Defer teardown accordingly.
Measurement callbacks retain their old restrictions and rejection timing.

Programmatic mutations before entry become the new baseline, without a delayed
callback on the next draw. Changes caused inside the observed layout/remeasure pass
are included, but carry UNKNOWN origin. Explicit mutators still execute synchronously
without callbacks; omission retains no handler and emits nothing. Repeated calls
observe each call's net change, with no once-per-frame deduplication. Result reads
never replay notifications. Applications choose results or observers for effects.

Virtual deferred final offsets can differ from `control_result.view.scroll`.
Preserve that mismatch and pending invalidation: never rerender rows to make a
payload agree with the old view, recompute/replace returned view, or call finish
twice. Pending false means this call has no new pending deferred window, not
render completion. Message tracked bounds and opacity wakeups do not schedule
future observers. There is no global capture/bubble, cancellation/veto, retained
registration, data receipt/read, paint/GPU completion or context-destruction callback.
Bindings and bridge allowlists remain unsupported until separately designed.

## Alternatives and implementation gates

Keeping both families result-only is compatible and allocation-free, but leaves
applications to reconstruct exact notification and cannot use Message `changed`
for offset/tail. A generic Scrollable facade misses virtual owner keys, risks a
second finish, and loses Message double/tail semantics. Retained model observers
would broaden mutation, target identity and destruction obligations unnecessarily.
Per-input events would require reducer instrumentation and a different multiplicity
contract. Choose call-local aggregate observation for this bounded follow-up.

1. Virtual Base/Styled: new event header, thin shared finalization helper, wrappers
   and focused tests. Preserve old API/layouts, row counts and pending results.
2. Message Base/Styled: private read-only snapshot seam shared by wrappers, no new
   opaque-state fields or eager layout. Existing Base and Styled duplicate the
   private state definition; do not add observer storage or independently diverge
   those definitions. Snapshot before all maintenance; preserve old results.
3. Integrator updates discovery/source metadata, consumer guidance, generated skill
   and distribution resources; retains every manifest entry. No automatic foreign
   callback exposure. Each production slice needs strict C11/C++ header checks,
   warning-enabled build and private-display CTest before signed submission.

Meaningful headless tests must compare old/new result fields, final state, consumed
input, semantics and geometry on equivalent contexts. Virtual tests include empty
and invalid/intrinsic rejection, nonfinite normalization/disabled, net-zero ordered
keys, missing handler, horizontal/cross-axis changes, real child-deferred movement
with one callback after rows and unchanged rendered view/pending, then stable next
frame with none. Verify render and measure counts, no double finish and no callback
from omission/protected/frame-zero work.

Message tests include offsets beyond float exactness with fractional deltas,
pre-layout reconciliation, width remeasure/anchor restore, prepend/measure between
calls remaining silent, tail-only changes and independent offset/tail ordering,
Home/End/bar/jump, preserved consumed/modifier behavior, opacity-only silence,
invalid/zero ID, missing handlers and protected/frame-zero work. Compare exact
legacy animation/wake/jumped fields; prove snapshot precedes remeasure rather than
mutating state through a getter. Retain copied payloads after later frames and safe
teardown, without destroying context during a callback. Demonstrate tracked bounds
reconcile only on later submissions and do not retain handlers. Run native tests
with `python scripts/test_local.py --build-dir build`; browser tests remain
headless. Hardware/GPU claims remain deferred. This documentation-only checkpoint
needs diff checking, not a new runtime test or build.

## Implemented VirtualList slice

`cui_virtual_events.h` now provides the Base/Styled call-local wrappers described
above. Read [VIRTUAL_EVENTS](../VIRTUAL_EVENTS.md) for the exact implemented
contract. Existing row/measurement callback contracts and permitted behavior are
unchanged: the no-active-mutation restriction applies to the new observer only.
MessageScroller production APIs in this checkpoint remain proposals. Virtual
window completion remains result-only. No manifest or binding parity promotion.
