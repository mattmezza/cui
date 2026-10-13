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
The pure model now exposes a shared Base graph solver plus the source ribbon
path separately; see [the shared graph contract](#shared-base-sankey-graph-model).
The original CUI Chart solver's math was extracted and audited against frozen
Plot Sankey. Chart now delegates topology, relaxation and end widths to that shared seam;
its adapter preserves Chart defaults, raw throughput and caller geometry. This
avoids counting the older Chart API itself as standalone Plot parity.


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

## Frozen Area fill/stroke paths

`cui_plot_area_paths` complements the extended `cui_plot_path(...,area=true)`
API. It models frozen `shape/area.rs` command construction explicitly: each
`cui_plot_area_sample` has independent X/Y availability, and the optional
baseline is one constant. Missing X/Y pairs are skipped and neighbors connect.
When a baseline is supplied, the fill closes only if at least two valid pairs
remain and both original endpoint X values exist. Those original X values are
used for closure even if their Y values are absent. A missing original endpoint
X leaves the fill open. Without a baseline the fill stays open. A singleton
returns a Move-only fill and stroke, reproducing the source's early return.

Fill and stroke have identical upper curves (Linear/Natural/StepAfter), then
only fill appends its baseline segments and Close. This keeps closing edges
out of the visible stroke, unlike painting a closed area with both fill/stroke
colors on one command array. Source constructors/accessors are represented by
caller-materialized samples rather than callbacks; accessor side effects are
not reproduced. Available X and paired Y coordinates must be finite; unused
Y values and absent baseline numeric fields are ignored. Curved-control overflow
is checked before either output is written.

`cui_plot_area_buffers` supplies separate stable/disjoint fill/stroke arrays and
capacities. CAPACITY publishes both required counts without touching either
array. INVALID/OVERFLOW preserve both counts and arrays. Count is bounded to
4096 samples. The implementation uses approximately 160 KiB of fixed sample/
index scratch storage on a typical 64-bit host and no allocations. Array contents
must remain stable during preflight and emission. Both arrays may be NULL with
zero capacities for a query. Returned counts describe commands, not whether a
renderer produces a nonempty tessellation from them.

The open fill path retains source command construction without claiming exact
GPUI tessellation. CUI's bounded rasterizer fills only explicitly closed runs;
append a Close command in separate renderer-owned storage if an open contour
should be implicitly filled. Native adapters can use their own implicit-fill
rules. Pass fill and stroke as separate `cui_plot_shape` objects:

```c
#include <assert.h>
#include "cui/cui_plot.h"
int main(void) {
    const cui_plot_area_sample points[] = {
        {0,2,true,true}, {1,4,true,true}, {2,0,true,false}, {3,3,true,true}
    };
    cui_plot_path_command fill[32], stroke[32];
    cui_plot_area_buffers output = {.fill=fill,.stroke=stroke,
                                    .fill_capacity=32,.stroke_capacity=32};
    assert(cui_plot_area_paths(points,4,CUI_PLOT_CURVE_LINEAR,true,0,
                               &output) == CUI_PLOT_OK);
    assert(output.stroke_count == 3 && output.fill_count == 6);
    assert(fill[5].kind == CUI_PLOT_PATH_CLOSE);
    assert(stroke[2].kind == CUI_PLOT_PATH_LINE);
    return 0;
}
```

Compile with the standalone command above (`-UNDEBUG` for assertions).

## Complete frozen Plot export audit

This is a feature audit of the pinned **base Plot module**, not a declaration
of complete parity. It covers every exported family from `plot/mod.rs`,
`scale.rs`, `shape.rs`, and public `label.rs`; private implementation behavior
was inspected where it changes the public result. Evidence is in
`test_plot.c`/`test_plot_styled.c` and the contracts above. A successful geometry
case does not validate all paint styles, lifecycle behavior or f32 corner cases.

| Frozen export/features | Current CUI route | Exact gaps or deliberate differences |
|---|---|---|
| `Scale`, `PlotValue`; f32/f64/feature-gated Decimal; tick and default nearest=0 | Explicit C double scale/category functions | No generic trait, Decimal arithmetic or named continuous nearest-index query; discrete nearest available |
| `ScaleLinear::new` from unordered domain extent; extrapolating tick; reversed/equal pixel range | `cui_plot_linear_extent_init/tick` plus strict `cui_plot_scale` navigation | Extent/tick supports unordered finite values, empty/equal domains with no tick, reversed/equal pixel ranges and extrapolation; strict map still rejects equal ranges; CUI log/ticks/clamp/navigation are extensions |
| `ScaleBand` dedup, leading reserved bands, width cap, inner/outer padding, step, nearest | Band init/layout/map/at/nearest | Uint64 application keys and bounded copied domain; stricter finite/padding/cap validation; no hash-generic domain or source unchecked negative widths |
| `ScalePoint` owned domain, duplicate-first map, tick-at, nearest | Point init/map/at/nearest | Uint64 keys, borrowed arrays, bounded finite validation rather than owned generic Vec |
| `ScaleOrdinal` domain/range replacement, unknown, first duplicate, cycling cloned output | Ordinal init/map; reinitialize to replace arrays | Byte copies preserve C value representations but do not deep-clone arbitrary payloads; pointers remain caller-owned; finite capacity bounds |
| `Curve::{Natural,Linear,StepAfter}` | Cartesian path and source Area builders | Source Catmull-Rom controls and omitted final StepAfter vertical reproduced; no additional upstream StepBefore/Step scale exists |
| `Line` data/X/Y accessors, source skip-gaps, stroke/width, dots/size/fill/stroke, paint/cache | Materialized samples, path commands, shape raster stroke | Dot geometry/themed drivers now available; no accessor callbacks, arbitrary Background/gradient, or source renderer mesh cache; caller-owned software segment cache available; numeric bounds/precision differ |
| `Area` data/X/Y1, optional Y0, fill plus separate 1px upper stroke, curves, paint/cache | `cui_plot_area_paths` separate contours; two raster shapes | Source accessor callbacks/cache/backgrounds absent; open/singleton GPUI tessellator result not reproduced; extended per-point closed areas remain separate |
| `Bar`, `BarAlignment` orientation/gradient angle, cross/band/base/value, fill(frame,alignment), labels, corner radii | `cui_plot_bar_layout`, projected vertical helper, rectangular paths | Geometry/label anchors, solid fill callback and Core per-corner styled driver available; pure independently rounded-bar paths available; no generic label callback or arbitrary gradient renderer; negative width is rejected |
| `Pie` positive-value arcs/index/padding and angular limits | `cui_plot_pie` | No accessor callback/borrowed arbitrary datum payload; stable normalized double summation replaces unchecked f32 sums; pad is metadata |
| `ArcData`, `Arc` radii, centroid, contains, padded paint/cache | Arc metadata, centroid/contains/path/flatten | Strict nonnegative ordered radii rather than source clamping; explicit SOURCE_PADDING reproduces >=pi .0001 seam override and epsilon behavior, legacy SEAMLESS remains available; spans above one turn rejected; flattened output can use software segment cache, no native mesh cache/Background |
| `RadialLine` index-aware angle/radius, open/closed stroke, >=3-point fill, dots, paint | Materialized radial paths plus raster | Themed radial dot driver available, but no accessor callbacks; nonnegative radii enforced; split gaps and radial area are extensions, not separate upstream exports |
| `Stack`, `StackPoint`, `StackSeries` keys/index, cloned datum, natural signed baseline, missing=0 | Series-major `cui_plot_stack` cells | Numeric stacking covered; key names/indices and data payload references supplied by application; no Rust-style cloning or retained series allocation |
| `Sankey`, graph/link/node/layout/error types, topology/layout/layout_from, alignment, linear/sqrt, relaxation, ribbon path | Shared Base graph/path model and Chart adapter | Shared pure topology/layout/layout_from, adjacency queries, source defaults, scaled values and tapered ribbon path available; bounded capacities/strict finite nonnegative values and checked doubles differ; Chart adapter delegates while preserving its public contracts |
| `PlotAxis`, `AxisText`, `AxisLabelSide`, `AXIS_GAP`, `axis_gutter` | Axis defaults/layout/paint/ticks/gutter | Line visibility, fixed source offsets/explicit colors via source constructor, legacy themed/font-scaled route preserved; dynamic Backgrounds absent |
| `AxisLabelPlacement::{Outside,Inside}` | Application places labels/reserves gutters; Chart has its own layer | No standalone Plot placement enum or automatic gutter allocation; source PlotAxis itself only records position/side |
| `Grid` X/Y, stroke, dash array, centered quads | Grid layout/themed paint | Dash parity/defaults and explicit-color paint covered; bounded counts, stricter finite input and DEGENERATE nonprogress behavior; no arbitrary Background |
| `Text`, `PlotLabel`, text width/truncation helpers | Caller-owned PlotLabel collection/layout/paint and measured UTF-8 ellipsis helper | No automatic tooltip content/prepaint; frozen font_weight builder records a field that PlotLabel.paint ignores, not a verified weighted-text rendering feature |
| `PathCache`, `PathCaches`, `ShapeKey` | Caller-owned flattened-segment cache, translated raster and shape-key helper | Cache hits skip builder/cubic flattening; empty builds are warm; opaque FNV keys preserve f32 bit sensitivity, not Rust hash values; no renderer-native stroke triangles or implicit keyed slot bank |
| `PlotMotion` pointer Spring, enter/exit/appear Transitions | Caller-owned scalar transition clock with signed delay, easing and reversal shortening | Enter/exit projected clocks and scalar pointer glide available; explicit host policies and first-paint generation binding; no automatic theme projection/keyed renderer clock |
| `PlotAppear` complete/progress/is_appearing/staggered and `PlotAppearScope` | Caller-owned completed-generation slot/registry and linear stagger clock | Bounded multi-plot completed map with explicit consecutive-frame lifetime; styled driver binds first-paint generation/linear time/easing/reduced motion via explicit host frame serial and active registry; no implicit scope tree |
| `TooltipState`, `PlotHover` state/progress/deprecated focus/is_hovered/is_entering/glide; scope helpers hover_progress/is_hover_entering/pointer_spring | Caller-owned index/dots/crosshair memory with lingering exit state | Scalar pointer glide and styled resolver/content driver available; no implicit scope lookup or automatic OS/event-to-datum routing |
| `Plot` trait prepaint/paint/id/interactive/appear_generation/appear/tooltip_state/hover/tooltip | Bitmap view plus optional `cui_plot_element` resolver/marks/children/tooltip driver | Caller-owned hover clocks and inert portal composition; current bounds/pointer/occlusion supplied by host; first-paint appearance binding available; no trait-equivalent renderer prepaint or automatic cache binding |
| `PlotElement` full-size layout, element-state appear, current-bounds cursor, occlusion-aware mouse/touch tracking, tooltip prepaint/paint | Core layout/image semantic and validated click capture | Fixed bitmap extent with styled tooltip composition/exit clocks and explicit redraw requests; no full-size auto layout, current-frame Core hitbox/prepaint query, OS gesture detector or implicit scope state |
| `origin_point` translation utility | Caller adds relative offsets before raster; Core offsets styled children | No separate named pure translation helper; ordinary finite coordinate addition remains caller responsibility |

Missing renderer/lifecycle rows are real remaining Plot scope. Similar Chart,
Core, effects or text APIs can be reused in application composition, but their
existence does not make the standalone Plot contract equivalent to the frozen
module. No row above asserts full widget parity.


## Dot geometry and source-style marker drivers

`cui_plot_dot_layout(center,size,output)` models the Line/RadialLine marker quad:
a square centered at the point, uniform corner radius `size/2`, and a fixed 1px
border. Zero size succeeds with degenerate geometry; negative/nonfinite size and
nonfinite centers are INVALID. Finite bound overflow preserves the entire output.
This is pure geometry without a Core/theme dependency. Input-coordinate precision
and half-size rounding follow C double arithmetic, not the frozen f32 bit pattern.

The optional styled layer supplies Cartesian/radial marker and rounded-bar
composition in `cui_plot_view.h`, described in [PLOT.md](PLOT.md). These reuse
Core's existing rounded-quad renderer; they do not turn the bounded path rasterizer
into a gradient renderer. The pure `cui_plot_bar_path` also represents per-corner rounding as bounded cubic path commands.
The frozen dot stroke fallback is reproduced for supported solid colors: explicit
stroke overrides fill, otherwise the 1px border uses the resolved fill color.
Theme defaults are a deliberate styled-layer policy, while frozen base dots
default to transparent fill and frozen base bars default to black.

## Caller-owned hover and appearance lifecycle

`cui_plot_hover_track` follows the frozen hover memory contract: a live datum
updates the index, crosshair, borrowed dots and cursor; exit retains the last
state until progress is zero. `hovered` means a currently live datum, while
`has_state` also includes a fading tooltip. `entering` means live with exactly
zero progress, which lets an application snap its existing pointer spring on
entry. Unlike the source's optional cursor path, this API rejects live data
without a cursor. Dot storage must remain valid throughout the exit fade;
clearing at zero releases the borrowed pointer. Invalid inputs preserve memory.
The helper does not resolve a datum, draw a tooltip or schedule frames.

`cui_plot_appear_scope` is one slot per application identity, rather than the
source's keyed map/thread-local scope. Call `cui_plot_appear_complete` before
starting a clock. Record its linear progress with `cui_plot_appear_record`;
only a completed generation persists. An interrupted appearance therefore
restarts when reconstructed; a completed generation can resume immediately.
Identity changes clear the slot. A new generation can replay without forgetting
the old completed generation until the new one completes. Reset the slot when
the enclosing application scope disappears: the helper cannot infer that event.

`cui_plot_appear_stagger` implements the source's linear-time staggering:
spread clamps to 0 through .95, the index clamps to the last mark, and count
zero or one uses the original clock. Apply the application's existing easing
**after** staggering. All clocks must be finite and in [0,1]. Pass progress one
for appearance and live hover (zero for exited hover) when `cui_reduced_motion`
is true; record completion so later frames do not restart the appearance.
Outside a source-style hover scope, use progress one and entering false.
This is a state contract, not an implementation of PlotElement, motion presets,
retained element scopes, tooltip layout, occlusion or renderer path caches.

## Source-compatible Linear extent and tick

The frozen `ScaleLinear::new` derives a domain extent from unordered input and
its `tick` returns no coordinate when the domain has zero extent. Pixel ranges
may be reversed or equal. `cui_plot_linear_extent_init` and
`cui_plot_linear_extent_tick` expose this behavior separately from the strict
`cui_plot_scale` navigation model: an empty input initializes [0,0], singleton
or all-equal values produce `CUI_PLOT_EMPTY` on tick, and a nondegenerate domain
with equal pixel endpoints maps every finite value to that endpoint. Tick
extrapolates outside the extent. Empty/error results preserve the output pixel;
invalid construction preserves the entire destination model.

Construction accepts at most 4096 finite doubles, copies only their extent,
and retains no input pointer. CUI uses checked double coordinates rather than
the source's generic PlotValue and f32 output; NaN/infinity are rejected and
unrepresentable extrapolation returns `CUI_PLOT_OVERFLOW`. This API has no
inverse, automatic nice bounds, clamp, tick collection or category behavior.
Existing strict scale validation, zoom and pan contracts remain unchanged.

```c
#include <assert.h>
#include "cui/cui_plot.h"
int main(void) {
    const double values[] = {3, 1, 2};
    cui_plot_linear_extent scale;
    double pixel;
    assert(cui_plot_linear_extent_init(&scale, values, 3, 12, 12) == CUI_PLOT_OK);
    assert(cui_plot_linear_extent_tick(&scale, 2, &pixel) == CUI_PLOT_OK);
    assert(pixel == 12);
    return 0;
}
```

## Rounded Bar paths for custom renderers

The frozen Bar paints a normalized rectangle with independent corner radii.
`cui_plot_bar_path` reuses `cui_plot_bar_layout` for all four orientations and
values on either side of the baseline. Radii are ordered top-left, top-right,
bottom-right, bottom-left in physical coordinates; they do not rotate when
alignment changes. Each finite nonnegative radius clamps to half the shorter
frame side, matching CUI's styled adapter policy. The source delegates rounded
quad tessellation to GPUI; this helper is a portable path representation of
that style, not a claim of identical renderer output.

A nondegenerate bar emits ten commands: Move, four alternating edge Lines and
corner Cubics, then Close. The quarter-circle cubics use kappa
`0.5522847498307934`; zero-radius corners still emit degenerate cubics. Existing
`cui_plot_raster` flattens these cubics to sixteen chords each and samples pixel
centers without antialiasing. Degenerate width/height returns `CUI_PLOT_EMPTY`
and required zero. A zero-capacity query returns `CUI_PLOT_CAPACITY` and required
ten; insufficient capacity preserves every output command. Invalid geometry or
radii preserve both commands and required count. Caller radii may alias output:
they are copied before any command is written. No gradient, labels, clipping,
retained shape or renderer cache is introduced.

## Explicit frozen Arc padding policy

`cui_plot_arc_path_ex` and `cui_plot_arc_flatten_ex` accept
`CUI_PLOT_ARC_SOURCE_PADDING` to select the frozen Arc's padding/default rules.
For every positive span at least π, the source overrides `ArcData.pad_angle`
with .0001 radians. This applies to half circles, large sectors and full circles,
not just the full-circle seam. Smaller sectors keep the supplied padding.
Annular inner padding retains the source's outer-radius pad-width conversion
and cap at 80 percent of the unpadded span. Inner radii at or below 1e-12 use a
center wedge; outer radii or positive spans below 1e-12 produce no path.

The source policy uses a single endpoint arc on each rim, leaving the intentional
seam even for a full turn. Flatten uses these same padded endpoints. It is an
explicit alternative to `CUI_PLOT_ARC_SEAMLESS`, the unchanged policy of legacy
`cui_plot_arc_path` and `cui_plot_arc_flatten`: legacy full turns ignore padding
and use two semicircles. The source's source-comment says full circle, but the
actual conditional is `da >= PI`; this API follows that wider condition.

Both policies retain CUI's finite nonnegative ordered-radius and at-most-one-turn
bounds. The source separately clamps negative radii and permits unordered radii;
those are not exposed here. Coordinates use checked doubles and a double π
threshold, not identical source f32 rounding. Paths close explicitly; no cached
GPUI renderer path or exact tessellation claim follows. Centroid and contains
remain independent of either paint policy, and contains deliberately ignores
padding as the source does. Capacity/invalid errors preserve command buffers.

## Tooltip cursor gating from PlotElement

The frozen PlotElement stores an optional cursor as a visibility gate, then
resolves coordinates from the live pointer and current bounds during prepaint.
`cui_plot_cursor_mouse` exposes its desktop per-frame refresh: supply the
application's occlusion-aware hovered decision and live global position each
frame. A visibility flip requests a corrective frame; movement changes the
stored relative position without that per-frame request. For mouse-move event
handling, notify/invalidate on `update.changed`, as the source event handler does.
A stationary pointer therefore picks up the new local position after relayout.

`cui_plot_cursor_long_press` exposes detected Started/Moved/Ended/Cancelled
phases. Started opens only when hovered, inside bounds, and not default-prevented,
and reports `prevent_default` for the application to consume. Moved does nothing
before a start; once active it follows the gesture even outside the bounds and
ignores subsequent hovered/default-prevented decisions. Ended and Cancelled
close it. Changed long-press state requests a frame. The application detects
long presses and owns gesture identity; these functions do not inspect OS input,
steal capture, synthesize delays or resolve Core semantic targets. Use separate
state for mouse and touch, or reset when switching modes.

`cui_plot_cursor_resolve` uses stored state only as a visibility gate. It derives
relative coordinates from the supplied live global pointer/current bounds,
returns no cursor outside bounds or during appearance, and preserves relative
output on no-hit. Bounds follow Core's half-open containment (left/top included,
right/bottom excluded), rather than claiming exact GPUI edge policy. Accepted
positions and positive bounds must be finite; unrepresentable subtraction
returns OVERFLOW. Invalid/overflow inputs preserve memory and outputs.

