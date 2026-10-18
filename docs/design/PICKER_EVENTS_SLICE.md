# Picker applied observer slices

Design-only audit at `ffb39cd7`. No proposed API below exists. Preserve all 78
manifest entries and the existing APIs, options, mutation/paint order and results.
These are independently reviewable applied notifications, not controlled requests.

## Actual state, callbacks and timing

| Family | Source contract | Observer implication |
| --- | --- | --- |
| Calendar | [`cui_picker.h`](../../include/cui/cui_picker.h) separates `cui_date_selection {start,end}` from state month/cursor/selecting_end and DAYS/YEARS/MONTHS chooser state. [`cui_calendar_reduce`](../../src/base/cui_picker.c) mutates these, returns changed/activated/navigated; `submit` registers semantics separately. Base reduces then submits; Styled Base-calls then paints. | Selected dates are the editable value. Display month, focus cursor, chooser page and range-in-progress are distinct presentation/lifecycle state. `navigated` does not mean selected date changed. |
| DatePicker | Base trigger updates toggle/dismissal; Base composite reduces only while open, closes on activated single date or completed range, then submits trigger semantics. Styled calls Base, paints trigger, places popup and submits/paints Calendar without reducing again. | Outer ownership must avoid observing nested reduction twice. Opening/closing popup is not date CHANGE. Same-date activation may close without changing value. |
| ColorPicker | Caller owns RGBA pointer and HSV/capture state. Base synchronizes HSV when external RGBA differs, registers parts, reduces palette/pointer/key input, updates part semantics/drag bits/last_color and returns final RGBA inequality. Styled calls Base once then paints. | Observe actual RGBA bytes, not internal HSV or drag bits. A hue change on achromatic/black color may have no RGBA CHANGE. No existing request/commit lifecycle exists. |

Calendar's only value-related function pointer is `is_disabled(user,date)`, an
enablement policy, not an on-change or acceptance hook. It can be called repeatedly
while scanning days/year/month availability. Do not turn it into a notification or
assume one call per user action. ColorPicker has no value observer callback;
palette is borrowed immutable data, not an event handler table.

The Calendar reducer repairs invalid presentation state (initial month, cursor,
view/page), normalizes month.day and adjusts cursor availability even without
selection input. It snapshots date selection after initialization and compares
start/end using `cui_date_compare`. Range first activation sets start/clears end
and selecting_end; second activation sorts the endpoints and completes the range.
Keyboard navigation can move cursor/month without selecting. Year/month chooser
navigation changes presentation only. `activated` and `navigated` are aggregates,
not event counts or copied evidence.

DatePicker closes before adding its modal scope, deliberately preserving input
routing. Its Styled path can return false on an invalid viewport after Base has
already reduced/mutated selection. Never gate notification solely on its bool.
Preserve that bool verbatim rather than repairing legacy behavior in a facade.
ColorPicker preserves alpha on palette selection when alpha editing is off;
readonly/disabled clear drag state without applying edits. External RGBA assignment
before entry resynchronizes HSV but is not a component-applied change.

## Recommended independently reviewable slices

Use a new `cui_picker_events.h` including `cui_picker.h` and shared metadata. Dates
and colors remain explicit value types, never float packing, strings or void values:

```c
typedef struct {
    cui_component_event_metadata event;
    cui_date_selection previous, value;
} cui_date_selection_change;
typedef void (*cui_date_selection_change_fn)(void *, const cui_date_selection_change *);
typedef struct { cui_date_selection_change_fn on_change; void *user; }
    cui_date_selection_handler;
typedef struct {
    cui_calendar_result control;
    size_t change_count;
} cui_calendar_event_result;
typedef struct { bool changed; size_t change_count; } cui_picker_event_result;
typedef struct {
    cui_component_event_metadata event;
    cui_color previous, value; /* RGBA bytes, including preserved alpha. */
} cui_color_change;
typedef void (*cui_color_change_fn)(void *, const cui_color_change *);
typedef struct { cui_color_change_fn on_change; void *user; } cui_color_handler;
```

1. **Calendar date selection only.** Add `cui_calendar_reduce_events`,
   `cui_base_calendar_events`, `cui_calendar_events`, mirroring existing signatures
   plus date handler and returning `cui_calendar_event_result`. Reduce-only is
   necessary for custom popup hosts that intentionally submit later. Each invokes
   its corresponding legacy operation once and compares committed start/end;
   whole Base includes semantic submission, whole Styled includes paint. No nested
   observer, no reducer replay. Notify at the outer return, preserving the chosen
   entrypoint's existing timing and result. Do not expose a submit-only observer.
2. **DatePicker selected dates.** Add `cui_base_date_picker_events` returning
   `cui_calendar_event_result` and `cui_date_picker_events` returning
   `cui_picker_event_result`, mirroring old arguments plus date handler. Wrap each
   whole operation exactly once, with a pre/post date snapshot; no observed nested
   Calendar facade. Styled notifications occur after popup/focus/paint work,
   including early returns after applied mutation. Event target is trigger ID;
   the Base's separate calendar ID remains a rendering/reduction identity.
   The standalone trigger has no selected-date observer.
3. **ColorPicker RGBA only.** Add `cui_base_color_picker_events` and
   `cui_color_picker_events`, mirroring old signatures plus color handler and
   returning `cui_picker_event_result`. Whole-call wrappers compare RGBA fields,
   preserve bool, and notify after all original state/semantic/paint work. Do not
   observe HSV synchronization, latent hue, drag start/end or formatting calls.

