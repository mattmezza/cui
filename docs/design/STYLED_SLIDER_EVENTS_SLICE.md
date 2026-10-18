# Styled controlled Slider event slice proposal

Design only at `6a7caab3`. No production API below exists yet. Keep legacy
`cui_slider` and its float pointer, layout and return semantics unchanged.

## Source finding and proposed API

[`cui_slider`](../../src/components/cui_components.c) builds an outer 260-unit
control containing its label and a 240-unit, 8-unit-high bar. Its Base reducer
uses the **outer ID bounds** for pointer normalization, not the visible bar.
Outer padding/borders and the label therefore participate in the old hit identity.
Do not alter that legacy geometry as part of the additive double API.

The double reducer in [`src/base/cui_base.c`](../../src/base/cui_base.c) already
validates ranges, retains preview/capture state, and dispatches actual indexed
phases through [`cui_base_slider_events`](../../include/cui/cui_slider_events.h).
It normalizes pointer x using `cui_bounds(ui,id)` and registers that same ID as a
Slider. Reuse it once, without changing Core or introducing a second event loop.

Propose a separate `cui_styled_slider_events.h` including `cui_slider_events.h`
and `cui_theme.h`, plus a discoverability comment in `cui_components.h`:

```c
typedef struct {
    float width;        /* 0 = 240 logical units; positive fixed width. */
    float track_height; /* 0 = 8; visual bar only. */
    float hit_height;   /* 0 = 24; must be >= resolved track_height. */
    float font_size;    /* 0 = theme.font_size. */
    bool hide_label;    /* Semantic name remains supplied. */
    bool custom_colors;
    cui_color track_color, fill_color;
} cui_slider_style_options;

cui_component_slider_result cui_slider_events(
    cui_context *, cui_id, const cui_theme *, cui_str name,
    double value, cui_slider_state *, cui_slider_options,
    cui_slider_style_options, cui_component_slider_handler);
```

Use explicit logical-unit size overrides rather than a new preset/density system.
No vertical orientation, thumb renderer, content callback, animation or hidden
retained state is proposed. Default colors use theme primary with the existing
20-percent-alpha track convention; disabled rendering uses a documented theme
muted treatment. Use theme radius clamped to half the visual bar height; radius
zero stays square. No platform/Clay types enter the API.

All resolved dimensions must be finite and positive, <=4096; zero means default,
negative is invalid. Validate theme inputs actually used, name and style before
opening boxes or invoking Base. Reject an effective hit height below track height.
Preserve Base's exact value/range/step/state-ID and callback-conflict admission:
legacy `on_request` and `user` must be NULL. Avoid partial invalid submissions by
performing the same required public-input validation before painting; do not
normalize invalid caller state into valid state or dispatch on validation failure.
Range span must be finite, nonnegative and confirmed value within bounds.

## Geometry and paint ownership

The public `id` identifies the track's entire fixed-width hit rectangle, whose
bounds are used by Base for both pointer normalization and semantic registration.
The painted bar starts at that rectangle's x and has **exactly its width**; y is
centered within hit_height. Thus the left/right painted endpoints map to min/max,
while the taller vertical hit area improves pointer usability without changing x.
Build a private derived-ID outer column for optional label plus the track; do not
register a second interactive outer semantic. Label stays noninteractive. Borders,
focus indication and padding must not shift the track's normalization endpoints;
paint focus decoration outside/around this fixed geometry. Expose no second
track-ID getter: callers already know `id`, and `cui_bounds(ui,id)` returns the
actual hit rectangle. Derived child IDs follow existing component conventions.

As with existing Base controls, events use the previous completed layout's bounds.
The first submission establishes bounds; it must not invent pointer geometry from
new style width before layout. A width change takes effect through the ordinary
next completed layout. Do not perform layout eagerly or call Base again to obtain
new bounds. Test resizing explicitly and document this retained-geometry boundary.

Confirmed `value`, range, step and `state.preview/start` stay double. Paint uses
`state.preview` only while `state.dragging` remains true after reduction; otherwise
paint the immutable confirmed entry value. Consequently a rejected drag previews
until release, then returns to confirmed value; accepted owner assignments appear
on the next submission. A same-call keyboard UPDATE/COMMIT must not paint a
rejected proposal merely because Base left `state.preview` populated. Escape or
option cancellation ends drag and returns paint to confirmed entry value; CANCEL
still proposes the captured start to the owner and never writes owner storage.
This is an explicit preview policy, not a confirmed CHANGE notification.

Clamp the chosen paint value to finite validated bounds in double, compute
`span = maximum-minimum` and, for positive span,
`ratio = (paint_value-minimum)/span`, clamp ratio to [0,1] in double, then convert
**only that bounded ratio** to float for the logical fill coordinate. For zero
span use zero fill, matching Base's normalized semantic value. Multiplication by
bounded finite width then remains finite. Never cast value/min/max/step/preview
through float. Core semantics remain normalized float 0..1 as already documented;
`value_description` supplies application units without changing numeric ABI.
No copied preview may replace the caller's confirmed snapshot in event payloads.

## Callback, capture and lifecycle contract

Use caller-owned zero-initialized `cui_slider_state`, one live state per ID. Base
owns its fields during submission. Keep handler/user, state, theme and context
alive; handlers may accept/reject by changing external owner state, but must not
mutate capture/preview state, destroy resources or recursively submit UI. Defer
omission/destruction until return and renderer consumption. Reuse all Base counts,
source/index evidence and BEGIN/REQUEST/COMMIT/CANCEL meanings verbatim.

Disabled, readonly, zero span and absent handler use Base cancellation behavior.
No callback means readonly, not a fallback float mutation. Removing the handler
stops capture without delivering unavailable notifications. Option-driven CANCEL
has its existing reason and absent input evidence. No synthetic activation is
added for drag/move/release. Two keys in one invocation propose from the same
confirmed entry snapshot, preserving Base behavior. Omission does not dispatch a
synthetic cancel; applications must finish/cancel according to existing state and
host lifecycle rules rather than assume drawing absence executes cleanup.

## Bounded implementation and verification

Proposed ownership: new public Styled Slider header, narrow additive implementation
in `src/components/cui_components.c`, discoverability comment in
`include/cui/cui_components.h`, new `tests/unit/test_slider_styled_events.c`, and
`docs/SLIDER_EVENTS.md` update. No Base reducer, Core, bridge, legacy float API,
CMake/generated SDK or manifest edits are needed; root owns registration/exports.

Tests must use actual queued input after completed layout, not call handlers by
hand. Compare Base and Styled phase payloads/counts for equal track bounds;
exercise pointer down/move/up, touch capture, nonprimary rejection, keyboard
Home/End/arrows, Escape, generic cancel, readonly/disabled/zero-span cancellation
and missing handler. Prove real bar endpoint pixels/coordinates at min/mid/max,
label placement, hit identity, resizing, and finite size validation. Use doubles
above float precision and narrow noninteger steps, owner accept/reject and multiple
same-frame keys; verify requested values remain exact and no same-call confirmed
paint read-through occurs. Check preview during drag, confirmed paint after reject,
copyable event evidence, result reads without duplicate effects, invalid arguments
atomic, and deferred omission. Re-run
[`test_slider_events.c`](../../tests/unit/test_slider_events.c),
[`test_slider_controlled.c`](../../tests/unit/test_slider_controlled.c), and legacy
Styled tests. Strict C11 and private-display native checks are required; a dedicated
headless browser fixture can follow once root wires the new header/source exports.
No browser/FFI or full Slider parity claim follows from native-only proof.