Feed a resolved cursor to the application's datum resolver, then pass that
optional datum to `cui_plot_hover_track`. Exiting can retain the previous tooltip
while its progress fades. This sequence represents actual source cursor/state
behavior without adding generic Plot callbacks, a tooltip renderer, an implicit
retained scope or the source's prepaint/paint child tree. Application hit testing,
occlusion, overlay content and paint order remain explicit.

## Source Sankey ribbon paths

The frozen `sankey_link_path` connects the source node's right edge to the target
node's left edge using a closed ribbon. `cui_plot_sankey_ribbon` holds those x
coordinates, each end's center y and independent widths. The pure
`cui_plot_sankey_link_path` generates the same five command roles: Move to the
source upper edge, Cubic to the target upper edge, Line to its lower edge,
reverse Cubic to the source lower edge, Close. Both cubics use the midpoint of
source and target x for their controls. Each end independently uses
`max(end_width,min_width)/2`; the nominal global link width is not substituted.
Origin offsets apply to every endpoint/control point. Reversed x directions
are supported; no flow-direction assumption or rerouting is added.

Pass existing `cui_chart_sankey_node.x1/x0` and
`cui_chart_sankey_link_geometry.source_y/target_y/source_width/target_width`
into this structure to render Chart-computed layouts through the bounded Plot
rasterizer or a custom path renderer. This keeps the pure model independent
of Chart headers and avoids claiming that Chart topology equals full Plot
Sankey parity. Graph construction, errors, layering, relaxation, layout_from, value scaling
and alignment are now exposed by the shared Base model below; Chart preserves
its own adapter policies.

