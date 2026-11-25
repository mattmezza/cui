# Table and DataTable

The frozen GPUI Kit 0.7.1 Table is a stateless composition of sections, rows and
cells. DataTable adds a data delegate, scrolling, selection and column management.
This module provides both composition primitives and a retained data model, with
portable C11 public types and independent base behavior.

`cui_table_begin`, `cui_table_begin_row`, `cui_table_begin_cell` and
`cui_table_end` compose a themed table containing arbitrary caller content. Every
begin requires a matching end. These register passive table, row and cell roles;
callers can set semantic parent and descriptive details using core setters. Head-cell, header/body/footer section and caption helpers preserve that composition. Validated cell-span helpers sum explicit column widths; failed validation does not begin a box. Callers place captions after their sections and provide accessible head-cell names. There are no implicit data cell labels.

## Row and header colors

Set `custom_background` with a `cui_color` on each composable Table row.
For stripes, set it on every second body row using the row's position in
the displayed order. Supply readable foreground colors for the content
you draw. This example goes inside an active frame.

```c
void draw_files(cui_context *ui, const cui_theme *theme) {
    const float width = 240, height = 32;
    cui_table_begin(ui, cui_id_from(ui, "files-table"), theme, width);
    cui_table_begin_row_ex(ui, cui_id_from(ui, "files-header"), theme,
        (cui_table_row_options){.height = height, .header = true,
            .custom_background = true, .background = theme->accent});
    cui_table_begin_head_cell(ui, cui_id_from(ui, "files-name"), width, height,
                              CUI_TABLE_ALIGN_LEFT, cui_string("Name"));
    cui_text(ui, cui_string("Name"), theme->font_size, theme->accent_foreground);
    cui_table_end(ui);
    cui_table_end(ui);

    const char *files[] = {"Notes", "Roadmap"};
    for (size_t row = 0; row < sizeof(files) / sizeof(files[0]); ++row) {
        if (!cui_push_id(ui, files[row]))
            continue;
        cui_table_begin_row_ex(ui, cui_id_from(ui, "row"), theme,
            (cui_table_row_options){.height = height,
                .custom_background = row % 2 != 0, .background = theme->muted_background});
        cui_table_begin_cell(ui, cui_id_from(ui, "name"), width, height,
                             CUI_TABLE_ALIGN_LEFT);
        cui_label(ui, theme, cui_string(files[row]));
        cui_table_end(ui);
        cui_table_end(ui);
        cui_pop_id(ui);
    }
    cui_table_end(ui);
}
```

DataTable's `cui_table_options.striped` alternates unselected data cells by
model row position. It uses the theme's `surface` and `background`, with
`muted_background` as the alternate when those tokens are equal. Selected
cells use `primary` and `on_primary`. Scrolling preserves the stripe phase,
and selection follows the row key when the caller rebinds sorted records.
Try **Custom header** and **Alternating rows** in the Table gallery, or
**Alternating rows** in DataTable.

## Virtual DataTable

`cui_table_column_default(key, label)` supplies a 100-pixel, resizable, movable column with a 20-pixel minimum and optional sorting disabled. `cui_table_options_default()` enables border and scrollbars with row selection and no stripes, matching the supported reference defaults. Zero initialization remains available for applications choosing their own configuration. Widths remain bounded by CUI's 65,536-pixel safety limit.

Create `cui_table_model` with a bounded row capacity, then bind borrowed rows and
mutable columns. Rows and columns need nonzero unique keys within each dimension.
Binding rejects invalid labels, duplicate keys, excessive counts, invalid widths
and invalid width limits transactionally. Maximum capacity is one million rows
and 128 columns. The model allocates two row-key hash banks once at creation;
rendering, navigation, resize and movement do not allocate. Caller metadata stays
valid until a successful bind. Rebind after changing row ordering or keys; the
caller implements sorting/filtering and may keep selection keyed through both.
Columns may be mutated through resize and move functions between frames. Columns marked fixed_left must form a contiguous prefix. Their header and cells remain fixed horizontally, with separate clip scopes preventing scrolling cells from receiving input beneath pinned cells. Column movement cannot cross the pinned boundary. Keep the total pinned width below the viewport width to leave space for scrolling columns. The column viewport describes the scrolling suffix; fixed columns are available through dedicated count/width accessors.

