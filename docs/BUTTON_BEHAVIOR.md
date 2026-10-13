# Button behavior

This change audits the Button at pinned GPUI component v0.7.1 commit
`87d10ae5e1299d1be18670a37570c5402f952e34` (`button/button.rs`,
`button/button_icon.rs`, `base/button.rs`). It covers the features below;
custom shadows, ordered tab indexes, action-keybinding tooltip registries,
and arbitrary tooltip builders remain outside this API. It does not certify
whole-family parity or physical-device accessibility.

`cui_button_ex` retains the original primary variant, four legacy sizes,
zero-initialized options, boolean activation result, and replacement `content`
callback. Design sizes continue to derive all fourteen metrics from the theme.
Default, Text and Custom append to the existing variants. Outline remains
orthogonal. Link and Text use a hand cursor and no horizontal padding; Link
keeps its legacy control height, while Text fits its natural content. Default
uses the theme's default button palette. Custom uses an explicitly resolved
palette; it does not reproduce the source's Oklab color mixing or shadow.

Positive finite `height`, `font_size`, and `icon_size` override derived metrics
(up to 4096 logical pixels). Rounding selects theme, none, half-theme,
double-theme, or a caller's nonnegative finite radius. Compact reduces padding
and keeps known text/icon content at least as wide as the control height.
Arbitrary callback content must supply a suitable `width` when it needs a
minimum width. Positive width is an intentional clipping constraint; negative
width retains Core's grow convention. Callers must allow enough height for
arbitrary content.

Named `icon`, `trailing_icon`, `loading_icon` and `dropdown_caret` use the real
SVG icon renderer. The catalog always lists all names, but default builds
embed 106 SVG assets; choose bundled names (such as `check`, `arrow-right`,
`chevron-down`, `loader`) or supply a theme loader / `CUI_ICON_ALL_ASSETS`
for other catalog assets. Loading suppresses activation, exposes busy metadata, and
fades the normal/selected palette to 80% rather than using disabled colors.
A named loading glyph rotates using frame time; reduced motion keeps it still.
A `leading_content` callback takes precedence over generated loading/leading
artwork, allowing a caller-owned Progress or Spinner composition.
`trailing_content` replaces trailing artwork. `children` appends content after
the label or existing replacement callback. Slot callbacks own their borrowed
user data, geometry, colors, and nested semantics.

`on_click` runs once per accepted activation. The boolean return still reports
that same activation: use one action path, rather than handling both as separate
actions. `on_hover` reports interactive hover edges when a caller-owned state
is supplied. Disabled/loading controls ignore activation and hover callbacks.
`selected` remains presentation-only, with stable semantic value zero;
`toggle_semantics` plus `toggled` explicitly supplies checked/value metadata.
Browser or accessibility adapters must map that metadata appropriately; this
change does not add a generic browser `aria-pressed` mapping.

Pointer focus defaults remain compatible. `prevent_pointer_focus` opts into
pointer activation/capture without replacing current keyboard focus, using
Core's `no_pointer_focus` policy. Tab, Enter, Space, programmatic focus and
cancellation continue through Core. `skip_tab` removes the tab stop;
`hide_focus_ring` affects paint only. Focus rings otherwise show only for
focus-visible interaction. `custom_cursor` uses the cursor-only setter and
preserves the control's other semantics.

Optional state stores only hover lifecycle, tooltip state, an ellipsis buffer,
and an absolute next-wake deadline. Zero-initialize it, keep one state per
stable ID, and keep it alive across frames; an ID change resets it. No state
heap allocation or cleanup is required. Existing icon resources remain owned
by the context. Tooltips use the shared hover implementation and require a
viewport in `tooltip_options`; Escape, disabled state and targeted cancellation
close/suppress them. Hosts should honor invalidation and
`state.next_wake_seconds` in the Core frame-time clock (zero means idle).

A positive `label_width` makes the plain label unwrapped and clipped. With
state, it also ellipsizes at a grapheme boundary while preserving the complete
original accessible name (or explicit `accessibility_label`). The scratch
buffer is 256 bytes: a truncated prefix is capped at 252 bytes plus U+2026 and
NUL. A fully fitting label remains borrowed and has no 256-byte cap. If the
ellipsis itself cannot fit, the visible label is empty. This is plain-label
truncation, not a rich-text filter, shaping replacement, or callback-content
truncation.

```c
#include "cui/cui_components.h"
static cui_button_state save_state;
static void save(void *user) { ++*(unsigned *)user; }
void save_control(cui_context *ui, const cui_theme *theme, unsigned *saves) {
    (void)cui_button_ex(ui, cui_id_from(ui, "save"), theme, cui_string("Save changes"),
        (cui_button_options){.icon = cui_string("check"), .on_click = save,
            .action_user = saves, .state = &save_state,
            .prevent_pointer_focus = true, .custom_cursor = true,
            .cursor = CUI_CURSOR_POINTER});
}
```

Validation is scoped to `test_button_behavior.c` and
`test_button_behavior.py`: controlled activation/cancellation and pointer focus,
keyboard activation, inert loading/disabled state, stable toggle metadata,
432 variant/size/density paint-containment cases, real SVG slot geometry,
custom content, Unicode/buffer-boundary ellipsis and tooltip lifecycle. The browser slot test also requires the host preview height to include the
expanded variant and custom-content rows, and fails if those drawings are
clipped by the canvas. The
browser uses Chromium CDP touch contacts and actual mouse/keyboard input,
not direct reducer calls. Native headless tests do not constitute SDL window
or physical touchscreen coverage.
