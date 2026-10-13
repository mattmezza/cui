/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <math.h>
typedef struct {
    cui_context *ui;
    unsigned depth, calls;
    bool invalid;
} measure_data;
static cui_size measure(void *user, cui_str text, float size) {
    measure_data *data = user;
    assert(!cui_frame_sequence(data->ui));
    assert(!cui_frame_is_submitting(data->ui));
    assert(!cui_event_accepted_activation(data->ui, 1));
    ++data->calls;
    if (!data->depth) {
        ++data->depth;
        cui_measure_text(data->ui, cui_string("Nested"), size);
        --data->depth;
    }
    return (cui_size){data->invalid ? NAN : text.length * size / 2, size};
}
static bool paint(const cui_prepaint_info *info, const void *payload, cui_prepaint_writer *writer) {
    (void)info;
    (void)writer;
    cui_context *ui = *(cui_context *const *)payload;
    assert(!cui_frame_sequence(ui));
    assert(!cui_frame_is_submitting(ui));
    assert(!cui_event_accepted_activation(ui, 1));
    return true;
}
static void box(cui_context *ui) {
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    cui_begin_absolute_box(ui, 10, (cui_box){0}, (cui_rect){10, 10, 80, 30});
}
static void finish(cui_context *ui) {
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(!cui_frame_sequence(ui));
    assert(!cui_event_accepted_activation(ui, 1));
}
static void click(cui_context *ui) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 30, .y = 20}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 30, .y = 20}));
}
int main(void) {
    measure_data data = {0};
    cui_context *ui = cui_create((cui_config){.measure = measure, .measure_user = &data});
    assert(ui);
    data.ui = ui;
    assert(!cui_frame_sequence(ui) && !cui_frame_sequence(NULL));
    assert(cui_begin_frame(ui, 200, 100, 0));
    uint64_t sequence = cui_frame_sequence(ui);
    assert(sequence);
    assert(cui_frame_is_submitting(ui));
    box(ui);
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), false, false, 0);
    cui_text(ui, cui_string("Ten"), 14, (cui_color){0});
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = paint,
                                                       .payload = &ui,
                                                       .payload_bytes = sizeof ui,
                                                       .command_budget = 1}));
    cui_measure_text(ui, cui_string("Direct"), 14);
    assert(cui_frame_sequence(ui) == sequence && data.calls >= 2);
    data.invalid = true;
    assert(cui_measure_text(ui, cui_string("Invalid"), 14).width == 0);
    assert(cui_frame_sequence(ui) == sequence);
    data.invalid = false;
    finish(ui);
    click(ui);
    click(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(cui_frame_sequence(ui) == sequence + 1);
    box(ui);
    assert(cui_event_activation(ui, 1) == 10 && cui_event_activation(ui, 3) == 10);
    assert(!cui_event_accepted_activation(ui, 1));
    assert(cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), false, false, 0).activated);
    assert(cui_event_accepted_activation(ui, 1) == 10 &&
           cui_event_accepted_activation(ui, 3) == 10);
    assert(cui_event_activation(ui, 1) == 10); /* Original generated evidence preserved. */
    finish(ui);
    click(ui);
    click(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    box(ui);
    assert(cui_cancel_event_activation(ui, 1));
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), false, false, 0);
    assert(!cui_event_accepted_activation(ui, 1) && cui_event_accepted_activation(ui, 3) == 10);
    finish(ui);
    click(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    box(ui);
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), true, false, 0);
    assert(!cui_event_accepted_activation(ui, 1));
    finish(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    box(ui);
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), false, false, 0);
    finish(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 10,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    assert(cui_begin_frame(ui, 200, 100, 0));
    box(ui);
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), false, false, 0);
    assert(cui_event_accepted_activation(ui, 0) == 10);
    assert(cui_set_semantic_role(ui, 10, CUI_ROLE_LINK));
    assert(!cui_event_accepted_activation(ui, 0));
    finish(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    box(ui);
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), false, false, 0);
    finish(ui);
    click(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    box(ui);
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), false, false, 0);
    assert(cui_event_accepted_activation(ui, 1) == 10);
    cui_end_box(ui);
    assert(cui_begin_focus_scope(ui, 200, true));
    cui_interact(ui, 20, CUI_ROLE_BUTTON, cui_string("Modal"), false, false, 0);
    assert(!cui_event_accepted_activation(ui, 1));
    cui_end_focus_scope(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    sequence = cui_frame_sequence(ui);
    cui_begin_box(ui, 30, (cui_box){.inert = true});
    assert(!cui_frame_sequence(ui));
    assert(cui_frame_is_submitting(ui));
    cui_end_box(ui);
    assert(cui_frame_sequence(ui) == sequence);
    assert(!cui_begin_frame(ui, 200, 100, 0));
    assert(cui_frame_sequence(ui) == sequence);
    cui_end_frame(ui); /* Failed lifecycle end still clears resolving-layout guard. */
    assert(!cui_begin_frame(ui, 200, 100, -1));
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(cui_frame_sequence(ui) == sequence + 1);
    cui_end_frame(ui);
    cui_destroy(ui);
}
