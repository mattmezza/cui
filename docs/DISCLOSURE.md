# Disclosure controls

`cui_disclosure.h` provides unstyled trigger behavior and styled Accordion and
Collapsible containers. The caller owns expanded booleans. No control allocates
persistent state. Trigger IDs must remain unique and stable across frames.

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

`draw_section(cui_context *, size_t index, void *)` runs synchronously only for
expanded panels. It can submit any CUI children, including nested accordions.
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
relationship; an Accordion group supplies a semantic parent. Native accessibility bridges are separate
unfinished infrastructure.

The implementation derives panel IDs by XORing the trigger ID with
`0x1d8f322539deca61`; Collapsible derives its outer container using
`0xb7c1c9d453922783`. Reserve those IDs when composing children. Duplicate IDs
are diagnosed by the core rather than silently reused.

`tests/unit/test_disclosure.c` exercises all four supplied themes, actual pointer
clicks, Enter and Space, disabled triggers, single/multiple expansion, required
expansion, keyboard traversal, and removal of focused hidden content. Portable
C11 and renderer-independent draw lists are supported; OS accessibility is not
claimed. Compared with GPUI Kit 0.7.1, a demonstrated native/browser gallery remain unfinished.
Immediate mount/unmount behavior is implemented. Full family parity is not yet
verified.

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
its legacy .2-second default response. A demonstrated gallery remains unfinished.
