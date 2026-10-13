/* SPDX-License-Identifier: MIT */
#include "cui/cui_editor.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    size_t calls;
    bool invalid;
} syntax_probe;
static size_t syntax(void *user, cui_str language, cui_str text, size_t start, size_t end,
                     cui_color *color) {
    syntax_probe *p = user;
    ++p->calls;
    assert(language.length == 1 && language.data[0] == 'c');
    assert(text.data && start < end);
    bool keyword = end - start >= 3 && !memcmp(text.data + start, "int", 3);
    *color = keyword ? (cui_color){250, 20, 30, 255} : (cui_color){20, 80, 190, 255};
    return p->invalid ? start : keyword ? start + 3 : end;
}
static cui_textarea_result draw(cui_context *ui, cui_editor_state *state, cui_editor_options o,
                                cui_draw_list *commands) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_textarea_result result = cui_editor(ui, 10, &theme, cui_string("Code"), state, o);
    *commands = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void editor_interactions(void) {
    char buffer[512] = "int x;\nfoo\nfoo\nlast";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 16});
    cui_textarea_line lines[64];
    cui_editor_state state;
    assert(edit && cui_editor_state_init(&state, edit, lines, 64));
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    syntax_probe p = {0};
    cui_editor_options o = cui_editor_default();
    assert(o.textarea.text_style.monospace);
    o.textarea.text_style.family = 7;
    o.textarea.text_style.weight = CUI_TEXT_WEIGHT_SEMIBOLD;
    o.textarea.text_style.italic = true;
    o.textarea.width = 200;
    o.textarea.height = 70;
    o.language = cui_string("c");
    o.highlight = syntax;
    o.highlight_user = &p;
    cui_draw_list commands;
    assert(draw(ui, &state, o, &commands).error == CUI_TEXTAREA_OK);
    assert(state.view.count == 4 && state.view.viewport.x > 20 && p.calls > 0);
    bool colored = false, numbered = false;
    for (size_t i = 0; i < commands.count; ++i) {
        const cui_draw_command *c = &commands.commands[i];
        if (c->type != CUI_DRAW_TEXT)
            continue;
        assert(c->text_style.family == 7 && c->text_style.weight == CUI_TEXT_WEIGHT_SEMIBOLD &&
               c->text_style.italic && c->text_style.monospace);
        if (c->color.r == 250 && c->color.g == 20)
            colored = true;
        if (c->text.length == 1 && c->text.data[0] == '1')
            numbered = true;
    }
    assert(colored && numbered);
    cui_focus(ui, 10);
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    assert(cui_editor_find(&state, cui_string("foo"), false, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 7 && cui_edit_get_state(edit).cursor == 10);
    assert(cui_editor_find(&state, cui_string("foo"), false, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 11);
    assert(cui_editor_find(&state, cui_string("foo"), false, false) == CUI_EDIT_BOUNDARY);
    assert(cui_editor_find(&state, cui_string("foo"), false, true) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 7);
    assert(cui_editor_find(&state, cui_string("foo"), true, true) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 11);
    draw(ui, &state, o, &commands);
    assert(state.view.scroll_y > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "bar"}));
    assert(draw(ui, &state, o, &commands).changed);
    assert(strstr(buffer, "foo\nbar"));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL}));
    assert(draw(ui, &state, o, &commands).changed && strstr(buffer, "foo\nfoo"));
    cui_edit_state before = cui_edit_get_state(edit);
    assert(cui_editor_find(&state, (cui_str){"\x80", 1}, false, true) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_get_state(edit).cursor == before.cursor);
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    draw(ui, &state, o, &commands);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = state.view.viewport.x + 1,
                                           .y = state.view.viewport.y + 21,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    draw(ui, &state, o, &commands);
    assert(cui_edit_get_state(edit).cursor == 7 && state.view.dragging);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = state.view.viewport.x + 1,
                                           .y = state.view.viewport.y + 21,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    draw(ui, &state, o, &commands);
    assert(!state.view.dragging);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = state.view.viewport.x + 1,
                                           .y = state.view.viewport.y + 1,
                                           .dy = -0.5f,
                                           .scroll_precise = true}));
    draw(ui, &state, o, &commands);
    assert(state.view.scroll_y > 0);
    o.textarea.readonly = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "oops"}));
    assert(!draw(ui, &state, o, &commands).changed);
    o.textarea.width = NAN;
    assert(draw(ui, &state, o, &commands).error == CUI_TEXTAREA_INVALID);
    assert(cui_edit_get_state(edit).revision == before.revision);
    o.textarea.width = 200;
    p.invalid = true;
    assert(draw(ui, &state, o, &commands).error == CUI_TEXTAREA_OK);
    for (size_t i = 0; i < commands.count; ++i)
        assert(commands.commands[i].type != CUI_DRAW_TEXT || commands.commands[i].color.r != 250);
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void base_and_utf8(void) {
    char buffer[64] = "é\n猫 é";
    cui_edit *edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    cui_textarea_line lines[16];
    cui_editor_state state;
    assert(cui_editor_state_init(&state, edit, lines, 16));
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    assert(cui_editor_find(&state, cui_string("é"), false, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 2);
    assert(cui_editor_find(&state, cui_string("é"), false, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 7);
    assert(cui_editor_find(&state, cui_string("é"), true, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 0);
    assert(cui_editor_find(&state, cui_string("é"), true, false) == CUI_EDIT_BOUNDARY);
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    state.view.viewport = (cui_rect){0, 0, 180, 80};
    cui_textarea_metrics metrics;
    cui_editor_options o = cui_editor_default();
    assert(cui_textarea_measure(o.textarea, 16, &metrics));
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_base_editor(ui, 10, cui_string("Code"), &state, o, metrics).error ==
           CUI_TEXTAREA_OK);
    assert(cui_end_frame(ui).count == 0);
    cui_editor_state saved = state;
    assert(!cui_editor_state_init(&state, NULL, lines, 16));
    assert(!memcmp(&saved, &state, sizeof state));
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
int main(void) {
    base_and_utf8();
    editor_interactions();
    puts("editor tests passed");
    return 0;
}
