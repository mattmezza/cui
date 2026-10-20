# Alert completed-dismiss observer checkpoint

Status: **design checkpoint only; no production observer is implemented**.
Audited at `e220b5ca`. This document narrows the intrinsic Alert gap identified
in [the render-family audit](AVATAR_EVENTS_SLICE.md) and
[remaining coverage](COMPONENT_EVENTS_REMAINING.md). It does not promote those
ledgers or claim per-input, full-family, foreign-binding or platform parity.
The [shared contract](COMPONENT_EVENTS_CONTRACT.md) remains authoritative.

## Existing behavior that must remain exact

[Public Alert options](../../include/cui/cui_content.h) contain an optional
caller-owned `bool *visible`, render slots and their `user`; they contain no
Dismiss listener. [The implementation](../../src/components/cui_content.c),
`cui_alert`, performs this sequence:

1. Reject null UI/theme, an invalid kind, or `visible != NULL && !*visible` at
   entry. The visibility guard runs once. There is no second guard after slots.
2. Submit the Alert box/semantic and header/icon, then title content unless
   banner mode suppresses it, body content, and action content in that order.
   These render callbacks run before the built-in Dismiss Button and may change
   caller-owned visibility or consume input using their existing facilities.
3. If dismissible, invoke `cui_button` with the derived child ID
   `cui_content_child(id, 1)`, the exact text `Dismiss`, and `disabled=false`.
   `cui_button` delegates to `cui_button_ex` with only `.disabled=false` in an
   otherwise zero-initialized options record. This is the legacy Button style,
   not an explicit design-size, Ghost, icon-only or release-policy replacement.
4. The Button processes the entire accepted activation batch, paints and
   completes its normal work, then returns an aggregate bool. Alert assigns
   `*visible = false` **once, after that Button return**, if the bool is true
   and storage exists. It does not assign visibility inside a per-input hook.
5. End the Alert box and return that aggregate bool. With no visibility pointer,
   the same accepted Dismiss action still returns true. It cannot guarantee the
   caller will omit the Alert next frame.

Multiple accepted Dismiss inputs can therefore produce one assignment and one
true return. A slot setting visibility false before the Button does not cancel
submission already in progress. It does not itself cause a true return. The
existing assignment still runs after a true Button return even if the value is
already false. An arbitrary action-slot child activation is not intrinsically
an Alert dismissal. No ownership acceptance, veto, Escape/outside close policy,
retained open state, animation completion or post-frame delivery exists here.

Relevant regression evidence:

- [Content](../../tests/unit/test_content.c) checks real Dismiss pointer input,
  subsequent hidden omission, light/dark Alert colors, SVG icons and semantics.
- [Alert/Bubble slots](../../tests/unit/test_alert_bubble_slots.c) checks slot
  ordering and foregrounds, banner title suppression, interactive custom slots,
  accessible naming and initially false visibility suppressing slot execution.
- [Button](../../src/components/cui_components.c), `cui_button_ex`, registers
  the Button, iterates accepted events, calls legacy/typed click listeners per
  event, then paints. Its aggregate bool remains true for any accepted event.
  [The shared Button facade](../../include/cui/cui_components.h) exposes a
  different, per-input contract; it must not redefine Alert's assignment timing.

These tests do not yet prove the proposed observer or the multiple-input and
callback-driven visibility cases below.

## Recommended first additive API

Add a native C-only `cui_alert_events` facade and dedicated pointer-free typed
completion payload. Leave `cui_alert_options` and `cui_alert` unchanged.
Proposed declarations, subject to implementation review:

```c
typedef struct {
    cui_component_event_metadata event;
    bool visibility_assigned;
} cui_alert_dismissal;
typedef void (*cui_alert_dismiss_fn)(void *user,
                                    const cui_alert_dismissal *dismissal);
typedef struct {
    cui_alert_dismiss_fn callback;
    void *user;
} cui_alert_dismiss_handler;
typedef struct {
    bool valid, submitted;
    size_t dismissal_count;
    bool legacy_dismissed;
} cui_alert_dismiss_result;

cui_alert_dismiss_result cui_alert_events(
    cui_context *, cui_id, const cui_theme *, cui_alert_options,
    cui_alert_dismiss_handler);
```

