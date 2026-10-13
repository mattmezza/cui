# Software triangle meshes

The deterministic software adapter implements `CUI_DRAW_MESH` and advertises
`CUI_RENDER_MESH`. Expanded vertex triples are borrowed and never modified.
Only XY is transformed by positive scale and origin; ST is interpolated in
screen space. Command bounds do not crop mesh geometry: ancestor clip commands
and the framebuffer viewport determine clipping.

Each pixel is sampled at its center. Triangles are oriented consistently and
use a top-left edge rule, so opposite sides of an exactly shared edge cannot
both blend the same center. Degenerate triangles are empty geometry. Double
precision edge, derivative, and interpolation calculations avoid float product
overflow; bounding boxes are intersected with the clip and viewport before any
integer conversion. Extremely ill-conditioned geometry remains subject to
finite floating-point precision.

Analytic coverage follows GPUI's pre-WGPU 0.3.8 `fs_path_rasterization` shader:
when `length(vec2(dSdx, dSdy)) < .001`, coverage is one. Otherwise it is
`clamp(.5 - (S*S-T)/length(2*S*gradS-gradT), 0, 1)`. Both screen derivatives
participate. At an exactly zero gradient, where shader division is undefined,
the software adapter defines coverage as one for negative `S*S-T`, zero for
positive, and one half for zero. Triangle boundaries themselves are sampled
without extra geometric antialiasing.

Uniform linear premultiplied color is unpremultiplied, clamped to SDR RGB,
sRGB encoded, and quantized to bytes; source alpha is multiplied by analytic
coverage and rounded to a byte. Rendering then uses the adapter's existing
straight-alpha byte-domain source-over blend. This does not provide linear-light
composition, HDR output, GPU bit identity, or full GPUI path API parity.

All meshes in the list are validated before clearing the framebuffer. Invalid
vertex counts, nonfinite inputs, invalid alpha/scale, and transformed bounds
outside the public mesh validity contract return false and preserve the target.
Other command failures retain the software adapter's existing partial-render
behavior. Borrowed arrays must remain valid through render completion.

`tests/unit/test_render.c` checks numeric sRGB/alpha oracles, partial and opaque
coverage with horizontal and vertical S derivatives and nonconstant T, shared
edges in both windings, transforms, clipping, immutable inputs, degenerate and
extreme geometry, and rejection before any framebuffer mutation.
