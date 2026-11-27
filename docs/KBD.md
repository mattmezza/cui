# Keyboard shortcut hints

`cui/cui_kbd.h` adds `cui_kbd_ex` for a typed **single chord**, preserving the
existing literal `cui_kbd` and byte-preserving `cui_kbd_format` APIs unchanged.
It displays hints. An application can look up the stroke through a host-owned
keymap and use that same keymap to handle input.

The frozen source stores one Keystroke. Binding lookup takes the first stroke;
several hints in its story are ordinary sibling components. The earlier
“grouped shortcut rendering” manifest phrase does not identify a missing
multi-shortcut source API. See the pinned [audit](design/KBD_SOURCE_AUDIT.md).
Compose hints normally, as in this complete C11 draw function:

```c
#include "cui/cui_kbd.h"

bool draw_shortcut_hints(cui_context *ui, const cui_theme *theme,
                          bool mac_symbols) {
    cui_begin_box(ui, 100, (cui_box){.row = true, .gap = 8});
    bool palette = cui_kbd_ex(ui, 101, theme,
        (cui_kbd_stroke){.modifiers = CUI_MOD_META | CUI_MOD_SHIFT,
                         .key = CUI_STR("ß")},
        (cui_kbd_options){.mac_symbols = mac_symbols});
    bool escape = cui_kbd_ex(ui, 102, theme,
        (cui_kbd_stroke){.key = CUI_STR("escape")},
        (cui_kbd_options){.outline = true, .mac_symbols = mac_symbols,
                          .accessible_name = CUI_STR("Escape key")});
    cui_end_box(ui);
    return palette && escape;
}
```

Call inside an active frame. The host selects platform convention explicitly:
CTRL/ALT/SHIFT/META display in that order, as text with `+` or macOS symbols
without separators. `META` is Win or Command. Host parser aliases such as
`secondary` must be resolved by the host; the component does not guess a keymap.
Known lower-case key names have platform labels including Esc, Enter/⏎,
Backspace/⌫, Delete/⌫, arrow names or symbols and Page Up/Page Down. Other keys
apply Unicode 17.0 default uppercase to the first scalar. For example, `éclair`
becomes `Éclair` and `ßeta` becomes `SSeta`. The remainder of the key stays
byte-for-byte as supplied. Locale and surrounding text do not affect the
mapping. Function keys and literal `+`/`-` work without a parser.

`cui_kbd_format_stroke` returns required bytes **excluding NUL**. It accepts
NULL output for size queries and capacity0 without writing; positive capacity
always NUL-terminates valid output within that capacity, truncating only complete
UTF-8 scalars. Invalid input returns SIZE_MAX and leaves the output unchanged.
Input key must be nonempty strict single-line UTF-8, at most128 bytes, with no
embedded NUL or ASCII control characters. Unknown modifier bits are invalid.
The maximum result is 151 bytes excluding NUL, including four modifiers
and up to four extra bytes for the first scalar. Allocate
`CUI_KBD_TEXT_CAPACITY` bytes for a complete label, including the terminator.
The output buffer may overlap the input key. Use `cui_kbd_format` when you
need the legacy byte-preserving behavior.

Zero options draw the source-shaped muted pill: muted foreground/background,
12-unit text at16-unit rem, horizontal padding4, vertical padding2, minimum
width20 and half theme radius. Outline adds a1-unit border and uses the theme
background. `plain=true` wins over outline and renders text using the enclosing
box's scoped size and solid color, falling back to the theme per field. Plain
hints have no pill padding, border or minimum width. CUI maps rem to
`theme.font_size/16`; the font must be finite(0,512], radius finite/nonnegative.

Hints are noninteractive LABEL semantics, excluded from tab traversal. The
accessible name defaults to formatted text; an explicit name overrides it.
Names must be strict single-line UTF-8 up to1024 bytes, without embedded NUL
or ASCII controls. Invalid arguments fail before layout or semantics. Input
strings and local formatted output are copied into Core frame storage at
submission and survive through that frame's renderer consumption. No retained
component state, allocator or cleanup is required.

## Display a configurable binding

