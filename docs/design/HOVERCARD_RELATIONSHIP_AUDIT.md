# HoverCard dismissal and accessible relationship audit

Design checkpoint only. No production change or parity-status promotion is authorized
by this document. CUI baseline: `876c654e`; frozen reference checkout:
`87d10ae5e1299d1be18670a37570c5402f952e34` (`/tmp/cui-gpui-reference`).

## Source findings

The manifest item combines several independent promises. Existing behavior is not
absent merely because the combined item remains `not_started`.

| Concern | Frozen upstream | CUI baseline | Remaining scope |
| --- | --- | --- | --- |
| Desktop dismissal | Base HoverCard schedules close after leaving trigger/content; epoch cancellation prevents stale delayed work | Caller-owned reducer advances explicit dt; trigger focus, panel hover and contained child focus can hold open | Different scheduling contracts; preserve CUI ownership |
| Escape | No explicit Escape handler in base HoverCard or its Popup host | Shared Tooltip/HoverCard scanner treats every Escape key-down as dismissal, including consumed, repeated, modified and composing input | Consumption-aware policy would be a CUI improvement, not demonstrated upstream parity |
| Mobile dismissal | Mobile trigger click toggles; content installs left mouse-up-out dismissal | No equivalent mobile-specific toggle/outside-dismiss policy in this helper | Explicitly deferred |
| Open notification | Base set_open changes state, notifies render, then synchronously calls current cloned on_open_change; same-value calls do nothing | Existing pure/styled lifecycle observers report aggregate model changes | Already supported, with different lifetime/timing guarantees |
| Accessible relationship | No explicit controls/expanded/labelled-by relation in base/component HoverCard or Popup source | Open panel is GROUP with accessible name and parent=trigger; trigger is not patched with controls/expanded; Tooltip instead patches trigger description | Add a bounded CUI semantic relationship; upstream source does not establish this requirement as implemented |

Reference authorities: `crates/base/src/hover_card.rs` (HoverCardState::set_open,
schedule_open/schedule_close, RenderOnce), `crates/component/src/hover_card.rs`
(wrapper), `crates/base/src/popup.rs` (placement/deferred occluding host), and
`crates/component-shell/src/shell/overlays/hover_card.rs` (typed on_open_change
forwarding). The reference tests exercise delayed pointer open/close, notification
sequence, mobile toggle, content click retention and outside dismissal. These
sources do not prove an upstream native accessibility mapping or Escape contract.

CUI authorities: `src/base/cui_content.c` (`cui_hover_update` and pure event
adapter); `src/components/cui_content.c` (`cui_content_hover_panel`, begin/end and
styled event adapter); `include/cui/cui_content.h`; `include/cui/cui_hover_events.h`.
Invalid viewport/anchor closes the model after reduction. Resource failure may
reject composition while leaving the model open. Thus open, begin admission and
completed visibility are distinct facts. Focus-inside retention uses previous
rectangular containment, not semantic ancestry; do not strengthen that claim.

## Compatibility and input ownership

`tests/unit/test_hover_styled_events.c` explicitly freezes the old behavior:
its step 4 consumes a repeated Ctrl+Escape before submission and still expects
the same close/suppression in legacy and observer paths. Changing the shared scan
to ignore consumed Escape would break a tested contract and also change Tooltip.
The pure reducer takes policy booleans; it cannot infer event acceptance.

Core exposes `cui_event_consumed` and `cui_consume_event`. Core also associates
Escape with a previous focus scope when one exists, but the current hover helper
never checks that target. A consumption check alone is not focus ownership,
topmost-popup arbitration or prevention of multiple open cards closing. A later
submitted consumer cannot undo an earlier HoverCard decision. Consumption is
sampled at the helper call, not retrospectively. Do not introduce a dispatcher,
focus trap or a fabricated accepted-activation event to solve this slice.

Initial relationship-only alternative (superseded as the recommended first slice
by the mobile amendment below): an additive, typed HoverCard-only extended entry point
with an explicitly enabled relationship policy. Existing entry points, Tooltip,
options layout and all-zero/absent policy retain their exact behavior. This can
reuse the private panel helper with default-preserving internal policy parameters;
no Core architecture or retained state ownership changes are needed.

