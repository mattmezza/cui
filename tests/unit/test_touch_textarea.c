/* SPDX-License-Identifier: MIT */
#include "cui/cui_textarea.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void frame(cui_context *ui, cui_edit *edit, cui_textarea_state *state) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_textarea_result result = cui_textarea(ui, 10, &theme, cui_string("Notes"), edit, state,
                                              (cui_textarea_options){.width = 200, .rows = 5});
    assert(result.error == CUI_TEXTAREA_OK);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void touch(cui_context *ui, cui_event_type type, float x, float y) {
    assert(cui_queue_event(
        ui, (cui_event){
                .type = type, .pointer_type = CUI_POINTER_TOUCH, .pointer_id = 9, .x = x, .y = y}));
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(ui);
    char document[128];
    for (size_t i = 0; i < 79; ++i)
        document[i] = i % 2 ? '\n' : 'a';
    document[79] = 0;
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = document,
                                                       .capacity = sizeof(document),
                                                       .multiline = true,
                                                       .history_bytes = 8192,
                                                       .max_history_steps = 32});
    assert(edit);
    cui_textarea_line lines[64];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 64));
    char preedit[128], view[256];
    cui_edit_composition composition = {.buffer = preedit,
                                        .capacity = sizeof(preedit),
                                        .view_buffer = view,
                                        .view_capacity = sizeof(view)};
    state.composition = &composition;
    frame(ui, edit, &state);
    cui_focus(ui, 10);
    cui_edit_select(edit, 0, 0);
    frame(ui, edit, &state);
    cui_edit_select(edit, 79, 79);
    frame(ui, edit, &state);
    assert(state.scroll_y == state.maximum_y && state.scroll_y > 35);
    float x = state.viewport.x + 10, y = state.viewport.y + state.viewport.h - 3;
    touch(ui, CUI_EVENT_POINTER_DOWN, x, y);
    frame(ui, edit, &state);
    float before = state.scroll_y;
    size_t caret = cui_edit_get_state(edit).cursor;
    touch(ui, CUI_EVENT_POINTER_MOVE, x, y + 35);
    frame(ui, edit, &state);
    /* Capture cancellation must not reveal the caret and undo the user's pan. */
    assert(fabsf(state.scroll_y - (before - 35)) < .01f);
    assert(!state.dragging && !composition.active);
    assert(cui_edit_get_state(edit).cursor == caret && strlen(document) == 79);
    touch(ui, CUI_EVENT_POINTER_UP, x, y + 35);
    frame(ui, edit, &state);
    frame(ui, edit, &state);
    assert(fabsf(state.scroll_y - (before - 35)) < .01f);
    cui_edit_destroy(edit);
    cui_destroy(ui);
    return 0;
}
