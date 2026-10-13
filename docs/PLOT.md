# Styled Plot composition

[`cui_plot_view.h`](../include/cui/cui_plot_view.h) adds immediate axis, bitmap
and path rendering composition above the standalone
[`cui_plot.h`](../include/cui/cui_plot.h) math module. Applications own scales,
samples, commands, RGBA pixels, labels and callback state. Core retains layout
regions and pointer capture; this layer has no retained series/widget tree,
implicit allocations or platform types. See [PLOT_MODEL.md](PLOT_MODEL.md) for
geometry, scales, stacking, arc and radial contracts.

The axis behavior was audited against frozen GPUI
`crates/base/src/plot/axis.rs` and `label.rs` at commit
`87d10ae5e1299d1be18670a37570c5402f952e34`. The C implementation is original.
The rasterizer below is a bounded software implementation rather than a claim
that GPUI's renderer/path cache was ported.

## Axes and numeric ticks

`cui_plot_axis_defaults(width,height)` starts with the X line enabled, Y line
hidden, and both axes absent. Set `has_x`/`has_y` and their positions relative
to the top/left. An absent axis suppresses its line and labels. `x_line` and
`y_line` hide only the line, preserving labels, matching GPUI PlotAxis.

Supply `cui_plot_axis_text` arrays with borrowed `cui_str` text, pixel tick,
font size and alignment. Zero font size selects the source default 10px.
START/CENTER/END alignment means left/center/right anchored about the label's
X coordinate. Alpha-zero label colors use theme `muted`; alpha-zero axis stroke
uses theme `border`. There is no way to request a transparent fallback color:
hide the line or omit the label explicitly.

At default 10px the source positions are preserved: X END labels start 6px
below the axis; X START labels start 14px above. Y END/START label anchors are
2px right/left of the line, with vertical position tick minus 5px. For larger
fonts, the C layer deliberately uses `-(font_size+4)` above X and
`tick-font_size/2` beside Y instead of the frozen source's fixed 10px offsets.
`cui_plot_axis_gutter(font_size)` returns font size plus 8px, matching the source
helper. It returns zero for invalid/nonpositive sizes. Gutters are advisory;
applications reserve their own plot rectangle.

`cui_plot_axis_layout` returns measured rectangles and original label indices
without submitting UI elements. It uses Core's configured font measurement.
Each axis is bounded to 256 labels. Inputs must be finite: positive surface
extent up to 16384, positions/ticks within +/-16384, font sizes up to 1024.
Label geometry and required count remain unchanged on INVALID; CAPACITY
publishes the required label count without partial geometry. Buffers must be
stable and disjoint during the call. `cui_plot_axis` submits the same rectangles
and centered 1px axis lines inside a fixed-size surface. Labels can extend outside that
surface; ancestor clipping still applies. This allows explicit gutters rather
than silently truncating labels at the axis edge.

`cui_plot_axis_ticks` combines linear/log scale ticks and mapping with caller
`cui_plot_numeric_tick` storage. It returns `%.17g` numeric text, values and
center-aligned labels. These structs contain pointers to their own text arrays:
keep them at their initialized addresses and copy only their `label` members
into X/Y label arrays. Keep tick structs and label arrays live through
`cui_end_frame` (and any renderer use of the draw list). Desired count is 2..256
and advisory; actual count may differ. Capacity queries return the actual count.
No locale, automatic category labels, formatter callback, collision avoidance,
rotated labels, grid lines or tick marks are supplied.

## Caller-owned software output

`cui_plot_raster` clears and writes a straight-alpha RGBA8 buffer. The caller
passes width, height, row stride and available byte capacity; at least
`stride * height` bytes are required. Padding bytes after each row are preserved.
Each `cui_plot_shape` points to pure Plot path commands with explicit fill,
stroke color and stroke width. Use theme colors when constructing shapes.
Alpha-zero fill/stroke disables that operation. Later shapes blend over earlier
ones with straight-alpha source-over composition.

Move/Line/Cubic/Close commands are supported. Cubics use 16 uniform parameter
chords; this is an explicit approximation without adaptive error control.
ARC commands are rejected: call `cui_plot_arc_flatten` first. Translate relative
arc/radial points to the desired center before rasterization. Bars are closed
rectangular paths produced from projected bounds; line/area commands come from
`cui_plot_path`. No extra Chart dependency is introduced.

