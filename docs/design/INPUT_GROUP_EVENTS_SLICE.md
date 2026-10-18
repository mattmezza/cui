# Input Group event checkpoint

Design only, audited at `be4820a6`. No declarations below are implemented.
Preserve all 78 families, old entrypoints/options/state/results and their timing.

## Existing state and reduction

[Form declarations](../../include/cui/cui_form.h) expose caller-owned edit model,
input behavior/scroll/viewport state and at most 64 borrowed addon descriptors.
Base [implementation](../../src/base/cui_form.c) validates IDs, labels, geometry
and enum values before registering the group and every addon in source order.
Button addons call Base Button; other addon kinds register a group. The last
activated addon in descriptor order becomes `result.activated_addon`; it is not
a chronological action log or a callback. Loading/disabled addons are disabled;
input readonly does not independently disable a button addon.

After addons, Base calls `cui_base_input_ex` once using the existing hit-test
fallback, then attaches input semantics to the group. Input owns actual editing
and validation, composition, history, secure redaction and Enter behavior.
[Styled](../../src/components/cui_form.c) resolves sizes/theme and bounded addon
geometry, calls Base once, and paints input/addons. Addon render callbacks are
presentation hooks, not built-in addon action callbacks. A custom renderer may
compose its own controls; those controls retain their own events.

Base Input already has an optional common path in
[`cui_base.c`](../../src/base/cui_base.c) that delivers per-event committed CHANGE
and eligible SUBMIT through `cui_base_input_events`, preserving the legacy
aggregate. Reuse that exact path instead of inventing a second text reducer or
wrapping the entire group with a final-text comparison that loses its ordering.

## Proposed additive API and ordering

New `cui_form_events.h` may include Form, shared metadata and Text Input events:

```c
typedef struct {
    cui_component_text_fn on_text;
    void (*on_addon)(void *, const cui_component_event_metadata *);
    void *user;
} cui_input_group_handler;
typedef struct {
    cui_input_group_result control;
    size_t addon_count, change_count, submit_count;
    bool count_overflow;
} cui_input_group_event_result;
```

Propose `cui_base_input_group_events` and `cui_input_group_events`, mirroring the
old arguments plus handler and returning this new result. A narrow private common
Base/Styled path must preserve every validation, hit-test, semantic and paint step.
Legacy entrypoints pass no observer. New Base invokes the existing observed Input
once when text observation is requested, retaining its exact control/errors and
actual counts; missing text handler retains old work and zero text counts. New
Styled calls observed Base once and paints once; it must not wrap another observed
Styled Input or replay any addon/input reduction.

At each existing Button addon interaction, preserve its aggregate boolean and
last-addon result write. If its new observer is present, deliver each actual
accepted routed activation belonging to that addon, in input order for that
addon. Use the same accepted-activation lookup, not queued-event type guessing.
Different addons retain descriptor traversal order; all addon notifications
precede the nested Input's event-ordered text notifications. Do not sort callbacks
across those phases or move them after paint. Missing addon handler ignores user
and preserves all legacy flags and child semantics. Custom/text addons have no
intrinsic new action; custom child Buttons retain their own API.

Text payloads reuse `cui_component_text_event` unchanged: metadata target is the
actual nested input ID, CHANGE/SUBMIT and redacted secure text rules match Input.
Addon ACTIVATE metadata target is the actual addon ID; caller knows the group
from the call/userdata. Preserve genuine accepted snapshot and input facts when
available, using honest absent evidence otherwise. No fabricated group activation,
old text, request rejection, COMMIT or generic form submit is implied.

Observers see the current reducer state before later group semantics and Styled
paint. Text count can exceed one; legacy input changed/submitted flags remain
aggregates and last-addon remains last in descriptor order. Reading results never
replays callbacks or mutations. Applications choose one side-effect path per
action. No new handler is retained, no workspace/allocation, no generic routing,
network stack or Core change is required.

## Lifetime, suppression and compatibility

Input's existing edit acceptance and value-owner model remain authoritative.
Notifications cannot reject an already applied edit, consume an input by return,
stop propagation or prevent a platform default. Preserve the old treatment of
consumed/cancelled activations at each existing interaction seam. Disabled/loading
addons and disabled/readonly input retain their distinct existing rules; readonly
input is not a blanket addon disable. Passive/inert/layout-only/omitted submission
must emit no new callback while preserving old maintenance.

Text bytes are borrowed through the callback only; scalar edit snapshots and
activation metadata copy by value. Secure payloads remain redacted. Bounded UTF-8
copying uses the shared copy helper. Handlers/users/descriptors/state/model and
frame resources must stay live; no active model/state/descriptor/input mutation,
recursive submission or context destruction during observers. Unrelated flags
and deferred removal after outer call plus renderer consumption are supported.
Legacy validation/render hooks keep their old timing and mutation rules; arbitrary
host edits in a later custom renderer are not replayed as nested Input events.

## Independent implementation and evidence

Before production, review exact accepted-activation and observed-Input forwarding
seams. Worker scope: new public header, Base/Styled Form common paths, focused
new test, new guide and this checkpoint status. Root owns test/build/WASI/docs
registration, metadata and generated distributions/SDK/resources. No Core, old
Input reducer or foreign notification schema changes.

Tests need paired old/new owner/state/result/focus/semantics/paint checks;
multiple addon activations with descriptor-order versus event-index distinction;
repeated text CHANGE/SUBMIT and rejected edits; addon-before-input timing; readonly
input with enabled addon; loading/disabled addon suppression; custom child action
isolation; missing handlers; invalid admission without work; secure redaction;
partial edit failure after earlier callback; passive/inert/omitted frames; copied
payload and supported deferred removal. Keep existing Form/Input regressions and
strict C11/C++ headers, private-wrapper native tests and portable WASI checks.

The bridge's existing InputGroup generic text producer and opt-in owning TEXT poll
are separate post-step queue contracts. They do not expose these proposed
synchronous per-event/addon handlers, rejection or native multiplicity. Binding
omissions must remain explicit until independently implemented and tested.
