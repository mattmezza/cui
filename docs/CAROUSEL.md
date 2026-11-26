# Carousel

The module follows the frozen GPUI Kit 0.7.1 Carousel state, content, item,
scroll mask, navigation controls and pagination contracts. Its pure base model
requires only its source and `libm`; styled rendering uses existing Core,
Theme, Icon and Tooltip components. There are no persistent allocations.

Initialize `cui_carousel_state` with caller-owned geometry storage for 1–1024
items. The state and storage must remain valid through `cui_end_frame`.
`count` may be zero; the selection is then `CUI_CAROUSEL_NO_SELECTION`.
The initial nonempty selection is zero. Styled rendering synchronizes the
model count to its supplied descriptor count on each call. Programmatic count/orientation/looping
setters cancel interaction; count changes clamp selection, and an empty-to-
nonempty transition selects zero. `cui_carousel_set_selected` clamps silently.
User selection and navigation emit `on_change` only for a valid, different index.
Callbacks execute synchronously; their user pointers remain application-owned.
The callback may inspect state, but must not re-enter a render or replace its
geometry workspace during a call.

The drawing-free geometry API accepts finite, positive, sorted nonoverlapping
item bounds along the primary axis. It validates the entire input before writing
state or storage. Viewport extent is positive. Nonlooping snaps align item starts
and clamp to the scrollable extent. Navigation skips items that share the same
clamped snap, while explicit controlled/user selection can still select them.
Nearest-snap ties choose the earlier logical item. The cycle includes a trailing
gap matching the first inter-item gap. Runtime looping requires more than one
item and a cycle at least as large as the viewport; a short cycle falls back to
ordinary bounded navigation. Before geometry is available, configured looping
supports logical wrapping, as in source.

The styled root is a focusable, named Region with a clipped frame and optional
controls/pagination callbacks. Each logical item renders exactly once every
frame, including offscreen items. Item wrappers expose Group semantics, an
accessible label and position/set-size metadata. Looping repositions the one
instance of each item around the visible cycle; it never duplicates callback
state, child control IDs or content. Adjacent first/last navigation targets the
neighboring cycle and silently rebases after settling, without another change
callback. Long active drags periodically normalize their coordinate so the
workspace and animation distance remain bounded.

`cui_carousel_default` supplies growing width, a 200px frame height and 16px
leading item padding. The root has a 16px gap between composed parts. This is
an explicit C layout default; source content height is supplied by its host
layout. Item `extent=0` fills the frame along the primary axis and includes the
leading padding. A positive extent is the complete outer item size, including
that padding, and must exceed `gap`. Customize the appearance with `box`.
Use `cui_carousel_with_layout` for content-sized slides or the pure geometry
API for separate presentation.

## Show several slides at once

Pass one fraction per item to `cui_carousel_with_basis`. Three items with
a basis of `1.f / 3` fill the viewport together. The fraction applies to
the padded viewport, so the first item aligns with the viewport edge and the
same `options.gap` appears between items. For a vertical Carousel the
basis uses the frame height.

```c
/* Called once. Retain the state and workspace through each frame. */
cui_carousel_state_init(&state, workspace, 4, 4);

/* Called during a frame. The host provides the available layout width. */
cui_carousel_options options = cui_carousel_default();
options.width = available_width;
options.height = 200;
options.gap = 12;
options.now_seconds = now_seconds; /* Monotonic host frame clock. */
const float basis[] = {1.f / 3, 1.f / 3, 1.f / 3, 1.f / 3};
cui_carousel_with_basis(ui, id, &theme, &state, slides, 4, options, basis);
```

Use zero for an item that should keep its absolute `extent` or fill the
viewport. A positive basis replaces the item's extent, leaves at least one
logical pixel after the gap, and clips content to the slide. Fractions outside
`[0,1]` and nonfinite fractions are rejected before writing the state.
`NULL` keeps the ordinary extent rules. A resize recomputes snaps around the
selected item without a selection callback. When several indices share the
final snap, Next and Previous skip the duplicates. Explicit pagination can
still select any index.

Choose **Two slides** or **Three slides** in the live Carousel gallery to
try the layout in either orientation.

