# Questionnaire copy-out snapshots contract

Design checkpoint only. No declarations or implementation in this document are
available APIs. Base is CUI `206ba95c`; frozen source authority is commit
`87d10ae5e1299d1be18670a37570c5402f952e34` in the local reference checkout.
This slice provides immutable caller-owned copies and scalar queries. Event
observers, callback mutation rules and changes to legacy reducers are separate.

## Source and existing API reconciliation

Frozen `crates/base/src/questionnaire/types.rs:254–324` defines copied choices
and optional freeform, excluding editor drafts, plus schema-ordered named answers.
`types.rs:454–500` defines navigation visibility and confirmability. Answer-change
payloads carry copied item name, answer and status (`535–566`); submission items
carry the same three fields (`568–621`). Current-change events copy optional
previous/current names (`623–632`).

Frozen `state.rs:180–191,254–284` provides enabled progress, navigation, individual
answers and all enabled answers. `state.rs:1088–1143` constructs submissions and
filters effective answers: disabled items return empty answers; disabled choices
and disabled input are excluded; choice order is definition order. Status can
remain Skipped even when an item is disabled. `state.rs:746–767` validates enabled
items, then constructs one submission before Completed(clone) and Submit.

Current `cui_questionnaire.h` and Base implementation provide find, total,
position, choice_selected, freeform, item_status and item_error. Freeform is a
borrowed view into mutable workspace. There is no copied answer/submission or
aggregate navigation query. Public structures expose schema/current state, but
reading them after an edit does not preserve an earlier answer. Existing tests
cover model correctness, capacities and event order; they do not establish a
retained copied submission. Keyboard policy APIs do not fill this gap.

The proposed copy-out submission is callable before completion. Like source's
answer collection, it reports current effective data. It does not validate,
submit, mark complete, flush an editor, tick a timer or invoke a callback. Its
name does not certify that the data is acceptable for transport.

## Proposed portable C11 surface

New header `cui_questionnaire_snapshots.h` includes the legacy model header only.
Names below are proposed exact declarations for review:

```c
typedef enum {
    CUI_QUESTIONNAIRE_SNAPSHOT_OK,
    CUI_QUESTIONNAIRE_SNAPSHOT_INVALID,
    CUI_QUESTIONNAIRE_SNAPSHOT_UNKNOWN_ITEM,
    CUI_QUESTIONNAIRE_SNAPSHOT_CAPACITY,
    CUI_QUESTIONNAIRE_SNAPSHOT_OVERFLOW
} cui_questionnaire_snapshot_result;

typedef struct {
    const cui_str *choices;          /* copied values in choice-definition order */
    size_t choice_count;
    bool has_freeform;
    cui_str freeform;                 /* copied effective answer, not editor draft */
} cui_questionnaire_answer_snapshot;

typedef struct {
    size_t item;                      /* original schema index, not enabled index */
    cui_str name;                     /* copied identity, not presentation label */
    cui_questionnaire_status status;
    bool disabled;                    /* useful for an individual disabled item */
    cui_questionnaire_answer_snapshot answer;
} cui_questionnaire_item_snapshot;

typedef struct {
    const cui_questionnaire_item_snapshot *items;
    size_t count;                     /* enabled items in schema order */
} cui_questionnaire_submission_snapshot;

typedef struct {
    size_t items, choices, text_bytes; /* bytes INCLUDE all appended terminators */
} cui_questionnaire_snapshot_requirements;

typedef struct {
    cui_questionnaire_item_snapshot *items;
    size_t item_capacity;
    cui_str *choices;
    size_t choice_capacity;
    char *text;
    size_t text_capacity;
} cui_questionnaire_snapshot_workspace;

typedef struct {
    size_t current_item;              /* schema index, SIZE_MAX when none */
    size_t position;                  /* enabled zero-based index, SIZE_MAX if none */
    size_t current, total;            /* one-based progress (or 0), enabled total */
    bool previous_visible, next_visible, skip_visible, submit_visible;
    bool confirmable, complete;
} cui_questionnaire_query_snapshot;

cui_questionnaire_snapshot_result cui_questionnaire_answer_measure(
    const cui_questionnaire_state *, size_t item,
    cui_questionnaire_snapshot_requirements *required);
cui_questionnaire_snapshot_result cui_questionnaire_answer_copy(
    const cui_questionnaire_state *, size_t item,
    cui_questionnaire_snapshot_workspace,
    cui_questionnaire_snapshot_requirements *required,
    cui_questionnaire_item_snapshot *out);
cui_questionnaire_snapshot_result cui_questionnaire_submission_measure(
    const cui_questionnaire_state *,
    cui_questionnaire_snapshot_requirements *required);
cui_questionnaire_snapshot_result cui_questionnaire_submission_copy(
    const cui_questionnaire_state *, cui_questionnaire_snapshot_workspace,
    cui_questionnaire_snapshot_requirements *required,
    cui_questionnaire_submission_snapshot *out);
cui_questionnaire_snapshot_result cui_questionnaire_query_copy(
    const cui_questionnaire_state *, cui_questionnaire_query_snapshot *out);
```