This is a **coalesced completed-dismiss-action observation**, zero or one
notification per invocation. `dismissal_count` summarizes the eligible observed
completed dismissal, never accepted input multiplicity or callback invocations.
It is one when an ordinary active, non-omitted submission completes the built-in
Dismiss action, independently of whether a callback is installed. Missing
callbacks do not change interaction results. No overflow flag is necessary for
this bounded 0/1 count. `legacy_dismissed` is exactly the bool the old function
would return; passive/ineligible contexts retain that legacy bool independently
of the observation count and cannot emit a notification. Reading the result does
not dispatch or replay an observation.

`visibility_assigned` records only whether the existing false assignment ran.
It is true even when the value was already false; it is false when no storage
was supplied. It is not a changed-value, accepted request, net transition,
retained closed state or promise about what the owner submits next frame.
The payload contains no owner pointer, message, slot resource or fabricated
before/after value.

Metadata describes the aggregate built-in action: `kind=ACTIVATE`,
`target=public Alert ID`, `source=UNKNOWN`, and only `HAS_FRAME` with the actual
active submission frame. UNKNOWN does not set `HAS_SOURCE`. `HAS_INDEX` and
`HAS_ACTIVATION` stay absent; zero-filled unused fields are not evidence.
Do not search for a last accepted child event after return, choose one of several
inputs, claim a pointer origin, copy coordinates, or infer programmatic origin
from the absence of input evidence. This deliberately conservative contract
also applies when only one input happened. It does not label dismissal CLOSE,
CHANGE, COMMIT or LIFECYCLE.

