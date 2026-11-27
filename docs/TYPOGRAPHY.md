# Typography controls

`cui_text_ex` combines a logical pixel font size, an optional logical line height,
a color, and `cui_text_style`. Styles offer Normal, Medium, Semibold, and Bold
weights, independent italic and monospace flags, and an explicit family ID.
Use `cui_text_line` for unwrapped text with the same controls. The basic
`cui_text` API uses the default family's regular face and its measured height.
Font size and line height are independent: use the selected face's actual font
metrics when alignment requires its ascent, descent, or leading.

For reusable application styles, use the shipped header-only
[`cui_typography_roles` and `cui_typography_token` helpers](TYPOGRAPHY_TOKENS.md).
They resolve caller-owned body, heading and code defaults into a copy that can be
overridden locally; they do not mutate the theme or a global current style:

```c
#include "cui/cui_typography.h"

cui_typography_token title;
if (cui_typography_resolve(&roles, CUI_TYPOGRAPHY_HEADING, &title)) {
    title.align = CUI_TEXT_ALIGN_CENTER;
    bool submitted = cui_typography_text(ui, cui_string("Workspace"), title, ink);
    (void)submitted; /* Handle failure and end-frame diagnostics in the host. */
}
```

Here `roles` and `ink` are application-owned values; call inside an active frame.
Resolution validates only the selected role and preserves output on failure.
See the token guide for a full role initializer, color spans, exact native-face
registration, and wide/narrow composed tests. Use `cui_typography_text_painted`
for [linear gradient glyph paint](TEXT_GRADIENT.md) on supported renderers.
Tracking, variable axes and arbitrary numeric weights remain future work.

## Share size and color with a box

In immediate C, set text defaults on an open box to share a font size and solid
foreground with its descendants. This example aligns an icon and label and
gives them matching 28-pixel styles.

```c
#include "cui/cui_components.h"
#include "cui/cui_icon.h"

void sync_label(cui_context *ui, const cui_theme *theme) {
    cui_begin_box(ui, cui_id_from(ui, "sync-row"),
                  (cui_box){.row = true, .align_y = CUI_ALIGN_CENTER, .gap = 8});
    cui_set_text_defaults(ui, (cui_text_defaults){
        .font_size = 28, .custom_color = true, .color = theme->info});
    cui_icon_named(ui, cui_id_from(ui, "sync-icon"), theme, cui_string("check"),
                   (cui_icon_options){0});
    cui_label(ui, theme, cui_string("Sync complete"));
    cui_end_box(ui);
}
```

Icons use the scoped size and tint as fallbacks. Labels including
`cui_label_source` pick up both defaults. Rich labels use a natural line height
when inheriting size and keep their highlight and secondary colors. Plain typed
Kbd hints use both defaults. Keycap and literal hints keep their own styles.
A Spinner uses the scoped tint and keeps its configured size. Component-specific
explicit values override the defaults.

For nested boxes, a zero font size or `custom_color = false` inherits the
corresponding field from the enclosing box. Setting defaults again replaces
the current box's refinements. Core copies the values during submission,
including for nested absolute boxes and portals, and closing the box restores
the surrounding defaults. Use `cui_get_text_defaults` to opt a custom component
into the same behavior. The setter accepts finite logical sizes from 0 to 4096.
Explicit `cui_text` calls take their arguments directly. Font families, weights
and gradients remain explicit.

### Share defaults in a retained UI

Set defaults on a retained **Row** or **Column**. Nested containers can set size
and color independently. Each call replaces that container's refinements.
An omitted field inherits from the parent. Clearing both restores both parent
values. The app copies the settings and uses them on the next step.

Icon and Label follow both fields. Both Spinner presentations follow the tint
and keep their own size. Per-Icon size and tint options and rotating Spinner's
explicit tint still take priority. A Badge indicator keeps its semantic paint
while the contents inside the Badge can inherit.

In retained C, call the optional setter after creating the Row or Column and
between bridge steps. The last argument distinguishes inheritance from an
explicit transparent tint.

```c
/* app, status_column and status_row are already created. */
cui_bridge_set_text_defaults_v1(app, status_column, 24, 0x3060b0ff, 1);
cui_bridge_set_text_defaults_v1(app, status_row, 0, 0xe09020ff, 1);
/* Clear the row's refinements to inherit both from the column. */
cui_bridge_set_text_defaults_v1(app, status_row, 0, 0, 0);
```

| Language | Set a scope | Run the authored example from a checkout |
| --- | --- | --- |
| Python | `column().text_defaults(24, 0x3060b0ff)` | `PYTHONPATH=bindings/python CUI_BRIDGE_LIBRARY="$PWD/build/libcui_bridge.so" python bindings/python/examples/text_defaults.py` |
| JavaScript | `column().textDefaults(24, 0x3060b0ff)` | `node bindings/node/examples/text-defaults.cjs` |
| TypeScript | `column().textDefaults(24, 0x3060b0ff)` | `node bindings/typescript/examples/text_defaults.ts` |
| Go | `column.TextDefaults(cui.TextDefaults{FontSize: 24, ColorRGBA: &tint})` | `cd bindings/go && go run ./examples/text_defaults` |
| Rust | `app.text_defaults(column, TextDefaults { font_size: 24., color_rgba: Some(0x3060b0ff) })?` | `cd bindings/rust && cargo run --offline --example text_defaults` |

