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

## Multi-selection requires its own bounded snapshot contract

Do not report only primary IDs as the complete selected set or reconstruct
changes from the final XOR hash. Add a separately reviewed set-observation API
with explicit caller workspace: two disjoint arrays of `cui_table_selection`,
each capacity at least the validated state selection capacity (legacy cap4096).
Preflight sizes/disjointness before any mutation; no hidden allocation or oversized
stack array. Copy before each real `apply_selection`/Escape operation and after
it, preserving primary IDs separately. Canonicalize the two scratch snapshots by
(row,column) key and compare exactly, including duplicate multiplicity; this
avoids hash-collision claims and ignores storage-order-only changes. Do not rewrite
or sort the actual caller selection array. Callback receives borrowed const
before/after spans, primary IDs, effective mode and operation metadata. Copy spans
explicitly for retention; struct copies alone are insufficient.

Observe each actual logical set/primary transition after the operation completes,
including multiple transitions per frame, not every internal range-array write.
A rejected range/capacity operation produces no event unless it actually changed
logical selection; earlier successful events remain delivered. Snapshot semantics
are independently documented and may reveal changes hidden by the old aggregate
hash, while leaving that old result untouched. This operation-level atomicity and
workspace requirement justify a separate slice from scalar selection. Do not
promise per-item deltas, full row payloads, an undo transaction or safe arbitrary
workspace mutation during callbacks.

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