Fill uses the even-odd rule across explicitly closed subpaths. Open runs receive
only strokes. Stroke segments have round ends/joins through their union, with
no dash/miter/cap style configuration. Pixel centers determine coverage: there
is no anti-aliasing, gradient, texture, shadow or color-space conversion. Bitmap
edges clip output. Defined coordinates/control points are finite within
+/-16384; stroke width is 0..1024. Surfaces are 1..1024 per dimension, at most 64
shapes, at most 4096 commands and flattened segments per shape. The total
`width * height * segment_count` work is capped at 64 million. Capacity failures
(buffer size, segment expansion, workload) and invalid geometry leave **all
pixels unchanged**, including when a later shape is invalid. Shape arrays,
commands and pixel storage must be disjoint and stable for the entire call.
The implementation uses a fixed segment scratch array, approximately 160 KiB
on typical 64-bit hosts, and no allocation. This favors bounded portable output
and small headless plots; a GPU/native path adapter can consume the same math
commands independently.

## Bitmap composition and interaction

`cui_plot_view` submits a borrowed `cui_bitmap`, then optional themed axes on top,
in a fixed-size box equal to the bitmap's pixel dimensions. Axis dimensions
must equal the bitmap dimensions. Any valid caller-created bitmap is accepted,
including higher-quality output from another renderer (up to 16384 per axis).
The bitmap's pixels must remain valid through renderer completion. Increment
its revision whenever pixels change; the view does not modify it. Labels and
their text must remain live through draw-list consumption. Invalid options
return before submitting elements. Core frame-capacity errors follow normal
Core behavior and cannot roll back an already submitted frame; check
`cui_last_error` after `cui_end_frame`.

The image semantic uses the supplied accessible `name`. Set `interactive=true`
to opt into primary-pointer activation. A validated release on the same view
invokes `on_activate` synchronously once per event with bitmap coordinates,
relative to the previous frame's view bounds. Release outside cancels capture;
secondary clicks, disabled views and passive views do not activate. The handler
can update application state for the next frame. Pointer activation does not
opt into focus or keyboard activation. The component does not pan, zoom,
select a datum, create a tooltip or retain a hover model automatically. Use
pure `cui_plot_inverse`, clipping/nearest/arc queries and application-owned
state to implement those interactions. Scales must be updated explicitly.

This complete example builds a themed line, numeric X labels and a view, then
consumes its bitmap with the software renderer:

```c
#include <assert.h>
#include "cui/cui_plot_view.h"
#include "cui/cui_software.h"
int main(int argc, char **argv) {
    assert(argc == 2); /* Pass a readable font file path. */
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    unsigned char pixels[96 * 96 * 4];
    cui_theme theme = cui_theme_light();
    cui_plot_sample points[] = {{8,0,70,true},{40,0,20,true},{80,0,50,true}};
    cui_plot_path_command commands[32];
    size_t count = 0;
    assert(cui_plot_path(points,3,CUI_PLOT_CURVE_NATURAL,CUI_PLOT_GAPS_SKIP,
                         false,commands,32,&count) == CUI_PLOT_OK);
    cui_plot_shape line = {.commands=commands,.count=count,
                           .stroke=theme.primary,.stroke_width=2};
    assert(cui_plot_raster(pixels,96,96,96*4,sizeof pixels,&line,1,
                           theme.surface) == CUI_PLOT_OK);
    cui_plot_scale scale = {CUI_PLOT_LINEAR,0,10,8,80,false};
    cui_plot_numeric_tick ticks[16];
    assert(cui_plot_axis_ticks(&scale,6,ticks,16,&count) == CUI_PLOT_OK);
    cui_plot_axis_text labels[16];
    for(size_t i=0;i<count;++i) labels[i]=ticks[i].label;
    cui_plot_axis_options axis = cui_plot_axis_defaults(96,96);
    axis.has_x=true; axis.x=72; axis.x_labels=labels; axis.x_count=count;
    cui_plot_view_options view = {.bitmap={pixels,96,96,96*4,1},.axis=&axis,
                                  .name={"Trend",5}};
    cui_context *ui = cui_create((cui_config){.width=128,.height=128,
        .measure=cui_font_measure,.measure_styled=cui_font_measure_styled,
        .measure_user=font,.font_metrics=cui_font_get_metrics});
    cui_software *renderer = cui_software_create(128,128,font);
    assert(ui && renderer);
    assert(cui_begin_frame(ui,128,128,.016f));
    assert(cui_plot_view(ui,44,&theme,&view) == CUI_PLOT_OK);
    cui_draw_list draws = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_software_render(renderer,&draws,theme.background));
    cui_software_destroy(renderer);
    cui_destroy(ui);
    cui_font_close(font);
    return 0;
}
```