The path uses finite doubles, finite origin and nonnegative end/min widths;
invalid inputs and unrepresentable translated geometry preserve output and
required. A capacity query reports five, and insufficient storage preserves
commands. Zero widths still produce a closed degenerate contour. Input geometry
may alias command storage because the complete path is prepared before copying.
The existing rasterizer uses sixteen chords per cubic, pixel-center sampling
and no antialiasing; renderer-specific tessellation/cache and general Background
styling remain unsupported.

```c
#include <assert.h>
#include "cui/cui_plot.h"
int main(void) {
    cui_plot_sankey_ribbon link={10,90,30,50,8,16};
    cui_plot_path_command commands[5];
    size_t required;
    assert(cui_plot_sankey_link_path(&link,1,(cui_plot_point){0,0},
                                    commands,5,&required)==CUI_PLOT_OK);
    assert(required==5 && commands[4].kind==CUI_PLOT_PATH_CLOSE);
    return 0;
}
```

## Shared Base Sankey graph model

`cui_plot_sankey_topology`, `cui_plot_sankey_layout_from` and
`cui_plot_sankey_layout` expose the frozen exported graph workflow without Core,
Chart or platform dependencies. The implementation extracts the original CUI
Chart C solver math into Base, with source-specific public contracts. This is
shared solver provenance rather than a second independently invented layout
algorithm. The Chart adapter preserves its existing defaults and raw throughput reporting;
the pure model uses source defaults and scaled values.

