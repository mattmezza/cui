# Frozen Kbd source audit and bounded slice

Design checkpoint, 2026-10-20; CUI baseline `9d491483`. Frozen GPUI Kit 0.7.1
checkout `/tmp/cui-gpui-reference` was verified at
`87d10ae5e1299d1be18670a37570c5402f952e34`.

[Component source](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/component/src/kbd.rs)
stores exactly one `Keystroke` (lines 12–17). Constructors accept one stroke.
`from_binding` explicitly selects `binding.keystrokes().first()` (105–108).
There is no grouped/multi-step shortcut rendering method in this component.
The [story](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/crates/story/src/stories/kbd_story.rs)
composes independent hints in an ordinary horizontal flex container with gap
and wrap. The [frozen guide](https://github.com/longbridge/gpui-kit/blob/87d10ae5e1299d1be18670a37570c5402f952e34/website/component/kbd.md)
also shows ordinary parent composition, one chord per Kbd.

The existing manifest already records that “grouped shortcut rendering” has no
mapped frozen multi-shortcut API. Modifier formatting tests cannot establish
such a contract. This checkpoint does not promote, rename or delete that row;
root owns its reconciliation. An invented grouping API would not close a
source-backed gap.

Actual remaining source features:

| Source contract | CUI at this checkpoint | Proposed slice |
| --- | --- | --- |
| Platform modifier order and special key display (`format`, 111–225) | Existing `cui_kbd_format` orders modifiers and safely truncates UTF-8, but leaves key spelling/case untouched | Add typed one-stroke formatter for common ASCII/special names; preserve old helper |
| Appearance enabled by default | Legacy `cui_kbd` always draws an outline | Add `cui_kbd_ex` with normal muted pill default |
| `appearance(false)` returns plain formatted text (234–236) | No option | Opt-in `plain`, overriding outline |
| `outline()` adds border and changes fill (245–249) | Only outlined legacy presentation | Explicit outline option alongside normal/plain |
| Padding X4/Y2, min width20, half theme radius, text12/lineheight1, no shrink (250–259) | Uniform padding4, text0.8*theme font, no minimum | Source-shaped geometry using theme font-size/16 rem scaling |
| Host Action/context/focus/global binding lookup (58–108) | CUI command bindings belong to host-owned command model, not GPUI Window/Action registry | Remains host responsibility; no simulated lookup or dispatcher |
| Unicode first-character uppercase and arbitrary Styled | Existing helper preserves key text | Non-ASCII casing and arbitrary styles remain explicit limits |

Proposed additive header `cui/cui_kbd.h`:

```c
typedef struct { unsigned modifiers; cui_str key; } cui_kbd_stroke;
typedef struct {
    bool plain, outline, mac_symbols;
    cui_str accessible_name;
} cui_kbd_options;
size_t cui_kbd_format_stroke(cui_kbd_stroke, bool mac_symbols,
                             char *output, size_t capacity);
bool cui_kbd_ex(cui_context *, cui_id, const cui_theme *,
                cui_kbd_stroke, cui_kbd_options);
```

The typed key is a strict UTF-8 single line of at most128 bytes. Modifier bits
are the existing CTRL/ALT/SHIFT/META constants; parser aliases such as secondary
must be mapped by the host. Required-size formatter returns SIZE_MAX on invalid
input, supports null/zero-capacity queries, and truncates only complete UTF-8
scalars. Formatting preserves non-ASCII casing rather than pretending to ship
a Unicode case database. Rendering submits one noninteractive label with an
optional explicit accessible name; it never handles input. No new global
architecture, allocations or retained state are required.

New base/component `cui_kbd.c` files avoid edits to shared content/components
sources and preserve the legacy function and formatter byte behavior. Root
must register those sources and the new header/test in canonical metadata/build
lists. A standalone worker fixture can compile the new sources against the
unchanged portable library. Proposed tests cover source key-name matrix,
scalar-safe capacities, validation, normal/outline/plain draw geometry and
software pixels, semantics/no activation, theme scaling, ordinary row-group
composition, and source-owned lifetimes. A guide example must compile as C11.
No claim of complete Kbd parity or runtime validation is made by this audit.
