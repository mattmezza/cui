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
embed 123 SVG assets; choose bundled names (such as `check`, `arrow-right`,
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

Constrained labels now use the current frame's laid-out label bounds. A positive
Button width, a grow width, or a positive `label_width` produces one clipped,
no-wrap label with a grapheme-boundary prefix and an ellipsis when it fits.
The original full label remains the semantic name. Leading/trailing artwork,
text, custom slots and the caret retain their own layout allocation; custom
content callbacks remain responsible for their internal layout. The content
clip also contains their paint. FIT buttons retain intrinsic sizing.

For fixed/grow Buttons, `content` children receive the current frame's inner width,
including space reserved for padding and leading/trailing slots. Ordinary wrapped
text and `cui_text_paragraph_colored(..., wrap=true)` wrap there just as they do
inside a normal box. A custom mail row can use a grow-width row containing a
fixed-width avatar and a grow-width column for the sender, subject and preview.
The callback runs once during submission; resizing needs no previous-frame bounds
or callback replay. Explicit fixed-width children and unwrapped text still obey
those choices, and the Button clips overflowing paint. Nested scroll/clipping
boxes retain their ordinary Core layout behavior.

`test_button_content_layout.c` compares wrapped line counts with normal boxes for
plain and paragraph text, fixed/grow widths, first-frame layout and resizing,
avatar/text columns and leading/trailing slots. It also checks text bounds, avatar
allocation and one callback invocation per submission. `test_button_content_font.c`
repeats these cases with a caller-opened native font.

`allow_content_overflow` is an explicit compatibility opt-out for the former
wrapping/overflow behavior.

Automatic label paint copies the full original bytes into the context's bounded
frame arena, using `cui_set_prepaint_parts`; it introduces no heap allocation or
new label-length limit. Borrowed slot resources retain their existing lifetime.
The optional caller-owned `cui_button_state.label` remains a 256-byte diagnostic
and compatibility buffer, so its truncated prefix keeps the previous bound.
Without that state, prefixes are limited by available width and frame capacity.
Prefixes preserve graphemes, while ellipsis fitting uses separately measured
prefix and ellipsis widths; this does not claim contextual shaping across the
join. Font shaping and glyph fallback remain adapter responsibilities.

Arena, command and registration capacity errors remain observable through Core's
error API and empty failed frame. Accepted callbacks occur during submission,
before later paint allocation failures; paint failure does not roll back an
already accepted action. Layout-only ancestry emits no label paint. Inert ancestry
paints and clips the same content without changing the optional state buffer.

`test_button_label_layout.c` covers fixed/grow current-frame resizing, all 18
legacy/design sizes in both densities, full semantics, leading/trailing slots,
caret, caller byte mutation, grapheme prefixes, malformed shapes, bounded arena
failure/recovery, inert/layout-only behavior and the opt-out. The independent
`test_button_label_font.c` repeats the matrix with a real caller-opened native
FreeType/HarfBuzz font; the portable ZWJ case is separate from the native font's
supported combining-mark case. `test_button_label_layout.py` compiles actual WASM,
checks Canvas pixels, current-frame widths and ellipsis commands, and dispatches
real Chromium mouse and CDP touch contacts. These tests close this label-layout
slice; they do not certify the entire Button family or physical device behavior.

Standalone Button composition under inherited inert or layout-only ancestry now
uses a passive path selected by Core's ordinary-submission phase queries. Inert
scopes retain content layout, artwork, clipping and presentation; layout-only
scopes retain layout without paint. Neither path registers Button metadata,
reduces tooltip state, invokes action/hover notifications, schedules a tooltip wake, or
changes any bytes of the caller-owned Button state. Content callbacks still run
to compose the visual children; they remain application code.

The original state pointer remains borrowed through deferred label paint, so its
existing 256-byte diagnostic-prefix cap stays consistent without a temporary
stack-state lifetime. The synchronous legacy overflow/explicit-ellipsis branch
uses a temporary paint copy whose visible bytes Core copies immediately; no
temporary state pointer reaches deferred paint. Passive ID reuse preserves state; a later ordinary frame
performs normal ID reconciliation. Explicit disabled/loading options in an
ordinary frame retain their existing blockers, hover/tooltip cleanup and loading
presentation. Calls outside ordinary submission, including font measurement and
prepaint callbacks, perform no standalone Button work. This is a CUI passive
composition correctness rule, not an upstream GPUI inert-scope parity claim.

The focused passive lifecycle fixture compares complete preloaded caller-state
bytes across fixed/grow labels, same/changed IDs, direct/inherited ancestry,
tooltip open/pending/suppressed states and zero/nonzero frame deltas. It retains
strict no-draw assertions for layout-only composition and checks ordinary touch
reuse, cancellation, disabled/loading behavior and guarded phases. The native
font fixture and independent Chromium WASM fixture extend that evidence to real
font measurement and actual hover-opened tooltips, Canvas paint and mouse/CDP
touch/keyboard reuse; they do not claim physical touchscreen coverage.

Visible inert loading artwork keeps its animation redraw requests. Layout-only
loading composes the same-sized icon slot without calling the artwork painter or
requesting invisible animation redraw. The existing Core paint-enabled query
distinguishes these policies; caller state/deadlines remain unchanged in both.
The fixture separately checks Core redraw state after each begin-frame reset and
verifies normal loading resumes, rather than inferring wake behavior from pixels.

### Numeric sizing, open presentation, and caret

`pixel_size` is an optional finite logical-pixel size in `(0, 4096]`; zero retains
legacy/design sizing. Negative, nonfinite, or larger values reject the Button
before layout, semantic registration, or callbacks. Numeric sizing follows the
pinned source's numeric `Size` branch: horizontal padding is 20% of the size,
leading/trailing artwork is 75%, normal text uses the theme's base font, and the
content gap uses the medium gap. A normal button has intrinsic content height;
an icon-only button defaults to a square of `pixel_size` except Link/Text. Compact mode does not
change numeric padding. Link and Text still have no horizontal padding.

Numeric sizing takes precedence over legacy/design metrics. Explicit `height`,
`font_size`, and `icon_size` then override their respective metrics; `width`
retains its existing FIT/grow/fixed behavior. Numeric dimensions are literal
logical pixels, independent of density, while base text, medium gap, and medium
caret follow theme density. Raster artwork reserves its rounded-up pixel extent;
the existing Icon resource capacity and validity rules still apply. This bounded
API does not promise successful artwork allocation for every large valid size.
Joined ButtonGroup children retain their own numeric sizing and `open` options
when group legacy/design/style inheritance is applied.

`open` selects active presentation without changing checked/value semantics,
selection, callbacks, focus, or managing a menu. It is independent of `selected`
and `toggled`. The dropdown caret uses normal foreground at 75% alpha even when
hover, press, open, disabled, or selected changes the label palette. Outline and
custom normal foregrounds are respected; loading and ancestor opacity still
compose over the whole control. Numeric sizing uses the medium caret size,
independent of numeric or explicitly overridden leading artwork size.

This slice implements those three documented contracts, not complete Button
family or platform parity. Its focused tests cover numeric precedence and geometry
across all 18 legacy/design sizes and two densities, normal/compact/icon-only
layout, explicit dimensions, custom/outline caret tint, disabled/loading paint,
and invalid numeric domains. A real Noto font fixture and Chromium WASM fixture
exercise the same implementation; Chromium input evidence is not physical device
validation. Existing constrained-label and passive lifecycle fixtures remain
separate regression gates.

The numeric formulas come from pinned `button/button.rs` lines 615–619 and
662–670, `sizing.rs` lines 337–343, and `select.rs` lines 59–67 at upstream
commit `87d10ae5e1299d1be18670a37570c5402f952e34`. Numeric Link/Text normal and icon-only buttons use intrinsic height and width,
matching the source no-padding branch; explicit caller dimensions still win.
The zero numeric default preserves CUI's established legacy icon-only sizing.

### Event-aware click callbacks

`on_click_event` receives one `cui_button_click_event` for every accepted
activation, in frame input order. The legacy `on_click` runs first for each event
when both are present, using the same `action_user`. Either callback can be used
alone. `cui_button_ex` continues to return one boolean indicating that at least
one activation was accepted; the callbacks preserve each individual event.

The payload contains the stable target ID, successful frame sequence, event
index and event type. Pointer activations include release coordinates in Core
logical pixels, pointer type/button/contact/device, release modifiers, and click
count (an unspecified zero normalizes to one). Keyboard activations carry the
accepted key and modifier bits, without a fabricated pointer position.
Accessibility activations have a distinct origin. Modifier bits for keyboard or
touch are a CUI extension: the pinned source ClickEvent helper reports no such
modifiers. This is an accepted-event snapshot, not a full source MouseClickEvent
with down and up samples, keyboard hitbox, first-mouse or touch long-press data.

The callback pointer is borrowed only until that callback returns. The structure
contains no borrowed pointers, so copying the value retains it safely across
frames or context destruction. `event_index` identifies that dispatched frame,
not a future queue entry; `frame_sequence` is scoped to context lifetime and its
existing wrap boundary. The callback must not end/reenter a frame or overwrite
an active Group workspace. Callback registration and descriptor fields are
snapshots; controlled model changes retain their existing semantics.

Disabled, loading, cancelled and passive inert/layout-only submissions produce
no callback. Repeated key events rejected by Core produce no callback. In a
styled ButtonGroup, child legacy and event callbacks both dispatch when the
group has no `on_change`. When `on_change` is present it overrides both child
callbacks, including the existing pointer-only/default or ALL_ACTIVATIONS policy.
The Base optional child event-index hook runs at that same dispatch point, after
legacy, and `result.callbacks` counts each actual callback invocation.

Cooperative `cui_consume_event` during the legacy notification cannot erase the
new notification for the already accepted event. The event-aware callback returns
`void`; it does not promise GPUI capture/bubble propagation or restore a consumed
event. The separate capture/propagation proposal records the remaining source
contracts in `validation/button-click-capture-next33.md`.

```c
struct button_model {
    unsigned clicks;
    cui_button_click_event last_click;
};
static void button_clicked(void *user, const cui_button_click_event *event) {
    struct button_model *model = user;
    model->last_click = *event;
    model->clicks++;
}
bool draw_event_button(cui_context *ui, const cui_theme *theme,
                       struct button_model *model) {
    return cui_button_ex(ui, 100, theme, cui_string("Open"),
        (cui_button_options){.on_click_event = button_clicked, .action_user = model});
}
```

The focused unit fixture verifies same-frame multiplicity and order, release
metadata, repeat/cancel/current-disabled rejection, cross-frame touch identity,
copy lifetime, accessibility, Group override and legacy-then-event order.
The real Noto native fixture uses that same implementation. Chromium WASM uses
actual mouse/keyboard/CDP touch input, modifiers, cancellation, passive rejection
and joined Group behavior; it is not evidence of physical OS/device input.
