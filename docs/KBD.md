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
                         .key = CUI_STR("p")},
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
ASCII first letters become uppercase; known lower-case key names map to source
labels, including Esc, Enter/⏎, Backspace/⌫, Delete/⌫, arrow names/symbols and
Page Up/Page Down. Function keys and literal `+`/`-` work without a parser.

`cui_kbd_format_stroke` returns required bytes **excluding NUL**. It accepts
NULL output for size queries and capacity0 without writing; positive capacity
always NUL-terminates valid output within that capacity, truncating only complete
UTF-8 scalars. Invalid input returns SIZE_MAX and leaves the output unchanged.
Input key must be nonempty strict single-line UTF-8, at most128 bytes, with no
embedded NUL or ASCII control characters. Unknown modifier bits are invalid.
The four existing portable flags are the only accepted mask. The longest result
is147 bytes, so internal required-count arithmetic cannot overflow. The output
buffer may overlap the input key. Non-ASCII casing is preserved: full Unicode
uppercase conversion from the Rust source is not implemented.

Zero options draw the source-shaped muted pill: muted foreground/background,
12-unit text at16-unit rem, horizontal padding4, vertical padding2, minimum
width20 and half theme radius. Outline adds a1-unit border and uses the theme
background. `plain=true` wins over outline and renders text at the theme font
and color without pill padding/border/minimum. CUI maps rem to
`theme.font_size/16`; the font must be finite(0,512], radius finite/nonnegative.
It does not provide arbitrary GPUI Styled modifiers or a general inherited
parent text-style stack. CUI solid theme tokens are the supported palette;
this is not proof of every upstream theme/background representation.

Hints are noninteractive LABEL semantics, excluded from tab traversal. The
accessible name defaults to formatted text; an explicit name overrides it.
Names must be strict single-line UTF-8 up to1024 bytes, without embedded NUL
or ASCII controls. Invalid arguments fail before layout or semantics. Input
strings and local formatted output are copied into Core frame storage at
submission and survive through that frame's renderer consumption. No retained
component state, allocator or cleanup is required.

Ordinary row composition fits its contents; it does not automatically turn a
shortcut list into a responsive wrapping grid. For narrow help panels, let the
owner split rows using the host's available-width/layout policy. Joining multiple
strokes into one sequence or looking up host Action/context/focus/global
bindings remains outside this API. Do not promote full Kbd parity from these
formatting and presentation additions.

The standalone worker fixture compiles the exact example and tests named-key
formatting, all output capacities, UTF-8/control rejection and overlap, normal/
outline/plain geometry, theme scaling, actual software fill pixels, text draw
colors, semantics and ordinary sibling composition. It checks legacy helpers
remain unchanged. Text rasterization is not claimed by portable pixel tests.
New sources require root-owned canonical source/test registration and matching
SDK/consumer-resource regeneration before distribution.

Worker validation used strict C11 Release (`-Wall -Wextra -Wpedantic -Werror`):
`kbd_source` and the existing `content` regression passed through the private
headless wrapper. The exact guide example compiled against the real header,
and strict C++11 header syntax passed. No native windows or browser were opened.

```sh
cmake -S tests/fixtures/kbd -B /path/to/private-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS='-Wall -Wextra -Wpedantic -Werror'
cmake --build /path/to/private-build --target test_kbd kbd_guide test_content
python scripts/test_local.py --build-dir /path/to/private-build -- -R '^(kbd_source|content)$'
```