`cui_table_state` is caller controlled: selected row/column, keyboard cursor, sort
column/direction, horizontal/vertical scroll and transient resize gesture. Row,
column and cell selection use the same state; zero keys mean no selection. The
reducer returns change flags and activated keys. Sort activation cycles default,
ascending and descending; a newly activated column starts ascending. Sorting is
requested by the result, without rearranging caller data automatically. On `sort_changed`, sort your application records, reorder the borrowed row array, then call `cui_table_bind` and invalidate the UI. Resolve cell labels from the newly ordered row keys, rather than from the original position. The gallery demonstrates ascending/descending/default ordering for Name and numeric Size, with stable key order for equal Status values.

The pure base API separates input reduction from semantic submission. A custom
renderer can call `cui_table_reduce`, submit the root outside clip scopes, submit
headers inside their pane clips, and submit body semantics inside its translated
body clip. Per-column/row/cell submission helpers support distinct pinned and scrolling clip scopes. Lay out the public stable header, resize, row and cell IDs. The
`cui_base_data_table` convenience registers all semantics without drawing; it
links only core and table base. `cui_data_table` performs that separation and
paints the fixed header and virtual rows using theme tokens. Sortable headers show SVG indicators for unsorted/ascending/descending states, with centered labels and a pointer cursor. The icons use the theme icon loader, so applications can replace their SVGs. A label callback
supplies borrowed accessible cell names. An optional render callback runs inside
each clipped cell and can draw arbitrary content; the application must govern
interactive children and disabled behavior consistently.

Up/Down skip disabled rows, Left/Right skip disabled columns, and Home/End move to
the first/last enabled row in row mode, or the first/last enabled column in the current row in cell/column mode. Escape clears controlled selection and range anchors while retaining the keyboard cursor; disabled/loading tables ignore it. PageUp/PageDown move a viewport of physical rows and skip disabled destinations. Navigation does not wrap unless requested. Multiple
events in one frame operate in queue order. Roving cell Tab focus follows the
cursor; the header cursor is a separate Tab stop, with Left/Right/Home/End navigation skipping disabled columns. Headers support Enter or pointer
sort activation, and Ctrl+Left/Right moves movable columns without crossing an
immovable column. Primary pointer dragging past four pixels reorders at header midpoints on drop; it respects pinned regions and immovable columns, suppresses sort activation, and cancels on Escape/cancel or release outside the header. Resize grips accept pointer dragging and Left/Right in eight
pixel increments. Secondary clicks return a contextual row/column request without changing selection; the application composes and controls the menu. Widths clamp to configured bounds; secondary button gestures
do not resize. Manual selection moves the cursor without changing selection.
Disabled state prevents reducer mutation.

Enabled movable column headers opt into touch drag priority: a primary touch
starting on the header reorders rather than handing off to ancestor scrolling.
Nonmovable headers and body content retain pan behavior. Touch drop admission
uses that UP event's coordinates and the prior clipped/modal-eligible semantic
region, so a later mouse sample in the same batch cannot change its result.
Mouse routing and callback timing remain unchanged. See [indexed pointer-region
admission](EVENT_HOVER.md) for the Core query contract and occlusion rules.

The body uses fixed row heights, fractional scroll offsets, bounded row and column overscan and
real ancestor clipping. The header remains fixed vertically and shares horizontal
scroll with the body. Width-aware horizontal column virtualization includes partial edge columns. An optional pinned row-number header selects whole rows and navigates enabled rows. Its result requests ROW selection mode; the caller applies that request to the controlled options. Row headers continue working when all data columns are disabled. Result fields report actual visible row/column windows (without overscan), changes to those windows, and an opt-in load-more request near the end. Load requests fire once per threshold entry or changed row count; applications append rows and rebind. They are suppressed while disabled. Both grid and body expose scroll extents for core wheel routing and boundary bubbling. Wheel scrolling can move the cursor offscreen without
forcing it back into view; keyboard navigation reveals the cursor again. Grid,
row, column-header and grid-cell semantics include hierarchy, selection, row and
column indices, and total grid counts. Indices are one based data indices and
exclude the header. IDs are stable keyed hashes; applications must avoid hash/ID
collisions with other controls as required by the core.

