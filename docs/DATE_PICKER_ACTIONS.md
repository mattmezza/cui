# Date-only DatePicker actions

Include `cui/cui_date_picker_actions.h` and rebuild with matching sources and
headers. This opt-in facade adds bounded presets and clear actions. Existing
DatePicker APIs and layouts are unchanged. It is not date-time editing, an event
observer, arbitrary styling or a complete DatePicker parity claim. The canonical library and generated distributions include both implementation
files. The focused tests compile and execute the example below.

This Base composition is compiled by the private fixture. Call it once inside a
frame; state and value persist across frames. Use a real host viewport rather
than a clock or platform date service in the widget.

```c
#include "cui/cui_date_picker_actions.h"
#include <stdio.h>

/* Small custom numeric Calendar control grid. It deliberately has no weekday
 * columns: each date/month/year is an explicitly labeled button. */
static void example_calendar_paint(cui_context *ui, cui_id root, size_t first) {
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    unsigned column = 0;
    cui_begin_box(ui, 0, (cui_box){.row = true});
    for (size_t i = first; i < count; i++) {
        if (sem[i].role != CUI_ROLE_BUTTON)
            continue;
        if (column == 7) {
            cui_end_box(ui);
            cui_begin_box(ui, 0, (cui_box){.row = true});
            column = 0;
        }
        char number[16];
        cui_str label = sem[i].name;
        if (sem[i].value > 0) {
            snprintf(number, sizeof number, "%.0f", (double)sem[i].value);
            label = cui_string(number);
        }
        const char *navigation[] = {"<<", "<", ">", ">>", "Year"};
        for (unsigned part = 1; part <= 5; part++)
            if (sem[i].id == cui_calendar_navigation_id(root, part))
                label = cui_string(navigation[part - 1]);
        if (sem[i].id == cui_calendar_month_toggle_id(root))
            label = cui_string("Month");
        cui_begin_box(ui, sem[i].id, (cui_box){.width = 40, .height = 32,
            .clip = true, .border_width = cui_focused(ui) == sem[i].id ? 2 : 0,
            .border = {255, 255, 255, 255},
            .background = sem[i].checked ? (cui_color){30, 100, 180, 255}
                                       : (cui_color){30, 30, 30, 255}});
        cui_text(ui, label, 12, sem[i].disabled ? (cui_color){100, 100, 100, 255}
                                             : (cui_color){255, 255, 255, 255});
        cui_end_box(ui);
        column++;
    }
    cui_end_box(ui);
}

void example_date_actions(cui_context *ui, cui_date_picker_actions_state *state,
                          cui_date_picker_value *value, cui_rect viewport) {
    /* Initialize once before the first frame:
     * cui_date_picker_actions_init(state, 702, (cui_date){2024, 2, 1});
     * *value = (cui_date_picker_value){.dates = {.start = {2024, 2, 15}}}; */
    const cui_date_picker_preset preset = {
        .id = 704, .label = {"March", 5},
        .value = {.dates = {.start = {2024, 3, 1}}}};
    cui_date_picker_actions_options options = {
        .picker = {.viewport = viewport},
        .calendar = {.presentation = {.month_toggle = true}},
        .presets = &preset, .preset_count = 1,
        .clear_id = 703, .clear_label = {"Clear", 5}, .cleanable = true,
        .delete_policy = CUI_DATE_PICKER_DELETE_TRIGGER};
    cui_date_picker_actions_result result = cui_base_date_picker_actions(
        ui, 701, 700, cui_string("Date"), state, value, options);
    if (!result.valid)
        return;
    cui_begin_box(ui, 701, (cui_box){.width = 180, .height = 32});
    cui_text(ui, cui_string("Choose date"), 14, (cui_color){255, 255, 255, 255});
    cui_end_box(ui);
    if (options.cleanable && cui_date_valid(value->dates.start)) {
        cui_begin_box(ui, 703, (cui_box){.width = 80, .height = 32});
        cui_text(ui, options.clear_label, 14, (cui_color){255, 255, 255, 255});
        cui_end_box(ui);
    }
    if (state->picker.overlay.open) {
        cui_rect panel = {viewport.x, viewport.y + 40, 300, 360};
        cui_overlay_set_bounds(&state->picker.overlay, panel);
        if (cui_overlay_begin(ui, &state->picker.overlay)) {
            cui_begin_portal(ui, 702, (cui_box){.clip = true}, panel, 100);
            cui_calendar_options calendar = options.picker.calendar;
            calendar.range = state->calendar_value.shape == CUI_DATE_PICKER_RANGE;
            calendar.disabled = calendar.disabled || options.picker.disabled;
            /* Capture the semantic range, submit once, then custom-paint its
             * numeric buttons without reducing input again. */
            size_t calendar_first;
            cui_semantics(ui, &calendar_first);
            cui_calendar_submit_ex(ui, 700, &state->picker.calendar,
                state->calendar_value.dates, calendar, options.calendar.presentation);
            example_calendar_paint(ui, 700, calendar_first);
            cui_date_picker_actions_submit_presets(ui, state, options);
            cui_begin_box(ui, 704, (cui_box){.width = 180, .height = 32});
            cui_text(ui, preset.label, 14, (cui_color){255, 255, 255, 255});
            cui_end_box(ui);
            cui_end_box(ui);
            cui_overlay_end(ui);
        }
    }
}
```