A separate opt-in `ignore_consumed_escape` policy can be included only with its
own documented semantics and tests: filter consumed indices before the existing
Escape predicate, leave repeat/modifier/IME/global scope behavior unchanged, and
do not consume Escape on behalf of the card. This policy is not exclusive input
ownership. It must not silently become the legacy default. If strict no-behavior-
change scope is preferred, ship relationship only and defer this policy.

## Proposed semantic contract

The trigger must already be submitted in the current frame, as the current API
requires. Patch its copied semantic details, preserving description, required,
readonly, focus-target, release-key policy and all unrelated fields. Never create
an invented trigger node or edit the previous frame as a fallback. The explicit
opt-in grants this card ownership of trigger controls/expanded for this call;
one relation field cannot represent several cards sharing one trigger.

After the final begin decision, write controls=card_id and expanded=true only
when the panel was actually admitted. Otherwise clear this owned controls relation
and write expanded=false, including disabled, Escape, pending close/open,
invalid-anchor/viewport and resource-failure paths. A pending close whose panel
remains admitted stays expanded. Do not derive expanded solely from state.open.
When no current trigger semantic exists, leave semantics untouched and report
relationship application failure separately from begin admission; children/end
balancing must continue to depend only on admission. Tests must pin this result.
Omitted calls apply no patch and cannot replay a transition or clear caller data;
ordinary immediate-mode trigger resubmission supplies fresh details.

Retain the panel GROUP role, accessible name and existing parent=trigger. Do not
add labelled_by=trigger automatically: the card may have its own explicit name,
and this slice supplies no name-merging contract. Do not turn the card into a
modal dialog, assert focus movement, or expose a dismiss accessibility action.
Do not add a public field just to label a callback as dismissal.

## Existing notification timing

Reuse `cui_begin_hover_card_events` semantics through an additive extended sibling
if observation and the opt-in policy need to be combined. Snapshot handler and
frame at entry. Observe the final legacy-equivalent model after begin/relationship
work and before arbitrary admitted children; end is still required exactly once
on true admission. A close observation happens after reducer/fallback mutation,
not after portal removal has been rendered or after `cui_end_hover_card`.
Do not add a second dismissal callback or silently change the existing result:
`changed` is independent of handler; `change_count` counts callbacks, not inputs.
Payload remains UNKNOWN source, genuine eligible frame only, no input index,
activation evidence, cause or completed-visibility claim. Multiple Escape inputs
can produce one close. Protected/frame-zero work keeps legacy maintenance and
suppresses callbacks. Keep existing restrictions on callback mutation/reentry;
no post-callback second state write or veto protocol is proposed.

## Accessibility proof boundary

`cui_semantic_details` already contains controls, expanded and parent. Core setter
replaces the full details record, so preserving unrelated data is necessary.
`backends/accesskit/cui_accessibility.c` translates controls only when the target
exists and emits expanded only when true or the node is TREEITEM. Consequently,
setting expanded=false on an ordinary button does not currently prove an explicit
collapsed property reaches AccessKit. A controls target absent while closed is
also filtered. Core semantic tests alone cannot establish collapsed AT behavior.

The bounded first implementation may honestly prove the CUI semantic snapshot
and admitted-panel controls mapping, while recording collapsed platform state as
unsupported. Full expanded/collapsed AT proof needs a separately reviewed adapter
presence policy (the present boolean cannot distinguish unspecified from false),
adapter snapshot assertions, and relevant native integration verification. Do not
infer live screen-reader announcements, speech, navigation or mobile AT behavior
from headless semantic snapshots. No adapter or public Core schema change is
approved in this checkpoint.

## Required production proof before status changes

- Preserve existing content, pure hover and styled hover event tests, including
  consumed/repeated global Escape on all legacy entry points and Tooltip.
- Add focused headless extended-API cases: closed/pending/open/pending-close,
  Escape suppression/rearm, disabled close, invalid geometry, resource rejection,
  missing trigger, omission and two independent cards. Assert controls/expanded,
  parent, names, preserved unrelated details and exact begin/end balance.
