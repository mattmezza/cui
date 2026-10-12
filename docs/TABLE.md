# Table and DataTable

The frozen GPUI Kit 0.7.1 Table is a stateless composition of sections, rows and
cells. DataTable adds a data delegate, scrolling, selection and column management.
This module provides both composition primitives and a retained data model, with
portable C11 public types and independent base behavior.

`cui_table_begin`, `cui_table_begin_row`, `cui_table_begin_cell` and
`cui_table_end` compose a themed table containing arbitrary caller content. Every
begin requires a matching end. These register passive table, row and cell roles;
callers can set semantic parent and descriptive details using core setters. There
are no implicit cell labels, column spans, sections or caption positioning.

Create `cui_table_model` with a bounded row capacity, then bind borrowed rows and
mutable columns. Rows and columns need nonzero unique keys within each dimension.
Binding rejects invalid labels, duplicate keys, excessive counts, invalid widths
and invalid width limits transactionally. Maximum capacity is one million rows
and 128 columns. The model allocates two row-key hash banks once at creation;
rendering, navigation, resize and movement do not allocate. Caller metadata stays
valid until a successful bind. Rebind after changing row ordering or keys; the
caller implements sorting/filtering and may keep selection keyed through both.
Columns may be mutated through resize and move functions between frames.

`cui_table_state` is caller controlled: selected row/column, keyboard cursor, sort
column/direction, horizontal/vertical scroll and transient resize gesture. Row,
column and cell selection use the same state; zero keys mean no selection. The
reducer returns change flags and activated keys. Sort activation cycles default,
ascending and descending; a newly activated column starts ascending. Sorting is
requested by the result, without rearranging caller data automatically.

The pure base API separates input reduction from semantic submission. A custom
renderer can call `cui_table_reduce`, submit the root outside clip scopes, submit
headers inside the header clip, and submit body semantics inside its translated
body clip. Lay out the public stable header, resize, row and cell IDs. The
`cui_base_data_table` convenience registers all semantics without drawing; it
links only core and table base. `cui_data_table` performs that separation and
paints the fixed header and virtual rows using theme tokens. A label callback
supplies borrowed accessible cell names. An optional render callback runs inside
each clipped cell and can draw arbitrary content; the application must govern
interactive children and disabled behavior consistently.

Up/Down skip disabled rows, Left/Right skip disabled columns, and Home/End move to
the first/last enabled row. PageUp/PageDown move a viewport of physical rows and skip disabled destinations. Navigation does not wrap unless requested. Multiple
events in one frame operate in queue order. Roving cell Tab focus follows the
cursor; the header cursor is a separate Tab stop, with Left/Right/Home/End navigation skipping disabled columns. Headers support Enter or pointer
sort activation, and Ctrl+Left/Right moves movable columns without crossing an
immovable column. Resize grips accept pointer dragging and Left/Right in eight
pixel increments. Secondary clicks return a contextual row/column request without changing selection; the application composes and controls the menu. Widths clamp to configured bounds; secondary button gestures
do not resize. Manual selection moves the cursor without changing selection.
Disabled state prevents reducer mutation.

The body uses fixed row heights, fractional scroll offsets, bounded row and column overscan and
real ancestor clipping. The header remains fixed vertically and shares horizontal
scroll with the body. Width-aware horizontal column virtualization includes partial edge columns. Result fields report actual visible row/column windows (without overscan), changes to those windows, and an opt-in load-more request near the end. Load requests fire once per threshold entry or changed row count; applications append rows and rebind. They are suppressed while disabled. Both grid and body expose scroll extents for core wheel routing and boundary bubbling. Wheel scrolling can move the cursor offscreen without
forcing it back into view; keyboard navigation reveals the cursor again. Grid,
row, column-header and grid-cell semantics include hierarchy, selection, row and
column indices, and total grid counts. Indices are one based data indices and
exclude the header. IDs are stable keyed hashes; applications must avoid hash/ID
collisions with other controls as required by the core.

Tests cover bounded row/column/cell ranges, toggles, capacity rejection, and transactional validation, keyed lookup in 10,000 rows, fractional
viewport arithmetic, clipped overscan, callback count and accessible labels,
fixed header geometry, horizontal clipping/virtualization, load request deduplication, ordered keyboard events, disabled skipping, manual
selection, sorting, column reorder, keyboard/pointer resize, width clamps,
secondary gestures, wheel/focus interaction and semantic hierarchy. The same
behavior tests run with a base-only core link; sanitizer validation instruments
both layout and reducer.

This is not full GPUI family parity. Remaining features include pinned left
columns,
column drag reorder, menu composition, asynchronous loading indicators, scrollbar
chrome, variable row heights, Table cell spans/sections/captions, and row-header
styling. Selection defaults to a single controlled keyed item in each dimension. An opt-in multiple-selection mode uses a caller-provided buffer of at most 4096 keyed items, with Ctrl/Meta toggles and Shift row/column/rectangular ranges. Disabled rows and columns are excluded from ranges. Capacity failures preserve the prior selection and keep the table rendered; callers may enlarge the buffer and retry. The anchor and buffer are controlled state, and callers must retain unique normalized keys in the buffer. Caller
sorting/filtering is supported but this module does not provide a comparator or
filter engine. Native accessibility adapters must map the portable grid roles.