Frame bounds are retained through `cui_track_bounds`. Growing width starts with
a 300px estimate until the first completed layout; fixed width uses its declared
value. A changed measured frame schedules a follow-up frame and recalculates
snaps. Geometry changes cancel active gestures and align the current selection.
The host drives frames until `cui_needs_redraw` clears. This also applies to
responsive items using growing width. `now_seconds` is an
explicit monotonic host clock; backwards motion timestamps hold their last value.
The default movement spring has a 280ms response, damping .85 and settling
precision .5px. Styled rendering uses Theme `spring_move` response/damping;
reduced motion or disabled travel settles immediately. Critical and overdamped
settings also have an analytic solution. Settled motion does not request frames.

### Fit individual slides to their content

Use `cui_carousel_with_layout` for content-sized slides in immediate C. Give
each fit slide its own zero-initialized `cui_carousel_fit` and keep it with the
slide's ID when items reorder. This example uses the first two slides' natural
sizes, one half-width slide, and one slide with its ordinary extent.

```c
/* Retain these fields beside the Carousel model in your view. */
cui_carousel_fit build_fit = {0}, preview_fit = {0};

/* During each frame, slide callbacks submit in-flow content such as text,
 * images with natural dimensions, or boxes with explicit dimensions. */
cui_carousel_fit *fit[] = {&build_fit, &preview_fit, NULL, NULL};
const float basis[] = {0, 0, .5f, 0};
cui_carousel_with_layout(ui, id, &theme, &state, slides, 4, options,
                         (cui_carousel_layout){.basis = basis, .fit = fit});
```

In a real view, put the two workspaces in the same persistent object as
`cui_carousel_state` and initialize them once. Both arrays in
`cui_carousel_layout` can be frame-local. Leave `extent` zero on a fit slide.
A fit slide cannot also have a positive basis. Styling applies to the natural
slide box, including its padding. A positive `box.max_width` or `box.max_height`
on the primary axis can cap that slide below the viewport size.

The first submission gives a fit slide the viewport's primary extent. Layout
measures the slide once its children are submitted. On the next requested
frame, Carousel uses that result to position slides and build snaps. Every
slide renderer still runs once per frame. Content that grows stays clipped to
its previous reserve until the new geometry is applied. A size or axis change
can request a follow-up frame and realign the selected slide without emitting
a change callback. Use `cui_needs_redraw` to drive those frames. A slide needs
at least one in-flow child or padding to give it a natural extent. A child
that only fills its parent supplies no preferred size.

Select **Fit to content** in the live gallery to try the same C behavior on
either axis and copy a full C example.

### Retained slides

In the C bridge, set each direct Carousel Item's fraction with
`cui_bridge_set_carousel_slide_basis_v1(app, item, fraction)` before or after
the first frame. The bridge stores the fraction with the Item. A positive
Carousel width from `cui_bridge_set_visual_size` fixes the viewport. Leaving
the width unset or passing zero grows it to the parent allocation while
any Item has a positive fraction. Reset to zero for a full-width slide.
Resetting or removing the last fractional slide restores the default 300px
viewport when no width was set. These layout edits are silent.

Python, JavaScript and TypeScript use `item(...).carousel_basis(0.5)`. Go uses
`Item(...).CarouselBasis(0.5)`. In Rust, use `carousel_basis(0.5)?` on an Item
Builder or `app.carousel_basis(item, 0.5)?` after creating the Item. Each binding
can query `supports_carousel_basis()` (Go `SupportsCarouselBasis()`). With an
older bridge, omit the setter to keep full-width slides and ordinary navigation.

For a vertical retained Carousel, call
`cui_bridge_set_carousel_axis_v1(app, carousel, CUI_BRIDGE_CAROUSEL_VERTICAL)`.
Set a positive height with `cui_bridge_set_visual_size` or use the default
200 logical pixels. Fractional slides use the height as their primary extent.
The width keeps the same fixed or growing rules. Changing the axis applies on
the next frame, ends the previous gesture and keeps the selected Item without
emitting a selection event.

Python, JavaScript and TypeScript use `carousel_axis("vertical")`, Go uses
`CarouselAxis(cui.CarouselVertical)`, and Rust uses
`carousel_axis(CarouselAxis::Vertical)?`. Query `supports_carousel_axis()`
(Go `SupportsCarouselAxis()`) before requesting vertical layout on older bridges.

To wrap from the last slide to the first and back, use
`cui_bridge_set_carousel_looping_v1(app, carousel, 1)`. The default is off.
Looping requires at least two slides and a measured track, including the closing
gap, at least as long as the viewport. A shorter track uses bounded navigation.
Changing the setting cancels any active gesture on the next frame. The selected
Item stays the same and the configuration itself emits no change event.

