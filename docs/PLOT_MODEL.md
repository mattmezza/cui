# Headless plot scales and geometry

`cui_plot.h` is an original allocation-free C11 math module. It has no core,
style, font, platform, or GPU dependency. It implements scale/viewport behavior
and projected geometry, not a Chart or Plot widget. The pinned GPUI Base linear, categorical
scale and bar modules were reviewed as functional references; this code does
not copy their Rust implementation.

A `cui_plot_scale` has a strictly ascending data domain, a pixel range that can
ascend or descend, a linear or base-10 logarithmic transform, and an optional
clamp flag. Mapping and inverse mapping reject nonfinite inputs. Log domains
and values must be positive. Inverted domains are invalid; equal domain/range
endpoints are degenerate. Finite ascending log endpoints that become identical
at the platform's transform precision are also rejected as degenerate.
Outputs and mutable scales remain unchanged on errors.

```c
cui_plot_scale y = { CUI_PLOT_LINEAR, 3, 97, 400, 20, false };
cui_plot_nice(&y, 6);            /* Domain expands to 0..100. */
double pixel;
cui_plot_map(&y, 25, &pixel);    /* 305, with larger values higher. */
cui_plot_zoom(&y, 2, pixel);     /* Keeps the value 25 at pixel 305. */
cui_plot_pan(&y, 10);           /* Plotted data moves 10 pixels down. */
```

`cui_plot_ticks` chooses ascending 1/2/5 steps in linear domains or transformed
base-10 exponent space. Desired count is advisory and must be 2–1000. Ticks
are finite, unique, and inside the current domain. Subnormal linear steps are
bounded by the smallest positive double; steps near large domain offsets are
bounded by representable spacing to prevent duplicate ticks. Log ticks can
use fractional powers of ten. Nice domains round outward in transformed
space, saturating at finite double limits while retaining original endpoints.
Nice-step threshold decisions allow a few long-double arithmetic ulps so a
libm rounding difference does not promote an exact unit step to two. Tick
generation inspects neighboring transformed grid indices and applies strict
final comparisons in the original double domain. Exact power-of-ten endpoints
remain inclusive; excluded endpoints are not pulled into range by a broad
relative epsilon or endpoint snapping.

Tick generation uses bounded temporary storage for at most 1024 values and
never allocates. Pass `NULL, 0` to query required capacity. Insufficient capacity
returns `CUI_PLOT_CAPACITY` and reports the required count, without writing a
partial result. A sufficiently sized output buffer receives the entire result.
A narrow domain can contain no representable chosen ticks, yielding count zero.

Pan shifts the transformed domain by a pixel delta. Zoom divides its span by
a positive factor around the data coordinate at the supplied pixel anchor.
A factor greater than one zooms in. Reversed ranges work with both operations.
Operations reject overflow, underflow to a nonpositive log domain, and zoom
levels that collapse the representable domain. Factor one and zero pan are
exact no-ops. Zoom anchors outside the pixel range use extrapolated coordinates,
even when subsequent mapping is clamped.

Long-double intermediates reduce cancellation where the compiler supports
extra precision. Scaled fractions also handle finite domains whose width
exceeds `DBL_MAX` on platforms where long double equals double. This does not
make doubles exact: narrow domains near enormous offsets have fewer meaningful
positions. Numerical failures are explicit; no NaN/Infinity sentinel is returned.

`cui_plot_clip_line` clips a segment to a finite ordered viewport; projection
helpers map and clip a data segment or a vertical bar with an explicit baseline
and pixel thickness. Reversed vertical ranges and negative-valued linear bars
are supported. Logarithmic bars require a positive value and baseline. Invisible
segments or zero-area clipped bars return `CUI_PLOT_EMPTY` without changing
output. Scale clamping applies before geometry clipping; use unclamped scales
when off-domain geometry should intersect the viewport naturally.