- If consumption opt-in ships, compare consumed and unconsumed Escape, repeated
  and modified input, preexisting suppression, and call-order boundary; assert
  no new consumption or claimed exclusive ownership. Keep legacy fixtures intact.
- Compare absent-policy legacy and extended draw commands, bounds and model;
  test contained-child focus retention and intrinsic content placement unchanged.
- Exercise observer before children, after final semantic patch, no-handler result,
  copied payload lifetime, frame-zero/inert/layout-only callback suppression and
  final-model fallback close. Never assert completed visibility from notification.
- Use strict portable C11/C++ header checks and the repository Xvfb test wrapper;
  no desktop windows. Adapter proof is additional scope, not implied by these tests.

Bridge currently renders HoverCard using legacy begin/end and separately reports
its before/after model state. This checkpoint adds no bridge options, foreign
callbacks, owned-content lifetime changes or Python/Node/TypeScript/Go/Rust API.
Document those omissions if the C slice ships. Mobile tap/outside dismissal,
exclusive Escape routing, live AT announcements and explicit collapsed adapter
state remain separate work. The combined manifest item cannot be called fully
complete from relationship snapshots alone. Root/integrator owns any approved
metadata, generated SDK/distribution and consumer skill updates after review.


## Review amendment: mobile toggle and bounded outside release

The first implementation proposal must cover genuine frozen mobile behavior as
well as the manifest relationship requirement. Relationship snapshots alone do
not close the source gap. This amendment supersedes the relationship-only first-
slice recommendation; all legacy compatibility and AT limitations above remain.
No production work is authorized by this amendment until separately reviewed.

### Accepted trigger input and explicit mode

Propose one additive extended HoverCard API with a typed policy containing
DESKTOP_HOVER (zero/default) and TAP_TOGGLE modes, relationship opt-in, explicit
accepted-trigger input and optional bounded outside-release tracking. Retain the
existing caller-owned hover state; add a separate caller-owned mobile workspace
rather than appending fields to the public existing state/options. No automatic
platform detection: the application selects touch/mobile policy. TAP_TOGGLE can
be selected on desktop for headless tests and mouse/pen accessibility consistency.
The old functions and an absent extended policy use the exact old helper path.

The application submits its trigger first and passes whether that trigger actually
accepted activation in this frame, using its Button bool/event result. The API
must name this input accepted_trigger_activation, not raw pressed/tapped. False
means no toggle; it must never be inferred from pointer-up coordinates, focus or
cui_interaction_state alone. This is an explicit application assertion, not a
claim that a public boolean is cryptographically validated. The implementation
can prove its documented integration using real Button acceptance and cancelled-
activation tests; arbitrary callers can still supply an incorrect assertion.

An optional copied activation snapshot may accompany the assertion only when
obtained at the trigger acceptance point. Validate target and current frame if
used; never turn absent evidence into an invented source/index/press. If detailed
snapshots add unnecessary scope, omit them entirely and keep UNKNOWN lifecycle
metadata. In particular do not rescan events after Button consumption to decide
whether its already accepted activation happened: accessibility acceptance can be
cancelled by later consumption, while ordinary accepted activation has different
rules. No change to either Core contract is proposed.

Snapshot entry open before applying outside dismissal or the accepted toggle.
A true trigger assertion sets open=!entry_open once, coalescing the aggregate
Button result; it does not toggle once per raw event. This matches the frozen
mobile trigger listener's captured rendered-open value and its explicit comment
about outside dismissal handling the same release first. If several transitions
must be represented within one frame in the future, a bool is insufficient; that
requires a separately specified ordered accepted-input API.

TAP_TOGGLE ignores hover/focus and open/close delays. It cancels pending elapsed
policy work, and keeps the open card until accepted trigger toggle, permitted
outside release, disabled close, legacy-global Escape or geometry fallback.
Retain the old Escape predicate, including consumed/repeated/modified input;
Escape wins over a toggle in the same aggregate call, with suppression preserved.
Do not ship ignore_consumed_escape as an accidental new default. The desktop
mode keeps its old reducer/containment rules. Closed or disabled mode, omission
resumption and explicit mode changes reset the outside workspace, preventing a
stale contact from closing a later card. On resume after a skipped frame, reset
pending contact instead of assuming omitted input did not cancel it.