Caller graph storage contains node/link arrays and capacities. Inputs use node
indices; topology copies and scales link values once, computes max(incoming,
outgoing) node values, longest-path depth/height, structural adjacency counts,
alignment and x coordinates. `cui_plot_sankey_link_breadths` factors the source
internal end-width calculation for adapters that already have positioned nodes;
it updates centers/end widths transactionally without changing nominal widths
or topology. This is a shared Base operation, not an extra frozen export claim.
Missing endpoints return INVALID with optional
`CUI_PLOT_SANKEY_MISSING_NODE` and the missing index; cycles (including self links
and zero-valued cycles) return INVALID with `CUI_PLOT_SANKEY_CIRCULAR_LINK`.
Zero-valued links still count as edges. All graph arrays/counts remain unchanged
on failure. Input buffers and the graph's two output arrays must be disjoint.

Layout_from accepts an unmodified graph from topology/layout and reuses its
ranks and already-scaled values, avoiding another topology sort. It recomputes
alignment and geometry for the current extent/options. It does not rescale if
value_scale changes, matching the frozen source. Node/source/target indices and
rank consistency are checked; arbitrary edits to graph topology/value metadata
are unsupported. `cui_plot_sankey_layer_count` returns zero for empty/invalid
graphs. `cui_plot_sankey_node_links` queries incoming/outgoing link indices sorted
by opposite node y0 with input-index ties; topology y0 zero gives input order.
It replaces the source's allocated per-node vectors with bounded caller output.

