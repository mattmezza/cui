# Retained DataTable paging checkpoint

This checkpoint proposes a retained C bridge API. The immediate C DataTable
already returns a near-end edge in `result.load_more`. Its aggregate result
and timing are covered in [the Table event checkpoint](DATA_TABLE_REMAINING_EVENTS_SLICE.md).
The retained renderer calls that reducer but currently drops the edge.

## Interface and ownership

Configure an existing retained matrix with
`cui_bridge_data_table_configure_load_more_v1(app, table, enabled, threshold)`.
Pass 0 or 1 for enabled and a threshold of 0 to 32 remaining rows. A matrix
with headers and no rows is a valid starting point. The threshold uses the
table's own visible window without overscan. It measures the table's viewport
and does not account for clipping by external scroll regions.

Poll with `cui_bridge_data_table_poll_load_more_v1(app, table, size, out)`.
The V1 record is 32 bytes. It contains the actual Core frame sequence, copied
row count, zero-based exclusive visible window, last supplied row key, bridge
row capacity, and a reserved zero field. The last key is zero for an empty
matrix. It identifies a local row and is independent of a service's pagination
cursor. `cui_bridge_data_table_load_more_size_v1()` reports the expected size.
Poll returns EMPTY when no edge is available and leaves the output untouched
on any unsuccessful call. A successful poll consumes one copied observation.
Use the same owner thread and normal between-step bridge calls. A copied record
outlives the table and application.

The edge reports demand from the viewport. The application owns scheduling,
one in-flight request per source, cancellation and stale-response rejection.
Polling has no effect on selection, input consumption, platform defaults or
the viewport. The API makes no claim that input or a fetch caused the edge.
Enabling an empty or short table near its end can raise an edge without input.

## Timing, coalescing and invalidation

- The default is off. Reduce once through the existing Styled table. The
  successful step makes its edge available to poll alongside existing event
  handling. At most one unclaimed edge per configured table is retained.
  Additional eligible edges coalesce into that existing record.
- Setters for successful data replacement keep the load-more policy, expire an
  unclaimed edge, and allow the new data to request another page. Failed
  replacement preserves existing rows, policy and unclaimed edge.
- Leaving the threshold and re-entering allows a new edge after consumption.
  A change to enabled or threshold clears the old edge and re-arms the
  calculation. Applying the same enabled policy is idempotent.
- A directly disabled table loses its unclaimed edge immediately. Disabled
  ancestry observed during submission also clears it. Omitted and inert
  submissions raise no new edge. Previously published edges from an omitted
  or inert table stay available until explicit cancellation, replacement,
  removal or poll. An owner may turn paging off while asynchronously loading
  and re-enable it on completion or retry.
- A full 32-row legacy bridge matrix emits no edge. Increasing the retained
  row capacity is separate work. The immediate API keeps its independent
  capacity and behavior.
- A failed step discards only edges staged by that step and re-arms those
  tables. Edges from earlier successful steps remain readable. Removal frees
  the mailbox with the retained matrix. A reused node slot requires a new
  generation.

Keep one copied record and a transient finish-list link per retained matrix.
Finish visits only matrices that produced a new edge. This uses bounded memory
and requires no app-wide queue reservation, allocation in submission or
changes to the existing event drain order. The list remains internal to a
step and is empty across public calls.

## Alternatives and compatibility

Adding a generic change kind or a new selection reason would conflate loading
demand with selection and introduce an unknown payload to old event readers.
A synchronous callback from composition would require a new retained lifetime
and reentrancy contract. Polling the copied edge is additive and keeps existing
handlers, structure sizes and dispatch timing. A binding may dispatch a typed
handler from its owner loop after polling. It should consume each edge through
one path and keep fetch initiation outside the renderer.

Implement and test C first. The optional symbols are the capability boundary
for Python, JS/TS, Go and Rust. Older bridges, including alpha.4, support the
unchanged positional table setter. Existing keyed bindings do not expose this
new paging API until their optional-provider and authored UI checks land.

## Evidence to collect

Cover empty startup, actual viewport entry and re-entry, coalescing, independent
tables, disable/cancel/re-enable, invalid options and output size, keyed
replacement and failure, omitted or inert submissions where reachable,
capacity and stale handles. Force failed composition or a full legacy event
queue and prove that old events and requests stay independent. Keep legacy
selection/sort and keyed replacement tests. A headless example should drive
queued input, deliver an offline page, and preserve row position and selection.
