# Retained keyed variable-height virtualization

`cui/cui_virtual.h` exposes a retained `cui_virtual_list`, separate from the existing stateless linear `cui_virtualize` utility. The application declares a maximum row capacity and positive default height at creation. The handle owns fixed arrays, a Fenwick tree, and two key tables; rebuilds, queries, updates, anchor capture, and anchor restoration do not allocate. `cui_virtual_memory_bytes` reports its requested allocation bytes, excluding allocator bookkeeping.

Rebuild supplies the ordered stable 64-bit keys, including key zero. Duplicate keys, excess row count, nonfinite/nonpositive heights, and unsupported totals reject the entire rebuild without changing the current list. Supplying NULL heights retains each old measurement by key across insertion, removal, and reordering; new keys receive the default height. Supplying heights explicitly replaces all measurements. Rebuild constructs the new key table and tree in O(capacity + row count) work, with expected constant-time key lookup from a mixed open-addressed hash table at less than 50% load. Adversarial key collisions can degrade hash lookup; the Fenwick operations retain their logarithmic bound.

`cui_virtual_offset(index)` returns the height before that row. `cui_virtual_lower_bound(offset)` returns the first row whose bottom exceeds the offset, with row count representing the end. Prefix offsets, updates, and lower-bound searches are O(log N). Key height updates combine the key lookup with the tree update. Heights are logical pixels stored as doubles. Zero-height rows are rejected. Totals above DBL_MAX/2 are rejected; like other floating geometry, extremely disparate magnitudes can lose subpixel precision, and repeated updates can accumulate rounding error.

Viewport ranges are half-open `[begin,end)`, with optional overscan measured in rows on both sides. The range reports top and bottom spacer heights, total extent, and scroll clamped to `[0,max(total-viewport,0)]`. A row beginning exactly at the viewport's bottom is excluded. Negative or nonfinite query scroll/viewport values normalize to zero. A zero viewport has an empty visible range before overscan. No rows need to be materialized to query the range.

Capture an anchor before changing measurements or rebuilding: it stores the row key and offset within that row. Restore looks up the same key in the new order and clamps its within-row offset to its new height. If the key was removed, it uses the captured ordinal, clamped to the remaining rows. Restored scroll is clamped to the new viewport extent. An empty list or invalid anchor restores zero. This supports stable scroll positioning during reorder, resize, and measurements; applications decide when to capture and restore.

This model does not create a styled Virtual/ScrollArea widget, own scrollbars, handle focus for unmounted controls, perform asynchronous measurements, or implement keyboard navigation. Those behaviors require a later widget layer that submits the returned rows and spacers with stable IDs. It is an efficient data/geometry model, not a full component parity claim.

Tests compare 20000 deterministic updates and queries against a linear reference, and cover exact boundaries, viewport overscan, keys including zero, retained measurements after reorder/removal, anchors, transactional rejection, empty lists, and oversized configurations under ASan/UBSan.

A portable `clock()` benchmark compiled with GCC `-O2` in this workspace measured:

| Rows | Rebuild | 100000 offset + lower-bound + update triples | Allocation bytes |
| ---: | ---: | ---: | ---: |
| 1000 | 0.035 ms | 6.851 ms | 105648 |
| 10000 | 0.612 ms | 9.909 ms | 1448688 |
| 100000 | 7.134 ms | 15.627 ms | 12388720 |

These are local CPU timings, not performance guarantees. Run `test_virtual --benchmark` to reproduce on the current machine. Normal tests impose no timing threshold.
