# Retained dock tree and panel model

`cui/cui_dock.h` provides an application-owned retained model for dock trees. Leaves are ordered tab groups; interior nodes are horizontal or vertical splits. Each panel/tab item has an application-supplied stable 64-bit key, including zero. Keys are globally unique within the model. Application content, labels, icons, and platform resources remain outside the model.

Tab groups and splits have generational node IDs. IDs remain valid while their nodes survive moves and reorders; collapsed/freed nodes reject stale IDs even if their storage slots are reused. Successful restore invalidates every previous node ID. Applications can rediscover a panel's current tab group by its stable key. Generation exhaustion rejects operations requiring new nodes.

Creation preallocates two node/panel state banks. Defaults are 128 nodes, 256 panels, depth 32, and 65536 serialized bytes. Explicit supported ceilings are `CUI_DOCK_NODE_LIMIT` (4095), `CUI_DOCK_PANEL_LIMIT` (4096), depth 64, and `CUI_DOCK_SERIALIZED_LIMIT` (1048576 bytes). Counts and allocation products are checked before allocation. Operations, geometry queries, serialization, and restore allocate no additional memory. `cui_dock_memory_bytes` reports requested handle/bank storage, excluding allocator bookkeeping. The initial model is an empty root tab group.

Insert creates a panel at a tab index; `SIZE_MAX` appends. Split wraps a nonempty tab group with a new split and a new tab group containing the supplied panel. Horizontal splits divide width; vertical splits divide height. Ratio refers to the first child's share of the available span after the gap. Move transfers a panel to another group and activates it there. Reorder uses its final index in the original group and preserves its active state. Removing or moving a group's last panel removes that empty leaf and promotes its sibling, collapsing the redundant split. An empty root is retained. Active-panel removal chooses its next neighbor, or previous neighbor at the end. Duplicate keys and invalid destinations reject without changing state.

Each node may declare nonnegative minimum dimensions up to 1e9 logical pixels. Child minima combine through the tree with the requested gap. Geometry returns one rectangle per tab group, with active key and panel count. Ratios clamp to child minima when those minima fit; otherwise children compress proportionally to their minimum spans and `fit_minimum` is false. When both minima are zero, the configured ratio applies. Gaps shrink to the available span. Geometry remains within the supplied rectangle even when the minimum requirements cannot be satisfied. NULL output queries the count; insufficient output capacity leaves geometry storage untouched.

The model uses fixed arrays and scans for panel/free-slot lookup. Most mutations are O(configured capacity), while linked tab insertion/reorder includes an ordinal walk. Layout computes minima through the bounded tree and may revisit subtrees, taking O(nodes * depth) work. Restore performs bounded scans for allocation and duplicate detection and has quadratic worst-case work in the configured node/panel counts. These deliberately modest caps suit desktop docking rather than huge datasets; the separate virtual-list model handles large row collections.

Serialization is deterministic for a given topology, panel order, active selection, ratios, and minima. Format version 1 is an ASCII prefix tree:

```text
CUIDOCK 1
T min_width min_height active_ordinal panel_count panel_key...
H ratio min_width min_height first_child second_child
V ratio min_width min_height first_child second_child
```

Whitespace separates tokens; the writer emits canonical whitespace, decimal numbers with 17 significant digits, and unsigned decimal keys. Split children recursively use the same grammar. The writer normalizes locale decimal punctuation without changing process locale. The parser parses ASCII decimal tokens independently of locale. Integer overflow, nonfinite values, oversized tokens/input, unsupported versions, duplicate keys, invalid active ordinals, excessive depth/capacity, incomplete trees, embedded NUL, trailing tokens, and nonroot empty groups reject. Decimal reconstruction follows the platform's C floating-point arithmetic; normal geometry roundtrips to the same canonical representation in the tested implementation.

Serialization first measures required bytes, excluding the terminating NUL. A NULL output queries that length; insufficient capacity or configured byte-budget exhaustion leaves the supplied buffer untouched. Restore parses into the preallocated scratch bank and only swaps the current tree after full validation. Failed restores preserve the entire current state and IDs. Calls belong on the owning UI thread; concurrent mutation and concurrent process-locale changes during serialization are unsupported.

Tests cover nested geometry and minimum compression, tab order/active selection, move/remove collapse, stale IDs, stable keys including zero and UINT64_MAX, deterministic roundtrip, transactional parser rejection, huge/deep inputs, explicit capacity rejection, 7000 deterministic mutations with tree/panel invariants, and 2000 malformed random restore inputs under ASan/UBSan.

This model does not implement styled Dock widgets, drag/drop targeting, resizing gestures, floating native windows, focus scopes, or panel rendering. It supplies the retained tree, operations, geometry, and persistence required for a later widget layer; it does not claim full Dock component parity.