Python, JavaScript and TypeScript expose `carousel_looping()` and accept
`carousel_looping(False)` or `carousel_looping(false)` to turn it off. Go uses
`CarouselLooping(true)`. In Rust, use `carousel_looping(true)?` on the builder or
`app.carousel_looping(carousel, true)?` on a mounted Carousel. Call
`supports_carousel_looping()` (Go `SupportsCarouselLooping()`) before setting the
option when loading an older bridge. Omitting it keeps bounded navigation.

Enable built-in arrows and slide markers independently with
`cui_bridge_set_carousel_controls_v1`. Pass `CUI_BRIDGE_CAROUSEL_ARROWS_V1`,
`CUI_BRIDGE_CAROUSEL_PAGINATION_V1`, their bitwise OR, or zero to hide both.
Both groups are off until enabled. They appear below the viewport, so the full
Carousel can be taller than its `visual_size` height. Markers wrap to fit the
viewport width. The arrows follow the current axis and disable at finite ends.
All buttons inherit the Carousel's disabled state. They use the same selected
Item change event as wheel, drag, and keyboard navigation. Configuration itself
is silent and preserves selection.

Use `carousel_controls()` in Python, JavaScript, and TypeScript to show both
groups. Python accepts keyword arguments such as `carousel_controls(pagination=False)`.
JavaScript and TypeScript take an options object such as
`carousel_controls({pagination: false})`. Go takes
`CarouselControls(cui.CarouselControls{Arrows: true, Pagination: true})`.
Rust takes `carousel_controls(CarouselControls { arrows: true, pagination: true })?`
on the builder or the corresponding App method. Probe
`supports_carousel_controls()` (Go `SupportsCarouselControls()`) when using
older bridge libraries and omit the option if the provider lacks it.

### Size retained slides to their content

Call `cui_bridge_set_carousel_slide_fit_v1(app, item, 1)` on a direct
Carousel Item to size it to its in-flow content on the active axis. The
bridge owns that Item's measurement storage. A fitted slide uses the
Carousel's available width when the Carousel has no explicit visual width.
It preserves selection and sends no selection notification when the
measured size changes. Headless apps should step again after a layout
or content change to apply the new measurement.

Enabling fit replaces a previous fraction. A subsequent `carousel_basis`
call replaces fit, including a zero fraction, which restores the full
viewport extent. Disabling fit restores a fitted Item's ordinary size.
Disabling fit on an Item with a fraction preserves that fraction.

Probe the optional API before enabling the mode if you load a shared
bridge from an earlier release:

| Language | Enable on a direct Item | Capability |
| --- | --- | --- |
| Python | `slide.carousel_fit()` | `app.supports_carousel_fit()` |
| JavaScript and TypeScript | `slide.carousel_fit()` | `app.supports_carousel_fit()` |
| Go | `slide.CarouselFit(true)` | `app.SupportsCarouselFit()` |
| Rust | `builder.carousel_fit(true)?` or `app.carousel_fit(slide, true)?` | `app.supports_carousel_fit()` |

With an older bridge or Node addon, enabling fit reports that the feature
is unavailable. Passing false is safe as a fallback. Keep ordinary or
fractional sizes on those providers.

This complete Python example mounts a horizontal Carousel and updates a
slide's contents. It runs headlessly by default.

```python
import cui_bridge as cui

with cui.App(width=640, height=320) as app:
    stage = cui.item("build", "Build")
    detail = cui.label("status", "Ready")
    stage.children(detail)
    release = cui.item("release", "Release")
    if app.supports_carousel_fit():
        stage.carousel_fit()
        release.carousel_fit()
    elif app.supports_carousel_basis():
        stage.carousel_basis(0.5)
        release.carousel_basis(0.5)

    rail = cui.carousel("workflow").visual_size(360, 120).children(stage, release)
    if app.supports_carousel_controls():
        rail.carousel_controls()
    app.mount(cui.column("root").children(rail))
    app.step()
    app.step()
    before = app.bounds(stage)[2]

    detail.label("Build artifacts are ready for download")
    app.step()
    app.step()
    after = app.bounds(stage)[2]
    print(f"Build slide width: {before:g} → {after:g}")
    if app.supports_carousel_fit():
        assert after > before
    else:
        assert after == before
```

### Size arrows and markers