Pure path commands, stacking and pie/arc queries are described below. No axis
labels, retained series storage, hover composition, legends, accessibility chart
description or animated transitions are supplied by the math module.
The optional [styled Plot layer](PLOT.md) adds themed axes and bounded software
path/bitmap composition without changing this standalone module.

`test_plot` checks 1500 deterministic forward/inverse and zoom-anchor cases,
linear/log ticks, reversed ranges, transactional failures, subnormal domains,
finite extreme domains/ranges, clipping, projected lines, and bar baselines.

## Browser demonstration

The Plot gallery displays `sin(x / 12)` rather than only reporting scale metadata. Each frame samples 65 values using `cui_plot_inverse` and paints them with the existing Chart renderer. Zoom in/out change the domain around the viewport center; Pan left/right shift it; Reset restores 0–100. The domain readout and nice-tick count update with the retained scale.

This demonstrates composing the numerical Plot API with a renderer. It does not introduce a separate public Plot widget or mouse gesture API. The scale is application-owned retained state; sampling and drawing are immediate. See `web/gallery.c` for the composed example.


## Categorical scales

Band, Point and Ordinal were audited against the frozen GPUI Base
`plot/scale/{band,point,ordinal}.rs` at commit
`87d10ae5e1299d1be18670a37570c5402f952e34`. The C implementation is original;
it uses application-defined `uint64_t` category keys instead of Rust generic
values. Intern string labels or other application values into stable keys;
the application owns collision handling and label formatting. Key zero is valid.
All three models remain application data and submit no rendering or semantics.
Existing linear/logarithmic APIs retain their previous behavior.

| Model | Domain ownership and duplicates | Output |
| --- | --- | --- |
| `cui_plot_band_scale` | Copies first-occurrence unique keys into caller storage | Band start, width, step, nearest slot |
| `cui_plot_point_scale` | Borrows the entire domain, including duplicates | Evenly spaced positions, nearest entry |
| `cui_plot_ordinal_scale` | Borrows domain and typed range; first duplicate wins lookup | A copy of a cycling range element or explicit unknown |

Domains/ranges must remain alive while their models are used; scale structs and
output arrays must also outlive their consumers. Band initialization uses a
bounded 32 KiB scratch array so overlapping input and destination storage are
safe and capacity failure cannot partially rewrite either storage or the model.
Its deduplication is quadratic, bounded to 4,096 entries. Key lookup is linear;
Point `at` and nearest calculations are constant time. These are deliberately
bounded allocation-free models, not retained hash maps or implicit insertion.
Point and Ordinal domain arrays may be replaced by calling their initializer
again; failed initialization preserves the previous binding.

### Band geometry

`cui_plot_band_init` reports the required **distinct** key count on
`CUI_PLOT_CAPACITY`. Other initialization failures leave `required` untouched.
A NULL storage pointer with capacity zero performs a capacity query (an empty
domain needs zero storage and initializes successfully). All other scale and
geometry outputs remain untouched on failures. Keep `required`, the scale
struct and destination storage disjoint; the input key array may alias storage.

The effective band count is the larger of distinct domain count and
`options.band_count`; extra bands remain empty. Unlike Point or Linear,
reversing Band's range does **not** reverse category order: Band always starts
at the lower endpoint, matching the frozen source. If span is `R` and effective
count is `N`, the source-specific policy is:

- Base average is `R / N`; uncapped width is average times `(1 - padding_inner)`.
- Outer inset is average times `padding_outer`.
- For `N > 1`, step is average times `(1 - 2 * padding_outer / N)` times
  `(1 + padding_inner / (N - 1))`.
- A single band is centered using its actual capped width, and its step is `R`.
  A single key with a larger explicit band count occupies the first band.

These formulas follow the pinned implementation, not the general D3 band
padding/align/round API. A width cap changes band widths, not multi-band starts
or steps. Inner padding must be finite in `[0,1]`; outer padding must be finite,
nonnegative and no larger than half the effective count. A supplied width cap
must be finite and nonnegative. Invalid options are rejected; the frozen source
accepts unchecked float setters, so this stricter numerical contract is explicit.
Empty/zero-width ranges remain valid. Derived values that cannot fit a finite
double report `CUI_PLOT_OVERFLOW` without publishing partial geometry.

