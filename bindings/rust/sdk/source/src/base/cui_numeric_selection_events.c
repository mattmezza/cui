/* SPDX-License-Identifier: MIT */
#include "cui_pagination_internal.h"
#include <math.h>
cui_numeric_selection_event_result
cui_base_pagination_events(cui_context *ui, cui_id id, size_t *page, size_t total, bool compact,
                           bool disabled, cui_pagination_event_handler handler) {
    return cui_base_pagination_events_ex(
        ui, id, page, total, (cui_pagination_options){.compact = compact, .disabled = disabled},
        handler);
}
cui_numeric_selection_event_result
cui_base_pagination_events_ex(cui_context *ui, cui_id id, size_t *page, size_t total,
                              cui_pagination_options options,
                              cui_pagination_event_handler handler) {
    return cui_base_pagination_observe_internal(ui, id, page, total, options,
                                                (cui_pagination_jump){0}, handler);
}
cui_numeric_selection_event_result
cui_base_pagination_observe_internal(cui_context *ui, cui_id id, size_t *page, size_t total,
                                     cui_pagination_options options, cui_pagination_jump jump,
                                     cui_pagination_event_handler handler) {
    uint64_t frame = cui_frame_sequence(ui);
    bool observe = handler.change && frame && id && page && !options.disabled;
    size_t before = observe ? *page : 0;
    cui_numeric_selection_event_result result = {0};
    result.control = cui_base_pagination_reduce_internal(ui, id, page, total, options, jump);
    if (observe && cui_frame_sequence(ui) == frame && before != *page) {
        cui_pagination_change_event event = {0};
        (void)cui_component_event_init(&event.event, CUI_COMPONENT_EVENT_CHANGE, id,
                                       CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
        event.event.flags = CUI_COMPONENT_EVENT_HAS_FRAME;
        event.event.frame_sequence = frame;
        event.before = before;
        event.after = *page;
        event.total = total;
        result.change_count = 1;
        handler.change(handler.user, &event);
    }
    return result;
}
cui_numeric_selection_event_result cui_base_stepper_events(cui_context *ui, cui_id id,
                                                           const cui_step *steps, size_t count,
                                                           size_t *current, bool disabled,
                                                           cui_stepper_event_handler handler) {
    uint64_t frame = cui_frame_sequence(ui);
    bool observe =
        handler.change && frame && id && current && !disabled && (steps || !count) && count <= 100;
    size_t before = observe ? *current : 0;
    cui_numeric_selection_event_result result = {0};
    result.control = cui_base_stepper(ui, id, steps, count, current, disabled);
    if (observe && cui_frame_sequence(ui) == frame && before != *current) {
        cui_stepper_change_event event = {0};
        (void)cui_component_event_init(&event.event, CUI_COMPONENT_EVENT_CHANGE, id,
                                       CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
        event.event.flags = CUI_COMPONENT_EVENT_HAS_FRAME;
        event.event.frame_sequence = frame;
        event.before = before;
        event.after = *current;
        event.count = count;
        result.change_count = 1;
        handler.change(handler.user, &event);
    }
    return result;
}
cui_numeric_selection_event_result cui_base_rating_events_ex(cui_context *ui, cui_id id,
                                                             cui_str name, float *value,
                                                             cui_rating_options options,
                                                             cui_rating_selection selection,
                                                             cui_rating_event_handler handler) {
    if (selection != CUI_RATING_SELECT_VALUE && selection != CUI_RATING_STEP_DOWN_FILLED)
        return (cui_numeric_selection_event_result){0};
    uint64_t frame = cui_frame_sequence(ui);
    bool observe = handler.change && frame && id && value && !options.disabled && !options.readonly;
    float before = observe ? *value : 0;
    cui_numeric_selection_event_result result = {0};
    result.control = cui_base_rating_ex(ui, id, name, value, options, selection);
    if (observe && cui_frame_sequence(ui) == frame && isfinite(*value) &&
        (!isfinite(before) || before != *value)) {
        cui_rating_change_event event = {0};
        (void)cui_component_event_init(&event.event, CUI_COMPONENT_EVENT_CHANGE, id,
                                       CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
        event.event.flags = CUI_COMPONENT_EVENT_HAS_FRAME;
        event.event.frame_sequence = frame;
        event.before = before;
        event.after = *value;
        event.before_finite = isfinite(before);
        event.maximum = options.maximum ? options.maximum : 5;
        if (event.maximum > 100)
            event.maximum = 100;
        result.change_count = 1;
        handler.change(handler.user, &event);
    }
    return result;
}
cui_numeric_selection_event_result cui_base_rating_events(cui_context *ui, cui_id id, cui_str name,
                                                          float *value, cui_rating_options options,
                                                          cui_rating_event_handler handler) {
    return cui_base_rating_events_ex(ui, id, name, value, options, CUI_RATING_SELECT_VALUE,
                                     handler);
}
