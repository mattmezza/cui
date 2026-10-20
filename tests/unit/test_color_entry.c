/* SPDX-License-Identifier: MIT */
#include "cui/cui_color_entry.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static bool same(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static void models(void) {
    cui_color sentinel = {1, 2, 3, 4}, out = sentinel;
    const char *valid[] = {"#abc",    "aBc",    "#abcd",     "abcd",
                           "#AABBCC", "aabbcc", "#AABBCCDD", "aabbccdd"};
    for (size_t i = 0; i < sizeof valid / sizeof *valid; ++i) {
        assert(cui_color_entry_parse(cui_string(valid[i]), &out) == CUI_COLOR_HEX_COMPLETE);
        assert(same(out, (cui_color){170, 187, 204, i == 2 || i == 3 || i >= 6 ? 221 : 255}));
    }
    const char *incomplete[] = {"", "#", "#1", "12", "#12345", "1234567"};
    for (size_t i = 0; i < sizeof incomplete / sizeof *incomplete; ++i) {
        out = sentinel;
        assert(cui_color_entry_parse(cui_string(incomplete[i]), &out) == CUI_COLOR_HEX_INCOMPLETE);
        assert(same(out, sentinel));
    }
    const char *invalid[] = {"#nope", "+abc", "-abc",       " abc",  "#abcdefghi",
                             "##abc", "#éff", "#123456789", "#123\n"};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
        out = sentinel;
        assert(cui_color_entry_parse(cui_string(invalid[i]), &out) == CUI_COLOR_HEX_INVALID);
        assert(same(out, sentinel));
    }
    assert(cui_color_entry_parse((cui_str){NULL, 1}, &out) == CUI_COLOR_HEX_INVALID);
    assert(!cui_color_parse_hex(cui_string("#abc"), &out));
    assert(!cui_color_parse_hex(cui_string("aabbcc"), &out));
    assert(cui_color_parse_hex(cui_string("#AABBCCDD"), &out));
    assert(cui_color_from_hsv((cui_hsv){0, 0, 0, .5f}).a == 128);
    assert(cui_color_entry_parse(cui_string("#0000007F"), &out) == CUI_COLOR_HEX_COMPLETE);
    assert(out.a == 127); /* Frozen float formatter's .5 truncation is a different byte. */
    char bounded[3] = {'x', 'x', 'z'};
    assert(cui_color_format_hex(out, true, bounded, 2) == 9);
    assert(bounded[0] == '#' && !bounded[1] && bounded[2] == 'z');

    cui_color_entry_state state = {0};
    assert(cui_color_entry_init(&state, (cui_color_entry_value){0}));
    assert(!state.preview.present && !state.committed.present && !state.draft[0]);
    assert(!cui_color_entry_init(&state, (cui_color_entry_value){0}));
    assert(cui_color_entry_set_draft(&state, cui_string("#1234")) == CUI_COLOR_ENTRY_OK);
    assert(state.preview.present && !state.committed.present && state.dirty);
    cui_color preview = state.preview.color;
    assert(cui_color_entry_set_draft(&state, cui_string("#12")) == CUI_COLOR_ENTRY_OK);
    assert(same(preview, state.preview.color));
    assert(cui_color_entry_commit(&state) == CUI_COLOR_ENTRY_INCOMPLETE);
    assert(cui_color_entry_restore(&state) == CUI_COLOR_ENTRY_OK);
    assert(!state.preview.present && !state.draft[0] && !state.dirty);
    assert(cui_color_entry_set_draft(&state, cui_string("#12")) == CUI_COLOR_ENTRY_OK);
    cui_edit_state before = cui_edit_get_state(state.edit);
    assert(cui_color_entry_sync(&state, (cui_color_entry_value){true, {0, 0, 0, 0}}, false) ==
           CUI_COLOR_ENTRY_CONFLICT);
    assert(!strcmp(state.draft, "#12") &&
           cui_edit_get_state(state.edit).revision == before.revision);
    assert(cui_color_entry_set_draft(&state, cui_string("abc")) ==
           CUI_COLOR_ENTRY_INVALID_ARGUMENT);
    assert(cui_color_entry_set_draft(&state, cui_string("#123456789")) ==
           CUI_COLOR_ENTRY_INVALID_ARGUMENT);
    assert(!strcmp(state.draft, "#12"));
    assert(cui_color_entry_sync(&state, (cui_color_entry_value){true, {0, 0, 0, 0}}, true) ==
           CUI_COLOR_ENTRY_OK);
    assert(state.committed.present && !strcmp(state.draft, "#00000000"));
    assert(!state.dirty && !cui_edit_get_state(state.edit).can_undo);
    assert(cui_color_entry_commit(&state) == CUI_COLOR_ENTRY_OK);
    assert(cui_color_entry_commit(&state) == CUI_COLOR_ENTRY_OK);
    assert(cui_color_entry_sync(&state, (cui_color_entry_value){0}, false) == CUI_COLOR_ENTRY_OK);
    assert(!state.preview.present && !state.committed.present && !state.draft[0]);
    cui_color_entry_destroy(&state);
    cui_color_entry_destroy(&state);
    assert(cui_color_entry_commit(&state) == CUI_COLOR_ENTRY_INVALID_ARGUMENT);
}
typedef struct {
    cui_context *ui;
    cui_color_entry_state state;
    cui_color_entry_options options;
    cui_color_entry_result copied;
    cui_color_entry_value next_owner;
    size_t calls;
    bool styled, inert, layout, consume_enter;
    cui_theme theme;
} fixture;
static void committed(void *user, const cui_color_entry_result *event) {
    fixture *f = user;
    assert(event->commit_count > 0 && event->submitted);
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(event->event.frame_sequence > 0 && event->event.target == 10);
    assert(same(event->committed.color, f->state.committed.color));
    size_t n = 0;
    const cui_semantic *semantics = cui_semantics(f->ui, &n);
    bool found = false;
    for (size_t i = 0; i < n; ++i)
        found |= semantics[i].id == 10 && semantics[i].role == CUI_ROLE_TEXTBOX;
    assert(found);
    f->copied = *event;
    ++f->calls;
    /* Owner updates are separate and applied only on next submission. */
    f->next_owner = (cui_color_entry_value){true, {7, 8, 9, 255}};
}
static cui_color_entry_result frame(fixture *f, bool observe) {
    assert(cui_begin_frame(f->ui, 400, 180, .016f));
    cui_begin_box(f->ui, 1, (cui_box){.inert = f->inert, .layout_only = f->layout});
    if (f->consume_enter) {
        size_t n = 0;
        const cui_event *events = cui_frame_events(f->ui, &n);
        for (size_t i = 0; i < n; ++i)
            if (events[i].key == CUI_KEY_ENTER)
                cui_consume_event(f->ui, i);
    }
    cui_color_entry_handler h = {observe ? committed : NULL, f};
    cui_color_entry_result result;
    if (f->styled)
        result = cui_color_entry(f->ui, 10, &f->theme, cui_string("Hex color"), &f->state,
                                 f->options, h);
    else {
        cui_begin_box(f->ui, 10, (cui_box){.width = 240, .height = 32});
        result = cui_base_color_entry(f->ui, 10, cui_string("Hex color"), &f->state, f->options, h);
        cui_end_box(f->ui);
    }
    cui_end_box(f->ui);
    cui_draw_list draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK && result.input.error == CUI_EDIT_OK);
    if (f->styled && !f->layout)
        assert(draw.count > 0);
    if (f->styled && !f->layout && !f->options.disabled) {
        bool background = false;
        for (size_t i = 0; i < draw.count; ++i)
            background |= draw.commands[i].type == CUI_DRAW_RECT &&
                          same(draw.commands[i].color, f->theme.input_background);
        assert(background);
    }
    size_t count = 0;
    const cui_semantic *semantics = cui_semantics(f->ui, &count);
    if (!f->inert && !f->layout) {
        bool named = false;
        for (size_t i = 0; i < count; ++i)
            if (semantics[i].id == 10) {
                assert(semantics[i].role == CUI_ROLE_TEXTBOX);
                assert(semantics[i].name.length == 9 &&
                       !memcmp(semantics[i].name.data, "Hex color", 9));
                assert(semantics[i].disabled == f->options.disabled);
                assert(semantics[i].details.readonly == f->options.readonly);
                named = true;
            }
        assert(named);
    }
    return result;
}
static void key(fixture *f, cui_key k, unsigned modifiers) {
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k, .modifiers = modifiers}));
}
static void replace(fixture *f, const char *text) {
    key(f, CUI_KEY_A, CUI_MOD_CTRL);
    cui_event event = {.type = CUI_EVENT_TEXT};
    assert(strlen(text) < sizeof event.text);
    memcpy(event.text, text, strlen(text) + 1);
    assert(cui_queue_event(f->ui, event));
}
static void fields(bool styled) {
    fixture f = {.styled = styled, .theme = cui_theme_light()};
    f.ui = cui_create((cui_config){.max_elements = 256, .max_commands = 512});
    assert(f.ui && cui_color_entry_init(&f.state, (cui_color_entry_value){true, {255, 0, 0, 255}}));
    frame(&f, true);
    cui_rect bounds = cui_bounds(f.ui, 10);
    assert(bounds.w > 0 && bounds.h > 0);
    cui_event click = {
        .type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 4, .y = bounds.y + bounds.h / 2};
    assert(cui_queue_event(f.ui, click));
    click.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(f.ui, click));
    frame(&f, true);
    assert(cui_focused(f.ui) == 10);
    if (styled)
        assert(cui_edit_get_state(f.state.edit).cursor == 0);
    replace(&f, "#abc");
    cui_color_entry_result r = frame(&f, true);
    assert(r.draft_changed && !r.commit_count && f.state.dirty && f.calls == 0);
    assert(same(f.state.committed.color, (cui_color){255, 0, 0, 255}));
    key(&f, CUI_KEY_ENTER, 0);
    replace(&f, "#1234");
    key(&f, CUI_KEY_ENTER, 0);
    replace(&f, "#ff");
    r = frame(&f, true);
    assert(r.commit_count == 2 && f.calls == 1 && r.draft_changed);
    assert(same(r.committed.color, (cui_color){17, 34, 51, 68}));
    assert(same(r.previous.color, (cui_color){255, 0, 0, 255}));
    assert(!strcmp(f.state.draft, "#ff") && f.state.dirty);
    key(&f, CUI_KEY_ENTER, 0);
    r = frame(&f, true);
    assert(!r.commit_count && r.rejected_commit_count == 1 && f.calls == 1);
    replace(&f, "oops");
    r = frame(&f, true);
    assert(r.input.rejected && !strcmp(f.state.draft, "#ff"));
    replace(&f, "#1234");
    key(&f, CUI_KEY_ENTER, 0);
    r = frame(&f, false);
    assert(r.commit_count == 1 && !f.state.dirty && f.calls == 1);
    key(&f, CUI_KEY_ENTER, 0);
    r = frame(&f, true);
    assert(r.commit_count == 1 && f.calls == 2); /* Same-value commit. */
    assert(cui_color_entry_sync(&f.state, f.next_owner, false) == CUI_COLOR_ENTRY_OK);
    frame(&f, true);
    assert(!strcmp(f.state.draft, "#070809") && f.calls == 2);
    f.consume_enter = true;
    key(&f, CUI_KEY_ENTER, 0);
    assert(!frame(&f, true).commit_count);
    f.consume_enter = false;
    assert(cui_queue_event(
        f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    assert(!frame(&f, true).commit_count);
    for (unsigned mode = 0; mode < 4; ++mode) {
        f.options.disabled = mode == 0;
        f.options.readonly = mode == 1;
        f.inert = mode == 2;
        f.layout = mode == 3;
        replace(&f, "#000");
        key(&f, CUI_KEY_ENTER, 0);
        r = frame(&f, true);
        assert(!r.commit_count && !r.submitted && !strcmp(f.state.draft, "#070809"));
    }
    f.options = (cui_color_entry_options){0};
    f.inert = f.layout = false;
    cui_theme themes[] = {cui_theme_dark(), cui_theme_high_contrast(), cui_theme_terminal()};
    for (size_t i = 0; i < sizeof themes / sizeof *themes; ++i) {
        f.theme = themes[i];
        frame(&f, true);
        assert(!strcmp(f.state.draft, "#070809"));
    }
    cui_destroy(f.ui);
    cui_color_entry_destroy(&f.state);
    assert(f.copied.commit_count == 1 && f.copied.committed.color.a == 68);
}
int main(void) {
    models();
    fields(false);
    fields(true);
    puts("ColorEntry model and real field tests passed");
    return 0;
}
