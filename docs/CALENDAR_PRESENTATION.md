# Calendar presentation opt-ins

Include `cui/cui_calendar_presentation.h`. The `_ex` Calendar functions accept a separate value-only record; zero preserves the legacy draw, semantics and reduction paths. Rebuild the library when adopting the new symbols. No binding support is implied.

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
        (cui_calendar_presentation_options){.month_toggle = true, .today = host_today});
}
```

Call initialization once and submission inside each active frame. The host supplies its local civil date; the core reads no clock or timezone. Invalid/zero `today` omits decoration. Today uses theme accent and accent foreground, selected cells use the existing primary paint, and disabled cells retain muted foreground. Today never changes selection or date eligibility. Configure fonts for the labels and consume draw/semantic data before the next frame.

With `month_toggle=true` and one displayed month (`months` zero or one), separate localized month and year headers open their respective grids. Activating an open header closes it; Escape returns to days and focuses that grid's header. Selecting a month returns to days and focuses its first enabled date. Year selection retains the legacy transition through the month grid. Header semantics expose selected/expanded state. The month header ID is `cui_calendar_month_toggle_id(root)`; the year header retains navigation part 5. Multimonth calendars ignore the month-toggle option. Existing arrows, Home/End, PageUp/Down, Enter/Space and policy checks remain in the same reducer; this adds no global key listener.

`cui_calendar_reduce_ex` and `cui_calendar_submit_ex` retain the split reducer/submission contract; Base performs both and Styled also paints. Locale strings and disabled predicate/user resources are borrowed and must remain valid through use and frame consumption. The presentation record contains no retained pointer. Predicates must not mutate or recursively submit active UI. Omission of the widget is allowed after its outer call and frame consumption; persistent application state remains caller-owned.

Navigation reports `navigated`, never `changed` or `activated` by itself. Existing typed CHANGE wrappers are for legacy Calendar options; there is no extended observer wrapper or event-facade bridge in this slice. Use the returned aggregate result for this API. Existing callback ordering is unchanged.

This implements independent month access and host-supplied today paint. Neighboring-day rendering, arbitrary item styling and unrestricted month counts remain outside this API. Seven builtin locale tables and custom tables already exist; this is not an OS locale engine, arbitrary date-pattern parser, non-Gregorian calendar or RTL layout implementation. Frozen GPUI source does not define a dedicated Calendar arrow-key contract; CUI's existing traversal contract is preserved.
