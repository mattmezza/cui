# Proposed text-input event slice

Status: audit/proposal only, at `32258fe5`. None of the declarations below are
shipped. Read the [event contract](COMPONENT_EVENTS_CONTRACT.md) and
[binding audit](COMPONENT_EVENTS_BINDINGS.md) with this proposal. Keep existing
model ownership and reducer order; do not make every text component expose every
event kind merely for symmetry.

## Actual mutation and evidence seams

| Control/path | Actual behavior | Safe additive seam |
| --- | --- | --- |
| [Input](../../src/base/cui_base.c): `cui_base_input_ex` / `cui_base_input_reduce_event` | Caller-owned `cui_edit` mutates synchronously. The outer function iterates queue indices in order and aggregates `changed/submitted/rejected/error`. Single-event reduction already exposes exact before/after revision and Enter result; Questionnaire uses it for ordered text/confirmation. | Optional observer inside the common single-event path, after the existing operation/presentation completion, before the next queue event. Freeze genuine index/source before calling it. Do not wrap only the aggregate bool and invent multiplicity. |
| [Checked edit engine](../../src/base/cui_edit.c): `cui_edit_apply_checked` | Whole candidate is normalized, validated/formatted/presented and applied transactionally using bounded workspaces. Validation can reject while preserving document/selection/history, but empty/old-invalid healing, Undo/Redo and composition rules are specific. | Preserve this gate unchanged. A general proposed-value REQUEST is not equivalent to the predicate. |
| [Textarea](../../src/base/cui_textarea.c) | Ordered event loop mutates model and selection, handles IME/clipboard/indentation, then reports aggregate revision/selection/scroll changes. `pressed_enter` and modifiers preserve only the last Enter, including newline insertion; `submit_on_enter` suppresses ordinary newline but Shift still inserts it. | First expose one post-invocation CHANGE from entry/final revisions. Genuine per-event notifications require factoring the existing event loop, not replaying the aggregate result. |
| [Editor](../../src/base/cui_editor.c): `cui_base_editor` | Completion events run first, multicursor events next, then Base Textarea; final results merge revisions/selections/errors. Search/completion/public model methods also mutate outside this submission. Highlight callbacks are paint services, not change handlers. | One post-invocation CHANGE preserves the current multipass ordering. Do not claim its final revision came from one particular event/index or that a wrapper can reconstruct all intermediate edits. |

[Public edit contracts](../../include/cui/cui_edit.h) distinguish logical document,
provisional composition view, selection/history and presentation staging.
Input's `validate` is a whole-value predicate, not an `on_change` callback;
`presentation_complete` runs after checked application and its external side
effects are outside rollback. Current [Input result](../../include/cui/cui_base.h)
contains no `on_submit`: its `submitted` bool reports eligible nonrepeated
single-line Enter. [Textarea result](../../include/cui/cui_textarea.h) and
[Editor options](../../include/cui/cui_editor.h) do not provide a universal edit
commit/submit handler. Do not describe existing result polling as callback dispatch.

## Event kinds: recommended bounded scope

| Kind | First slice | Explicit exclusion |
| --- | --- | --- |
| REQUEST | Keep existing Input acceptance API; no new generic REQUEST callback yet. | `validate` can be called for candidate and previous value, never for empty values, and is bypassed for multiline/Undo/Redo/START/CANCEL. A false predicate only rejects when the old visible value passes. It cannot promise one rejectable request per edit. |
| CHANGE | Input: per admitted event whose committed revision changed. Textarea/Editor: at most one aggregate notification per invocation. | Post-mutation notifications cannot veto or undo the edit. Selection/preedit-only changes and failed/rejected edits are not document CHANGE. Programmatic edits outside the facade are not retrospectively replayed. |
| COMMIT | Defer. | An IME host COMMIT can be rejected or commit unchanged text; blur, undo grouping, ordinary typing and form submission are different boundaries. Do not manufacture a universal “editing finished” event. A later explicitly named IME lifecycle API may report attempted/applied commit separately. |
| SUBMIT | Input: per existing eligible single-line Enter result, after processing that event. | Not model validation success or form submission. Textarea/Editor last-Enter aggregates cannot support exact per-event submit callbacks without another reviewed reducer slice. Newlines/Shift-Enter are not automatically SUBMIT. |

This first delivery is intentionally notifications plus existing validation,
not a controlled immutable-string editor. A general rejectable REQUEST for
Textarea/multicursor/undo requires an engine transaction design: complete candidate
and selection/history ownership, rejection policy, preedit lifetime and bounded
storage. Obtain an explicit architectural decision before implementing it. Never
mutate then label the notification rejectable, or implement rejection by ad hoc
undo (which changes history and callbacks).

## Proposed minimal declarations

New header `cui_text_input_events.h` can depend on Base/shared metadata/Editor
public headers; no Core dependency on components. Names/types are proposals:

```c
typedef struct {
    cui_component_event_metadata event; /* CHANGE or SUBMIT */
    cui_edit_state before, after;       /* Scalar snapshots, UTF-8 byte offsets */
    bool has_text;
    cui_str text;                       /* Current committed text, borrowed */
} cui_component_text_event;
typedef void (*cui_component_text_fn)(void *, const cui_component_text_event *);
typedef struct { cui_component_text_fn callback; void *user; }
    cui_component_text_handler;
typedef struct {
    cui_input_result control;           /* Preserve old aggregate fields */
    size_t change_count, submit_count;  /* Actual observer calls */
    bool count_overflow;
} cui_input_event_result;
typedef struct {
    cui_textarea_result control;
    size_t change_count;                /* 0 or 1, invocation aggregate */
} cui_multiline_event_result;

cui_input_event_result cui_base_input_events(
    cui_context *, cui_id, cui_str, cui_edit *, cui_input_behavior_state *,
    cui_base_input_options, cui_component_text_handler);
cui_input_event_result cui_base_input_reduce_event_events(
    cui_context *, cui_id, cui_edit *, cui_input_behavior_state *,
    cui_base_input_options, size_t event_index, cui_component_text_handler);
cui_multiline_event_result cui_base_textarea_events(
    cui_context *, cui_id, cui_str, cui_edit *, cui_textarea_state *,
    cui_textarea_options, cui_textarea_metrics, cui_component_text_handler);
cui_multiline_event_result cui_base_editor_events(
    cui_context *, cui_id, cui_str, cui_editor_state *, cui_editor_options,
    cui_textarea_metrics, cui_component_text_handler);
```

