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

No axis labels, retained series storage, missing-data
segmentation, stacked bars, paths, hit testing, hover, legends, accessibility
chart description, animated transitions, or rendering adapters are supplied.

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
round trips in both range directions. Axis/grid labels, shape builders, stacking,
radial/pie/Sankey primitives, hover composition and complete styled Plot rendering
remain separate work; these scales do not establish full Plot parity.
