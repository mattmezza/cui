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

## Approved implementation refinement

Use private Base and Styled common functions. Legacy entrypoints pass no observer.
After each existing Button addon interaction, preserve its old aggregate flag and
last-addon result write, then its existing semantic details. Only then enumerate
actual accepted activations for that addon in input order. Copy genuine snapshot
when available; if accepted ID is proven but snapshot unavailable, use only actual
frame/index/type-source facts, never fabricate activation. No accepted entry means
no synthetic callback merely because an aggregate button boolean was true.

Addon descriptor traversal still precedes exactly one nested Input operation.
When on_text is supplied, call existing `cui_base_input_events` and forward the
same handler user, preserving actual per-event counts/control/errors. Otherwise
use the old Input entrypoint. Styled calls this observed Base once at its old
seam, then performs its unchanged paint/custom rendering once. No hidden workspace,
allocation, second reduction, aggregate text wrapper or Core change.

Compatibility boundary: current Styled InputGroup does not forward masked or
content_type from behavior options into its Input paint options. The event slice
must preserve this old paint path for paired equivalence, while secure event
payloads inherit the existing observed Base Input redaction. Do not claim secure
paint parity. A separate review/fix must audit actual secure paint fields and
prove no secret text leaks through draw commands, semantics or selection paths.

## Event compatibility implementation checkpoint

Implemented the private Base/Styled common paths and new public event facade,
without changing legacy layouts or secure paint forwarding. Tests compare old/new
state/results/focus/redraw/semantics/paint, addon traversal versus input order,
per-event text/submit counts, rejection/partial failure, IME/undo, masked/password
payload redaction, disabled/loading/readonly distinctions, actual inert/layout
scopes, custom child isolation, missing handlers, invalid input and deferred
omission. Secure Styled paint remains the separate reviewed follow-up; event
redaction does not imply visual masking. No binding or full parity promotion.

## Secure Styled InputGroup fix checkpoint (review required)

Source audit after compatibility commit `52c6d3d5`. No production implementation
of this section exists. The approved event reducer and callback timing remain the
baseline; this is a separate correction to existing Styled secure rendering.

### Actual gap and purpose distinction

`src/components/cui_form.c` builds `cui_input_options paint_options` without
`o.input.masked` or `content_type`, reduces through Base Input, then calls
`cui_input_paint`. Base already sets secret/password semantics, configures host
text-input purpose and redacts events using the real behavior options. But paint
reads actual committed/composition text unless its own `options.masked` is true.

Merely forwarding masked is insufficient: `cui_input_paint` consumes a private
prepared mask cache and otherwise paints blank. `cui_input_common` in
`src/components/cui_components.c` currently owns mask binding, staged checked
presentation, masked caret/hit conversion, visual geometry, cache completion and
paint. Reuse that path; do not implement a second bullet string builder, fake edit
model, post-edit allocation, second reducer or paint raw text as a failure fallback.

`masked` alone controls visual hiding. PASSWORD/NEW_PASSWORD with masked=false is
intentionally PASSWORD_VISIBLE: actual text remains visible, while semantic value
and event text remain redacted. Preserve that public distinction, including secure
lifecycle/history/clipboard policy; do not automatically turn purpose into masking.

### Exact private layering and shared seam

Recommend two small private headers so Base never depends on components types:

1. `src/base/cui_form_private.h`: declare the existing common Base group operation
   with an optional synchronous nested-input delegate/user. Delegate signature uses
   only Base types: context/input ID/name/edit/behavior/options/text handler and
   returns `cui_input_event_result`. Legacy Base entrypoints use NULL delegate.
   Delegate runs exactly at the existing nested Input seam, **after** addon
   traversal/notifications and hit-test fallback construction, before input-parent
   association. It does not dispatch addon work or paint. No retained registration.
2. `src/components/cui_input_private.h`: declare a shared Styled preparation/reduce
   helper using existing components options/state types, plus explicit output for
   final paint options and whether paint is valid/required. It accepts optional
   full Base behavior overrides, the same text handler, and an observation flag.
   It returns the exact nested input control/count/error result. Its local mask/hit
   scratch lives through the one synchronous Base Input reduction; retained mask
   buffers/geometry continue using the existing resource ownership, not new owners.

Factor the existing `cui_input_common` preparation/reduction into this helper,
keeping standard Input's order and behavior. Standard Input then paints once when
its existing path would paint. Specifically preserve its existing early-invalid
no-paint returns versus masked-preparation-error disabled/secret invalid paint.
No early `cui_input_paint` may remain in a helper used by InputGroup, or Group would
paint twice. No new public API or mask allocator/storage option on InputGroup.

Styled Group uses a stack delegate descriptor with its resolved frameless input
paint options/theme and temporary `cui_input_state`. Its common Base call selects
this delegate only for masked Styled input. Unmasked and purpose-only controls
retain the exact event-compatibility path, apart from forwarding explicit paint
masked/content_type fields. At the delegate seam call shared Styled preparation
and exactly one Base Input operation, then copy result/counts normally. After Base
returns, perform the unchanged group/addon layout and one `cui_input_paint` using
prepared masked cache and the finalized invalid/placeholder paint options.

No symbol from Base invokes components directly; only the optional synchronous
function pointer supplied by Styled crosses that seam. Base-only builds have no
components link dependency. Core, unrelated public APIs and generated sources stay
untouched. If factoring cannot preserve standard Input failure branches exactly,
stop for review rather than hiding behavior changes in the helper.

### Behavior override and custom policy

The helper must start from the complete supplied `cui_base_input_options`, not
silently rebuild a subset. Preserve disabled/readonly/required/invalid, clipboard,
validate/user, normalization, delegate_up_down/delegate_home_end, format/workspace,
matcher and secure purpose. Use the Group's resolved font, input viewport and
scroll for default masked geometry. Forward masked/content_type/format/workspace
and relevant validation fields into the shared Styled configuration. Preserve
all existing theme/size/design/frameless padding/height resolution; no new public
paint configuration is invented.