With the native text/software adapters enabled, compile using their CMake
library outputs, for example:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude plot_view_example.c src/components/cui_plot.c build/libcui_software.a build/libcui_text.a build/libcui.a $(pkg-config --libs harfbuzz freetype2) -lm -o plot_view_example
./plot_view_example /path/to/readable-font.ttf
```

The explicit component source works before the integrator adds it to the library;
normal consumers can link the integrated CUI target instead. A geometry-only
software renderer without a font accepts bitmap/rectangle output but reports
failure on text commands; supply a font when rendering axis labels. The example
keeps all borrowed buffers and its font alive until rendering completes.

Tests check axis defaults, side/alignment geometry, themed text/line commands,
reversed-range numeric ticks, line/area/bar/pie and cubic pixels, stride padding,
transactional failures, software bitmap output, and captured pointer behavior.
This is a reusable styled Plot foundation; it does not establish complete
GPUI Plot rendering or interaction parity.


## Themed dashed grids

`cui_plot_grid` paints caller-specified `cui_plot_grid_options` from the pure
module as fixed-size themed quads. Alpha-zero stroke selects theme `border`.
Its preflight uses `cui_plot_grid_layout`; errors submit no elements. Geometry,
dash parity, clipping and budget rules are documented in [PLOT_MODEL.md](PLOT_MODEL.md).
The view does not implicitly create or retain a grid. Submit a grid in an
application overlay/absolute box before the bitmap if it should appear behind
transparent shapes; alternatively use the pure quads to create closed paths
for `cui_plot_raster`. Core element/command budgets still apply to a large styled
grid; check `cui_last_error` after `cui_end_frame`. The 1px quads are centered on
coordinates, matching frozen Grid and the corrected styled axis stroke placement.
Software tests verify both dash pixels and gaps using pixel-center coverage.

## Source Area rendering and remaining Plot scope

For frozen-style areas use `cui_plot_area_paths` rather than applying both fill
and stroke to a single closed generic area path. Submit its fill contour with
fill only, then its upper stroke contour with stroke only (1px reproduces the
source default). Software tests verify that the closing baseline stays filled
without being stroked. The builder also preserves independent X/Y missingness,
original endpoint closure and the singleton early return. Open contours need an
adapter's implicit-fill policy or an explicit Close appended by the caller;
the bounded rasterizer intentionally fills only explicitly closed runs.

The [complete frozen export audit](PLOT_MODEL.md#complete-frozen-plot-export-audit)
lists exact supported routes and remaining differences. In particular this
styled layer does not yet implement arbitrary gradient backgrounds or label-content callbacks,
PlotLabel truncation/content collections, tessellation caches, hover/tooltip
state and exit motion, appear-generation/scope tracking, or the PlotElement
prepaint lifecycle. Chart-specific versions of some features do not automatically
satisfy standalone Plot parity. Axis default positions and centered strokes are
covered, while nondefault font scaling and theme fallback are deliberate CUI
policies rather than byte-for-byte source behavior.


## Dot and rounded-bar composition

`cui_plot_dots` accepts Cartesian samples (`x`/`y1`); `cui_plot_radial_dots`
accepts radial samples and centers them at half the supplied surface extent,
matching frozen RadialLine. Both produce source-style circular quads with a
fixed 1px border, skipping undefined samples. Cartesian `y0` and radial `radius0`
are ignored. `cui_plot_dot_defaults()` selects size 4 and theme `primary` fill.
This is a styled default: frozen base dots use transparent fill. Set
`use_theme_fill=false` for an explicit solid fill, including transparency.
Without `custom_stroke`, the border uses the resolved fill color, matching the
source's solid-fill fallback. With `custom_stroke=true`, the explicit stroke
color is used unchanged, even if transparent. `size=0` remains zero rather than
selecting a default. Negative radius/size is rejected instead of allowing the
source's unchecked geometry.

`cui_plot_bars` accepts materialized `cui_plot_bar_sample` data, orientation,
constant band width and per-corner radii. Samples missing cross or value skip
both geometry and the fill callback; their numeric fields are ignored. The
source-relative frame comes from `cui_plot_bar_layout`. `cui_plot_bar_defaults()`
selects Bottom, zero band width and theme `primary` fill. Frozen base instead
defaults to black. Set `use_theme_fill=false` for explicit color; if `fill_for`
is supplied its result overrides either default, including a transparent result.
The callback receives the original sample pointer, original index, painted frame,
and alignment, synchronously once per eligible sample—even a degenerate bar.
It may update separate application bookkeeping, but must not submit UI or mutate
borrowed samples/options. The callback runs during component composition, never
inside software/native renderer execution. A late invalid sample prevents all
callbacks and UI submission in that call.

Per-corner radii map to Core's `cui_corner_radii` and existing rounded rectangles.
The adapters clamp each corner to half the shorter side. Explicit zero corners
stay square. This supports rounded bar tips in every orientation without another
rasterizer. It is not a claim of byte-identical GPUI corner normalization. Solid
color fills are supported; arbitrary GPUI Background/gradients and a generic
bar label-content callback are not. `cui_plot_bar_geometry.label_origin` remains
available for application label composition.

Each driver creates a fixed-size decorative surface without datum semantics,
hit testing or implicit event callbacks. Overlay it in an application absolute
box when marks should share a bitmap/axis view; the bitmap view can own pointer
activation. Surfaces are positive finite up to 16384 per dimension, at most 4096
samples, dot sizes 0..1024, corner radii 0..1024, and projected Cartesian/bar
coordinates within +/-16384. Bars have nonnegative band width up to 16384 and
must keep their frame endpoints in that coordinate range. Radial values are
finite/nonnegative and projected centers must fit that range. Geometry preflight
errors submit nothing. Core element/command capacity and clipping still apply;
check `cui_last_error` after ending the frame. Caller arrays stay stable during
composition. The drivers allocate no private buffers or retained widget state.

This complete geometry-only software example overlays outlined dots on rounded
bar tips without a font dependency:

```c
#include <assert.h>
#include "cui/cui_plot_view.h"
#include "cui/cui_software.h"
int main(void) {
    cui_context *ui=cui_create((cui_config){.width=80,.height=80});
    cui_software *renderer=cui_software_create(80,80,NULL);
    assert(ui && renderer);
    cui_theme theme=cui_theme_light();
    cui_plot_bar_sample bars[]={{8,48,12,true,true},{36,48,28,true,true}};
    cui_plot_bar_options style=cui_plot_bar_defaults();
    style.band_width=20; style.corners=(cui_corner_radii){6,6,0,0};
    cui_plot_sample points[]={{18,0,12,true},{46,0,28,true}};
    cui_plot_dot_options dots=cui_plot_dot_defaults();
    dots.size=8; dots.use_theme_fill=false; dots.fill=theme.surface;
    dots.custom_stroke=true; dots.stroke=theme.primary;
    assert(cui_begin_frame(ui,80,80,.016f));
    cui_begin_box(ui,1,(cui_box){.width=64,.height=64,.fixed_width=true,.fixed_height=true});
    cui_begin_absolute_box(ui,2,(cui_box){0},(cui_rect){0,0,64,64});
    assert(cui_plot_bars(ui,3,&theme,(cui_size){64,64},bars,2,&style)==CUI_PLOT_OK);
    cui_end_box(ui);
    cui_begin_absolute_box(ui,4,(cui_box){0},(cui_rect){0,0,64,64});
    assert(cui_plot_dots(ui,5,&theme,(cui_size){64,64},points,2,dots)==CUI_PLOT_OK);
    cui_end_box(ui); cui_end_box(ui);
    cui_draw_list draw=cui_end_frame(ui);
    assert(cui_last_error(ui)==CUI_OK);
    assert(cui_software_render(renderer,&draw,theme.background));
    cui_software_destroy(renderer); cui_destroy(ui); return 0;
}
```

For a geometry-only build, compile this example with `src/components/cui_plot.c`,
`backends/software/cui_software.c`, `build/libcui.a` and `-lm`, using the same strict
C11 flags above. Tests cover circular corner exclusion, custom/fallback border
pixels, theme fills, radial centers, independent rounded/square bar corners,
renderer clamping, fill callback inputs, missing/degenerate samples and invalid
preflight suppression.
