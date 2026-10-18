/* SPDX-License-Identifier: MIT */
#include "cui/cui_picker_events.h"
cui_calendar_event_result cui_calendar_events(
    cui_context *ui, cui_id id, const cui_theme *theme, cui_calendar_state *state,
    cui_date_selection *selection, cui_calendar_options options, cui_date_selection_handler handler) {
    cui_calendar_event_result result = {0};
    if (!ui || !id || !theme || !state || !selection)
        return result;
    cui_date_selection before = *selection;
    uint64_t frame = !options.disabled && cui_frame_paint_enabled(ui) ? cui_frame_sequence(ui) : 0;
    result.control = cui_calendar(ui, id, theme, state, selection, options);
    if (frame && handler.on_change &&
        (cui_date_compare(before.start, selection->start) ||
         cui_date_compare(before.end, selection->end))) {
        cui_date_selection_change event = {
            .event = {.kind = CUI_COMPONENT_EVENT_CHANGE, .target = id,
                      .source = CUI_COMPONENT_EVENT_SOURCE_UNKNOWN,
                      .flags = CUI_COMPONENT_EVENT_HAS_FRAME, .frame_sequence = frame},
            .previous = before, .value = *selection};
        handler.on_change(handler.user, &event);
        result.change_count = 1;
    }
    return result;
}
