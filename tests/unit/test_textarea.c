/* SPDX-License-Identifier: MIT */
#include "cui/cui_textarea.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_draw_list drawing;
static cui_textarea_result frame(cui_context *ui, cui_edit *edit, cui_textarea_state *state,
                                 cui_textarea_options options) {
    cui_theme theme = cui_theme_light();
    theme.caret = (cui_color){17, 91, 203, 255};
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_textarea_result result =
        cui_textarea(ui, 10, &theme, cui_string("Notes"), edit, state, options);
    drawing = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static cui_edit *editor(char *buffer, size_t capacity) {
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = capacity,
                                                       .multiline = true,
                                                       .history_bytes = 8192,
                                                       .max_history_steps = 32});
    assert(edit);
    return edit;
}
static void key(cui_context *ui, cui_key key, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key, .modifiers = modifiers}));
}
static void text(cui_context *ui, const char *string) {
    cui_event event = {.type = CUI_EVENT_TEXT};
    assert(strlen(string) < sizeof(event.text));
    memcpy(event.text, string, strlen(string) + 1);
    assert(cui_queue_event(ui, event));
}
static void test_metrics_and_layout(void) {
    cui_textarea_metrics metrics;
    assert(cui_textarea_measure(cui_textarea_default(), 16, &metrics));
    assert(metrics.font_size == 14 && metrics.line_height == 20 && metrics.padding_x == 10 &&
           metrics.padding_y == 8);
    assert(
        cui_textarea_measure((cui_textarea_options){.size = CUI_TEXTAREA_CUSTOM, .custom_size = 24},
                             16, &metrics) &&
        metrics.font_size == 21);
    assert(
        !cui_textarea_measure((cui_textarea_options){.size = CUI_TEXTAREA_CUSTOM}, 16, &metrics));
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    char buffer[128] = "ab\n\ncd\n";
    cui_edit *edit = editor(buffer, sizeof(buffer));
    cui_textarea_line lines[32];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 32));
    assert(frame(ui, edit, &state, (cui_textarea_options){.width = 180}).error == CUI_TEXTAREA_OK);
    assert(state.count == 4 && lines[0].start == 0 && lines[0].end == 2 && lines[0].next == 3);
    assert(lines[1].start == 3 && lines[1].end == 3 && lines[3].start == 7 && lines[3].end == 7);
    assert(cui_bounds(ui, 10).h == 38 && state.viewport.h == 20 && state.maximum_y == 60);
    cui_edit_select(edit, 0, cui_edit_get_text(edit).length);
    cui_edit_insert(edit, cui_string("abcdef"));
    assert(frame(ui, edit, &state,
                 (cui_textarea_options){.width = 47.2f, .auto_grow = true, .max_rows = 2})
               .error == CUI_TEXTAREA_OK);
    assert(state.count == 2 && lines[0].end == 3 && lines[1].start == 3 &&
           cui_bounds(ui, 10).h == 58);
    assert(cui_textarea_hit_test(ui, edit, &state, state.viewport.x + 100, state.viewport.y + 1) ==
               3 &&
           state.upstream_affinity);
    cui_edit_select(edit, 3, 3);
    cui_focus(ui, 10);
    frame(ui, edit, &state, (cui_textarea_options){.width = 47.2f, .rows = 2});
    /* A controlled cursor change resets affinity to the downstream row. */
    assert(state.caret_line == 1 && !state.upstream_affinity);
    key(ui, CUI_KEY_LEFT, 0);
    frame(ui, edit, &state, (cui_textarea_options){.width = 47.2f, .rows = 2});
    assert(cui_edit_get_state(edit).cursor == 3 && state.caret_line == 0 &&
           state.upstream_affinity);
    key(ui, CUI_KEY_RIGHT, 0);
    frame(ui, edit, &state, (cui_textarea_options){.width = 47.2f, .rows = 2});
    assert(cui_edit_get_state(edit).cursor == 3 && state.caret_line == 1);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void test_editing_navigation_scroll(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    char buffer[128] = "abcdef\nx\nabcdef\nlast";
    cui_edit *edit = editor(buffer, sizeof(buffer));
    cui_textarea_line lines[32];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 32));
    cui_textarea_options options = {.width = 180, .rows = 2};
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    cui_edit_select(edit, 5, 5);
    frame(ui, edit, &state, options);
    key(ui, CUI_KEY_DOWN, 0);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 8);
    key(ui, CUI_KEY_DOWN, CUI_MOD_SHIFT);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 14 && cui_edit_get_state(edit).anchor == 8 &&
           state.scroll_y == 20);
    key(ui, CUI_KEY_END, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == strlen(buffer) && state.scroll_y == 40);
    key(ui, CUI_KEY_ENTER, 0);
    text(ui, "new");
    cui_textarea_result result = frame(ui, edit, &state, options);
    assert(result.changed && strstr(buffer, "last\nnew") && state.count == 5 &&
           state.scroll_y == 60);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(strstr(buffer, "last\n") && !strstr(buffer, "new"));
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(!strstr(buffer, "last\n"));
    key(ui, CUI_KEY_Y, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(strstr(buffer, "last\n"));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = state.viewport.x + 1,
                                           .y = state.viewport.y + 1,
                                           .dy = 2}));
    assert(frame(ui, edit, &state, options).scrolled);
    assert(state.scroll_y < state.maximum_y);
    cui_edit_select(edit, 0, 0);
    cui_edit_insert(edit, cui_string("controlled\n"));
    result = frame(ui, edit, &state, options);
    assert(!result.changed && state.count == 6 &&
           state.revision == cui_edit_get_state(edit).revision);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