Give the hint the same keymap used by the host to dispatch actions. A scope of
`CUI_KBD_BINDING_CURRENT` asks for the active key context.
`CUI_KBD_BINDING_CONTEXT` takes an application context ID in `target`.
`CUI_KBD_BINDING_FOCUS` takes a focus target ID in `target`, which the host maps
to its context. `CUI_KBD_BINDING_GLOBAL` asks for the empty context. The host
resolves precedence, context and dispatch, including multi-stroke sequences.

```c
#include "cui/cui_kbd.h"

enum { REFRESH_RESULTS = 1 };

/* Pass the winning sequence for the active area from your application keymap. */
typedef struct {
    const cui_kbd_stroke *refresh;
    size_t refresh_count;
} workspace_keymap;

static size_t lookup_workspace(void *user, const cui_kbd_binding_query *query,
                               const cui_kbd_stroke **strokes) {
    const workspace_keymap *keys = user;
    if (query->action != REFRESH_RESULTS || query->scope != CUI_KBD_BINDING_CURRENT)
        return 0;
    *strokes = keys->refresh;
    return keys->refresh_count;
}

void draw_refresh_hint(cui_context *ui, cui_id hint, const cui_theme *theme,
                       workspace_keymap *keys) {
    cui_kbd_stroke first;
    if (cui_kbd_binding_stroke((cui_kbd_binding_source){lookup_workspace, keys},
                               (cui_kbd_binding_query){.action = REFRESH_RESULTS}, &first))
        cui_kbd_ex(ui, hint, theme, first, (cui_kbd_options){0});
}
```

Return the full winning sequence from the lookup. CUI validates and borrows
the first stroke. Keep its key bytes alive until you submit the hint or copy
them. A missing or invalid binding leaves the output unchanged. An unbound
action can simply omit its hint. The gallery's **Use alternate shortcut**
option rebinds **Refresh results** from Shift+P to Shift+K. Focus the preview
and press the displayed shortcut to refresh.

Python, JavaScript, TypeScript, Go and Rust resolve actions in application
code and pass the selected first stroke through the typed builder or update
method below. The synchronous lookup hook is available in C.

## Style the keycap

`cui_kbd_styled` takes an optional `cui_kbd_style` for the normal and outline
keycaps. Each null field uses that mode's defaults. A supplied `cui_box`
replaces the entire pill box, including its fill, border, rounding, padding
and size, so copy the values you want to keep. A `cui_typography_token` selects
the font family, weight, line height, alignment and wrapping. Leave the box
unset for an intrinsically sized pill measured using the chosen font. To wrap
a hint as its parent narrows, set a bounded box width, keep the height at zero
for fit, and set `typography.wrap = true`. Use a supported font provider for
tracking and a supported renderer for linear text paint.

```c
#include "cui/cui_kbd.h"

bool draw_styled_shortcut(cui_context *ui, const cui_theme *theme, cui_id id) {
    cui_box keycap = {
        .width = -1, .max_width = 176, .padding = 6, .radius = 8,
        .background = theme->muted_background,
        .border = theme->border, .border_width = 1,
    };
    cui_typography_token type = {
        .size = 14, .line_height = 20, .align = CUI_TEXT_ALIGN_CENTER,
        .wrap = true, .style = {.monospace = true},
    };
    cui_text_gradient_stop stops[] = {{0, theme->text}, {1, theme->accent}};
    cui_text_paint paint = {
        .kind = CUI_TEXT_PAINT_LINEAR, .x1 = 1, .stops = stops, .stop_count = 2,
    };
    return cui_kbd_styled(ui, id, theme,
        (cui_kbd_stroke){.modifiers = CUI_MOD_ALT, .key = CUI_STR("pagedown")},
        (cui_kbd_options){0},
        &(cui_kbd_style){.box = &keycap, .typography = &type, .text_paint = &paint});
}
```

Use `cui_kbd_label_styled` to apply the same style to a preformatted literal.
The given label is also its accessible name. Plain typed hints use the enclosing
size and solid color and ignore keycap overrides. Style values are read during
the call. Gradient stops are copied for that
frame, so local arrays are safe. Invalid typography and gradient definitions
fail before the hint is submitted. To add a shadow, use
`cui_element_set_shadow_preset` on the submitted keycap ID. Backgrounds use
Core's solid box fill.
See [shared typography](TYPOGRAPHY.md) for renderer and provider requirements.

