# Retained DataTable capacity checkpoint

Status: the opt-in C, Python, JavaScript and TypeScript APIs are implemented on the integration branch.
The ordinary setters accept up to 32 rows. See the
[supported API](../BRIDGE_COMPONENTS.md#retain-more-than-32-datatable-rows-in-c)
and [measured workload](../PERFORMANCE.md#retained-c-datatable-snapshots).
The design below records the approach and remaining language work.

## Starting constraints

- The bridge owns a matrix copy, a row array, a Base model, and a single
  coalesced paging observation. The initial setters allocated space for 32 rows
  and 512 cell pointers even for empty tables.
- The Base model borrows rows and owns two key indexes sized by its
  constructor capacity. Its lookup and viewport helpers already support large
  datasets. Existing C inbox checks use 1,000 and 10,000 rows.
- The legacy setter assigns positional keys and resets interaction state.
  The keyed setter keeps surviving keys and the old within-row offset.
  Both are transactional. Several bindings have their own 32-row validation
  or fixed-size marshalling buffers, including the Node addon.
- The V1 paging record already contains a row capacity. Paging applies to the
  submitted matrix and leaves remote cursors and request identity to the app.

## Interface

Add an optional keyed replacement API that takes a row capacity with each
matrix, plus a query for the provider's upper row capacity. This lets an app
bind an empty or small first page and opt into a larger table in the same
call. The public declarations are in `include/cui/cui_bridge.h`.

```c
uint32_t cui_bridge_data_table_max_row_capacity_v1(void);
cui_bridge_status cui_bridge_data_table_set_keyed_capacity_v1(
    cui_bridge_app *, cui_bridge_node, const char *const *headers,
    uint32_t column_count, const char *const *cells, const uint32_t *row_keys,
    uint32_t row_count, uint32_t row_capacity);
```

Keep the 16-column and 128 UTF-8 byte limits. Set a maximum
of 10,000 rows per retained table so the bridge can exercise the established
large-inbox workload. The requested capacity only governs future row admission
and near-end demand. It must be nonzero and at least the supplied row count.
Allocate storage and construct the Base model for the supplied rows, with
the minimum needed to represent an empty model. A later keyed replacement
can change the capacity atomically. Use checked arithmetic for rows, cells
and bytes before allocating. Return `CUI_BRIDGE_INVALID_ARGUMENT` for an
invalid capacity/count combination and `CUI_BRIDGE_CAPACITY` for a valid
request that exceeds the provider's row or byte budget.

The two existing setters keep their 32-row limit and original error behavior.
A successful call to either setter binds a matrix with capacity 32. Keep the
V1 paging threshold range of 0 through 32. It controls how early to request
a page. The V1 record reports the active matrix capacity, including the new
capacity for an opted-in table. Demand stays suppressed when that matrix is
full. No event payload size or callback timing needs to change.

## Ownership and bounded storage

- Stage owned storage for the entire replacement, including validated row
  keys and packed cell strings. Reuse the Base bind and anchor helpers.
  With headers and cells packed in a single arena, partial failure can free
  the staged storage in one path. Column and row views may borrow from that
  arena and the model must be destroyed before freeing it.
- Use an app-wide budget for opted-in tables. Charge the table metadata,
  row and cell indices, packed UTF-8 bytes and Base lookup buffers. Share the
  lookup size calculation with Base so the budget cannot drift from allocation.
  Keep the 32-row setters outside this budget.
- The 32 MiB retained budget admits a full 10,000-row, 16-column matrix
  with 128-byte cells. The measured charge on x86-64 is 24,569,992 bytes,
  including 20,640,000 bytes of cell strings with NUL terminators.
- Check the candidate's committed charge before allocation, with credit for
  the prior opted-in matrix on the same table. One replacement on the owner
  thread may briefly hold both versions. A 32 MiB retained budget and at most
  one staged allocation of up to 32 MiB bound the charged peak at 64 MiB.
  This excludes legacy matrices and allocator overhead. Release the recorded
  charge when a legacy matrix replaces the table, or on removal or shutdown.
- Publish only after validating strings, keys and limits and binding the new
  Base model. On failure, preserve data, selection, cursor, viewport, sort,
  load-more policy, unclaimed observation and accounting. A successful
  replacement keeps policy, clears stale demand, and queues no change event.

## Alternatives considered

- Raising the constants in both existing setters would change old input
  validation and allocate more memory for ordinary tables. It would also
  leave fixed marshalling limits in older addons.
- Reserving matrix storage eagerly would simplify accounting but make an
  empty table's cost proportional to its configured maximum.
- A borrowed row provider can offer much larger tables, but callbacks,
  lifetime, sorting and thread rules need a separate contract. The owned
  matrix is the first delivery path.

## Review and acceptance sequence

1. **Verified.** Implement the C API with checked allocation and accounting. Test 33 rows,
   1,000 and 10,000 rows, the capacity boundary, several simultaneous tables,
   a forced allocation failure, retry, removal and shutdown. Include a legacy
   differential fixture for the 32-row limit and positional behavior.
2. **Verified.** Drive actual scrolling, selection, keyboard continuation, sorted snapshots,
   an empty initial page, an append across 32 rows and a full table. Assert
   no input or callback replay and exact viewport anchoring. Check that a
   failed replacement preserves previously raised demand.
3. **Measured.** Measure bind and copy time, retained bytes, peak bytes and steady-frame
   behavior for narrow and wide matrices. Compare the old and modern paths
   at 32 rows to detect small-table regressions. Verify the number of
   submitted visible cells stays bounded as row count increases.
4. **Python and JS/TS verified. Go and Rust open.** Check the complete symbol
   group and the provider limit before marshalling. Keep installed legacy
   providers usable with their prior matrices. Add larger authored inboxes
   and verify copied input, rollback and the same scroll behavior.
5. **Open for bindings and gallery.** Update bridge guides, status metadata, packaged SDKs, the consumer skill
   and gallery examples as each supported surface is verified.
