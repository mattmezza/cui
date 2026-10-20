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

Recommended first slice: an additive, typed HoverCard-only extended entry point
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
