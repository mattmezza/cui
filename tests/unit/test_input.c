/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <string.h>

static char clipboard[128];
static bool reject_copy;
static bool copy(void *user, cui_str value) {
    (void)user;
    if (reject_copy || value.length >= sizeof clipboard)
        return false;
    memcpy(clipboard, value.data, value.length);
    clipboard[value.length] = 0;
    return true;
}
static cui_str paste(void *user) {
    (void)user;
    return cui_string(clipboard);
}
static void key(cui_context *ui, cui_key code, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code, .modifiers = modifiers}));
}
static cui_input_result base_frame(cui_context *ui, cui_edit *edit, cui_input_behavior_state *state,
                                   cui_base_input_options options) {
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 40});
    cui_input_result result = cui_base_input_ex(ui, 1, cui_string("Input"), edit, state, options);
    cui_end_box(ui);
    assert(cui_end_frame(ui).count == 0);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void clipboard_and_readonly(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    char buffer[128] = "caf\xc3\xa9";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .history_bytes = 4096,
                                                       .max_history_steps = 32});
    assert(ui && edit);
    cui_platform host = {
        .capabilities = CUI_PLATFORM_CLIPBOARD, .clipboard_set = copy, .clipboard_get = paste};
    cui_input_hit_geometry hit = {.ui = ui, .viewport = {10, 0, 200, 40}, .font_size = 10};
    assert(cui_base_input_hit_test(&hit, cui_string("ab"), 13, 0) == 1);
    assert(cui_base_input_hit_test(&hit, cui_string("ab"), -10, 0) == 0);
    assert(cui_base_input_hit_test(&hit, cui_string("ab"), 100, 0) == 2);
    assert(cui_base_input_hit_test(&hit, cui_string("\xc3"), 20, 0) == SIZE_MAX);
    cui_input_behavior_state state = {0};
    cui_base_input_options options = {.clipboard = &host, .required = true};
    base_frame(ui, edit, &state, options);
    cui_focus(ui, 1);
    assert(cui_edit_select(edit, 0, 5) == CUI_EDIT_OK);
    key(ui, CUI_KEY_X, CUI_MOD_CTRL);
    assert(base_frame(ui, edit, &state, options).changed);
    assert(!strcmp(clipboard, "caf\xc3\xa9") && !buffer[0]);
    key(ui, CUI_KEY_V, CUI_MOD_CTRL);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    assert(base_frame(ui, edit, &state, options).changed && !buffer[0]);
    key(ui, CUI_KEY_Z, CUI_MOD_META | CUI_MOD_SHIFT);
    assert(base_frame(ui, edit, &state, options).changed && !strcmp(buffer, clipboard));
    assert(cui_edit_select(edit, 0, 5) == CUI_EDIT_OK);
    reject_copy = true;
    key(ui, CUI_KEY_X, CUI_MOD_CTRL);
    cui_input_result result = base_frame(ui, edit, &state, options);
    assert(!result.changed && result.error == CUI_EDIT_CLIPBOARD_UNAVAILABLE &&
           !strcmp(buffer, clipboard));
    reject_copy = false;
    strcpy(clipboard, "\xc3");
    key(ui, CUI_KEY_V, CUI_MOD_CTRL);
    result = base_frame(ui, edit, &state, options);
    assert(!result.changed && result.error == CUI_EDIT_INVALID_UTF8 &&
           !strcmp(buffer, "caf\xc3\xa9"));
    options.readonly = options.invalid = true;
    key(ui, CUI_KEY_C, CUI_MOD_META);
    key(ui, CUI_KEY_BACKSPACE, 0);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "ignored"}));
    result = base_frame(ui, edit, &state, options);
    assert(!result.changed && result.error == CUI_EDIT_OK && !strcmp(buffer, clipboard));
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(count == 1 && semantics[0].details.readonly && semantics[0].details.required &&
           semantics[0].details.invalid);
    options.disabled = true;
    key(ui, CUI_KEY_LEFT, 0);
    assert(!base_frame(ui, edit, &state, options).changed && cui_edit_get_state(edit).cursor == 5);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
