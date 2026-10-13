# VirtualList model and styled adapter

`cui_virtual.h` remains a standalone C11 variable-height model: compile
`src/base/cui_virtual.c` with the C runtime and math library, without Core, Scroll,
Theme, or a renderer. It stores stable keys and heights, supports keyed rebuilds,
retains measurements by key, and uses prefix sums for offsets, visible ranges,
height updates, and anchor capture/restore. Duplicate keys and invalid heights
reject atomic rebuilds. Memory is proportional to the configured model capacity;
virtualization bounds rendered rows, not model memory.

`cui_virtual_scroll_to` calculates Top, Center, Bottom, or Nearest placement for
an item and clamps to the content bounds. Invalid arguments preserve the output.
For a horizontal widget, Top/Bottom mean left/right. An oversized item uses its
start edge for Nearest. Apply the returned value to the widget's main scroll
coordinate. The model uses double precision; the existing UI scroll state uses
float coordinates, so very large scroll offsets lose subpixel resolution.

Include `cui_virtual_scroll.h` for the optional interaction and styled adapter.
Compile `src/base/cui_virtual_scroll.c` with Core, the virtual model, and base
Scroll. It resolves the model's main content extent, delegates wheel gestures
and scrollbar behavior to existing scroll infrastructure, and adds keyboard
scrolling for the focusable owner: Home/End, PageUp/PageDown, and main-axis arrows.
Compile `src/components/cui_virtual.c` with the styled Scroll component for
`cui_virtual_list_draw`. These dependencies are explicit; the model-only source
retains its previous independent linkage.

```c
cui_virtual_widget_state state = {0}; /* Persistent, caller-owned. */
cui_virtual_widget_options options = {
    .scroll = {.width = 320, .height = 240, .wheel_step = 48},
    .overscan = 2,
};
/* Inside a frame; model and theme are borrowed. */
cui_virtual_widget_result result = cui_virtual_list_draw(
    ui, list_id, theme, model, &state, options, render_item, user);
```

The item callback receives its index, stable key, wrapper ID, and exact logical
size. It runs only for the model window, including explicit overscan. Content is
placed inside a fixed-size clipped wrapper; callback content does not change the
model height. Include desired inter-row spacing in the model extents, since the
scroll option's gap must be zero. Keep keys and heights unchanged during a draw;
measure or update them before the next frame. No per-frame allocation occurs in
the adapter. Model storage and host row resources remain caller-controlled.

By default the adapter is vertical. Set `options.horizontal = true` to interpret
model heights as horizontal item widths. Vertical lists can enable cross-axis
horizontal scrolling through `scroll.horizontal` and `scroll.content_width`;
horizontal lists can enable vertical scrolling through `scroll.vertical` and
`scroll.content_height`. The main extent always comes from the model. Scrollbar
visibility, wheel step, axis locking, and precise gesture locking use the existing
Scroll options. Zero dimensions retain Scroll's 320 by 240 defaults. Invalid
options, a nonzero gap, or main totals beyond one quarter of the float range
reject before callbacks and scroll-state mutation.

Rows use IDs derived from the list root and the stable key, so reordering does
not change their identities. Use distinct child IDs inside callbacks; do not
submit another painted wrapper with the supplied row ID. The scroll owner has
focusable group semantics, and rows have passive list-item semantics with parent,
position, and total count. Overscan rows are also represented. These are partial
virtual children, not a fully materialized accessibility tree. Set
`no_item_semantics` when the host supplies its own row semantics. Selection,
row activation, focus retention for offscreen controls, and domain labels belong
to the host. Keyboard scrolling does not intercept keys targeted at row controls.

Rows are positioned using model offset minus scroll in double precision before
converting to local float coordinates. This avoids large spacer elements and
cancellation of small row differences at large global offsets. Precise wheel
input retains fractional scroll amounts within the float state's resolution.
When a nested child defers a wheel event after row callbacks ran, the adapter
updates this frame's translation, sets `window_pending`, and invalidates the UI.
The next frame renders the new window. `result.view` describes the callbacks that
ran, including overscan; `state.scroll` holds the final scroll position.

Verified scope: both orientations, variable extents, keyed reorder identity,
visible-window callback bounds, overscan, cross-axis scroll, precise wheel motion,
owner and scrollbar keyboard scrolling, disabled behavior, invalid-input state
preservation, empty models, nested deferred scroll translation, large local row
positions, and software clipping. The frozen GPUI Kit reference also supports
intrinsic cross-axis measurement, sizing behaviors, deferred scroll strategies,
and richer scroll-handle integration. CUI requires explicit cross extents and
precomputed heights; it does not automatically measure a selected row, implement
all upstream sizing modes, or guarantee focus restoration for virtualized children.
This adapter is a tested vertical slice, not a claim of full VirtualList parity.
