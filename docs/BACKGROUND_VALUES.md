# Owned background values and pure sampling

Unreleased C-only API in `include/cui/cui_background.h`. These are independent
owned values and allocation-free math. They do not apply paint to elements or
provide gradient rendering. All existing V1 commands, boxes and renderers retain
their behavior. Full background rendering across software, SDL_Renderer, SDL_GPU,
Sokol and Canvas, all78 component coverage and foreign authoring remain pending.
The approved transport direction is an explicit versioned draw/list contract;
its records and adapters still require review.

A value is either a solid straight-linear sRGB color or a fixed two-stop linear
gradient. There are no pointers, borrowed arrays, caches, contexts or resources.
Constructors copy active fields and zero inactive fields. Directly authored values
are validated on every sample. SOLID ignores model/space/angle/stops; LINEAR ignores
solid. Invalid active enums, nonfinite numbers and out-of-domain inputs fail
atomically. Failed bool calls preserve output. Constructors accept input values
drawn from their output object.

Colors have finite straight linear RGB and alpha in [0,1]; hidden RGB at zero alpha
is retained in owned values. `cui_background_color_from_srgba8` explicitly decodes
encoded byte channels. `cui_background_color_from_source_038_hsla` accepts normalized
HSLA and matches the source shader's conversion convention: its resulting RGB is
treated as linear, without an extra sRGB decode. These conversions intentionally
differ for gray. Neither quantizes to bytes or premultiplies the stored color.

```c
#include <cui/cui_background.h>

cui_background background;
cui_background_color red = cui_background_color_from_srgba8((cui_color){255, 0, 0, 0});
cui_background_color blue = cui_background_color_from_srgba8((cui_color){0, 0, 255, 255});
if (cui_background_make_linear(CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, 90,
        (cui_background_stop){red, 0}, (cui_background_stop){blue, 1}, &background)) {
    cui_color midpoint;
    if (cui_background_sample_srgba8(&background, (cui_rect){0, 0, 100, 20},
                                   50, 10, &midpoint)) {
        /* midpoint is straight encoded sRGBA8 {0,0,255,128}. */
    }
}
```

Angles are finite [0,360] degrees: 0 up, 90 right, 180 down;360 equals0. Stop
positions are finite [0,1]; outer colors extend beyond the interval. CSS requires
ordered stops; SOURCE_038 also accepts reversed stops. Equal stops switch to the
second color at/after the common position. This deterministic equal-stop extension
avoids the source shader's division by zero; it is not a source undefined-result
parity claim. Neither mode implements multiple stops, repeating gradients, source
patterns, CSS parsing, wide-gamut inputs or a CSS framework.

CSS uses the gradient line spanning the full box diagonal projection and
alpha-premultiplied interpolation in the selected space, followed by unpremultiply
before converting back to linear RGB. SOURCE_038 instead rescales the short axis,
uses the dominant-axis projection, interpolates straight selected-space channels
and alpha independently, then premultiplies linear output. Both select encoded
sRGB or Oklab interpolation. A transparent-red/opaque-blue midpoint is blue in CSS
and purple in SOURCE_038; unequal rectangles also expose the geometry difference.

`cui_background_sample` returns premultiplied linear float RGBA. Oklab excursions
outside sRGB are preserved, including negative RGB or RGB above alpha. There is
no gamut mapping. `sample_srgba8` unpremultiplies this float result, clips straight
linear channels to [0,1], encodes sRGB and rounds half upward to bytes; alpha rounds
independently. Exact zero alpha produces transparent black. Byte export therefore
cannot preserve out-of-gamut or hidden transparent colors. It is one explicit
output conversion, not a future renderer's mandatory framebuffer/color-space policy.

Frame x/y and sample x/y are finite in [-1e6,1e6]; frame w/h are finite positive
and <=1e6. Samples are logical points with no implicit pixel-center offset,
coverage, clip, corner radius, opacity or transform. There is no raster loop or
allocation in these functions. Per-pixel callers should decide their own sample
positions; physical preparation and GPU upload ownership remain future work.

Math is adapted from the already audited Plot transforms, without changing Plot,
effects or text-paint implementations. Source evidence is the pinned
[0.3.8 shader](https://docs.rs/crate/gpui-pre-wgpu/0.3.8/source/src/shaders.wgsl)
and [CSS interpolation](https://www.w3.org/TR/css-color-4/#interpolation-alpha)
and [gradient geometry](https://www.w3.org/TR/css-images-3/#linear-gradients).
Frozen independent Python reference numbers cover both models/spaces and unequal
rectangles. Tests additionally compare the existing source-HSLA Plot sampler and
run existing Plot/effects/text oracles. No new browser/native GPU output is claimed.
Python/JS/TS/Go/Rust typed exports, bridge/WASM exports and V2 draw support are absent.