`cui_plot_band_map` maps a key; `cui_plot_band_at` maps an existing deduplicated
domain index. Unknown keys and indices beyond the domain return `CUI_PLOT_EMPTY`.
Nearest rounds relative to **band starts**, not centers, with halfway ties toward
the next index. It clamps to the effective band count and can identify a reserved
empty band past the domain. Empty or single-band layouts return index zero.
For a multi-band layout with zero step, a pixel above the collapsed start returns
the last slot; pixels at/below the start return zero, matching the source's
saturating rounded-index behavior. Point's zero-range nearest instead returns
zero for every pixel.

### Point and Ordinal

Point preserves input domain order and duplicates. Positions run from the first
range endpoint to the second; reversed ranges reverse their direction. A single
entry sits at the midpoint. Key lookup uses the first matching entry, while
`cui_plot_point_at` addresses later duplicate entries independently. Nearest
rounds to the closest entry, with positive halfway ties toward the next index,
and clamps to domain bounds. Empty, single-entry and zero-range scales return
nearest index zero. Empty/unknown keys and unavailable indices return EMPTY
without writing the pixel output. Nonfinite endpoints/pixels are invalid.

Ordinal copies generic C range elements with `memmove`, so colors, numbers and
application structs can be supplied without importing platform or Core types.
For a key's first matching domain index `i`, output is `range[i % range_count]`.
Duplicate entries remain in the domain and affect later unique keys' indices.
The optional unknown pointer supplies one borrowed element for absent keys;
known keys with an empty range still return EMPTY even with unknown configured.
No implicit domain extension occurs. Element size must be nonzero, the range
has at most 4,096 elements, and its total byte count and each individual element
must each fit the 1 MiB bound (unknown is separate and shares the element size).
These limits are checked before pointer arithmetic or copying.

The output must have capacity at least `scale.element_size`; capacity failure
leaves its bytes unchanged. NULL output with zero capacity queries that size for
a resolvable key and returns CAPACITY. NULL output with sufficient capacity is
invalid. The borrowed output data and destination must represent the same C
type/element size; the scale does not reinterpret values or validate struct
members. An output may overlap its source element because copying uses memmove;
do not overwrite a borrowed domain/model or a different registered range value
unless the application intends to change that model data.

### Reusable example

The following complete C11 program links only `src/base/cui_plot.c` and the C
math library. It computes bar geometry and a category palette without a renderer:

```c
#include "cui/cui_plot.h"
#include <assert.h>
int main(void) {
    const uint64_t categories[] = {10, 20, 10, 30};
    uint64_t storage[3];
    cui_plot_band_scale bands;
    size_t required;
    assert(cui_plot_band_init(&bands, storage, 3, categories, 4,
        (cui_plot_band_options){.range_start = 0, .range_end = 300},
        &required) == CUI_PLOT_OK);
    cui_plot_band_geometry geometry;
    assert(cui_plot_band_layout(&bands, &geometry) == CUI_PLOT_OK);
    double x;
    assert(cui_plot_band_map(&bands, 20, &x) == CUI_PLOT_OK);
    assert(x == 100 && geometry.band_width == 100);
    const uint32_t colors[] = {0xff3366ff, 0xff6699ff};
    cui_plot_ordinal_scale palette;
    assert(cui_plot_ordinal_init(&palette, storage, 3, colors, 2,
                                sizeof *colors, NULL) == CUI_PLOT_OK);
    uint32_t color;
    assert(cui_plot_ordinal_map(&palette, 20, &color, sizeof color) == CUI_PLOT_OK);
    assert(color == colors[1]);
    /* Compose a bar at x with geometry.band_width and the application's y scale. */
    return 0;
}
```

