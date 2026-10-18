# Picker applied observers

Include `cui/cui_picker_events.h` for additive applied date-selection observers:
`cui_calendar_reduce_events`, `cui_base_calendar_events`, and `cui_calendar_events`.
Each mirrors its legacy operation, appends `cui_date_selection_handler`, and returns
`cui_calendar_event_result` with the exact legacy `control` and `change_count`.

```c
static void selected_date(void *user, const cui_date_selection_change *event) {
    app_state *app = user;
    app->last_selection = event->value; /* Pointer-free copy. */
}
cui_calendar_event_result result = cui_calendar_events(
    ui, calendar_id, &theme, &calendar, &selection, options,
    (cui_date_selection_handler){selected_date, app});
```

This slice does not add TimeField, navigation, popup lifecycle or controlled-request
observers. A CHANGE reports an already-applied
date selection and cannot reject it. The existing disabled-date predicate remains
an enablement policy, not a notification or a newly rejectable transaction.

## Delivery and comparison

Each facade invokes its corresponding legacy operation **once** and compares
selected start/end at the outer return. Reduce-only does not submit semantics;
Base does; Styled completes painting before the observer. No nested observer or
reduction replay occurs. Custom popup hosts may use reduce-only and submit later,
preserving their own modal/close ordering. There is no submit-only observer.

At most one callback runs per operation. Multiple changes coalesce, and returning
to the initial selection produces no CHANGE. `change_count` counts actual callback
invocations; a missing handler still permits editing and yields zero count. Its
unused user pointer is ignored. Reading `control` never dispatches again.

Comparison uses `cui_date_compare` for each endpoint, exactly as the legacy
Calendar change calculation does. It compares date fields rather than struct bytes
or formatted strings. Zero/end-unset dates remain part of the legacy selection
representation; the facade adds no validity check, selection normalization or
range ownership rule. Range first selection starts/restarts the range, and range
completion can reorder endpoints. Same-date activation can set `control.activated`
without a CHANGE.

Displayed month, cursor movement, DAYS/YEARS/MONTHS chooser state, year page and
range-in-progress are distinct presentation state. Navigation may set the legacy
`navigated` flag without changing selected dates. Repairing invalid month/cursor
presentation also does not produce selection CHANGE. External assignments completed
before entry are not component-applied changes.

The copied payload contains previous/final `cui_date_selection`, CHANGE kind and
the calendar ID. Metadata is UNKNOWN source, HAS_FRAME with the actual nonzero
active frame, no event index, activation, cause or host timestamp. Aggregate
flags are not evidence from which these missing facts can be reconstructed.

## Passive work and resource ownership

Required context/state/selection pointers, Styled theme and nonzero ID are checked
before calling legacy code; failed admission has no maintenance, callbacks or
state writes. Otherwise existing passive maintenance remains intact. Notifications
are suppressed outside active painted submission, under layout-only ancestry or
when Calendar is disabled. Inert/modal routing is still Core's existing behavior;
this API adds no routing or propagation policy and does not claim every reducer
branch uniformly checks input consumption.

Observer/user are borrowed through the call. Keep selection, state, theme, locale
strings, options and disabled-policy userdata alive and unchanged. Existing policy
calls can occur repeatedly during scanning and painting; there is no one-call-per-
action guarantee. The facade does not copy/retain policy userdata. Observers must
not mutate, free or rebind active resources, destroy context, or recursively submit
UI. They may set unrelated flags; apply omission/removal after outer return and
defer teardown through frame/renderer consumption. Payload fields themselves are
pointer-free and remain safe when copied after later frames or context destruction.

`tests/unit/test_picker_events.c` compares old/new selections, state, policy call
counts, focus, legacy result, semantics and draw commands for all three entrypoints.
It covers multiple/net-zero changes, same-date activation, keyboard navigation,
reversed range completion, year/month chooser silence, disabled-day bounds/policy,
passive presentation repair, disabled/inert/layout-only silence, missing observer,
copy lifetime, deferred omission and atomic pointer/ID admission. Callback-time
checks distinguish reduce-only semantics from Base/Styled completion. Legacy
`picker` and `picker_base_only` suites remain required. Tests use the private native
display wrapper. No browser, host accessibility or foreign binding proof is implied.


## DatePicker selected dates