`cui_calendar_submit_ex` submits semantics, not layout/paint. The example paints
a simple numeric button grid including navigation and selectors; it is not a
weekday-aligned Calendar design. Use the Styled facade for the standard Calendar
layout or implement a custom Base layout for all submitted IDs.
The preset helper submits semantics only, inside the caller's one popup scope;
it does not begin/end a scope, paint, reduce input or invoke predicates/callbacks.
Use the same valid options as the successful Base call and paint each submitted
preset ID within the visible panel. Do not call it twice. It publishes copied
submission identities only after every interaction submission succeeds. Its state
argument is mutable solely for that bounded metadata. No borrowed pointer is
retained. A helper failure cannot turn a partial list into next-frame eligibility.

The new state deliberately keeps the outer picker value and child Calendar value
separate. Initialize once with `cui_date_picker_actions_init`; NULL is a no-op.
The first successful call synchronizes the supplied outer value once. Later
external value changes are detected by comparing dates/shape with `observed_value`.
A rejected child update is not retried on ordinary render, reopening or a matcher
change alone. Do not edit internal observed/submission fields or manually toggle
the overlay; use ordinary trigger/dismiss input. Calendar view/cursor remain
caller-owned, like the existing picker. Reinitialization invalidates all identities.

SOURCE presets accept the outer value first, including shape changes, then attempt
child synchronization. Child eligibility uses CUI bounds and the pure disabled
predicate; Calendar-only `disabled` does not reject assigned values. Rejection
leaves child selection/month/view/range phase unchanged. Acceptance sets the child
selection and range phase, and recenters month/year to the start without resetting
view/year page/cursors. These are the frozen source's independent ownership and
accepted-start recenter semantics, with explicit CUI min/max extension. Presets
close and unconditionally request trigger focus. An accepted complete child day activation publishes the child's value/shape back
to the outer value even if the child date did not change: upstream Calendar emits
Selected for this case. Actual child edits also publish incomplete ranges under
CUI's existing partial-range semantics; this is not a claim of upstream's
completion-only subscription delivery. Navigation and rejected activation do not
overwrite a diverged outer value.

MATCH_SHAPE rejects mismatched preset shapes but otherwise keeps SOURCE child
semantics. MATCH_SHAPE_ENABLED additionally atomically rejects a preset if either
endpoint is disabled/out of bounds. No interior range scan occurs. Relative dates
are resolved by the host. `picker.calendar.range` is ignored by this facade;
the outer value and independent child each carry explicit shape.

Clear empties both values, keeps outer shape, resets the child's range phase,
closes without an explicit focus request, and preserves month/year/view/cursors.
Empty candidates never call the date predicate. `cleanable` controls visibility;
Delete is separately OFF (default), TRIGGER or SCOPE. Only unmodified nonrepeating
Delete is handled; Backspace is not. SCOPE covers prior submitted enabled
Calendar navigation/month/year headers, grids/day cells and preset buttons.
Outer disabled gates every action; Calendar-only disabled gates day input.
Preset buttons match the source's disabled Tab-stop policy; pointer, accessibility
and explicit focus plus Enter activation remain available. The existing modal
outside-dismiss policy still applies to trigger-row controls outside an open
popup: dismiss first, then activate clear, or use scope Delete. This slice does
not change Core hit routing to promise one-click outside-popup clear.
There is no future text-entry child in this date-only slice.

