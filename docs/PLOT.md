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
styled layer does not yet implement arbitrary gradient backgrounds, renderer-native
mesh caches, implicit appear scope tracking or the PlotElement renderer prepaint
lifecycle. Caller-owned software segment caches, exit clocks and tooltip content
composition are available; current bounds/pointer/occlusion remain host supplied. Chart-specific versions of some features do not automatically
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

## Tooltip and appearance state

Use the pure caller-owned lifecycle helpers described in
[PLOT_MODEL.md](PLOT_MODEL.md#caller-owned-hover-and-appearance-lifecycle) to retain
a selected datum during exit. Keep its borrowed dot array alive until
`has_state` becomes false, and paint the tooltip only when `has_state` is true.
The bitmap view does not populate this state automatically. Feed progress from
the application's existing animation policy and request another frame with
`cui_invalidate` while the clock is moving. Honor `cui_reduced_motion` by snapping
to the target, and suppress hover overlays while appearance is incomplete if
you want the source PlotElement ordering. Completed appearance generations and
linear stagger timing are reusable independently of drawing; the application
owns scope disposal, easing, pointer springs, and the tooltip's content/layout.

For a bitmap or custom path renderer, use `cui_plot_bar_path` instead of submitting
the styled quad. It emits a closed cubic path with independent physical corner
radii, consumed directly by `cui_plot_raster`. See
[PLOT_MODEL.md](PLOT_MODEL.md#rounded-bar-paths-for-custom-renderers) for its
capacity contract and explicit cubic approximation limits. The styled driver
continues to use Core rounded quads; it does not route through this rasterizer.

For source-style Pie/Arc seams, choose `CUI_PLOT_ARC_SOURCE_PADDING` with
`cui_plot_arc_flatten_ex` before rasterizing. It reproduces the source's .0001
radian override for spans at least π and its radius/span epsilon rules. Existing
flatten calls retain seamless full turns. See the
[Arc policy contract](PLOT_MODEL.md#explicit-frozen-arc-padding-policy); hover hit
testing still ignores visual padding, matching the frozen source.

## General labels and measured ellipsis

`cui_plot_labels_layout` measures a caller-owned array of `cui_plot_label` using
Core's configured text measurement. Origins are relative to the plot surface;
START leaves x unchanged, CENTER subtracts half the measured width, END subtracts
its full width. Font size zero selects the source's 10px default. Text is a
single line, and y is its Core top origin. `cui_plot_labels` submits the collection
in a fixed surface with the same geometry. Supply explicit colors: transparent
labels stay transparent rather than falling back to theme muted. The frozen
Text font_weight builder stores a field that its painter ignores; this API uses
normal text rather than claiming weighted source rendering. Shaping/baseline
metrics follow the configured CUI font backend, not identical GPUI ShapedLine.

Arrays are bounded to 256 labels, text to 4096 bytes each, font size to 1024px,
origins/extents/label dimensions to 16384px. Layout errors preserve the output
rectangles; insufficient capacity reports the count. Drawing preflights the
whole collection before submitting any boxes. Keep label text alive through
renderer completion. The label collection does not add retained state, automatic gutter allocation
or a semantic chart model; tooltip content composition is a separate driver.

`cui_plot_truncate_text` copies into caller-owned NUL-terminated storage and
reports required bytes including that terminator. It follows the source's
binary search over Unicode scalar boundaries for the longest measured prefix
plus `…`. A nonpositive width budget keeps the original text. When no prefix
fits, it returns the ellipsis alone even if that also exceeds the budget. Input
must be valid UTF-8 and at most 4096 bytes; size zero defaults to 10px. Measurement
must return finite nonnegative dimensions no greater than 16777216px. This does
not split UTF-8 byte sequences, but may split a combining or emoji grapheme,
as does the source. No locale, wrapping or automatic truncation is implied.
Invalid inputs preserve output and required; insufficient capacity preserves
output and sets required. Input/output may alias. If storage is used as a label,
keep it stable through renderer completion.

```c
#include <assert.h>
#include "cui/cui_plot_view.h"
static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){(float)text.length * size / 2, size};
}
int main(void) {
    /* Configure the application's real font callbacks for shaped text. */
    cui_context *ui = cui_create((cui_config){.width=100, .height=60, .measure=measure});
    assert(ui);
    char text[32];
    size_t required;
    assert(cui_plot_truncate_text(ui,(cui_str){"Revenue",7},10,30,
                                  text,sizeof text,&required)==CUI_PLOT_OK);
    cui_plot_label label={.text={text,required-1}, .origin={50,10},
                          .align=CUI_ALIGN_CENTER, .color={30,30,30,255}};
    assert(cui_begin_frame(ui,100,60,.016f));
    assert(cui_plot_labels(ui,123,(cui_size){100,60},&label,1)==CUI_PLOT_OK);
    cui_draw_list draw=cui_end_frame(ui);
    assert(draw.count>0);
    /* Render draw before releasing text or destroying ui. */
    cui_destroy(ui);
    return 0;
}
```

The pure [tooltip cursor gate](PLOT_MODEL.md#tooltip-cursor-gating-from-plotelement)
handles current-bounds desktop coordinates and explicit long-press state before
your datum resolver runs. Refresh desktop state each frame using the application's
hit/occlusion decision; use `update.changed` to invalidate from mouse-move events.
Long-press detection and capture stay in the adapter/application. Resolve the
live cursor, suppress during appearance, then update hover memory; this does not
automatically make the bitmap view interactive or render an overlay.

Use the pure `cui_plot_sankey_link_path` to draw a tapered ribbon from existing
Chart node/link geometry through `cui_plot_raster`. It preserves independent end
widths, minimum painted width and source midpoint-X cubic controls. See the
[source ribbon contract](PLOT_MODEL.md#source-sankey-ribbon-paths); this adds a
path builder without a second graph/layout solver or renderer cache.

The [shared Base Sankey graph model](PLOT_MODEL.md#shared-base-sankey-graph-model)
now supplies topology, alignment and relayout independently of Chart. Use
source-default options for standalone Plot; the Chart adapter preserves its
own defaults, raw throughput reporting and legacy zero-flow alignment. The graph and tapered
ribbon path are caller-owned math outputs, not an automatic styled Sankey widget.

## Frozen Axis offsets and explicit colors

`cui_plot_axis_source_defaults` selects the frozen base Axis contract without
changing existing styled calls. It sets `source_offsets` and `explicit_colors`.
X labels sit at axis+6px on End or axis-14px on Start; Y labels sit at axis±2px
and tick-5px. These offsets stay fixed even for 20px or 30px fonts, exactly as the
source derives them from its 10px constants. Measured text alignment still
subtracts half/full width for CENTER/END, and each label retains its selected
font size. The source route submits a single normal line through Core; shaping
and baseline metrics remain backend-specific rather than identical GPUI pixels.

Explicit colors preserve alpha zero for labels and axis strokes; supply the
actual solid colors you want. Alpha zero no longer chooses theme muted/border.
With legacy `cui_plot_axis_defaults`, offsets continue to scale the Start gap
and Y centering with font size, and zero-alpha colors continue to use theme
fallback. The two appended booleans can be selected independently. Axis absence
and line visibility still gate line/labels independently as before; no automatic
gutters, tick formatter or placement policy is introduced.

`cui_plot_grid_ex(...,stroke,true)` likewise preserves its supplied color,
including transparent, matching the source Grid stroke behavior. Pass false or
use the unchanged `cui_plot_grid` for alpha-zero theme fallback. Dash geometry
and its existing bounded-output/error contract are shared by both paint routes.
General gradient/Background fills and renderer path caching remain separate
unsupported features; these options implement actual solid-color behavior.

For a virtualized list of plots, the
[bounded appearance scope registry](PLOT_MODEL.md#bounded-appearance-scope-registry)
remembers multiple completed generations while its enclosing scope is painted
consecutively. Begin/end it explicitly each host frame and pass the innermost
registry into child composition. Returning completed children can skip draw-in;
interrupted children restart. No retained list/plot tree or implicit scheduler
is introduced by this numeric state helper.

For enter/exit animation, sample a separate `cui_plot_transition_clock` per
channel with `cui_plot_transition_step`, feed its value to
`cui_plot_hover_track`, and schedule a frame when `request_frame` is true.
Initialize the channel at zero before its first hover target of one, since the
source transition adopts its first target immediately. Signed delays, cubic-out
(default) or synchronous custom easing, reversal shortening and reduced-motion
snapping are supported. This host-driven clock does not add renderer keyed state. See [projected transition clocks](PLOT_MODEL.md#projected-transition-clocks)
for exact policy-change and ownership rules.

Pointer glide uses `cui_plot_spring_step` with caller-owned scalar state, explicit
seconds and a policy from `cui_plot_spring_defaults`. Pass hover memory's
`entering` flag to snap on entry; later target changes preserve momentum and
advance toward the previous target before retargeting. Schedule while
`request_frame` is true. Use independent states for each coordinate. This is an
explicit host seam, not automatic scoped renderer state or theme projection.

The gallery shape selector exposes linear, natural cubic, step-after, area, radial, rounded-bar and dot previews. All use the same projected samples, scale and native C geometry; bars/dots sample every eighth point to bound software preview work. Zoom/pan resamples the function through the current scale. These previews do not imply automatic shape hit testing, animation clocks or renderer path caching.

The software path route can retain caller-owned `cui_plot_path_cache` storage
across frames. `cui_plot_path_cache_get` rebuilds and flattens only on a changed
shape key; `cui_plot_raster_cached` accepts its borrowed relative segments plus
current origin and colors. Origin/color changes reuse tessellation, and empty
paths are cached. Pixel rasterization still runs; this does not add a Core native
mesh cache or implicit element cache lifetime. See
[cached tessellation](PLOT_MODEL.md#shape-keys-and-cached-tessellation) for bounds,
key composition and view lifetimes.

## Styled tooltip composition

`cui_plot_element` composes the existing view plus datum resolution, marks,
ordinary children and tooltip content. It uses caller-owned hover/transition
state and Core frame time/reduced motion. Supply current global layout bounds,
live pointer position and an occlusion-aware hover flag; previous `cui_bounds`
cannot provide those current bounds automatically. Tooltip content runs last
inside an inert plot-sized portal with inherited hover opacity and may position
its own body relative to that portal. The selected datum/cursor survives exit
until progress reaches zero. The driver invalidates while its clock requests
frames. See [the exact host seam](PLOT_MODEL.md#styled-plotelement-composition)
for touch gates, callback ordering, ownership and renderer limitations.

This complete example intentionally knows its absolute plot placement. A host
with flowing/scrolling layout must provide that frame's actual placement instead.

```c
#include "cui/cui_plot_view.h"
#include <assert.h>
static cui_plot_result datum_at(cui_plot_point p, cui_plot_rect bounds,
                                cui_plot_tooltip_state *out, void *user) {
    (void)bounds; (void)user;
    *out = (cui_plot_tooltip_state){.index = p.x < 32 ? 0 : 1, .cross_line = p};
    return CUI_PLOT_OK;
}
static void tooltip_body(cui_context *ui, cui_id id, const cui_theme *theme,
                          const cui_plot_hover_memory *hover,
                          cui_plot_rect bounds, void *user) {
    (void)bounds; (void)user;
    cui_begin_absolute_box(ui, id + 10, (cui_box){.background = theme->surface},
        (cui_rect){(float)hover->cursor.x + 4, (float)hover->cursor.y + 4, 40, 20});
    cui_text(ui, cui_string("datum"), 10, theme->text);
    cui_end_box(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 100, .height = 100});
    assert(ui);
    static unsigned char pixels[64 * 64 * 4];
    cui_plot_view_options view = {.bitmap = {pixels, 64, 64, 256, 1}, .interactive = true};
    cui_plot_element_state state = {0};
    cui_plot_element_options element = {
        .bounds = {10, 15, 74, 79}, .pointer = {30, 35}, .hovered = true,
        .enter = {.duration = .12}, .exit = {.duration = .16},
        .resolve = datum_at, .tooltip = tooltip_body
    };
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 100, 100, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 100, .height = 100,
                                 .fixed_width = true, .fixed_height = true});
    cui_begin_absolute_box(ui, 2, (cui_box){0}, (cui_rect){10, 15, 64, 64});
    assert(cui_plot_element(ui, 3, &theme, &view, &state, &element) == CUI_PLOT_OK);
    cui_end_box(ui); cui_end_box(ui);
    (void)cui_end_frame(ui); /* render this list before the next begin_frame */
    cui_destroy(ui);
    return 0;
}
```

## First-paint appearance

Set `element.has_appearance=true`, supply `generation`, `frame_serial` and the
`appear` transition policy, and retain the same `cui_plot_element_state` across
frames. Increment the serial for every host frame, including omitted plots.
The first paint starts at linear time zero; zero duration/reduced motion is
complete immediately. `on_appear` runs before datum resolution and drawing,
allowing caller-owned marks or bitmap data to use its eased progress. The driver
does not choose a reveal style. Use `cui_plot_appear_value_stagger` on the sample
for staggered marks: local linear time is eased independently for each mark.
The callback sample pointer is borrowed for that call; payloads obey the normal
renderer lifetimes and bitmap revision rules.

Owner rekey, a frame gap or a new generation restarts the appearance. Pass an
active innermost `appear_scope` registry begun with the same frame serial to
remember completed generations across omitted plots. Begin/end that scope even
on frames without the plot. Unfinished appearances replay after omission, and
scope rekey/gaps forget completions. Appearance still runs for passive/disabled
plots and keeps ordinary children composed; hover is suppressed until linear
time reaches one. Default policy is Base's zero duration, with product timing
supplied by the host. See [channel binding](PLOT_MODEL.md#first-paint-appearance-channel-binding)
for capacity failures and exact lifetime rules.
