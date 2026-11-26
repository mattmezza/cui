# Keyboard shortcut hints

`cui/cui_kbd.h` adds `cui_kbd_ex` for a typed **single chord**, preserving the
existing literal `cui_kbd` and byte-preserving `cui_kbd_format` APIs unchanged.
It displays hints only: no shortcut registration, action lookup, sequence
recognition, activation or global event dispatch occurs.

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
background. `plain=true` wins over outline and renders text at the theme font
and color without pill padding/border/minimum. CUI maps rem to
`theme.font_size/16`; the font must be finite(0,512], radius finite/nonnegative.

Hints are noninteractive LABEL semantics, excluded from tab traversal. The
accessible name defaults to formatted text; an explicit name overrides it.
Names must be strict single-line UTF-8 up to1024 bytes, without embedded NUL
or ASCII controls. Invalid arguments fail before layout or semantics. Input
strings and local formatted output are copied into Core frame storage at
submission and survive through that frame's renderer consumption. No retained
component state, allocator or cleanup is required.

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

Plain mode keeps its theme typography and ignores all keycap overrides.
Style values are read during the call. Gradient stops are copied for that
frame, so local arrays are safe. Invalid typography and gradient definitions
fail before the hint is submitted. To add a shadow, use
`cui_element_set_shadow_preset` on the submitted keycap ID. Backgrounds use
Core's solid box fill. The retained builders accept a preformatted label.
See [shared typography](TYPOGRAPHY.md) for renderer and provider requirements.

Ordinary row composition fits its contents; it does not automatically turn a
shortcut list into a responsive wrapping grid. For narrow help panels, let the
owner split rows using the host's available-width/layout policy. Joining multiple
strokes into one sequence or looking up host Action/context/focus/global
bindings remains outside this API. Do not promote full Kbd parity from these
formatting and presentation additions.

The C and WASI fixtures exercise all 1,580 changed Unicode 17 uppercase
mappings, scalar-safe truncation, overlap, untouched suffixes, the legacy
helper, geometry and accessible labels. The style fixture covers first-frame
width, responsive wrapping, copied gradient stops and the unstyled plain
path. The headless gallery inspects the semantic and draw labels, pill width,
monospace style and gradient stops.

```sh
cmake -S tests/fixtures/kbd -B /path/to/private-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /path/to/private-build --target test_kbd kbd_guide test_content
python scripts/test_local.py --build-dir /path/to/private-build -- -R '^(kbd_source|content)$'
```