Defaults are node width 24px, padding 8px, Justify, six relaxation iterations,
Linear scaling and extent[0,0,1,1]. Explicit zero width/padding/iterations stay
zero, unlike the older Chart adapter's zero-selects-default policies. Alignment
supports Left, Right, Center and Justify, including source structural-edge rules
for isolated/zero-value nodes. The explicit `legacy_value_alignment` option
(default false) supports Chart's older sum-based Justify/Center behavior, where
zero-flow edges do not anchor alignment. This compatibility option is a CUI
extension, not a frozen builder export. Relaxation sorts neighbors, resolves collisions,
centers each column, and alternates a .15-height stagger for equal-height
single-node columns when slack permits. End widths independently fill each
node side proportionally, including imbalanced and sqrt-scaled graphs. Nominal
link width remains the global value-to-height scale, not a minimum painted width;
apply that minimum through the ribbon path helper. Coordinates/indices/scaled
values remain explicit in output, with no element tree or renderer cache.

Bounds are 256 nodes/1024 links/100 iterations, finite nonnegative values and
width/padding<=16384. Extent coordinates have absolute value<=16384, positive
height and nonnegative width; node width may exceed the extent width, as with
source defaults in its unit extent. Empty/all-zero/isolated graphs are supported.
Sqrt scales once during topology. Negative/NaN/infinite values are rejected
rather than inheriting unchecked source arithmetic. Checked double/long-double
math may differ from source f32 rounding; unrepresentable totals/intermediates
return OVERFLOW transactionally. Large per-call fixed scratch is stack-owned
(approximately 300KiB at the deepest layout call); no allocations occur. Caller
storage stays live while queried/drawn. No full Sankey widget or source-error
formatting/owned-vector API equivalence is claimed.