Compile this assertion-based example with `-UNDEBUG`:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -UNDEBUG -Iinclude plot_example.c src/base/cui_plot.c -lm -o plot_example
./plot_example
```

Headless tests exercise the source's categorical cases plus transactional errors,
capacity queries, duplicate handling, generic struct palettes, extreme finite
Point ranges, overlapping Band storage, and 500 randomized forward/nearest
round trips in both range directions. These scales do not establish full Plot parity.


## Allocation-free shape geometry

`cui_plot_path`, `cui_plot_stack`, and `cui_plot_pie` accept caller arrays and
write caller storage. Pass `NULL, 0` for output to query capacity. They return
`CUI_PLOT_CAPACITY` and the required element count when storage is too small;
empty input succeeds with count zero. Invalid inputs and numerical overflow
leave both output and required count unchanged. No error writes partial output.
Inputs, outputs and required-count pointers must be disjoint; buffers must
remain stable for the whole call. The builders do not retain pointers or invoke
callbacks. Sample/arc counts and total stack cells are bounded by
`CUI_PLOT_CATEGORY_LIMIT` (4096). Path building uses a fixed index scratch array
of approximately 32 KiB on 64-bit hosts; output can exceed 4096 commands.

These contracts were audited against frozen `shape/line.rs`, `area.rs`,
`stack.rs`, `pie.rs`, `arc.rs`, and `bar.rs` at the reference commit above.
They expose portable geometry rather than GPUI paint, path-cache, style,
callback or label objects.

### Lines and areas

Each `cui_plot_sample` contains already projected `x`, lower `y0`, upper `y1`,
and `defined`. Line building ignores `y0`. Undefined samples' numeric fields
are ignored, including NaN; defined coordinates must be finite. The caller
maps data through linear/log/category scales before building commands.

`CUI_PLOT_GAPS_SKIP` drops undefined samples and connects their neighbors,
matching frozen GPUI. `CUI_PLOT_GAPS_SPLIT` is an explicit extension: it starts
a new path at each contiguous defined run. Leading, trailing and consecutive
missing samples create no commands. Every singleton run emits a Move command.

The upper path supports Linear, Natural (the source's Catmull-Rom-to-cubic
formula with duplicated end neighbors), and StepAfter. StepAfter intentionally
omits the final vertical leg, matching frozen GPUI rather than assuming a D3
step curve. Cubic controls that exceed finite double range return OVERFLOW.
Control differences divide their operands before subtracting to avoid
intermediate overflow on targets where `long double` has double's range.
Floating-point rounding still applies near the representable limits.

With `area=true`, each run follows its upper curve, then visits all lower
`y0` points in reverse with straight Line commands and emits Close. This allows
stacked per-point lower baselines, an extension to frozen GPUI's optional
constant `y0`. Single-point areas are closed degenerate polygons. Areas always
close at defined endpoints; they do not reproduce the frozen area behavior
that could leave the fill open when the first/last original sample was missing.
The lower boundary is linear even when the upper boundary is Natural or
StepAfter. Commands contain points and two controls for Cubic; unused members
are zero. A renderer interprets Move/Line/Cubic/Close, applies origin/clipping,
and chooses fill/stroke/dot styles. No tessellation is performed here.

### Natural-order stacking

`cui_plot_stack` consumes a series-major `cui_plot_value` matrix. Cell
`series * point_count + point` produces the corresponding `cui_plot_stack_point`
with lower `y0` and upper `y1`. Each column starts at zero and sums series in
input order; undefined values contribute zero. Negative values share this
signed cumulative baseline, matching frozen GPUI; positive/negative divergent
stacks, normalization, key sorting and alternate offsets are not implemented.
Finite cumulative overflow fails transactionally. Original datum identity
remains the caller's series/point index rather than a cloned payload.

This complete example stacks two series, projects their boundaries through
an inverted vertical scale, then produces closed area commands:

```c
#include <assert.h>
#include "cui/cui_plot.h"
int main(void) {
    const cui_plot_value data[] = {{2,true},{3,true},{4,true},{1,true}};
    cui_plot_stack_point stacked[4];
    size_t required = 0;
    assert(cui_plot_stack(data, 2, 2, stacked, 4, &required) == CUI_PLOT_OK);
    const cui_plot_scale y = {CUI_PLOT_LINEAR,0,8,80,0,false};
    cui_plot_sample samples[2];
    cui_plot_path_command commands[8];
    for (size_t series = 0; series < 2; ++series) {
        for (size_t point = 0; point < 2; ++point) {
            const cui_plot_stack_point cell = stacked[series * 2 + point];
            samples[point] = (cui_plot_sample){(double)point * 100,0,0,true};
            assert(cui_plot_map(&y, cell.y0, &samples[point].y0) == CUI_PLOT_OK);
            assert(cui_plot_map(&y, cell.y1, &samples[point].y1) == CUI_PLOT_OK);
        }
        assert(cui_plot_path(samples, 2, CUI_PLOT_CURVE_LINEAR,
               CUI_PLOT_GAPS_SPLIT, true, commands, 8, &required) == CUI_PLOT_OK);
        assert(required == 5 && commands[4].kind == CUI_PLOT_PATH_CLOSE);
        /* Submit commands to the application's path renderer here. */
    }
    return 0;
}
```

Compile with the same standalone C11 command shown above, using `-UNDEBUG`
for this assertion-based example.

### Pie angles and arc queries

`cui_plot_pie` keeps only defined positive finite values, preserving original
indices and input order. Zero and negative values are skipped; missing numeric
fields are ignored. It distributes the configured angular interval in
proportion to values without sorting. Padding is finite nonnegative metadata,
not subtracted from angles; start/end may reverse or coincide. Normalized
summation avoids overflow when finite values have an enormous total. The last
arc ends exactly at the configured endpoint. This differs from the source's
unchecked f32 accumulator by retaining meaningful proportions for extreme
finite inputs; very small proportions may still round to zero width.

Angles are radians, zero at 12 o'clock and positive clockwise.
`cui_plot_arc_centroid` computes the midpoint angle at the average radius,
relative to an application-selected center. `cui_plot_arc_contains` takes a
position relative to that center, includes both radius boundaries, and uses a
normalized screen angle in `[0, 2pi)` with a half-open
`[start_angle, end_angle)` interval. It ignores padding, exactly as frozen
GPUI's hit test does. Reversed or unnormalized intervals do not automatically
wrap; applications must split wrapped intervals themselves. Radii must be
finite with `0 <= inner <= outer` (stricter than source radius clamping).
Query errors preserve output. Arc `value` and `index` are payload metadata,
not inputs to centroid/hit arithmetic.

Arc paths, tessellation and radial geometry are described below. Rounded
corners, bar orientations/labels, path caches, axis/grid labels and full styled
Plot composition remain separate work. Existing projected vertical bars still
use explicit pixel width, clipping and baseline. Pure commands and math queries
do not establish complete GPUI Plot parity.


## Arc paths and bounded tessellation

`cui_plot_arc_path` produces a closed circular/annular sector relative to
`(0,0)`. `CUI_PLOT_PATH_ARC` appends an endpoint-arc command to the existing
Move/Line/Cubic/Close vocabulary. Its `point` is the endpoint, `radius` is both
circular radii, `clockwise` selects the screen sweep direction, and `large_arc`
selects the sweep greater than pi. There is no ellipse rotation. Unused fields
are zero. The renderer starts at the previous command's endpoint. Commands
now have appended radius/flag members; rebuild consumers against the current
header instead of assuming the old structure's byte size.

A normal annular sector emits Move, clockwise outer Arc, Line to the inner
endpoint, counterclockwise inner Arc, Close. Solid sectors use Line to the
center instead of the inner Arc. Full turns use two semicircles per rim so
coincident start/end points never collapse into an empty endpoint arc. At most
seven commands are needed. Unlike the frozen implementation's artificial
`0.0001` angular seam, full turns have no seam and ignore padding. Spans within
`1e-12` radians of one turn normalize to exactly one turn.

Finite radii satisfy `0 <= inner <= outer`; negative/reversed radii are errors,
not source-style clamped radii. Positive spans must be at most one turn (apart
from the tolerance above); greater spans are INVALID. Reversed/zero spans,
zero outer radius, and sectors consumed by padding succeed with zero commands.
For smaller sectors, outer angular inset is half `pad_angle`. For a nonzero
inner radius the inner inset is half
`min(outer / inner * pad_angle, span * 0.8)`. This follows the frozen padding
width policy, but retains user padding for spans above pi instead of the
source's `0.0001` override. Tiny nonzero radii/spans are accepted rather than
applying the source's f32 epsilon cutoff. Angles reduce modulo one turn before
coordinate arithmetic; finite large angle offsets can still lose a small
configured span in the input double representation.

`cui_plot_arc_flatten` constructs the same sector with Move/Line/Close only.
Pass `segments_per_turn` from 4 to 1024. Each rim uses
`ceil(rim_span * segments_per_turn / (2*pi))` equal angular chords, at least
one for a nonempty rim. Maximum radial sagitta per chord is
`radius * (1 - cos(pi / segments_per_turn))`, apart from floating-point rounding.
The outer rim is traversed clockwise, the inner rim in reverse; a solid sector
adds the center. At most 2051 commands are needed. This is bounded polygonal
approximation, not a triangle mesh, anti-aliasing or GPU tessellation API.
The application applies center translation, clipping, fill rule and styling.
Both builders share capacity-query and no-partial-write contracts above.

## Radial lines and areas

`cui_plot_radial_path` uses `cui_plot_radial_sample` with angle, `radius0`,
`radius1`, and defined status. Angles follow the pie convention; vertices are
relative to `(0,0)`. Defined radii are finite/nonnegative, stricter than the
frozen RadialLine's unchecked radius accessor. Lines use only `radius1` and
ignore `radius0`. SKIP bridges missing samples, matching frozen GPUI; SPLIT
starts separate contiguous runs. Input order controls polygon order.

Open strokes emit Move/Line. A closed stroke adds Close only for runs of at
least three vertices. Fill paths always close and omit runs with fewer than
three vertices, matching frozen RadialLine; request fill and stroke separately
if both are needed. `area=true` instead follows `radius1`, reverses all
`radius0` vertices, and closes each run, including degenerate singleton runs.
It ignores closed/fill flags. Radial area is a CUI extension: the frozen source
contains RadialLine but no separate RadialArea generator. There are no natural
radial curves, angular sorting, radius interpolation, dots or paint callbacks.
The fixed scratch index array has the same 4096-sample bound as Cartesian paths.

This standalone example builds a solid pie sector and a closed radar polygon:

```c
#include <assert.h>
#include <math.h>
#include "cui/cui_plot.h"
int main(void) {
    const double pi = acos(-1.);
    const cui_plot_arc quarter = {0,1,0,pi/2,0};
    cui_plot_path_command commands[32];
    size_t count = 0;
    assert(cui_plot_arc_path(&quarter,0,20,commands,32,&count) == CUI_PLOT_OK);
    assert(count == 4 && commands[1].kind == CUI_PLOT_PATH_ARC);
    assert(cui_plot_arc_flatten(&quarter,0,20,16,commands,32,&count) == CUI_PLOT_OK);
    assert(count == 7 && commands[6].kind == CUI_PLOT_PATH_CLOSE);
    const cui_plot_radial_sample radar[] = {
        {0,0,10,true}, {pi/2,0,20,true}, {pi,0,15,true}, {1.5*pi,0,12,true}
    };
    assert(cui_plot_radial_path(radar,4,CUI_PLOT_GAPS_SKIP,true,false,false,
                                commands,32,&count) == CUI_PLOT_OK);
    assert(count == 5 && commands[4].kind == CUI_PLOT_PATH_CLOSE);
    return 0;
}
```

Compile with the standalone C11 command above (`-UNDEBUG` for assertions).
Tests cover quadrant coordinates, radius boundaries, arc direction/large flags,
full turns, hollow/solid shapes, padding exhaustion and inner-pad clamping,
tiny/extreme radii, flatten counts, missing radial runs and transactional errors.

## Reusing existing Sankey geometry

The frozen Plot exports a real Sankey shape/layout solver (`shape/sankey.rs`),
including topology, alignment, value scaling and link paths. CUI already
exposes caller-owned Sankey node/link geometry via
`cui_chart_sankey_layout` and `cui_chart_sankey_link_bounds` in
[`cui_chart.h`](../include/cui/cui_chart.h), with Chart options for alignment,
linear/sqrt value scale and relaxation iterations. See
[`CHART.md`](CHART.md) for the current Chart implementation and limits.
Those APIs belong to the Chart layer and depend on its data/options contracts;
the pure `cui_plot.h` module does not duplicate them or claim a standalone
Sankey topology/path generator. Adding another solver here would require a
separate ownership/API decision rather than counting the existing Chart solver
as full Plot parity.


## Bar alignments and grid quads

`cui_plot_bar_layout` consumes already projected `cross`, band width, baseline,
and value-end coordinates. Cross is the band's leading edge, unlike the older
`cui_plot_project_bar` helper's mapped center. Bottom/Top produce vertical
frames; Left/Right produce horizontal frames. The value axis bounds use
`min(baseline,value)` and `max(baseline,value)`, so each orientation supports
values on either side of the baseline. Zero band width or equal baseline/value
returns valid degenerate geometry without clipping or rendering.

The output includes source-exact default label origins. Vertical labels anchor
at `cross+width/2`; above means value minus 12px, below means value plus 2px.
Bottom chooses above on ties; Top chooses below on ties. Horizontal labels
anchor at value plus/minus 2px and `cross+width/2-5`; Left chooses right on ties,
Right chooses left on ties. Text alignment, actual text measurement and custom
label content remain the application's responsibility. `gradient_angle` is
0/180/90/270 degrees for Bottom/Top/Left/Right, matching the frozen metadata;
this does not add a gradient renderer. Inputs are finite, band width is
nonnegative, and overflow/invalid inputs preserve the entire output.

`cui_plot_grid_layout` implements frozen `grid.rs` geometry: vertical X lines
first, then horizontal Y lines, each as a 1px quad centered on the configured
coordinate. For example X=10 produces X bounds 9.5..10.5. Line lengths follow
positive width/height; positions outside the surface are not silently clipped.
Applications or ancestor boxes decide clipping.

Dash entries alternate dash/gap starting at zero for each line. Odd patterns
repeat twice to preserve SVG parity. Negative entries clamp to zero; empty or
all-nonpositive patterns draw solid lines, matching the source. An even pattern
with only positive gaps draws no quads. Final dashes clip to the line's endpoint.
Nonfinite entries are INVALID. Tiny positive steps that cannot advance at the
current double position return DEGENERATE rather than looping indefinitely.

Extents are positive finite at most 16384; positions are finite within
+/-16384. At most 256 lines per axis, 64 dash entries and 4096 quads are accepted.
Capacity queries publish exact counts unless more than 4096 quads are needed;
then CAPACITY publishes 4097 as a **lower bound** and no larger buffer bypasses
the bounded generator. Coarsen the dash pattern or split the grid into smaller
calls. Other errors preserve required count; all errors preserve output quads.
No allocation or retained pointers are used; arrays must be disjoint and stable
for the call. Tests cover orientation/tie behavior, source dash cases, centered
quads, numerical failures, budget limits and transactional capacity failures.

The frozen exports only ScaleLinear/Band/Point/Ordinal, and only Natural/Linear/
StepAfter Cartesian curves. There is no frozen StepBefore curve or Step scale
being omitted under another name. Decimal value support, retained paint caches,
custom label/content callbacks and renderer-specific styling are separate from
these original C geometry APIs.
