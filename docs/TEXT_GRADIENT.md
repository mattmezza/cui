# Gradient text

`cui_text_paragraph_painted` paints real glyph coverage with a continuous linear
color field. One glyph can contain many colors. Wrapping, shaping, fallback,
contextual line sources, alignment, carets and selection retain their existing
geometry. The paint does not insert extra text runs or split shaping clusters.

```c
cui_text_gradient_stop stops[] = {
    {0, {236, 72, 153, 255}},
    {1, {59, 130, 246, 255}},
};
cui_text_paint paint = {
    .kind = CUI_TEXT_PAINT_LINEAR,
    .x0 = 0, .y0 = 0, .x1 = 1, .y1 = 0,
    .anchor = CUI_TEXT_ANCHOR_BLOCK,
    .stops = stops, .stop_count = 2,
};
bool ok = cui_text_paragraph_painted(ui, cui_string("A colorful heading"),
    36, 44, (cui_text_style){0}, true, CUI_TEXT_ALIGN_LEFT, paint);
```

Call inside an active frame. Check the returned boolean and frame error. Core
copies the 2–8 stops into frame storage; the caller may reuse its array after the
call. The emitted stop pointers and text remain borrowed until frame invalidation.
A retained draw-list snapshot must deep-copy both. `cui_typography_text_painted`
provides the same operation using a typography token. A zero-initialized solid
paint uses `paint.color` through the existing solid paragraph path.

## Coordinates and interpolation

Input endpoints are finite normalized coordinates in [0, 1], and must differ.
Use (0,0) to (1,0) for rightward paint, (0,0) to (0,1) for downward paint, or
(0,0) to (1,1) for a diagonal. Reversing endpoints reverses the direction.
`CUI_TEXT_ANCHOR_BLOCK` uses the complete paragraph layout container, including
hard breaks and empty rows. Every visual row receives the same resolved axis.
`CUI_TEXT_ANCHOR_LINE` restarts the axis in each visual row's advance box, after
alignment. Ink overhang can extend beyond that box. Anchors use layout line boxes,
not glyph ink bounds. Draw commands contain resolved global logical coordinates.
A resolved axis collapsed by an empty layout dimension samples its final stop.

Stop positions are finite, nondecreasing values in [0, 1]. Outside the first or
last stop, extend that stop's color. Equal positions make a hard edge: the last
stop at the exact position wins. Project each pixel center onto the axis.
Interpolate **premultiplied, encoded sRGB byte channels**, including alpha, then
unpremultiply and round to RGBA8. This is neither linear-light nor Oklab
interpolation. Transparent red to opaque blue therefore has a blue midpoint with
half opacity. Multiply the sampled alpha by glyph coverage. Ancestor opacity is
applied once to copied stop alpha in Core. Existing surface compositing follows.
This explicit contract does not promise CSS gradient color-space equivalence.

## Renderer support

Software and SDL advertise `CUI_RENDER_TEXT_LINEAR_GRADIENT` when a native font is
available. They sample actual glyph masks. SDL samples in logical coordinates at
the current raster density, preserving Retina positioning and existing solid
text cache behavior. Its temporary colored glyph surface is bounded to 4 MiB;
allocation or renderer errors return failure. Painted glyph results are not
cached by paint identity.

The Canvas module `web/modules/cui_text_paint.js` paints one complete shaped
visual row into a coverage mask, then samples the same field per physical pixel.
It preserves contextual shaping and italic ink overhang. Existing clipping,
alpha and compositing apply when the mask is drawn. Its temporary mask is bounded
to 4 MiB per row. Positive axis-aligned scale and translation are supported;
rotation, shear, invalid paint and oversized masks throw explicit errors.

Sokol and SDL_GPU reject gradient draw lists before clearing or submitting any
commands. They do not advertise the capability. GPU implementation and hardware
acceptance remain future work. Custom renderers must inspect the capability and
paint kind, and implement the documented sampling or reject the list. Ignoring
paint and drawing a solid substitute is unsupported.

Gradient paragraphs currently do not combine with byte color spans. Prepaint
writers reject gradient commands explicitly. There are no radial gradients,
textures, animated stop ownership, automatic selection colors or binding APIs in
this change. Selection highlighting remains a separate draw operation.

## Validation and integration

`tests/integration/test_text_gradient.c` checks copied stops, opacity, block and
line anchors, wrapping and identical bounds/caret/selection queries, hard edges,
transparent interpolation and two colors inside one actual native glyph.
`test_text_gradient_sdl.c` checks real captured glyph pixels and clips at densities
1, 1.5 and 2 under private Xvfb. `tests/web/test_text_gradient.py` runs headless
Chromium pixel checks at the same densities and compares contextual italic mask
coverage with a direct full-line Canvas fill. These checks do not constitute
macOS hardware or GPU renderer acceptance.

For integration, register the native tests with an explicit font fixture, copy
the new browser module with the gallery, and inline the private paint helper when
generating the Core amalgamation. Regenerate distribution/SDK artifacts together
with the public draw-command layout change. Test native builds through
`python scripts/test_local.py --build-dir <build>`; browser tests stay headless.

## Public gallery example

Open **Theme** in the public gallery, then change **Typography paint** from
**Solid text** to **Linear gradient text**. The typography sample uses the real
Core painted-paragraph API with copied pink-to-blue stops; the browser receives
the resolved axis and stops from WASM. The font family, size, weight and italic
controls continue to apply. The gradient's paragraph wraps within the sample
container. The solid sample keeps its existing single-line presentation.

The component's copy button supplies the painted-paragraph C example while the
gradient is selected. Its **Gradient text** guide link opens this guide. This
uses the existing Theme entry and preserves all 78 catalog families. It does not
reuse byte color spans as a gradient substitute.

`tests/web/test_gradient_gallery.py --directory <built-gallery>` loads the real
compiled gallery, changes the public paint control, checks serialized paint
stops and resolved coordinates, and reads actual glyph interiors. It exercises
desktop and 390px viewports at device pixel ratios 1 and 2, copyable code, the
return to solid paint and guide navigation. The guide must be included in the
public documentation generator before this integration test runs.
