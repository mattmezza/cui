# Deterministic software renderer

This adapter allocates an owned, straight-alpha RGBA8 image with dimensions up to
16384 by 16384. Resize is transactional: allocation failure preserves the old
image. Pixel pointers remain valid until the next resize or destruction.

Each render clears the image, composites solid and rounded geometry, draws
rounded borders and intersects nested clipping rectangles (maximum depth 64).
Coverage is tested at pixel centers; geometry edges are not antialiased. Alpha
composition preserves straight RGB even when destination alpha is zero.
Malformed clipping stacks and nonfinite geometry return `false`.

With `CUI_HAS_TEXT`, the caller may supply an owned `cui_font` to render shaped
grayscale glyphs. Keep that font alive until renderer destruction. Without a
font or text support, text commands return `false`; the text capability is unset.
Geometry-only compilation requires the C11 standard library and math library,
without SDL, FreeType or HarfBuzz.

PPM capture exports RGB and discards alpha; it is intended for opaque screenshot
frames. No GPU acceleration or native accessibility is claimed.

The unreleased C-only V2 route uses `cui_software_query_v2` before composition and
`cui_software_render_v2` for bounded transactional rendering; it is separate from
the unchanged V1 function table. It supports owned backgrounds, AREA shadows and
independently scaled grayscale text with explicit limits. See
[the software V2 guide](../../docs/SOFTWARE_V2.md) for query/defaults, failures,
resource/font ownership and busy destroy retry. Other adapters/bindings remain
pending. The private guard adds8 bytes on measured native/WASM32 objects; V1 field
offsets/table/pixels and ordinary allocation call counts are preserved.
