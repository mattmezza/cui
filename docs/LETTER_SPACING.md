# Letter spacing

Set `cui_text_style.letter_spacing` to a signed distance in logical pixels. A
value of `2` adds two logical pixels between eligible visual shaping clusters;
`-1` compresses them by one. Density scales the distance with the rest of the UI.
Zero preserves the existing natural shaping and layout path.

This changes the C ABI: rebuild Core, providers, renderers and applications
against the same headers. Zero-initialize style/config values before setting
individual fields, or use designated initializers. `cui_text_style`, embedded draw commands and
`cui_config` have grown. Do not mix prebuilt libraries or generated bindings
from an older build. The flat bridge style API does not yet expose tracking.

## Configure the provider

Tracking is opt-in. The provider must measure, shape contextual lines and draw
with the same cluster policy. Native `cui_text` with Bidi enabled supports it:

```c
cui_config config = {
    .measure_user = font,
    .measure_styled = cui_font_measure_styled,
    .text_geometry = cui_font_text_geometry,
    .text_geometry_line = cui_font_text_geometry_line,
    .text_capabilities = cui_font_text_capabilities(),
};
cui_context *ui = cui_create(config);

/* Inside a frame, within the desired available width. */
cui_text_style style = {.letter_spacing = 1.5f};
bool ok = cui_text_paragraph_colored(
    ui, cui_string("Clear typography"), 24,
    (cui_color){255, 255, 255, 255}, 30, style, true, NULL, 0);
```

`cui_text_capabilities(ui) & CUI_TEXT_CAP_LETTER_SPACING` reports the configured
provider promise; it requires both styled measurement and contextual geometry.
Custom providers may set the bit only when all their measurement, geometry and
rendering paths implement this contract. Their caches must include tracking
where they retain positioned shapes. A legacy measure callback is insufficient.
The capability does not guarantee that a particular font covers the text.

| Provider / renderer | Nonzero tracking |
| --- | --- |
| Native HarfBuzz + Bidi, software and SDL3 | Supported |
| Native build without Bidi | Unavailable |
| Canvas gallery provider | Unavailable |
| Sokol / SDL_GPU adapters | Rejected during preflight |

The native bridge host configures the capability, but its foreign style
representation still needs a separate additive tracking API. Other native hosts
must opt in through the configuration above. Manual glyph callbacks receive
already positioned glyph coverage; do not add spacing again.

Unsupported geometry returns `CUI_TEXT_LAYOUT_UNAVAILABLE`; measurement returns
zero, boolean submission/drawing returns false, and void text helpers submit
nothing. Prefer boolean paragraph/contextual APIs when failure matters. Invalid
values return the existing invalid/failure result. No provider silently counts
UTF-8 bytes, draws individual characters or substitutes solid default text.
Canvas `letterSpacing` is deliberately not used: it has not been verified to
match this shaping-cluster policy and the native caret/ligature contract.

## Shaping and negative values

HarfBuzz shapes complete directional/script runs first, preserving required and
optional ligatures, kerning, Arabic joining and combining-mark placement. A
ligature remains one cluster; tracking does not split it into individual letters.
Spacing is then inserted between positive-advance clusters in visual order,
including spaces and across visual run boundaries. Zero-advance clusters,
including standalone default-ignorables, earn no gap. Combining and emoji
sequences follow the shaper's cluster boundaries. This is cluster tracking, not
CSS per-character spacing.

There is no gap after the final eligible cluster of a visual line. Hard breaks
reset spacing; empty lines and a single cluster gain no width. Tabs retain the
provider's existing shaping behavior; this API introduces no tab stops.

Negative tracking can overlap glyph ink. Each eligible cluster's advance after
its following gap must remain nonnegative and all geometry must remain finite.
A value that would reverse progression rejects the line; it is never clamped.
Glyph ink may extend outside advance bounds, as it already can for italic text.
Cluster-edge carets and selection include the following gap. Hit testing remains
based on those edges, with its existing affinity rule for coincident positions.
Original glyph advances remain available in `cui_text_glyph`; adjusted glyph
positions, cluster spans and layout width describe the tracked result.

Wrapping measures the exact contextual candidate line, so its last cluster has
no trailing gap. It never breaks an intact shaped cluster. Tracked wrapping may
shape multiple candidate lines and cost more than natural wrapping; the zero
path retains its existing algorithm. As with ordinary contextual wrapping,
joining or ligatures can change at a chosen line boundary.

Color spans, gradients, alignment, clipping and inherited opacity use the same
tracked geometry. Paint changes do not change shaping. Native glyph raster
cache keys stay unchanged because tracking moves glyph origins and does not
change their glyph IDs, faces or raster sizes.

## Verification scope

`test_letter_spacing.c` checks real-font cluster and glyph identity, positive and
negative widths, no final gap, combining marks, ligatures, zero-width text,
caret/hit/selection consistency, narrow/wide reflow, unsupported providers and
software pixels. An optional Arabic font argument exercises joining, RTL and
mixed-direction runs. `test_letter_spacing_sdl.c` checks actual SDL pixels at
1×, 1.5× and 2×, including gradient coverage. Native tests run only through the
private-display wrapper. GPU hardware and unimplemented Canvas tracking are not
claimed as verified.