### Outside release support that current Core can prove

Core authorities: cui_event_target resolves against previous-frame regions and
may identify captured content even after the pointer leaves it. The scope-hit
helpers require active focus scopes; current HoverCard creates none. Final
cui_pressed is current capture state, not per-release history. Core touch handling
tracks contacts, drag threshold, generated cancellation and scroll suppression,
but no public universal accepted blank-space release snapshot exists. A blank
outside tap therefore cannot honestly be described as an accepted widget
activation. Existing overlay/surface outside policies dismiss on pointer-down;
they cannot be reused unchanged to claim the frozen mouse-up-out contract.

Propose an explicitly bounded outside-tap policy with one caller-owned pending
contact: record primary down outside both previous valid panel and trigger bounds,
its type/device/contact, starting point and frame. Accept a matching unconsumed
primary up outside both regions only if no cancellation or drag invalidated that
contact and ordinary submission/modal admission still permits the card. Reject
nonfinite/unknown bounds, missing down, unmatched up, secondary button, mode or
context reset, and ambiguous simultaneous contacts. Observe matching cancel even
if consumed, so cancelled state cannot survive until a later release. Consumed
matching down/move/up, scroll/gesture takeover, or movement reaching an explicit
8-logical-pixel radial threshold invalidates the pending tap; do not rearm within
that contact. Include final up displacement. This threshold is the new policy's
bounded rule, not a promise of universal host gesture equivalence. Movement must
be processed in input order even when the final pointer returns near its start.
Do not consume input or capture the outside target on behalf of this card.

A down inside card/trigger never arms outside dismissal. Thus child clicks stay
open, trigger release is owned by the accepted trigger assertion, and a drag from
card to outside does not dismiss. A blank outside down/up does dismiss, including
across adjacent frames. Touch cancellation, scrolling and pinch must retain the
card. Multiple contacts fail closed for dismissal rather than guessing the primary
finger. The tracker must see relevant events every submitted frame; it is not a
second global dispatcher and does not establish topmost-popup ownership. Modal
checks use existing cui_pointer_scope_allowed; unrelated overlays without a
shared ownership protocol remain outside this guarantee.

This strict outside-tap contract intentionally narrows frozen mouse-up-out, which
does not itself require an outside press origin or a movement threshold. Preserve
that distinction in status/consumer guidance: required frozen happy paths can be
covered while drag/cancel safety receives its own explicit rule. If exact arbitrary
mouse-up-out semantics are also required, expose a separately named raw-release
policy and document its drag behavior; never claim the strict tap mode is exact
full parity. Prefer the strict policy first for the user's touch requirement.

### Composition, observation and proof

Apply the final mobile open decision before normal shared placement/admission.
Reuse the existing observer contract after final model and relation patch, before
children, without retaining handlers or changing payload/result meaning. A toggle
plus outside close coalesces into the final model transition; no fabricated input
multiplicity or completed dismissal event. Lifecycle metadata stays UNKNOWN/frame-
only even when the application has supplied a real trigger snapshot: this
aggregate can also include Escape, disabled or placement fallback. Relationship
reflects actual admission, and collapsed AccessKit limitations remain explicit.

Before production approval, required focused tests must additionally use actual
Button accepted activation (mouse, touch, keyboard and accessibility as policy
permits), cancelled trigger release and accessibility consumption timing; mobile
hover/delay no-op; open, trigger retoggle, content click retention and blank outside
release; same-frame trigger/outside ordering; cross-frame down/up; cancel before
up; drag across threshold and back; consumed release; card-child drag out; contact/
device mismatch; second contact/pinch/scroll; mode switch, omitted frame, disabled,
Escape and invalid-geometry reset; modal blocking; no-handler results and callback
ordering. Keep the frozen upstream test scenarios traceable separately from new
safety cases. Tests run through the repository headless wrapper with assertions
and strict warnings. No new source, bridge/binding coverage, adapter promise or
full touch/full parity status is implied by this design-only checkpoint.