Keep old signatures/options layouts/results unchanged; old entry points pass NULL
observers to internal common implementations. A missing observer changes no edit
behavior and is **not** read-only (unlike controlled Checkbox/Slider requests).
Styled Input requires a corresponding `cui_input_events(..., cui_input_state *,
cui_input_options, handler)` composition adapter to avoid bypassing secure
presentation/mask resources; its declaration belongs beside `cui_input_ex` in the
component header. Styled Textarea/Editor adapters can follow once Base contracts
are proven. Do not promise them from this proposed header alone.

Counts are passive, per invocation, saturating with overflow for Input. Calling
both old and new reducers for one event would double-edit; a composite such as
Questionnaire must select exactly one reduction path and keep its existing
answer synchronization/confirmation ordering. Do not export a second whole-frame
pass that replays Input events after Questionnaire consumed them.

## Ordering, lifetimes and callback mutation

Input notification order: existing checked operation, existing presentation
completion, final before/after comparison, CHANGE if revision changed, SUBMIT if
the old reducer reports Enter, then next queue event. Preserve IME rejection cleanup
before observation: host COMMIT ends its session even if document acceptance
rejected it. Preedit UPDATE normally changes only provisional view, not revision;
it must not become a document CHANGE. START/CANCEL produce no document notification
unless the actual operation changes revision. Preserve current guards for consumed,
disabled, readonly, repeat, composition and preferred-character input; add none
that silently changes legacy Enter behavior.

Input uses real event indices and known keyboard/text/pointer origins; never claim
activation evidence for text/composition. Unknown composition/host origin stays
UNKNOWN. Textarea/Editor aggregate CHANGE has current frame identity only, no
invented single input index/source/activation. Lifecycle changes from focus/rebind
without an input index cannot borrow another event's metadata.

Payload before/after are scalar snapshots, not complete multicursor selections or
old text. `text` borrows the model's committed buffer through the callback only;
masked/password Input sets `has_text=false` and returns an empty span. No implicit
plaintext copying of secure fields or borrowed clipboard/preedit payload is allowed.
Copying the struct alone does not retain text: use the shared bounded UTF-8 copy
helper with caller storage, or omit it. Deferred copies need explicit capacity and
failure handling. Do not allocate document-sized snapshots inside every callback.
Full selection sets, deltas and old-text snapshots are later explicit APIs.

Callbacks may update independent app state or request later omission/teardown.
They must not mutate/destroy/rebind the active edit model, edit caller buffers,
state/workspaces, or recursively submit. Apply requested programmatic edits after
frame/renderer completion and resubmit. The engine's acceptance guard protects
same-model mutators *inside its predicate*; it does not automatically protect a
new post-reducer observer. First-slice restrictions must be documented as
preconditions unless a separate engine guard is approved. Do not claim automatic
reentrancy/destruction rejection from a wrapper.

## Acceptance and delivery order

1. Input Base per-event observer with no Styled change: compare old/new model,
   selection, history, errors, consumption and phase timing. Retain
   [acceptance](../../tests/unit/test_input_acceptance.c),
   [normalization](../../tests/unit/test_input_normalization.c),
   [formatting](../../tests/unit/test_input_format_control.c),
   [password](../../tests/unit/test_password_input.c) and
   [Questionnaire ordered batches](../../tests/unit/test_questionnaire_styled.c).
   Add TEXT/Enter and Enter/TEXT batches, repeated Enter, rejected/no-op edits,
   Undo/Redo, clipboard failures, preedit UPDATE versus accepted/rejected COMMIT,
   consumed events, readonly/disabled, exact metadata/multiplicity and no result replay.
2. Styled Input adapter: prove identical mask/presentation completion timing,
   no plaintext leakage, metadata and independent app mutation/deferred teardown;
   real headless browser/SDL composition supplements deterministic engine tests.
3. Aggregate Textarea/Editor CHANGE wrappers: preserve
   [Textarea](../../tests/unit/test_textarea.c) and
   [Editor](../../tests/unit/test_editor.c), including multicursor, completion,
   indentation, selections, history, errors and IME. Explicitly test several
   revisions aggregate to one CHANGE and selection-only changes do not notify.
   Per-event multiline submission/REQUEST/COMMIT is a later approved slice.

[The bridge](../../src/bridge/cui_bridge.c) currently checks aggregate `changed`
and queues post-mutation CHANGE for Input/Textarea/Editor; it does not forward
Input `submitted` or per-event index/selection/IME evidence. Python/Node/Go/Rust
callbacks are later queue consumers and cannot reject those edits. This proposal
does not ship a binding facade, alter queued timing or expose candidate validation
into foreign callbacks. Any later binding work needs capability/versioned typed
payloads, bounded copied text/privacy and language exception/ownership tests.
Native verification uses the private-display wrapper; browser tests stay headless.
No production, generated files or runtime tests accompany this audit.
