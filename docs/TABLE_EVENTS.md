# DataTable transitions

Include `cui/cui_table_events.h`. The additive `cui_table_reduce_events`,
`cui_base_data_table_events` and `cui_data_table_events` take a `cui_table_event_handler`
and return unchanged legacy `control`, validity/submission flags and actual
`selection_count`, `sort_count`, `column_width_count` and `column_move_count`
(saturating with `count_overflow`). A missing callback ignores user and preserves
reduction, including multi-selection behavior.

Each applied sort key/direction transition notifies inside reduction, before
final semantics and every Styled render callback. Three cycles can return to the
entry state while producing three typed events and preserving the legacy
`sort_changed` flag. Both legacy ascending-first and source descending-first
cycles remain unchanged. This changes sort state only; caller rows are not sorted.
Earlier successful notifications remain delivered if later input fails.

The pointer-free payload preserves exact native `cui_id` keys and direction
values. Shared CHANGE metadata has the table root target and genuine current
frame/index/source; HAS_ACTIVATION is present only when a real snapshot exists.
Do not assume final semantics, geometry or paint are available during callbacks.
Disabled/loading/inert/layout-only paths preserve legacy bookkeeping without
sort callbacks. No new consumed-event filtering is applied.

Handler/user are borrowed synchronously. Copy payloads/change unrelated application
state, but do not mutate/rebind the model, rows, columns, state or workspace;
recurse; destroy storage/context; or unwind foreign exceptions during callbacks.
Defer model edits/removal/teardown until outer submission and renderer consumption
finish. Existing render/label callback arguments and virtual-window timing remain
unchanged. New facades require a nonzero root ID; legacy APIs remain unchanged.

Single selection emits `cui_table_selection_change` for each actual primary
row/column pair transition, with the effective operation mode. Navigation in
manual mode does not select; activation may still select. Same-pair activation
emits no CHANGE, while A->B->A emits twice even though the legacy final
`selection_changed` is false. Escape clearing is observed. Row-header operations
can apply ROW selection while also producing the old mode-request result; the
callback precedes later result bookkeeping and does not reject that selection.
A column activation can emit selection then sort in the existing write order.
Selection-disabled/nonselectable policies do not disable independent sorting.

A non-NULL `on_selection` with `multiple_selection=true` returns `valid=false`,
`control.error=CUI_TABLE_INVALID` before reduction, state/column mutation or paint.
There is no silent primary-only multi observation. Nil selection/sort-only handlers
still support legacy multi reduction, including accepted progress before errors.
The separate set-observation API below supports complete multi selection. Column
movement/resize are covered below; activation, requests and viewport/load events
remain separate slices. Existing bridge V1 table notifications remain aggregate post-step snapshots; native multiplicity and
64-bit keys here do not imply binding parity.


## Multi-selection snapshots

Use `cui_table_reduce_set_events`, `cui_base_data_table_set_events` or
`cui_data_table_set_events` with `cui_table_set_event_handler` and
`cui_table_selection_workspace`. The handler supports selection, sort, column-width and column-move callbacks;
its selection callback receives `cui_table_selection_set_change`, not the scalar
payload. Its return is `cui_table_set_event_result`: `events` contains the same
legacy result and actual callback counts as above, and `preflight` distinguishes
workspace/argument failure from a later legacy capacity error. This requires
`multiple_selection=true` when `on_selection` is non-NULL. Scalar facades keep
their existing explicit multi-observer rejection.

```c
cui_table_selection before[32], after[32]; /* Each >= state.selection_capacity. */
cui_table_selection_workspace scratch = {before, after, 32};
cui_table_set_event_handler handler = {
    .on_selection = on_set_change, .on_sort = on_sort_change, .user = application
};
cui_table_set_event_result result = cui_base_data_table_set_events(
    ui, table_id, model, &state, options, scratch, handler);
/* Keep scratch and all owner resources alive until this call completes. */
```

