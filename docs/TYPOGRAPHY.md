# Typography controls

`cui_text_ex` combines a logical pixel font size, an optional logical line height,
a color, and `cui_text_style`. Styles offer Normal, Medium, Semibold, and Bold
weights, independent italic and monospace flags, and an explicit family ID.
Use `cui_text_line` for unwrapped text with the same controls. The basic
`cui_text` API uses the default family's regular face and its measured height.
Font size and line height are independent: use the selected face's actual font
metrics when alignment requires its ascent, descent, or leading.

Family IDs are application-defined `uint32_t` keys. Zero retains the default
family. Register regular and variant faces for a named family on the same
`cui_font` used by the context's measurement callbacks and renderer:

```c
const uint32_t editorial = 7;
cui_text_style body = {.family = editorial};
cui_text_style title = {.family = editorial, .weight = CUI_TEXT_WEIGHT_BOLD};
/* Check registration results and supply application-owned font paths. */
bool ready = cui_font_register_style(font, body, body_path) &&
             cui_font_register_style(font, title, bold_path);
if (ready) {
    cui_text_metrics_changed(ui); /* Between frames, before the next layout. */
    /* Inside the next frame: */
    cui_text_ex(ui, cui_string("Editorial heading"), 30, ink, 36, title);
}
```

Registration is per call, so a successful regular registration remains if the
following bold registration fails. For an atomic application font switch, prepare
a separate font object and its registrations before switching measurement and
renderer references. Fonts and paths are explicit resources; these APIs do not
fetch fonts, discover installed families, or synthesize bold or italic faces.
An unset named-family variant uses that family's regular face. Unknown families
fall back to the default family's requested style and then its regular face.

Set `cui_config.measure_styled = cui_font_measure_styled`,
`cui_config.font_metrics = cui_font_get_metrics`, and `measure_user = font` for
native FreeType/HarfBuzz text. A custom host provides equivalent callbacks and
maps the complete style, including family, when drawing. Measurement and drawing
must agree on their selected face. Reuse stable family IDs, and invalidate text
metrics in every affected context when registered faces change.

See [TEXT.md](TEXT.md) for ownership, fallback ordering, native registration,
thread confinement, and headless test fixture details. These controls are text
primitives; they do not imply that every component has a per-control family
option or that TextView supports arbitrary family changes within a shaped run.
