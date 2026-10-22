/* SPDX-License-Identifier: MIT */
#ifdef CUI_DATETIME_ALL
#define CUI_IMPLEMENTATION_ALL
#include "cui_all.h"
#else
#define CUI_IMPLEMENTATION
#include "cui.h"
#define CUI_BASE_IMPLEMENTATION
#include "cui_base.h"
#define CUI_THEME_IMPLEMENTATION
#include "cui_theme.h"
#define CUI_COMPONENTS_IMPLEMENTATION
#include "cui_components.h"
#define CUI_DATE_TIME_PICKER_IMPLEMENTATION
#include "cui_date_time_picker.h"
#endif
#include <assert.h>
static cui_date_time_picker_result frame(cui_context *ui, const cui_theme *theme,
                                         cui_date_time_picker_state *state,
                                         cui_date_time_value *value) {
    assert(cui_begin_frame(ui, 1000, 800, .016f));
    cui_date_time_picker_result result = cui_date_time_picker(
        ui, 31, theme, cui_string("Appointment"), state, value,
        (cui_date_time_picker_options){.picker = {.viewport = {0, 0, 1000, 800}},
                                       .precision = CUI_DATE_TIME_SECOND});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && result.valid);
    return result;
}
int main(void) {
    cui_context *ui = cui_create(
        (cui_config){.width = 1000, .height = 800, .max_elements = 1024, .max_commands = 2048});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_date_time_picker_state state;
    cui_date_time_picker_init(&state, 32, (cui_date){2024, 2, 1});
    cui_date_time_value value = {{2024, 2, 15}, {9, 30, 45}};
    frame(ui, &theme, &state, &value);
    assert(cui_queue_accessibility_action(ui, 31, CUI_ACCESSIBILITY_ACTIVATE));
    frame(ui, &theme, &state, &value);
    assert(state.actions.picker.overlay.open);
    cui_focus(ui, cui_time_segment_id(cui_date_time_picker_time_id(31), 1));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    cui_date_time_picker_result result = frame(ui, &theme, &state, &value);
    assert(result.time_edited && result.source_change && value.time.minute == 31);
    assert(cui_queue_accessibility_action(
        ui, cui_calendar_day_id(cui_date_picker_calendar_id(31), value.date),
        CUI_ACCESSIBILITY_ACTIVATE));
    result = frame(ui, &theme, &state, &value);
    assert(result.confirmed && !state.actions.picker.overlay.open && !result.value_changed);
    cui_destroy(ui);
    return 0;
}