```c
#include <assert.h>
#include "cui/cui_plot.h"
int main(void) {
    cui_plot_sankey_node nodes[3];
    cui_plot_sankey_link_layout links[2];
    cui_plot_sankey_graph graph={nodes,links,0,0,3,2};
    const cui_plot_sankey_link input[]={{0,1,9},{1,2,4}};
    cui_plot_sankey_options options=cui_plot_sankey_defaults();
    options.value_scale=CUI_PLOT_SANKEY_SQRT;
    assert(cui_plot_sankey_topology(&graph,3,input,2,&options,0)==CUI_PLOT_OK);
    /* Measure labels against graph layers, then fix the available extent. */
    options.extent=(cui_plot_rect){0,0,200,100};
    assert(cui_plot_sankey_layout_from(&graph,&options)==CUI_PLOT_OK);
    assert(graph.nodes[1].value==3 && graph.links[1].value==2);
    return 0;
}
```

## Bounded appearance scope registry

The frozen `PlotAppearScope` holds a map from plot global identity to its last
completed generation. This matters in virtual lists: a plot that disappeared
after completion skips its draw-in when it returns, while an interrupted plot
starts again. `cui_plot_appear_registry` now models that multi-plot behavior
rather than asking one single slot to represent the entire scope.

Initialize it with caller-owned `cui_plot_appear_scope` storage (capacity 0..4096),
then call `cui_plot_appear_registry_begin` for every frame the scope is painted,
using a stable scope identity and host frame serial. The same serial is
idempotent; the next serial preserves completed entries. A skipped/backward
frame or changed scope identity clears the logical count. Serial wrap is treated
as a discontinuity. Call end when leaving the scope. Query/record require an
active scope, preventing accidental use of a disposed scope's entries. Explicit
init discards remembered completions without touching borrowed storage.

Before starting a plot's clock, query `(plot_identity,generation)`. A completed
match draws immediately at progress one. Otherwise run its independent clock
and record linear progress. Only progress one inserts/replaces the completed
generation. An incomplete appearance consumes no slot and leaves the previous
completed generation intact, exactly as the source map does. A completed plot's
entry remains when that child disappears while the enclosing scope is still
painted. At capacity, completing a new identity returns CAPACITY without mutation;
replacing an existing identity still succeeds. The host chooses a storage/error
policy instead of silently evicting remembered plots.

Nested source scopes choose the innermost map. Portable composition passes the
explicit inner registry to its children; after leaving it, use the outer one
again. There is no global/thread-local lookup, retained Element child, implicit
window state or first-paint scheduler. Identity values represent application
plot identities unique inside that scope (not automatic global path hashing).
The registry stores only numeric completed generations, not plots/resources or
motion state. Scope disposal and frame cadence remain host responsibilities.

