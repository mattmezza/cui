# DatePicker with Calendar options

Include `cui/cui_date_picker_calendar.h` and rebuild with matching headers/library. This additive value record forwards Calendar presentation and behavior through DatePicker; zero preserves the legacy DatePicker path without resizing existing public records.

```c
#include "cui/cui_date_picker_calendar.h"
bool reservation_date(cui_context *ui, const cui_theme *theme,
    cui_date_picker_state *state, cui_date_selection *selection,
    cui_rect viewport, cui_date host_today) {
    return cui_date_picker_ex(ui, 100, theme, cui_string("Reservation date"), state, selection,
        (cui_date_picker_options){.viewport = viewport,
            .calendar = {.range = true, .locale = cui_date_locale_builtin(CUI_DATE_LOCALE_IT)}},
        (cui_date_picker_calendar_options){
            .presentation = {.month_toggle = true, .today = host_today, .neighboring_days = true},
            .behavior = {.range_transition = CUI_CALENDAR_RANGE_RESTART_EARLIER,
                .year_destination = CUI_CALENDAR_YEAR_OPEN_DAYS,
                .recenter = CUI_CALENDAR_RECENTER_SELECTION_START,
                .range_validation = CUI_CALENDAR_VALIDATE_RANGE_ENDPOINTS}});
}
```

Call `cui_date_picker_init(state, popup_id, initial_date)` once, then submit each active frame. The caller retains state and selection. Supply a finite positive viewport and a distinct popup ID. Locale/custom labels, first weekday, 1–3 months, cell size, range, bounds and disabled-date predicate already belong to `options.calendar`; they are not duplicated in the new record. Today is a host-supplied civil date, with no clock lookup; invalid/zero omits its paint. Month toggle is effective only for one month. See [Calendar presentation](CALENDAR_PRESENTATION.md) and [behavior policies](CALENDAR_BEHAVIOR.md) for exact normalization, occurrence IDs, eligibility and paint precedence.

DatePicker reduces Calendar exactly once, then closes after the entire queued Calendar input when an accepted single selection or completed range remains. An earlier endpoint restart remains open; a rejected completion remains open. A same-date single activation closes even when the bool is false because the final selection did not change. Queued targets use the original grid and panel identity even when an earlier activation recenters. Completion followed by another start in the same queue remains open. Opening, rendering or supplying policies does not reset Calendar state or synchronize range phase.

The existing pass order remains Escape-from-month/year handling, overlay dismissal/trigger toggles, Calendar reduction, close, trigger and popup submission/paint. These are separate passes, not a globally chronological dispatcher. Two Escapes queued in a selector frame are handled by the existing selector prepass; they are not equivalent to Escape in two successive frames. Closing restores trigger focus; a newly opened Styled popup focuses the existing Calendar cursor or selector. Outer disabled closes the popup; Calendar-only disabled leaves the trigger operable while suppressing Calendar actions. Omitted components do not reduce input or manufacture close events.

The Styled bool is the exact legacy result. In particular, an invalid viewport or late overlay-begin failure can return false after a selection edit; inspect caller state/selection when handling such errors. `cui_base_date_picker_ex` returns the full aggregate Calendar result but owns neither layout nor popup submission. A Base host calls it once, then positions/begins its overlay and calls `cui_calendar_submit_ex` with the same presentation record; do not reduce a second time. `cui_date_picker_calendar_id(trigger)` exposes Styled's unchanged Calendar ID for `cui_calendar_cell_resolve`; zero trigger or zero derived ID returns zero, and ordinary ID collision discipline still applies.

Extension values are copied and not retained. Keep state/selection, locale strings, predicate and user resources alive under existing submission/frame/render-consumption lifetimes. Disabled predicates must be pure and must not mutate active UI or recursively submit. The existing typed CHANGE wrappers retain their legacy path, final-value comparison and callback timing; they do not accept these options. There is no new selection observer, open/close notification, cancellation cause, event-source attribution or binding-builder support in this slice. Full DatePicker time editing, presets, cleanable controls, arbitrary styling and browser extension parity remain separate work.
