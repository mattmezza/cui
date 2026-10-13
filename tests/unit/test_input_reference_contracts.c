/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <string.h>
static cui_draw_list painted;
static cui_input_result frame(cui_context *ui, cui_edit *edit, cui_input_state *state,
                              cui_input_options options) {
    assert(cui_begin_frame(ui, 600, 180, .016f));
    cui_theme theme = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    cui_input_result result =
        cui_input_ex(ui, 10, &theme, cui_string("Reference Input"), edit, state, options);
    cui_button(ui, 20, &theme, cui_string("Other focus"), false);
    cui_end_box(ui);
    painted = cui_end_frame(ui);
    assert(painted.count && cui_last_error(ui) == CUI_OK);
    return result;
}
static void key(cui_context *ui, cui_key key, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key, .modifiers = modifiers}));
}
static void text(cui_context *ui, const char *value) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = value, .text_length = strlen(value)}));
}
static void navigation_history(void) {
    cui_context *ui = cui_create((cui_config){0});
    char buffer[128] = "";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 16});
    cui_input_state state = {0};
    assert(ui && edit);
    frame(ui, edit, &state, (cui_input_options){0});
    cui_focus(ui, 10);
    text(ui, "ab");
    frame(ui, edit, &state, (cui_input_options){0});
    text(ui, "cd");
    frame(ui, edit, &state, (cui_input_options){0});
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(!strcmp(buffer, ""));
    key(ui, CUI_KEY_Y, CUI_MOD_CTRL);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(!strcmp(buffer, "abcd"));
    assert(cui_edit_select(edit, 2, 2) == CUI_EDIT_OK);
    key(ui, CUI_KEY_UP, 0);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(cui_edit_get_state(edit).cursor == 2 && cui_edit_get_state(edit).anchor == 2);
    key(ui, CUI_KEY_DOWN, CUI_MOD_SHIFT | CUI_MOD_CTRL);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(cui_edit_get_state(edit).cursor == 2 && cui_edit_get_state(edit).anchor == 2);
    key(ui, CUI_KEY_DOWN, 0);
    frame(ui, edit, &state, (cui_input_options){.delegate_up_down = true});
    assert(!cui_event_consumed(ui, 0));
    assert(cui_edit_select(edit, 4, 4) == CUI_EDIT_OK);
    text(ui, "x");
    frame(ui, edit, &state, (cui_input_options){0});
    cui_focus(ui, 20);
    frame(ui, edit, &state, (cui_input_options){0});
    cui_focus(ui, 10);
    text(ui, "y");
    frame(ui, edit, &state, (cui_input_options){0});
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(!strcmp(buffer, "abcdx"));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void grapheme_states(void) {
    cui_context *ui = cui_create((cui_config){0});
    char buffer[128] = "e\xcc\x81\xf0\x9f\x87\xa8\xf0\x9f\x87\xad";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 16});
    cui_input_state state = {0};
    frame(ui, edit, &state, (cui_input_options){0});
    cui_focus(ui, 10);
    assert(cui_edit_select(edit, strlen(buffer), strlen(buffer)) == CUI_EDIT_OK);
    key(ui, CUI_KEY_LEFT, 0);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(cui_edit_get_state(edit).cursor == 3);
    key(ui, CUI_KEY_BACKSPACE, 0);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(strlen(buffer) == 8);
    key(ui, CUI_KEY_Z, CUI_MOD_META);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(strlen(buffer) == 11);
    key(ui, CUI_KEY_Z, CUI_MOD_META | CUI_MOD_SHIFT);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(strlen(buffer) == 8);
    cui_input_options options = {.readonly = true, .required = true, .invalid = true};
    text(ui, "blocked");
    cui_input_result result = frame(ui, edit, &state, options);
    assert(!result.changed && result.error == CUI_EDIT_OK && strlen(buffer) == 8);
    size_t count = 0;
    const cui_semantic *sem = cui_semantics(ui, &count);
    bool found = false;
    for (size_t i = 0; i < count; i++)
        if (sem[i].id == 10) {
            found = true;
            assert(sem[i].details.readonly && sem[i].details.required && sem[i].details.invalid &&
                   !sem[i].disabled);
        }
    assert(found);
    bool danger = false;
    cui_color expected = cui_theme_light().danger;
    for (size_t i = 0; i < painted.count; i++) {
        cui_color c = painted.commands[i].color;
        if (c.r == expected.r && c.g == expected.g && c.b == expected.b && c.a == expected.a)
            danger = true;
    }
    assert(danger); /* Explicit invalid is semantic and painted in the same frame. */
    options.disabled = true;
    key(ui, CUI_KEY_BACKSPACE, 0);
    result = frame(ui, edit, &state, options);
    assert(!result.changed && strlen(buffer) == 8);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void multiline_rebind_and_error(void) {
    cui_context *ui = cui_create((cui_config){0});
    char first[64] = "ab\ncd", second[64] = "";
    cui_edit *a = cui_edit_create(
        (cui_edit_config){.buffer = first, .capacity = sizeof first, .multiline = true});
    cui_edit *b = cui_edit_create((cui_edit_config){.buffer = second,
                                                    .capacity = sizeof second,
                                                    .history_bytes = 1024,
                                                    .max_history_steps = 8});
    cui_input_state state = {0};
    assert(ui && a && b);
    frame(ui, a, &state, (cui_input_options){0});
    cui_focus(ui, 10);
    assert(cui_edit_select(a, 1, 1) == CUI_EDIT_OK);
    key(ui, CUI_KEY_DOWN, 0);
    frame(ui, a, &state, (cui_input_options){0});
    assert(cui_edit_get_state(a).cursor == 4);
    key(ui, CUI_KEY_UP, CUI_MOD_SHIFT);
    frame(ui, a, &state, (cui_input_options){0});
    assert(cui_edit_get_state(a).anchor == 4 && cui_edit_get_state(a).cursor == 1);
    assert(cui_edit_type(b, cui_string("prior")) == CUI_EDIT_OK);
    cui_edit_destroy(a); /* Retained state may compare, never dereference the old identity. */
    text(ui, "new");
    frame(ui, b, &state, (cui_input_options){0});
    assert(!strcmp(second, "priornew"));
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, b, &state, (cui_input_options){0});
    assert(!strcmp(second, "prior"));
    uint64_t revision = cui_edit_get_state(b).revision;
    text(ui, "\n");
    cui_input_result result = frame(ui, b, &state, (cui_input_options){0});
    assert(!result.changed && result.error == CUI_EDIT_LINE_BREAK &&
           cui_edit_get_state(b).revision == revision && !strcmp(second, "prior"));
    size_t count;
    const cui_semantic *semantic = cui_semantics(ui, &count);
    bool invalid = false;
    for (size_t i = 0; i < count; i++)
        if (semantic[i].id == 10)
            invalid = semantic[i].details.invalid;
    assert(invalid); /* Failure status is published by the actual composed reducer. */
    cui_edit_destroy(b);
    cui_destroy(ui);
}
static void composition_history(void) {
    cui_context *ui = cui_create((cui_config){0});
    char buffer[64] = "seed";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 8});
    cui_input_state state = {0};
    frame(ui, edit, &state, (cui_input_options){0});
    cui_focus(ui, 10);
    assert(cui_edit_select(edit, 0, 4) == CUI_EDIT_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .text_data = "\xe6\xbc\xa2",
                                           .text_length = 3,
                                           .composition_cursor = 3}));
    assert(!frame(ui, edit, &state, (cui_input_options){0}).changed && !strcmp(buffer, "seed"));
    assert(state.behavior.composition && state.behavior.composition->active);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                                           .text_data = "\xe6\xbc\xa2",
                                           .text_length = 3,
                                           .composition_cursor = 3}));
    assert(frame(ui, edit, &state, (cui_input_options){0}).changed && strlen(buffer) == 3);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(!strcmp(buffer, "seed"));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .text_data = "x",
                                           .text_length = 1,
                                           .composition_cursor = 1}));
    frame(ui, edit, &state, (cui_input_options){0});
    cui_focus(ui, 20);
    frame(ui, edit, &state, (cui_input_options){0});
    assert(!state.behavior.composition->active && !strcmp(buffer, "seed"));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void legacy_grouping(void) {
    cui_context *ui = cui_create((cui_config){0});
    char buffer[32] = "";
    cui_edit *edit = cui_edit_create((cui_edit_config){
        .buffer = buffer, .capacity = sizeof buffer, .history_bytes = 512, .max_history_steps = 8});
    for (unsigned i = 0; i < 4; i++) {
        if (i == 1) {
            cui_focus(ui, 10);
            text(ui, "a");
        }
        if (i == 2)
            text(ui, "b");
        if (i == 3)
            key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
        assert(cui_begin_frame(ui, 300, 80, .016f));
        cui_begin_box(ui, 10, (cui_box){.width = 200, .height = 32});
        assert(cui_base_input(ui, 10, cui_string("Legacy"), edit, false).error == CUI_EDIT_OK);
        cui_end_box(ui);
        (void)cui_end_frame(ui);
        if (i == 2)
            assert(!strcmp(buffer, "ab"));
    }
    assert(!strcmp(buffer, ""));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
int main(void) {
    navigation_history();
    grapheme_states();
    multiline_rebind_and_error();
    composition_history();
    legacy_grouping();
    return 0;
}
