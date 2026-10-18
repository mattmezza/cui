# Calendar and DatePicker selection observers

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

This slice does not add ColorPicker, TimeField, navigation, popup lifecycle or
controlled-request observers. A CHANGE reports an already-applied
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
