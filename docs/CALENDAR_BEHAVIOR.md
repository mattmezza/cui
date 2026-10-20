# Calendar selection and view behavior

Include `cui/cui_calendar_behavior.h` and rebuild with matching headers/library. The independent value-only record leaves existing picker and presentation structs unchanged. Zero behavior preserves the existing Calendar path; unknown enum values independently fall back to their zero values.

```c
#include "cui/cui_calendar_behavior.h"
cui_calendar_result reservation_calendar(cui_context *ui, const cui_theme *theme,
    cui_calendar_state *state, cui_date_selection *selection, cui_date host_today) {
    return cui_calendar_with_behavior(ui, 100, theme, state, selection,
        (cui_calendar_options){.range = true},
        (cui_calendar_presentation_options){.month_toggle = true,
            .today = host_today, .neighboring_days = true},
        (cui_calendar_behavior_options){
            .range_transition = CUI_CALENDAR_RANGE_RESTART_EARLIER,
            .year_destination = CUI_CALENDAR_YEAR_OPEN_DAYS,
            .recenter = CUI_CALENDAR_RECENTER_SELECTION_START,
            .range_validation = CUI_CALENDAR_VALIDATE_RANGE_ENDPOINTS});
}
```

Initialize caller-owned state with `cui_calendar_init` once and submit inside an active frame; the host supplies its current civil date. The four independent policies are:

- `RANGE_LEGACY` uses `selecting_end` and swaps reversed endpoints. `RANGE_RESTART_EARLIER` infers phase from the live valid endpoint shape on activation: empty/completed ranges start again; a pending earlier date restarts; a pending same/later date completes. It updates `selecting_end` only on acceptance. This includes supplied partial ranges whose flag disagrees with their endpoint shape. Merely changing policies or rendering does not synchronize phase or edit values.
- `YEAR_OPEN_MONTHS` retains year→month→day selection. `YEAR_OPEN_DAYS` preserves the displayed month and returns directly to days after an eligible year activation, without changing selection. It clamps the civil cursor's day for the new year (Feb 29→28), prefers that enabled date, or searches that month's dates. With no enabled date in that month it keeps a valid civil cursor and focuses the year header, even if another displayed month is enabled.
- `RECENTER_LEGACY` keeps existing conditional visible-window behavior. `RECENTER_ACTIVATED_DATE` always puts the accepted date's month first. `RECENTER_SELECTION_START` always puts the resulting start's month first, including incomplete starts and swapped ranges. Explicit modes focus the canonical anchor. If that retained anchor is disabled, they choose the first enabled visible canonical date (at most 93 civil dates), or keep a valid anchor cursor and focus the year header when none exists. A selected endpoint outside the anchored window remains selected but does not become an invisible focus target.
- `VALIDATE_ACTIVATED_DATE` retains the existing live check of the activated date. `VALIDATE_RANGE_ENDPOINTS` also checks both endpoints of a completed candidate against current bounds/predicate before committing endpoints and phase. A newly disabled old start rejects completion atomically. Incomplete starts still validate the activated date. Interior disabled dates are allowed. Single selection ignores both range policies.

Queued targets retain the outer call's original grid/date-and-panel identity; earlier recentering cannot reinterpret a cell index as another date. Transitions use updated selection/phase and current pure policy checks for each activation. Accepted incomplete changes report `activated`; `changed` compares final endpoints, including a net-zero queue. Recenter and year navigation alone do not create selection changes. Explicit recenter reports navigation when its month/cursor/focus destination changes. No synthetic activation, new dispatch architecture or global keyboard listener is added.

`cui_calendar_reduce_with_behavior` and `cui_base_calendar_with_behavior` provide reduction and Base composition; Styled also paints. Split callers reduce with behavior then call `cui_calendar_submit_ex` with the same presentation options. Policy records contain copied values and are not retained. Keep locale strings, disabled predicate/user resources, state and selection alive under their existing submission/frame-consumption contracts; predicates must not mutate active UI or recursively submit. Render/paint callbacks are not action hooks.

Existing typed CHANGE wrappers still call legacy operations once and keep their original callback timing and phase-only/navigation silence. There is no behavior/presentation observer wrapper in this slice: use the returned result. Language builders and DatePicker do not accept these options; DatePicker close-on-completion behavior requires its own future extension. The API supports CUI's bounded Gregorian months, year range and keyboard contract; it does not claim full Calendar styling, locale, event or browser parity. See [presentation options](CALENDAR_PRESENTATION.md) for occurrence IDs and paint/lifetime limits.
