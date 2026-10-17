# Reusable typography tokens

`cui_typography.h` is a header-only authoring convenience over existing Core text
APIs. It gives applications three semantic roles without owning fonts, modifying
the current theme or introducing new layout behavior.

```c
#include "cui/cui_typography.h"

const cui_typography_roles type = {
    .body = {.style = {.family = 11}, .size = 16.5f,
             .line_height = 26, .wrap = true},
    .heading = {.style = {.family = 12, .weight = CUI_TEXT_WEIGHT_BOLD},
                .size = 30, .line_height = 38, .wrap = true,
                .align = CUI_TEXT_ALIGN_CENTER},
    .code = {.style = {.family = 13, .monospace = true},
             .size = 14, .line_height = 23, .wrap = false}
};
cui_typography_token heading;
if (cui_typography_resolve(&type, CUI_TYPOGRAPHY_HEADING, &heading)) {
    /* In an open frame, inside a constrained layout parent. */
    bool submitted = cui_typography_text(ui, cui_string("A clear heading"), heading,
                                         (cui_color){24, 30, 40, 255});
    (void)submitted; /* Real applications handle submission/capacity errors. */
}
```

Roles are ordinary caller-owned values. Resolution copies the selected token;
change the copy to override one call without changing defaults. Only the selected
role must be valid. Invalid roles/tokens preserve the output. Valid sizes are
finite logical pixels in `(0, 65535]`; line height is finite in `[0, 65535]`, with
zero selecting natural metrics. Alignment is physical left/center/right. Weight
uses the existing normal/medium/semibold/bold enum. There is no implicit scaling,
inheritance, mutable current style or guessed font registration.

`cui_typography_text_colored` accepts existing paragraph-relative UTF-8 byte color
spans. Both submission helpers use `cui_text_paragraph_colored_aligned`, retaining
strict UTF-8 validation, grapheme-safe wrapping and contextual geometry when the
host supplies it. `wrap=false` retains hard breaks but may overflow; use a clipping
ancestor where needed. A constrained parent width is required to demonstrate
wrapping. Explicit line height sets spacing, not glyph stretching. Check the
helper result and the existing end-frame error contract for layout failures.

`cui_typography_text_painted(ui, text, token, paint)` uses the same token geometry
with a solid or copied linear text paint. See [gradient text](TEXT_GRADIENT.md)
for stops, directions, paragraph/line anchors, renderer capabilities and lifetime.
This helper does not combine gradient paint with byte color spans.

## Native font registration and composed example

`examples/typography/app.c` composes a centered heading, wrapped body, an italic
local override and clipped unwrapped code. All three families have distinct IDs.
Compile with `CUI_TYPOGRAPHY_NATIVE` to enable `typography_example_font`: it takes
five explicit file paths (body regular, heading regular, heading bold, body
italic, code regular), opens an owner and registers the exact styles used by the
example. Failure closes the partially built owner and returns NULL. There is no
OS name lookup or synthetic italic/bold claim. The returned owner must outlive
the renderer/context use and be closed by the host.

Configure the native context and renderer with the same font owner:

```c
cui_config config = {
    .width = 900, .height = 700,
    .measure_styled = cui_font_measure_styled, .measure_user = font,
    .font_metrics = cui_font_get_metrics,
    .text_geometry = cui_font_text_geometry,
    .text_geometry_line = cui_font_text_geometry_line
};
```

Then call `typography_example_frame` from the native host's frame loop and submit
its draw list to the existing renderer. The example itself creates no window.
After changing registrations, call `cui_text_metrics_changed` between frames.
Custom hosts and Canvas must apply the same family mapping in measurement and
paint; tokens do not make browser and native shaping identical.

`tests/unit/test_typography_tokens.c` checks validation, immutable resolution,
failed-input atomicity, styled/color-span forwarding and centered geometry.
`examples/typography/test_app.c` checks family/style/float-size commands, body
baseline spacing, wrapped line counts and stable roles across wide/narrow/wide
frames. Compiled with `CUI_TYPOGRAPHY_NATIVE`, it requires all five explicit path
environment variables `CUI_TYPOGRAPHY_BODY`, `CUI_TYPOGRAPHY_HEADING`,
`CUI_TYPOGRAPHY_HEADING_BOLD`, `CUI_TYPOGRAPHY_BODY_ITALIC`, `CUI_TYPOGRAPHY_CODE`;
without them it returns 77. It compares native measurement and raster output for
each used exact variant against the separately opened actual file. Use distinct
family files to exercise the intended body/heading/code configuration.

## Extending typography later

These tokens expose supported properties only. Tracking, numeric weight/stretch
and variable axes require complete provider, measurement, geometry and cache
contracts before authoring fields can be added. A future
extended layout/paint descriptor can have a new token type and helpers while
keeping these functions as wrappers for zero tracking and solid/span paint.
Current source callers need no migration to use this convenience header; font
registration, context callbacks and existing text entry points remain available.
