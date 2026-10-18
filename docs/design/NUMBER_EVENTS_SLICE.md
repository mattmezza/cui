# NumberInput numeric CHANGE observer proposal

Design only, audited at `4ecd73dd`. NumberInput's owner is a **double**; its
borrowed `cui_edit` is a separate draft. Propose one aggregate committed numeric
CHANGE observer per submission. Do not reconstruct REQUEST, per-event CHANGE,
COMMIT or SUBMIT from the existing aggregate result. No Core, bridge, dispatcher,
legacy callback, state layout or generated SDK change belongs in this proposal.

## Actual reduction and rendering

The public [`cui_number_state/options/result`](../../include/cui/cui_numeric.h#L23)
separate owner value, initialization/last-value bookkeeping, focus history, invalid
draft state, composition and scroll state. `cui_number_result.changed` is final
numeric inequality, `submitted` is an aggregate Enter flag, `step_direction` is the
last attempted step direction, and `error` is an edit result, not a transaction
rollback indication. None is an event count or a unique cause.

[`cui_base_number_input`](../../src/base/cui_numeric.c#L292) rejects a nonfinite
owner, multiline editor, invalid bounds or absent required pointers before normal
reduction. Initial submission or an owner differing from `last_value` formats that
owner into the draft. This can change edit revision without changing the owner.
The reducer then captures its numeric original, handles composition/input/steps,
parses the final draft, publishes semantics, updates `last_value/was_focused`,
sets `changed = *value != original`, and invalidates on numeric change.

Valid draft text updates the owner even outside minimum/maximum until Enter or
editable blur requests clamping. Incomplete drafts such as `-` can change edit
revision while preserving the last owner and setting `invalid`. Rejected text
can report INVALID_ARGUMENT while leaving the prior valid owner/draft. Enter
sets `submitted` even without a numeric change and without the activation API's
nonrepeat qualification; blur can normalize without Enter. The reducer's final
commit flag coalesces the frame; it does not identify one commit event. Preserve
all these behaviors rather than adding new emitters inferred from flags.

[`cui_numeric_try_step`](../../src/base/cui_numeric.c#L251) starts from the owner,
uses a parsed draft when available, records attempted direction, and only writes
the stepped owner after rewriting the draft succeeds. The public
[`cui_number_step`](../../src/base/cui_numeric.c#L93) uses `step_by(user,value,sign)`
as a **step magnitude policy**, not a resulting-value callback. It is called
before bound no-op checks; finite positive magnitude, finite arithmetic and an
actual numeric difference are required. Do not call it twice, turn it into a new
request callback, veto it through an observer, or change its timing/side effects.
A step at a bound may run policy and emit no numeric CHANGE.

On commit, [`final parse/clamp`](../../src/base/cui_numeric.c#L426) writes the
clamped owner and tries to normalize the draft. If normalization fails, owner is
set back to the **parsed draft value**, not necessarily the entry owner, while the
first edit error is retained. Earlier mutations are not rolled back wholesale.
Initial draft synchronization and later operations can also leave error results;
no proposal may equate `error != OK` with "nothing changed."

[`cui_number_input`](../../src/components/cui_numeric.c#L79) validates Styled
metrics, width and string storage first, then reduces Base exactly once, then
paints from `s->edit`, composition/invalid state and computed metrics. It does not
read the owner again to rebuild displayed text after Base. Its semantic value is
[`float-clamped`](../../src/base/cui_numeric.c#L150), but that does not narrow the
actual double owner. Payloads must not reuse semantic float as numeric truth.

## Exact additive API proposal

Place declarations in a separate `cui_number_events.h`; Base declarations depend
only on existing Base/numeric and shared metadata headers. The Styled declaration
may share that header as with other family facades.

```c
typedef struct {
    cui_component_event_metadata event;
    double before, after;
} cui_number_change_event;
typedef void (*cui_number_change_fn)(void *user, const cui_number_change_event *event);
typedef struct {
    cui_number_change_fn change;
    void *user;
} cui_number_event_handler;
typedef struct {
    cui_number_result control;
    size_t change_count; /* Actual observer invocations: zero or one. */
} cui_number_event_result;
cui_number_event_result cui_base_number_input_events(
    cui_context *, cui_id, cui_str name, cui_number_state *, double *,
    cui_number_options, cui_number_event_handler);
cui_number_event_result cui_number_input_events(
    cui_context *, cui_id, const cui_theme *, cui_str name,
    cui_number_state *, double *, cui_number_options, cui_number_event_handler);
```

An empty handler still performs ordinary reduction; a non-NULL user pointer with
an absent callback is accepted and ignored, matching immediate Input/selection
handlers. `step_by/step_user` remain independent legacy policy callbacks. No
retained registration is needed: handler/user are borrowed for this call only.

Capture the valid finite double owner at entry, before native mutation; after
native reduction finishes capture its finite final double once. Emit iff exact
C numeric inequality `before != after` holds, independent of error and handler
presence. With an observer, freeze both doubles and shared metadata, then invoke
it once after Base's final bookkeeping/semantics/invalidation and before Styled
painting. Count actual calls only. An absent observer yields zero count, while
`control.changed` retains native meaning. Preserve every control result field.
Do not infer change from draft revision, text length, `submitted`, step direction,
error, callback count, normalization, or consumed-event bits.

Use a private common implementation for the Styled seam, leaving its legacy
entrypoint on the no-observer path; never run a legacy Styled call followed by a
second Base call. All returns after mutation must run the change finalizer, even
when an edit error is already set. A later Styled paint/layout error cannot undo
an already completed owner change/callback. Preflight rejection before reduction
notifies nothing; nonfinite entry values are invalid input, not a NaN CHANGE.
Callback count does not certify rendering or input validity.

Metadata is CHANGE with the NumberInput root ID, source UNKNOWN and HAS_FRAME
only when `cui_frame_sequence` proves the same nonzero active submission before
and after reduction. That API excludes inert/layout-only ancestry and protected
phases. No index/source/activation flag is inferred from an aggregate. Notify only
for ordinary active, enabled, editable submissions: inert/layout-only/outside-frame,
disabled and readonly calls do not notify. Still run the original reducer exactly
once, preserving any legacy synchronization/normalization and diagnostics; lack
of callback is not a promise of no model mutation. Draft synchronization and
normalization are not evidence of physical input. Compare legacy/facade passive
state and owner behavior explicitly in tests rather than silently fixing Base.

## Equality, normalization and lifetime

Keep finite double precision with no float conversion, epsilon, decimal rounding
or bitwise comparison. Thus `+0.0` and `-0.0` compare equal and do not emit; distinct
representable finite values do, including subnormals. Several changes ending at
the entry value emit nothing. This is aggregate numeric observation, not edit
revision observation or an audit log of intermediate values.

`cui_number_parse` accepts strict finite ASCII decimal/exponent grammar, rejects
NaN/Inf, whitespace, commas and underflow to zero, and accepts representable
subnormals. Insertion separately normalizes supported full-width digits/signs and
punctuation before validating the candidate draft, with the existing 127-byte
limit. Do not add locale-dependent parser policy, enable exponent input when the
existing option forbids it, canonicalize signed zero for event comparison, or
reformat the owner a second time for notification. Invalid/nonfinite step magnitude
or overflow retains the existing no-op policy and emits nothing unless another
operation in the same submission actually changed the owner.

The payload is pointer-free and copyable beyond the callback. State/editor/value,
composition resources and owner/user storage must remain alive until the outer
call returns. No context/editor/state destruction, recursive submission, edit
mutation, selection changes or composition mutation during observer delivery.
The observer may assign a **finite** new application owner value through its own
captured pointer, or update unrelated application state. Its event remains the
frozen native before/after snapshot and does not recursively notify. Do not update
`last_value` or resynchronize the draft afterward: the next submission detects the
owner mismatch normally. Current Styled paint still shows the already-reduced
draft/semantics, not the observer's newly assigned value; this intentional boundary
must be in the public guide and tested. A nonfinite observer assignment makes the
next native submission invalid; it is outside the supported owner-update contract.
Do not alter `step_by`'s legacy policy contract to enforce these new observer rules.
Foreign exceptions must not unwind through C; language adapters would catch/report
them after returning from native code in a separate binding integration.

## Evidence and implementation gate

Existing [`models`](../../tests/unit/test_numeric.c#L35) exercises finite/subnormal
parse, invalid NaN/Inf syntax, locale-independent formatting, dynamic step policy,
nonfinite step and overflow. [`number_events`](../../tests/unit/test_numeric.c#L135)
covers draft `99`, Enter clamp, invalid `-`, subsequent valid negative text, blur
clamp, full-width normalization, readonly, disabled and external owner synchronization.
[`number_composition`](../../tests/unit/test_numeric.c#L602) covers IME paths; size,
geometry and part-button tests in the same file cover Styled composition.
The similarly named `test_input_number_control.c`/`test_input_number_engine.c`
exercise formatted text Input, a different control: do not count those alone as
proof of NumberInput numeric observation.

New headless tests must compare legacy and facade owner, draft, result fields,
semantics, focus and geometry. Cover one/multiple steps, changes cancelling to the
entry value, repeated Enter without numeric change, clamp on Enter/blur, pending
invalid draft, full-width/IME commit versus preedit, Undo/Redo, external owner sync,
readonly/disabled and invalid options. Exercise signed zero, adjacent doubles,
subnormals, NaN/Inf owner rejection, no-op overflow/bounds and `step_by` call order/
count. Force draft-capacity/normalization errors with actual edit storage to prove
postmutation owner-change delivery; do not mirror the reducer in a fake fixture.
Test zero/one observer counts, empty handler, ignored user data without callback,
retained payload copies and finite observer owner updates with next-frame draft
sync. Keep legacy tests unchanged, compile strict C11/C++ headers, and run native
CTest through the private headless wrapper before a production slice is proposed.

Bridge NumberInput currently queues legacy CHANGE from its aggregate result; its
V1 payload remains LEGACY, not the reserved SCALAR tag. No new typed bridge schema,
per-event delivery or cross-binding parity follows from this C-only proposal.