Each actual selection operation snapshots the multiset and primary pair before
and after the old mutation. Payload `previous`/`value` spans are sorted by exact
unsigned `(row,column)` keys; duplicates retain multiplicity. Storage-order-only
changes do not notify; primary-only changes do. Ctrl toggles, range replacement,
row-header mode and Escape follow their existing policies. Repeated/net-zero
transitions notify separately; rejected operations with no change do not. Earlier
notifications survive a later capacity error, while old flags/errors remain
unchanged. Callbacks run before final semantics and Styled rendering. No per-item
delta or transaction rollback is provided.

Both scratch arrays have `capacity` writable entries; capacity must be at least
`state.selection_capacity` and at most 4096. Zero capacity permits NULL arrays.
An iterative heapsort canonicalizes only scratch: O(n log n) observation time,
O(1) auxiliary storage and exactly two caller buffers, with no allocation or
owner-array sorting. This does not change the complexity of legacy range selection.
Spans are borrowed **only during the callback** and reused at the next operation;
copy entries into application-owned storage to retain them. Copying the payload
struct alone does not retain the text-free spans. Handler/user borrow synchronously.
No mutation of active owner/model/scratch, recursion, destruction or foreign unwind
is supported; defer edits and teardown until submission/render consumption finish.

Preflight checks all arguments/counts/extents/alignment before any bookkeeping,
input consumption, callbacks or painting. Full declared scratch ranges must be
disjoint from each other, state, its full selection buffer, model object and bound
row/column descriptor arrays. Invalid arguments/extents take precedence over short
capacity, then overlap. Failure returns `events.valid=false`, no submitted/counts,
`events.control.error=CUI_TABLE_INVALID` and INVALID, WORKSPACE_CAPACITY or OVERLAP
in `preflight`, leaving owner/columns/scratch/input consumption unchanged. Success
sets `preflight=OK`, including a subsequent operation's CUI_TABLE_CAPACITY error.
Scratch allocations must additionally be exclusive of context/internal allocations,
label/string/group bytes, userdata and other resources: opaque aliases, dangling
pointers and actual allocation lengths cannot be universally checked. They remain
caller preconditions, not a runtime memory-safety guarantee.

A NULL selection callback ignores workspace entirely, preserves single/multi
legacy reduction and still supports optional sort observation; unused userdata is
ignored. Passive/disabled/loading behavior and bridge omissions above also apply.


## Column width and movement

Both scalar and set handlers add optional `on_column_width` and `on_column_move`.
Use designated initializers for these unreleased structs and rebuild clients when
updating the header. Legacy state/options/result layouts and entrypoints are
unchanged. The shared result adds saturating `column_width_count` and
`column_move_count`, with the existing `count_overflow` flag.

`cui_table_column_width_change` copies `{event, column, previous_width, width}`
with exact float widths after the old helper's clamps. It observes each successful
pointer-move resize or grip Left/Right key resize. Repeated clamped no-ops do not
notify. `cui_table_column_move_change` copies `{event, column, previous_index,
index}` after a successful Ctrl+Left/Right header reorder or actual pointer drop.
Indices are operation-time array positions; the stable exact `cui_id` identifies
the column after subsequent moves. Both payloads are pointer-free. Capture facts
inside the callback or copy the struct for later use.

Notifications run immediately after the successful helper mutation, before later
gesture cleanup, semantics and Styled paint. A drop callback still sees the active
drag state; it must not mutate it. Event metadata is frozen before descriptor
movement, uses the table root target and genuine triggering frame/index/source;
a pointer-move resize has pointer provenance without inventing an activation.
Callback order follows the reducer's existing operation order, including repeated
changes and cycles returning to the entry width/order. No extra consumed-event
filtering, focus changes or legacy result-flag reinterpretation is added.

Programmatic `cui_table_resize`/`cui_table_move_column` calls stay silent. Fixed
regions, immovable/disabled columns, finite/clamped widths, >4px drag threshold,
passive/layout-only/loading and missing callback behavior remain unchanged.
A below-threshold header gesture may still sort as an ordinary click; an actual
drag suppresses that click. Cancel/Escape ends the existing gesture without
rolling back an already-applied resize; no begin/commit/cancel event is invented.
Handlers/user remain borrowed, and the mutation/reentrancy/teardown restrictions
above apply. Base only submits semantics, so pointer routing requires actual
caller-provided geometry (the tests prime with Styled geometry). No new dispatch,
GPU validation or bridge/binding event coverage is implied.
