# Disclosure controls

For retained node trees, see the [C bridge and language builders](BRIDGE_COMPONENTS.md#style-retained-accordion-and-collapsible-from-c).

`cui_disclosure.h` provides unstyled trigger behavior and styled Accordion and
Collapsible containers. The caller owns expansion and motion state. Trigger IDs
must remain unique and stable across frames.

```c
cui_disclosure_item sections[] = {
    {11, cui_string("Account"), false},
    {12, cui_string("Advanced"), false},
};
bool open[] = {true, false}; /* keep in application state */
cui_accordion(ui, 10, &theme, sections, 2, open,
    (cui_accordion_options){.collapsible = true, .bordered = true},
    draw_section, application);
```

Without motion, `draw_section(cui_context *, size_t index, void *)` runs
synchronously for expanded panels. It can submit nested accordions or other CUI
children.
Children of collapsed panels are omitted from rendering and focus order.
Collapsible uses the same callback shape with index zero.

Accordion supports single or multiple expanded sections, optional collapse of the
last expanded section, group and individual disabled states, and optional borders.
Click, Enter and released Space activate a trigger. Up/Down wrap among enabled
triggers; Home/End jump to the first/last enabled trigger. Arrow traversal moves
focus without opening panels. The return value reports controlled state changes.
Applications can also change the booleans themselves between frames.

The base functions submit trigger semantics without painting. Custom designs can
call them and then submit boxes with matching trigger IDs. The styled functions
use theme colors, padding, gap, radius and control height; themes provide density
and high contrast without duplicating behavior. Trigger semantics expose
expanded state and the controlled panel ID. Mounted panels expose their trigger
relationship. An Accordion group supplies a semantic parent. The WASM gallery
projects these relationships to browser accessibility nodes.

The implementation derives panel IDs by XORing the trigger ID with
`0x1d8f322539deca61`; Collapsible derives its outer container using
`0xb7c1c9d453922783`. Reserve those IDs when composing children. Duplicate IDs
are diagnosed by the core rather than silently reused.

`tests/unit/test_disclosure.c` exercises four supplied themes, actual pointer
clicks, Enter and Space, disabled triggers, single/multiple expansion, required
expansion, keyboard traversal, and removal of focused hidden content. Portable
C11 and renderer-independent draw lists are supported. Physical native
assistive-technology behavior still requires verification.

## Size and indicators

Use `cui_accordion_present` or `cui_collapsible_present` from
`cui_disclosure_events.h` to size the default header and choose an SVG indicator.
Keep expansion state in your application and call the presenter each frame.

```c
#include <cui/cui_disclosure_events.h>

static void draw_section(cui_context *ui, size_t index, void *user) {
    const cui_theme *theme = user;
    cui_text(ui, cui_string(index ? "Manage access." : "Update your details."),
             theme->font_size, theme->text);
}

void draw_preferences(cui_context *ui, cui_theme *theme, bool open[2]) {
    const cui_disclosure_item items[] = {
        {cui_id_from(ui, "profile"), cui_string("Profile"), false},
        {cui_id_from(ui, "permissions"), cui_string("Permissions"), false},
    };
    cui_accordion_present(ui, cui_id_from(ui, "preferences"), theme, items, 2, open,
        (cui_accordion_options){.collapsible = true, .bordered = true},
        (cui_disclosure_presentation){.width = -1, .design_size = CUI_DESIGN_LG,
            .collapsed_icon = cui_string("plus"), .expanded_icon = cui_string("minus")},
        draw_section, theme, (cui_disclosure_handler){0});
}
```

The shared `xs`–`9xl` tokens set the default title font, line and trigger height,
indicator size, padding and gap. `CUI_DESIGN_INHERIT` takes its metrics from the
theme. Width `-1` fills the parent, a positive width sets a fixed outer extent,
and zero fits the content. At a bounded width, a single-line title clips within
the available lane while its accessible name retains the entire label. Panel
children choose their own typography. Try **Custom icons and panels** and
**Nested panels** in the gallery to see alternate indicators and separate,
caller-owned expansion arrays.

Each icon name resolves through the same catalog and theme SVG loader as
`cui_icon_named`. An empty name uses the default chevron for that state. Icons
are decorative, so the trigger remains a single focus and activation target.
Use a custom header callback to compose leading artwork, a badge, or an icon
from a caller-owned SVG state. An optional `cui_disclosure_handler` observes the
same [applied transitions](DISCLOSURE_EVENTS.md) as the older event entry points.
Retained C applications can copy sizes, icons, group options and trigger
styles into the bridge while keeping its queued events. Typed language
builders currently expose their existing disclosure controls.
[Retained C guide and example](BRIDGE_COMPONENTS.md#style-retained-accordion-and-collapsible-from-c)

### Joined groups and trigger styles

Set `.joined = true` on the Accordion presenter to put all triggers and panels
in one card. Dividers follow each section, including its expanded content,
and the last section has no divider. `options.bordered` draws the rounded
outline. The gallery's **Borderless** mode keeps the dividers and removes
that outline. For a custom outer background, padding, radius or clipping,
supply `container_box`. A nonzero presentation width overrides the box width.
The default rounded card insets its rectangular child clips inside the corners.

`trigger_styles` contains optional refinements indexed by item, bounded by
`trigger_style_count`. Collapsible uses the first entry. If more than one custom
background applies, disabled takes precedence, followed by pressed, hovered
and expanded. The trigger box supplies the fallback color. Hover and press
respond to the current layout. Focus independently uses the custom outline
color, and a custom cursor applies to an enabled trigger. Set each
`custom_*` flag to apply its value, including transparent colors.

```c
/* sections and expanded come from application state. */
cui_disclosure_trigger_style styles[2] = {
    {.custom_hover = true, .hover = theme.list_hover,
     .custom_pressed = true, .pressed = theme.list_active,
     .custom_cursor = true, .cursor = CUI_CURSOR_POINTER},
    {0},
};
cui_accordion_present(ui, 10, &theme, sections, 2, expanded,
    (cui_accordion_options){.bordered = true, .collapsible = true},
    (cui_disclosure_presentation){.width = -1, .joined = true,
        .trigger_styles = styles, .trigger_style_count = 2},
    draw_section, application, (cui_disclosure_handler){0});
```

Presentation boxes and trigger styles are borrowed for the submission call.
Motion state remains valid through `cui_end_frame`.

### Custom headers and panel boxes

Set Accordion `render_header` and `header_user` to compose a custom title, icon,
badge or other header body. `cui_collapsible_ex` exposes the same callback and
optional `borderless` appearance; `cui_collapsible` retains its existing defaults.
The callback replaces the default title/indicator body, receives the current
expanded state and effective disabled item, and executes inside the themed trigger
box. The original item's label remains the accessible name. Trigger activation,
keyboard navigation and panel mounting continue to use base behavior. Callback
item pointers are borrowed only for that invocation. Headless tests verify custom
headers, semantic names, individual/group disabled propagation and actual pointer
and keyboard activation of both components.

Accordion `trigger_boxes` and `panel_boxes` are optional borrowed per-item arrays
of full `cui_box` styles; `style_count` bounds both nonnull arrays, and later
items keep the theme defaults. `cui_collapsible_ex` accepts individual
`trigger_box` and `panel_box` pointers. These override layout/paint properties
without replacing base activation or accessible names. Header focus still uses
the theme focus color and border width; explicitly masked border edges remain
a caller style decision. Tests assert distinct overridden header/panel bounds
and colors, fallback styles beyond the array and pointer toggling in both APIs.

### Measured reveal

Opt into measured reveal with a zero-initialized `cui_disclosure_motion` kept in
application state. Collapsible accepts its pointer; Accordion accepts an array
and `motion_count`, with later panels retaining immediate mount/unmount. Set
`dt` to elapsed seconds; optional positive `response` overrides the theme spring
natural period in seconds. A reversible theme-configured spring drives a clipped viewport from zero to the
full measured panel height. Initial state snaps; `reduced_motion` also snaps.
Invalid/nonpositive dt leaves progress unchanged. Measurement updates at end_frame,
so changed content sizes affect the viewport on the following frame. The motion
state and array must remain valid through end_frame and be dedicated to stable
trigger IDs; reusing a state for another ID resets it.

With motion enabled, panel callbacks also execute while collapsed to measure their
full natural layout. They must submit UI rather than perform application side
effects. Settled collapsed bodies use layout-only boxes: no paint, semantics,
events or focus. Closing bodies keep painting inside the shrinking clip but
become inert immediately when the controlled boolean becomes false. Pending
animation and changed measurement invalidate the frame. Custom panel styles
remain inside the reveal viewport. Header and controlled expanded semantics change
immediately; the return value still reports only controlled state changes.

Headless tests exercise actual pointer/keyboard expansion, measured initial hidden
content, intermediate height, reversal, closing paint with immediate focus/event
withdrawal, dynamic content size, reduced motion and bounded Accordion storage.
Styled panels use `theme.spring_control` response/damping/epsilon/travel. Natural
frequency is TAU/response, as in the reference. A zero response or false travel
snaps even at zero dt. Damping below one may overshoot; the retained spring value
keeps that trajectory while reveal height clamps to zero/full height. Settings
with invalid response/damping/epsilon use safe defaults. Tests compare differently
configured response/damping, preserve underdamped overshoot, and verify theme
travel/zero-response snap through actual trigger clicks. The public `_step_ex`
accepts the same portable policy; the original `_step` keeps critical damping and
its legacy .2-second default response.