Every initial facade has **zero or one actual callback** per outer submission.
Missing handler still mutates normally and yields zero count; unused user is
ignored. Several updates coalesce; final equality means no notification. Payload
contains previous/final values, CHANGE kind and compound target, UNKNOWN source,
actual nonzero active frame with HAS_FRAME only. No event index, activation,
physical device, reason or timestamp can be inferred from aggregate flags.
Observers are not rejectable and cannot retroactively veto writes or dismissal.

Before production, settle minimal new-entry admission checks (nonnull required
pointers and nonzero target) without changing old functions' behavior. Preserve
legacy passive maintenance, but emit only during active painted non-disabled
submission; Color readonly additionally suppresses observations. Calendar has no
readonly option. Core inert/modal targeting remains authoritative, not a new
picker suppression policy. Test swallowed/consumed event behavior as it exists:
these reducers do not uniformly check `cui_event_consumed` in every loop, so do not
promise new propagation or consumption guarantees as part of a notification API.

## Explicit exclusions and later alternatives

No generic NAVIGATE or popup lifecycle event is in the initial slice. If requested,
Calendar presentation notifications need a separate typed snapshot covering month,
cursor, chooser view/page/year/month cursor and selecting_end, with specified
normalization versus input causes. A month-only payload cannot describe chooser
navigation. Do not call it selected-date CHANGE, and do not use result.navigated
as a count. Popup open/close outcomes need reasons and owner/focus timing beyond
sampling overlay.open. Existing Surface contracts are not automatically inherited.

Color drag COMMIT/CANCEL or owner-controlled color requests would require a new
reviewed reducer seam/ownership contract; an aggregate bool cannot implement them.
A per-event selected-date/color alternative is possible only with genuine loop
hooks covering all branches and typed before/after values. It would have different
multiplicity and must not silently replace these aggregate APIs. TimeField is a
separate family/slice, despite sharing the picker source files.

## Lifetimes, tests and foreign boundary

Values/metadata are pointer-free and copyable. Keep model/state/options, locale
strings, disabled-policy users, palette, theme and context alive through submission;
overlay state and resources retain their existing longer lifetimes. Observers
must not mutate/free/rebind these resources or recursively submit UI. Unrelated
application flags are allowed; defer omission/teardown until outer return and
frame/renderer consumption. Do not dispatch again from result inspection.
The facade does not change the existing disabled policy's callback requirements.

Existing evidence is [`test_picker.c`](../../tests/unit/test_picker.c): Calendar
model/year-grid/events/range/weekend policy/locale/presentation cases, DatePicker
popup/focus/escape/outside/disabled cases, and ColorPicker random model roundtrips,
channels, palette, alpha, plane drag and readonly/disabled cases. Some tests compile
with `CUI_PICKER_BASE_ONLY`; retain both Base and Styled variants where registered.

New dedicated tests should pair legacy/facade contexts and compare date/RGBA,
state, focus, results, semantics and draw commands. Cover several changes/net-zero
within a frame, same-date activation, reversed range completion, month/year chooser
and keyboard cursor motion without selection, invalid presentation initialization
without notification, disabled-policy scanning and bounded dates, popup-close
focus before observer, invalid viewport after mutation, alpha-off palette rules,
latent HSV changes without RGBA change, pointer/key edits, readonly/disabled drag
cleanup, inert/layout-only silence with preserved maintenance, absent handler,
payload copies after later frames, callback/result coexistence and deferred omission.
Use real queued input rather than invoking notification helpers alone. Private
native display/headless browser rules apply when implementation is authorized.

No builds were run for this audit. Python/JS/TS/Go/Rust queued V1 notifications do
not automatically expose new C picker observers; a separate typed schema/wrapper
slice needs full date fields/RGBA tags, stable IDs, owning bounds, handler retention
and exception containment. Existing notification queues cannot reject applied
changes. See [the binding audit](COMPONENT_EVENTS_BINDINGS.md). No new source/test
or feature-ledger status is promoted by this checkpoint.

## First slice implementation status

Calendar selected-date observers only are implemented by `cui_picker_events.h`
and the new Base/Styled wrapper sources. See [Calendar observer guide](../PICKER_EVENTS.md).
Each whole legacy operation is called once; date comparison, callback count,
passive silence and outer completion timing follow the first slice above.
DatePicker and ColorPicker remain proposals awaiting separate review. No TimeField,
navigation/popup lifecycle, foreign binding or parity-ledger promotion is included.


## Second slice implementation status

Base and Styled DatePicker selected-date wrappers are implemented in the same
owned event header/sources. They observe the complete outer legacy operation once,
including late false Styled returns after mutation, with trigger-ID metadata and
post-dismiss/focus timing. The guide records the distinct legacy result types.
ColorPicker remains awaiting review; no popup lifecycle or foreign API is added.


## Third slice implementation status

Base and Styled ColorPicker RGBA observers are implemented in the same event
header/wrapper sources. Whole-operation comparison uses explicit RGBA fields,
retains alpha policy and HSV/capture maintenance, and never treats latent HSV or
external synchronization as applied CHANGE. See the implementation guide.
TimeField, navigation/lifecycle, controlled requests and foreign wrappers remain
excluded. Completion of these three bounded slices does not promote family parity.
