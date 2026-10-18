# OTP aggregate notification slice

Design checkpoint, audited at `ffb39cd7`. No production, dispatch, bridge or SDK
changes in this checkpoint. Prefer one aggregate CHANGE observer first: the
legacy result already supplies operation/completion counts, but not intermediate
text snapshots or submission intent.

## Existing contracts and important differences

[`cui_otp.h`](../../include/cui/cui_otp.h) owns the public state/options/result.
State borrows a NUL-terminated caller buffer and capacity; `state.length` is the
**target cell count**, not current bytes or an editor revision. Targets are at
most 256, including zero. `cui_otp_count` counts UTF-8 code points, not grapheme
clusters. Programmatic `cui_otp_set_value` permits arbitrary valid UTF-8 and
values longer than the target; there is no 256-byte model bound. It changes no
per-frame result and invalidates caller-owned composition sessions. There is no
validation callback, editor revision, legacy observer or readonly option.

The [Base reducer](../../src/base/cui_otp.c#L164) validates state, UTF-8 and
options before interaction; it registers a textbox, updates focus transitions,
processes existing targeted/unconsumed events, updates blink state and final
semantic count/range, then returns `cui_otp_result`. No network verification or
credential-validity decision exists. Details that a facade must preserve:

- Ordinary text appends only the first ASCII/fullwidth digit, normalizing
  fullwidth to ASCII; it does not paste all digits from a text event. Ctrl/Meta/Alt
  text and IME-composing keydown are excluded. A full model rejects more digits.
- Backspace removes one UTF-8 code point, and is accepted even on empty text.
  Accepted ordinary operations increment `change_count`/set `changed` even if
  final bytes equal the entry bytes. Do not replace this with final inequality.
- Each accepted ordinary operation whose resulting count equals the target
  increments `complete_count`/sets `complete`. Thus deleting an overlength value
  down to the target can complete; empty Backspace with target zero also completes.
  Filling, deleting, refilling can produce multiple completions in one call;
  final text can be incomplete despite `complete=true` earlier in that call.
- Composition updates/cancellation can change session/paint without model CHANGE.
  Commit appends only its first eligible digit; duplicate stale commit does not
  append again. A successful append increments counts, including completion when
  reaching the target. Composition events are consumed; ordinary accepted text/
  Backspace do not explicitly consume in this reducer. Preserve that distinction.
- Capacity/UTF-8/boundary/session errors can coexist with earlier accepted changes.
  Ordinary append capacity failure sets an error and continues. Composition
  capacity failure can retain visible preedit. Never gate all observation on an
  entirely successful `error` result or roll back previous changes.
- Disabled prevents input but can report blur/cancel composition; caret time,
  focus and resource/session bookkeeping are independent of CHANGE. Inert and
  layout-only behavior must be measured against legacy, not described as frozen
  state merely because the new observer is suppressed.

[Styled OTP](../../src/components/cui_otp.c#L4) validates theme/design size and
resolves design metrics before Base once. It then paints cells from the final
model and active preedit, masking each visible code point with `*` when requested,
and updates the text-input rectangle. Base validation failure skips paint;
other errors do not. No callback currently sits between reduction and paint.

## Proposed first API

New `cui_otp_events.h` includes existing OTP types/shared metadata; old public
state/options/result layouts and entrypoints remain untouched.

```c
typedef struct {
    cui_component_event_metadata event; /* CHANGE, aggregate completion frame. */
    size_t before_bytes, after_bytes;
    size_t before_count, after_count, target_length;
    size_t accepted_count, completion_count; /* Legacy operation counts. */
    bool final_complete; /* after_count == target_length, not result.complete. */
    bool has_text;
    cui_str text; /* Final committed bytes, borrowed only through callback. */
} cui_otp_change_event;
typedef void (*cui_otp_change_fn)(void *user, const cui_otp_change_event *event);
typedef struct { cui_otp_change_fn change; void *user; } cui_otp_event_handler;
typedef struct {
    cui_otp_result control; /* Exact original result, including errors/counts. */
    size_t change_count; /* Actual callback invocations, zero or one. */
} cui_otp_event_result;
cui_otp_event_result cui_base_otp_events(cui_context *, cui_id,
    cui_otp_state *, cui_otp_options, cui_otp_event_handler);
cui_otp_event_result cui_otp_events(cui_context *, cui_id, const cui_theme *,
    cui_otp_state *, cui_otp_options, cui_otp_event_handler);
```

Capture entry scalar counts/bytes before reduction, call legacy Base exactly
once, then observe `control.changed`, including accepted no-ops and return to
entry. Do not replay `control.change_count` callbacks with the same final text.
The payload's `accepted_count` and `completion_count` preserve those original
counts, whereas outer `change_count` counts actual callbacks. A small whole-Base
wrapper suffices; Styled needs a private common seam preserving its preflight,
design resolution and verbatim paint, with notification between Base and paint.
No Base per-event extraction is needed for this first slice.

Notify only in an ordinary active, enabled submission: same nonzero
`cui_frame_sequence` before/after, valid entry, and no initial invalid-argument
failure. Preserve post-mutation error results and still notify for their accepted
changes. Absent callback accepts/ignores non-NULL user and reduces normally.
No synthetic notifications for programmatic setter calls, focus/blur, blink,
composition-only updates or reading results. Disabled/inert/layout-only calls
retain old reduction/result behavior and emit no callback. Metadata is root target,
CHANGE, UNKNOWN source, HAS_FRAME only. No source/index/activation can be recovered
from aggregate counts, even if only one change occurred.

## Completion is not submission

Shared [metadata kinds](../../include/cui/cui_component_events.h#L10) have no
COMPLETE kind. Do not map OTP completion to SUBMIT, COMMIT or surface LIFECYCLE.
Enter has no submission branch in the legacy reducer. This first slice exposes
completion counts and final completeness on CHANGE, without a separate complete
callback/kind. Applications can inspect those counts and choose separate deferred
verification work; no automatic verification is implied. Even final equality to
target is a length condition, not proof of a valid code, user intent or a fresh
transition. An already-full programmatic model emits no completion on submission.

A future per-event seam is feasible only by extracting the actual mutation loop
and composition-commit helper, notifying at their real acceptance points. It
would need event-local before/after text, zero-target/accepted-no-op semantics,
post-error ordering and consumption tests. Aggregate counts cannot reconstruct
those snapshots, completion-time values or temporal input provenance. Defer it
until a concrete consumer needs that additional surface; preserve the old loop
exactly if extraction is later authorized.

## Text, ownership and security limits

Follow [Input's borrowed-text convention](../../include/cui/cui_text_input_events.h):
when masked, `has_text=false` and empty `text`, including composition preedit;
unmasked events expose only final committed text, never preedit or old text.
Scalar byte/code-point counts remain visible as existing model metadata; masking
is presentation/redaction, not length secrecy, encryption or authentication.
No raw code/preedit logging, diagnostics, automatic telemetry or persistence.

The payload contains a borrowed `cui_str`, so copying its struct alone does not
own its text. Consumers needing retention explicitly call
[`cui_component_event_copy_utf8`](../../include/cui/cui_component_events.h#L121)
inside the callback with their own maximum byte count, capacity including NUL,
and disjoint storage. Preserve that helper's limit/capacity/UTF-8 failure result;
never silently truncate or treat copy failure as rejection of the accepted edit.
No mandatory allocation/copy, implicit fixed-size array or invented 256-byte
bound: a caller can preload arbitrarily long UTF-8 up to its buffer capacity.
Only scalar metadata/payload fields remain usable after their explicit copies;
text storage remains the caller's responsibility. No old plaintext snapshot or
preedit copy is needed to compare aggregate CHANGE.

Handler/user are borrowed synchronously, with no registration, replay or teardown
hook. Forbid mutation/rebinding/destruction of state, model, composition/session,
context or any borrowed storage during callbacks, recursive submission and foreign
exception unwinding. Unrelated application state may change. Defer model edits,
removal and teardown until outer submission and renderer consumption complete;
Styled drawing can borrow model/preedit bytes after return. No safe owner-update
promise is offered and no context-destruction guard is added.

## Focused verification and omissions

Keep [existing OTP tests](../../tests/unit/test_otp.c) unchanged: `models` covers
arbitrary/overlength UTF-8 and target validation; `editing` covers fullwidth text,
coalesced counts, delete/refill, capacity-after-partial-progress and blink;
`composition` covers masked preedit, stale commits, cancellation, focus/disabled,
rebound buffers and retained session errors. Extend with paired legacy/new Base
and Styled calls checking full result, text/state/session, semantics, focus,
text-input rectangle, geometry and masked/unmasked paint:

- Multiple edits, return-to-entry, empty Backspace and target-zero completion;
  delete overlength down to target, multiple completions with final incomplete
  model, first-digit-only multi-character text/commit, rejected modifiers/IME.
- Already-full programmatic input/no callback; nil callback with userdata;
  actual callbacks/counts and payload scalar snapshots after final semantics,
  before Styled paint. Distinguish aggregate count from accepted operation count.
- Partial accepted changes followed by capacity/UTF-8/composition errors;
  invalid preflight causes no callback/mutation. No error rollback claim.
- Disabled, inert, layout-only, omitted frames and composition-only behavior
  against legacy; copied unmasked UTF-8 retention across later model changes;
  masked redaction, copy maximum/capacity failures, overlength byte counts.
- Borrowed userdata, deferred removal/teardown, strict C11 and C++ header use;
  native execution only through the private-display wrapper. No GPU parity claim.

[Bridge OTP rendering](../../src/bridge/cui_bridge.c#L4251) currently uses the old
Styled call, maps any edit error to frame error and emits legacy `bridge_change`
when `result.changed`. It has no typed completion/count payload. Leave it and
Python/Node/Go/Rust notifications unchanged; this native proposal does not imply
binding completion support, secret transport policy or registry/schema changes.