typedef struct {
    char bytes[128];
    unsigned writes;
} clipboard;
static bool copy(void *user, cui_str string) {
    clipboard *host = user;
    assert(string.length < sizeof(host->bytes));
    memcpy(host->bytes, string.data, string.length);
    host->bytes[string.length] = 0;
    host->writes++;
    return true;
}
static cui_str paste(void *user) {
    return cui_string(((clipboard *)user)->bytes);
}
static void test_pointer_readonly_disabled(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    char buffer[128] = "abcd\nefgh";
    cui_edit *edit = editor(buffer, sizeof(buffer));
    cui_textarea_line lines[32];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 32));
    clipboard host = {0};
    cui_platform platform = {.user = &host,
                             .capabilities = CUI_PLATFORM_CLIPBOARD,
                             .clipboard_set = copy,
                             .clipboard_get = paste};
    cui_textarea_options options = {
        .width = 180, .rows = 2, .readonly = true, .required = true, .clipboard = &platform};
    frame(ui, edit, &state, options);
    float x = state.viewport.x, y = state.viewport.y;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x + 8.4f,
                                           .y = y + 1}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x + 25.2f,
                                           .y = y + 21}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = x + 25.2f,
                                           .y = y + 21}));
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).anchor == 1 && cui_edit_get_state(edit).cursor == 8 &&
           cui_focused(ui) == 10 && !state.dragging);
    key(ui, CUI_KEY_C, CUI_MOD_CTRL);
    text(ui, "ignored");
    frame(ui, edit, &state, options);
    assert(host.writes == 1 && !strcmp(host.bytes, "bcd\nefg") && !strcmp(buffer, "abcd\nefgh"));
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 1 && sem[0].role == CUI_ROLE_TEXTBOX && sem[0].details.multiline &&
           sem[0].details.readonly && sem[0].details.required);
    bool highlight = false, caret = false;
    for (size_t i = 0; i < drawing.count; i++) {
        cui_draw_command command = drawing.commands[i];
        highlight |=
            command.type == CUI_DRAW_RECT && command.bounds.h == 20 && command.bounds.w > 1;
        caret |= command.type == CUI_DRAW_RECT && command.bounds.w == 1 && command.bounds.h == 20 &&
                 command.color.r == 17 && command.color.g == 91 && command.color.b == 203;
    }
    assert(highlight && caret);
    options.disabled = true;
    key(ui, CUI_KEY_A, CUI_MOD_CTRL);
    key(ui, CUI_KEY_BACKSPACE, 0);
    text(ui, "ignored");
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "abcd\nefgh") && cui_edit_get_state(edit).cursor == 8);
    sem = cui_semantics(ui, &n);
    assert(sem[0].disabled && !sem[0].focused);
    options.disabled = options.readonly = false;
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    strcpy(host.bytes, "one\ntwo");
    key(ui, CUI_KEY_V, CUI_MOD_CTRL);
    assert(frame(ui, edit, &state, options).changed && !strcmp(buffer, "aone\ntwoh"));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void test_capacity_utf8_and_styles(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    char buffer[64] = "\xc3\xa9\xf0\x9f\x98\x80\n";
    cui_edit *edit = editor(buffer, sizeof(buffer));
    cui_textarea_line lines[4];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 4));
    assert(frame(ui, edit, &state, (cui_textarea_options){.width = 30, .rows = 2}).error ==
           CUI_TEXTAREA_OK);
    assert(state.count == 3 && lines[0].end == 2 && lines[1].end == 6 && lines[2].start == 7);
    cui_focus(ui, 10);
    cui_edit_select(edit, 0, 0);
    key(ui, CUI_KEY_RIGHT, 0);
    frame(ui, edit, &state, (cui_textarea_options){.width = 30, .rows = 2});
    assert(cui_edit_get_state(edit).cursor == 2);
    state.capacity = 1;
    assert(frame(ui, edit, &state, (cui_textarea_options){.width = 30}).error ==
               CUI_TEXTAREA_CAPACITY &&
           !state.count);
    state.capacity = 4;
    cui_textarea_options options = {.width = 180,
                                    .no_wrap = true,
                                    .no_appearance = true,
                                    .no_border = true,
                                    .no_tab_stop = true,
                                    .custom_role = true,
                                    .role = CUI_ROLE_GROUP};
    assert(frame(ui, edit, &state, options).error == CUI_TEXTAREA_OK);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 1 && sem[0].role == CUI_ROLE_GROUP && sem[0].tab_index == -1);
    assert(frame(ui, edit, &state, (cui_textarea_options){.height = NAN}).error ==
           CUI_TEXTAREA_INVALID);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void test_submit_and_horizontal(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    char buffer[64] = "abcdefghij";
    cui_edit *edit = editor(buffer, sizeof(buffer));
    cui_textarea_line lines[16];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 16));
    cui_textarea_options options = {.width = 60, .no_wrap = true, .submit_on_enter = true};
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_END, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(state.scroll_x > 0 && state.scroll_x == state.maximum_x);
    key(ui, CUI_KEY_ENTER, CUI_MOD_CTRL);
    cui_textarea_result result = frame(ui, edit, &state, options);
    assert(result.pressed_enter && result.enter_secondary && !result.changed);
    assert(strcmp(buffer, "abcdefghij") == 0);
    key(ui, CUI_KEY_ENTER, CUI_MOD_SHIFT);
    result = frame(ui, edit, &state, options);
    assert(result.pressed_enter && result.enter_shift && result.changed && state.count == 2);
    assert(strcmp(buffer, "abcdefghij\n") == 0);
    options.readonly = true;
    key(ui, CUI_KEY_ENTER, 0);
    assert(!frame(ui, edit, &state, options).pressed_enter);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static cui_textarea_result focus_frame(cui_context *ui, cui_edit *edit, cui_textarea_state *state,
                                       cui_textarea_options options) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_textarea_result result =
        cui_textarea(ui, 10, &theme, cui_string("Notes"), edit, state, options);
    cui_interact(ui, 11, CUI_ROLE_BUTTON, cui_string("Next"), false, false, 0);
    cui_begin_box(ui, 11, (cui_box){.width = 60, .height = 24});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void test_tab_indent_and_focus(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    char buffer[64] = "a\nb";
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_textarea_line lines[16];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 16));
    cui_textarea_options options = {.width = 180, .rows = 2};
    focus_frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    assert(cui_edit_select(edit, 0, 3) == CUI_EDIT_OK);
    key(ui, CUI_KEY_TAB, 0);
    cui_textarea_result result = focus_frame(ui, edit, &state, options);
    assert(result.changed && !result.edit_error && cui_focused(ui) == 10 &&
           strcmp(buffer, "  a\n  b") == 0);
    assert(cui_edit_get_state(edit).anchor == 2 && cui_edit_get_state(edit).cursor == 7);
    key(ui, CUI_KEY_TAB, CUI_MOD_SHIFT);
    result = focus_frame(ui, edit, &state, options);
    assert(result.changed && cui_focused(ui) == 10 && strcmp(buffer, "a\nb") == 0);
    assert(cui_edit_get_state(edit).anchor == 0 && cui_edit_get_state(edit).cursor == 3);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB, .repeat = true}));
    result = focus_frame(ui, edit, &state, options);
    assert(result.changed && cui_focused(ui) == 10 && strcmp(buffer, "a  \nb") == 0);
    options.no_indent = true;
    focus_frame(ui, edit, &state, options);
    key(ui, CUI_KEY_TAB, 0);
    result = focus_frame(ui, edit, &state, options);
    assert(!result.changed && cui_focused(ui) == 11 && strcmp(buffer, "a  \nb") == 0);
    options.no_indent = false;
    options.readonly = true;
    focus_frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_TAB, 0);
    result = focus_frame(ui, edit, &state, options);
    assert(!result.changed && cui_focused(ui) == 11);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
int main(void) {
    test_tab_indent_and_focus();
    test_submit_and_horizontal();
    test_metrics_and_layout();
    test_editing_navigation_scroll();
    test_pointer_readonly_disabled();
    test_capacity_utf8_and_styles();
    return 0;
}
