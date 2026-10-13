# Scrollbar appearance and idle hiding

Set `theme.scrollbar` once for all themed scrolling owners, or pass a borrowed
`scrollbar_style` override in the relevant options. An override replaces the
shared skin; it is read only during the widget call/frame and is never retained.
Surface begin copies its skin and palette until the paired footer/end calls.

```c
theme.scrollbar = (cui_scrollbar_style){
    .visibility = CUI_SCROLLBAR_AUTO_HIDE,
    .thickness = 8,
    .min_thumb = 24,
    .thumb_radius = 4,
    .no_track = true,
    .idle_seconds = 1,
    .fade_seconds = .15f,
};
```

`INHERIT` is the zero-initialized compatibility policy: existing bars remain
available, and owners that previously had no bars remain barless. `VISIBLE`
adds bars to overflowing formerly barless viewports. `HIDDEN` omits their paint,
pointer/keyboard bar semantics and drag handling. It preserves the content's
scroll owner and wheel, touch and existing keyboard behavior. A legacy explicit
`hide_scrollbars` option still wins over the skin.

Positive finite thickness/minimum values override the owner's defaults.
Thickness is capped at half the smaller viewport dimension. Thumb minimum is
capped at track length. Thumb and track radii are independent, with zero choosing
square corners. Existing Tabs retain their legacy rounded bar under a zero skin.
`custom_thumb_color` and `custom_track_color` allow RGBA overrides, including
transparent colors; otherwise the owner's palette supplies colors. `no_track`
removes the painted track without removing its visible thumb's hit region.

Existing gutter bars keep their gutter during auto-hide, avoiding reflow as
opacity changes. HIDDEN releases their gutter. Newly added bars are overlays and
never reserve a gutter. Overlay corners use the horizontal bar's hit region
where both axes meet. Bars are bounded by the owner's clip and stay fixed as
content translates.

## Owners and axes

| Owner | Actual axes | INHERIT availability | Style route |
| --- | --- | --- | --- |
| Scrollable | Caller-selected horizontal/vertical | Existing gutter bars | `cui_scroll_options`, theme |
| Sidebar body | Vertical | Existing gutter bar | Sidebar options, theme |
| DataTable / virtualized table | Horizontal/vertical, preserving pinned columns | Existing `scrollbars` option | Table options, theme |
| VirtualList | Horizontal/vertical according to its model | Existing gutter bars | Its nested scroll options, theme |
| MessageScroller / conversation | Vertical | Existing `scrollbar` option | Message scroller options, theme |
| Tabs and Dock tab strip | Active horizontal or vertical tab axis | Existing `scrollbar` option | Tabs options, theme; Dock forwards theme |
| Textarea and Editor | Vertical; horizontal when no-wrap | No bars | Textarea options, theme |
| Tree | Vertical | No bars | Tree options, theme |
| List, ListMulti, searchable/choice/combobox popup lists | Vertical when windowed | No bars | List/popup options, theme |
| MenuTree, nested Menu, ContextMenu, split/dropdown and overflow menus | Vertical per overflowing popup level | No bars | Menu options, theme |
| Dialog, AlertDialog, Sheet, Popover body | Vertical | No bars | Surface options, theme |
| Command palette | Vertical | Existing Scrollable bar | Theme through common Scrollable |
| Settings body | Vertical | Existing Scrollable bar | Theme through common Scrollable |
| TextView body | Vertical when explicitly scrollable | Existing Scrollable bar | TextView options, theme |
| TextView wide table | Horizontal | No bars | TextView options, theme |
| AttachmentGroup | Horizontal | No bars | Explicit `scrollbar_style` and optional `scrollbar_theme` |
| Finite Carousel | Its configured horizontal or vertical axis | No bars | Carousel options, theme |

AttachmentGroup's existing API has no theme argument. Its optional borrowed
`scrollbar_theme` supplies the application palette and shared policy; absent
that pointer it uses the light palette and the inherited no-bar policy. Explicit
skin colors work without a palette pointer.

Lists and menus preserve discrete row windows. Bar movement changes the window,
not the selected value or action. Their active keyboard row is kept in the new
window so the existing next-frame reveal does not undo bar movement. Menu rows
use their actual varying heights. Finite Carousel bar movement uses the model's
nearest slide and normal change callback; it does not create a second scroll
model.

Looping Carousel has no finite beginning/end, and intentionally has no bar even
with VISIBLE/AUTO_HIDE. A raw `cui_table_begin` composition, resizable/dock content
callbacks and plain boxes do not create scroll models; use Scrollable inside
them. Single-line Input caret reveal is not a general panel scroll owner. Plot
domain pan/zoom is a separate interaction model and is unchanged by this skin.
No unsupported horizontal axis is invented for Tree, Surface or row lists.

## Auto-hide and lifetime

The portable timer uses `cui_frame_time`, one context resource per auto-hide bar,
and no OS clock or per-frame allocation. Initial submission, a scroll value
change, owner hover entry/movement and owner/bar scroll or keyboard activity
refresh the idle deadline. Bar hover, capture, drag, direct owner/bar focus and
an applicable named focus scope keep it visible. Arbitrary focused descendants
of an ordinary box do not hold an overlay visible indefinitely.

Default idle/fade durations are 1/.15 seconds. Positive finite durations are
capped at 60/5 seconds. The helper requests redraw only before the idle/fade
deadline. The final zero-opacity frame requests no further redraw. A held bar
requires no timer frames; releasing it starts a fresh finite idle period.
`no_fade`, skin reduced motion, component reduced motion where exposed, and
context reduced motion remove fading while retaining the idle deadline. Hosts
must honor ordinary `cui_needs_redraw` invalidations and advance frame time.

Visible/hidden/inherited common bars allocate no timer. Formerly barless overlay
owners use one bounded context drag resource while opted in. Surface additionally
uses one bounded stack of 64 paired submissions, allowing nested inactive and
active skins without retaining caller state past frame completion. An inherited
Surface with no prior opt-in stack allocates nothing. Capacity exhaustion uses
the existing context capacity error; Surface refuses begin before opening its
composition if its stack cannot be obtained. Never pair footer/end with a failed
begin. Resource cleanup follows the existing complete-frame omission rule.

The new `cui_resource_find` is an in-frame lookup of an existing resource. It
never allocates and reports no error for missing/invalid input. A successful
lookup marks the resource used this frame. Returned memory remains owned by the
context and follows the existing renderer-completion/next-begin lifetime.

The split overlay helpers reduce before content layout and paint afterward.
Paint receives final offsets explicitly, so wheel correction and discrete row
quantization use the actual model position. Its local viewport coordinates must
cancel any ancestor content translation; built-in callers do this. Invalid
nonfinite geometry/offsets leave caller offsets unchanged. These helpers add bar
interaction, not wheel/touch ownership or automatic wrapping.

## Validation

Headless tests cover shared colors/radii/thickness/minimums, hidden hit regions
and wheel movement, finite fade completion/no perpetual redraw, reduced motion,
capture retention, list/menu window movement without selection callbacks, both
tab axes, tree and editor bars, fixed Surface footers, Surface allocation failure
and inherited zero allocation, finite Carousel selection, and wide TextView
table geometry. Existing interaction suites retain legacy INHERIT coverage.

`tests/web/test_scrollbar_appearance.py` drives the actual WASM gallery and CDP
touch contacts. It checks common families, hidden semantic ownership and real
wheel/touch scrolling, plus idle/capture behavior. The gallery controls apply
one shared policy to the current example and respect light/dark palettes. This
does not claim OS-native scrollbar rendering, infinite-range scrollbar support,
or new scrolling behavior for components without a scroll model.
