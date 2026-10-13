# Portable backgrounds and bitmap effects

`cui_effects.h` generates reusable RGBA8 pixels in caller-owned storage. It uses
only core CUI types and portable C11, with no allocation or platform dependency.
All functions return false for invalid input before writing output. Row padding
is preserved. The caller must provide actual storage matching the declared sizes;
source `cui_bitmap` follows the core borrowed-storage contract.

`cui_effect_linear_gradient` accepts ordered color stops and a finite angle in
degrees. Zero runs left to right, 90 top to bottom, 180 right to left. Angles span
the projected pixel-center bounds, so the first and last projected corners sample
0 and 1 exactly. One-pixel images sample 0. Stops must increase strictly within
[0,1]; colors outside the stop interval extend to the edges. Interpolation uses
premultiplied sRGB values, then writes straight-alpha RGBA8. This avoids hidden
transparent colors leaking into visible gradient pixels; it is not linear-light
or Oklab interpolation.

`cui_effect_opacity` scales alpha without scaling RGB, which is correct for the
straight-alpha pixels required by `cui_bitmap`. Use `cui_bitmap_paint.opacity` for
draw-time fading when regenerated pixels are unnecessary. Existing `cui_box`
opacity also applies to descendants. Color fills use `cui_box.background`.

`cui_effect_blur` is a separable box blur with replicated edge samples. Its radius
is a pixel radius, not a CSS Gaussian blur length. It processes premultiplied RGBA
in caller-owned float scratch, then returns straight-alpha pixels; fully
transparent results have RGB zero. Query scratch with
`cui_effect_blur_scratch_floats`, allocate that many floats once, and reuse them.
Zero from this query indicates invalid dimensions or size overflow. Scratch must
be float-aligned, large enough, and disjoint from both images. Source/destination
may use identical pointers and strides, or disjoint storage; partial overlap is
rejected. Radius zero copies the original bytes, including transparent RGB.
The sliding-window implementation runs in O(width * height), including for very
large radii. This is a single box pass, not a Gaussian approximation.

Paint generated pixels through `cui_paint_bitmap` with an explicit opacity of 1
and a box radius for rounded corners. Alternatively use the existing bitmap
underlay API to place them behind content. Keep the pixels alive through renderer
completion; increment bitmap revision whenever pixels change so adapters can
refresh cached uploads. Reuse generated backgrounds instead of recalculating
unchanged gradients or blurs every frame.

A backdrop blur requires the host to capture the pixels behind the element and
supply that bitmap to `cui_effect_blur`. These primitives do not capture a native
window, sample earlier draw commands, or provide automatic CSS backdrop-filter
behavior. They can blur an image, a generated background, or a host-supplied
capture. Host capture coordinates, timing, and upload remain host responsibilities.

`examples/effects/background.c` provides a reusable slate-to-teal gradient tile
and a rounded painting helper. Initialize its caller-owned state once, then paint
it inside a CUI frame. This example has no host or renderer dependency.