DataTable uses the same default as Scrollable: one ordinary wheel tick moves
48 logical pixels. Touch panning and host events with `scroll_precise = true`
move by their supplied logical pixel deltas, including fractional values.
Hosts that supplied pixel deltas to DataTable before this fix should set
`scroll_precise`. Wheel events over headers can scroll horizontally. The body
can scroll on both axes. DataTable consumes a delta that moves either axis and
offers later refused deltas at the edge to a scrollable ancestor.

Keep a model bound across frames and rebind when the dataset changes. The
private-X11 inbox check shows how a custom cell can use `cui_hovered` with a
stable row ID, while composition stays within the virtual viewport. See the
[reproducible workload and timings](PERFORMANCE.md#native-datatable-inbox).

### Keep the visible row during a live update

Capture the first visible row before mutating or replacing the current borrowed
rows. After a successful bind, restore the scroll position before the next
submission. Use the corresponding table options at each step so both helpers
account for the body viewport and any scrollbar gutters.
If the styled table inherits scrollbar settings from its theme, set
`options.scrollbar_style` to that style before calculating offsets, and set
`options.scrollbars` when the effective style shows or auto-hides the bars.

```c
/* Between frames. next_rows and columns stay live while bound. */
cui_table_row_anchor anchor =
    cui_table_capture_row_anchor(model, state.scroll_y, options);
if (cui_table_bind(model, next_rows, next_count, columns, column_count) == CUI_TABLE_OK) {
    state.scroll_y = cui_table_restore_row_anchor(model, anchor, options);
}
```

The offset follows the same key and preserves the scrolled distance within its
row. If the key disappeared, restoration uses the former index, or the final
row when the list is now shorter. If the row is shorter than the saved offset,
it aligns to the row's top. The viewport can clamp at either edge. An empty
model or a zero-key anchor restores to the top.

These C helpers only calculate offsets and allocate nothing. Selection and the
keyboard cursor still follow their existing keyed contracts. Bind and scroll
policy remain application-controlled, so a deliberate change such as sorting can
choose a new scroll position. Supply valid replacement arrays to avoid changing
the current borrowed rows on a failed bind.

Choose **Live updates** in the DataTable gallery and scroll partway down.
**Receive a file** schedules one update two seconds later, giving you time to
focus a cell before the file arrives. The gallery prepends the file by default
and respects any active sort. The selected cell and visible row stay in place
while the model rebinds. Changing examples or disabling the preview cancels
the pending update. The example demonstrates the UI-thread rebind step using a
local timer.

Retained C bridge consumers can use the [keyed DataTable setter](BRIDGE_COMPONENTS.md#replace-datatable-rows-without-losing-the-viewport) to copy a bounded matrix and preserve the viewport on replacement. Use the [opt-in C setter](BRIDGE_COMPONENTS.md#retain-more-than-32-datatable-rows-in-c) for up to 10,000 rows per table within the app's storage budget.

### Request another page

Choose **Paged inbox** in the gallery. Start with 16 files and scroll near the
bottom to load the next 16. The preview keeps the loaded rows interactive and
shows progress below the table. It stops at 48 files. A local timer simulates
arrival, so the example works offline. Switching stories or disabling the
preview cancels a pending page.

Set `options.load_more` while the data source has more rows, with
`load_more_threshold` set to the count of rows remaining after the visible
window. Use `result.load_more` to start a request. Keep a separate in-flight
flag because the table can signal again after leaving and re-entering the
threshold. Reserve `options.loading` for a view that replaces its rows with
a loading placeholder. For incremental loads, draw the busy status alongside
the existing table.

On the UI thread, check that the response belongs to the current dataset and
request before applying it. Capture and restore the row anchor around a
successful bind of the complete updated view, with the effective scrollbar
options. Keep the bound snapshot alive until it is replaced. Clear the in-flight
flag after completion or cancellation. To rearm after cancellation, submit with
`load_more` off while disabled, as the gallery does. With a failed bind the
gallery keeps its old rows and lets the user leave and re-enter the threshold
to retry. The copyable C example shows the request and completion steps.

Retained C, Python, JavaScript, TypeScript, Go and Rust applications can configure the
threshold and poll one copied near-end snapshot after a successful step. The
examples exercise final-page delivery. The C, Python, JavaScript, Go and Rust
examples also demonstrate logical cancellation and rejection of a late response.
[C paging](BRIDGE_COMPONENTS.md#load-another-datatable-page) ·
[Python paging](BINDING_BUILDERS.md#page-a-python-datatable) ·
[JavaScript and TypeScript paging](BINDING_BUILDERS.md#page-a-javascript-or-typescript-datatable) ·
[Go paging](GO_BUILDERS.md#page-a-go-datatable) ·
[Rust paging](RUST_BUILDERS.md#page-a-rust-datatable)

Custom leaf-header, group-header, row-header, whole-row and trailing-column render hooks use option userdata. Group headers accept up to sixteen levels of bounded spans. Every level must cover all data columns exactly, and no group may cross the fixed-column boundary; `cui_table_group_headers_valid` lets applications validate a configuration before rendering. Group cells expose column-header semantics at their first column; the callback receives the complete span. Whole-row callbacks run only for virtualized rows and can add row-level decoration or content before the cells. The optional trailing column uses a caller-selected width (12 logical pixels by default) and participates in horizontal scroll extent; its callback runs for each header level, the leaf header and visible rows. Loading replaces body rows with a busy status and suppresses input/load requests; empty and loading placeholders have optional render hooks and accessible default labels.

Set `scrollbars` to reserve scrollbar gutters only when an axis overflows. `bar_thickness` controls their logical thickness. The drawing-free reducer delegates keyboard, track and thumb behavior to the reusable Scroll base primitive, while the styled table paints the same scrollbar state. The grid and scrollbar semantics share the table as their scroll owner. Applications that build a custom renderer can use `cui_table_scroll_layout` to obtain the reduced viewport and scroll maxima.

Tests cover four-theme static sections/spans/caption geometry, multi-level group geometry and semantics, virtual whole-row and trailing-column callbacks, row-header selection mode requests, drag reorder/cancellation, empty/loading/custom hooks, bounded row/column/cell ranges, toggles, capacity rejection, and transactional validation, keyed lookup in 10,000 rows, fractional
viewport arithmetic, clipped overscan, callback count and accessible labels,
fixed header geometry, pinned header/cell geometry and clipping/hit behavior, horizontal clipping/virtualization, load request deduplication, ordered keyboard events, disabled skipping, manual
selection, sorting, column reorder, keyboard/pointer resize, width clamps,
secondary gestures, wheel/focus interaction, scrollbar keyboard/thumb behavior and semantic hierarchy. The same
behavior tests run with a base-only core link; sanitizer validation instruments
both layout and reducer.

This is not full GPUI family parity. Remaining features include composed menus, cell Tab traversal, double-click events, per-column padding/default sort configuration, and variable row heights. Selection defaults to a single controlled keyed item in each dimension. An opt-in multiple-selection mode uses a caller-provided buffer of at most 4096 keyed items, with Ctrl/Meta toggles and Shift row/column/rectangular ranges. Disabled rows and columns are excluded from ranges. Capacity failures preserve the prior selection and keep the table rendered; callers may enlarge the buffer and retry. The anchor and buffer are controlled state, and callers must retain unique normalized keys in the buffer. Caller
sorting/filtering is supported but this module does not provide a comparator or
filter engine. Native accessibility adapters must map the portable grid roles.
