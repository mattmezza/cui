# Headless plot scales and geometry

`cui_plot.h` is an original allocation-free C11 math module. It has no core,
style, font, platform, or GPU dependency. It implements scale/viewport behavior
and projected geometry, not a Chart or Plot widget. The pinned GPUI Base linear
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

No axis labels, band/ordinal scales, retained series storage, missing-data
segmentation, stacked bars, paths, hit testing, hover, legends, accessibility
chart description, animated transitions, or rendering adapters are supplied.

`test_plot` checks 1500 deterministic forward/inverse and zoom-anchor cases,
linear/log ticks, reversed ranges, transactional failures, subnormal domains,
finite extreme domains/ranges, clipping, projected lines, and bar baselines.

## Browser demonstration

The Plot gallery displays `sin(x / 12)` rather than only reporting scale metadata. Each frame samples 65 values using `cui_plot_inverse` and paints them with the existing Chart renderer. Zoom in/out change the domain around the viewport center; Pan left/right shift it; Reset restores 0–100. The domain readout and nice-tick count update with the retained scale.

This demonstrates composing the numerical Plot API with a renderer. It does not introduce a separate public Plot widget or mouse gesture API. The scale is application-owned retained state; sampling and drawing are immediate. See `web/gallery.c` for the composed example.