The retained bridge accepts independent arrow and marker sizes with
`cui_bridge_set_carousel_control_sizes_v1`. Use the shared `xs`, `sm`,
`base`, `md`, `lg`, `xl`, and `2xl` through `9xl` scale. Inherit
restores 32px arrows and 24px markers. The size names select the square
button's edge using the shared design-scale control height. Carousel caps
oversized buttons at the viewport width, stacks arrows when needed and wraps
markers onto more rows. Selection and activation use the ordinary event path.
Invalid input leaves both sizes intact.

- Python uses `rail.carousel_control_sizes(arrows="lg", pagination="xs")`.
- JavaScript and TypeScript use
  `rail.carousel_control_sizes({arrows: "lg", pagination: "xs"})`.
- Go uses `rail.CarouselControlSizes(cui.CarouselControlSizes{Arrows: cui.LG, Pagination: cui.XS})`.
- Rust uses `rail.carousel_control_sizes(DesignSize::Lg, DesignSize::Xs)?` on a builder,
  or the App method with the Carousel node.

Set either group to `inherit` to restore its default. Probe
`supports_carousel_control_sizes()` (Go `SupportsCarouselControlSizes()`)
with older bridge providers. The live gallery applies its **Design scale**
selection to both groups. Its copyable immediate C example uses
`cui_design_measure` and `CUI_CAROUSEL_CONTROL_CUSTOM`.

### Customize retained control content

Create a passive content row with `cui_bridge_carousel_slot_create_v1`.
Previous and Next slots belong to the Carousel. A Marker slot belongs to its
direct slide Item, so it follows the slide when its position changes. Each
owner allows one slot per role. Enable the corresponding control group to
show it. The 64-slide limit applies to slide Items.

Use the slot's text as the accessible name and give its children the visible
content. An empty name restores the default. An empty row keeps the built-in
artwork. Rows and columns can compose labels, badges, avatars, keyboard
hints, tags, icons and images. Content is centered and clipped to the
built-in button bounds. The button keeps focus, activation, disabled state
and its ordinary selection event, even when a pointer lands on its artwork.
Removing the slot restores the default control.

Python, JavaScript and TypeScript use `carousel_slot(key, role, label)` with
`"previous"`, `"next"` or `"marker"`. Go uses
`cui.CarouselSlot(key, cui.CarouselSlotNext).Text(label)`. Rust uses
`app.build_carousel_slot(owner, CarouselSlot::Next, key)?.text(label)?`.
Probe `supports_carousel_slots()` (Go `SupportsCarouselSlots()`) when loading
an older provider. Ordinary arrows and markers can still be enabled if
their capability is available.

### Place your own navigation buttons

Omit the built-in arrows when your app owns its navigation controls.
Place ordinary Buttons anywhere in your layout. Read the next reachable
slide with `cui_bridge_carousel_adjacent_v1`. In the button handler use
`cui_bridge_carousel_step_v1` to select that slide and queue the ordinary
Carousel change. Both operations take 0 for Previous or 1 for Next. The
returned Item is zero when navigation in that direction is unavailable.

These operations use the last laid out snaps. They honor disabled state,
skip duplicate snaps, and use the effective looping policy. The current
slide order, axis, and looping policy must have been submitted. When
a slide size changes, the new snaps are available after the corresponding
layout. A successful Step uses the existing copied change event, with the
selected Item and the layout's frame sequence. A query leaves selection and
the queue unchanged. A full queue rejects Step without changing selection.
Use `cui_bridge_set_selected` for a silent programmatic selection.

| Language | Find the neighboring Item | Select it and queue the change |
| --- | --- | --- |
| Python, JavaScript and TypeScript | `rail.carousel_adjacent(next)` | `rail.carousel_step(next)` |
| Go | `app.CarouselAdjacent(rail, next)` | `app.CarouselStep(rail, next)` |
| Rust | `app.carousel_adjacent(rail, next)?` | `app.carousel_step(rail, next)?` |

Use `supports_carousel_navigation` (`SupportsCarouselNavigation` in Go)
before using these methods with an older library or Node addon. A caller
can use the built-in arrows when those are available. The Python example
places its own buttons above the slides and refreshes their disabled
states after the initial layout and when the selection changes.