Ordinary row composition fits its contents; it does not automatically turn a
shortcut list into a responsive wrapping grid. For narrow help panels, let the
owner split rows using the host's available-width/layout policy. A hint renders
one stroke. The host owns sequence recognition and activation.

## Show a shortcut in a retained UI

Use a Kbd node beside the action it describes. The same formatter handles
special key names, Unicode uppercase, and native-style modifier labels in all
five language builders. Choose the macOS convention explicitly. Supply an
accessible name when the symbols alone give too little spoken context.

```python
from cui_bridge import App, column, kbd, kbd_stroke, label, row

with App(backend="none", width=400, height=180) as app:
    hint = (kbd_stroke("keys", "k", modifiers=2 | 1, outline=True,
                       accessible_name="Control Shift K")
            if app.supports_kbd_stroke() else kbd("keys", "Ctrl+Shift+K"))
    if app.supports_kbd_style():
        hint.kbd_style({"max_width": 128, "wrap": True, "font_size": 14,
                        "line_height": 20, "monospace": True, "radius": 6})
    app.mount(column("root").children(
        row("shortcut").children(label("action", "Open command palette"), hint)
    ))
    app.step()
    print(app.bounds(hint))
```

```javascript
const {App, column, kbd, kbdStroke, label, row} = require('@cui/native');
const app = new App({backend: 'headless', width: 400, height: 180});
try {
  const hint = app.supportsKbdStroke()
    ? kbdStroke('keys', 'k', {modifiers: 2 | 1, outline: true, accessibleName: 'Control Shift K'})
    : kbd('keys', 'Ctrl+Shift+K');
  if (app.supportsKbdStyle()) {
    hint.setKbdStyle({
      maxWidth: 128, wrap: true, fontSize: 14, monospace: true, radius: 6,
      textGradient: {stops: [
        {position: 0, color: 0x0e7490ff},
        {position: 1, color: 0x7e22ceff},
      ]},
    });
  }
  app.mount(column('root').children(
    row('shortcut').children(label('action', 'Open command palette'), hint)
  ));
  app.step();
  console.log(hint.bounds());
} finally {
  app.dispose();
}
```

The scripts run headlessly. See the [language guide](LANGUAGE_BINDINGS.md) for installation,
opening a native window and running the application loop. TypeScript uses the
same `kbdStroke` function and exports `KbdStrokeOptions`. Python and JS/TS use
modifier bits Shift = 1, Control = 2, Alt = 4 and Meta = 8.

Go creates the hint with
`cui.KbdStroke("keys", "k", cui.KbdStrokeOptions{Modifiers: cui.KbdCtrl | cui.KbdShift, Outline: true})`.
Probe `app.SupportsKbdStroke()` before choosing the typed or literal factory.
`hint.ConfigureKbdStroke(key, options)` updates a mounted hint on the owner
thread. Rust probes `app.supports_kbd_stroke()` and creates the hint with
`app.child_kbd_stroke(parent, "keys", "k", &options)?`. Use
`KbdModifiers::CTRL | KbdModifiers::SHIFT` in `KbdStrokeOptions`, and use
`app.kbd_stroke(node, key, &options)?` to update it.

In retained C, create a `CUI_BRIDGE_KBD` node and configure it with
`cui_bridge_set_kbd_stroke_v1`. Pass explicit lengths in bytes. The bridge
copies the key and accessible name during the call. The key takes 1–128
UTF-8 bytes and an optional name takes up to 1024 bytes. The setter rejects
multiline strings and ASCII control characters. A failed call keeps the
previous hint. An empty accessible name uses the formatted label. Plain
presentation takes precedence over outline. Typed hints report a passive
label and emit no interaction events.

Passing zero flags, zero modifiers, two NULL pointers and two zero lengths
restores the literal held by the node. The literal remains available through
`cui_bridge_get_text` and can be changed with `cui_bridge_set_text`. Older
providers keep the literal API. The language probes detect the optional
export before the typed node is attached.

### Fit the retained keycap to its container

