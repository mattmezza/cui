# Plot viewport interaction

Plot navigation is opt-in. Existing numerical scale functions and
`cui_plot_view` retain their previous behavior. Retain a `cui_plot_viewport`
beside your application model; it owns the current and initial X/Y scales.
No operating-system types, allocation, animation clock, or bitmap is needed
by the viewport model.

Initialize once, reduce input before projecting data, then bind the submitted
surface. For example, inside a frame with a stable `plot_id`:

```c
/* Retain viewport across frames, initialize once. */
cui_plot_viewport viewport;
cui_plot_viewport_init(&viewport,
    (cui_plot_scale){CUI_PLOT_LINEAR, 0, 100, 28, 628, false},
    (cui_plot_scale){CUI_PLOT_LINEAR, -1.2, 1.2, 142, 12, false});
```

```c
cui_plot_viewport_options navigation = cui_plot_viewport_defaults();
navigation.disabled = disabled;
navigation.x_limit = (cui_plot_viewport_limit){
    .bounded = true, .minimum = -100, .maximum = 200,
    .min_span = 10, .max_span = 300};
cui_plot_viewport_reduce(ui, plot_id, &viewport, navigation);
/* Project your data with viewport.x / viewport.y, build retained geometry or
 * pixels, and update axes from those same scales before submitting the view. */
cui_plot_view(ui, plot_id, &theme, &view);
cui_plot_viewport_bind(ui, plot_id, &viewport, navigation);
```

The default options returned by `cui_plot_viewport_defaults` enable both axes
for pan and zoom. Zero-initialized options disable navigation. `pan_x`, `pan_y`,
`zoom_x`, and `zoom_y` select relevant axes independently. Changing the surface
size requires updating the current scale ranges before reduction; reset restores
initial domains while preserving those current pixel ranges.

The reducer supports:

- Shift plus primary mouse drag. Core capture keeps dragging outside the
  original surface; release or cancellation stops it and suppresses activation.
- Ctrl plus scroll, anchored at the pointer. Precise deltas use logical pixels;
  legacy wheel steps use 32 pixels. Relative factor is `exp(dy * .002)`, with
  the exponent bounded to ±20. A host normally negates DOM `deltaY` so upward
  wheel movement zooms in.
- Core touch gestures: one contact pans, two contacts pan and pinch. Pinch
  scale is relative to the preceding event. Pan runs before zoom at the new
  contact center, preserving the data under the old center at the moved center.
- Disabled or modal-blocked surfaces reject navigation. Plain mouse drag and
  plain wheel do not change the viewport.

Call reduction once per surface per frame. It registers a transient nonmodal
focus scope using the surface ID to query Core's original clipped, topmost hit
for scroll input. Do not separately register another focus scope with that same
ID. Binding preserves unrelated semantic details and enables gesture routing
and the grab/grabbing cursors only for the opted-in enabled surface. Supply the
same `disabled` state to the view and navigation options.

`cui_plot_viewport_transform` is also available for application commands. Both
axes update transactionally: invalid scales, invalid limits, nonfinite arguments,
or numeric overflow leave the viewport unchanged. Logarithmic scales transform
in log10 space; reversed pixel ranges are supported. Domain bounds and span
limits are expressed in transformed units, so logarithmic bounds are exponents.
Zero span limits are unconstrained. A bound narrower than `min_span` is invalid.
Span limits clamp zoom and domain bounds clamp translation; at a boundary the
pointer anchor may move because satisfying the configured bounds takes priority.
There is no inertia, rotation, animation, kinetic scrolling, or automatic data
clipping. Applications still clip/project and render their own data.

The browser gallery enables navigation and uses the resulting X/Y scales for
the line, area, step, radial, natural, rounded-bar, and dot examples. Its reset
button restores both domains. The demo constrains X to −100..200 with span
10..300 and Y to −5..5 with span .1..10 to remain within the software geometry
budget. These are gallery policies, not limits imposed by the public model.

Browser hosts must preserve input modifiers, send cancellation on unexpected
pointer capture loss, and prevent browser Ctrl-wheel zoom only for a claimed
plot surface. The gallery's Plot-only wheel branch leaves ordinary page wheel
scrolling unchanged, denies modal/disabled claims, and queues a precise
Ctrl-scroll event for authoritative Core dispatch. Touch remains in the shared
Core contact/gesture path; opted-in surfaces keep page-pan fallback from
stealing their gestures. Native callers use the same reducer with adapter input;
this change does not add a new native-device validation claim.

`tests/unit/test_plot.c` checks exact two-axis anchors, pan/pinch composition,
reset, span/domain limits, logarithmic reversed ranges, transactional overflow,
disabled and zero-option behavior, actual Core contact dispatch, mouse capture
cancellation, plain-wheel preservation, and modal rejection.
`tests/web/test_plot_interactions.py` uses real Chromium mouse input and CDP
touch contacts. It verifies exact projected mouse/touch deltas and wheel anchors,
two-contact pinch, capture loss, cancellation and next tap, disabled behavior,
and changes to painted data for all seven gallery shapes. Configuration inputs
and read-only domain/pixel observations are allowed; component actions do not
invoke exported model reducers. Chromium automation is not physical iOS,
Android, pen, or native touch-device coverage.

Validation of this change: warning-enabled native build; 93/94 existing CTest
checks passed, with the sole failure being the root-owned stale generated
distribution. Plot and styled Plot passed under ASAN/UBSAN (2/2). The strict
new browser suite passed 17/17, and the existing touch suite passed 42/42 while
rendering all 78 stories. The two public API examples above compile in real
C11 contexts with `-Wall -Wextra -Wpedantic -Werror`.

Run the browser regression after building the gallery:

```sh
uv run tests/web/test_plot_interactions.py --directory build/web
```