```python
import cui_bridge as cui

with cui.App(width=640, height=320) as app:
    build = cui.item("build", "Build")
    release = cui.item("release", "Release")
    rail = cui.carousel("workflow").visual_size(320, 120).children(build, release)
    root = cui.column("root")
    if app.supports_carousel_navigation():
        back = cui.button("back").label("Previous").disabled()
        forward = cui.button("forward").label("Next").disabled()
        back.on_click(lambda event: rail.carousel_step(False))
        forward.on_click(lambda event: rail.carousel_step(True))

        def refresh_controls():
            back.disabled(rail.carousel_adjacent(False) is None)
            forward.disabled(rail.carousel_adjacent(True) is None)

        rail.on_change(lambda event: refresh_controls())
        root.children(cui.row("toolbar").children(back, forward), rail)
        app.mount(root)
        app.step()
        refresh_controls()
        app.step()

        # Simulate a user clicking Next in this headless example.
        x, y, width, height = app.bounds(forward)
        app.queue_pointer("down", x + width / 2, y + height / 2)
        app.queue_pointer("up", x + width / 2, y + height / 2)
        app.step()
        app.dispatch()
        assert rail.selected() is release
    else:
        if app.supports_carousel_controls():
            rail.carousel_controls()
        app.mount(root.children(rail))
        app.step()
        assert rail.selected() is build
```

Step queues the change for the binding's normal dispatch. Python's
dispatcher can deliver it after the button handler returns. Refresh
external controls after changing the axis, looping policy, slide order
or available size as well as after selection changes.

### Bounds and runnable example

For host-driven input and tests, the existing numeric part bounds API reports
positions from the latest completed frame. Parts 1 and 2 are Previous and Next.
Part `3 + i` is the marker for zero-based slide index `i`. Disabled controls
still have bounds. Omitted controls return an invalid-state error. Bounds use
the current retained slide order, so refresh them after edits and a completed frame.

This Python example mounts a Carousel with two visible slides and a third to
scroll to. It enables vertical layout and wrapping when the loaded bridge
supports each option.
The equivalent JavaScript or TypeScript uses the same builder methods.
Use the native app constructor and run loop to show the UI in a window. The
[builder guide](BINDING_BUILDERS.md) shows how to start a native app.

```python
import cui_bridge as cui

with cui.App(width=640, height=360) as app:
    slides = [
        cui.item("overview", "Overview"),
        cui.item("activity", "Activity"),
        cui.item("reports", "Reports"),
    ]
    if app.supports_carousel_basis():
        for slide in slides:
            slide.carousel_basis(0.5)

    rail = cui.carousel("dashboard").visual_size(320, 200)
    vertical = app.supports_carousel_axis()
    if vertical:
        rail.carousel_axis("vertical")
    looping = app.supports_carousel_looping()
    if looping:
        rail.carousel_looping()
    controls = app.supports_carousel_controls()
    if controls:
        rail.carousel_controls()
        if app.supports_carousel_control_sizes():
            rail.carousel_control_sizes(arrows="lg", pagination="xs")
        if app.supports_carousel_slots():
            rail.children(cui.carousel_slot("forward", "next", "Next dashboard").children(
                cui.label("+"),
            ))
            slides[-1].children(cui.carousel_slot("reports-marker", "marker", "Go to reports").children(
                cui.label("R"),
            ))
    rail.children(*slides)
    app.mount(cui.column("root").children(rail))
    app.step()
    if controls:
        # In a native window, click the arrows or slide markers directly.
        next_x, next_y, width, height = app.numeric_part_bounds(rail, 2)
        app.queue_pointer("down", next_x + width / 2, next_y + height / 2)
        app.queue_pointer("up", next_x + width / 2, next_y + height / 2)
        app.step()
        assert rail.selected() is slides[1]
        rail.select(slides[0])
        app.step()
    x, y, _, _ = app.bounds(rail)
    dx, dy = (0, -1) if vertical else (-1, 0)
    app.queue_scroll(x + 30, y + 30, dx, dy)
    app.step()
    assert rail.selected() is slides[1]
    if looping:
        for _ in range(2):
            app.queue_scroll(x + 30, y + 30, -dx, -dy)
            app.step()
        assert rail.selected() is slides[-1]
```

Retained Carousel uses a built-in control layout and spacing. Immediate C
supports custom button boxes and layout. Use `cui_carousel_control_inline` for an
axis-aware navigation button in ordinary flow or `cui_carousel_control` for
controls positioned at the sides of the slide viewport.

## Input ownership

