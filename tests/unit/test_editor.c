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
    cui_focus(ui, field);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    result = search_frame(ui, &editor, &search, false);
    assert(result.closed && !search.open && !matcher.active && cui_focused(ui) == 10);
    cui_destroy(ui);
    cui_edit_destroy(r);
    cui_edit_destroy(q);
    cui_edit_destroy(edit);
}
int main(void) {
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