[Toolbar's aggregate observer](../../include/cui/cui_toolbar_events.h) is useful
precedent for bounded synchronous aggregate observation; its optional
single-activation evidence recovery is **not** part of this Alert proposal.
[Shared metadata](../../include/cui/cui_component_events.h) supplies immutable
presence flags without requiring a new dispatch architecture.

`valid` means the new facade's narrow preflight passed: nonnull UI/theme,
nonzero ID, valid kind and structurally readable supplied string descriptors.
This does not guarantee all later rendering succeeded. `submitted` means an
ordinary active frame and a non-omitted Alert at entry. An initially false
visibility value is a valid omission with `submitted=false`; a nondismissible
Alert can submit normally with no dismissal. Inert/layout-only submission
retains its legacy visual/slot behavior with `dismissal_count=0` and no user
dismissal callback, independently of the legacy return. Do not introduce new legacy-entry rejection rules or suppress existing slots
merely to normalize result bookkeeping.

## Preserve the existing owner-write boundary

Implement the facade as a `static inline` function in the new dedicated
`cui_alert_events.h`. Freeze the by-value options and handler, narrow preflight,
ordinary active submission eligibility and actual frame identity at entry. Then
call the existing `cui_alert` **exactly once**, including in passive contexts
where its existing visual/slot behavior must remain untouched. The legacy call
already returns after the optional owner assignment and `cui_end_box`.

Store its unchanged bool in `legacy_dismissed`. The frozen operation fact
`visibility_assigned = legacy_dismissed && options.visible != NULL` is sufficient:
the legacy true return proves that its optional false assignment ran. Set
`dismissal_count=1` only when the frozen entry eligibility and legacy bool are
both true, regardless of handler presence. Freeze the frame-only payload, then
invoke the optional observer before returning the result. No Button hook,
second reduction, `content.c` hook, public options layout change or `content.h`
declaration is needed. Passive/ineligible observation leaves the count zero;
it must not suppress or rewrite any legacy operation.

A private common implementation would also preserve the existing boundary, but
is unnecessary for this coalesced slice. The header-only wrapper is preferred:
it observes the actual legacy return directly, has no source ownership conflict,
and leaves the zero-option Button invocation, child ID, text, paint, semantics,
focus and input policy entirely in the existing function. This remains
synchronous submission-time observation, not end-frame or a queued close event.
Slot `user` and observer `user` remain independent.

The observer sees the false assignment already applied if storage exists. It
may deliberately set that owner value true again; the frozen payload, original
aggregate bool and 0/1 dismissal count do not change. Such an owner write is not
a second generated event and must not be overwritten after observation. Likewise,
visibility changed by earlier title/body/action slots follows the existing single
entry guard; do not recheck it to skip the Button or classify a fake transition.
No callback return value may veto an action already accepted and assigned.

Callbacks may copy the pointer-free payload or update application flags. Borrowed
UI/theme/options/resources, caller visibility and slot/observer users remain alive
through their existing submission/rendering boundaries. Copying this payload
survives frame/context destruction; retaining its callback pointer does not.
Recursive same-frame submission, context destruction, unwinding across C or
freeing/rebinding active resources is unsupported. Request teardown and apply
it after the frame. No guard, scheduler or destruction interception is added.

## Separate future alternative: detailed per-input activation

A future, separately reviewed observer could hook the existing typed Button click
boundary, retain genuine `cui_button_click_event`/activation evidence and report
each accepted Dismiss input in reducer order. Public Alert identity and the real
Dismiss child evidence target must remain distinguishable. No rewritten child
snapshot, synthesized accepted event or general routing is justified.

Those callbacks would execute **before** the Button returns, and therefore before
Alert's one aggregate visibility assignment. They can see or change a different
owner state from a completed-dismiss observer, and may consume later input before
that input is processed. Moving `*visible=false` into that hook, stopping after the
first input, or calling completion once per input breaks the legacy contract.
The first slice above intentionally does not expose or count these per-input
activations. Adding both observers later requires explicit dual-count and order
proof; it is not permission to reinterpret `dismissal_count`.

## Required implementation proof and exclusions

Before production, add meaningful real-input headless tests for:

- Pointer and touch, Enter/Space and genuine supported accessibility Dismiss
  activation; preserve child semantics, exact Button pixels/options and legacy
  bool/visibility timing, including no listener and no visibility storage.
- Two accepted Dismiss activations in one submission: the original Button batch
  completes, visibility is assigned once, completed observer runs once after
  box end, metadata has frame only, and the aggregate bool remains true.
- Initial false visibility; nondismissible; hidden/invalid/inert/layout-only
  cases; no notification for custom child actions, hover, unrelated input or
  external visibility writes without accepted Dismiss activation.
- Title/body/action order and independent users. Slots change visibility false
  or consume a later Dismiss input: preserve entry-only guard and actual Button
  outcome. Completed observer sees the applied assignment, can set visibility
  true again, and causes no extra synthetic completion or second assignment.
- Null visibility storage still yields true plus one observation on accepted
  dismissal, with `visibility_assigned=false`. Already-false storage at the
  write boundary still yields assignment fact, not a fabricated transition.
- Copied payload survives frame/context lifetime. An eligible completed dismissal
  has count one both with and without a handler; the callback-less case executes
  no callback and reading its result never replays one. Passive/ineligible
  contexts have count zero while preserving the independent legacy bool and
  mutations. Initial omission versus submitted state is explicit. Verify exact
  legacy drawing/semantic results alongside the new facade.
- Strict portable C11 and C++ header checks; targeted `content`,
  `alert_bubble_slots`, Button event regressions and a new Alert observer CTest.
  On Linux run only through `python scripts/test_local.py --build-dir <build>`;
  no desktop display or GPU parity claim.

Approved implementation ownership: new `include/cui/cui_alert_events.h`
containing all new types and the header-only facade, new focused tests and native
consumer guide/example. The dedicated header includes the existing content
header. Do not modify `src/components/cui_content.c` or
`include/cui/cui_content.h`; neither is needed for this slice.
Integrator owns registration, source metadata, generated SDK/distribution/skill
refresh and ledger decisions. This checkpoint changes none of them.

[The retained bridge](../../src/bridge/cui_bridge.c) currently constructs an
Info Alert with title/detail and no dismissibility/visibility pointer; its
arbitrary child actions are not this native operation. Python, Node/TypeScript,
Go and Rust do not export these proposed native handlers/results, synchronous
callback timing, owner-write facts or per-input activations. Existing foreign
queued notifications are not native observer proof; see
[binding coverage](COMPONENT_EVENTS_BINDINGS.md). No binding hook, close/veto API,
AlertDialog forwarding, dismissal policy, controlled request, or full Alert event
parity is part of this slice.
