# Dialog, AlertDialog, Sheet and composed Popover surfaces

`cui_surface.h` adds application-owned surface state, drawing-free reducers and paired styled composition. A surface ID and its derived close, confirm, cancel and title IDs must be unique within the UI. Applications call `cui_surface_open` with a valid trigger, submit open surfaces every frame, and consume explicit outcomes with `cui_surface_take_result`. `cui_surface_request_close` queues a programmatic close safely between frames and applies the decision policy once on the next submission. `on_close` runs after state changes and focus handling, including direct child-action closure. Call `cui_surface_close` during an active frame; if begin succeeded, its matching end remains required even after closing from a child action.

Dialogs, alerts and sheets trap focus. Popovers are nonmodal by default, with an explicit modal option. Already-consumed events are ignored by the reducer. Escape and outside primary pointer presses can dismiss an open surface; the options disable either policy independently. Alerts suppress outside dismissal and their header close button by default. A decision callback can reject confirm, cancel or close before state changes. Closing withdraws current scoped semantics and restores an enabled trigger, including when a nested dialog closes back into its parent.

`cui_popover_begin_with_outside_focus` and `cui_base_surface_begin_with_outside_focus`
accept `CUI_POPOVER_OUTSIDE_FOCUS_KEEP_POINTER` for nonmodal Popovers. When
an unconsumed outside primary press is accepted for dismissal, the Popover
closes and `on_close` observes the focus Core already resolved from input.
A clicked control can keep both its focus and activation. This also preserves
cleared focus when the press lands in blank space. The policy leaves input
consumption, veto, result and callback timing alone. Escape, modal dismissal,
child actions and programmatic close use ordinary trigger restoration.
See [Popover anchors](POPOVER_ANCHORS.md#keyboard-focus-and-the-trigger).

Custom triggers can use `cui_popover_take_secondary_press` after they submit
an enabled interactive ID. Apply your chosen open or request-close action
before composing the Popover. [Right-click trigger](POPOVER_ANCHORS.md#right-click-trigger)

The styled begin/end accepts arbitrary CUI children. `cui_surface_footer_begin` ends the body and opens a fixed footer row, keeping its controls visible even when the body contains oversized content; the regular surface end closes that footer. The body measures its own content through completed-layout bounds tracking, routes wheel events to its clipped viewport and clamps retained scroll when content shrinks. Changed geometry schedules correction frames and unchanged surfaces return to idle. Custom base layouts use parts 6/7 for the viewport/content and submit body semantics with `cui_base_surface_submit_body`. It draws a backdrop, title, optional description, close button and clipped body as viewport portals. `cui_alert_dialog` composes destructive confirm and outline cancel buttons. Alert title, description and actions share one vertically centered content block when it fits the panel. Overflowing alert content switches to a clipped, scrollable layout starting at the top, so the full content remains reachable. Each alert has an alertdialog semantic role; other surfaces use dialog, with a copied description and stable title relationship. A caller can compose its own alert actions using the documented derived IDs. Results persist until consumed or the surface is opened again.

Dialog dimensions clamp within an explicit viewport and optional uniform margin. Sheets support left, right, top and bottom placement with explicit width/height. Their inner edge has a captured pointer resize control, directional arrow sizing, Home/End limits and semantic size/range. Resize is enabled by default; options disable it or configure minimum/maximum extents. The application-owned `sheet_size` retains user sizing; set it to zero to return to option sizing. `cui_surface_current_geometry` includes that retained size. Custom base-only layouts submit the resize semantic inside its own box using `cui_base_surface_submit_resize`; the helper draws nothing. Popovers use the existing side-based flip/clamp popup geometry. For manually submitted nested surfaces, use distinct IDs and increasing layers. Styled Root manages layers for its dialog stack.

`test_surface` exercises actual styled confirmation buttons, rejected acceptance, pointer cancellation, all four themes, all four sheet edges, header close, Escape, outside dismissal, nested focus isolation/restoration and portal escape from an ancestor clip. The same behavior fixtures also run as `surface_base_only`, linking only core, models, overlay and surface base sources. Those fixtures emit only their own clipping commands, demonstrating no theme or component dependency in surface behavior.

This is partial GPUI Kit 0.7.1 parity. Remaining scope includes dialog backdrop styling and animated transitions, keyboard focus reveal in composed scrollers, and host lifecycle coverage. Popover supports both legacy flip placement and explicit anchor/offset/arrow placement. Native platform accessibility and IME behavior remain separate unfinished requirements.

For a child such as a command palette that handles Escape first, set `no_escape_dismiss` on its surface. Let the child consume Escape while clearing its query; on its empty-query cancel callback, call `cui_surface_request_close`. The next submission applies the decision callback and restores focus before registering modal content. Begin-phase dismissal cannot inspect event consumption that occurs later in child rendering.

## Sheet motion

Set `options.sheet_animation_seconds = 0.22f` to slide a sheet in and out from its configured edge. The default zero keeps immediate behavior. Motion reads `cui_frame_time(ui)`, which advances from the delta passed to `cui_begin_frame`; continue scheduling invalidated frames. `cui_surface_current_geometry` reports the moving bounds. Resize controls appear once entry completes.

With animation enabled, closing keeps the modal mounted during its exit and consumes input events. Focus returns to the trigger, `on_close` fires, and a result becomes available only when the exit completes. `state.sheet_closing` exposes this phase to application composition. Calling open during exit reverses toward the open position. Pass `options.reduced_motion = true` to snap directly to the destination; changing it during exit completes the close in the next submitted frame. Duration is finite, positive and capped at ten seconds; invalid values disable animation. Dialog/AlertDialog/Popover behavior remains immediate.

AlertDialog with unspecified/nonpositive finite height fits its measured title,
wrapped description and action row plus theme padding. The first frame measures
content; the next invalidated frame settles its height. Viewport margins still
cap the dialog, and overflowing content remains scrollable. Explicit positive
heights retain the fixed viewport contract. Resize, font or content changes can
require another measured frame.

## Independent Popover geometry

`cui_popover_position_resolve` calculates an anchor-mode rectangle without opening
a surface or modifying state. It supports eight anchors named for the popup's
own edge, signed explicit offsets, nonzero viewport origins and no-flip/no-shrink
clamping. Set `reference_rem` from application root sizing; zero selects 16 logical
pixels. The default gap is one quarter rem; enabling the arrow adds three eighths
rem. `cui_popover_arrow_resolve` calculates the triangle and border join patch for
the final clamped rectangle. Read `cui_surface.h` for finite-input, overflow and
output-preservation rules. Geometry remains fractional; host pixel snapping is
separate.

Use `cui_popover_begin` to apply this positioning and paint an optional arrow,
then balance a successful begin with `cui_surface_end`. Existing
`cui_surface_begin` retains legacy placement. See [Popover anchors](POPOVER_ANCHORS.md)
for ownership, failure, rendering and adapter limits. SDL3 and browser mesh adapters have bounded analytic coverage; exact
reference raster antialiasing remains unfinished. This is scoped C support, not
complete family or language-builder parity.

## Additive intrinsic Dialog composition

The separate C Base/Styled six-slot route is documented in
[DIALOG_COMPOSITION.md](DIALOG_COMPOSITION.md). It resolves current-frame
intrinsic sizing, independent Header/Title/Description, Content/Body allocation
and Footer ownership using Core layout V2. The legacy helpers documented above
keep their fixed or previous-measurement behavior. Retain the new layout storage
through end_frame and use its open helper for each new lifetime. The first
composed frame drops previous scoped activations and captures, including held
mouse, touch and Space. [Styled Root](ROOT.md#mount-a-composed-dialog-or-alertdialog)
can mount the six-slot surface automatically. The [retained C bridge](DIALOG_COMPOSITION.md#compose-a-retained-c-dialog)
exposes named child containers and queued typed close results. Native
window-service integration and language slot builders remain open.
