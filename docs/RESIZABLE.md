# Resizable panels

This module follows the frozen GPUI Kit 0.7.1 Resizable group, panel, state and
handle sources. Base owns sizing and interaction; components paint the theme's
hairline and engagement indicator. It uses portable C11 and caller-owned bounded
storage, with no platform types or allocation during layout/drag.

```c
cui_resizable_slot slots[3];
cui_resizable_state state;
cui_resizable_state_init(&state, slots, 3); /* once, outside rendering */
cui_resizable_panel panels[] = {
    {.id = 101, .has_initial_size = true, .initial_size = 240,
     .content = sidebar, .content_user = app},
    {.id = 102, .content = main_content, .content_user = app},
};
cui_resizable_options options = cui_resizable_options_default();
options.now_seconds = now_seconds; /* monotonic host time */
options.reduced_motion = reduced_motion;
/* Between begin_frame/end_frame, inside a sized parent: */
cui_resizable(ui, 100, &theme, &state, panels, 2, options);
```

`disabled` freezes handle input while panel content remains usable. Whole-subtree
inert presentation can be selected through the normal group box option.

The state and slots must remain valid through `cui_end_frame`, which writes
retained group/panel bounds. Capacity is fixed on initialization, at most 1024.
Configure core `max_elements`, command and frame capacities for the composed
content as well: slot capacity does not reserve renderer/layout capacity. Slot
fields and measured bounds are exposed for inspection, not arbitrary mutation.
Reusing a state for unrelated simultaneously rendered groups is unsupported.

Horizontal is the default sizing axis; Vertical stacks panes. `extent` sets the
size along that axis, and `cross_size` sets the other dimension, matching the
source group's cross-axis `size` option. Zero selects automatic parent sizing;
box fixed-zero flags permit explicit collapsed dimensions. Group/panel box styles
permit padding, colors and borders, while group direction/gap and panel axis
size/min/max stay managed. Panel callbacks run inside their available content
area and may compose further Resizable groups with distinct IDs and states.
IDs are stable and unique across the UI; reserve the IDs returned by
`cui_resizable_handle_id` for this group.

Default ranges are 100 pixels to the largest float. Explicit initial preferences
are used when a stable panel ID first appears; otherwise remaining space is shared
by flexible panes. Automatic layout reads its actual first-frame panel bounds
before settling the retained model, preserving a 240-pixel sidebar beside a
flexible pane. Later container resizing preserves proportions subject to
constraints, with no second-frame divider jump. Both axes, nested groups and
first-frame full-cross-axis handle bands use real layout rather than root portals.
`hidden` removes a pane's body and adjoining handle while retaining its slot for
later reuse. Visible neighbors receive its available space.

`cui_resizable_sync` is the pure model entry point. It validates declarations before
mutation, preserves state through stable-ID reorder/replacement, and reports
`CAPACITY`, `INVALID` or `INFEASIBLE`. Infeasible minima deliberately overflow;
insufficient maxima deliberately leave unused space. Sizes still respect declared
ranges; `fit_constraints` exposes whether the full extent can be filled. This is
an improvement over the frozen source's unconstrained proportional scaling and
recipient growth beyond its maximum. Core clipping is a caller policy.

Dragging expands the preceding pane by borrowing from subsequent visible panes
in order. Shrinking can cascade into earlier panes at their minima and distributes
freed space to subsequent panes subject to maxima. Pointer capture continues when
the pointer leaves the nine-pixel band; a hard constraint remains fixed until the
pointer returns past that constraint. A primary drag release emits one completed
resize via `state.on_resize`; Escape, cancellation, disabling or removing the
active handle cancels ownership without a completion callback. Completion follows
the initiating primary button, rather than the source's global any-mouse-up hook. The observer and
userdata are borrowed, called synchronously, and must not replace state/storage
or declarations during rendering.

`cui_resizable_resize_panel` uses the same redistribution rules and emits a
completion even when a valid request is constrained to the existing size. The last
visible pane is driven through its previous sibling. Invalid indices/values are
no-ops. `insert`, `remove`, `reset` and `clear` provide explicit lifecycle operations;
keep the next rendered panel declarations in agreement with the state. Reset
preserves size and the currently declared ranges while clearing motion and active
ownership; it does not invalidate a collapsed pane's legal zero minimum. Lifecycle operations do not emit completed user-resize events.

`save` writes sizes in current stable-ID order; store matching IDs alongside them
in application preferences. `restore` validates the whole vector, applies current
constraints/proportions and emits one completion. This is a portable snapshot
boundary, not an implicit filesystem or JSON service.

Pane IDs have passive Group semantics, with an optional `accessibility_label`.
Handles are focusable Separators with the controlled pane ID, current size,
reachable range, visual divider orientation and resize cursor hint. Native/browser
cursor adapters are supplied by core. Horizontal handles accept Left/Right;
Vertical handles accept Up/Down. The step defaults to ten pixels; Shift multiplies
it by ten. Home collapses preceding panes as far as constraints permit; End grows
the controlled pane as far as it can borrow. These keyboard alternatives are a C
accessibility improvement: the frozen source implements pointer resizing only.
`cui_base_resize_handle` submits semantics and consumes actual targeted events
without painting; applications draw its band at the same ID.

The styled divider is a one-pixel theme border line. Its three-pixel rounded pill
has idle/hovered/pressed/dragging lengths 0/20/28/44 and opacities 0/.35/.6/.9.
Supply monotonic `now_seconds`. The persistent indicator retargets from its current
sample, including fade-out,
and requests frames only during motion. Defaults match the source's 120ms
cubic Bézier (.2,0,0,1). `duration_seconds` and `reduced_motion` are explicit host
options because C Theme currently has no shared fast-duration/easing tokens.
Use the constructor for source defaults; zero-initialized options select immediate
motion. `appearance` may replace the paint and can decline to retain the default;
its band uses -1 for a cross dimension that fills the current parent. It does not
replace hit testing, cursor, constraints or ownership.

`cui_resize_handle_rectangles` exposes source nine-pixel straddling and five-pixel
leading/trailing bands with exact outermost-pixel seams. The reusable
`cui_resize_handle_appearance` paints inside an application-owned open band. Unlike
the source's deferred hugging-edge pill, this helper respects ancestor clipping;
a host that needs the outer overhang may compose its own overlay appearance.
Clipping a following panel also clips its straddling handle, as the frozen source's
panel-style warning describes. Do not set panel clipping when that outside half
must remain grabbable; group/ancestor clipping safely constrains the whole layout.

Tracked boundaries: shared theme motion tokens and arbitrary native GPUI drag
payload objects are not built-ins here. Applications retain state and provide
content/callbacks; durable preference storage is host-owned. This implementation
provides substantive sizing, composition, painting and real input, not a claim of
complete native API equivalence.

The gallery demonstrates a small workspace with selectable files, a read-only code preview, contrasting pane backgrounds and live pane dimensions. Drag the divider or focus it and press arrow keys; the browser checks both the retained sizes and the visible pane widths/positions. The vertical mode demonstrates the same model with pane heights.
