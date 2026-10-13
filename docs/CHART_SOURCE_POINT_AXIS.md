# Source point-axis layout

`CUI_CHART_LAYOUT_SOURCE_POINT` is an explicit Line/Area layout mode matching the point-axis geometry in GPUI component 0.7.1, pinned at `87d10ae5e1299d1be18670a37570c5402f952e34` (`chart/mod.rs` point-count normalization and `AXIS_GAP`, `crates/base/src/plot/axis.rs` label offsets). Zero-initialized options retain `CUI_CHART_LAYOUT_LEGACY`.

```c
cui_chart_options options = {
    .kind = CUI_CHART_LINE,
    .data = points, .count = point_count,
    .width = 320, .height = 180,
    .layout_mode = CUI_CHART_LAYOUT_SOURCE_POINT,
    .interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME,
};
cui_chart(ui, chart_id, &chart_state, &options, cui_string("Revenue"));
```

Source mode reserves 18 raster pixels for the x axis, places the x-label top at the axis plus 6 pixels, and uses `max(point_count, count)` point slots. A zero point count therefore uses the data length; a smaller requested count also uses the data length. `hide_x_axis` removes the gutter. The independent legacy `hide_axes` switch still suppresses composition labels without changing raster geometry. The shared query `cui_chart_point_axis_layout` reports the effective slots and gutter without reading or retaining data arrays; it checks kind, enum, count (1–4096), and point-count capacity (at most 1,000,000), leaving both outputs untouched on failure. Output storage must be disjoint.

Raster point positions, hit selection, grid/tick domain positions, measured y-axis gutter, hover crosshair endpoint, x-label placement, and source cache fingerprints consume this geometry. Raster pixels are scaled into `logical_width`/`logical_height` when supplied; label typography continues to use the existing 10-unit host font contract. Arbitrary custom label content is not introduced.

Legacy mode keeps the 20-pixel gutter and existing label offsets. Render and point-coordinate APIs continue to reject a nonzero point count smaller than the data length. The historically permissive raw legacy hit-test geometry remains unchanged. Source mode is rejected for other chart kinds and invalid enums are rejected before styled UI/state changes. Base render validation continues to preserve raster/state on invalid options. Source cache keys use effective slot counts, so equivalent normalized requests reuse a frame; legacy zero-mode cache keys retain their existing structure.

This slice does not certify complete Chart parity. Typed source tooltip composition, source popover styling/shadows, custom Radar-title suppression, and accepted mobile long-press phase/prevention remain separate gaps. Legend composition belongs to the source story rather than a Chart legend API; this mode adds no keyboard or legend API.

Validation: `tests/unit/test_chart_source_point_axis.c` covers coordinates, normalized selection/cache reuse, source versus legacy labels/crosshair endpoints, invalid output atomicity, and raw legacy hit compatibility. `tests/web/test_chart_source_point_axis.py` builds pinned Zig 0.16.0 WASM and verifies actual Chromium canvas raster pixels for source/legacy and expanded slots. Register the new unit executable against `cui` in the integration CMake tree; this worker does not edit global registrations or regenerate distributions. Browser proof concerns raster geometry, not native accessibility certification.