Preflight rejects all invalid options before mutation, event consumption or
predicates. Limit8 presets and96 UTF-8 bytes per nonempty label; no embedded NUL,
invalid scalar, zero/duplicate/conflicting IDs or invalid dates. Single end must
be absent; range can be empty/start-only/ordered complete, never end-only.
Presets are complete. Active clear support needs a distinct nonzero ID and label.
Viewport must be finite/positive; width and cell size finite/nonnegative, with
zero retaining the existing defaults and small positive cells using normalization.
Bounds must be absent or valid and ordered. Unknown action enums fail; inherited
Calendar extension normalization remains its existing contract. Styled also
requires finite positive font size/nonnegative padding/gap/radius and enough
viewport width/height for all preset rows; it does not submit clipped-off rows.
Ordinary global caller ID collision discipline remains required, including custom
Base layouts. Known Calendar ID conflicts are rejected before predicate calls.

The array and preset values are borrowed for the call; label/locale strings and
predicate/user resources remain valid through normal frame/render consumption.
Do not mutate UI from predicates. State/value belong to the owner UI thread. Reinitialize before using the state
with a different context; frame sequences are local to one context.
No resources are allocated or retained by this API. It is not a stable binary
layout extension to old records; rebuild new state users with matching headers.

Queue order is the existing selector Escape prepass, overlay dismissal/trigger
pass, one entire Calendar reduction, then one action pass in frame event order.
Only the first eligible accepted action wins. Rejected presets do not consume.
Calendar and action phases are not global chronological interleaving: a queued
preset can run after a later queued day edit. Calendar auto-close in that one
reduction does not erase original submitted action eligibility; prior overlay
dismissal does erase it; trigger toggles and Escape
invalidate popup action eligibility for the call. Prior records expire after an
omitted frame, omitted/failed popup submission, or a close/reopen transition.
They contain only copied IDs/grid/week-start/month-count/frame sequence and at
most 144 target IDs for submitted enabled Calendar/preset targets,
not a host/context pointer. Scope Delete additionally checks current view/grid
and list membership; a reserved but absent/off-view year/day ID cannot qualify.
The 144 bound includes at most 126 day occurrences, navigation
and the 8 presets; the helper publishes no partial target set. A real previous
semantic/input target is still required;
retained IDs alone cannot fabricate an activation. Current preset ID and policy
must also match. Same-frame root reuse and global ID collisions remain invalid.

`valid` denotes preflight success; later Core capacity/layout errors are not
transaction rollback. `calendar` is the single Calendar reduction result;
`action_activated` is acceptance independently of a value edit; `changed` compares
final outer dates/shape with the entry value. Clear on empty and identical presets
can be accepted without `changed`. No typed observer wrapper or binding builder
is added here. No live AT, browser or platform date-time behavior is implied.

Scope Delete additionally requires current view/list eligibility. A submitted year
then hidden by a day-view switch, or a day hidden by a month change, is rejected.
Unsubmitted year/day IDs are not retained. Core may redirect programmatic focus
on an unknown ID to a valid modal target before dispatch; Delete then applies to
that actual target, not to the unknown requested ID. The popup root itself is not
a Scope Delete target.


## Verified worker fixture

The isolated fixture registers the new sources without changing canonical build
metadata. Strict C11 (`-Wall -Wextra -Wpedantic -Werror`) and the C++ header compile
pass. The guide code block is extracted, compiled and exercised by the test.

```sh
cmake -S tests/fixtures/date_picker_actions -B build-actions -G Ninja
cmake --build build-actions --target test_date_picker_actions date_picker_actions_header date_picker_actions_guide test_picker test_picker_base test_picker_events test_date_picker_calendar test_calendar_behavior test_calendar_presentation
python scripts/test_local.py --build-dir build-actions -- -V -R '^(date_picker_actions|picker|picker_base_only|picker_events|date_picker_calendar|calendar_behavior|calendar_presentation)$'
```

Results: native 7/7; separate `-DCUI_SANITIZE=ON` build, ASan/UBSan 7/7.
The action suite reports 1,738 assertions for Base and Styled, including source
child-rejection/retention, complete no-op adoption, clear/preset focus, queue order,
preflight failures, previous-submission expiry, off-view target rejection,
reduce-once and legacy zero-option semantic/software-pixel equivalence.

The private `tests/fixtures/date_picker_actions/test_wasi.py --build-dir BUILD`
uses the existing repository pinned WASI driver: Zig 0.16.0, wasmtime 49.0.0,
WASI Preview 1. It passed 1/1 suite and the same 1,738 assertions, with source
fingerprint validation. No dependencies were installed. Native checks used the
private-display wrapper; no browser, desktop, live AT or bindings were exercised.
Canonical source/umbrella/amalgamation/binding/metadata registration, generated
skill resources and full integration checks remain the integrator's work.