The same header provides `cui_base_date_picker_events` and
`cui_date_picker_events`, mirroring the old arguments plus date handler. Base
returns `cui_calendar_event_result`; Styled returns `cui_picker_event_result` with
its exact old bool in `changed` and zero/one actual `change_count`. Base requires
both trigger and calendar IDs nonzero; Styled requires its trigger ID and theme.
Required pointers/IDs are checked atomically before legacy code.

Each wraps the complete original operation once, with no nested Calendar observer.
The event target is the **outer trigger ID**, even though Base also takes a separate
calendar ID. The observer runs after all original popup dismissal, focus restoration,
trigger formatting, layout and paint work (or the original early return). Opening,
closing, navigating and activating the same selected date do not alone notify.
A first range selection may notify while the popup stays open; completing a range
can notify after it closes. Only selected start/end date comparisons decide CHANGE.

A late Styled failure can return false even after Base has changed dates: for
example, first range selection followed by invalid popup viewport placement.
The facade preserves that bool and still delivers the applied date CHANGE after
close/focus restoration. Do not treat `changed` as a callback count or successful
render signal, and do not gate applied notification on that bool. There is no
invented error or close reason in the payload.

Global disabled or Calendar-disabled options suppress observers while keeping old
trigger/overlay maintenance. Active painted/frame, UNKNOWN/no-index evidence,
missing-handler editing, copied payload, borrowed policy/locale data and deferred
teardown rules match Calendar above. DatePicker overlay state retains its existing
lifetime and the standalone trigger has no date observer API.

DatePicker tests pair legacy/facade results, state, focus, locale/policy calls,
semantics and draw commands. They cover Italian locale labels, same-date close,
range completion, invalid viewport after mutation, close/focus at callback time,
null handler, multiple Base inputs with one callback, passive/disabled/inert/layout
silence, payload copying after destruction, deferred omission and atomic calendar-ID
admission. These remain native source proofs, not foreign or browser coverage.


## ColorPicker applied RGBA

`cui_base_color_picker_events` and `cui_color_picker_events` mirror the respective
legacy signatures and append `cui_color_handler`. Each returns
`cui_picker_event_result`, preserving the exact legacy bool in `changed` and
counting zero/one actual callbacks. The typed `cui_color_change` contains metadata
and previous/final `cui_color` RGBA bytes. All fields are pointer-free and copyable.

Each whole legacy operation runs once, including final HSV/drag/last_color state,
semantics and (Styled) painting, before notification. Comparison explicitly checks
r/g/b/a fields; it does not compare padding, formatted text or HSV. Several changes
coalesce and net-zero RGBA means no callback. A latent hue change while black or
achromatic need not change RGBA and is not an applied color event. External color
assignment before entry resynchronizes HSV without a component CHANGE.

A palette selection preserves the old alpha when alpha editing is off; the payload
reports that actual final RGBA, not the palette entry's unused alpha. Missing
handler does not inhibit edits. The unused user pointer is ignored. Required
context/state/color pointers, nonzero ID and Styled theme are checked before
legacy code. All other legacy validation remains unchanged.

Delivery requires active painted submission, a proven nonzero frame, enabled and
non-readonly options. Passive maintenance still runs, including HSV synchronization
and readonly/disabled drag cleanup. Metadata has CHANGE and outer picker ID,
UNKNOWN source, HAS_FRAME only, no index/activation/cause. There is no rejection,
BEGIN/COMMIT/CANCEL or owner-controlled color contract added here.

Keep palette/color/state/theme and callback userdata alive through the call. Do
not mutate/free/rebind active resources or recursively submit UI in the observer;
set unrelated flags and defer omission/teardown until return and frame/render
consumption. No payload points into the palette or retains userdata.

Color tests pair Base and Styled legacy/facade contexts for real palette clicks,
channel keys and saturation/value-plane pointer drags, comparing RGBA, HSV/capture,
semantics, results and draw commands. They verify alpha-off preservation, net-zero
updates, latent HSV silence, external RGBA resynchronization, absent-observer
editing, readonly/disabled cleanup, inert/layout-only silence, copied payload after
context destruction and deferred omission. Together with the original Picker and
Base-only suites these are native source proofs; foreign/browser parity is not
implied. The existing time field remains a separate future slice.