**Explicit custom hit_test:** preserve it and hit_test_user unchanged; its existing
contract returns actual model UTF-8 byte offsets, so the caller owns the mapping
policy. Distinguish a caller-provided hit test from Group's internally synthesized
plain-text fallback: when no caller hit test was supplied, replace that internal
fallback with the shared masked visual hit test. Never use real-text width for
masked default pointer positioning. Document this responsibility and test custom
callback/user forwarding and multibyte returned offsets.

**Explicit custom presentation:** recommend rejecting masked Styled Group when
`presentation.prepare` or `presentation_complete` is configured, with
CUI_EDIT_INVALID_ARGUMENT at the nested-input seam. A mask stage and arbitrary
external transactional presentation cannot safely share the existing single
presentation slot without a separately designed composition/rollback contract.
Do not silently replace the caller's prepare/complete/user/workspaces or call both
without engine rollback support. Malformed descriptors retain normal validation;
unused user-only fields must not be dereferenced. This is an explicit correction
for a previously unmasked visual path, not a new ownership transfer. Base Group,
unmasked Styled Group and purpose-only PASSWORD_VISIBLE keep custom presentation
unchanged. If compatibility requires composing these policies rather than this
narrow rejection, obtain that decision before implementation.

### State, atomic failure and timing

Initialize temporary `cui_input_state` from Group's exact behavior and scroll_x.
Use the Group's existing tracked input viewport and resolved metrics, not a second
persistent state or a new owner ID. Shared preparation/reduction owns temporary
hit scratch until it returns; no stack pointer may remain in retained mask state.
Copy behavior back immediately after nested reduction, including error paths so
focus/composition/secure lifecycle cleanup is not lost. Copy scroll back after
existing paint updates it; track the same viewport ID as before. Do not reset
scroll each frame. Preserve error/control/count delivery and addon-before-text
ordering. Text observers still run inside the existing Base Input reducer before
outer Group semantics/paint; no deferred notification or extra reduction.

Initial mask preparation or custom-policy rejection must perform no text-event
mutation, acceptance callback or text notification. Register a disabled, readonly,
secret/invalid textbox using the existing masked Input failure policy and paint
fail-closed (empty display if no ready mask); never disclose source text. Preserve
the owner's text/revision/selection/history, except documented lifecycle state
cleanup already done by the shared path. Do not claim whole-group rollback:
earlier addon callbacks already happened and remain counted.

During editing, mask staging remains a hard checked presentation gate before
acceptance/commit. A failed growth/geometry/format stage leaves the rejected edit's
model/revision/selection/history and last good displayed mask intact; no CHANGE
for it. Earlier successful events and callbacks remain, first error wins as in
Input. Distinguish initial preparation error from later per-event failure. Do not
retry or replay events, overwrite errors, or turn failures into raw-text paint.
Composition preedit must use the same staged provisional mask and cancellation
policy; no secret preedit in draw commands. Security-mode changes retain existing
Input lifecycle cancellation and history policy.

### Evidence and test plan

Existing references to preserve and reuse:

- `tests/unit/test_password_input.c`: masked text, selection/caret, clipboard/history,
  mask allocation/storage and failure behavior, semantic secret/password state.
- `tests/unit/test_password_present.c` and `test_password_engine.c`: checked prepare,
  borrowed candidate and atomic presentation failure/rollback engine contracts.
- `tests/unit/test_input_number_control.c` and `test_input_format_control.c`: masked
  formatted display, caller storage capacity rejection before acceptance, retained
  model/selection/paint on error.
- `tests/unit/test_input_pointer_selection.c`: actual pointer selection policy and
  custom hit-test offsets, including masked mode transitions.
- `tests/unit/test_input_group_events.c`, `test_form.c`/Base-only and
  `test_text_input_events.c`: exact one-pass reduction, inherited redaction,
  callbacks/multiplicity, bounded geometry and baseline aggregate behavior.

Add dedicated secure Group cases to the owned focused test (or a new focused test
with root registration if approved): prepopulated multibyte secret displays the
right bullet count, never raw secret or secret preedit in any text draw command;
secret/password semantics expose zero value, correct host HIDDEN versus VISIBLE
purpose; actual selection/caret/scroll and pointer hit-test use masked geometry;
composition commit/cancel, undo/redo, mode changes and repeated frames stay coherent.
Prove purpose-only password stays visibly unchanged while events remain redacted.
Compare legacy and observed Styled entrypoints under the **fixed** path, preserving
all event counts/order and result fields; retain original unmasked equivalence.

Test custom hit_test/user untouched, custom presentation rejected only for masked
Styled (no prepare/complete calls, no mutation/text callbacks, fail-closed paint),
and unchanged Base/purpose-only custom presentation behavior. Exercise failure
before first edit and failure after accepted progress; use existing shared Input
caller-storage/allocator fault fixtures for helper staging regressions rather
than pretend Group exposes an allocator it does not. Include nonfinite/invalid
metrics and missing mask resources without assuming atomic rollback of unrelated
addon actions. Verify no double semantics/paint/reduction or stale temporary state.

Strict C11/private-wrapper tests: Group events, Form/Base-only, Input events,
password_input/present/engine, secure_focus/generation, format/number controls and
pointer selection, plus C/C++ headers. Root owns CMake/WASI/docs registration and
dist/SDK regeneration. Scope extension requested: `cui_components.c`, the two
private headers above, existing owned Form source/tests/guide/checkpoint only.
No production changes before checkpoint review; tests described here are planned,
not claimed passing coverage of the proposed fix.