The examples render a nested workspace status UI. They start headless. Follow
the [language build guides](BINDING_BUILDERS.md) to build or select the bridge
and Node addon. The TypeScript command uses Node with built-in type stripping.
Python, Node, Go and Rust offer a native window option in their source examples.
For mixed provider versions, call `supports_text_defaults()` in Python, JS/TS
or Rust, or `SupportsTextDefaults()` in Go, before authoring this option. An
older provider can still mount and render a tree with these settings omitted.

## Use multiple font families

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
a separate font object and its registrations, then update the wrapper host
and renderer references together between frames and invalidate affected contexts.
If the context uses the font directly as its fixed `measure_user`, recreate that
context to bind another font, or change registrations on the existing font.
Keep the old font alive until its outstanding draw lists have been consumed. Fonts and paths are explicit resources; these APIs do not
fetch fonts, discover installed families, or synthesize bold or italic faces.
An unset named-family variant uses that family's regular face. Unknown families
fall back to the default family's requested style and then its regular face.

The native `cui_font_measure`, `cui_font_measure_styled`, and
`cui_font_get_metrics` functions already accept `void *` as their first
parameter and match the corresponding `cui_config` callback types. Direct
assignment is valid when `measure_user` is the font itself. No function-pointer
cast is needed. If your user pointer contains application state instead, use
wrappers that extract its font, as in this complete C11 program:

```c
#include "cui/cui_text.h"

typedef struct {
    cui_font *font;
} text_host;

static cui_size text_measure(void *user, cui_str text, float size) {
    text_host *host = user;
    return cui_font_measure(host->font, text, size);
}
static cui_size text_measure_styled(void *user, cui_str text, float size,
                                    cui_text_style style) {
    text_host *host = user;
    return cui_font_measure_styled(host->font, text, size, style);
}
static bool text_metrics(void *user, float size, cui_text_style style,
                         cui_font_metrics *out) {
    text_host *host = user;
    return cui_font_get_metrics(host->font, size, style, out);
}

int main(int argc, char **argv) {
    /* Paths: default regular face, editorial regular face, editorial bold face. */
    if (argc != 4)
        return 1;
    text_host host = {.font = cui_font_open(argv[1])};
    if (!host.font)
        return 1;
    cui_text_style body = {.family = 7};
    cui_text_style title = {.family = 7, .weight = CUI_TEXT_WEIGHT_BOLD};
    if (!cui_font_register_style(host.font, body, argv[2]) ||
        !cui_font_register_style(host.font, title, argv[3])) {
        cui_font_close(host.font);
        return 1;
    }
    cui_context *ui = cui_create((cui_config){
        .measure = text_measure,
        .measure_styled = text_measure_styled,
        .font_metrics = text_metrics,
        .measure_user = &host
    });
    if (!ui) {
        cui_font_close(host.font);
        return 1;
    }
    if (!cui_begin_frame(ui, 600, 300, .016f)) {
        cui_destroy(ui);
        cui_font_close(host.font);
        return 1;
    }
    cui_text_ex(ui, cui_string("Editorial heading"), 30,
                (cui_color){20, 20, 20, 255}, 36, title);
    cui_text_ex(ui, cui_string("Body copy"), 16,
                (cui_color){20, 20, 20, 255}, 24, body);
    cui_draw_list draws = cui_end_frame(ui);
    int status = draws.count && cui_last_error(ui) == CUI_OK ? 0 : 1;
    /* A real host paints draws with host.font before ending its lifetime. */
    cui_destroy(ui);
    cui_font_close(host.font);
    return status;
}
```

Save the complete program as `typography.c`. With the default native library
build and existing FreeType/HarfBuzz development packages, compile it with:

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude typography.c build/libcui_text.a build/libcui.a $(pkg-config --cflags --libs freetype2 harfbuzz) -lm -o typography
./typography /path/default-regular.ttf /path/editorial-regular.ttf /path/editorial-bold.ttf
```

Link the optional native `cui_text` adapter and Core; its FreeType/HarfBuzz
requirements are described in [TEXT.md](TEXT.md). This example submits a draw
list; the application supplies its renderer and window/event loop. The host
struct and font must outlive the context's measurement callbacks and every draw
that uses them. A custom renderer maps the complete style, including family,
to the same faces used for measurement. Reuse stable family IDs, and call
`cui_text_metrics_changed` between frames in every affected context after
changing registrations.

See [TEXT.md](TEXT.md) for ownership, fallback ordering, native registration,
thread confinement, and headless test fixture details. These controls are text
primitives; they do not imply that every component has a per-control family
option or that TextView supports arbitrary family changes within a shaped run.
