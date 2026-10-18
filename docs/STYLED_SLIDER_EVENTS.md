# Styled double Slider

Include `cui/cui_styled_slider_events.h` for `cui_slider_events`. This additive
controlled Slider paints a horizontal bar and optional label, using the existing
[Base double event reducer](SLIDER_EVENTS.md) exactly once. The old float-pointer
`cui_slider` API, layout and behavior are unchanged.

```c
static void position_request(void *user, const cui_component_slider_request *r) {
    double *owner = user;
    if (r->phase == CUI_SLIDER_UPDATE)
        *owner = r->requested; /* Accept; leave unchanged to reject. */
}
/* Persistent caller-owned data: */
cui_slider_state drag = {0};
double position = 10000000000.25;
/* During UI submission: */
cui_component_slider_result result = cui_slider_events(
    ui, slider_id, &theme, cui_string("Position"), position, &drag,
    (cui_slider_options){.minimum=10000000000., .maximum=10000000001., .step=.125},
    (cui_slider_style_options){.width=240},
    (cui_component_slider_handler){position_request, &position});
```

`result` has the Base facade's admission flags and actual callback counts;
reading it never dispatches another effect. BEGIN/UPDATE/COMMIT/CANCEL and their
copied evidence retain the Base contract. UPDATE is a request, not proof of owner
acceptance; no confirmed CHANGE is emitted. Legacy `options.on_request` and
`options.user` must both be NULL. A missing new handler is readonly.

## Geometry and style

The public ID identifies the fixed-width hit rectangle. The centered painted bar
has exactly its x and width, so its endpoints map to minimum and maximum. Its
default visual height is 8 logical units inside a 24-unit-high hit area. The label
sits above it and is noninteractive; hiding it does not remove the semantic name.
`cui_bounds(ui, slider_id)` reports the hit rectangle, not the private outer column.

Style zero values select defaults: width240, track_height8, hit_height24 and
font_size from the theme. Positive overrides are logical units, finite and <=4096;
hit_height must be at least track_height. Negative, nonfinite or oversized values
reject the call. Theme radius must be finite, nonnegative and <=4096; it is clamped
to half each painted shape's height. Zero radius stays square.

The default fill uses theme primary; the default track uses primary at 20-percent
alpha. `custom_colors` selects explicit track/fill colors. Disabled controls use
theme muted and its 20-percent-alpha track, including when custom colors exist.
Keyboard/pointer focus adds a 15-percent-alpha theme focus background to the hit
rectangle without a border or padding that could shift normalization endpoints.
No thumb, vertical variant, preset size or animation is implied.

Input uses **previous completed layout** bounds, as Base does. Submit once to
establish geometry. On a width change, this submission's queued input still uses
the last completed width; new paint/layout uses the new width. The next submission
uses that completed geometry. The facade does not rerun layout or dispatch.

## Confirmed value and drag preview

Owner value, range, step, preview and gesture start stay double. The range and its
span must be finite, ordered, with the confirmed value inside it; step is finite
and nonnegative. For rendering only, clamp the selected paint value within bounds,
compute a ratio in double, clamp it to [0,1], then convert that bounded ratio to
float for the logical fill coordinate. No confirmed or proposed value is cast
through float. Zero span paints zero fill. Semantic numeric values remain Base's
normalized float0..1; supply `value_description` for application units.

While `drag.dragging` remains true after reduction, paint its preview. Otherwise
paint the immutable confirmed value passed into the call. A rejected drag thus
previews while held and returns to confirmed paint on release. Keyboard requests
do not paint an unaccepted proposal. Accepting in the callback updates external
owner state, observed on the next submission; same-call semantics also retain the
entry snapshot. Two keys in one call propose from that same snapshot.

Escape/generic cancel and disabled/readonly/zero-span transitions retain Base
cancellation behavior. CANCEL offers the captured start to the owner; it does not
write owner storage. When capture ends, paint uses the confirmed entry value,
even if different from start. Removing the handler ends capture without delivering
callbacks to a missing handler. Omission itself dispatches no synthetic cancel.

Keep one zero-initialized caller-owned state per stable ID. During submission,
Base owns its capture/preview fields. Handlers may accept/reject through external
owner state, but must not mutate active state, theme or borrowed data, destroy
context/state/users, or recursively submit UI. Defer omission/destruction until
return and renderer consumption. All invalid public inputs are checked before
opening boxes: no callbacks, capture mutation or UI submission on rejection.

`tests/unit/test_styled_slider_events.c` uses software pixels for bar endpoints,
preview/rejection/acceptance and finite extreme ranges, plus real queued touch,
keyboard and cancellation input. It compares Base/Styled phases, exact doubles
above float precision, normalized semantics, resize boundaries, missing handlers,
invalid-input atomicity and deferred omission. The label test inspects text command
placement; its font-free pixel fixture does not claim glyph raster coverage.
Legacy Slider and component tests remain required. Browser/foreign bindings and
full Slider-family parity are outside this slice.
