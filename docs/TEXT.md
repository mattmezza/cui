# Styled text and font faces

`cui_text_ex` and `cui_measure_text_ex` take `cui_text_style`: Normal, Medium, Semibold or Bold weight, plus independent italic and monospace flags and an application-defined `uint32_t family` ID. Family zero selects the default; nonzero IDs identify separately registered families. `cui_text_ex` also accepts an optional logical line height. The older `cui_text`, `cui_text_with_line_height` and `cui_measure_text` APIs use regular text. The style travels through Clay measurement and the draw list to the software, SDL3, SDL_GPU and Sokol renderers.

Set `cui_config.measure_styled = cui_font_measure_styled` and `measure_user = font` when creating a context with a native `cui_font`. The renderer must use the same font object. The legacy `measure` callback remains available; if `measure_styled` is absent, layout uses that callback for every style. Native selection uses the exact family/style registration first, then that named family's registered regular face, then the default family's matching style, then the opened regular face. Measurement, font metrics, and drawing use the same selection. CUI does not synthesize bold, italic or monospace glyphs from a regular face.

Use `cui_font_register_style(font, style, path)` to open an actual family or variant face owned by the regular font. Registrations are keyed by family, weight, italic, and monospace. A nonzero family can register its regular style; the default family's regular face remains the one opened by `cui_font_open`. A failed path leaves the previous registration intact; replacing a variant or closing the regular font releases the owned face. `cui_font_set_style(font, style, variant)` borrows a face opened by the caller. Keep borrowed faces alive through every frame that uses them, then close them separately. Call `cui_text_metrics_changed(ui)` between frames after changing registrations so Clay remeasures cached text.

The gallery registers Noto Sans Medium, Semibold, Bold and Italic plus Noto Sans Mono only when the selected regular font is the known Noto Sans path. A custom `--font` or `CUI_FONT_PATH` retains its chosen family and uses regular fallback until the application registers matching variants. The renderer swap demos configure styled measurement with their selected font; they do not assume matching variant paths for custom fonts.

`cui_measure_font_metrics(ui, size, style, &metrics)` queries the actual selected face's ascent, nonnegative descent and renderer line height in logical pixels. Set `cui_config.font_metrics = cui_font_get_metrics` with the same `measure_user = font` for the FreeType adapter. Custom hosts supply the optional `cui_font_metrics_fn` callback; it shares the measurement user pointer and receives the complete text style. A missing provider, invalid input or malformed callback result returns false and preserves the output. This API deliberately supplies no guessed baseline from string height. Line spacing can be smaller than ascent plus descent, and callers must account for leading when positioning inline content.

The native gallery and renderer swap examples register the provider. Core tests cover unavailable providers, style and fractional-size forwarding, live metric changes, invalid results and output preservation; native tests compare selected variants to their underlying faces and check metric scaling against measured line height. This is infrastructure for inline-object layout; it does not by itself implement TextView's mixed text/image flow or complete shaping-aware selection.


For example, choose a stable application ID such as `7`, register its regular
face with `(cui_text_style){.family = 7}`, and register its bold face with
`(cui_text_style){.family = 7, .weight = CUI_TEXT_WEIGHT_BOLD}`. Pass those styles
to both measurement and drawing. Missing variants remain in the named family
when a regular face is registered; CUI does not synthesize variants. Family IDs
are opaque keys, not operating-system font names or automatic font discovery.
The native registry allocates sparse entries for registered IDs and supports the
full nonzero `uint32_t` range. Removing a family's regular registration leaves
its separately registered variants intact. Removing an absent style succeeds.

One caller-opened face can be borrowed by several registrations. Replacing or
removing a borrowed registration does not close that face, and closing the
registration owner leaves borrowed faces alive. Path registration opens an
independent owned face; replacement, removal, and owner close release it. Keep
borrowed faces alive while any measurement or draw can use them, and close them
once separately. Face operations remain confined to the UI thread. Never retain
an owned face pointer beyond its registration lifetime.

Core style interning includes family alongside size, line height, weight, italic,
and monospace, so equal text at equal size in different families has distinct
layout cache keys. After changing any registration, call
`cui_text_metrics_changed(ui)` between frames for every context using the font.
A custom styled measurement provider receives the family unchanged; a legacy
measurement provider may use one fallback face for all families. A custom
renderer or browser host must apply the same family mapping used for measurement.
Zero-initialized styles keep default-family behavior; rebuild consumers after
changing public headers because the style structure has gained a member.

`tests/integration/test_text.c` compares actual proportional and monospace faces
across measurement, font metrics, glyph raster coverage, core layout, fallback,
registration failure, replacement, aliases, and owned/borrowed close lifetimes.
Set `CUI_TEST_FONT` and `CUI_TEST_FAMILY_FONT` to distinct font files when system
fixtures are unavailable. The test returns 77 when no fixture pair is found.