```c
#include <assert.h>
#include "cui/cui_plot.h"
int main(void) {
    cui_plot_appear_scope entries[4];
    cui_plot_appear_registry scope;
    bool complete;
    assert(cui_plot_appear_registry_init(&scope,entries,4)==CUI_PLOT_OK);
    assert(cui_plot_appear_registry_begin(&scope,100,1)==CUI_PLOT_OK);
    assert(cui_plot_appear_registry_record(&scope,7,42,1)==CUI_PLOT_OK);
    assert(cui_plot_appear_registry_end(&scope)==CUI_PLOT_OK);
    /* Child 7 may be absent in frame 2; scope persistence remains. */
    assert(cui_plot_appear_registry_begin(&scope,100,2)==CUI_PLOT_OK);
    assert(cui_plot_appear_registry_complete(&scope,7,42,&complete)==CUI_PLOT_OK);
    assert(complete);
    assert(cui_plot_appear_registry_end(&scope)==CUI_PLOT_OK);
    return 0;
}
```

### Projected transition clocks

`cui_plot_transition_step` supplies the scalar `Transition` clock used by frozen
`PlotMotion` enter/exit channels. Zero-initialize a separate caller-owned clock
per stable channel. Supply monotonic seconds, a finite target, and a policy with
nonnegative duration and signed delay; NULL easing uses source cubic-out. A
synchronous easing callback may implement another easing, must return finite
samples, and must not mutate the clock. It is called once per sample and again
at a retarget's initial progress. Returned `request_frame` tells the host when
to schedule another frame; the helper does not schedule, retain element IDs, or
project theme policies automatically.

The first target is adopted immediately. Later retargets first sample the old
trajectory, then start from that value. Reversing to the prior origin shortens
the new duration by the traversed easing fraction, including the source's
repeated-reversal factor. Other retargets restore the policy duration. Current
delay/easing apply to every sample, while duration is stored at retarget time.
Positive delay yields `DELAYED`; negative delay advances the initial sample.
Reduced motion or zero duration snaps immediately and bypasses delay. Backward
timestamps saturate elapsed time to zero. Invalid policies/state/callback results
preserve clock and output; nonfinite interpolation reports overflow. These are
finite double scalar channels, rather than generic source `Interpolate` values.

### Scalar pointer spring glide

`cui_plot_spring_step` covers the frozen `PlotHover.glide` scalar Pixels channel.
Use separate zero-initialized `cui_plot_spring_clock` states for independently
keyed x/y coordinates. `cui_plot_spring_defaults(response_seconds)` matches source
damping 1, epsilon .001 and travel enabled; response zero snaps. Response is the
undamped oscillation period, not a completion deadline. Damping below one can
overshoot, above one approaches slowly, and zero is undamped. The analytic
oscillator is adapted from CUI's existing tab spring C mathematics, with original
clock ordering implementing the frozen source contract.

The first value is adopted immediately. A later call advances elapsed time
toward the **previous** target before adopting its new target, preserving current
position and velocity. `request_frame` remains true until both displacement and
speed are below epsilon, then position snaps exactly to target with zero velocity.
A resting call leaves its timestamp unchanged. Backward timestamps saturate the
elapsed step to zero; the host should normally provide monotonic seconds. Current
policy parameters govern the elapsed step, as in the source. Reduced motion,
`travel=false`, response zero, or `entering=true` pins both position and velocity
to the target. Feed `cui_plot_hover_memory.entering` to the latter argument to
avoid gliding from a previous hover location on the entering frame.

Finite double channels and checked arithmetic are CUI's bounded contract, rather
than source f32 renderer values. Invalid parameters/state preserve state/output;
nonfinite propagation reports OVERFLOW without mutation. Very small responses,
extreme magnitudes/damping or elapsed oscillation angles may exceed representable
math. The host must schedule requested frames and project its theme policy.
Renderer keyed clocks, implicit PathCaches slot lifetime and renderer prepaint
tooltip layout remain absent; host-serial first-paint appearance and styled immediate
portal composition is available through the host-bound PlotElement seam.

### Shape keys and cached tessellation

`cui_plot_shape_key_init(extra)` plus `_point` and `_float` hash the same kinds
of origin-relative projected points and geometry extras used by frozen ShapeKey.
CUI uses original FNV-1a over native representations, not Rust DefaultHasher;
keys are opaque process-local values and must not be persisted or compared with
upstream numeric hashes. Point coordinates are checked finite and float-range,
then converted to f32 to follow source point key sensitivity. Float append keeps
raw bits, including signed zero. Include curve style and other geometry inputs
in extras; hashes can collide, as with the source's opaque 64-bit key.

The styled `cui_plot_path_cache_get` caches flattened relative segments in caller
storage. A key change invokes the build callback once, consumes its borrowed
Move/Line/Cubic/Close commands synchronously, and tessellates each cubic into 16
chords once. Hits skip both callback and tessellation. An empty build is cached
and `is_warm` is true even on its first empty build, matching the actual source
implementation rather than the misleading reuse wording in its doc comment.

`cui_plot_raster_cached` consumes those cached segments and a separate origin,
fill, stroke and stroke width. Scrolling/origin and paint-color changes reuse the
same geometry. Origin translation affects pixels while retained relative segment
storage remains unchanged. Pixel rasterization still happens on every call; this
is a software polyline tessellation cache, not cached pixels or GPUI's native
stroke/fill triangle mesh. Stroke width is evaluated at raster time here, unlike
source PathBuilder's stroke tessellation, so this route can reuse geometry across
width changes. ARC commands must first be flattened using the existing arc helper.

