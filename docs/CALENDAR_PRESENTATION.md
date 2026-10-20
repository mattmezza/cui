# Calendar presentation opt-ins

Include `cui/cui_calendar_presentation.h`. The `_ex` Calendar functions accept a separate value-only record; zero preserves the legacy draw, semantics and reduction paths. Rebuild with matching headers and library when adopting these symbols. The unpublished presentation record now includes `neighboring_days`; mixing earlier record layouts with the new library is unsupported. No binding support is implied.

```c
#include "cui/cui_calendar_presentation.h"
typedef struct {
    cui_calendar_state calendar;
    cui_date_selection selection;
} appointment_calendar;
void appointment_calendar_init(appointment_calendar *app, cui_date initial) {
    app->selection = (cui_date_selection){.start = initial};
    cui_calendar_init(&app->calendar, initial);
}
cui_calendar_result appointment_calendar_submit(cui_context *ui, const cui_theme *theme,
                                                appointment_calendar *app,
                                                cui_date host_today) {
    return cui_calendar_ex(ui, 100, theme, &app->calendar, &app->selection,
        (cui_calendar_options){.locale = cui_date_locale_builtin(CUI_DATE_LOCALE_IT)},
        (cui_calendar_presentation_options){.month_toggle = true, .today = host_today,
                                                .neighboring_days = true});
}
```

Call initialization once and submission inside each active frame. The host supplies its local civil date; the core reads no clock or timezone. Invalid/zero `today` omits decoration. Today uses theme accent and accent foreground, selected cells use the existing primary paint, and disabled cells retain muted foreground. Today never changes selection or date eligibility. Configure fonts for the labels and consume draw/semantic data before the next frame.

With `month_toggle=true` and one displayed month (`months` zero or one), separate localized month and year headers open their respective grids. Activating an open header closes it; Escape returns to days and focuses that grid's header. Selecting a month returns to days and focuses its first enabled date. Year selection retains the legacy transition through the month grid. Header semantics expose selected/expanded state. The month header ID is `cui_calendar_month_toggle_id(root)`; the year header retains navigation part 5. Multimonth calendars ignore the month-toggle option. Existing arrows, Home/End, PageUp/Down, Enter/Space and policy checks remain in the same reducer; this adds no global key listener.

`cui_calendar_reduce_ex` and `cui_calendar_submit_ex` retain the split reducer/submission contract; Base performs both and Styled also paints. Locale strings and disabled predicate/user resources are borrowed and must remain valid through use and frame consumption. The presentation record contains no retained pointer. Predicates must not mutate or recursively submit active UI. Omission of the widget is allowed after its outer call and frame consumption; persistent application state remains caller-owned.

Navigation reports `navigated`, never `changed` or `activated` by itself. Existing typed CHANGE wrappers are for legacy Calendar options; there is no extended observer wrapper or event-facade bridge in this slice. Use the returned aggregate result for this API. Existing callback ordering is unchanged.

`neighboring_days=true` paints and submits valid outside-month cells as selectable dates. Muted unselected neighbors retain ordinary eligibility checks; selected/range, disabled and today paint use the precedence above. Neighbor occurrences have date-and-panel IDs; in-month cells retain `cui_calendar_day_id`. Only the canonical cursor is a sequential day tab stop. A neighbor can still receive pointer/programmatic/AT focus and Enter/Space activation. Selection focuses the canonical occurrence, retaining the displayed window when that date is in another visible panel or rebasing to its month otherwise. Existing CUI range endpoint swapping is preserved; frozen GPUI instead restarts an earlier second endpoint and recenters on range start. This slice does not change that established CUI selection model.

`cui_calendar_cell_resolve(root, first_month, first_weekday, panel, cell, &info)` returns a copied date, ID and neighboring flag for Base layout, automation or host inspection. A valid first month is normalized to day 1; weekday is modulo seven. Panel must be 0..2 and cell 0..41. It does not test whether the panel was submitted or the date is enabled. Invalid inputs, unrepresentable year-edge cells and zero-derived occurrence/canonical IDs return false and zero the output; NULL output returns false. Keep ordinary root-scoped ID collision discipline: no guarantee is made across unrelated caller roots. Repeated visible dates intentionally have distinct neighbor IDs. Queued input resolves against the outer call's original displayed grid and applies current policy per activation; preceding navigation cannot reinterpret the same ID as a different civil date.

This implements independent month access, host-supplied today paint and opt-in neighboring cells under CUI's one-to-three-month/six-row bounds. Arbitrary item styling and unrestricted month counts remain outside this API. Seven builtin locale tables and custom tables already exist; this is not an OS locale engine, arbitrary date-pattern parser, non-Gregorian calendar or RTL layout implementation. Frozen GPUI source does not define a dedicated Calendar arrow-key contract; CUI's existing traversal contract is preserved.

DatePicker retains its legacy composition and does not accept this presentation record. The cell helper and presentation calls do not allocate or retain per-cell pointers; frame data and borrowed locale/policy lifetimes remain as described above.

The legacy range, year-view and recenter defaults above can be selected independently through [Calendar behavior policies](CALENDAR_BEHAVIOR.md), including optional completed-endpoint validation. These are separate value-only options and APIs; they do not resize the presentation record or change legacy calls.
