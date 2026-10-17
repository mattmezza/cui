/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {
    cui_slider_request requests[64];
    unsigned count;
} log_t;
static void request(void *user, const cui_slider_request *r) {
    log_t *log = user;
    assert(log->count < 64);
    log->requests[log->count++] = *r;
}
static void frame(cui_context *ui, cui_slider_state *s, cui_slider_options o, double value) {
    assert(cui_begin_frame(ui, 400, 100, .016f));
    cui_begin_box(ui, 10,
                  (cui_box){.width = 200, .height = 40, .fixed_width = true, .fixed_height = true});
    assert(cui_base_slider_controlled(ui, 10, cui_string("Position"), value, s, o));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void event(cui_context *ui, cui_event_type type, float x, cui_pointer_type pointer) {
    assert(cui_queue_event(
        ui, (cui_event){.type = type, .x = x, .y = 20, .pointer_type = pointer, .pointer_id = 7}));
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 100});
    assert(ui);
    log_t log = {0};
    cui_slider_state state = {0};
    double value = 10000000000.125;
    cui_slider_options o = {.minimum = 10000000000,
                            .maximum = 10000000001,
                            .step = .125,
                            .value_description = cui_string("0.125 seconds"),
                            .on_request = request,
                            .user = &log};
    frame(ui, &state, o, value);
    cui_focus(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    frame(ui, &state, o, value);
    assert(log.count == 3 && log.requests[0].phase == CUI_SLIDER_BEGIN &&
           log.requests[2].phase == CUI_SLIDER_COMMIT);
    assert(log.requests[2].value == 10000000000.25 && value == 10000000000.125);
    /* Rejected owner change: next submission restores controlled presentation. */
    frame(ui, &state, o, value);
    assert(state.preview == value);
    size_t count = 0;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].value == .125f && sem[0].details.maximum == 1);
    assert(sem[0].details.description.length == 13);
    log.count = 0;
    event(ui, CUI_EVENT_POINTER_DOWN, 50, CUI_POINTER_TOUCH);
    frame(ui, &state, o, value);
    assert(state.dragging && log.count == 2);
    event(ui, CUI_EVENT_POINTER_MOVE, 150, CUI_POINTER_TOUCH);
    frame(ui, &state, o, value);
    assert(state.preview == 10000000000.75);
    event(ui, CUI_EVENT_CANCEL, 150, CUI_POINTER_TOUCH);
    frame(ui, &state, o, value);
    assert(!state.dragging && log.requests[log.count - 1].phase == CUI_SLIDER_CANCEL);
    assert(state.preview == value && log.requests[1].event.pointer_type == CUI_POINTER_TOUCH);
    log.count = 0;
    event(ui, CUI_EVENT_POINTER_DOWN, 50, CUI_POINTER_MOUSE);
    frame(ui, &state, o, value);
    event(ui, CUI_EVENT_POINTER_UP, 100, CUI_POINTER_MOUSE);
    frame(ui, &state, o, value);
    assert(!state.dragging && log.requests[log.count - 1].phase == CUI_SLIDER_COMMIT);
    assert(log.requests[log.count - 1].value == 10000000000.5);
    unsigned old = log.count;
    o.on_request = NULL;
    event(ui, CUI_EVENT_POINTER_DOWN, 50, CUI_POINTER_MOUSE);
    frame(ui, &state, o, value);
    assert(log.count == old && !state.dragging);
    sem = cui_semantics(ui, &count);
    assert(sem[0].details.readonly);
    o.on_request = request;
    o.disabled = true;
    event(ui, CUI_EVENT_POINTER_UP, 50, CUI_POINTER_MOUSE);
    frame(ui, &state, o, value);
    assert(log.count == old);
    /* Numeric validation is exercised during active submission, not hidden by
     * the out-of-frame guard. */
    assert(cui_begin_frame(ui, 400, 100, .016f));
    cui_slider_state before = state;
    assert(!cui_base_slider_controlled(ui, 10, cui_string("Position"), NAN, &state, o));
    assert(memcmp(&before, &state, sizeof state) == 0);
    cui_end_frame(ui);
    cui_destroy(ui);
    return 0;
}