Cache init accepts at most 4096 segments. Invalid commands/origins and capacity
failures preserve cache/storage/output (a failed miss invokes its callback, but
does not change the key). Successful EMPTY changes the key and returns an empty
borrowed path. Relative and translated coordinates must stay within +/-16384.
Borrowed views remain valid until a successful miss replaces storage. All arrays
must be stable/disjoint from cache metadata, output and pixels. The host owns one
cache per stable shape slot, or two independent caches for Area fill/stroke; no
window/element keyed `PathCaches::for_paint` lifecycle or slot auto-growth exists.
The Core renderer still has no native cached path command/triangle seam.

### Styled PlotElement composition

`cui_plot_element` layers a portable driver onto the existing bitmap/axis view.
The host supplies **current** global bounds, live pointer position, and an
occlusion-aware `hovered` decision; those bounds must match actual frame layout
and bitmap extent. Core's `cui_bounds` is previous-frame data and must not be
passed as current bounds during scrolling/relayout. A real Core prepaint hook and
current hitbox query are still required for automatic source renderer behavior.

One zero-initialized state is retained per stable owner. Resolver preflight uses
live pointer minus current bounds every frame, including a stationary pointer
while layout moves; appearance suppresses the live datum. The synchronous
resolver returns OK with relative datum geometry or EMPTY. It may not compose UI
or mutate driver storage. Enter/exit transition policies use Core frame time and
reduced-motion, must produce normalized progress, and request redraw through
`cui_invalidate`. Borrowed dot arrays must survive replacement or completed exit.
The last datum and relative cursor remain during exit, then are dropped at zero.

Bitmap/axes compose first, then marks and ordinary-child callbacks. The primary
activation callback uses existing validated Core release routing with current
host bounds. Tooltip content composes last in a plot-sized global portal at
layer1 (or a supplied positive layer), with inherited hover opacity. The callback
positions its body relative to that portal; un-clipped content may extend outside
plot bounds. Tooltip descendants are inert: they paint, but cannot take semantics,
focus or activation. This display-overlay policy is a bounded CUI choice; source
`AnyElement` tooltips can contain arbitrary interactive content.

Touch hosts run the existing long-press reducer on `state.cursor` before this
call and set `touch=true`; desktop mouse visibility is then not synthesized.
Long-press default prevention and host OS gesture detection stay explicit. On a
passive/disabled plot the driver clears hover/exit-clock state while preserving
appearance, skips resolver/tooltip,
and still composes ordinary children. Owner changes reset state. Callback box
stacks must balance, and text/bitmap payloads remain live through renderer
completion. Preflight errors preserve state and submit no plot; general Core
composition capacity is subject to the existing partial-submission contract.
There is no separate callback measurement/prepaint pass or retained Element tree.

### First-paint appearance channel binding

`cui_plot_appear_step` implements frozen Presence's first-frame 0→1 clock,
separate from transition's first-target adoption. It stores linear time, applies
projected easing only to `progress`, and supplies `cui_plot_appear_value_stagger`
to sample each mark's clamped local time before easing. Default NULL easing is
cubic-out; zero duration or reduced motion completes immediately. Completed
samples return one without invoking easing. Signed delay and checked clock
arithmetic follow the existing transition seam. Policy callback/user data remain
borrowed while a sample is used; invalid callback results preserve state/output.

Enable `cui_plot_element_options.has_appearance`, choose generation and policy,
and increment host `frame_serial` every frame, including omitted plots. The
styled driver owns the first-paint clock in its caller state, restarts after a
serial gap/backward serial, owner rekey or generation change, and notifies
`on_appear` before datum resolution and bitmap composition. This callback may
prepare caller-owned bitmap/mark geometry, but must not submit UI or mutate
options/driver/scope storage; its sample pointer lives only for that call.
The driver does not choose a reveal/fade style for a bitmap: the source Plot
also decides what appearance looks like. Geometry callbacks can use the sampled
progress or stagger values; changed bitmap payloads require the normal Core
bitmap revision/lifetime contract.

Appearance runs even for disabled/passive plots and ordinary children still
compose. Hover is suppressed while linear time is below one, even if easing
already returned one, and appearance/hover frame requests are combined. Existing
manual `appearing` suppression remains an additional gate. No generation means
a complete sample. Defaults are Base's zero-duration policy; product timing is
explicitly projected by the host, not inferred from a generic theme field.

Pass an active innermost `appear_scope` registry begun with the same serial.
Only a completed generation is recorded. A finished plot omitted then remounted
under that still-painted scope is complete immediately; an unfinished one
restarts. Changing/removing the scope resets the local appearance channel. Scope
rekey/gaps clear remembered completions through its existing begin/end contract.
Registry capacity errors are preflight failures: no plot/state/completion entry
is submitted or changed. Explicit caller frames, registry storage and unique
owner IDs substitute for window keyed element state; they do not add an implicit
retained scope tree or a renderer layout/prepaint scheduler.
