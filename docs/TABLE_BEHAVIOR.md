# Table and DataTable behavior contracts

Source reference: GPUI Kit 0.7.1, commit `87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/component/src/table/{table,column,state}.rs`. These are original portable C implementations. This guide describes the implemented subset and compatibility choices, not full family parity or native accessibility certification.

## Selection and action columns

`cui_table_column.nonselectable = true` prevents column/cell selection without disabling the column. Header sorting, resizing, reordering and focus remain available; custom child buttons keep their independent base behavior. Whole-row selection still highlights cells in that column, matching the source's action-column granularity. `disabled` retains its separate existing behavior.

`cui_table_options.row_selection_disabled`, `column_selection_disabled`, and `cell_selection_disabled` independently block the corresponding selection mode. They suppress selected presentation for that mode while preserving stored selected keys, so re-enabling the policy restores controlled selection. They do not disable sorting, cursor navigation or custom row/cell controls. Row-header activation does not request row-selection escalation while row selection is blocked. Nonselectable cells remain focusable grid cells with readonly selection semantics, enabling navigation through a row containing custom action controls. Activation result fields can still report a clicked cell; `selection_changed` stays false when selection is rejected.

Range selection skips nonselectable columns in cell/column mode before counting required capacity. It does not insert phantom action-column selections. Application-provided selection storage remains bounded by its declared capacity. The model-free `cui_table_is_selected()` helper sees mode policies and stored keys; because it has no model argument, callers implementing custom paint must additionally check `column.nonselectable` outside row mode. Styled DataTable applies this check itself.

```c
columns[1].nonselectable = true;
cui_table_result result = cui_data_table(ui, id, theme, model, &state,
    (cui_table_options){
        .width = 480, .height = 240,
        .selection_mode = CUI_TABLE_SELECT_CELL,
        .row_selection_disabled = true,
        .sort_cycle = CUI_TABLE_SORT_CYCLE_SOURCE,
        .accessible_name = cui_string("Workspace files")
    }, label_cell, render_cell, userdata);
```

## Sort requests and borrowed data

Zero-initialized `sort_cycle` remains `CUI_TABLE_SORT_CYCLE_LEGACY`: Default→Ascending→Descending→Default. Explicit `CUI_TABLE_SORT_CYCLE_SOURCE` follows the pinned source: Default→Descending→Ascending→Default. Both mouse/header activation and keyboard activation use the same reducer. Switching to another column begins at that policy's first direction. Invalid sort-cycle enums reject submission before layout/state changes.

Sorting updates `state.sort_column`/`state.sort` and returns `sort_changed`; it does not mutate borrowed row data. The application handles the request, reorders/rebinds its rows, and retains stable row keys. Selection and semantic row indices reconcile against those keys. Result flags describe the final state for one frame; multiple queued input events may coalesce into one flag, so this is not an event-by-event callback log. A frame without a new sort event does not repeat the request.

The styled implementation invokes the supplied label callback once per rendered cell for semantic and default visible text, rather than twice. Custom render callbacks also run once per rendered cell. Hosts may themselves call the label callback within custom rendering; CUI does not prevent that application choice. Models, callback userdata and arrays stay borrowed according to the existing bind contract; no ownership transfer or implicit async work occurs.

## Named composable Table and cell sizing

`cui_table_begin_ex()` adds an explicit root accessible name and row/column counts. `cui_table_begin_cell_ex()` supplies header/body role, accessible name, parent ID, one-based row/column indices, alignment and sizing. `cui_table_begin_row_checked()` validates row options before opening a row. End these checked begins only when they return true. A caption remains visible descriptive content; it does not replace the explicit root name.

Names are synchronously copied by Core into its bounded frame storage; input string bytes need only remain valid during submission. Root counts and cell/row indices must fit `INT_MAX`. Dimensions are finite 0..65536, padding finite 0..4096, and enums must be defined values. Invalid checked inputs, including a self-parent relationship, add neither layout nor semantics. Legacy void begin/end helpers retain their existing defaults and normalization behavior, including the generic `Table`/`Cell` names and 4px cell padding.

Explicit positive cell height overrides design sizing. A non-INHERIT `design_size` with zero height uses `cui_design_measure()` control height at density `theme.font_size / 16`. Default design padding uses its vertical control padding so the design line height fits; an explicit `custom_padding` override wins. This is an intentional symmetric padding adaptation, not the source's independent edge-padding API. Row design sizing follows the same height rule. Child typography is host-owned: derive font and line height through `cui_design_measure()` and use those when submitting the child text. Choosing a design size does not silently resize arbitrary nested controls.

```c
cui_design_metrics metrics;
cui_design_measure(CUI_DESIGN_LG, theme->font_size / 16, &metrics);
if (cui_table_begin_cell_ex(ui, cell_id, theme,
        (cui_table_cell_options){.width = 220, .design_size = CUI_DESIGN_LG,
          .name = cui_string("Paid"), .parent = row_id,
          .row_index = 1, .column_index = 2})) {
    cui_text_with_line_height(ui, cui_string("Paid"), metrics.font_size,
                              theme->text, metrics.line_height);
    cui_table_end(ui);
}
```

Column spans continue to use `cui_table_span_width()` and the existing span begin helper. Core has no row/column-span semantic fields, so this change does not invent them or claim span announcements. Independent per-column edge padding, full source Table flex/minimum-size behavior, dedicated Table palette tokens and independent scrollbar-axis visibility remain gaps. The source's pinned feature is fixed-left columns; no pinned-row feature is introduced.

## Evidence

`tests/unit/test_table_selection_contracts.c` tests source/legacy cycles, borrowed-data immutability and stable-key rebind, selection-only opt-outs, restored stored keys, header sort/resize and custom child pointer/Enter activation, range capacity, label/render callback counts, strict invalid-input behavior, explicit header/root/row/cell semantics, and actual nested text containment at all 14 design sizes in two densities. Existing `test_table.c` retains legacy navigation/resize/move/virtualization/selection coverage.

The real WASM gallery exposes canvas buttons for selection, action-column and sort-policy choices. Its Table story uses named parts, actual design typography and horizontal scrolling for large text. `tests/web/test_table_selection_contracts.py` checks Chromium 1208 source sort cycling, action-button touch activation with selection blocked, readonly action-cell semantics, all-size metadata/text sizing, mobile touch and disabled controls. Worker validation passed strict C11 warning builds, the scoped headless suite and ASan/UBSan with leak detection. Standard CTest passed 102/103; only stale generated `dist/cui_table.h` failed its distribution check, which the integrator owns. Browser evidence uses the approved generated-JS bridge for the integrator-owned Table selector/height and DataTable420 preview. The integrator owns test registration. Native screen-reader output and per-platform accessibility bridges remain separately unverified.