An answer copy requires one workspace item record. `out` receives a by-value
copy of that record; nested pointers refer only to caller snapshot workspace.
Submission output refers to the first `required.items` workspace records. Both
operations use the same representation so a submission's answers project directly
to source's named-answer collection; no duplicate all-answers serialization API
is necessary. Status/disabled/index are additional portable query information.

## Exact data rules

- Individual lookup uses the existing schema index; invalid state is INVALID and
  an out-of-range index is UNKNOWN_ITEM. A disabled existing item still produces
  its copied name/status, `disabled=true`, and an empty effective answer. Disabled
  items are omitted entirely from a submission. Empty schemas yield a successful
  zero-item submission and zero requirements. Empty submissions use items=NULL;
  zero-choice answers use choices=NULL even if spare workspace was supplied.
- All enabled items appear, including Unanswered and Skipped, in original schema
  order. This is not an answered-only collection. Choices contain copied values,
  not labels or indices; retain original definition order, deduplicated by the
  existing retained model, excluding disabled/unselected choices.
- Freeform uses the effective retained answer. Unsynchronized editor drafts,
  initial-answer/reset buffers and disabled input are excluded. Existing model
  blank normalization determines absence; copying does not trim or normalize
  bytes further. `has_freeform=false` implies `{NULL,0}` and consumes no text
  bytes. Present freeform preserves leading/trailing whitespace around nonblank
  content and exact UTF-8 bytes.
- Skipped means copied status Skipped and the effective answer given by the
  existing model; ordinary skip clears it. Copy-out must not independently invent
  a second status reducer. Reenabled retained choices follow existing live policy.
- Each included name, choice value and present freeform is independently copied
  followed by one NUL. `cui_str.length` excludes that terminator. Requirements'
  `text_bytes` includes it, including one byte for an empty name/value. No string
  deduplication, interning or shared empty-string sentinel. Pack each item name,
  then its selected choice values, then present freeform; process items in schema
  order, without alignment padding in the text arena. Existing accepted
  embedded NUL bytes are preserved, not rejected by a new snapshot-only rule;
  consumers must use lengths rather than assume arbitrary values are C strings.
- Labels, descriptions, validator messages, editor objects and callbacks are not
  copied. No platform/Clay types, heap allocation, global buffers or renderer
  dependencies. Definitions and model workspace must remain valid during each
  operation; copied names/values no longer depend on them after success.

## Measurement, bounds and transactional failure

`required` is mandatory for measure and copy; `out` is mandatory for copy/query.
Null required output pointers return INVALID without dereferencing them. Other
provided output records are reset to their documented empty state on failure:
requirements all zero, item/submission all zero; query has current_item/position
SIZE_MAX and all remaining fields zero. CAPACITY is the exception for requirements:
it publishes exact requirements, but leaves snapshot output empty.

Measure is a pure full scan with checked additions for item/choice counts and
`length+1` per string; record-array byte multiplication is checked too. Any
unrepresentable count, byte extent or total returns OVERFLOW with zero requirements.
A bounded fake-length test must reach the checked arithmetic without reading the
fictitious bytes; this does not promise safety for arbitrary invalid C pointers.
There is no saturating “successful” requirement or truncated copy.

A workspace pointer may be NULL exactly when its capacity is zero. Nonzero
capacity with NULL pointer is INVALID. Capacity zero with non-NULL pointer is
valid but still has zero writable elements. The complete measured requirements
must fit all three capacities before the first workspace write. Insufficient
capacity returns CAPACITY, no partial records/bytes, no truncation and no NUL
write even if text capacity is positive. Thus a failed copy preserves a previously
published snapshot in those buffers; the new `out` is empty and cannot appear
partially valid. This differs intentionally from a truncating text formatter.

