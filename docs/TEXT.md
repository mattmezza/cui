# Styled text and font faces

`cui_text_ex` and `cui_measure_text_ex` take `cui_text_style`: Normal, Medium, Semibold or Bold weight, plus independent italic and monospace flags and an application-defined `uint32_t family` ID. Family zero selects the default; nonzero IDs identify separately registered families. `cui_text_ex` also accepts an optional logical line height. The older `cui_text`, `cui_text_with_line_height` and `cui_measure_text` APIs use regular text. The style travels through Clay measurement and the draw list to the software, SDL3, SDL_GPU and Sokol renderers.

Set `cui_config.measure_styled = cui_font_measure_styled` and `measure_user = font` when creating a context with a native `cui_font`. These native callbacks take `void *` and match the Core callback signatures directly; do not cast function pointers. If `measure_user` points to an application host struct, extract its font inside correctly typed wrappers; see the complete [typography example](TYPOGRAPHY.md). The renderer must use the same font object. The legacy `measure` callback remains available; if `measure_styled` is absent, layout uses that callback for every style. Native selection uses the exact family/style registration first, then that named family's registered regular face, then the default family's matching style, then the opened regular face. Measurement, font metrics, and drawing use the same selection. CUI does not synthesize bold, italic or monospace glyphs from a regular face.

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


The native integration test also configures Core through wrappers whose shared
user pointer is a host struct containing the font. It checks styled family
forwarding against actual layout/raster output and routes the Core font-metrics
query through that same host. A wrapper's first parameter must remain `void *`;
a function taking `cui_font *` directly is a different C function-pointer type,
even though object pointers can be converted inside the wrapper.

`cui_text`, `cui_text_with_line_height`, and `cui_text_ex` all word-wrap to the
available width of their layout parent. Give the parent a constrained width
(for example, `(cui_box){.width = 240, .fixed_width = true}`) to lay out a
paragraph; a content-sized parent has no narrower width to wrap against. Explicit
line height and font style do not change wrapping. `cui_text_line` preserves a
caller-laid-out line and may overflow its parent, so use a clipping ancestor
when overflow must remain hidden. For strict UTF-8 paragraphs with explicit hard
breaks and optional color spans, `cui_text_paragraph_colored` selects word wrapping
with its `wrap` argument.

## Embed fonts in a native executable

`cui_font_open_memory(const void *data, size_t size)` opens the first face in
an in-memory font using the native `cui_text` adapter. It performs no font file
I/O and does not copy the bytes. This supports fonts compiled directly into
an executable, with no font files to install beside it.

For example, generate a C header from a font you have permission to redistribute:

```sh
python3 - <<'PYFONT'
from pathlib import Path
font = Path("assets/App-Regular.ttf").read_bytes()
with Path("app_font.h").open("w") as out:
    out.write("static const unsigned char app_font[] = {\n")
    for start in range(0, len(font), 16):
        out.write(",".join(str(b) for b in font[start:start + 16]) + ",\n")
    out.write("};\n")
PYFONT
```

Include the generated header in one C source file:

```c
#include "cui/cui_text.h"
#include "app_font.h"

/* During application initialization: */
cui_font *font = cui_font_open_memory(app_font, sizeof app_font);
if (!font) {
    /* Report initialization failure. */
    return 1;
}
/* Use font for native measurement and rendering as with cui_font_open(). */
/* After destroying the UI and any retained text layouts: */
cui_font_close(font);
```

The buffer is borrowed and must stay immutable and valid until the actual face
is released. A layout can retain a face after `cui_font_close`, so a dynamically
allocated buffer must also outlive those layouts. A `static const` array stays
valid for the entire process and needs no separate cleanup. NULL, zero-length,
oversized, or invalid font data returns NULL.

Embed bold, italic, and fallback fonts as separate arrays, open each with
`cui_font_open_memory`, and register their handles with `cui_font_set_style` or
`cui_font_set_fallbacks`. These registrations borrow handles; keep the faces
alive while registered and close them separately after use. File-based and
memory-based faces can be mixed.

Embedding increases executable size; large multilingual fallback sets may be
better shipped separately. Check each font's redistribution and embedding
license. Embedding assets is independent of static linking: link CUI and its
native dependencies statically as well if your distribution requires that.
This API concerns native fonts; browser font loading continues to use the web host.

## Native letter spacing

See [Letter spacing](LETTER_SPACING.md) for the opt-in cluster tracking API,
provider capability checks, ABI rebuild requirement, negative-spacing policy,
and renderer/binding limitations. Canvas and GPU tracking are not implemented.
