# DataTable typed transition checkpoint

Design only, audited at `d342f4d5`. Preserve the
[shared contract](COMPONENT_EVENTS_CONTRACT.md), [family inventory](COMPONENT_EVENTS_COVERAGE.md#L111)
and [binding exceptions](COMPONENT_EVENTS_BINDINGS.md#L310). Stateless composable
Table is not DataTable: child controls retain their own events. No new dispatch,
render timing, public legacy layout, sorting algorithm or bridge schema is proposed.

## Actual owners, timing and results

[Public API](../../include/cui/cui_table.h): model binds borrowed rows and mutable
columns transactionally; stable nonzero keys are unique within each dimension.
Columns move/resize in place. State owns controlled primary row/column, separate
navigation/header cursors, sort key/direction, scrolling/drag bookkeeping, and a
borrowed multi-selection workspace/count/capacity. Rows are not sorted by changing
sort state: the application owns data ordering/rebinding. Selection options are
passed by value; a requested mode does not rewrite the caller's options.

[Reducer](../../src/base/cui_table.c#L609) validates inputs, normalizes cursor and
scroll bookkeeping, processes input in order, then computes final viewport and
load-more flags. It has no existing behavior observer. Important distinctions:

- `selection_changed` is overwritten at return with entry/final primary ID and
  multi count/XOR-hash comparison. A->B->A can vanish; hash equality is not an exact
  selection transcript. `sort_changed` is set on each eligible header activation
  even if repeated cycling returns to entry. `columns_changed` ORs successful
  resize/move operations. None of these bools counts transitions.
- [Selection helper](../../src/base/cui_table.c#L386) normalizes IDs by row/column/
  cell mode. Multi selection supports replace, Ctrl/Meta toggle and Shift range,
  skipping disabled rows/disabled or nonselectable columns. Capacity checking can
  reject an operation after earlier operations succeeded. Capacity error does not
  roll back previous changes. Escape clears primary IDs, anchors and selection.
- `manual_selection` suppresses navigation-driven selection, not cursor/focus
  movement. `nonselectable` blocks selection without disabling sorting, resizing
  or interactive cell children. Preserve separate per-mode disable flags.
- [Row-header path](../../src/base/cui_table.c#L550) can apply row selection and
  also report `selection_mode_requested=ROW`; that request is not permission to
  undo the already-applied selection. `activated_row/column` retain the latest
  cell/row-header activation, including potential no-op/rejected selection.
- Secondary press sets the latest context row/column independently of selection.
  Header sort, resize and move paths coexist with focus and selection in one
  input iteration. The loop does not globally skip every consumed event; do not
  add such filtering or reinterpret child-control behavior during this work.
- [Resize/move helpers](../../src/base/cui_table.c#L179) return actual changes:
  resize clamps width and rejects unchanged values; move enforces bounds,
  fixed-column region, movability and disabled policy. Drag threshold is >4px;
  Cancel/Escape clears gesture bookkeeping without restoring prior resize widths.
  Do not invent commit/cancel rollback events from these fields.
- Post-input wheel, reveal, clamping and scrollbar passes compute final visible
  windows. First submission marks windows changed. `load_more` is edge/pending
  state near the end, reset by leaving the threshold, changed row count or turning
  the option off. Disabled/loading cancel gestures and suppress input/load-more
  while other bookkeeping remains active.

[`cui_base_data_table`](../../src/base/cui_table.c#L1170) reduces then submits
semantics for OK/CAPACITY. [Styled](../../src/components/cui_table.c#L390) resolves
theme scrollbar policy before reducing once, then paints virtual rows/columns,
headers/groups/trailing slots/placeholders and cells. Render/label callbacks are
presentation/accessibility producers, not legacy change handlers. Per-transition
observers would run inside reduction, before final semantics and every render
callback; do not claim final geometry/semantics are already refreshed. No retained
observer registration is needed. Deferred child/renderer lifetimes still apply.

## First scalar transition API

New `cui_table_events.h`; no fields added to old state/options/result. Proposed
first declarations (land sort first, single-selection next; declare only the
supported fields/functions at each reviewed production checkpoint):

```c
typedef struct {
    cui_component_event_metadata event;
    cui_table_selection previous, value;
    cui_table_selection_mode mode; /* Effective operation mode, including row header. */
} cui_table_selection_change;
typedef struct {
    cui_component_event_metadata event;
    cui_id previous_column, column;
    cui_table_sort previous, value;
} cui_table_sort_change;
typedef void (*cui_table_selection_change_fn)(void *, const cui_table_selection_change *);
typedef void (*cui_table_sort_change_fn)(void *, const cui_table_sort_change *);
typedef struct {
    cui_table_selection_change_fn on_selection;
    cui_table_sort_change_fn on_sort;
    void *user;
} cui_table_event_handler;
typedef struct {
    cui_table_result control; /* Unchanged old result/error, not callback counts. */
    bool valid, submitted, count_overflow;
    size_t selection_count, sort_count; /* Actual synchronous calls. */
} cui_table_event_result;
cui_table_event_result cui_table_reduce_events(cui_context *, cui_id,
    cui_table_model *, cui_table_state *, cui_table_options, cui_table_event_handler);
cui_table_event_result cui_base_data_table_events(cui_context *, cui_id,
    cui_table_model *, cui_table_state *, cui_table_options, cui_table_event_handler);
cui_table_event_result cui_data_table_events(cui_context *, cui_id, const cui_theme *,
    cui_table_model *, cui_table_state *, cui_table_options,
    cui_table_label_fn, cui_table_cell_fn, void *, cui_table_event_handler);
```

Use a private common reducer with the old entrypoint's unobserved path; do not
wrap final result bools. Capture scalars/metadata at each actual operation, mutate
once, then notify exact pair or sort-key/direction inequality. Emit A->B->A and
sort cycles individually; same-value selection emits no CHANGE. Preserve legacy
result flags even when their interpretation differs from typed counts. One input
can select then sort: callbacks follow those existing write points, not an invented
family ordering. Keep successful earlier notifications when later operations fail.

First selection support is explicitly **single selection only**. If a non-NULL
`on_selection` is supplied with `multiple_selection`, the new API must return
`valid=false`, `control.error=CUI_TABLE_INVALID` before reduction/paint; no silent
partial coverage. Sort-only/nil observers may still reduce multi-selection using
unchanged legacy behavior. Missing callbacks ignore userdata and preserve writes.
This restriction belongs only to the additive facade, never the old API. Root
must review it before production; full multi support is the next independent slice.

## Multi-selection revision: exact API and workspace (review required)

This is a proposal only; none of these declarations is implemented. Keep the
scalar API's `on_selection + multiple_selection` rejection. Add a distinct
set-observation family, rather than changing scalar payload interpretation:

```c
typedef struct {
    const cui_table_selection *items;
    size_t count;
} cui_table_selection_span;
typedef struct {
    cui_table_selection *previous, *value;
    size_t capacity; /* Entries in EACH array, not combined bytes. */
} cui_table_selection_workspace;
typedef struct {
    cui_component_event_metadata event;
    cui_table_selection previous_primary, value_primary;
    cui_table_selection_mode mode; /* Effective operation mode. */
    cui_table_selection_span previous, value;
} cui_table_selection_set_change;
typedef void (*cui_table_selection_set_change_fn)(void *user,
    const cui_table_selection_set_change *change);
typedef struct {
    cui_table_selection_set_change_fn on_selection;
    cui_table_sort_change_fn on_sort;
    void *user;
} cui_table_set_event_handler;
typedef enum {
    CUI_TABLE_SET_PREFLIGHT_OK,
    CUI_TABLE_SET_PREFLIGHT_INVALID,
    CUI_TABLE_SET_PREFLIGHT_WORKSPACE_CAPACITY,
    CUI_TABLE_SET_PREFLIGHT_OVERLAP
} cui_table_set_preflight;
typedef struct {
    cui_table_event_result events;
    cui_table_set_preflight preflight;
} cui_table_set_event_result;
cui_table_set_event_result cui_table_reduce_set_events(cui_context *, cui_id,
    cui_table_model *, cui_table_state *, cui_table_options,
    cui_table_selection_workspace, cui_table_set_event_handler);
cui_table_set_event_result cui_base_data_table_set_events(cui_context *, cui_id,
    cui_table_model *, cui_table_state *, cui_table_options,
    cui_table_selection_workspace, cui_table_set_event_handler);
cui_table_set_event_result cui_data_table_set_events(cui_context *, cui_id,
    const cui_theme *, cui_table_model *, cui_table_state *, cui_table_options,
    cui_table_label_fn, cui_table_cell_fn, void *,
    cui_table_selection_workspace, cui_table_set_event_handler);
```

`events.control` remains the legacy result, including aggregate flags and partial
operation errors. `events.selection_count` counts actual set callback calls,
`sort_count` actual sort calls, with existing saturation/overflow policy. Callback
order follows existing write points. No other handler fields are exposed. With a
NULL set callback, ignore workspace and userdata (unless sort uses userdata),
allow either selection mode, and preserve the sort-only/legacy path. With a
non-NULL set callback, require `multiple_selection=true`; this family does not
silently reinterpret single-selection state as a set.

### Atomic preflight and alias contract

Before **any** reducer bookkeeping, callback, semantics or Styled painting:

1. Validate existing reducer arguments and multi count/capacity invariants without
   normalization/writes. Require nonzero root ID and valid mode/sort fields, as in
   the scalar facade. Non-NULL set callback with single selection is INVALID.
2. For set observation, require `state.selection_capacity <= workspace.capacity
   <= 4096`. Both scratch arrays contain `workspace.capacity` writable entries.
   A zero capacity permits NULL arrays; a nonzero capacity requires both pointers,
   suitable alignment and overflow-safe byte extents. Count must not exceed state
   capacity; the state selection pointer is required when that capacity is nonzero.
   Scratch shortage reports WORKSPACE_CAPACITY; invalid pointer/alignment/extent,
   impossible count/capacity or oversized declared capacity reports INVALID.
3. Reject overlap between the **full declared** scratch byte extents, and between
   either scratch extent and the state object, state selection's full capacity,
   opaque model object, bound row descriptors or bound mutable column descriptors.
   These known ranges are available to the private Base implementation. Empty
   ranges do not overlap. Reject unrepresentable extent/address endpoints before
   arithmetic; compare checked integer address intervals on supported targets
   providing `uintptr_t`, never order unrelated C pointers or subtract IDs.
   Any detected overlap reports OVERLAP. Do not accept two partially overlapping
   views just because their currently used counts are small.

For multiple simultaneous errors, use the above order, with argument/extent
INVALID checks preceding capacity shortage and overlap checks. A failure returns
`events.valid=false`, `events.submitted=false`, zero counts/overflow,
`events.control.error=CUI_TABLE_INVALID`, and the separate precise preflight code.
State, columns, both scratch arrays and input consumption remain byte-for-byte
unchanged. This intentionally distinguishes facade workspace failure from a
later **legacy selection operation** returning CUI_TABLE_CAPACITY. Once preflight
succeeds, `preflight=OK` even if a subsequent operation fails; preserve earlier
mutations/notifications and legacy Base semantics-on-OK/CAPACITY policy.

Scratch is caller-owned exclusive storage for the complete call, including
callbacks and render consumption. It must also not alias context/internal model
allocations, label/group/string bytes, render or handler userdata, or other live
resources. Those arbitrary/opaque allocations cannot all be introspected: their
nonaliasing and actual allocation size/readability are explicit caller
preconditions, **not** claimed runtime checks. Passing a dangling pointer is not
made safe by interval checking. Workspace is passed by value, so no retained
workspace descriptor or descriptor lifetime/alias rule is needed. The proposal
must not claim universal alias validation from the enumerated checks.

### Exact operation snapshots without allocation

Capture primary IDs and copy the current selection entries to `previous` before
each actual selection helper/Escape operation. Run the original operation once,
then capture primary IDs and copy entries to `value`, even on a partial error.
Do not snapshot merely cursor movement or every internal range-array write.
Canonicalize each copied span by unsigned lexicographic `(row, column)` order
using an iterative in-place binary heapsort: bottom-up heap construction, then
max-root extraction/sift-down. Use bounded indices (`root < count/2` before
forming children), direct ID comparisons, no subtraction comparator, recursion,
`qsort`, variable-size stack array, allocator or sorting of the owner's array.
Maximum scratch is exactly `2 * workspace.capacity * sizeof(cui_table_selection)`
bytes, supplied by the caller; extra algorithm storage is O(1).

Compare counts, primary pairs and sorted entries exactly. Duplicate multiplicity
is retained, not deduplicated or repaired: caller-populated duplicates remain
truthfully visible under legacy behavior. Storage-order-only changes produce no
CHANGE; same set with changed primary does. Deliver one callback per operation
with a real multiset/primary difference, including A->B->A in one frame. An
accepted no-op or rejected range with no change emits none. Operation failure
after actual change must notify that observed change without changing its error.
Effective row-header mode and genuine input metadata follow scalar conventions.

For a logical operation with n before/m after entries, observation costs
O(n log n + m log m) time and O(n+m) copies/comparison, bounded by 4096 entries
per snapshot. The existing reducer's range-generation/model lookup cost is
unchanged; this is not a claim that the whole reducer is O(n log n). No quadratic
set-membership scan is introduced. Scratch reuse avoids per-input allocation.
Payload spans are sorted, borrowed and readable **only during the callback**;
copy their entries explicitly for retention. Copying the payload struct alone
retains dangling/overwritten views. Handlers cannot mutate owner/workspace/model
or recurse; no per-item delta, rollback, safe owner mutation or undo promise.

### Required independent production evidence

Test all three facades against unchanged legacy results/owner/paint; set+primary
changes, same-set order permutations, duplicate multiplicity, exact UINT64 keys,
Ctrl toggles, Shift ranges, row/column/cell and row-header mode, Escape, repeated
net-zero input, no-ops and capacity errors after progress. Freeze callback spans
into test-owned storage and verify subsequent scratch reuse. Exercise both
identical and partially overlapping buffers, aliases into state/selection/rows/
columns/model, zero/max capacity, short scratch, count overflow, misalignment and
checked extent overflow; failed preflight must preserve state/columns/scratch and
input consumption. Do not dereference fabricated invalid pointers in such tests.
Pair NULL observer behavior and passive/disabled/loading cases with legacy paths.
Test sorted/reverse/equal-key maximum-size heapsort cases, comparator edge IDs,
and instrument comparison counts with a generous O(n log n) bound; do not use
wall-clock thresholds. Public C11/C++ header checks and private-wrapper CTest
remain required. No multi production work is approved by this revision itself.

## Subsequent independent slices and aggregate exceptions

1. Column CHANGE: distinct pointer-free `{column, before_width, after_width}` and
   `{column, before_index, after_index}` payloads at actual successful resize/move
   calls, including pointer moves, keyboard resize, Ctrl reorder and drag drop.
   Indices are operation-time positions; stable column key remains authoritative.
   No begin/commit/cancel gesture vocabulary until a separate lifecycle audit.
2. Repeated cell/row-header ACTIVATE, context request and mode request: capture at
   their actual branches, including same-item activation and several context
   requests per frame currently collapsed in results. Keep activation separate
   from selection success. Mode REQUEST concerns caller-owned options; ignoring
   it does not reject/roll back an already-applied row selection. Context REQUEST
   asks the application to decide popup behavior. No callback return/rejection hook.
3. Viewport/load-more: justified aggregate completion observations, once after
   reveal/clamp/scrollbar calculations. Emit a typed final rows/columns view with
   initialization/old window-index facts; do not invent an unavailable old full
   view or causal wheel event. Load-more observes the actual pending edge, not
   dataset completion or a network instruction. No synchronous fetch in Core.

Only active ordinary enabled/nonloading submissions notify input transitions;
passive/layout-only/disabled/loading preserve old normalization/bookkeeping.
Aggregate viewport initialization/load policy needs explicit passive tests before
activation; no blanket no-mutation promise. Use shared CHANGE/ACTIVATE/REQUEST
only where the above meaning applies. Input metadata has root table target and
actual frame/index/source when known; HAS_ACTIVATION only when a real snapshot
exists. Semantics are submitted after reduction, so never fabricate it. Aggregate
completion metadata is UNKNOWN/HAS_FRAME only. No bubbling or new input routing.

## Lifetimes, regression evidence and bridge omissions

Callbacks/user borrow synchronously. Stable IDs/scalars are pointer-free copies;
model rows, mutable column descriptors, labels, state, selection buffers, group
headers/render userdata and any new snapshot workspace keep their existing
submission/render lifetimes. Cell label callback must return readable stable
borrowed bytes; render callbacks still run only for the virtual window. Typed
observers may copy payloads or update unrelated application state, but must not
rebind/sort/resize/move model storage, mutate state/workspace, recurse, destroy
context/resources or unwind foreign exceptions. Defer edits/removal/teardown
until submission and renderer consumption complete. No new retention mechanism.

Keep [table tests](../../tests/unit/test_table.c) and
[selection contracts](../../tests/unit/test_table_selection_contracts.c) unchanged:
virtual/load pending, sort cycles, nonselectable child buttons, manual navigation,
row/column/cell mode gates, resize/move/fixed columns, context and row-header mode
requests are existing obligations. New per-slice tests should use real queued
pointer/key/supported accessibility input, repeated/net-zero transitions, no-ops,
capacity after progress, exact copied IDs, optional handlers, frozen metadata,
callback-before-render traces, disabled/loading/passive behavior, and paired old/
new owner/result/semantic/geometry/paint comparisons. Add exact multi set snapshots,
range/capacity/duplicate-entry policy and workspace bounds/disjointness tests before
that API exists. Strict C11/C++ and private-display wrapper checks follow production;
this design checkpoint runs no builds and makes no accessibility parity claim.

[Bridge rendering](../../src/bridge/cui_bridge.c#L3628) still calls the old Styled
API, emits at most aggregate selection and sort notifications, and returns on
any table error before queuing changes. [V1 table payloads](../../src/bridge/cui_bridge.c#L2839)
already preserve uint32 row/column/sort fields, alongside copied legacy JSON;
they do not expose native per-transition multiplicity, cui_id-width keys, complete
multi sets, column movement/resize, activation/context/mode requests or load edges.
Python/Node/Go/Rust manual polling remains a separate post-step queue contract.
Do not replace it or claim native observer parity from these new C declarations.

## Implementation checkpoints

Sort now has a private common reducer with optional typed observation and additive
reduce/Base/Styled facades. Old paths call the reducer without an observer and
retain legacy flags/consumed-event handling. Only implemented sort handler/count
fields were public at the sort checkpoint. Single selection now adds an optional
primary-pair handler at actual selection operations and Escape, preserving their
order relative to sort and later result/semantic work. Non-NULL selection handler
with multiple_selection is rejected before reduction/paint; nil/sort-only handlers
keep old multi behavior. Multi-set/workspace APIs remain unimplemented and require
the separate exact-interface/algorithm review described above.
