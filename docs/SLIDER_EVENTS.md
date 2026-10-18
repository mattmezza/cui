# Controlled Slider events

Include `cui/cui_slider_events.h` for `cui_base_slider_events`. This additive,
drawing-free facade uses the existing controlled double-precision Slider reducer.
Submit it inside a box with the same ID. The legacy
`cui_base_slider_controlled`, `cui_slider_options` binary layout, phase timing and
capture/presentation state remain unchanged.

```c
static void position_request(void *user, const cui_component_slider_request *r) {
    double *owner = user;
    if (r->phase == CUI_SLIDER_UPDATE)
        *owner = r->requested; /* Accept. Leave unchanged to reject. */
    /* COMMIT is a final proposal, not proof the owner accepted an UPDATE. */
}
/* Persistent state: */
cui_slider_state drag = {0};
double position = 0.25;
/* During submission inside a same-ID track box: */
cui_component_slider_result result = cui_base_slider_events(
    ui, track_id, cui_string("Position"), position, &drag,
    (cui_slider_options){.minimum=0, .maximum=1, .step=0.01},
    (cui_component_slider_handler){position_request, &position});
```

This header exposes the Base reducer. Use the separate
[Styled facade](STYLED_SLIDER_EVENTS.md) for themed track/thumb rendering. Source bounds,
clamping, step rounding, pointer capture and normalized float semantics match the
old controlled reducer; requested values and owner snapshots remain double.

## Typed phases and owner decisions

| Phase | Shared metadata kind | `requested` | Result count |
| --- | --- | --- | --- |
| BEGIN | LIFECYCLE | Current owner value at gesture/key start | `begin_count` |
| UPDATE | REQUEST | Proposed clamped/snapped value | `request_count` |
| COMMIT | COMMIT | Final proposed gesture/key value | `commit_count` |
| CANCEL | LIFECYCLE | Gesture starting value | `cancel_count` |

No confirmed CHANGE notification is emitted. BEGIN/CANCEL describe lifecycle,
not proposed UPDATEs; COMMIT does not imply application acceptance. CANCEL does
not restore external owner storage: the owner decides whether to restore its
value from `requested`. State `preview/start/dragging` belongs to the reducer;
do not mutate it from the handler. It is presentation/capture state, not a hidden
confirmed value.

`current` is the immutable value passed to this invocation, including for later
callbacks after an owner assignment. For two Right keys in one submission both
propose from that same entry value, preserving the existing controlled policy.
An active gesture's cancel value can differ from that invocation's `current`.
Owner acceptance is observed on the next submission. Same-call semantics also
retain the entry value. Request input consumption and application acceptance are
separate; rejection does not unconsume or propagate anything.

The old `on_request` and `user` options must both be NULL. Conflicts reject before
submission, state mutation or callback dispatch. A missing new handler is read-only.
Disabling, making read-only, removing the handler or making the range zero-width
cancels an active drag according to the old reducer. If a handler remains, an
option-driven CANCEL is delivered with its precise reason. Removing the handler
cannot deliver a callback; capture still stops and counts stay zero.

## Genuine evidence and copied lifetime

Each input-driven callback carries the actual local frame sequence/event index,
known pointer/keyboard source and a pointer-free scalar input snapshot. All
callbacks from a single key or pointer-down/up event share that input identity;
counts are phase callback counts, not distinct input counts. No activation
snapshot is fabricated for pointer movement, release, generic cancel or keyboard
phase dispatch. `HAS_ACTIVATION` is absent even for BEGIN/COMMIT.

A generic `CUI_EVENT_CANCEL` has UNKNOWN source and no `HAS_SOURCE`; its actual
input type/index still exist. Do not infer a pointer origin just because unused
pointer fields have defaults. Option-driven cancellation has `has_input=false`,
UNKNOWN source and no frame/index/activation presence flags. Its reason is
DISABLED, READONLY or ZERO_SPAN (that precedence if several apply). It is not an
input event or an invented programmatic action. Input-driven reasons are INPUT.
Only presence flags establish shared evidence; zero unused fields are not proof.

The input snapshot deliberately excludes text pointers, inline text, composition
and unrelated event payloads. The entire `cui_component_slider_request` is thus
copyable by value beyond callback/frame lifetime; frame identity remains local
to that context. Handler/user are borrowed for the call only. No registration,
retained callback or event queue is added by the facade.

Do not recursively submit frames/controls, destroy context/state/users, free
active storage or mutate drag state during dispatch. Record teardown/omission
requests in app state and apply them after frame completion/renderer consumption.
Changing the application's owner value is supported; general reentrancy is not.

## Results and verification

`valid/submitted` are true when the existing reducer admitted this active
submission, including disabled/read-only submissions; outside active submission
or on invalid arguments they are false. Later rendering can still fail under
Core's existing frame contract. Counts increment only when the new callback is
actually invoked and saturate at SIZE_MAX with `count_overflow`; dispatch is not
stopped by saturation. Reading/copying results never invokes a callback. Do not
perform the same side effect in both the handler and the frame-count consumer.

`tests/unit/test_slider_events.c` checks actual queued pointer/key batches,
ordered phases, double values, genuine frame/index/source and contact data,
absence of invented activation, owner rejection/acceptance and immutable
same-call values, passive idle frames, copied payload lifetime, semantic snapshots,
Escape/generic/option-driven cancellation, missing-handler read-only cleanup and
invalid/conflicting-option atomicity. Existing `test_slider_controlled.c` and
`test_base.c` remain behavioral regressions for the original path. Native tests
run through `python scripts/test_local.py --build-dir <build>`.

Base and Styled tests establish portable reducer behavior and software rendering.
They do not establish all-family uniformity, browser/physical-touch/GPU acceptance
or bridge/binding exposure. Headers and sources are distributed through the normal
source metadata/amalgamation/SDK tooling.

For themed rendering with the same double-valued phases, use the implemented
[Styled Slider facade](STYLED_SLIDER_EVENTS.md). Foreign request handlers remain unsupported.