Pointer processing happens before child callbacks using Core's previous-frame
logical scope ownership and the measured slide-frame bounds. Navigation and pagination controls outside that frame never start a drag, so clicking them during spring motion cannot first snap to a different slide. A tap retains ordinary child press/release/activation.
Movement greater than 2px locks to the dominant axis; ties lock to the Carousel
axis. A stronger cross-axis move cancels the drag so ancestors can handle it,
and suppresses that sequence's click. A locked release selects the nearest snap.
Both locked drags and cross-axis cancellation cancel only their pending release
activation, leaving the release and capture cleanup available to the child.
Controls can be clicked normally again after the gesture. Primary-axis pointer
motion/release are cooperatively consumed; unrelated controls and overlays keep
Core hit priority. A disabled Carousel cancels gestures and ignores keys and
wheel navigation. Its built-in arrows and pagination inherit that state on the
same frame, including when a held button is released or a queued keyboard or
accessibility activation arrives. The current slide stays selected. Apply your
own disabled policy to custom interactive children.

Wheel delta signs match Core and Scroll: a positive delta moves backward,
negative forward. A discrete wheel burst steps once and remains owned until
28ms silence; a burst beginning at an edge belongs to its ancestor. Diagonal
wheel motion uses its dominant axis, with ties vertical. Precise input uses
`scroll_precise` and `scroll_phase` to maintain a gesture-wide dominant-axis
lock and continuous pixel offset. Started/Moved gestures retain ownership at
an edge, including overshoot. Ended snaps once; Cancelled restores the original
offset and emits no change. Zero-delta Ended/Cancelled still finish an active
gesture. Moved-only streams settle after 28ms silence. Host adapters must supply
reliable precise/phase metadata where available; SDL/DOM wheel inference is not
provided by this module.

Scroll handoff is cooperative. Pair a parent `cui_scrollable_begin` with
`cui_scrollable_end_ex` and its mutable state, so end-phase reduction receives
child-deferred events, avoids replay, and updates the same frame's layout offset.
The tests cover this actual styled Scroll composition as well as an independent
ancestor reducer after children. The older const-state `cui_scrollable_end` cannot
revisit a dynamically deferred quiet-burst event. Carousel still observes a
raw-hit edge event already reduced by the parent begin phase so a reversed delta
in that same burst remains deferred rather than selecting a slide.

Arrow keys navigate only the configured orientation. Home/End select the first
or last item. Children process input before Carousel navigation, so a consuming
text editor or inner component retains its key handling. Changed navigation
consumes the key; an invalid boundary action remains available to ancestors.
The root and controls preserve ordinary Tab stops. A pointer activation of a
navigation/pagination control focuses the root and suppresses its focus ring;
a keyboard activation preserves control focus. Root pointer focus keeps arrow
navigation without drawing the ring; losing focus restores it. The ring can be
hidden explicitly.

Navigation controls default to Medium 32px round outline buttons with real
16px Chevron icons, chosen for axis/direction. They sit 16px outside the frame
and use fixed dimensions immediately, or retained/estimated growing width.
A growing frame's first placement is provisional and corrects after layout. `size` selects XS/Small/Medium/Large (20/24/32/32px); their icons are
12/14/16/24px. Custom uses `custom_size` and a 75% icon dimension; `custom_box` and `content` replace presentation.
Previous/next disable themselves when no distinct reachable snap exists.
Optional caller-owned Tooltip state/options compose the real delayed Tooltip;
its host viewport must be supplied. Labels default to “Previous slide” and
“Next slide”; hosts can provide localized/accessibility labels.

Pagination items default to XS 20px compact buttons with selected metadata.
They deliberately have no invented dot/number content: applications provide
`content` and `custom_box` to match their chosen pagination presentation. Invalid
indices disable the item. The default accessible label includes the slide
number. `cui_carousel_pagination_begin/end` supplies the source named Group and an
8px-spaced, centered row, with an optional custom box. All callback arrays,
strings and Theme pointers are borrowed for their call. Wrapper IDs must be
unique and differ from IDs submitted by callback-owned controls.

Captured touch scrolling uses `cui_carousel_touch_scroll`: precise pixel deltas retain their dominant axis and live offset until an explicit ended/cancelled phase. Pauses between finger movements do not trigger the 28 ms wheel-burst timeout. Styled Carousel selects this path for Core events whose pointer type is touch. Hosts using the headless model must deliver a terminal phase or call `cui_carousel_cancel` on capture/focus loss. Mouse wheels and ordinary precise trackpad bursts retain their documented timing policy.
