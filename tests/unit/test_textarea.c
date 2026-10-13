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
    key(ui, CUI_KEY_UP, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 0 && state.scroll_y == 0);
    key(ui, CUI_KEY_DOWN, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == strlen(buffer) && state.scroll_y == 40);
    key(ui, CUI_KEY_LEFT, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == (size_t)(strstr(buffer, "last") - buffer));
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
static void test_composition_navigation(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    char buffer[64] = "ab\ncd";
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_textarea_line lines[16];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 16));
    cui_textarea_options options = {.width = 180, .rows = 3};
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    assert(cui_edit_select(edit, 4, 4) == CUI_EDIT_OK);
    frame(ui, edit, &state, options);
    const cui_key keys[] = {CUI_KEY_UP,    CUI_KEY_DOWN,      CUI_KEY_HOME, CUI_KEY_END,
                            CUI_KEY_ENTER, CUI_KEY_BACKSPACE, CUI_KEY_TAB};
    for (unsigned flag = 0; flag < 2; flag++) {
        cui_edit_state before = cui_edit_get_state(edit);
        for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++)
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                   .key = keys[i],
                                                   .ime_composing = flag == 0,
                                                   .prefer_character_input = flag == 1}));
        cui_textarea_result result = frame(ui, edit, &state, options);
        cui_edit_state after = cui_edit_get_state(edit);
        assert(result.error == CUI_TEXTAREA_OK && after.cursor == before.cursor &&
               after.anchor == before.anchor && after.revision == before.revision &&
               !strcmp(buffer, "ab\ncd") && cui_focused(ui) == 10);
    }
    text(ui, "é");
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "ab\ncéd"));
    key(ui, CUI_KEY_UP, 0);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor < 3);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void assert_visible_line(cui_textarea_state *state, const char *line) {
    bool found = false;
    for (size_t i = 0; i < drawing.count; ++i) {
        const cui_draw_command *command = &drawing.commands[i];
        if (command->type == CUI_DRAW_TEXT && command->text.length == strlen(line) &&
            memcmp(command->text.data, line, command->text.length) == 0) {
            found = true;
            assert(command->bounds.y >= state->viewport.y - .1f);
            assert(command->bounds.y + command->bounds.h <=
                   state->viewport.y + state->viewport.h + 1);
        }
    }
    assert(found);
}
static void test_repeated_down_visible_rows(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    char buffer[256] =
        "Row 01\nRow 02\nRow 03\nRow 04\nRow 05\nRow 06\nRow 07\nRow 08\nRow 09\nRow 10";
    cui_edit *edit = editor(buffer, sizeof(buffer));
    cui_textarea_line lines[32];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 32));
    cui_textarea_options options = {.width = 180, .rows = 2};
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    cui_edit_select(edit, 0, 0);
    for (unsigned press = 0; press < 20; ++press) {
        key(ui, CUI_KEY_DOWN, 0);
        frame(ui, edit, &state, options);
        if (press >= 9) {
            assert(state.caret_line == 9 && state.scroll_y == state.maximum_y);
            assert_visible_line(&state, "Row 10");
        }
    }
    key(ui, CUI_KEY_HOME, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(state.scroll_y == 0);
    assert_visible_line(&state, "Row 01");
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
typedef struct {
    float width;
    unsigned calls;
    cui_text_style style;
} styled_probe;
static cui_size styled_measure(void *user, cui_str text, float size, cui_text_style style) {
    styled_probe *p = user;
    p->style = style;
    ++p->calls;
    float width = p->width * (style.family == 7 ? 2 : 1);
    return (cui_size){width * text.length, size};
}
static void test_complete_text_style(void) {
    styled_probe p = {.width = 5};
    cui_context *ui = cui_create((cui_config){
        .width = 400, .height = 300, .measure_styled = styled_measure, .measure_user = &p});
    char buffer[128] = "abcdefgh";
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_textarea_line lines[32];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 32));
    cui_textarea_options o = {.width = 62, .rows = 2};
    assert(frame(ui, edit, &state, o).error == CUI_TEXTAREA_OK && state.count == 1);
    o.text_style = (cui_text_style){
        .family = 7, .weight = CUI_TEXT_WEIGHT_BOLD, .italic = true, .monospace = true};
    assert(frame(ui, edit, &state, o).error == CUI_TEXTAREA_OK && state.count == 2);
    assert(state.lines[0].end == 4 && state.lines[0].width == 40);
    assert(state.text_style.family == 7 && p.style.italic && p.style.monospace &&
           p.style.weight == CUI_TEXT_WEIGHT_BOLD);
    assert(cui_textarea_hit_test(ui, edit, &state, state.viewport.x + 14, state.viewport.y + 1) ==
           1);
    bool found = false;
    for (size_t i = 0; i < drawing.count; ++i)
        if (drawing.commands[i].type == CUI_DRAW_TEXT) {
            assert(drawing.commands[i].text_style.family == 7 &&
                   drawing.commands[i].text_style.weight == CUI_TEXT_WEIGHT_BOLD &&
                   drawing.commands[i].text_style.italic &&
                   drawing.commands[i].text_style.monospace);
            found = true;
        }
    assert(found);
    o.no_wrap = true;
    cui_edit_select(edit, 0, 0);
    frame(ui, edit, &state, o);
    cui_edit_select(edit, 8, 8);
    cui_focus(ui, 10);
    frame(ui, edit, &state, o);
    assert(state.maximum_x == 41 && state.scroll_x == 41);
    p.width = 3;
    cui_text_metrics_changed(ui);
    frame(ui, edit, &state, o);
    assert(state.lines[0].width == 48 && state.maximum_x == 9 && state.scroll_x == 9);
    o.text_style.family = 0;
    frame(ui, edit, &state, o);
    assert(state.lines[0].width == 24 && state.scroll_x == 0);
    cui_textarea_options scaled = {.width = 400, .height = 200, .text_style = o.text_style};
    for (int size = CUI_DESIGN_XS; size <= CUI_DESIGN_9XL; ++size) {
        scaled.design_size = (cui_design_size)size;
        cui_design_metrics metrics;
        assert(cui_design_measure(scaled.design_size, 1, &metrics));
        assert(frame(ui, edit, &state, scaled).error == CUI_TEXTAREA_OK);
        assert(state.font_size == metrics.font_size && state.line_height == metrics.line_height);
        assert(state.viewport.w == 400 - 2 * (metrics.padding_x + 1));
        for (size_t i = 0; i < drawing.count; ++i)
            if (drawing.commands[i].type == CUI_DRAW_TEXT)
                assert(drawing.commands[i].font_size == metrics.font_size &&
                       drawing.commands[i].text_style.monospace);
    }
    o.text_style.weight = (cui_text_weight)99;
    cui_edit_state saved = cui_edit_get_state(edit);
    assert(frame(ui, edit, &state, o).error == CUI_TEXTAREA_INVALID);
    assert(cui_edit_get_state(edit).revision == saved.revision);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void test_fold_projection(void) {
    styled_probe p = {.width = 5};
    cui_context *ui = cui_create((cui_config){
        .width = 400, .height = 300, .measure_styled = styled_measure, .measure_user = &p});
    char buffer[128] = "abcd\r\nxx\r\nyy\r\nzz\r\nend";
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_textarea_line lines[16];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 16));
    state.viewport.h = 100;
    cui_textarea_fold folds[] = {{0, 3, true}};
    assert(cui_textarea_set_folds(&state, edit, folds, 1));
    cui_textarea_metrics metrics = {14, 20, 0, 0};
    assert(cui_textarea_layout_ex(ui, edit, &state, metrics, 10, true, (cui_text_style){0}) ==
           CUI_TEXTAREA_OK);
    assert(state.count == 5 && lines[0].start == 0 && lines[1].start == 2 && lines[2].start == 14 &&
           lines[3].start == 18 && lines[4].start == 20);
    assert(cui_textarea_hit_test(ui, edit, &state, 1, 41) == 14);
    folds[0].end_line = SIZE_MAX;
    assert(cui_textarea_layout_ex(ui, edit, &state, metrics, 10, true, (cui_text_style){0}) ==
           CUI_TEXTAREA_INVALID);
    folds[0].end_line = 3;
    cui_edit_select(edit, 6, 8);
    assert(cui_textarea_layout_ex(ui, edit, &state, metrics, 10, true, (cui_text_style){0}) ==
           CUI_TEXTAREA_OK);
    assert(!folds[0].collapsed && state.count == 7 && state.caret_line == 2);
    assert(cui_textarea_set_folds(&state, edit, NULL, 0));
    assert(!state.fold_count);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void test_grapheme_geometry(void) {
    styled_probe probe = {.width = 5};
    cui_context *ui = cui_create((cui_config){
        .width = 400, .height = 300, .measure_styled = styled_measure, .measure_user = &probe});
    const char *clusters[] = {"é", "👩🏽‍💻", "🇨🇭", "क्‍ष", "각", " ́"};
    for (size_t i = 0; i < sizeof clusters / sizeof clusters[0]; i++) {
        char buffer[128] = "A";
        strcat(buffer, clusters[i]);
        strcat(buffer, "Z");
        cui_edit *edit = editor(buffer, sizeof buffer);
        cui_textarea_line lines[32];
        cui_textarea_state state;
        assert(cui_textarea_state_init(&state, lines, 32));
        state.viewport = (cui_rect){0, 0, 100, 100};
        cui_textarea_metrics metrics = {14, 20, 0, 0};
        /* A cluster wider than the viewport occupies an overflowing row as a
         * whole; neither wrapping nor pointer hit testing splits it. */
        assert(cui_textarea_layout_ex(ui, edit, &state, metrics, 5, true, (cui_text_style){0}) ==
               CUI_TEXTAREA_OK);
        assert(state.count == 3);
        assert(lines[0].end == 1 && lines[1].start == 1);
        assert(lines[1].end == 1 + strlen(clusters[i]));
        for (size_t row = 0; row < state.count; row++) {
            assert(cui_grapheme_boundary(cui_edit_get_text(edit), lines[row].start));
            assert(cui_grapheme_boundary(cui_edit_get_text(edit), lines[row].end));
            for (int x = 0; x < 100; x++)
                assert(cui_grapheme_boundary(
                    cui_edit_get_text(edit),
                    cui_textarea_hit_test(ui, edit, &state, (float)x, row * 20.f + 1)));
        }
        assert(cui_textarea_layout_ex(ui, edit, &state, metrics, 100, false, (cui_text_style){0}) ==
               CUI_TEXTAREA_OK);
        for (int x = 0; x < 100; x++)
            assert(cui_grapheme_boundary(cui_edit_get_text(edit),
                                         cui_textarea_hit_test(ui, edit, &state, (float)x, 1)));
        cui_edit_destroy(edit);
    }
    char buffer[128] = "AéZ\nA👩🏽‍💻Z";
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_textarea_line lines[32];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 32));
    cui_textarea_options options = {.width = 220, .height = 100, .no_wrap = true};
    assert(frame(ui, edit, &state, options).error == CUI_TEXTAREA_OK);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_RIGHT, 0);
    assert(frame(ui, edit, &state, options).error == CUI_TEXTAREA_OK);
    assert(cui_edit_get_state(edit).cursor == 4);
    key(ui, CUI_KEY_DOWN, 0);
    assert(frame(ui, edit, &state, options).error == CUI_TEXTAREA_OK);
    assert(cui_grapheme_boundary(cui_edit_get_text(edit), cui_edit_get_state(edit).cursor));
    size_t emoji_end = strlen(buffer) - 1;
    assert(cui_edit_select(edit, emoji_end, emoji_end) == CUI_EDIT_OK);
    key(ui, CUI_KEY_LEFT, CUI_MOD_SHIFT);
    assert(frame(ui, edit, &state, options).error == CUI_TEXTAREA_OK);
    assert(cui_edit_get_state(edit).cursor == 7);
    assert(cui_edit_get_state(edit).anchor == emoji_end);
    key(ui, CUI_KEY_DELETE, 0);
    assert(frame(ui, edit, &state, options).error == CUI_TEXTAREA_OK);
    assert(strcmp(buffer, "AéZ\nAZ") == 0);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void test_vertical_selection_document_edges(void) {
    styled_probe probe = {.width = 5};
    cui_context *ui = cui_create((cui_config){
        .width = 400, .height = 300, .measure_styled = styled_measure, .measure_user = &probe});
    for (unsigned mode = 0; mode < 4; mode++) {
        char buffer[128];
        strcpy(buffer, mode == 0   ? "abcde\nfghij\nklmno"
                       : mode == 1 ? "abcdefghijklmnopq"
                       : mode == 2 ? "abcde\r\nfghij\r\nklmno"
                                   : "abcdefghijklmnopq");
        cui_edit *edit = editor(buffer, sizeof buffer);
        cui_textarea_line lines[32];
        cui_textarea_state state;
        assert(cui_textarea_state_init(&state, lines, 32));
        cui_textarea_options options = {
            .width = 40, .height = 100, .no_wrap = mode != 1, .readonly = mode == 3};
        assert(frame(ui, edit, &state, options).error == CUI_TEXTAREA_OK);
        uint64_t revision = cui_edit_get_state(edit).revision;
        cui_focus(ui, 10);
        assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
        key(ui, CUI_KEY_UP, 0);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == 1);
        key(ui, CUI_KEY_UP, CUI_MOD_SHIFT);
        assert(frame(ui, edit, &state, options).selection_changed);
        assert(cui_edit_get_state(edit).cursor == 0 && cui_edit_get_state(edit).anchor == 1);
        key(ui, CUI_KEY_UP, CUI_MOD_SHIFT);
        assert(!frame(ui, edit, &state, options).selection_changed);
        size_t last = state.lines[state.count - 1].start + 1;
        assert(last < strlen(buffer));
        assert(cui_edit_select(edit, last, last) == CUI_EDIT_OK);
        key(ui, CUI_KEY_DOWN, 0);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == last);
        key(ui, CUI_KEY_DOWN, CUI_MOD_SHIFT);
        assert(frame(ui, edit, &state, options).selection_changed);
        assert(cui_edit_get_state(edit).cursor == strlen(buffer));
        assert(cui_edit_get_state(edit).anchor == last);
        /* Selection orientation never changes the sticky anchor. */
        assert(cui_edit_select(edit, 0, last) == CUI_EDIT_OK);
        key(ui, CUI_KEY_DOWN, CUI_MOD_SHIFT);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == strlen(buffer));
        assert(cui_edit_get_state(edit).anchor == 0);
        assert(cui_edit_get_state(edit).revision == revision);
        assert(!cui_edit_get_state(edit).can_undo);
        options.disabled = true;
        assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
        key(ui, CUI_KEY_UP, CUI_MOD_SHIFT);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == 1 && cui_edit_get_state(edit).anchor == 1);
        cui_edit_destroy(edit);
    }
    char buffer[128] = "abcde\nfghij\nklmno\npqrst\nuvwxy";
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_textarea_line lines[32];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 32));
    cui_textarea_fold fold = {0, 4, true};
    assert(cui_textarea_set_folds(&state, edit, &fold, 1));
    cui_textarea_options options = {.width = 100, .height = 80, .no_wrap = true};
    frame(ui, edit, &state, options);
    assert(state.count == 2);
    cui_focus(ui, 10);
    cui_edit_select(edit, 1, 1);
    key(ui, CUI_KEY_UP, CUI_MOD_SHIFT);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 0 && fold.collapsed);
    cui_edit_select(edit, 25, 25);
    key(ui, CUI_KEY_DOWN, CUI_MOD_SHIFT);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == strlen(buffer) && fold.collapsed);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void test_unicode_word_keyboard(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    char buffer[128] = "one,  two";
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_textarea_line lines[16];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 16));
    cui_textarea_options options = {.width = 200, .no_wrap = true};
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    cui_edit_select(edit, 0, 0);
    key(ui, CUI_KEY_RIGHT, CUI_MOD_CTRL);
    assert(frame(ui, edit, &state, options).selection_changed);
    assert(cui_edit_get_state(edit).cursor == 3);
    key(ui, CUI_KEY_RIGHT, CUI_MOD_ALT | CUI_MOD_SHIFT);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 4 && cui_edit_get_state(edit).anchor == 3);
    key(ui, CUI_KEY_DELETE, CUI_MOD_CTRL);
    assert(frame(ui, edit, &state, options).changed);
    assert(!strcmp(buffer, "one  two"));
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "one,  two"));
    assert(cui_edit_get_state(edit).cursor == 4 && cui_edit_get_state(edit).anchor == 3);
    options.readonly = true;
    uint64_t revision = cui_edit_get_state(edit).revision;
    key(ui, CUI_KEY_BACKSPACE, CUI_MOD_CTRL);
    assert(!frame(ui, edit, &state, options).changed);
    assert(cui_edit_get_state(edit).revision == revision && !strcmp(buffer, "one,  two"));
    key(ui, CUI_KEY_RIGHT, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 9);
    key(ui, CUI_KEY_LEFT, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 6);
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void test_vertical_selection_collapse(void) {
    styled_probe probe = {.width = 5};
    cui_context *ui = cui_create((cui_config){
        .width = 400, .height = 300, .measure_styled = styled_measure, .measure_user = &probe});
    char buffer[128] = "abcde\nfghij\nklmno\npqrst";
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_textarea_line lines[32];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 32));
    cui_textarea_options options = {.width = 100, .height = 100, .no_wrap = true};
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    for (unsigned reversed = 0; reversed < 2; reversed++) {
        cui_edit_select(edit, reversed ? 15 : 8, reversed ? 8 : 15);
        frame(ui, edit, &state, options);
        key(ui, CUI_KEY_UP, 0);
        assert(frame(ui, edit, &state, options).selection_changed);
        assert(cui_edit_get_state(edit).cursor == 0 && cui_edit_get_state(edit).anchor == 0);
        assert(state.preferred_x == 0);
        key(ui, CUI_KEY_DOWN, 0);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == 6);
        cui_edit_select(edit, reversed ? 15 : 8, reversed ? 8 : 15);
        frame(ui, edit, &state, options);
        key(ui, CUI_KEY_DOWN, 0);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == 21 && cui_edit_get_state(edit).anchor == 21);
        assert(state.preferred_x == 15);
    }
    /* Exact frozen byte-biased Up calculation when selection begins at a
     * logical line start: floor(start-2), then move one visible row. */
    cui_edit_select(edit, 12, 16);
    frame(ui, edit, &state, options);
    key(ui, CUI_KEY_UP, 0);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 4);
    /* Page motion is explicitly non-collapsing in the frozen reference. */
    cui_edit_select(edit, 15, 8);
    frame(ui, edit, &state, options);
    key(ui, CUI_KEY_PAGEDOWN, 0);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 20 && cui_edit_get_state(edit).anchor == 20);
    options.readonly = true;
    cui_edit_select(edit, 15, 8);
    frame(ui, edit, &state, options);
    key(ui, CUI_KEY_DOWN, 0);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 21);
    options.disabled = true;
    cui_edit_select(edit, 8, 15);
    frame(ui, edit, &state, options);
    key(ui, CUI_KEY_UP, 0);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 15 && cui_edit_get_state(edit).anchor == 8);
    assert(cui_edit_get_state(edit).revision == 0 && !cui_edit_get_state(edit).can_undo);
    cui_edit_destroy(edit);
    strcpy(buffer, "abcdefghijklmnopqrst");
    edit = editor(buffer, sizeof buffer);
    assert(cui_textarea_state_init(&state, lines, 32));
    options = (cui_textarea_options){.width = 40, .height = 100};
    frame(ui, edit, &state, options);
    assert(state.count == 7 && lines[2].start == 6 && lines[4].start == 12);
    cui_focus(ui, 10);
    for (unsigned reversed = 0; reversed < 2; reversed++) {
        cui_edit_select(edit, reversed ? 13 : 8, reversed ? 8 : 13);
        frame(ui, edit, &state, options);
        key(ui, CUI_KEY_UP, 0);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == 3);
        cui_edit_select(edit, reversed ? 13 : 8, reversed ? 8 : 13);
        frame(ui, edit, &state, options);
        key(ui, CUI_KEY_DOWN, 0);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == 16);
        /* The end of a wrap row and start of the next share byte9. Only an
         * active endpoint with upstream affinity retains the previous row. */
        cui_edit_select(edit, reversed ? 9 : 4, reversed ? 4 : 9);
        frame(ui, edit, &state, options);
        state.upstream_affinity = !reversed;
        key(ui, CUI_KEY_DOWN, 0);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == 12 && cui_edit_get_state(edit).anchor == 12);
        assert(state.caret_line == (reversed ? 4 : 3));
        assert(state.upstream_affinity == !reversed);
        assert(state.preferred_x == (reversed ? 0 : 15));
    }
    cui_edit_destroy(edit);
    strcpy(buffer, "abcde\nfghij\nklmno\npqrst\nuvwxy\nzABCD");
    edit = editor(buffer, sizeof buffer);
    assert(cui_textarea_state_init(&state, lines, 32));
    cui_textarea_fold fold = {1, 4, true};
    assert(cui_textarea_set_folds(&state, edit, &fold, 1));
    options = (cui_textarea_options){.width = 100, .height = 100, .no_wrap = true};
    frame(ui, edit, &state, options);
    assert(state.count == 4);
    cui_focus(ui, 10);
    for (unsigned reversed = 0; reversed < 2; reversed++) {
        cui_edit_select(edit, reversed ? 26 : 24, reversed ? 24 : 26);
        frame(ui, edit, &state, options);
        key(ui, CUI_KEY_UP, 0);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == 5 && fold.collapsed);
        cui_edit_select(edit, reversed ? 26 : 24, reversed ? 24 : 26);
        frame(ui, edit, &state, options);
        key(ui, CUI_KEY_DOWN, 0);
        frame(ui, edit, &state, options);
        assert(cui_edit_get_state(edit).cursor == 32 && fold.collapsed);
    }
    assert(cui_edit_get_state(edit).revision == 0 && !cui_edit_get_state(edit).can_undo);
    cui_edit_destroy(edit);
    strcpy(buffer, "abcde\néXYZ\nabcde");
    edit = editor(buffer, sizeof buffer);
    assert(cui_textarea_state_init(&state, lines, 32));
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    cui_edit_select(edit, 9, 15);
    frame(ui, edit, &state, options);
    key(ui, CUI_KEY_UP, 0);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 0);
    cui_edit_select(edit, 0, 7); /* scalar endpoint inside e + combining mark */
    frame(ui, edit, &state, options);
    key(ui, CUI_KEY_DOWN, 0);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 16);
    assert(cui_grapheme_boundary(cui_edit_get_text(edit), cui_edit_get_state(edit).cursor));
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
static void test_atomic_inline_tokens(void) {
    styled_probe probe = {.width = 5};
    cui_context *ui = cui_create((cui_config){
        .width = 400, .height = 300, .measure_styled = styled_measure, .measure_user = &probe});
    char buffer[128] = "Hello @Mary Jane!";
    cui_edit *edit = editor(buffer, sizeof buffer);
    cui_edit_token_span span = {
        6, 16, {cui_string("mary"), cui_string("@Mary Jane"), cui_string("Mary")}};
    assert(cui_edit_set_tokens(edit, &span, 1) == CUI_EDIT_OK);
    cui_textarea_line lines[32];
    cui_textarea_state state;
    assert(cui_textarea_state_init(&state, lines, 32));
    cui_textarea_options options = {.width = 240, .no_wrap = true};
    assert(frame(ui, edit, &state, options).error == CUI_TEXTAREA_OK);
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(count == 1 && !semantics[0].details.handles_tab);
    assert(cui_textarea_hit_test(ui, edit, &state, state.viewport.x + 45, state.viewport.y + 1) ==
           6);
    cui_focus(ui, 10);
    cui_edit_select(edit, 6, 6);
    key(ui, CUI_KEY_RIGHT, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 16);
    key(ui, CUI_KEY_LEFT, CUI_MOD_CTRL | CUI_MOD_SHIFT);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 6 && cui_edit_get_state(edit).anchor == 16);
    key(ui, CUI_KEY_BACKSPACE, CUI_MOD_CTRL);
    assert(frame(ui, edit, &state, options).changed);
    assert(!strcmp(buffer, "Hello !") && cui_edit_get_content(edit).token_count == 0);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "Hello @Mary Jane!") && cui_edit_get_content(edit).token_count == 0);
    assert(cui_edit_set_tokens(edit, &span, 1) == CUI_EDIT_OK);
    options.readonly = true;
    cui_edit_select(edit, 6, 6);
    key(ui, CUI_KEY_RIGHT, 0);
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 16);
    key(ui, CUI_KEY_BACKSPACE, CUI_MOD_CTRL);
    assert(!frame(ui, edit, &state, options).changed);
    assert(cui_edit_get_content(edit).token_count == 1 && !strcmp(buffer, "Hello @Mary Jane!"));
    options.readonly = false;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = state.viewport.x + 45,
                                           .y = state.viewport.y + 1,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    frame(ui, edit, &state, options);
    assert(cui_edit_get_state(edit).cursor == 6 && cui_edit_get_state(edit).anchor == 6);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = state.viewport.x + 45,
                                           .y = state.viewport.y + 1,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    frame(ui, edit, &state, options);
    /* An inline atom remains one row even when its raw text is wider than the
     * viewport; no visual wrap boundary may split an annotation. */
    options.width = 40;
    options.no_wrap = false;
    frame(ui, edit, &state, options);
    bool whole = false;
    for (size_t i = 0; i < state.count; i++) {
        assert(!(lines[i].start > 6 && lines[i].start < 16));
        assert(!(lines[i].end > 6 && lines[i].end < 16));
        whole |= lines[i].start == 6 && lines[i].end == 16;
    }
    assert(whole);
    uint64_t revision = cui_edit_get_state(edit).revision;
    size_t cursor = cui_edit_get_state(edit).cursor;
    span.end = SIZE_MAX; /* simulated invalid borrowed range */
    key(ui, CUI_KEY_DELETE, CUI_MOD_CTRL);
    assert(frame(ui, edit, &state, options).error == CUI_TEXTAREA_INVALID);
    assert(cui_edit_get_state(edit).revision == revision &&
           cui_edit_get_state(edit).cursor == cursor);
    assert(!strcmp(buffer, "Hello @Mary Jane!"));
    semantics = cui_semantics(ui, &count);
    assert(count == 0);
    assert(cui_edit_set_tokens(edit, NULL, 0) == CUI_EDIT_OK);
    span.end = 16;
    cui_edit_destroy(edit);
    cui_destroy(ui);
}
int main(void) {
    test_atomic_inline_tokens();
    test_vertical_selection_collapse();
    test_unicode_word_keyboard();
    test_vertical_selection_document_edges();
    test_grapheme_geometry();
    test_fold_projection();
    test_complete_text_style();
    test_repeated_down_visible_rows();
    test_composition_navigation();
    test_tab_indent_and_focus();
    test_submit_and_horizontal();
    test_metrics_and_layout();
    test_editing_navigation_scroll();
    test_pointer_readonly_disabled();
    test_capacity_utf8_and_styles();
    return 0;
}
