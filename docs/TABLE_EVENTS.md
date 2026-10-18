# DataTable sort transitions

Include `cui/cui_table_events.h`. The additive `cui_table_reduce_events`,
`cui_base_data_table_events` and `cui_data_table_events` take `{on_sort, user}`
and return unchanged legacy `control`, validity/submission flags and actual
`sort_count` (saturating with `count_overflow`). A missing callback ignores user
and preserves reduction, including multi-selection behavior.

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

This checkpoint exposes sort only. Selection, column movement/resize, activation,
requests and viewport/load events are separate slices. Existing bridge V1 table
notifications remain aggregate post-step snapshots; native multiplicity and
64-bit keys here do not imply binding parity.
