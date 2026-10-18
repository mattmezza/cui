/* SPDX-License-Identifier: MIT */
#include "cui/cui_editor.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
static cui_editor_search_result search_frame(cui_context *ui, cui_editor_state *editor,
                                             cui_editor_search_state *search, bool disabled) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 600, 400, .016f));
    cui_editor_search_result result =
        cui_editor_find_bar(ui, 20, 10, &theme, editor, search, 500, disabled);
    cui_editor_options o = cui_editor_default();
    o.textarea.width = 500;
    assert(cui_editor(ui, 10, &theme, cui_string("Code"), editor, o).error == CUI_TEXTAREA_OK);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    if (result.closed) {
        cui_rect ime;
        assert(cui_focused(ui) == 10 && cui_text_input_rect(ui, &ime));
        assert(ime.w == 1 && ime.h > 0);
    }
    return result;
}
static void find_panel(void) {
    char document[128] = "one two one";
    char query[64] = "one";
    cui_edit *edit = cui_edit_create(
        (cui_edit_config){.buffer = document, .capacity = sizeof document, .multiline = true});
    cui_edit *q = cui_edit_create((cui_edit_config){.buffer = query, .capacity = sizeof query});
    cui_textarea_line lines[32];
    cui_editor_state editor;
    cui_editor_search_state search;
    assert(cui_editor_state_init(&editor, edit, lines, 32));
    assert(cui_editor_search_init(&search, q));
    assert(!cui_editor_search_init(&search, edit));
    assert(cui_editor_match_count(&editor, cui_string("one")) == 2);
    assert(cui_editor_match_count(&editor, cui_string("")) == 0);
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    search_frame(ui, &editor, &search, false);
    cui_focus(ui, 10);
    cui_edit_select(edit, 0, 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_F, .modifiers = CUI_MOD_CTRL}));
    cui_editor_search_result r = search_frame(ui, &editor, &search, false);
    cui_id field = 20 ^ UINT64_C(0x790a4100f05b190d);
    assert(search.open && cui_focused(ui) == field && r.matches == 2);
    assert(cui_edit_get_state(q).anchor == 0 && cui_edit_get_state(q).cursor == 3);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    r = search_frame(ui, &editor, &search, false);
    assert(r.navigated && cui_edit_get_state(edit).anchor == 0 && !r.query_changed);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    r = search_frame(ui, &editor, &search, false);
    assert(r.navigated && cui_edit_get_state(edit).anchor == 8);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .modifiers = CUI_MOD_SHIFT}));
    r = search_frame(ui, &editor, &search, false);
    assert(r.navigated && cui_edit_get_state(edit).anchor == 0);
    cui_rect next = cui_bounds(ui, 20 ^ UINT64_C(0x73bbf8e9cae70b45));
    assert(next.w > 0 && next.h > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = next.x + next.w / 2,
                                           .y = next.y + next.h / 2,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    search_frame(ui, &editor, &search, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = next.x + next.w / 2,
                                           .y = next.y + next.h / 2,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    r = search_frame(ui, &editor, &search, false);
    assert(r.navigated && cui_edit_get_state(edit).anchor == 8);
    cui_focus(ui, field);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "zzz"}));
    r = search_frame(ui, &editor, &search, false);
    assert(r.query_changed && r.matches == 0 && !strcmp(query, "zzz"));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    r = search_frame(ui, &editor, &search, false);
    assert(!r.navigated && r.error == CUI_EDIT_BOUNDARY);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    r = search_frame(ui, &editor, &search, false);
    assert(r.closed && !search.open && cui_focused(ui) == 10);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_F, .modifiers = CUI_MOD_CTRL}));
    search_frame(ui, &editor, &search, true);
    assert(!search.open);
    cui_destroy(ui);
    cui_edit_destroy(q);
    cui_edit_destroy(edit);
}
static void folding(void) {
    char buffer[256] = "open\none\ninner\ntwo\nend\nclose\ntail";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 512,
                                                       .max_history_steps = 8});
    cui_textarea_line lines[32];
    cui_editor_state state;
    assert(cui_editor_state_init(&state, edit, lines, 32));
    cui_editor_fold folds[] = {{0, 5, true}, {2, 4, true}};
    assert(cui_editor_set_folds(&state, folds, 2));
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_editor_options o = cui_editor_default();
    o.textarea.width = 200;
    o.textarea.height = 58;
    cui_draw_list commands;
    assert(draw(ui, &state, o, &commands).error == CUI_TEXTAREA_OK);
    assert(state.view.count == 3 && lines[0].start == 0 && lines[1].start == 23 &&
           lines[2].start == 29 && state.view.maximum_y == 20);
    assert(!strcmp(buffer, "open\none\ninner\ntwo\nend\nclose\ntail"));
    assert(cui_textarea_hit_test(ui, edit, &state.view, state.view.viewport.x + 1,
                                 state.view.viewport.y + 21) == 23);
    bool closing_number = false, fold_image = false;
    for (size_t i = 0; i < commands.count; ++i)
        if (commands.commands[i].type == CUI_DRAW_TEXT && commands.commands[i].text.length == 1 &&
            commands.commands[i].text.data[0] == '6')
            closing_number = true;
    for (size_t i = 0; i < commands.count; ++i) {
        if (commands.commands[i].type == CUI_DRAW_IMAGE)
            fold_image = true;
        if (commands.commands[i].type == CUI_DRAW_TEXT)
            assert(commands.commands[i].text.length != 1 ||
                   (commands.commands[i].text.data[0] != '+' &&
                    commands.commands[i].text.data[0] != '-'));
    }
    assert(closing_number && fold_image);
    size_t semantic_count;
    const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
    cui_id expand = 0;
    for (size_t i = 0; i < semantic_count; ++i)
        if (semantics[i].role == CUI_ROLE_BUTTON && semantics[i].name.length == 13 &&
            !memcmp(semantics[i].name.data, "Expand line 1", 13))
            expand = semantics[i].id;
    assert(expand);
    cui_rect control = cui_bounds(ui, expand);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = control.x + 4,
                                           .y = control.y + 4,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    draw(ui, &state, o, &commands);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = control.x + 4,
                                           .y = control.y + 4,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    draw(ui, &state, o, &commands);
    assert(!folds[0].collapsed);
    draw(ui, &state, o, &commands);
    assert(state.view.count == 6);
    assert(cui_editor_set_folded(&state, 0, true) == CUI_EDIT_OK);

    cui_edit_select(edit, 0, 0);
    cui_focus(ui, 10);
    draw(ui, &state, o, &commands);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    draw(ui, &state, o, &commands);
    assert(cui_edit_get_state(edit).cursor == 23);
    assert(cui_editor_set_folded(&state, 0, false) == CUI_EDIT_OK);
    draw(ui, &state, o, &commands);
    assert(state.view.count == 6 && folds[1].collapsed);
    cui_edit_select(edit, 16, 17);
    assert(cui_editor_set_folded(&state, 0, true) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 4 && cui_edit_get_state(edit).anchor == 4);
    draw(ui, &state, o, &commands);
    assert(state.view.count == 3);
    cui_edit_select(edit, 0, 0);
    assert(cui_editor_find(&state, cui_string("one"), false, false) == CUI_EDIT_OK);
    draw(ui, &state, o, &commands);
    assert(!folds[0].collapsed && state.view.count == 6);
    cui_edit_select(edit, 16, 17);
    draw(ui, &state, o, &commands);
    assert(!folds[1].collapsed && state.view.count == 7);
    cui_editor_fold invalid[] = {{0, 99, true}};
    cui_textarea_fold *saved = state.view.folds;
    assert(!cui_editor_set_folds(&state, invalid, 1) && state.view.folds == saved);
    cui_editor_fold duplicate[] = {{0, 5, false}, {0, 4, false}};
    assert(!cui_editor_set_folds(&state, duplicate, 2));
    assert(cui_editor_set_folded(&state, 99, true) == CUI_EDIT_INVALID_ARGUMENT);
    cui_edit_select(edit, 0, cui_edit_get_text(edit).length);
    assert(cui_editor_set_folded(&state, 0, true) == CUI_EDIT_OK);
    draw(ui, &state, o, &commands);
    assert(state.view.count == 3 && cui_edit_get_state(edit).anchor == 0 &&
           cui_edit_get_state(edit).cursor == cui_edit_get_text(edit).length);
    cui_edit_select(edit, 4, 4);
    assert(cui_edit_insert(edit, cui_string("!")) == CUI_EDIT_OK);
    draw(ui, &state, o, &commands);
    assert(state.view.fold_count == 0 && state.view.count == 7);
    assert(cui_editor_set_folded(&state, 0, false) == CUI_EDIT_INVALID_ARGUMENT);
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void narrow_editor_clipping(void) {
    char buffer[1024];
    memset(buffer, 'W', sizeof buffer - 1);
    buffer[sizeof buffer - 1] = 0;
    cui_edit *edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    cui_textarea_line lines[8];
    cui_editor_state state;
    assert(cui_editor_state_init(&state, edit, lines, 8));
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 300});
    cui_editor_options o = cui_editor_default();
    o.textarea.width = 252;
    o.textarea.height = 100;
    cui_draw_list commands;
    for (unsigned frame = 0; frame < 3; ++frame) {
        assert(draw(ui, &state, o, &commands).error == CUI_TEXTAREA_OK);
        cui_rect outer = cui_bounds(ui, 10);
        assert(outer.w == 252 && state.view.viewport.w < 252 && state.view.maximum_x > 252);
        cui_rect clips[16];
        size_t depth = 0;
        bool text = false;
        for (size_t i = 0; i < commands.count; ++i) {
            cui_draw_command c = commands.commands[i];
            if (c.type == CUI_DRAW_CLIP_PUSH) {
                assert(depth < 16);
                clips[depth++] = c.bounds;
            } else if (c.type == CUI_DRAW_CLIP_POP) {
                assert(depth);
                --depth;
            } else if (c.type == CUI_DRAW_TEXT) {
                float left = c.bounds.x, right = c.bounds.x + c.bounds.w;
                for (size_t k = 0; k < depth; ++k) {
                    left = fmaxf(left, clips[k].x);
                    right = fminf(right, clips[k].x + clips[k].w);
                }
                assert(right <= outer.x + 252 && left >= outer.x);
                if (c.text.length > 100)
                    text = true;
            }
        }
        assert(text && depth == 0);
    }
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void completion_model(void) {
    char buffer[32] = "épr()";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 8});
    cui_textarea_line lines[16];
    cui_editor_state state;
    assert(cui_editor_state_init(&state, edit, lines, 16));
    cui_editor_completion_item items[] = {
        {cui_string("print"), cui_string("function"), cui_string("print")}};
    cui_edit_select(edit, 2, 4);
    assert(cui_editor_show_completions(&state, items, 1, 2, 4) == CUI_EDIT_OK);
    cui_editor_completion_state saved = state.completion;
    assert(cui_editor_show_completions(&state, items, 1, 1, 4) == CUI_EDIT_INVALID_ARGUMENT);
    assert(!memcmp(&saved, &state.completion, sizeof saved));
    cui_editor_completion_item invalid[] = {{cui_string("bad"), {0}, {"\x80", 1}}};
    assert(cui_editor_show_completions(&state, invalid, 1, 2, 4) == CUI_EDIT_INVALID_UTF8);
    assert(!memcmp(&saved, &state.completion, sizeof saved));
    assert(cui_editor_accept_completion(&state) == CUI_EDIT_OK && !state.completion.open);
    assert(!strcmp(buffer, "éprint()") && cui_edit_get_state(edit).cursor == 7);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "épr()"));
    assert(cui_edit_get_state(edit).anchor == 2 && cui_edit_get_state(edit).cursor == 4);
    assert(cui_editor_show_completions(&state, items, 1, 2, 4) == CUI_EDIT_OK);
    assert(cui_edit_insert(edit, cui_string("changed")) == CUI_EDIT_OK);
    char changed[32];
    strcpy(changed, buffer);
    assert(cui_editor_accept_completion(&state) == CUI_EDIT_INVALID_ARGUMENT);
    assert(!state.completion.open && !strcmp(buffer, changed));
    cui_editor_completion_item large[] = {
        {cui_string("large"),
         {0},
         cui_string("this completion replacement cannot fit into thirty two bytes")}};
    assert(cui_editor_show_completions(&state, large, 1, 2, 9) == CUI_EDIT_OK);
    cui_edit_state before = cui_edit_get_state(edit);
    assert(cui_editor_accept_completion(&state) == CUI_EDIT_CAPACITY);
    cui_edit_state after = cui_edit_get_state(edit);
    assert(state.completion.open && before.revision == after.revision &&
           before.cursor == after.cursor && before.anchor == after.anchor &&
           before.can_undo == after.can_undo && before.can_redo == after.can_redo &&
           !strcmp(buffer, changed));
    cui_editor_cancel_completion(&state);
    assert(!state.completion.items && !state.completion.count);
    cui_edit_destroy(edit);
}
static void completion_popup(void) {
    char buffer[256] = "pri()";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 4096,
                                                       .max_history_steps = 16});
    cui_textarea_line lines[32];
    cui_editor_state state;
    assert(cui_editor_state_init(&state, edit, lines, 32));
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_editor_options o = cui_editor_default();
    o.textarea.width = 300;
    o.textarea.height = 200;
    o.textarea.text_style.family = 7;
    o.completion_rows = 3;
    o.completion_width = 240;
    cui_draw_list commands;
    draw(ui, &state, o, &commands);
    cui_focus(ui, 10);
    cui_edit_select(edit, 3, 3);
    char labels[10][16];
    cui_editor_completion_item items[10];
    for (size_t i = 0; i < 10; ++i) {
        snprintf(labels[i], sizeof labels[i], "print%zu", i);
        items[i] = (cui_editor_completion_item){cui_string(labels[i]), cui_string("function"),
                                                cui_string(labels[i])};
    }
    assert(cui_editor_show_completions(&state, items, 10, 0, 3) == CUI_EDIT_OK);
    draw(ui, &state, o, &commands);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    unsigned options = 0;
    bool list = false;
    for (size_t i = 0; i < count; ++i) {
        if (sem[i].role == CUI_ROLE_OPTION) {
            ++options;
            assert(sem[i].details.set_size == 10 && sem[i].details.no_focus);
        }
        if (sem[i].role == CUI_ROLE_LISTBOX) {
            list = true;
            cui_rect bounds = cui_bounds(ui, sem[i].id);
            assert(bounds.x >= state.view.viewport.x && bounds.y >= state.view.viewport.y &&
                   bounds.x + bounds.w <= state.view.viewport.x + state.view.viewport.w &&
                   bounds.y + bounds.h <= state.view.viewport.y + state.view.viewport.h);
        }
    }
    assert(options == 3 && list && state.completion.selected == 0);
    o.textarea.width = 180;
    draw(ui, &state, o, &commands);
    assert(state.completion.open);
    sem = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (sem[i].role == CUI_ROLE_LISTBOX)
            assert(cui_bounds(ui, sem[i].id).w <= state.view.viewport.w);
    o.textarea.width = 300;
    draw(ui, &state, o, &commands);

    for (size_t i = 0; i < commands.count; ++i)
        if (commands.commands[i].type == CUI_DRAW_TEXT)
            assert(commands.commands[i].text_style.family == 7);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    draw(ui, &state, o, &commands);
    assert(state.completion.selected == 9 && cui_edit_get_state(edit).cursor == 3);
    assert(cui_bounds(ui, cui_editor_completion_item_id(10, 9)).h > 0);
    assert(cui_bounds(ui, cui_editor_completion_item_id(10, 0)).h == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    draw(ui, &state, o, &commands);
    assert(state.completion.selected == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    draw(ui, &state, o, &commands);
    assert(state.completion.selected == 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    cui_textarea_result r = draw(ui, &state, o, &commands);
    assert(r.changed && !state.completion.open && !strcmp(buffer, "print1()"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "pri()"));
    assert(cui_editor_show_completions(&state, items, 10, 0, 3) == CUI_EDIT_OK);
    draw(ui, &state, o, &commands);
    cui_rect second = cui_bounds(ui, cui_editor_completion_item_id(10, 1));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = second.x + 8,
                                           .y = second.y + 8,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    draw(ui, &state, o, &commands);
    assert(state.completion.open && cui_focused(ui) == 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = second.x + 8,
                                           .y = second.y + 8,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    r = draw(ui, &state, o, &commands);
    assert(r.changed && !strcmp(buffer, "print1()") && !state.completion.open);
    cui_edit_undo(edit);
    cui_editor_show_completions(&state, items, 10, 0, 3);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    draw(ui, &state, o, &commands);
    assert(!state.completion.open && !strcmp(buffer, "pri()"));
    cui_editor_show_completions(&state, items, 10, 0, 3);
    cui_edit_select(edit, 2, 2);
    draw(ui, &state, o, &commands);
    assert(!state.completion.open);
    cui_edit_select(edit, 3, 3);
    cui_editor_show_completions(&state, items, 10, 0, 3);
    cui_focus(ui, 10);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT, .modifiers = CUI_MOD_CTRL}));
    draw(ui, &state, o, &commands);
    assert(!state.completion.open && cui_edit_get_state(edit).cursor == 4);
    assert(!strcmp(buffer, "pri()"));
    cui_edit_select(edit, 3, 3);
    cui_editor_show_completions(&state, items, 10, 0, 3);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DELETE, .modifiers = CUI_MOD_CTRL}));
    assert(draw(ui, &state, o, &commands).changed);
    assert(!state.completion.open && !strcmp(buffer, "pri)"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "pri()") && cui_edit_get_state(edit).cursor == 3);
    cui_editor_show_completions(&state, items, 10, 0, 3);
    o.textarea.readonly = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    draw(ui, &state, o, &commands);
    assert(state.completion.open && !strcmp(buffer, "pri()"));
    o.textarea.readonly = false;
    o.completion_width = NAN;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(draw(ui, &state, o, &commands).error == CUI_TEXTAREA_INVALID);
    assert(state.completion.open && !strcmp(buffer, "pri()"));
    o.completion_width = 240;
    draw(ui, &state, o, &commands);
    cui_focus(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "x"}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    draw(ui, &state, o, &commands);
    assert(!state.completion.open && !strcmp(buffer, "prix\n()"));
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void inline_tokens_reject_code_mode(void) {
    char buffer[64] = "Hi @Mary Jane!";
    cui_edit *edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    assert(edit);
    cui_edit_token_span span = {
        3, 13, {cui_string("mary"), cui_string("@Mary Jane"), cui_string("Mary")}};
    cui_editor_state state;
    cui_textarea_line lines[16];
    assert(cui_editor_state_init(&state, edit, lines, 16));
    assert(cui_edit_set_tokens(edit, &span, 1) == CUI_EDIT_OK);
    cui_editor_state untouched = state;
    assert(!cui_editor_state_init(&state, edit, lines, 16));
    assert(!memcmp(&state, &untouched, sizeof state));
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_theme theme = cui_theme_light();
    assert(cui_editor(ui, 10, &theme, cui_string("Code"), &state, cui_editor_default()).error ==
           CUI_TEXTAREA_INVALID);
    assert(cui_base_editor(ui, 10, cui_string("Code"), &state, cui_editor_default(),
                           (cui_textarea_metrics){14, 20, 10, 8})
               .error == CUI_TEXTAREA_INVALID);
    cui_end_frame(ui);
    size_t count;
    cui_semantics(ui, &count);
    assert(count == 0 && !strcmp(buffer, "Hi @Mary Jane!"));
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void replace_workflow(void) {
    char buffer[64] = "one oneone \xc3\xa9";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 16});
    cui_textarea_line lines[16];
    cui_editor_state state;
    assert(edit && cui_editor_state_init(&state, edit, lines, 16));
    char scratch[128];
    size_t count = 99;
    cui_edit_select(edit, 0, 3);
    assert(cui_editor_replace_current(&state, cui_string("two"), cui_string("x")) ==
           CUI_EDIT_BOUNDARY);
    assert(cui_edit_get_state(edit).cursor == 3 && !strcmp(buffer, "one oneone \xc3\xa9"));
    assert(cui_editor_replace_current(&state, cui_string("one"), cui_string("two")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "two oneone \xc3\xa9"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "one oneone \xc3\xa9"));
    uint64_t revision = cui_edit_get_state(edit).revision;
    assert(cui_editor_replace_all(&state, cui_string("one"), cui_string("abc"), scratch, 3,
                                  &count) == CUI_EDIT_CAPACITY);
    assert(count == 99 && cui_edit_get_state(edit).revision == revision);
    assert(cui_editor_replace_all(&state, cui_string("one"), cui_string("x"), buffer, sizeof buffer,
                                  &count) == CUI_EDIT_INVALID_ARGUMENT);
    state.view.scroll_x = 20;
    state.view.scroll_y = 30;
    assert(cui_editor_replace_all(&state, cui_string("one"), cui_string("x"), scratch,
                                  sizeof scratch, &count) == CUI_EDIT_OK);
    assert(count == 3 && !strcmp(buffer, "x xx \xc3\xa9") && !state.view.scroll_x &&
           !state.view.scroll_y);
    assert(cui_edit_get_state(edit).cursor == 0 && cui_edit_get_state(edit).anchor == 0);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "one oneone \xc3\xa9"));
    assert(cui_edit_redo(edit) == CUI_EDIT_OK && !strcmp(buffer, "x xx \xc3\xa9"));
    revision = cui_edit_get_state(edit).revision;
    assert(cui_editor_replace_all(&state, cui_string("none"), cui_string(""), scratch,
                                  sizeof scratch, &count) == CUI_EDIT_OK);
    assert(count == 0 && cui_edit_get_state(edit).revision == revision);
    assert(cui_editor_replace_all(&state, cui_string("x"), cui_string(""), scratch, sizeof scratch,
                                  &count) == CUI_EDIT_OK);
    assert(count == 3 && !strcmp(buffer, "  \xc3\xa9"));
    cui_edit_destroy(edit);
}
static void retained_matcher(void) {
    char buffer[128] = "foo FOO foo \xc3\x89 \xc3\xa9";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 16});
    cui_textarea_line lines[32];
    cui_editor_state editor;
    assert(edit && cui_editor_state_init(&editor, edit, lines, 32));
    cui_editor_match ranges[8];
    cui_editor_matcher matcher;
    assert(cui_editor_matcher_init(&matcher, ranges, 8));
    assert(cui_editor_matcher_set_query(&editor, &matcher, cui_string("foo"), true) == CUI_EDIT_OK);
    assert(matcher.count == 3 && matcher.current == 0 && ranges[1].start == 4);
    assert(cui_editor_matcher_navigate(&editor, &matcher, false) == CUI_EDIT_OK);
    assert(matcher.current == 1 && cui_edit_get_state(edit).anchor == 4 &&
           cui_edit_get_state(edit).cursor == 7);
    assert(cui_editor_matcher_navigate(&editor, &matcher, true) == CUI_EDIT_OK &&
           matcher.current == 0);
    assert(cui_editor_matcher_navigate(&editor, &matcher, true) == CUI_EDIT_OK &&
           matcher.current == 2);
    assert(cui_editor_matcher_navigate(&editor, &matcher, false) == CUI_EDIT_OK &&
           matcher.current == 0);
    assert(cui_editor_matcher_anchor(&editor, &matcher, 7) == CUI_EDIT_OK && matcher.current == 1);
    char scratch[256];
    assert(cui_editor_matcher_replace_current(&editor, &matcher, cui_string("bar"), scratch,
                                              sizeof scratch) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "foo bar foo \xc3\x89 \xc3\xa9") && matcher.count == 2 &&
           matcher.current == 1);
    assert(cui_editor_matcher_replace_current(&editor, &matcher, cui_string("bar"), scratch,
                                              sizeof scratch) == CUI_EDIT_OK);
    assert(matcher.count == 1 && matcher.current == 0);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    assert(cui_editor_matcher_sync(&editor, &matcher) == CUI_EDIT_OK && matcher.count == 2 &&
           matcher.current == 0);
    cui_editor_match saved[8];
    memcpy(saved, ranges, sizeof ranges);
    cui_editor_matcher snapshot = matcher;
    assert(cui_editor_matcher_replace_current(&editor, &matcher,
                                              cui_string("foo foo foo foo foo foo foo foo foo"),
                                              scratch, sizeof scratch) == CUI_EDIT_CAPACITY);
    assert(!memcmp(saved, ranges, sizeof ranges) && !memcmp(&snapshot, &matcher, sizeof matcher));
    assert(cui_editor_matcher_set_query(&editor, &matcher, cui_string("foo"), false) ==
               CUI_EDIT_OK &&
           matcher.count == 2);
    assert(cui_editor_matcher_set_query(&editor, &matcher, cui_string("\xc3\xa9"), true) ==
               CUI_EDIT_OK &&
           matcher.count == 1);
    cui_editor_matcher_close(&matcher);
    assert(!matcher.active);
    assert(cui_editor_matcher_sync(&editor, &matcher) == CUI_EDIT_OK && !matcher.active);
    assert(cui_editor_matcher_set_query(&editor, &matcher, cui_string(""), true) == CUI_EDIT_OK &&
           !matcher.count);
    assert(cui_editor_matcher_navigate(&editor, &matcher, false) == CUI_EDIT_BOUNDARY);
    assert(cui_editor_matcher_set_query(&editor, &matcher, cui_edit_get_text(edit), true) ==
           CUI_EDIT_INVALID_ARGUMENT);
    cui_editor_matcher small;
    cui_editor_match one[1];
    assert(cui_editor_matcher_init(&small, one, 1));
    snapshot = small;
    assert(cui_editor_matcher_set_query(&editor, &small, cui_string("foo"), false) ==
           CUI_EDIT_CAPACITY);
    assert(!memcmp(&snapshot, &small, sizeof small));
    cui_edit_destroy(edit);
}
static void replacement_panel(void) {
    char buffer[128] = "foo FOO foo", query[32] = "foo", replacement[32] = "bar", scratch[256];
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 16});
    cui_edit *q = cui_edit_create((cui_edit_config){.buffer = query, .capacity = sizeof query});
    cui_edit *r =
        cui_edit_create((cui_edit_config){.buffer = replacement, .capacity = sizeof replacement});
    cui_textarea_line lines[32];
    cui_editor_state editor;
    cui_editor_search_state search;
    cui_editor_match ranges[16];
    cui_editor_matcher matcher;
    assert(edit && q && r && cui_editor_state_init(&editor, edit, lines, 32));
    assert(cui_editor_search_init(&search, q) && cui_editor_matcher_init(&matcher, ranges, 16));
    assert(!cui_editor_search_enable_replace(&search, &matcher, q, scratch, sizeof scratch));
    assert(cui_editor_search_enable_replace(&search, &matcher, r, scratch, sizeof scratch));
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    search_frame(ui, &editor, &search, false);
    cui_focus(ui, 10);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_H, .modifiers = CUI_MOD_CTRL}));
    cui_editor_search_result result = search_frame(ui, &editor, &search, false);
    assert(search.open && search.replace_mode && result.matches == 3 && matcher.case_insensitive);
    cui_id field = 20 ^ UINT64_C(0x1b4a6c72d005ab81);
    assert(cui_bounds(ui, field).w > 0);
    cui_focus(ui, field);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    result = search_frame(ui, &editor, &search, false);
    assert(result.replaced == 1 && !strcmp(buffer, "bar FOO foo") && matcher.count == 2 &&
           matcher.current == 0);
    search.replace_disabled = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    result = search_frame(ui, &editor, &search, false);
    assert(!result.replaced && !strcmp(buffer, "bar FOO foo"));
    search.replace_disabled = false;
    search_frame(ui, &editor, &search, false);
    cui_rect bounds = cui_bounds(ui, 20 ^ UINT64_C(0x4891147fe2443891));
    assert(bounds.w > 0 && bounds.h > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = bounds.x + bounds.w / 2,
                                           .y = bounds.y + bounds.h / 2,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    search_frame(ui, &editor, &search, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = bounds.x + bounds.w / 2,
                                           .y = bounds.y + bounds.h / 2,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    result = search_frame(ui, &editor, &search, false);
    assert(result.replaced == 2 && !strcmp(buffer, "bar bar bar") && !matcher.count);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "bar FOO foo"));
    search_frame(ui, &editor, &search, false);
    cui_theme theme = cui_theme_light();
    theme.buttons[CUI_THEME_BUTTON_PRIMARY].active = (cui_color){7, 8, 9, 255};
    theme.buttons[CUI_THEME_BUTTON_PRIMARY].normal = (cui_color){17, 18, 19, 255};
    assert(cui_begin_frame(ui, 400, 400, .016f));
    assert(cui_editor_find_bar(ui, 20, 10, &theme, &editor, &search, 330, false).error ==
           CUI_EDIT_OK);
    cui_editor_options narrow = cui_editor_default();
    narrow.textarea.width = 330;
    narrow.search = &matcher;
    assert(cui_editor(ui, 10, &theme, cui_string("Code"), &editor, narrow).error ==
           CUI_TEXTAREA_OK);
    cui_draw_list narrow_commands = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_rect mode = cui_bounds(ui, 20 ^ UINT64_C(0x43b926eab194d100));
    cui_rect match_case = cui_bounds(ui, 20 ^ UINT64_C(0x51a814631c060f34));
    bool mode_selected = false, case_unselected = false;
    for (size_t i = 0; i < narrow_commands.count; ++i) {
        cui_draw_command c = narrow_commands.commands[i];
        if (c.type == CUI_DRAW_RECT && c.bounds.x == mode.x && c.bounds.y == mode.y &&
            c.color.r == 7 && c.color.g == 8)
            mode_selected = true;
        if (c.type == CUI_DRAW_RECT && c.bounds.x == match_case.x && c.bounds.y == match_case.y &&
            c.color.r == 17 && c.color.g == 18)
            case_unselected = true;
    }
    assert(mode_selected && case_unselected);
    cui_rect panel = cui_bounds(ui, 20);
    uint64_t controls[] = {
        UINT64_C(0x790a4100f05b190d), UINT64_C(0x1b4a6c72d005ab81), UINT64_C(0x391cfd020af58d20),
        UINT64_C(0x73bbf8e9cae70b45), UINT64_C(0x0092b880a17f63dd), UINT64_C(0x51a814631c060f34),
        UINT64_C(0x43b926eab194d100), UINT64_C(0x311b89208297ba14), UINT64_C(0x4891147fe2443891)};
    for (size_t i = 0; i < sizeof controls / sizeof controls[0]; ++i) {
        cui_rect bounds = cui_bounds(ui, 20 ^ controls[i]);
        assert(bounds.w > 0 && bounds.h > 0 && bounds.x >= panel.x - .01f &&
               bounds.x + bounds.w <= panel.x + 330.01f);
    }
    assert(cui_bounds(ui, 20 ^ controls[0]).w >= 20);
    cui_focus(ui, field);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    result = search_frame(ui, &editor, &search, false);
    assert(result.closed && !search.open && !matcher.active && cui_focused(ui) == 10);
    cui_destroy(ui);
    cui_edit_destroy(r);
    cui_edit_destroy(q);
    cui_edit_destroy(edit);
}
static cui_size search_measure(void *user, cui_str text, float size, cui_text_style style) {
    (void)user;
    return (cui_size){text.length * (style.family == 7 ? 9.0f : 5.0f), size};
}
static void search_activation_and_paint(void) {
    char buffer[128] = "foo\nfoo\nfoo\nfoo", query[32] = "old";
    cui_edit *edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    cui_edit *q = cui_edit_create((cui_edit_config){.buffer = query, .capacity = sizeof query});
    cui_textarea_line lines[32];
    cui_editor_state editor;
    cui_editor_search_state search;
    cui_editor_match ranges[16];
    cui_editor_matcher matcher;
    assert(edit && q && cui_editor_state_init(&editor, edit, lines, 32));
    assert(cui_editor_search_init(&search, q) && cui_editor_matcher_init(&matcher, ranges, 16));
    search.matcher = &matcher;
    cui_context *ui =
        cui_create((cui_config){.width = 400, .height = 300, .measure_styled = search_measure});
    cui_editor_options options = cui_editor_default();
    options.textarea.width = 200;
    options.textarea.height = 100;
    options.textarea.text_style.family = 7;
    options.textarea.readonly = true;
    cui_draw_list commands;
    assert(draw(ui, &editor, options, &commands).error == CUI_TEXTAREA_OK);
    editor.view.scroll_y = editor.view.line_height * 2;
    cui_edit_select(edit, 0, 3);
    uint64_t revision = cui_edit_get_state(edit).revision;
    assert(cui_editor_search_open(&editor, &search, false) == CUI_EDIT_OK);
    assert(!strcmp(query, "foo") && matcher.current == 2 && matcher.count == 4 &&
           search.activation_revision == 1);
    /* Offset8 is the first visible row start, matching source anchor lookup. */
    assert(cui_edit_get_state(edit).anchor == 0 && cui_edit_get_state(edit).cursor == 3 &&
           cui_edit_get_state(edit).revision == revision);
    matcher.current = 3;
    assert(cui_editor_search_open(&editor, &search, false) == CUI_EDIT_OK && matcher.current == 3 &&
           search.activation_revision == 2);
    cui_edit_select(edit, 0, 0);
    options.search = &matcher;
    options.custom_search_colors = true;
    options.search_color = (cui_color){11, 22, 33, 255};
    options.search_current_color = (cui_color){44, 55, 66, 255};
    editor.view.scroll_y = 0;
    for (unsigned frame = 0; frame < 2; ++frame) {
        assert(draw(ui, &editor, options, &commands).error == CUI_TEXTAREA_OK);
        size_t ordinary = 0, current = 0;
        for (size_t i = 0; i < commands.count; ++i) {
            cui_draw_command c = commands.commands[i];
            if (c.type == CUI_DRAW_RECT && c.color.r == 11 && c.color.g == 22) {
                ++ordinary;
                assert(fabsf(c.bounds.w - 27) < .01f &&
                       fabsf(c.bounds.h - editor.view.line_height) < .01f);
            }
            if (c.type == CUI_DRAW_RECT && c.color.r == 44 && c.color.g == 55)
                ++current;
        }
        assert(ordinary == 3 && current == 1);
    }
    assert(cui_editor_matcher_set_query(&editor, &matcher, cui_string("foo\nfoo"), false) ==
           CUI_EDIT_OK);
    assert(draw(ui, &editor, options, &commands).error ==
           CUI_TEXTAREA_OK); /* split match IDs stay unique */
    cui_editor_matcher_close(&matcher);
    assert(draw(ui, &editor, options, &commands).error == CUI_TEXTAREA_OK);
    for (size_t i = 0; i < commands.count; ++i)
        assert(commands.commands[i].color.r != 11 || commands.commands[i].color.g != 22);
    cui_editor_fold fold = {0, 3, true};
    assert(cui_editor_set_folds(&editor, &fold, 1));
    assert(draw(ui, &editor, options, &commands).error == CUI_TEXTAREA_OK &&
           editor.view.count == 2);
    assert(cui_edit_replace_range(q, 0, cui_edit_get_text(q).length, cui_string("old")) ==
           CUI_EDIT_OK);
    editor.view.scroll_y = editor.view.line_height;
    cui_edit_select(edit, 0, 3);
    assert(cui_editor_search_open(&editor, &search, false) == CUI_EDIT_OK && matcher.current == 3);
    assert(draw(ui, &editor, options, &commands).error == CUI_TEXTAREA_OK);
    size_t ordinary = 0, current = 0, first_match = SIZE_MAX, selection = SIZE_MAX;
    cui_theme theme = cui_theme_light();
    for (size_t i = 0; i < commands.count; ++i) {
        cui_draw_command c = commands.commands[i];
        if (c.type == CUI_DRAW_RECT && c.color.r == 11 && c.color.g == 22) {
            ++ordinary;
            if (first_match == SIZE_MAX)
                first_match = i;
        }
        if (c.type == CUI_DRAW_RECT && c.color.r == 44 && c.color.g == 55)
            ++current;
        if (c.type == CUI_DRAW_RECT && c.color.r == theme.selection.r &&
            c.color.g == theme.selection.g && c.color.b == theme.selection.b &&
            selection == SIZE_MAX)
            selection = i;
    }
    assert(ordinary == 1 && current == 1 &&
           first_match < selection); /* hidden rows have no paint */
    /* Selected query capacity rejection preserves both models and activation. */
    cui_edit_select(edit, 0, cui_edit_get_text(edit).length);
    size_t activation = search.activation_revision;
    char saved[32];
    strcpy(saved, query);
    assert(cui_editor_search_open(&editor, &search, false) == CUI_EDIT_LINE_BREAK);
    assert(!strcmp(query, saved) && search.activation_revision == activation);
    char tiny_buffer[3] = "x";
    cui_edit *tiny =
        cui_edit_create((cui_edit_config){.buffer = tiny_buffer, .capacity = sizeof tiny_buffer});
    assert(tiny);
    search.query = tiny;
    cui_edit_select(edit, 0, 3);
    cui_editor_matcher old_matcher = matcher;
    assert(cui_editor_search_open(&editor, &search, false) == CUI_EDIT_CAPACITY);
    assert(!strcmp(tiny_buffer, "x") && search.activation_revision == activation &&
           !memcmp(&old_matcher, &matcher, sizeof matcher));
    search.query = q;
    cui_edit_destroy(tiny);
    cui_editor_match small_ranges[1];
    cui_editor_matcher small;
    assert(cui_editor_matcher_init(&small, small_ranges, 1));
    search.matcher = &small;
    assert(cui_editor_search_open(&editor, &search, false) == CUI_EDIT_CAPACITY &&
           search.activation_revision == activation && !strcmp(query, saved));
    cui_destroy(ui);
    cui_edit_destroy(q);
    cui_edit_destroy(edit);
}
static void multiple_cursor_adapter(void) {
    char buffer[128] = "abcd\nx\nabcd";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 4096,
                                                       .max_history_steps = 4});
    cui_edit_selection live[8];
    cui_edit_selection_snapshot history[5];
    assert(edit && cui_edit_attach_selections(edit, live, 8, history, 5) == CUI_EDIT_OK);
    assert(cui_edit_select(edit, 3, 3) == CUI_EDIT_OK);
    cui_textarea_line lines[32];
    cui_editor_state editor;
    assert(cui_editor_state_init(&editor, edit, lines, 32));
    cui_editor_cursor_anchor anchors[8];
    assert(!cui_editor_multicursor_init(&editor, anchors, 7));
    assert(cui_editor_multicursor_init(&editor, anchors, 8));
    cui_context *ui =
        cui_create((cui_config){.width = 400, .height = 300, .measure_styled = search_measure});
    assert(ui);
    cui_focus(ui, 10);
    cui_set_reduced_motion(ui, true);
    cui_editor_options o = cui_editor_default();
    o.textarea.width = 240;
    o.textarea.height = 180;
    o.textarea.text_style.family = 7;
    cui_draw_list commands;
    draw(ui, &editor, o, &commands);
    for (size_t i = 0; i < 2; ++i) {
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                               .key = CUI_KEY_DOWN,
                                               .modifiers = CUI_MOD_SHIFT | CUI_MOD_ALT}));
        cui_textarea_result r = draw(ui, &editor, o, &commands);
        assert(r.selection_changed && r.edit_error == CUI_EDIT_OK);
    }
    size_t count;
    const cui_edit_selection *c = cui_edit_get_selections(edit, &count);
    assert(count == 3 && c[0].cursor == 3 && c[1].cursor == 6 && c[2].cursor == 10);
    assert(editor.cursor_anchors[2].valid && editor.cursor_anchors[2].x == 27);
    cui_theme theme = cui_theme_light();
    size_t carets = 0;
    for (size_t i = 0; i < commands.count; ++i) {
        cui_draw_command cmd = commands.commands[i];
        if (cmd.type == CUI_DRAW_RECT && cmd.bounds.w == 1 && cmd.color.r == theme.caret.r &&
            cmd.color.g == theme.caret.g && cmd.color.b == theme.caret.b)
            ++carets;
    }
    assert(carets == 3);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "!"}));
    assert(draw(ui, &editor, o, &commands).changed);
    assert(!strcmp(buffer, "abc!d\nx!\nabc!d"));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL}));
    assert(draw(ui, &editor, o, &commands).changed && !strcmp(buffer, "abcd\nx\nabcd"));
    c = cui_edit_get_selections(edit, &count);
    assert(count == 3 && c[0].cursor == 3 && c[1].cursor == 6 && c[2].cursor == 10);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT, .modifiers = CUI_MOD_SHIFT}));
    assert(draw(ui, &editor, o, &commands).selection_changed);
    size_t paints = 0;
    for (size_t i = 0; i < commands.count; ++i) {
        cui_draw_command cmd = commands.commands[i];
        if (cmd.type == CUI_DRAW_RECT && cmd.color.r == theme.selection.r &&
            cmd.color.g == theme.selection.g && cmd.color.b == theme.selection.b)
            ++paints;
    }
    assert(paints == 3);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DELETE}));
    assert(draw(ui, &editor, o, &commands).changed && !strcmp(buffer, "abc\nxabc"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcd\nx\nabcd"));
    c = cui_edit_get_selections(edit, &count);
    assert(count == 3 && c[0].anchor == 3 && c[0].cursor == 4 && c[1].anchor == 6 &&
           c[1].cursor == 7);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    assert(draw(ui, &editor, o, &commands).selection_changed);
    c = cui_edit_get_selections(edit, &count);
    assert(count == 1 && c[0].anchor == 3 && c[0].cursor == 4);
    cui_edit_selection add = {.id = 100, .anchor = 6, .cursor = 6};
    assert(cui_edit_add_cursors(edit, &add, 1) == CUI_EDIT_OK);
    draw(ui, &editor, o, &commands);
    size_t row = cui_textarea_row_for_offset(&editor.view, 6, false);
    assert(row == 1 && cui_textarea_row_x(ui, edit, &editor.view, row, 6) == 9);
    assert(cui_textarea_row_offset(ui, edit, &editor.view, row, 27) == 6);
    assert(isnan(cui_textarea_row_x(ui, edit, &editor.view, 99, 6)));
    assert(cui_textarea_row_offset(ui, edit, &editor.view, 99, 27) == SIZE_MAX);
    assert(cui_edit_select(edit, 3, 3) == CUI_EDIT_OK);
    draw(ui, &editor, o, &commands);
    float px = editor.view.viewport.x + 9,
          py = editor.view.viewport.y + editor.view.line_height * 1.5f;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = px,
                                           .y = py,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .modifiers = CUI_MOD_ALT}));
    assert(draw(ui, &editor, o, &commands).selection_changed);
    c = cui_edit_get_selections(edit, &count);
    assert(count == 2 && c[0].cursor == 3 && c[1].cursor == 6);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = px,
                                           .y = py,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    draw(ui, &editor, o, &commands);
    draw(ui, &editor, o, &commands);
    c = cui_edit_get_selections(edit, &count);
    assert(count == 2 && c[0].cursor == 3 && c[1].cursor == 6);
    o.textarea.readonly = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "X"}));
    assert(!draw(ui, &editor, o, &commands).changed && !strcmp(buffer, "abcd\nx\nabcd"));
    c = cui_edit_get_selections(edit, &count);
    assert(count == 2);
    draw(ui, &editor, o, &commands);
    c = cui_edit_get_selections(edit, &count);
    assert(count == 2);
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void multicursor_visual_rows(void) {
    char buffer[128] = "abcdefghijklmnop";
    cui_edit *edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    cui_edit_selection live[4];
    assert(edit && cui_edit_attach_selections(edit, live, 4, NULL, 0) == CUI_EDIT_OK);
    cui_textarea_line lines[32];
    cui_editor_state editor;
    cui_editor_cursor_anchor anchors[4];
    assert(cui_editor_state_init(&editor, edit, lines, 32));
    assert(cui_editor_multicursor_init(&editor, anchors, 4));
    cui_context *ui =
        cui_create((cui_config){.width = 400, .height = 300, .measure_styled = search_measure});
    cui_focus(ui, 10);
    cui_editor_options o = cui_editor_default();
    o.line_numbers = false;
    o.textarea.width = 90;
    o.textarea.height = 100;
    o.textarea.no_wrap = false;
    o.textarea.text_style.family = 7;
    cui_draw_list commands;
    draw(ui, &editor, o, &commands);
    draw(ui, &editor, o, &commands);
    assert(editor.view.count > 1);
    size_t boundary = lines[0].end;
    assert(cui_edit_select(edit, boundary, boundary) == CUI_EDIT_OK);
    draw(ui, &editor, o, &commands);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
    draw(ui, &editor, o, &commands);
    assert(live[0].cursor == boundary && anchors[0].upstream);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    draw(ui, &editor, o, &commands);
    assert(live[0].cursor == boundary && !anchors[0].upstream);
    assert(cui_edit_replace_range(edit, 0, cui_edit_get_text(edit).length,
                                  cui_string("aa\nbb\ncc\ndd\nee")) == CUI_EDIT_OK);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    cui_editor_fold fold = {0, 3, true};
    assert(cui_editor_set_folds(&editor, &fold, 1));
    o.textarea.no_wrap = true;
    draw(ui, &editor, o, &commands);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                           .key = CUI_KEY_DOWN,
                                           .modifiers = CUI_MOD_CTRL | CUI_MOD_ALT}));
    draw(ui, &editor, o, &commands);
    size_t count;
    cui_edit_get_selections(edit, &count);
    assert(count == 2 && live[1].cursor == 10 && fold.collapsed);
    cui_edit_selection hidden = {99, 7, 7};
    assert(cui_edit_add_cursors(edit, &hidden, 1) == CUI_EDIT_OK);
    draw(ui, &editor, o, &commands);
    assert(!fold.collapsed);
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
typedef struct {
    char bytes[128];
    size_t writes, reads;
    bool fail;
    cui_edit *mutate;
} multi_clipboard;
static bool multi_copy(void *user, cui_str text) {
    multi_clipboard *host = user;
    ++host->writes;
    if (host->fail)
        return false;
    assert(text.length < sizeof host->bytes);
    memcpy(host->bytes, text.data, text.length);
    host->bytes[text.length] = 0;
    return true;
}
static cui_str multi_paste(void *user) {
    multi_clipboard *host = user;
    ++host->reads;
    if (host->mutate)
        assert(cui_edit_select(host->mutate, 0, 0) == CUI_EDIT_OK);
    return cui_string(host->bytes);
}
static cui_textarea_result multi_key(cui_context *ui, cui_editor_state *editor,
                                     cui_editor_options o, cui_key key, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key, .modifiers = modifiers}));
    cui_draw_list commands;
    return draw(ui, editor, o, &commands);
}
static void multicursor_column_history(void) {
    char buffer[128] = "abcd\nx\nabcdef";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 512,
                                                       .max_history_steps = 3});
    cui_edit_selection live[4];
    cui_edit_selection_snapshot history[4];
    cui_edit_column_anchor columns[4];
    cui_edit_column_snapshot column_history[4];
    assert(edit && cui_edit_attach_selections(edit, live, 4, history, 4) == CUI_EDIT_OK);
    assert(cui_edit_attach_columns(edit, columns, 4, column_history, 4) == CUI_EDIT_OK);
    cui_textarea_line lines[32];
    cui_editor_state editor;
    cui_editor_cursor_anchor anchors[4];
    assert(cui_editor_state_init(&editor, edit, lines, 32) &&
           cui_editor_multicursor_init(&editor, anchors, 4));
    cui_context *ui =
        cui_create((cui_config){.width = 400, .height = 300, .measure_styled = search_measure});
    cui_focus(ui, 10);
    cui_editor_options options = cui_editor_default();
    options.line_numbers = false;
    options.textarea.width = 240;
    options.textarea.height = 100;
    cui_draw_list commands;
    draw(ui, &editor, options, &commands);
    assert(cui_edit_select(edit, 3, 3) == CUI_EDIT_OK);
    draw(ui, &editor, options, &commands);
    assert(multi_key(ui, &editor, options, CUI_KEY_DOWN, 0).selection_changed);
    assert(live[0].cursor == 6 && anchors[0].valid);
    float preferred = anchors[0].x;
    size_t count = 0;
    const cui_edit_column_anchor *stored = cui_edit_get_columns(edit, &count);
    assert(count == 1 && stored[0].valid && stored[0].x == preferred && stored[0].column == 3);
    assert(cui_edit_insert(edit, cui_string("!")) == CUI_EDIT_OK);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && live[0].cursor == 6);
    stored = cui_edit_get_columns(edit, &count);
    assert(count == 1 && stored[0].valid && stored[0].x == preferred && stored[0].column == 3);
    draw(ui, &editor, options, &commands);
    assert(anchors[0].valid && anchors[0].x == preferred);
    options.textarea.text_style.family = 7;
    draw(ui, &editor, options, &commands);
    assert(anchors[0].valid && anchors[0].x == 27);
    assert(multi_key(ui, &editor, options, CUI_KEY_DOWN, 0).selection_changed);
    assert(live[0].cursor == 10);
    assert(cui_edit_replace_range(edit, 0, cui_edit_get_text(edit).length,
                                  cui_string("abcdefghijklmnop")) == CUI_EDIT_OK);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    options.textarea.width = 40;
    options.textarea.no_wrap = false;
    draw(ui, &editor, options, &commands);
    assert(editor.view.count > 1);
    assert(multi_key(ui, &editor, options, CUI_KEY_DOWN, 0).selection_changed);
    stored = cui_edit_get_columns(edit, &count);
    assert(live[0].cursor == 3 && count == 1 && stored[0].column == 1 && stored[0].valid);
    assert(cui_edit_replace_range(edit, 0, cui_edit_get_text(edit).length,
                                  cui_string("aa\nbb\ncc\ndd\nee")) == CUI_EDIT_OK);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    cui_editor_fold fold = {0, 3, true};
    assert(cui_editor_set_folds(&editor, &fold, 1));
    options.textarea.no_wrap = true;
    draw(ui, &editor, options, &commands);
    assert(multi_key(ui, &editor, options, CUI_KEY_DOWN, 0).selection_changed);
    stored = cui_edit_get_columns(edit, &count);
    assert(live[0].cursor == 10 && fold.collapsed && count == 1 && stored[0].column == 1);
    assert(cui_edit_replace_range(edit, 0, cui_edit_get_text(edit).length,
                                  cui_string("a\xC3\xA9Z\nx\nabc")) == CUI_EDIT_OK);
    assert(cui_edit_select(edit, 3, 3) == CUI_EDIT_OK);
    draw(ui, &editor, options, &commands);
    assert(multi_key(ui, &editor, options, CUI_KEY_DOWN, 0).selection_changed);
    stored = cui_edit_get_columns(edit, &count);
    assert(live[0].cursor == 6 && count == 1 && stored[0].column == 2 && stored[0].valid);
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void multicursor_delete_history(void) {
    char buffer[128] = "abc\nxyz";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 7});
    cui_edit_selection live[4];
    cui_edit_selection_snapshot history[8];
    assert(edit && cui_edit_attach_selections(edit, live, 4, history, 8) == CUI_EDIT_OK);
    cui_textarea_line lines[32];
    cui_editor_state editor;
    cui_editor_cursor_anchor anchors[4];
    assert(cui_editor_state_init(&editor, edit, lines, 32) &&
           cui_editor_multicursor_init(&editor, anchors, 4));
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_focus(ui, 10);
    cui_editor_options options = cui_editor_default();
    options.textarea.width = 240;
    options.textarea.height = 100;
    cui_edit_selection cursors[] = {{1, 3, 3}, {2, 7, 7}};
    assert(cui_edit_select_many(edit, cursors, 2) == CUI_EDIT_OK);
    cui_draw_list commands;
    draw(ui, &editor, options, &commands);
    assert(multi_key(ui, &editor, options, CUI_KEY_BACKSPACE, 0).changed);
    assert(multi_key(ui, &editor, options, CUI_KEY_BACKSPACE, 0).changed);
    assert(!strcmp(buffer, "a\nx"));
    assert(multi_key(ui, &editor, options, CUI_KEY_Z, CUI_MOD_CTRL).changed);
    assert(!strcmp(buffer, "abc\nxyz") && live[0].cursor == 3 && live[1].cursor == 7);
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void multicursor_clipboard_and_indent(void) {
    char buffer[128] = "1\n2\n3", scratch[128];
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 4096,
                                                       .max_history_steps = 4});
    cui_edit_selection live[8], input[] = {{3, 4, 4}, {1, 0, 0}, {2, 2, 2}};
    cui_edit_selection_snapshot history[5];
    assert(edit && cui_edit_attach_selections(edit, live, 8, history, 5) == CUI_EDIT_OK);
    cui_textarea_line lines[32];
    cui_editor_state editor;
    cui_editor_cursor_anchor anchors[8];
    assert(cui_editor_state_init(&editor, edit, lines, 32) &&
           cui_editor_multicursor_init(&editor, anchors, 8));
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_focus(ui, 10);
    multi_clipboard host = {.bytes = "x\ny\nz"};
    cui_platform platform = {.capabilities = CUI_PLATFORM_CLIPBOARD,
                             .user = &host,
                             .clipboard_set = multi_copy,
                             .clipboard_get = multi_paste};
    cui_editor_options o = cui_editor_default();
    o.textarea.width = 240;
    o.textarea.height = 180;
    o.textarea.clipboard = &platform;
    o.clipboard_scratch = scratch;
    o.clipboard_capacity = sizeof scratch;
    assert(cui_edit_select_many(edit, input, 3) == CUI_EDIT_OK);
    cui_draw_list commands;
    draw(ui, &editor, o, &commands);
    assert(multi_key(ui, &editor, o, CUI_KEY_TAB, 0).changed && !strcmp(buffer, "  1\n  2\n  3"));
    assert(multi_key(ui, &editor, o, CUI_KEY_TAB, CUI_MOD_SHIFT).changed &&
           !strcmp(buffer, "1\n2\n3"));
    /* Activates once the integrator supplies portable bracket key identities. */
    cui_key right_bracket = cui_key_from_character(']'), left_bracket = cui_key_from_character('[');
    if (right_bracket != CUI_KEY_NONE && left_bracket != CUI_KEY_NONE) {
        cui_edit_selection ends[] = {{3, 5, 5}, {1, 1, 1}, {2, 3, 3}};
        assert(cui_edit_select_many(edit, ends, 3) == CUI_EDIT_OK);
        assert(multi_key(ui, &editor, o, right_bracket, CUI_MOD_CTRL).changed &&
               !strcmp(buffer, "  1\n  2\n  3") && live[0].cursor == 11);
        assert(multi_key(ui, &editor, o, left_bracket, CUI_MOD_META).changed &&
               !strcmp(buffer, "1\n2\n3") && live[0].cursor == 5);
        assert(cui_edit_select_many(edit, input, 3) == CUI_EDIT_OK);
    }
    assert(multi_key(ui, &editor, o, CUI_KEY_V, CUI_MOD_CTRL).changed &&
           !strcmp(buffer, "x1\ny2\nz3"));
    assert(host.reads == 1);
    assert(multi_key(ui, &editor, o, CUI_KEY_Z, CUI_MOD_CTRL).changed &&
           !strcmp(buffer, "1\n2\n3"));
    size_t count;
    cui_edit_get_selections(edit, &count);
    assert(count == 3 && live[0].cursor == 4);
    assert(multi_key(ui, &editor, o, CUI_KEY_DOWN, 0).selection_changed);
    assert(anchors[0].valid);
    cui_editor_cursor_anchor saved_anchor = anchors[0];
    assert(!multi_key(ui, &editor, o, CUI_KEY_C, CUI_MOD_CTRL).changed);
    assert(anchors[0].valid && anchors[0].x == saved_anchor.x &&
           anchors[0].upstream == saved_anchor.upstream);
    host.writes = 0;
    cui_edit_selection selected[] = {{3, 4, 5}, {1, 0, 1}, {2, 2, 2}};
    assert(cui_edit_select_many(edit, selected, 3) == CUI_EDIT_OK);
    assert(!multi_key(ui, &editor, o, CUI_KEY_C, CUI_MOD_CTRL).changed &&
           !strcmp(host.bytes, "1\n3"));
    assert(host.writes == 1);
    host.fail = true;
    cui_textarea_result r = multi_key(ui, &editor, o, CUI_KEY_X, CUI_MOD_CTRL);
    assert(!r.changed && r.edit_error == CUI_EDIT_CLIPBOARD_UNAVAILABLE &&
           !strcmp(buffer, "1\n2\n3"));
    host.fail = false;
    o.clipboard_capacity = 2;
    r = multi_key(ui, &editor, o, CUI_KEY_X, CUI_MOD_CTRL);
    assert(!r.changed && r.edit_error == CUI_EDIT_CAPACITY && host.writes == 2);
    o.clipboard_capacity = sizeof scratch;
    r = multi_key(ui, &editor, o, CUI_KEY_X, CUI_MOD_CTRL);
    assert(r.changed && !strcmp(buffer, "\n2\n") && !strcmp(host.bytes, "1\n3"));
    assert(multi_key(ui, &editor, o, CUI_KEY_Z, CUI_MOD_CTRL).changed &&
           !strcmp(buffer, "1\n2\n3"));
    cui_edit_get_selections(edit, &count);
    assert(count == 3 && live[0].anchor == 4 && live[0].cursor == 5);
    o.textarea.readonly = true;
    size_t writes = host.writes, reads = host.reads;
    assert(!multi_key(ui, &editor, o, CUI_KEY_X, CUI_MOD_CTRL).changed &&
           host.writes == writes + 1);
    assert(!multi_key(ui, &editor, o, CUI_KEY_V, CUI_MOD_CTRL).changed && host.reads == reads);
    o.textarea.readonly = false;
    host.bytes[0] = 0;
    assert(!multi_key(ui, &editor, o, CUI_KEY_V, CUI_MOD_CTRL).changed);
    cui_edit_get_selections(edit, &count);
    assert(count == 3 && live[0].anchor == 4);
    o.clipboard_scratch = (char *)(void *)anchors;
    r = multi_key(ui, &editor, o, CUI_KEY_C, CUI_MOD_CTRL);
    assert(r.edit_error == CUI_EDIT_INVALID_ARGUMENT && host.writes == writes + 1);
    o.clipboard_scratch = scratch;
    host.mutate = edit;
    strcpy(host.bytes, "unexpected");
    r = multi_key(ui, &editor, o, CUI_KEY_V, CUI_MOD_CTRL);
    assert(!r.changed && r.edit_error == CUI_EDIT_INVALID_ARGUMENT && !strcmp(buffer, "1\n2\n3"));
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void rectangle_selection(void) {
    char buffer[256] = "abcdef\nab\nabcdef";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 4096,
                                                       .max_history_steps = 4});
    cui_edit_selection live[8];
    cui_edit_selection_snapshot history[5];
    assert(edit && cui_edit_attach_selections(edit, live, 8, history, 5) == CUI_EDIT_OK);
    cui_textarea_line lines[64];
    cui_editor_state editor;
    cui_editor_cursor_anchor anchors[8];
    assert(cui_editor_state_init(&editor, edit, lines, 64) &&
           cui_editor_multicursor_init(&editor, anchors, 8));
    cui_context *ui =
        cui_create((cui_config){.width = 400, .height = 300, .measure_styled = search_measure});
    cui_focus(ui, 10);
    cui_editor_options o = cui_editor_default();
    o.textarea.width = 240;
    o.textarea.height = 180;
    o.textarea.text_style.family = 7;
    cui_draw_list commands;
    draw(ui, &editor, o, &commands);
    /* End on a short row three space columns beyond its clipped end. */
    assert(cui_editor_select_rectangle(&editor, (cui_editor_column_point){1, 0},
                                       (cui_editor_column_point){9, 3}) == CUI_EDIT_OK);
    size_t count;
    cui_edit_get_selections(edit, &count);
    assert(count == 2 && live[0].anchor == 1 && live[0].cursor == 5 && live[1].anchor == 8 &&
           live[1].cursor == 9);
    assert(cui_edit_undo(edit) == CUI_EDIT_NO_HISTORY);
    assert(cui_editor_select_rectangle(&editor, (cui_editor_column_point){15, 0},
                                       (cui_editor_column_point){1, 0}) == CUI_EDIT_OK);
    cui_edit_get_selections(edit, &count);
    assert(count == 3);
    assert(live[0].anchor == 1 && live[0].cursor == 5 && live[1].anchor == 8 &&
           live[1].cursor == 9 && live[2].anchor == 11 && live[2].cursor == 15);
    cui_edit_selection saved[3];
    memcpy(saved, live, sizeof saved);
    uint64_t next = editor.next_cursor_id;
    assert(cui_editor_select_rectangle(&editor, (cui_editor_column_point){1, SIZE_MAX},
                                       (cui_editor_column_point){9, 0}) == CUI_EDIT_BOUNDARY);
    assert(!memcmp(saved, live, sizeof saved) && next == editor.next_cursor_id);
    assert(cui_edit_replace_selections(edit, cui_string("!")) == CUI_EDIT_OK &&
           !strcmp(buffer, "a!f\na!\na!f"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcdef\nab\nabcdef") &&
           !memcmp(saved, live, sizeof saved));
    draw(ui, &editor, o, &commands);
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    draw(ui, &editor, o, &commands);
    float x = editor.view.viewport.x, y = editor.view.viewport.y, h = editor.view.line_height;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = x + 9,
                                           .y = y + h * .5f,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .modifiers = CUI_MOD_ALT}));
    draw(ui, &editor, o, &commands);
    cui_edit_get_selections(edit, &count);
    assert(count == 2);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = x + 45,
                                           .y = y + h * 1.5f,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    assert(draw(ui, &editor, o, &commands).selection_changed);
    cui_edit_get_selections(edit, &count);
    assert(count == 2 && live[0].anchor == 1 && live[0].cursor == 5 && live[1].anchor == 8 &&
           live[1].cursor == 9);
    assert(editor.multi_pointer && editor.column_dragged);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = x + 45,
                                           .y = y + h * 1.5f,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    draw(ui, &editor, o, &commands);
    memcpy(saved, live, 2 * sizeof(*live));
    assert(!editor.multi_pointer);
    draw(ui, &editor, o, &commands);
    assert(!memcmp(saved, live, 2 * sizeof(*live)));
    /* Explicit block press replaces old cursors; readonly still permits selection. */
    o.textarea.readonly = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = x + 45,
                                           .y = y + h * 2.5f,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .modifiers = CUI_MOD_ALT | CUI_MOD_SHIFT}));
    draw(ui, &editor, o, &commands);
    cui_edit_get_selections(edit, &count);
    assert(count == 1 && live[0].cursor == 15);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = x + 9,
                                           .y = y + h * .5f,
                                           .pointer_button = CUI_POINTER_PRIMARY}));
    draw(ui, &editor, o, &commands);
    cui_edit_get_selections(edit, &count);
    assert(count == 3 && live[0].anchor == 1 && live[0].cursor == 5);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "NO"}));
    assert(!draw(ui, &editor, o, &commands).changed && !strcmp(buffer, "abcdef\nab\nabcdef"));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    draw(ui, &editor, o, &commands);
    assert(!editor.multi_pointer && !editor.column_dragged);
    cui_edit_get_selections(edit, &count);
    assert(count == 3);
    memcpy(saved, live, sizeof saved);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x + 18, .y = y + h * .5f}));
    draw(ui, &editor, o, &commands);
    assert(!memcmp(saved, live, sizeof saved));
    /* Folded interiors contribute no rectangular rows. */
    o.textarea.readonly = false;
    assert(cui_edit_replace_range(edit, 0, cui_edit_get_text(edit).length,
                                  cui_string("aaaa\nbbbb\ncccc\ndddd\neeee")) == CUI_EDIT_OK);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    cui_editor_fold fold = {0, 3, true};
    assert(cui_editor_set_folds(&editor, &fold, 1));
    draw(ui, &editor, o, &commands);
    assert(cui_editor_select_rectangle(&editor, (cui_editor_column_point){1, 0},
                                       (cui_editor_column_point){23, 0}) == CUI_EDIT_OK);
    cui_edit_get_selections(edit, &count);
    assert(count == 3 && live[1].anchor == 16 && live[1].cursor == 18 && fold.collapsed);
    assert(cui_editor_select_rectangle(&editor, (cui_editor_column_point){6, 0},
                                       (cui_editor_column_point){23, 0}) == CUI_EDIT_BOUNDARY);
    /* Caller capacity failure preserves collection and fresh-ID counter. */
    assert(cui_edit_attach_selections(edit, live, 2, history, 5) == CUI_EDIT_OK);
    assert(cui_editor_multicursor_init(&editor, anchors, 8));
    cui_edit_selection before_capacity = live[0];
    next = editor.next_cursor_id;
    assert(cui_editor_select_rectangle(&editor, (cui_editor_column_point){1, 0},
                                       (cui_editor_column_point){23, 0}) == CUI_EDIT_CAPACITY);
    assert(!memcmp(&before_capacity, live, sizeof before_capacity) &&
           editor.next_cursor_id == next);
    x = editor.view.viewport.x;
    y = editor.view.viewport.y;
    h = editor.view.line_height;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = x + 9,
                                           .y = y + h * .5f,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .modifiers = CUI_MOD_ALT | CUI_MOD_SHIFT}));
    draw(ui, &editor, o, &commands);
    before_capacity = live[0];
    next = editor.next_cursor_id;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x + 27, .y = y + h * 2.5f}));
    cui_textarea_result failed = draw(ui, &editor, o, &commands);
    assert(failed.edit_error == CUI_EDIT_CAPACITY &&
           !memcmp(&before_capacity, live, sizeof before_capacity) &&
           editor.next_cursor_id == next);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    draw(ui, &editor, o, &commands);
    assert(cui_edit_attach_selections(edit, live, 8, history, 5) == CUI_EDIT_OK);
    assert(cui_editor_multicursor_init(&editor, anchors, 8));
    assert(cui_editor_set_folds(&editor, NULL, 0));
    assert(cui_edit_replace_range(edit, 0, cui_edit_get_text(edit).length,
                                  cui_string("abcdefghijklmnop")) == CUI_EDIT_OK);
    o.line_numbers = false;
    o.textarea.width = 90;
    o.textarea.no_wrap = false;
    draw(ui, &editor, o, &commands);
    draw(ui, &editor, o, &commands);
    assert(editor.view.count > 1 && editor.view.count <= 8);
    size_t row_count = editor.view.count;
    assert(cui_editor_select_rectangle(&editor, (cui_editor_column_point){0, 0},
                                       (cui_editor_column_point){16, 20}) == CUI_EDIT_OK);
    cui_edit_get_selections(edit, &count);
    assert(count == row_count);
    for (size_t i = 0; i < count; ++i)
        assert(live[i].anchor == lines[i].start && live[i].cursor == lines[i].end);
    /* Adjacent soft-wrap ranges stay distinct through the next atomic edit/Undo. */
    assert(cui_edit_replace_selections(edit, cui_string("!")) == CUI_EDIT_OK);
    assert(cui_edit_get_text(edit).length == row_count);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcdefghijklmnop"));
    cui_edit_get_selections(edit, &count);
    assert(count == row_count);
    assert(cui_edit_replace_range(edit, 0, 16, cui_string("abcd\na\xcc\x81z")) == CUI_EDIT_OK);
    o.textarea.width = 240;
    o.textarea.no_wrap = true;
    draw(ui, &editor, o, &commands);
    assert(cui_editor_select_rectangle(&editor, (cui_editor_column_point){2, 0},
                                       (cui_editor_column_point){5, 0}) == CUI_EDIT_OK);
    cui_edit_get_selections(edit, &count);
    assert(count == 2 && live[0].anchor == 0 && live[0].cursor == 2 && live[1].anchor == 5 &&
           live[1].cursor == 5);
    /* Auto-scroll continues on stationary-pointer frames and stops on cancel. */
    assert(cui_edit_replace_range(edit, 0, cui_edit_get_text(edit).length,
                                  cui_string("aa\naa\naa\naa\naa\naa\naa\naa\naa\naa\naa\naa")) ==
           CUI_EDIT_OK);
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    editor.view.scroll_y = 0;
    o.textarea.height = 60;
    draw(ui, &editor, o, &commands);
    draw(ui, &editor, o, &commands);
    x = editor.view.viewport.x;
    y = editor.view.viewport.y;
    h = editor.view.line_height;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = x + 9,
                                           .y = y + h * .5f,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .modifiers = CUI_MOD_ALT | CUI_MOD_SHIFT}));
    draw(ui, &editor, o, &commands);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = x + 18,
                                           .y = y + editor.view.viewport.h - 1}));
    assert(draw(ui, &editor, o, &commands).scrolled);
    float scroll = editor.view.scroll_y;
    assert(scroll > 0);
    assert(draw(ui, &editor, o, &commands).scrolled && editor.view.scroll_y > scroll);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    draw(ui, &editor, o, &commands);
    scroll = editor.view.scroll_y;
    draw(ui, &editor, o, &commands);
    assert(editor.view.scroll_y == scroll && !editor.multi_pointer);
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void large_document_editor(void) {
    const size_t rows = 200000, bytes = rows * 2;
    char *buffer = malloc(bytes + 64);
    cui_textarea_line *lines = malloc((rows + 1) * sizeof(*lines));
    assert(buffer && lines);
    for (size_t i = 0; i < rows; ++i) {
        buffer[2 * i] = 'a';
        buffer[2 * i + 1] = '\n';
    }
    buffer[bytes] = 0;
    cui_edit *edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = bytes + 64, .multiline = true});
    assert(edit);
    cui_editor_state state;
    assert(cui_editor_state_init(&state, edit, lines, rows + 1));
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(ui);
    cui_focus(ui, 10);
    cui_editor_options options = cui_editor_default();
    options.textarea.width = 200;
    options.textarea.height = 80;
    cui_draw_list commands;
    assert(draw(ui, &state, options, &commands).error == CUI_TEXTAREA_OK);
    assert(state.view.count == rows + 1);
    assert(cui_edit_select(edit, bytes - 2, bytes - 2) == CUI_EDIT_OK);
    assert(draw(ui, &state, options, &commands).error == CUI_TEXTAREA_OK);
    assert(state.view.caret_line == rows - 1 && state.view.scroll_y > 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "z"}));
    assert(draw(ui, &state, options, &commands).error == CUI_TEXTAREA_OK);
    assert(buffer[bytes - 2] == 'z' && buffer[bytes - 1] == 'a');
    cui_destroy(ui);
    cui_edit_destroy(edit);
    free(lines);
    free(buffer);
}
int main(void) {
    large_document_editor();
    rectangle_selection();
    multicursor_clipboard_and_indent();
    multicursor_visual_rows();
    multicursor_column_history();
    multicursor_delete_history();
    multiple_cursor_adapter();
    search_activation_and_paint();
    replacement_panel();
    retained_matcher();
    replace_workflow();
    inline_tokens_reject_code_mode();
    completion_model();
    completion_popup();
    narrow_editor_clipping();
    folding();
    find_panel();
    base_and_utf8();
    editor_interactions();
    puts("editor tests passed");
    return 0;
}
