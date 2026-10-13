/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include <assert.h>
#include <string.h>
static size_t hit_offset;
static size_t hit(void *user, cui_str text, float x, float y) {
    (void)user;
    (void)text;
    (void)x;
    (void)y;
    return hit_offset;
}
static cui_rect bounds;
static bool consume_press;
static cui_input_result frame(cui_context *ui, cui_edit *edit, cui_input_behavior_state *state,
                              cui_base_input_options options) {
    assert(cui_begin_frame(ui, 400, 120, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1});
    cui_begin_box(ui, 10, (cui_box){.width = 300, .height = 40});
    options.hit_test = hit;
    if (consume_press)
        cui_consume_event(ui, 0);
    cui_input_result result =
        cui_base_input_ex(ui, 10, cui_string("Pointer"), edit, state, options);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    bounds = cui_bounds(ui, 10);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void pointer(cui_context *ui, cui_event_type type, unsigned clicks, unsigned modifiers,
                    size_t offset) {
    hit_offset = offset;
    assert(cui_queue_event(ui, (cui_event){.type = type,
                                           .x = bounds.x + 10,
                                           .y = bounds.y + 10,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .click_count = clicks,
                                           .modifiers = modifiers}));
}
static void selection(cui_edit *edit, size_t anchor, size_t cursor) {
    cui_edit_state s = cui_edit_get_state(edit);
    assert(s.anchor == anchor && s.cursor == cursor);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    char document[512] = "one two three";
    cui_edit *edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 16});
    assert(edit);
    cui_input_behavior_state state = {0};
    cui_base_input_options options = {0};
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_OK);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 1, 0, 3);
    frame(ui, edit, &state, options);
    selection(edit, 3, 3);
    pointer(ui, CUI_EVENT_POINTER_UP, 0, 0, 3);
    frame(ui, edit, &state, options);
    selection(edit, 3, 3);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 5);
    assert(!frame(ui, edit, &state, options).changed);
    selection(edit, 4, 7);
    pointer(ui, CUI_EVENT_POINTER_MOVE, 0, 0, 2);
    frame(ui, edit, &state, options);
    selection(edit, 7, 2);
    pointer(ui, CUI_EVENT_POINTER_MOVE, 0, 0, 6);
    frame(ui, edit, &state, options);
    selection(edit, 4, 7);
    pointer(ui, CUI_EVENT_POINTER_MOVE, 0, 0, 10);
    frame(ui, edit, &state, options);
    selection(edit, 4, 10);
    pointer(ui, CUI_EVENT_POINTER_UP, 0, 0, 10);
    frame(ui, edit, &state, options);
    assert(!state.dragging && !state.pointer_word_active);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 1, CUI_MOD_SHIFT, 2);
    frame(ui, edit, &state, options);
    selection(edit, 4, 2);
    pointer(ui, CUI_EVENT_POINTER_UP, 0, 0, 2);
    frame(ui, edit, &state, options);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 3, 0, 5);
    frame(ui, edit, &state, options);
    selection(edit, 0, 13);
    pointer(ui, CUI_EVENT_POINTER_MOVE, 0, 0, 6);
    frame(ui, edit, &state, options);
    selection(edit, 0, 6); /* Source clears sticky range for line selection. */
    pointer(ui, CUI_EVENT_POINTER_UP, 0, 0, 6);
    frame(ui, edit, &state, options);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, CUI_MOD_SHIFT, 8);
    frame(ui, edit, &state, options);
    selection(edit, 8, 13);
    pointer(ui, CUI_EVENT_POINTER_UP, 0, 0, 13);
    frame(ui, edit, &state, options);
    assert(cui_edit_select(edit, 2, 2) == CUI_EDIT_OK);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 13);
    frame(ui, edit, &state, options);
    selection(edit, 2, 2); /* Source end-of-value no word. */
    pointer(ui, CUI_EVENT_POINTER_UP, 0, 0, 13);
    frame(ui, edit, &state, options);
    selection(edit, 2, 2);
    options.readonly = true;
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 5);
    frame(ui, edit, &state, options);
    selection(edit, 4, 7);
    options.disabled = true;
    pointer(ui, CUI_EVENT_POINTER_DOWN, 1, 0, 1);
    frame(ui, edit, &state, options);
    selection(edit, 4, 7);
    assert(!state.dragging && !state.pointer_word_active);
    options.disabled = false;
    options.readonly = false;
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 5);
    frame(ui, edit, &state, options);
    assert(state.pointer_word_active);
    options.masked = true;
    frame(ui, edit, &state, options);
    assert(!state.pointer_word_active);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 5);
    frame(ui, edit, &state, options);
    selection(edit, 0, 13);
    options.masked = false;
    frame(ui, edit, &state, options);
    cui_edit_token_span token = {
        .start = 4,
        .end = 7,
        .token = {.id = {"tag", 3}, .text = {"two", 3}, .label = {"Token", 5}}};
    assert(cui_edit_set_tokens(edit, &token, 1) == CUI_EDIT_OK);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    consume_press = true;
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 5);
    frame(ui, edit, &state, options);
    selection(edit, 1, 1);
    consume_press = false;
    assert(cui_edit_set_tokens(edit, NULL, 0) == CUI_EDIT_OK);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 5);
    frame(ui, edit, &state, options);
    assert(state.pointer_word_active);
    consume_press = true;
    pointer(ui, CUI_EVENT_POINTER_UP, 0, 0, 5);
    frame(ui, edit, &state, options);
    assert(!state.pointer_word_active && !state.dragging);
    consume_press = false;

    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 5);
    frame(ui, edit, &state, options);
    assert(state.pointer_word_active);
    char other_document[32] = "other";
    cui_edit *other =
        cui_edit_create((cui_edit_config){other_document, sizeof other_document, false, 0, 0});
    assert(other);
    frame(ui, other, &state, options);
    assert(!state.pointer_word_active);
    cui_edit_destroy(other);
    frame(ui, edit, &state, options);
    cui_edit_destroy(edit);
    strcpy(document, "rök é 中文🎉 [()]  ");
    edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 16});
    assert(edit);
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 1);
    frame(ui, edit, &state, options);
    selection(edit, 0, 4);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 6);
    frame(ui, edit, &state, options);
    selection(edit, 5, 8);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 9);
    frame(ui, edit, &state, options);
    selection(edit, 9, 12);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 12);
    frame(ui, edit, &state, options);
    selection(edit, 12, 15);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 15);
    frame(ui, edit, &state, options);
    selection(edit, 15, 19);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 20);
    frame(ui, edit, &state, options);
    selection(edit, 20, 21);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 24);
    frame(ui, edit, &state, options);
    selection(edit, 24, 26);
    cui_edit_destroy(edit);
    memset(document, 'a', 400);
    document[400] = 0;
    edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 16});
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 2, 0, 200);
    frame(ui, edit, &state, options);
    selection(edit, 72, 329); /* Exactly128 scalars each side. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    frame(ui, edit, &state, options);
    assert(!state.pointer_word_active && !state.dragging);
    cui_edit_destroy(edit);
    strcpy(document, "one\ntwo");
    edit = cui_edit_create((cui_edit_config){document, sizeof document, true, 2048, 16});
    assert(edit);
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 3, 0, 5);
    frame(ui, edit, &state, options);
    selection(edit, 5, 5);
    cui_destroy(ui);
    cui_edit_destroy(edit);
    return 0;
}
