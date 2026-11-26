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

## Preserve position during live updates

For a vertical list, capture the first visible item before changing the model,
then restore its position before drawing the next frame. This example assumes
the list has completed a frame and keeps the same viewport height.

```c
cui_virtual_anchor anchor = cui_virtual_capture_anchor(list, state.scroll.y);
if (cui_virtual_rebuild(list, next_keys, NULL, next_count) == CUI_VIRTUAL_OK) {
    size_t index;
    if (anchor.valid && !cui_virtual_index(list, anchor.key, &index))
        anchor.within = 0;
    state.scroll.y = (float)cui_virtual_restore_anchor(list, anchor,
                                                       state.last_viewport.height);
}
```

Use the same capture and restore around `cui_virtual_set_height` when updating
row measurements. For horizontal lists use `state.scroll.x` and the viewport
width. A failed update leaves the model and scroll position intact. When the
viewport size changes, supply the new extent along the scroll axis.

The **Live inbox** gallery example uses variable heights and retains the visible
item and its partial-row offset when you prepend a message or resize the row
above it. Removing the first visible item clears its saved offset so its
successor starts at the top, subject to end-of-list clamping. This C workflow
uses the existing model helpers.

### Live updates in the retained C bridge

Create a `CUI_BRIDGE_VIRTUAL_LIST` with direct `CUI_BRIDGE_ITEM` children.
`cui_bridge_virtual_list_set_rows_v1` atomically replaces their order and heights.
Pass each current child once in display order. Height zero uses the default
32 logical pixels. The bridge copies the array. A malformed or stale handle,
duplicate row, invalid height or mismatched count leaves the previous order
and heights intact. Use the existing node APIs to change membership between
steps. The projection covers up to 64 retained rows. The immediate model
supports larger application-owned lists.

```c
#include <cui/cui_bridge.h>

/* The existing list contains exactly these three children. The newly created
 * Item is placed before the two previous ones, with its own extent. */
cui_bridge_status update_inbox(cui_bridge_app *app, cui_bridge_node inbox,
                               cui_bridge_node new_message, cui_bridge_node first,
                               cui_bridge_node second) {
    const cui_bridge_virtual_list_row_v1 rows[] = {
        {new_message, 48}, {first, 56}, {second, 32}
    };
    return cui_bridge_virtual_list_set_rows_v1(app, inbox, sizeof rows[0], rows, 3);
}
```

Call `cui_bridge_virtual_list_preserve_anchor_v1(app, inbox, 1)` when creating
the list. After its first layout, subsequent row edits retain the first visible
Item and the number of pixels scrolled inside it, clamped to the row and new
viewport. If that Item is removed, the next surviving Item in the previous
order starts at the top. If none follows, the nearest previous survivor is
used. When every previous Item has gone, the list starts at its beginning.
An explicit `cui_bridge_set_value` before the next step takes precedence.
Query `cui_bridge_get_value` for the resolved offset after the step.
Restoration itself is silent. Wheel and keyboard scrolling keep their queued
change behavior, including when they occur in the same step as an edit.
Turning preservation off keeps the numeric scroll offset, subject to clamping.

The runnable C inbox wires two buttons to prepend a message and resize the
first row. With a portable bridge build, the example drives those buttons
headlessly and checks that the same row stays visible at the same offset.

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_BRIDGE=ON -DCUI_BUILD_NATIVE=OFF
cmake --build build --target bridge_virtual_inbox_example
./build/bridge_virtual_inbox_example
```

To open an actual window, build with an SDL-enabled C bridge and pass the path
to a TrueType font as the first argument. [C example](../bindings/bridge/examples/virtual_inbox.c) ·
[Native bridge prerequisites](BINDING_BUILDERS.md)

The retained bridge currently uses a vertical viewport with explicit row
extents. Use the immediate API for horizontal lists or a host measurement
callback. Python, JavaScript, TypeScript, Go and Rust keep their existing
uniform-row retained builders.

Verified scope: both orientations, variable extents, keyed reorder identity,
visible-window callback bounds, overscan, cross-axis scroll, precise wheel motion,
owner and scrollbar keyboard scrolling, disabled behavior, invalid-input state
preservation, empty models, nested deferred scroll translation, large local row
positions, and software clipping. The frozen GPUI Kit reference also supports
intrinsic cross-axis measurement, sizing behaviors, deferred scroll strategies,
and richer scroll-handle integration. CUI requires precomputed main extents;
selected-row intrinsic measurement and Auto/Infer sizing are described below.
It does not automatically measure render callbacks or guarantee focus restoration
for virtualized children.
This adapter is a tested vertical slice, not a claim of full VirtualList parity.

Intrinsic cross sizing and Infer sizing
-------------------------------------

The frozen GPUI implementation measures one selected row, rather than every
row, to obtain the cross extent. The styled CUI adapter now supports this via
`options.measure`: a host callback receives the clamped `measure_index`, stable
key, and previous vertical viewport width (zero means unconstrained). Horizontal
measurement is unconstrained. It returns an intrinsic `cui_size`; the model
continues to own every main-axis item extent. Empty lists skip measurement.
Explicit content cross dimensions override the measured cross dimension.
Measurement runs once each styled draw, before any scroll state mutation.
Failure or nonfinite/negative measured dimensions rejects that draw. Hosts must
keep model keys/heights unchanged during measurement as during rendering.

`CUI_VIRTUAL_SIZE_INFER` resolves each viewport dimension from explicit scroll
width/height, then `available`, then intrinsic content extent. An otherwise empty
axis uses a one-pixel minimum because the scroll API reserves zero for its
legacy default. `CUI_VIRTUAL_SIZE_AUTO` remains the zero/default behavior with
320 by 240 fallback dimensions. The drawing-free reducer can use Infer with
explicit content cross dimensions; it does not invoke host measurement. This is
explicit host measurement, not automatic offscreen measurement of render rows.
The adapter does not reproduce the upstream horizontal Infer implementation's
use of known width when resolving height: explicit width and height remain
independent.