The API performs all fallible checks before committing writes. Once sufficient
capacity and valid source are established, filling must not invoke fallible
callbacks or allocate. State/schema/editor mutation during measure or copy is
forbidden; no concurrency, generation lock or mutation detector is implied.
Separate measure and copy may observe different states: copy always remeasures
and reports its own requirements. Never trust a stale measured count to write.

Snapshot workspace arrays/text, required and output records must be mutually
disjoint and disjoint from the model, schema, editor data and model workspace.
These are explicit caller preconditions, consistent with the legacy workspace;
this slice does not promise arbitrary overlap detection or pointer provenance
validation. In particular `out` must not be `workspace.items`. Reusing the same
workspace for a successful copy invalidates its previous snapshot; reusing it for
a failed copy does not. Caller copying a record alone does not clone its buffers.

Existing model limits remain: at most 4096 items, answer-byte capacity defaults to
4096 and is bounded at 1 MiB per input. Choices/schema string bytes are bounded by
actual caller declarations/workspaces, not an invented new fixed snapshot cap.
Requirements are size_t and checked against SIZE_MAX on every accumulation and
record extent. Copy iterates the validated initialized schema; forged retained
structures remain outside the model contract. No dependency installation needed.

## Scalar query semantics

Query does not need workspace and runs no validators. Enabled total and position
match existing accessors for valid model states. No current enabled item gives
SIZE_MAX item/position, progress current 0, all visibility/confirmable flags false;
complete still reflects the retained complete flag. With a current enabled item:
previous is position>0, next is position+1<total, skip is !required, submit is
position+1==total, confirmable is status!=Unanswered. Progress current is
position+1. These mirror source `navigation_state`, not custom UI options such as
an ancestor disabled flag or whether the host omitted an action button.

Errors do not make confirmable false merely because validation failed: source
confirmability means answered/skipped, not valid. Query neither clears pending
confirmation nor repairs externally forged current/disabled state. Treat a
current index not identifying an enabled item as no current item for this query.

## Alternatives and future event compatibility

A borrowed answer view is cheaper but cannot survive model edits or definition
teardown; existing accessors already serve that use. A heap-owned clone would
complicate allocator/ABI/binding ownership. An offset-only arena is relocatable
but introduces accessor/validation machinery for this small C11 surface. Proposed
pointer records use ordinary cui_str and explicit caller lifetimes; callers may
serialize or clone them using another measured copy, but cannot memcpy-relocate
an arena and expect pointers to rebase automatically.

A later typed observer may reuse these records to freeze answer-change and
Completed/Submit payloads. That needs a separate policy for event workspace
capacity, legacy callback timing, live per-event eligibility, reentrant mutation,
previous/current name copies and paired payload retention. Copying inside each
legacy callback independently cannot reproduce source's single frozen
Completed/Submit pair if the first callback changes state. This contract makes
no observer facade, event-delivery, callback-mutation or thread-safety claim.

Bindings will need bounded output buffer marshalling, optional-freeform distinction,
size_t conversion and explicit ownership. Shipping a header in SDK source does
not expose these records through the existing generic bridge. Legacy callbacks,
validators and submission/navigation behavior are unchanged by this slice.

## Proposed verification and ownership

Approve a new header, Base-only copy/query helpers, dedicated tests, worker fixture
and compiling guide as the first implementation. No Styled/core/bridge change is
needed. Canonical registration, standalone/amalgamation/header/C++ checks,
WASI list, SDK/dist/source metadata and generated skill are later integration.
Do not promote the whole Questionnaire family from this snapshot slice.

Tests must use real initialized models: empty schema; all statuses; mixed input;
disabled item/choice/input; schema-order and Unicode/empty/embedded-NUL values
where legacy accepts them; duplicate selection normalization; exact required bytes;
each one-short capacity with poisoned unchanged buffers; zero/NULL arguments;
overflow arithmetic; stale measurement followed by a larger answer; and retained
snapshot bytes/pointers after answer edits, reset, disable, editor/schema teardown.
Verify query against source navigation vectors including required-last, skipped,
invalid-but-answered and all-disabled. Assert callback silence and no model/editor/
pending/completion mutation. Failure preserves a previous successful snapshot.

Run strict portable C11 Release via the private invisible-display wrapper,
focused WASI tests, compiled C usage and C++ header; pinned format.py --check
runs after final edits. Software/browser pixels are unnecessary for pure copy-out;
actual WASM execution is relevant. A later browser consumer can prove bridge
marshalling only after that separate binding API exists. No desktop windows,
network, dependency installs or physical-AT claims belong in this checkpoint.