Style is optional for both literal and typed hints. Probe it independently of
typed stroke support. The alpha.14 provider supports typed and literal
labels using their default styles.

| Language | Capability probe | Replace or restore a style |
| --- | --- | --- |
| Python | `app.supports_kbd_style()` | `hint.kbd_style({"max_width": 128, "wrap": True})` or `hint.kbd_style(None)` |
| JavaScript / TypeScript | `app.supportsKbdStyle()` | `hint.setKbdStyle({maxWidth: 128, wrap: true})` or `hint.setKbdStyle(null)` |
| Go | `app.SupportsKbdStyle()` | `hint.KbdStyle(cui.KbdStyleOptions{MaxWidth: &maxWidth, Wrap: true})` or `hint.ClearKbdStyle()` |
| Rust | `app.supports_kbd_style()` | `app.kbd_style(hint, &KbdStyle { max_width: Some(128.), wrap: true, ..Default::default() })?` or `app.reset_kbd_style(hint)?` |

In retained C, pass a copied, versioned `cui_bridge_kbd_style_v1` record to
`cui_bridge_set_kbd_style_v1`. For example, use a column with bounded width
to let a hint grow up to 176 logical pixels and wrap as the column narrows.

```c
#include <cui/cui_bridge.h>

cui_bridge_status style_shortcut(cui_bridge_app *app, cui_bridge_node hint) {
    const cui_bridge_kbd_style_v1 style = {
        .struct_size = sizeof style, .schema_version = 1,
        .fields = CUI_BRIDGE_KBD_STYLE_FILL | CUI_BRIDGE_KBD_STYLE_MAX_WIDTH |
                  CUI_BRIDGE_KBD_STYLE_WRAP | CUI_BRIDGE_KBD_STYLE_PADDING |
                  CUI_BRIDGE_KBD_STYLE_RADIUS | CUI_BRIDGE_KBD_STYLE_TEXT_GRADIENT,
        .max_width = 176, .padding_x = 8, .padding_y = 4, .radius = 6,
        .x1 = 1, .stop_count = 2, .stops = {{0, 0x0e7490ff}, {1, 0x7e22ceff}},
    };
    return cui_bridge_set_kbd_style_v1(app, hint, &style);
}
```

A style replaces the previous record. Omitted fields inherit the pill's
theme or mode defaults. Set `fill` to use the space offered by the parent,
`max_width` (`maxWidth` in JS/TS) to cap it, and `wrap` for a height that
fits the resulting lines. Use a bounded container to give wrapping a width.
Padding sets horizontal and vertical edges as a group. Python and JS/TS
supply 4 and 2 logical pixels for a missing padding axis when the other
axis is specified. Supply both border width and border color to override
the border. Colors are numeric `0xRRGGBBAA`, including transparent colors.

Text can use a solid color or a block-anchored linear gradient. Provide 2–8
ordered stops and distinct endpoints in normalized coordinates. Python
and JS/TS default the endpoints to left-to-right. Gradient rendering needs a
compatible renderer. Font size, line height, weight, italic and monospace use
the host font provider. Explicit font family and letter spacing are available
through immediate C. A plain typed hint ignores the saved style. Resetting the
stroke restores the literal without clearing the style. Styling stays passive
and produces no events.

The previous style stays active when an update fails. A provider that lacks
style support can mount the same tree after
clearing a detached style, with normal default pills.

## Verification

The C and WASI fixtures exercise all 1,580 changed Unicode 17 uppercase
mappings, scalar-safe truncation, overlap, untouched suffixes, the legacy
helper, geometry and accessible labels. The style fixture covers first-frame
width, responsive wrapping, copied gradient stops and the unstyled plain
path. The headless gallery inspects the semantic and draw labels, pill width,
monospace style and gradient stops.
Retained tests cover copied inputs, width and line count during resize,
gradient draw commands, accessible names, passive pointer behavior,
malformed updates, reset and removal, and fallback on alpha.14.

```sh
cmake -S tests/fixtures/kbd -B /path/to/private-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /path/to/private-build --target test_kbd kbd_guide test_content
python scripts/test_local.py --build-dir /path/to/private-build -- -R '^(kbd|content)$'
```