#ifndef CUI_INPUT_BASE_ONLY
static cui_size measure(void *user, cui_str text, float font_size) {
    (void)user;
    (void)font_size;
    float width = 0;
    for (size_t i = 0; i < text.length; ++i) {
        unsigned char c = (unsigned char)text.data[i];
        if ((c & 0xc0) == 0x80)
            continue;
        width += c == 'W' ? 20 : c == 'i' ? 4 : c >= 128 ? 12 : 10;
    }
    return (cui_size){width, 20};
}
static cui_draw_list styled_frame(cui_context *ui, cui_edit *edit, const cui_theme *theme,
                                  cui_input_state *state, cui_input_options options) {
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_input_result result = cui_input_ex(ui, 1, theme, cui_string("Name"), edit, state, options);
    assert(result.error == CUI_EDIT_OK);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static cui_rect text_bounds(cui_draw_list list, const char *text) {
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_TEXT &&
            list.commands[i].text.length == strlen(text) &&
            !memcmp(list.commands[i].text.data, text, strlen(text)))
            return list.commands[i].bounds;
    assert(false);
    return (cui_rect){0};
}
static void pointer(cui_context *ui, cui_event_type type, float x, float y, unsigned modifiers) {
    assert(cui_queue_event(ui, (cui_event){.type = type, .x = x, .y = y, .modifiers = modifiers}));
}
static void paint_only_reduction(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200, .measure = measure});
    char buffer[32] = "ab";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_state state = {0};
    cui_theme theme = cui_theme_light();
    assert(ui && edit && cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    styled_frame(ui, edit, &theme, &state, (cui_input_options){0});
    cui_focus(ui, 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Q"}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_input_result reduced = cui_base_input_ex(ui, 1, cui_string("Name"), edit, &state.behavior,
                                                 (cui_base_input_options){0});
    assert(reduced.changed);
    cui_input_paint(ui, 1, &theme, cui_string("Name"), edit, &state, (cui_input_options){0});
    cui_end_frame(ui);
    assert(!strcmp(buffer, "aQb"));
    size_t count;
    assert(cui_semantics(ui, &count) && count == 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "R"}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_input_paint(ui, 1, &theme, cui_string("Name"), edit, &state, (cui_input_options){0});
    cui_end_frame(ui);
    assert(!strcmp(buffer, "aQb") && cui_last_error(ui) == CUI_OK);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void styled_selection(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t theme_index = 0; theme_index < 4; ++theme_index) {
        cui_context *ui = cui_create((cui_config){.width = 400, .height = 200, .measure = measure});
        char buffer[128] = "Wi\xc3\xa9x";
        cui_edit *edit =
            cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
        cui_input_state state = {0};
        cui_input_options options = {.width = 180};
        assert(ui && edit && cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
        cui_draw_list list = styled_frame(ui, edit, &themes[theme_index], &state, options);
        cui_rect bounds = text_bounds(list, buffer);
        assert(bounds.w == 46);
        pointer(ui, CUI_EVENT_POINTER_DOWN, bounds.x + 21, bounds.y + 5, 0);
        pointer(ui, CUI_EVENT_POINTER_UP, bounds.x + 21, bounds.y + 5, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(cui_edit_get_state(edit).cursor == 1 && cui_edit_get_state(edit).anchor == 1);
        pointer(ui, CUI_EVENT_POINTER_DOWN, bounds.x + 35, bounds.y + 5, CUI_MOD_SHIFT);
        pointer(ui, CUI_EVENT_POINTER_UP, bounds.x + 35, bounds.y + 5, CUI_MOD_SHIFT);
        list = styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(cui_edit_get_state(edit).cursor == 4 && cui_edit_get_state(edit).anchor == 1);
        cui_rect selected = text_bounds(list, "i\xc3\xa9");
        assert(selected.w == 16);
        bool selection_paint = false, caret_paint = false;
        for (size_t i = 0; i < list.count; ++i) {
            cui_draw_command command = list.commands[i];
            if (command.type == CUI_DRAW_RECT && command.bounds.w == 16 &&
                !memcmp(&command.color, &themes[theme_index].primary, sizeof command.color))
                selection_paint = true;
            if (command.type == CUI_DRAW_RECT && command.bounds.w == 1 && command.bounds.h == 20)
                caret_paint = true;
        }
        assert(selection_paint && caret_paint);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "Q"}));
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(!strcmp(buffer, "WQx"));
        pointer(ui, CUI_EVENT_POINTER_DOWN, bounds.x + 1, bounds.y + 5, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        pointer(ui, CUI_EVENT_POINTER_MOVE, bounds.x + 40, bounds.y + 5, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        pointer(ui, CUI_EVENT_POINTER_UP, bounds.x + 40, bounds.y + 5, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(cui_edit_get_state(edit).anchor == 0 && cui_edit_get_state(edit).cursor == 3 &&
               !state.behavior.dragging);
        assert(cui_edit_insert(edit, cui_string("WWWWWWWWWWWW")) == CUI_EDIT_OK);
        options.width = 100;
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(state.scroll_x > 0 && isfinite(state.scroll_x));
        key(ui, CUI_KEY_HOME, 0);
        styled_frame(ui, edit, &themes[theme_index], &state, options);
        assert(state.scroll_x == 0 && cui_edit_get_state(edit).cursor == 0);
        cui_edit_destroy(edit);
        cui_destroy(ui);
    }
}
#endif
static void input_consumes_caret_keys(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    char buffer[32] = "abc";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_behavior_state state = {0};
    assert(ui && edit);
    base_frame(ui, edit, &state, (cui_base_input_options){0});
    cui_focus(ui, 1);
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    for (unsigned readonly = 0; readonly < 2; ++readonly) {
        key(ui, CUI_KEY_LEFT, 0);
        assert(cui_begin_frame(ui, 400, 200, .016f));
        assert(cui_begin_focus_scope(ui, 90, false));
        cui_base_input_ex(ui, 1, cui_string("Input"), edit, &state,
                          (cui_base_input_options){.readonly = readonly != 0});
        assert(cui_event_consumed(ui, 0));
        /* Even an arrow at the start is handled by the editor. */
        assert(cui_edit_get_state(edit).cursor == 0);
        cui_end_focus_scope(ui);
        cui_end_frame(ui);
    }
    key(ui, CUI_KEY_RIGHT, 0);
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(cui_edit_apply_events(edit, ui, 1, false) == CUI_EDIT_OK);
    assert(cui_event_consumed(ui, 0) && cui_edit_get_state(edit).cursor == 1);
    assert(cui_edit_apply_events(edit, ui, 1, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 1);
    cui_end_frame(ui);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void consumed_release_ends_selection(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    char buffer[32] = "abc";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    cui_input_behavior_state state = {0};
    assert(ui && edit);
    base_frame(ui, edit, &state, (cui_base_input_options){0});
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 10, .y = 10}));
    base_frame(ui, edit, &state, (cui_base_input_options){0});
    assert(state.dragging);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 20, .y = 10}));
    assert(cui_begin_frame(ui, 400, 200, .016f));
    assert(cui_consume_event(ui, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 40});
    cui_base_input_ex(ui, 1, cui_string("Input"), edit, &state, (cui_base_input_options){0});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(!state.dragging && !cui_pressed(ui, 1));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
int main(void) {
    consumed_release_ends_selection();
    input_consumes_caret_keys();
    clipboard_and_readonly();
#ifndef CUI_INPUT_BASE_ONLY
    paint_only_reduction();
    styled_selection();
#endif
    return 0;
}
