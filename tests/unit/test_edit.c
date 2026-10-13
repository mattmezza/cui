#include "cui/cui_edit.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static cui_edit *make(char *buffer, size_t capacity, bool multiline, size_t steps) {
    cui_edit *e = cui_edit_create((cui_edit_config){buffer, capacity, multiline, 8192, steps});
    assert(e);
    return e;
}
static void test_unicode(void) {
    char b[64] = "A\xc3\xa9\xf0\x9f\x98\x80Z";
    cui_edit *e = make(b, sizeof(b), false, 10);
    assert(!cui_edit_get_state(e).can_undo);
    assert(cui_edit_get_state(e).revision == 0);
    assert(cui_edit_select(e, 2, 2) == CUI_EDIT_BOUNDARY);
    assert(cui_edit_select(e, 3, 3) == CUI_EDIT_OK);
    assert(cui_edit_delete(e) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).revision == 1);
    assert(!strcmp(b, "A\xc3\xa9Z"));
    assert(cui_edit_undo(e) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).revision == 2);
    assert(cui_edit_get_state(e).cursor == 3 && cui_edit_get_state(e).anchor == 3);
    assert(!strcmp(b, "A\xc3\xa9\xf0\x9f\x98\x80Z"));
    assert(cui_edit_backspace(e) == CUI_EDIT_OK);
    assert(!strcmp(b, "A\xf0\x9f\x98\x80Z"));
    assert(cui_edit_undo(e) == CUI_EDIT_OK);
    assert(cui_edit_redo(e) == CUI_EDIT_OK);
    assert(cui_edit_move(e, CUI_EDIT_END, false) == CUI_EDIT_OK);
    assert(cui_edit_move(e, CUI_EDIT_PREVIOUS, true) == CUI_EDIT_OK);
    assert(cui_edit_insert(e, cui_string("!")) == CUI_EDIT_OK);
    assert(!strcmp(b, "A\xf0\x9f\x98\x80!"));
    assert(cui_edit_select(e, 0, 1) == CUI_EDIT_OK);
    assert(cui_edit_insert(e, (cui_str){b + 1, 4}) == CUI_EDIT_OK);
    assert(!memcmp(b, "\xf0\x9f\x98\x80\xf0\x9f\x98\x80!", 9));
    cui_edit_destroy(e);
}
static void test_rejection(void) {
    const char *invalid[] = {
        "\x80", "\xc0\xaf", "\xe0\x80\x80", "\xed\xa0\x80", "\xf4\x90\x80\x80", "\xf0\x9f", "\xff"};
    char b[8] = "ok";
    cui_edit *e = make(b, sizeof(b), false, 3);
    for (size_t i = 0; i < sizeof(invalid) / sizeof(*invalid); i++) {
        assert(!cui_edit_valid_utf8(cui_string(invalid[i])));
        assert(cui_edit_insert(e, cui_string(invalid[i])) == CUI_EDIT_INVALID_UTF8);
        assert(!strcmp(b, "ok"));
    }
    char nul[3] = {'a', 0, 'b'};
    assert(!cui_edit_valid_utf8((cui_str){nul, 3}));
    assert(!cui_edit_valid_utf8((cui_str){NULL, 1}));
    assert(cui_edit_valid_utf8((cui_str){NULL, 0}));
    assert(cui_edit_insert(e, cui_string("\n")) == CUI_EDIT_LINE_BREAK);
    assert(cui_edit_insert(e, cui_string("123456")) == CUI_EDIT_CAPACITY);
    assert(!cui_edit_get_state(e).can_undo);
    cui_edit_destroy(e);
    char no_nul[2] = {'a', 'b'};
    assert(!cui_edit_create((cui_edit_config){.buffer = no_nul, .capacity = 2}));
    char malformed[8] = "\xed\xa0\x80";
    assert(!cui_edit_create((cui_edit_config){.buffer = malformed, .capacity = 8}));
}
static void test_history_lines(void) {
    char b[64] = "a\n\xc3\xa9z";
    cui_edit *e = make(b, sizeof(b), true, 2);
    assert(cui_edit_move(e, CUI_EDIT_LINE_START, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).cursor == 2);
    assert(cui_edit_move(e, CUI_EDIT_LINE_END, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).cursor == 5);
    for (int i = 0; i < 5; i++)
        assert(cui_edit_insert(e, cui_string("x")) == CUI_EDIT_OK);
    assert(cui_edit_undo(e) == CUI_EDIT_OK);
    assert(cui_edit_undo(e) == CUI_EDIT_OK);
    assert(cui_edit_undo(e) == CUI_EDIT_NO_HISTORY);
    assert(cui_edit_redo(e) == CUI_EDIT_OK);
    assert(cui_edit_insert(e, cui_string("y")) == CUI_EDIT_OK);
    assert(cui_edit_redo(e) == CUI_EDIT_NO_HISTORY);
    cui_edit_destroy(e);
    b[0] = 0;
    e = cui_edit_create((cui_edit_config){.buffer = b, .capacity = 64, .history_bytes = 1});
    assert(e);
    assert(cui_edit_insert(e, cui_string("a")) == CUI_EDIT_OK);
    assert(cui_edit_undo(e) == CUI_EDIT_NO_HISTORY);
    cui_edit_destroy(e);
}
static unsigned rng = 0x12345u;
static unsigned random_number(void) {
    rng = rng * 1664525u + 1013904223u;
    return rng;
}
static void test_random_operations(void) {
    char b[128] = "";
    cui_edit *e = make(b, sizeof(b), true, 12);
    const char *texts[] = {"a", "\xc3\xa9", "\xe4\xb8\xad", "\xf0\x9f\x98\x80", "\n", "\xcc\x81"};
    for (size_t i = 0; i < 15000; i++) {
        unsigned r = random_number();
        switch (r % 9) {
        case 0:
        case 1:
            (void)cui_edit_insert(e, cui_string(texts[(r >> 8) % 6]));
            break;
        case 2:
            (void)cui_edit_backspace(e);
            break;
        case 3:
            (void)cui_edit_delete(e);
            break;
        case 4:
            (void)cui_edit_undo(e);
            break;
        case 5:
            (void)cui_edit_redo(e);
            break;
        case 6:
            (void)cui_edit_move(e, CUI_EDIT_PREVIOUS, (r & 0x100) != 0);
            break;
        case 7:
            (void)cui_edit_move(e, CUI_EDIT_NEXT, (r & 0x100) != 0);
            break;
        default:
            (void)cui_edit_move(e, (r & 0x100) ? CUI_EDIT_START : CUI_EDIT_END, (r & 0x200) != 0);
            break;
        }
        cui_edit_state st = cui_edit_get_state(e);
        assert(st.length < sizeof(b) && st.length == strlen(b));
        assert(cui_edit_valid_utf8(cui_edit_get_text(e)));
        assert(st.cursor <= st.length && st.anchor <= st.length);
        assert(st.cursor == st.length || ((unsigned char)b[st.cursor] & 0xc0u) != 0x80u);
        assert(st.anchor == st.length || ((unsigned char)b[st.anchor] & 0xc0u) != 0x80u);
    }
    cui_edit_destroy(e);
}
static void textbox(cui_context *ui, cui_id id) {
    cui_begin_box(ui, id, (cui_box){.width = 100, .height = 30});
    cui_interact(ui, id, CUI_ROLE_TEXTBOX, cui_string("input"), false, false, 0);
    cui_end_box(ui);
}
static void test_events(void) {
    char a[64] = "", b[64] = "";
    cui_edit *ea = make(a, sizeof(a), false, 3), *eb = make(b, sizeof(b), false, 3);
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    assert(cui_begin_frame(ui, 300, 200, 0));
    textbox(ui, 1);
    textbox(ui, 2);
    cui_end_frame(ui);
    cui_focus(ui, 1);
    cui_event t = {.type = CUI_EVENT_TEXT};
    strcpy(t.text, "first");
    assert(cui_queue_event(ui, t));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    strcpy(t.text, "second");
    assert(cui_queue_event(ui, t));
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(cui_edit_apply_events(ea, ui, 1, false) == CUI_EDIT_OK);
    assert(cui_edit_apply_events(eb, ui, 2, false) == CUI_EDIT_OK);
    assert(!strcmp(a, "first") && !strcmp(b, "second"));
    textbox(ui, 1);
    textbox(ui, 2);
    cui_end_frame(ui);
    memset(t.text, 'x', sizeof(t.text));
    assert(cui_queue_event(ui, t));
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(cui_edit_apply_events(eb, ui, 2, false) == CUI_EDIT_INVALID_ARGUMENT);
    assert(!strcmp(b, "second"));
    textbox(ui, 1);
    textbox(ui, 2);
    cui_end_frame(ui);
    cui_destroy(ui);
    cui_edit_destroy(ea);
    cui_edit_destroy(eb);
}

static void test_revision_and_max_scalar(void) {
    char b[32] = "abc";
    cui_edit *e = make(b, sizeof(b), true, 5);
    assert(cui_edit_get_state(e).multiline);
    assert(cui_edit_select(e, 0, 3) == CUI_EDIT_OK);
    assert(cui_edit_insert(e, cui_string("abc")) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).revision == 0);
    assert(!cui_edit_get_state(e).can_undo);
    assert(cui_edit_select(e, 0, 3) == CUI_EDIT_OK);
    assert(cui_edit_insert(e, cui_string("xyz")) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).revision == 1);
    assert(cui_edit_undo(e) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).revision == 2);
    assert(!strcmp(b, "abc"));
    assert(cui_edit_redo(e) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).revision == 3);
    assert(!strcmp(b, "xyz"));
    assert(cui_edit_insert(e, cui_string("\xf4\x8f\xbf\xbf")) == CUI_EDIT_OK);
    assert(cui_edit_valid_utf8(cui_edit_get_text(e)));
    cui_edit_destroy(e);
}
static void test_atomic_indent(void) {
    char buffer[64] = "\xc3\xa9\nx\n";
    cui_edit *edit = make(buffer, sizeof buffer, true, 8);
    assert(cui_edit_select(edit, 5, 0) == CUI_EDIT_OK);
    assert(cui_edit_indent(edit, false, 2, false) == CUI_EDIT_OK);
    assert(strcmp(buffer, "  \xc3\xa9\n  x\n  ") == 0);
    cui_edit_state state = cui_edit_get_state(edit);
    assert(state.anchor == 11 && state.cursor == 2 && state.can_undo);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    state = cui_edit_get_state(edit);
    assert(strcmp(buffer, "\xc3\xa9\nx\n") == 0 && state.anchor == 5 && state.cursor == 0);
    assert(cui_edit_redo(edit) == CUI_EDIT_OK);
    state = cui_edit_get_state(edit);
    assert(state.anchor == 11 && state.cursor == 2);
    assert(cui_edit_indent(edit, true, 2, false) == CUI_EDIT_OK);
    assert(strcmp(buffer, "\xc3\xa9\nx\n") == 0);
    state = cui_edit_get_state(edit);
    assert(state.anchor == 5 && state.cursor == 0);
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    assert(cui_edit_indent(edit, false, 2, true) == CUI_EDIT_OK);
    assert(strcmp(buffer, "\t\xc3\xa9\nx\n") == 0);
    assert(cui_edit_get_state(edit).cursor == 1);
    assert(cui_edit_indent(edit, true, 2, true) == CUI_EDIT_OK);
    assert(strcmp(buffer, "\xc3\xa9\nx\n") == 0);
    state = cui_edit_get_state(edit);
    assert(state.anchor == 0 && state.cursor == 0);
    assert(cui_edit_indent(edit, true, 2, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).revision == state.revision);
    cui_edit_destroy(edit);

    char tight[8] = "a\nb";
    edit = make(tight, sizeof tight, true, 4);
    assert(cui_edit_select(edit, 0, 3) == CUI_EDIT_OK);
    state = cui_edit_get_state(edit);
    assert(cui_edit_indent(edit, false, 3, false) == CUI_EDIT_CAPACITY);
    cui_edit_state after = cui_edit_get_state(edit);
    assert(strcmp(tight, "a\nb") == 0 && after.revision == state.revision &&
           after.anchor == state.anchor && after.cursor == state.cursor && !after.can_undo);
    assert(cui_edit_indent(edit, false, 0, false) == CUI_EDIT_INVALID_ARGUMENT);
    cui_edit_destroy(edit);
}

static void test_atomic_range(void) {
    char b[16] = "a\xc3\xa9z";
    cui_edit *e = make(b, sizeof b, false, 10);
    assert(cui_edit_select(e, 3, 1) == CUI_EDIT_OK);
    assert(cui_edit_replace_range(e, 0, 4, cui_string("reset")) == CUI_EDIT_OK);
    assert(!strcmp(b, "reset") && cui_edit_get_state(e).cursor == 5);
    assert(cui_edit_undo(e) == CUI_EDIT_OK);
    assert(!strcmp(b, "a\xc3\xa9z"));
    cui_edit_state before = cui_edit_get_state(e);
    assert(before.anchor == 3 && before.cursor == 1 && !before.can_undo && before.can_redo);
    assert(cui_edit_replace_range(e, 2, 3, cui_string("x")) == CUI_EDIT_BOUNDARY);
    assert(cui_edit_replace_range(e, 3, 1, cui_string("x")) == CUI_EDIT_BOUNDARY);
    assert(cui_edit_replace_range(e, 0, 99, cui_string("x")) == CUI_EDIT_BOUNDARY);
    assert(cui_edit_replace_range(e, 0, 4, cui_string("\n")) == CUI_EDIT_LINE_BREAK);
    assert(cui_edit_replace_range(e, 0, 4, cui_string("1234567890123456")) == CUI_EDIT_CAPACITY);
    assert(cui_edit_replace_range(e, 0, 4, cui_string("\x80")) == CUI_EDIT_INVALID_UTF8);
    cui_edit_state after = cui_edit_get_state(e);
    assert(after.anchor == before.anchor && after.cursor == before.cursor &&
           after.revision == before.revision && after.can_redo && !after.can_undo);
    assert(cui_edit_redo(e) == CUI_EDIT_OK && !strcmp(b, "reset"));
    assert(cui_edit_replace_range(e, 0, 5, (cui_str){b + 1, 3}) == CUI_EDIT_OK);
    assert(!strcmp(b, "ese"));
    assert(cui_edit_undo(e) == CUI_EDIT_OK && !strcmp(b, "reset"));
    cui_edit_destroy(e);
}
static void test_composition_keys(void) {
    char buffer[64] = "ab\ncd";
    cui_edit *edit = make(buffer, sizeof buffer, true, 10);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    cui_edit_state before = cui_edit_get_state(edit);
    const cui_key keys[] = {CUI_KEY_LEFT,  CUI_KEY_RIGHT,     CUI_KEY_HOME,
                            CUI_KEY_END,   CUI_KEY_BACKSPACE, CUI_KEY_DELETE,
                            CUI_KEY_ENTER, CUI_KEY_A,         CUI_KEY_Z};
    for (unsigned flag = 0; flag < 2; flag++)
        for (size_t i = 0; i < sizeof keys / sizeof keys[0]; i++) {
            assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                          .key = keys[i],
                                                          .modifiers = CUI_MOD_CTRL,
                                                          .ime_composing = flag == 0,
                                                          .prefer_character_input = flag == 1}) ==
                   CUI_EDIT_OK);
            cui_edit_state after = cui_edit_get_state(edit);
            assert(after.cursor == before.cursor && after.anchor == before.anchor &&
                   after.revision == before.revision && !strcmp(buffer, "ab\ncd"));
        }
    assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_TEXT, .text = "é"}) ==
           CUI_EDIT_OK);
    assert(!strcmp(buffer, "aéb\ncd"));
    assert(cui_edit_apply_event(
               edit, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 1);
    cui_edit_destroy(edit);
}
static void test_word_and_vertical_navigation(void) {
    char buffer[128] = "one,  two café_42 😀 三个";
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    assert(edit);
    size_t two = (size_t)(strstr(buffer, "two") - buffer);
    size_t cafe = (size_t)(strstr(buffer, "café") - buffer);
    size_t chinese = (size_t)(strstr(buffer, "三个") - buffer);
    uint64_t revision = cui_edit_get_state(edit).revision;
    assert(cui_edit_move(edit, CUI_EDIT_WORD_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == chinese);
    assert(cui_edit_move(edit, CUI_EDIT_WORD_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == cafe);
    assert(cui_edit_move(edit, CUI_EDIT_WORD_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == two);
    assert(cui_edit_move(edit, CUI_EDIT_WORD_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 0);
    assert(cui_edit_move(edit, CUI_EDIT_WORD_NEXT, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == two);
    assert(cui_edit_move(edit, CUI_EDIT_WORD_NEXT, true) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == cafe && cui_edit_get_state(edit).anchor == two);
    assert(cui_edit_move(edit, CUI_EDIT_WORD_NEXT, true) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == chinese && cui_edit_get_state(edit).anchor == two);
    assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}) ==
           CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 0);
    assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                  .key = CUI_KEY_DOWN,
                                                  .modifiers = CUI_MOD_SHIFT}) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == strlen(buffer) &&
           cui_edit_get_state(edit).anchor == 0);
    assert(cui_edit_get_state(edit).revision == revision);
    cui_edit_destroy(edit);

    strcpy(buffer, "abcde\nx\nABCDE");
    edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    assert(edit && cui_edit_select(edit, 4, 4) == CUI_EDIT_OK);
    assert(cui_edit_move(edit, CUI_EDIT_LINE_NEXT, true) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 7 && cui_edit_get_state(edit).anchor == 4);
    assert(cui_edit_move(edit, CUI_EDIT_LINE_NEXT, true) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 12 && cui_edit_get_state(edit).anchor == 4);
    assert(cui_edit_move(edit, CUI_EDIT_LINE_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 7);
    assert(cui_edit_move(edit, CUI_EDIT_LINE_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 4);
    assert(cui_edit_move(edit, CUI_EDIT_LINE_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 0);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    assert(cui_edit_move(edit, CUI_EDIT_LINE_NEXT, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 7);
    assert(cui_edit_move(edit, CUI_EDIT_LINE_NEXT, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 9);
    assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                  .key = CUI_KEY_DOWN,
                                                  .modifiers = CUI_MOD_CTRL}) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == strlen(buffer));
    assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                  .key = CUI_KEY_UP,
                                                  .modifiers = CUI_MOD_CTRL}) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 0);
    cui_edit_destroy(edit);

    strcpy(buffer, "é好z\nq\nÉ好Z");
    edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    assert(edit && cui_edit_select(edit, 5, 5) == CUI_EDIT_OK);
    assert(cui_edit_move(edit, CUI_EDIT_LINE_NEXT, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 8);
    assert(cui_edit_move(edit, CUI_EDIT_LINE_NEXT, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 14);
    assert(cui_edit_move(edit, (cui_edit_motion)99, false) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_get_state(edit).cursor == 14);
    cui_edit_destroy(edit);
}

#include "cui_edit_grapheme_tests.inc"
static void test_grapheme_conformance(void) {
    for (size_t i = 0; i < sizeof grapheme_cases / sizeof grapheme_cases[0]; i++) {
        cui_str text = {grapheme_cases[i].text, grapheme_cases[i].length};
        size_t count = grapheme_cases[i].count;
        cui_grapheme_iterator iterator;
        assert(cui_grapheme_iterator_init(&iterator, text));
        for (size_t j = 1; j < count; j++)
            assert(cui_grapheme_iterator_next(&iterator) == grapheme_cases[i].offsets[j]);
        assert(cui_grapheme_iterator_next(&iterator) == text.length);
        for (size_t j = 0; j < count; j++) {
            size_t at = grapheme_cases[i].offsets[j];
            assert(cui_grapheme_boundary(text, at));
            /* Seeding at any proven boundary has the same cluster suffix. */
            iterator = (cui_grapheme_iterator){text, at};
            assert(cui_grapheme_iterator_next(&iterator) ==
                   grapheme_cases[i].offsets[j + (j + 1 < count)]);
            assert(cui_grapheme_next(text, at) == grapheme_cases[i].offsets[j + (j + 1 < count)]);
            assert(cui_grapheme_previous(text, at) == grapheme_cases[i].offsets[j - (j > 0)]);
        }
        for (size_t at = 0; at <= text.length; at++) {
            bool expected = false;
            for (size_t j = 0; j < count; j++)
                expected |= grapheme_cases[i].offsets[j] == at;
            assert(cui_grapheme_boundary(text, at) == expected);
        }
    }
    assert(cui_grapheme_next((cui_str){0}, 0) == 0);
    assert(cui_grapheme_previous((cui_str){0}, 0) == 0);
    assert(cui_grapheme_boundary((cui_str){0}, 0));
    const cui_str invalid[] = {
        {NULL, 1}, {"\xc0\x80", 2}, {"\xed\xa0\x80", 3}, {"a\xf4\x90\x80\x80", 5}, {"a\xc2", 2}};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        assert(cui_grapheme_next(invalid[i], 0) == SIZE_MAX);
        assert(cui_grapheme_previous(invalid[i], 0) == SIZE_MAX);
        assert(!cui_grapheme_boundary(invalid[i], 0));
    }
    cui_str text = (cui_str){"é", 2};
    assert(cui_grapheme_next(text, 1) == SIZE_MAX);
    assert(cui_grapheme_previous(text, 1) == SIZE_MAX);
    assert(cui_grapheme_next(text, 3) == SIZE_MAX);
}
static void test_grapheme_editing(void) {
    const char *clusters[] = {"é", "👩🏽‍💻", "🇨🇭", "क्‍ष", "\r\n", "각"};
    for (size_t i = 0; i < sizeof clusters / sizeof clusters[0]; i++) {
        char b[128];
        snprintf(b, sizeof b, "A%sZ", clusters[i]);
        size_t end = 1 + strlen(clusters[i]);
        cui_edit *e = make(b, sizeof b, true, 10);
        assert(cui_edit_select(e, 1, 1) == CUI_EDIT_OK);
        assert(cui_edit_apply_event(e, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                   .key = CUI_KEY_RIGHT,
                                                   .modifiers = CUI_MOD_SHIFT}) == CUI_EDIT_OK);
        cui_edit_state state = cui_edit_get_state(e);
        assert(state.cursor == end && state.anchor == 1);
        assert(cui_edit_apply_event(e, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                   .key = CUI_KEY_BACKSPACE}) == CUI_EDIT_OK);
        assert(strcmp(b, "AZ") == 0);
        assert(cui_edit_undo(e) == CUI_EDIT_OK);
        state = cui_edit_get_state(e);
        assert(state.cursor == end && state.anchor == 1);
        assert(cui_edit_select(e, end, end) == CUI_EDIT_OK);
        assert(cui_edit_backspace_grapheme(e) == CUI_EDIT_OK);
        assert(strcmp(b, "AZ") == 0);
        assert(cui_edit_undo(e) == CUI_EDIT_OK);
        assert(cui_edit_select(e, 1, 1) == CUI_EDIT_OK);
        assert(cui_edit_delete_grapheme(e) == CUI_EDIT_OK);
        assert(strcmp(b, "AZ") == 0);
        cui_edit_destroy(e);
    }
    char b[64] = "AéZ";
    cui_edit *e = make(b, sizeof b, false, 10);
    assert(cui_edit_select(e, 2, 2) == CUI_EDIT_OK);
    assert(cui_edit_move(e, CUI_EDIT_GRAPHEME_NEXT, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).cursor == 4);
    assert(cui_edit_move(e, CUI_EDIT_GRAPHEME_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).cursor == 1);
    /* Scalar APIs deliberately remain available. */
    assert(cui_edit_move(e, CUI_EDIT_NEXT, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).cursor == 2);
    assert(cui_edit_select(e, 2, 4) == CUI_EDIT_OK);
    assert(cui_edit_backspace_grapheme(e) == CUI_EDIT_OK);
    assert(strcmp(b, "AZ") == 0);
    assert(cui_edit_undo(e) == CUI_EDIT_OK);
    assert(cui_edit_get_state(e).anchor == 2 && cui_edit_get_state(e).cursor == 4);
    assert(cui_edit_select(e, 2, 2) == CUI_EDIT_OK);
    assert(cui_edit_delete_grapheme(e) == CUI_EDIT_OK);
    assert(strcmp(b, "AZ") == 0);
    assert(cui_edit_undo(e) == CUI_EDIT_OK);
    assert(cui_edit_select(e, 2, 3) == CUI_EDIT_BOUNDARY);
    assert(strcmp(b, "AéZ") == 0);
    assert(cui_edit_select(e, 4, 4) == CUI_EDIT_OK);
    assert(cui_edit_backspace(e) == CUI_EDIT_OK);
    assert(strcmp(b, "AeZ") == 0);
    assert(cui_edit_undo(e) == CUI_EDIT_OK);
    assert(cui_edit_select(e, 2, 2) == CUI_EDIT_OK);
    assert(cui_edit_backspace_grapheme(e) == CUI_EDIT_OK);
    assert(strcmp(b, "AZ") == 0);
    assert(cui_edit_delete_grapheme(NULL) == CUI_EDIT_INVALID_ARGUMENT);
    cui_edit_destroy(e);
}

#include "cui_edit_word_tests.inc"
static void test_word_conformance(void) {
    for (size_t i = 0; i < sizeof word_cases / sizeof word_cases[0]; i++) {
        cui_str text = {word_cases[i].text, word_cases[i].length};
        size_t count = word_cases[i].count;
        for (size_t at = 0; at <= text.length; at++) {
            bool expected = false;
            for (size_t j = 0; j < count; j++)
                expected |= word_cases[i].offsets[j] == at;
            if (cui_word_boundary(text, at) != expected) {
                fprintf(stderr, "word conformance case %zu offset %zu expected %d\n", i, at,
                        expected);
                assert(false);
            }
        }
        for (size_t j = 0; j < count; j++) {
            size_t at = word_cases[i].offsets[j];
            assert(cui_word_next(text, at) == word_cases[i].offsets[j + (j + 1 < count)]);
            assert(cui_word_previous(text, at) == word_cases[i].offsets[j - (j > 0)]);
        }
    }
    assert(cui_word_next((cui_str){0}, 0) == 0);
    assert(cui_word_boundary((cui_str){0}, 0));
    const cui_str invalid[] = {{NULL, 1}, {"a\xc2", 2}, {"\xed\xa0\x80", 3}};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; i++) {
        assert(cui_word_next(invalid[i], 0) == SIZE_MAX);
        assert(cui_word_previous(invalid[i], 0) == SIZE_MAX);
        assert(!cui_word_boundary(invalid[i], 0));
    }
    assert(cui_word_next((cui_str){"é", 2}, 1) == SIZE_MAX);
    assert(cui_word_previous((cui_str){"é", 2}, 3) == SIZE_MAX);
}
static void test_unicode_word_events(void) {
    const char *tokens[] = {"can't",    "3.14", "foo_bar",         "אב\"גד",
                            "カタカナ", "é",    "👩🏽‍💻", "क्‍ष"};
    for (size_t i = 0; i < sizeof tokens / sizeof tokens[0]; i++) {
        char buffer[128];
        snprintf(buffer, sizeof buffer, "  %s  ", tokens[i]);
        cui_edit *edit = make(buffer, sizeof buffer, false, 16);
        size_t end = 2 + strlen(tokens[i]);
        cui_edit_select(edit, 0, 0);
        assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                      .key = CUI_KEY_RIGHT,
                                                      .modifiers = CUI_MOD_CTRL | CUI_MOD_SHIFT}) ==
               CUI_EDIT_OK);
        assert(cui_edit_get_state(edit).cursor == end && cui_edit_get_state(edit).anchor == 0);
        assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                      .key = CUI_KEY_LEFT,
                                                      .modifiers = CUI_MOD_ALT}) == CUI_EDIT_OK);
        assert(cui_edit_get_state(edit).cursor == 2 && cui_edit_get_state(edit).anchor == 2);
        assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                      .key = CUI_KEY_DELETE,
                                                      .modifiers = CUI_MOD_CTRL}) == CUI_EDIT_OK);
        assert(!strcmp(buffer, "    "));
        assert(cui_edit_undo(edit) == CUI_EDIT_OK);
        assert(cui_edit_get_state(edit).cursor == 2);
        assert(cui_edit_select(edit, end, end) == CUI_EDIT_OK);
        assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                      .key = CUI_KEY_BACKSPACE,
                                                      .modifiers = CUI_MOD_ALT}) == CUI_EDIT_OK);
        assert(!strcmp(buffer, "    "));
        assert(cui_edit_undo(edit) == CUI_EDIT_OK);
        assert(cui_edit_get_state(edit).cursor == end);
        cui_edit_destroy(edit);
    }
    char buffer[128] = "one,  two";
    cui_edit *edit = make(buffer, sizeof buffer, false, 16);
    const size_t targets[] = {3, 4, 9};
    cui_edit_select(edit, 0, 0);
    for (size_t i = 0; i < 3; i++) {
        assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                      .key = CUI_KEY_RIGHT,
                                                      .modifiers = CUI_MOD_CTRL}) == CUI_EDIT_OK);
        assert(cui_edit_get_state(edit).cursor == targets[i]);
    }
    const size_t backwards[] = {6, 3, 0};
    for (size_t i = 0; i < 3; i++) {
        assert(cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_PREVIOUS, false) == CUI_EDIT_OK);
        assert(cui_edit_get_state(edit).cursor == backwards[i]);
    }
    const char *remaining[] = {",  two", "  two", ""};
    for (size_t i = 0; i < 3; i++) {
        assert(cui_edit_delete_word(edit) == CUI_EDIT_OK);
        assert(!strcmp(buffer, remaining[i]));
    }
    for (size_t i = 0; i < 3; i++)
        assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "one,  two") && cui_edit_get_state(edit).cursor == 0);
    cui_edit_select(edit, 9, 9);
    assert(cui_edit_backspace_word(edit) == CUI_EDIT_OK && !strcmp(buffer, "one,  "));
    assert(cui_edit_backspace_word(edit) == CUI_EDIT_OK && !strcmp(buffer, "one"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && cui_edit_get_state(edit).cursor == 6);
    cui_edit_destroy(edit);
    /* Ideographs are separate default segments, not a dictionary word. */
    strcpy(buffer, "三个");
    edit = make(buffer, sizeof buffer, false, 16);
    cui_edit_select(edit, 0, 0);
    cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_NEXT, false);
    assert(cui_edit_get_state(edit).cursor == 3);
    cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_NEXT, false);
    assert(cui_edit_get_state(edit).cursor == 6);
    cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_PREVIOUS, false);
    assert(cui_edit_get_state(edit).cursor == 3);
    cui_edit_destroy(edit);
    /* Unicode whitespace is skipped, while combining whitespace segments are
     * non-whitespace, matching the frozen trim_start predicate. */
    strcpy(buffer, "\xc2\xa0"
                   "word");
    edit = make(buffer, sizeof buffer, false, 16);
    cui_edit_select(edit, 0, 0);
    cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_NEXT, false);
    assert(cui_edit_get_state(edit).cursor == 6);
    cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_PREVIOUS, false);
    assert(cui_edit_get_state(edit).cursor == 2);
    cui_edit_destroy(edit);
    /* The frozen reference segments the prefix/suffix sliced at the cursor,
     * rather than simply consulting boundaries in the entire document. */
    strcpy(buffer, "can't");
    edit = make(buffer, sizeof buffer, false, 16);
    cui_edit_select(edit, 4, 4);
    cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_PREVIOUS, false);
    assert(cui_edit_get_state(edit).cursor == 3);
    cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_PREVIOUS, false);
    assert(cui_edit_get_state(edit).cursor == 0);
    cui_edit_select(edit, 3, 3);
    cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_NEXT, false);
    assert(cui_edit_get_state(edit).cursor == 4);
    cui_edit_destroy(edit);
    strcpy(buffer, "a ̈b");
    edit = make(buffer, sizeof buffer, false, 16);
    cui_edit_select(edit, 1, 1);
    cui_edit_move(edit, CUI_EDIT_UNICODE_WORD_NEXT, false);
    assert(cui_edit_get_state(edit).cursor == 4);
    cui_edit_destroy(edit);
    strcpy(buffer, "AéZ");
    edit = make(buffer, sizeof buffer, false, 16);
    cui_edit_select(edit, 2, 2);
    assert(cui_edit_delete_word(edit) == CUI_EDIT_OK && !strcmp(buffer, "AZ"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && cui_edit_get_state(edit).cursor == 2);
    cui_edit_select(edit, 2, 4);
    assert(cui_edit_backspace_word(edit) == CUI_EDIT_OK && !strcmp(buffer, "AZ"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 2 && cui_edit_get_state(edit).cursor == 4);
    assert(cui_edit_delete_word(NULL) == CUI_EDIT_INVALID_ARGUMENT);
    cui_edit_destroy(edit);
}
static void test_inline_content(void) {
    cui_edit_token_span spans[] = {
        {3, 13, {cui_string("user42"), cui_string("@Mary Jane"), cui_string("Mary")}}};
    cui_edit_content content = {cui_string("Hi @Mary Jane!"), spans, 1};
    assert(cui_edit_content_valid(content));
    size_t start = 777, end = 888;
    assert(cui_edit_token_boundary(content, 5, false, &start) == CUI_EDIT_OK && start == 3);
    assert(cui_edit_token_boundary(content, 5, true, &end) == CUI_EDIT_OK && end == 13);
    assert(cui_edit_token_boundary(content, 3, true, &end) == CUI_EDIT_OK && end == 3);
    assert(cui_edit_token_boundary(content, 13, false, &start) == CUI_EDIT_OK && start == 13);
    assert(cui_edit_normalize_token_range(content, 5, 8, &start, &end) == CUI_EDIT_OK &&
           start == 3 && end == 13);
    assert(cui_edit_normalize_token_range(content, 5, 5, &start, &end) == CUI_EDIT_OK &&
           start == 13 && end == 13);
    start = 777;
    end = 888;
    assert(cui_edit_normalize_token_range(content, 8, 5, &start, &end) == CUI_EDIT_BOUNDARY);
    assert(start == 777 && end == 888);
    assert(cui_edit_token_boundary(content, 15, true, &start) == CUI_EDIT_BOUNDARY && start == 777);
    cui_edit_token_span adjacent[] = {
        {0, 2, {cui_string("same"), cui_string("@A"), cui_string("A")}},
        {2, 4, {cui_string("same"), cui_string("@B"), cui_string("B")}}};
    assert(cui_edit_content_valid((cui_edit_content){cui_string("@A@B"), adjacent, 2}));
    adjacent[1].start = 1;
    assert(!cui_edit_content_valid((cui_edit_content){cui_string("@A@B"), adjacent, 2}));
    cui_edit_token_span bad = spans[0];
    cui_edit_content invalid = {content.text, &bad, 1};
    bad.start = bad.end;
    assert(!cui_edit_content_valid(invalid));
    bad = spans[0];
    bad.end = SIZE_MAX;
    assert(!cui_edit_content_valid(invalid));
    bad = spans[0];
    bad.token.text = cui_string("mismatch");
    assert(!cui_edit_content_valid(invalid));
    bad = spans[0];
    bad.token.id = cui_string(" \xc2\xa0");
    assert(!cui_edit_content_valid(invalid));
    const char *labels[] = {"", "bad\n", "\xc2\x85", "\xe2\x80\xa8", "\xe2\x80\xa9", "\xc0\x80"};
    for (size_t i = 0; i < sizeof labels / sizeof labels[0]; i++) {
        bad = spans[0];
        bad.token.label = cui_string(labels[i]);
        assert(!cui_edit_content_valid(invalid));
    }
    bad = (cui_edit_token_span){1, 2, {cui_string("id"), cui_string("e"), cui_string("e")}};
    assert(!cui_edit_content_valid((cui_edit_content){cui_string("AéZ"), &bad, 1}));
    assert(!cui_edit_content_valid((cui_edit_content){content.text, NULL, 1}));
    assert(!cui_edit_content_valid((cui_edit_content){content.text, spans, 65537}));
    assert(cui_edit_content_valid((cui_edit_content){0}));
    char buffer[128] = "old";
    cui_edit *edit = make(buffer, sizeof buffer, true, 16);
    assert(cui_edit_set_content(edit, content) == CUI_EDIT_OK && !strcmp(buffer, "Hi @Mary Jane!"));
    cui_edit_content snapshot = cui_edit_get_content(edit);
    assert(snapshot.tokens == spans && snapshot.token_count == 1 && snapshot.text.data == buffer);
    uint64_t revision = cui_edit_get_state(edit).revision;
    assert(cui_edit_select(edit, 5, 5) == CUI_EDIT_OK); /* exact scalar API survives */
    assert(cui_edit_select_atomic(edit, 5, 5) == CUI_EDIT_OK &&
           cui_edit_get_state(edit).cursor == 3);
    assert(cui_edit_select_atomic(edit, 8, 5) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 13 && cui_edit_get_state(edit).cursor == 3);
    assert(cui_edit_select(edit, 3, 3) == CUI_EDIT_OK);
    assert(cui_edit_move(edit, CUI_EDIT_NEXT, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 4); /* explicit scalar compatibility */
    assert(cui_edit_move(edit, CUI_EDIT_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 3);
    assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                  .key = CUI_KEY_RIGHT,
                                                  .modifiers = CUI_MOD_CTRL}) == CUI_EDIT_OK &&
           cui_edit_get_state(edit).cursor == 13);
    assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                  .key = CUI_KEY_LEFT,
                                                  .modifiers = CUI_MOD_CTRL}) == CUI_EDIT_OK &&
           cui_edit_get_state(edit).cursor == 3);
    assert(cui_edit_apply_event(edit, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                                  .key = CUI_KEY_RIGHT,
                                                  .modifiers = CUI_MOD_SHIFT}) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).cursor == 13 && cui_edit_get_state(edit).anchor == 3);
    assert(cui_edit_get_state(edit).revision == revision);
    assert(cui_edit_delete_word(edit) == CUI_EDIT_OK && !strcmp(buffer, "Hi !"));
    assert(cui_edit_get_content(edit).token_count == 0);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "Hi @Mary Jane!"));
    assert(cui_edit_get_state(edit).cursor == 13 && cui_edit_get_state(edit).anchor == 3);
    assert(cui_edit_get_content(edit).token_count == 0); /* no token-history claim */
    assert(cui_edit_set_tokens(edit, spans, 1) == CUI_EDIT_OK);
    cui_edit_select(edit, 5, 5);
    assert(cui_edit_insert(edit, cui_string("?")) == CUI_EDIT_OK &&
           !strcmp(buffer, "Hi @Mary Jane?!"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && cui_edit_get_state(edit).cursor == 5);
    assert(cui_edit_set_tokens(edit, spans, 1) == CUI_EDIT_OK);
    assert(cui_edit_replace_range(edit, 5, 8, cui_string("X")) == CUI_EDIT_OK &&
           !strcmp(buffer, "Hi X!"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && cui_edit_get_state(edit).cursor == 5);
    assert(cui_edit_set_tokens(edit, spans, 1) == CUI_EDIT_OK);
    assert(cui_edit_indent(edit, false, 2, false) == CUI_EDIT_INVALID_ARGUMENT);
    bad = spans[0];
    bad.end = SIZE_MAX;
    assert(cui_edit_set_tokens(edit, &bad, 1) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_get_content(edit).tokens == spans);
    assert(cui_edit_set_content(edit, (cui_edit_content){content.text, &bad, 1}) ==
           CUI_EDIT_INVALID_ARGUMENT);
    assert(!strcmp(buffer, "Hi @Mary Jane!") && cui_edit_get_content(edit).tokens == spans);
    assert(cui_edit_set_tokens(edit, NULL, 0) == CUI_EDIT_OK);
    assert(cui_edit_get_content(edit).token_count == 0);
    cui_edit_destroy(edit);
    char small[16] = "Hi @Mary Jane!";
    edit = make(small, sizeof small, true, 16);
    cui_edit_select(edit, 5, 5);
    assert(cui_edit_set_tokens(edit, spans, 1) == CUI_EDIT_OK);
    cui_edit_state before = cui_edit_get_state(edit);
    assert(cui_edit_insert(edit, cui_string("long")) == CUI_EDIT_CAPACITY);
    cui_edit_token_span large_span = {6, 16, spans[0].token};
    assert(cui_edit_set_content(edit, (cui_edit_content){cui_string("XX Hi @Mary Jane!"),
                                                         &large_span, 1}) == CUI_EDIT_CAPACITY);
    cui_edit_state after = cui_edit_get_state(edit);
    assert(before.cursor == after.cursor && before.anchor == after.anchor &&
           before.revision == after.revision);
    assert(!after.can_undo && !strcmp(small, "Hi @Mary Jane!") &&
           cui_edit_get_content(edit).tokens == spans);
    assert(cui_edit_insert(edit, (cui_str){"\xc0", 1}) == CUI_EDIT_INVALID_UTF8);
    /* Simulated invalid live annotation storage rejects before any edit. */
    spans[0].end = SIZE_MAX;
    assert(cui_edit_delete_word(edit) == CUI_EDIT_INVALID_ARGUMENT);
    assert(!strcmp(small, "Hi @Mary Jane!") && cui_edit_get_state(edit).cursor == 5);
    assert(cui_edit_set_tokens(edit, NULL, 0) == CUI_EDIT_OK);
    spans[0].end = 13;
    cui_edit_destroy(edit);
    strcpy(buffer, "@Mary Jane!");
    edit = make(buffer, sizeof buffer, true, 16);
    cui_edit_token_span alias = {
        0, 10, {cui_string("id"), (cui_str){buffer, 10}, cui_string("Mary")}};
    assert(cui_edit_set_content(edit, (cui_edit_content){(cui_str){buffer, 10}, &alias, 1}) ==
           CUI_EDIT_INVALID_ARGUMENT);
    assert(!strcmp(buffer, "@Mary Jane!") && cui_edit_get_state(edit).revision == 0);
    cui_edit_destroy(edit);
    /* A host may attach a sub-buffer; reject metadata beginning before it but
     * overlapping mutable edit storage, not only pointers beginning inside. */
    strcpy(buffer, "@Mary Jane!");
    edit = make(buffer + 3, sizeof buffer - 3, true, 16);
    alias =
        (cui_edit_token_span){0, 10, {cui_string("id"), (cui_str){buffer, 10}, cui_string("Mary")}};
    assert(cui_edit_set_content(edit, (cui_edit_content){cui_string(buffer), &alias, 1}) ==
           CUI_EDIT_INVALID_ARGUMENT);
    assert(!strcmp(buffer, "@Mary Jane!") && cui_edit_get_state(edit).revision == 0);
    cui_edit_destroy(edit);
}
static void test_token_replacement(void) {
    char buffer[64] = "A @one @two Z";
    cui_edit *edit = make(buffer, sizeof buffer, true, 16);
    cui_edit_token_span spans[2] = {
        {2, 6, {cui_string("1"), cui_string("@one"), cui_string("One")}},
        {7, 11, {cui_string("2"), cui_string("@two"), cui_string("Two")}}};
    assert(cui_edit_set_tokens(edit, spans, 2) == CUI_EDIT_OK);
    cui_edit_token_span output[4];
    memset(output, 0x55, sizeof output);
    cui_edit_token_span saved[4];
    memcpy(saved, output, sizeof output);
    size_t count = 99;
    cui_edit_inline_token token = {cui_string("3"), cui_string("@x"), cui_string("X")};
    assert(cui_edit_replace_range_with_token(edit, 3, 4, token, output, 1, &count) ==
           CUI_EDIT_CAPACITY);
    assert(count == 99 && !memcmp(saved, output, sizeof output) &&
           !strcmp(buffer, "A @one @two Z"));
    assert(cui_edit_replace_range_with_token(edit, 3, 4, token, spans, 2, &count) ==
           CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_replace_range_with_token(edit, 3, 4, token, output, 4, &count) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "A @x @two Z") && count == 2);
    assert(output[0].start == 2 && output[0].end == 4 && output[1].start == 5 &&
           output[1].end == 9);
    assert(cui_edit_content_valid(cui_edit_get_content(edit)));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "A @one @two Z"));
    assert(cui_edit_get_content(edit).token_count == 0);
    assert(cui_edit_set_tokens(edit, spans, 2) == CUI_EDIT_OK);
    cui_edit_select(edit, 0, 1);
    uint64_t revision = cui_edit_get_state(edit).revision;
    assert(cui_edit_replace_range_with_token(edit, 2, 6, spans[0].token, output, 4, &count) ==
           CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).anchor == 0 && cui_edit_get_state(edit).cursor == 1 &&
           cui_edit_get_state(edit).revision == revision);
    cui_edit_select(edit, 3, 4);
    assert(cui_edit_replace_with_token(edit, token, spans, 2, &count) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "A @x @two Z"));
    cui_edit_destroy(edit);
    strcpy(buffer, "a");
    edit = make(buffer, sizeof buffer, true, 16);
    token.text = cui_string("\xcc\x81");
    count = 99;
    memcpy(output, saved, sizeof output);
    assert(cui_edit_replace_range_with_token(edit, 1, 1, token, output, 4, &count) ==
           CUI_EDIT_BOUNDARY);
    assert(count == 99 && !memcmp(output, saved, sizeof output) && !strcmp(buffer, "a") &&
           cui_edit_get_state(edit).revision == 0);
    cui_edit_destroy(edit);
}
static void test_multiple_selections(void) {
    char buffer[64] = "abc def ghi";
    cui_edit *edit = make(buffer, sizeof buffer, true, 4);
    assert(cui_edit_insert(edit, cui_string("!")) == CUI_EDIT_OK);
    cui_edit_selection storage[8];
    memset(storage, 0x55, sizeof storage);
    cui_edit_selection_snapshot history[5];
    memset(history, 0x55, sizeof history);
    assert(cui_edit_selection_history_slots(edit) == 5);
    assert(cui_edit_attach_selections(edit, storage, 8, history, 4) == CUI_EDIT_INVALID_ARGUMENT);
    assert((unsigned char)storage[0].id == 0x55 && (unsigned char)history[0].count == 0x55);
    assert(cui_edit_attach_selections(edit, (cui_edit_selection *)(void *)buffer, 1, history, 5) ==
           CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_attach_selections(edit, storage, 8, history, 5) == CUI_EDIT_OK);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abc def ghi") &&
           storage[0].id == 0 && storage[0].cursor == 11); /* pre-attachment history */
    size_t count = 0;
    assert(cui_edit_get_selections(edit, &count) == storage && count == 1);
    cui_edit_selection merged[3] = {{9, 4, 2}, {2, 0, 2}, {5, 6, 7}};
    assert(cui_edit_select_many(edit, merged, 3) == CUI_EDIT_OK);
    cui_edit_get_selections(edit, &count);
    assert(count == 2 && storage[0].id == 9 && storage[0].anchor == 4 && storage[0].cursor == 0);
    cui_edit_selection input[3] = {{9, 2, 1}, {7, 4, 5}, {3, 8, 8}};
    assert(cui_edit_select_many(edit, input, 3) == CUI_EDIT_OK);
    cui_edit_selection saved[8];
    memcpy(saved, storage, sizeof storage);
    cui_edit_selection_snapshot saved_history[5];
    memcpy(saved_history, history, sizeof history);
    uint64_t revision = cui_edit_get_state(edit).revision;
    cui_edit_selection duplicate[2] = {{1, 0, 0}, {1, 2, 2}};
    assert(cui_edit_select_many(edit, duplicate, 2) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_replace_selections(edit, cui_string("abcdefghijklmnopqrstuvwxyz")) ==
           CUI_EDIT_CAPACITY);
    assert(!memcmp(saved, storage, sizeof storage) &&
           !memcmp(saved_history, history, sizeof history) && !strcmp(buffer, "abc def ghi") &&
           cui_edit_get_state(edit).revision == revision);
    assert(cui_edit_set_tokens(edit, NULL, 0) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_replace_selections(edit, cui_string("X")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "aXc Xef Xghi") && storage[0].id == 9 && storage[0].cursor == 2);
    cui_edit_get_selections(edit, &count);
    assert(count == 3);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abc def ghi"));
    assert(storage[0].id == 9 && storage[0].anchor == 2 && storage[0].cursor == 1 &&
           storage[1].id == 7 && storage[2].id == 3);
    assert(cui_edit_redo(edit) == CUI_EDIT_OK && !strcmp(buffer, "aXc Xef Xghi"));
    assert(storage[0].anchor == 2 && storage[0].cursor == 2 && storage[2].cursor == 9);
    for (unsigned i = 0; i < 6; ++i)
        assert(cui_edit_replace_selections(edit, cui_string("x")) == CUI_EDIT_OK);
    unsigned undos = 0;
    while (cui_edit_undo(edit) == CUI_EDIT_OK) {
        cui_edit_get_selections(edit, &count);
        assert(count == 3 && storage[0].id == 9);
        ++undos;
    }
    assert(undos == 4); /* paired selection snapshots survive document-history eviction */
    while (cui_edit_redo(edit) == CUI_EDIT_OK) {
        cui_edit_get_selections(edit, &count);
        assert(count == 3);
    }
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    cui_edit_get_selections(edit, &count);
    assert(count == 1 && storage[0].id == 9);
    assert(cui_edit_insert(edit, cui_string("!")) == CUI_EDIT_OK);
    cui_edit_detach_selections(edit);
    assert(!cui_edit_get_selections(edit, &count) && !count);
    assert(cui_edit_set_tokens(edit, NULL, 0) == CUI_EDIT_OK);
    cui_edit_destroy(edit);
    strcpy(buffer, "a\xcc\x81 b");
    edit = make(buffer, sizeof buffer, true, 4);
    assert(cui_edit_attach_selections(edit, storage, 8, history, 5) == CUI_EDIT_OK);
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK); /* legacy scalar position remains legal */
    assert(cui_edit_replace_selections(edit, cui_string("x")) == CUI_EDIT_BOUNDARY);
    cui_edit_selection invalid = {1, 1, 1};
    assert(cui_edit_select_many(edit, &invalid, 1) == CUI_EDIT_BOUNDARY);
    cui_edit_selection unicode[2] = {{8, 3, 0}, {9, 4, 5}};
    assert(cui_edit_select_many(edit, unicode, 2) == CUI_EDIT_OK);
    assert(cui_edit_replace_selections(edit, cui_string("\xf0\x9f\x98\x80")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "\xf0\x9f\x98\x80 \xf0\x9f\x98\x80"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "a\xcc\x81 b") &&
           storage[0].anchor == 3);
    cui_edit_destroy(edit);
    strcpy(buffer, "abc");
    edit = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    assert(edit && !cui_edit_selection_history_slots(edit));
    assert(cui_edit_attach_selections(edit, storage, 1, NULL, 0) == CUI_EDIT_OK);
    cui_edit_selection two[2] = {{1, 0, 0}, {2, 2, 2}};
    assert(cui_edit_select_many(edit, two, 2) == CUI_EDIT_CAPACITY);
    assert(cui_edit_undo(edit) == CUI_EDIT_NO_HISTORY);
    cui_edit_destroy(edit);
    char large[256];
    memset(large, 'a', 128);
    large[128] = 0;
    edit = cui_edit_create(
        (cui_edit_config){.buffer = large, .capacity = sizeof large, .multiline = true});
    cui_edit_selection all[64], many[64];
    assert(edit && cui_edit_attach_selections(edit, all, 64, NULL, 0) == CUI_EDIT_OK);
    for (size_t i = 0; i < 64; ++i)
        many[i] = (cui_edit_selection){63 - i, (63 - i) * 2, (63 - i) * 2};
    assert(cui_edit_select_many(edit, many, 64) == CUI_EDIT_OK);
    assert(cui_edit_replace_selections(edit, cui_string("x")) == CUI_EDIT_OK);
    cui_edit_get_selections(edit, &count);
    assert(count == 64 && all[0].id == 63 && all[0].cursor == 190 &&
           cui_edit_get_text(edit).length == 192);
    cui_edit_destroy(edit);
}
static void test_multi_motion_and_erasure(void) {
    char buffer[128] = "one two three";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 4096,
                                                       .max_history_steps = 4});
    cui_edit_selection live[3];
    cui_edit_selection_snapshot history[5];
    assert(edit && cui_edit_attach_selections(edit, live, 3, history, 5) == CUI_EDIT_OK);
    cui_edit_selection input[2] = {{11, 3, 3}, {22, 7, 7}};
    assert(cui_edit_select_many(edit, input, 2) == CUI_EDIT_OK);
    cui_edit_selection output = {99, 99, 99};
    assert(cui_edit_selection_motion(edit, input[0], CUI_EDIT_UNICODE_WORD_NEXT, false, &output) ==
           CUI_EDIT_OK);
    assert(output.id == 11 && output.cursor == 7 && live[0].cursor == 3);
    assert(cui_edit_erase_selections(edit, true, true) == CUI_EDIT_OK &&
           !strcmp(buffer, "  three"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "one two three"));
    size_t count;
    cui_edit_get_selections(edit, &count);
    assert(count == 2 && live[0].id == 11 && live[0].cursor == 3 && live[1].cursor == 7);
    assert(cui_edit_move_selections(edit, CUI_EDIT_GRAPHEME_PREVIOUS, true) == CUI_EDIT_OK);
    assert(live[0].anchor == 3 && live[0].cursor == 2);
    assert(cui_edit_erase_selections(edit, false, false) == CUI_EDIT_OK &&
           !strcmp(buffer, "on tw three"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && live[0].anchor == 3 && live[0].cursor == 2);
    cui_edit_selection duplicate = {33, 2, 2};
    assert(cui_edit_add_cursors(edit, &duplicate, 1) == CUI_EDIT_OK);
    cui_edit_get_selections(edit, &count);
    assert(count == 2);
    cui_edit_selection endpoint = {33, 3, 3};
    assert(cui_edit_add_cursors(edit, &endpoint, 1) == CUI_EDIT_OK);
    cui_edit_get_selections(edit, &count);
    assert(count == 3 && live[2].cursor == 3);
    cui_edit_selection overflow = {44, 12, 12};
    cui_edit_selection saved[3];
    memcpy(saved, live, sizeof saved);
    assert(cui_edit_add_cursors(edit, &overflow, 1) == CUI_EDIT_CAPACITY);
    assert(!memcmp(saved, live, sizeof saved) && !strcmp(buffer, "one two three"));
    assert(cui_edit_replace_selections(edit, cui_string("!")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "on!! tw! three"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !memcmp(saved, live, sizeof saved));
    cui_edit_destroy(edit);
}
static void test_multi_indent_and_clipboard(void) {
    char buffer[128] = "1\n2\n3";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 4096,
                                                       .max_history_steps = 4});
    cui_edit_selection live[8], input[] = {{31, 4, 4}, {11, 0, 0}, {21, 2, 2}};
    cui_edit_selection_snapshot history[5];
    assert(edit && cui_edit_attach_selections(edit, live, 8, history, 5) == CUI_EDIT_OK);
    assert(cui_edit_select_many(edit, input, 3) == CUI_EDIT_OK);
    assert(cui_edit_indent_selections(edit, false, false, 2, false) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "  1\n  2\n  3") && live[0].cursor == 10 && live[1].cursor == 6 &&
           live[2].cursor == 2);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "1\n2\n3"));
    assert(live[0].id == 31 && live[0].cursor == 4);
    assert(cui_edit_redo(edit) == CUI_EDIT_OK);
    assert(cui_edit_indent_selections(edit, true, false, 2, false) == CUI_EDIT_OK &&
           !strcmp(buffer, "1\n2\n3"));
    assert(cui_edit_paste_selections(edit, cui_string("x\ny\nz")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "x1\ny2\nz3") && live[0].cursor == 7 && live[1].cursor == 4 &&
           live[2].cursor == 1);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "1\n2\n3"));
    assert(cui_edit_paste_selections(edit, cui_string("x\ny\n")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "x1\ny2\n3") && live[0].cursor == 6);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    assert(cui_edit_paste_selections(edit, cui_string("x\ny")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "x\ny1\nx\ny2\nx\ny3"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    cui_edit_selection selected[] = {{31, 5, 4}, {11, 0, 1}, {21, 2, 2}};
    assert(cui_edit_select_many(edit, selected, 3) == CUI_EDIT_OK);
    char copy[32] = "untouched";
    cui_str output = cui_string("sentinel");
    assert(cui_edit_copy_selections(edit, copy, 3, &output) == CUI_EDIT_CAPACITY);
    assert(!strcmp(copy, "untouched") && output.length == 8);
    assert(cui_edit_copy_selections(edit, (char *)(void *)history, sizeof history, &output) ==
           CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_copy_selections(edit, copy, sizeof copy, &output) == CUI_EDIT_OK &&
           !strcmp(copy, "1\n3"));
    cui_edit_selection saved[3];
    memcpy(saved, live, sizeof saved);
    assert(cui_edit_paste_selections(edit, (cui_str){0}) == CUI_EDIT_OK &&
           !memcmp(saved, live, sizeof saved));
    assert(cui_edit_can_replace_selections(edit, cui_string("a")) == CUI_EDIT_OK &&
           !memcmp(saved, live, sizeof saved));
    /* Any selected range makes every selection a block; shared rows edit once. */
    assert(cui_edit_indent_selections(edit, false, false, 2, true) == CUI_EDIT_OK &&
           !strcmp(buffer, "\t1\n\t2\n\t3"));
    assert(live[0].anchor == 8 && live[0].cursor == 7);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !memcmp(saved, live, sizeof saved));
    assert(cui_edit_select(edit, 0, 0) == CUI_EDIT_OK);
    assert(cui_edit_replace_range(edit, 0, 5, cui_string("  aa\nbb")) == CUI_EDIT_OK);
    cui_edit_selection same_row[] = {{1, 1, 1}, {2, 3, 3}, {3, 6, 6}};
    assert(cui_edit_select_many(edit, same_row, 3) == CUI_EDIT_OK);
    assert(cui_edit_indent_selections(edit, true, false, 2, false) == CUI_EDIT_OK &&
           !strcmp(buffer, "aa\nbb"));
    assert(live[0].cursor == 0 && live[1].cursor == 1 && live[2].cursor == 4);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    assert(cui_edit_indent_selections(edit, true, false, 3, false) == CUI_EDIT_OK &&
           !strcmp(buffer, "  aa\nbb"));
    assert(cui_edit_indent_selections(edit, false, true, 2, false) == CUI_EDIT_OK &&
           !strcmp(buffer, "    aa\n  bb"));
    assert(live[0].cursor == 3 && live[1].cursor == 5 && live[2].cursor == 10);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "  aa\nbb"));
    assert(cui_edit_indent_selections(edit, false, false, 0, false) == CUI_EDIT_INVALID_ARGUMENT);
    cui_edit_destroy(edit);
    char tiny[8] = "abcd";
    edit = cui_edit_create(
        (cui_edit_config){.buffer = tiny, .capacity = sizeof tiny, .multiline = true});
    assert(edit && cui_edit_attach_selections(edit, live, 8, NULL, 0) == CUI_EDIT_OK);
    cui_edit_selection two[] = {{1, 0, 0}, {2, 3, 3}};
    assert(cui_edit_select_many(edit, two, 2) == CUI_EDIT_OK);
    assert(cui_edit_indent_selections(edit, false, false, 2, false) == CUI_EDIT_CAPACITY &&
           !strcmp(tiny, "abcd"));
    assert(live[0].cursor == 0 && live[1].cursor == 3);
    assert(cui_edit_paste_selections(edit, cui_string("long\nlong")) == CUI_EDIT_CAPACITY &&
           !strcmp(tiny, "abcd"));
    cui_edit_destroy(edit);
}
static void test_change_span_metadata(void) {
    char buffer[64] = "aa\nbb\ncc";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 256,
                                                       .max_history_steps = 4});
    assert(edit && !cui_edit_get_change(edit).valid);
    uint64_t revision = cui_edit_get_state(edit).revision;
    assert(cui_edit_select(edit, 7, 7) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).revision == revision && !cui_edit_get_change(edit).valid);
    assert(cui_edit_replace_range(edit, 7, 7, cui_string("X")) == CUI_EDIT_OK);
    cui_edit_change change = cui_edit_get_change(edit);
    assert(change.valid && change.revision == cui_edit_get_state(edit).revision &&
           change.first_byte == 7);
    revision = change.revision;
    assert(cui_edit_replace_range(edit, 7, 8, cui_string("X")) == CUI_EDIT_OK);
    assert(cui_edit_get_state(edit).revision == revision);
    assert(
        cui_edit_replace_range(
            edit, 7, 8,
            cui_string("too much text for the buffer capacity and then much more text indeed")) ==
        CUI_EDIT_CAPACITY);
    assert(cui_edit_get_state(edit).revision == revision &&
           cui_edit_get_change(edit).first_byte == 7);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && cui_edit_get_change(edit).first_byte == 0);
    assert(cui_edit_redo(edit) == CUI_EDIT_OK && cui_edit_get_change(edit).first_byte == 0);
    cui_edit_selection selections[4];
    cui_edit_selection_snapshot history[5];
    assert(cui_edit_attach_selections(edit, selections, 4, history, 5) == CUI_EDIT_OK);
    cui_edit_selection cursors[] = {{7, 7, 7}, {3, 3, 3}};
    assert(cui_edit_select_many(edit, cursors, 2) == CUI_EDIT_OK);
    assert(cui_edit_replace_selections(edit, cui_string("!")) == CUI_EDIT_OK);
    assert(cui_edit_get_change(edit).first_byte == 3);
    cui_edit_destroy(edit);
}
static void test_typing_history_coalescing(void) {
    char buffer[128] = "";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 8});
    assert(edit);
    assert(cui_edit_type(edit, cui_string("a")) == CUI_EDIT_OK);
    assert(cui_edit_type(edit, cui_string("b")) == CUI_EDIT_OK);
    assert(cui_edit_type(edit, cui_string("c")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "abc"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, ""));
    assert(cui_edit_redo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abc"));
    assert(cui_edit_move(edit, CUI_EDIT_GRAPHEME_PREVIOUS, false) == CUI_EDIT_OK);
    assert(cui_edit_type(edit, cui_string("X")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "abXc"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abc"));
    assert(cui_edit_type(edit, cui_string("!")) == CUI_EDIT_OK);
    assert(cui_edit_insert(edit, cui_string("?")) == CUI_EDIT_OK);
    assert(cui_edit_type(edit, cui_string("z")) == CUI_EDIT_OK);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "ab!?c"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "ab!c"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abc"));
    cui_edit_destroy(edit);
}
static void test_multicursor_typing_history(void) {
    char buffer[128] = "ab\ncd";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 8});
    assert(edit);
    cui_edit_selection selections[4];
    cui_edit_selection_snapshot history[9];
    assert(cui_edit_attach_selections(edit, selections, 4, history, 9) == CUI_EDIT_OK);
    cui_edit_selection cursors[] = {{11, 1, 1}, {22, 4, 4}};
    assert(cui_edit_select_many(edit, cursors, 2) == CUI_EDIT_OK);
    assert(cui_edit_type_selections(edit, cui_string("x")) == CUI_EDIT_OK);
    assert(cui_edit_type_selections(edit, cui_string("y")) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "axyb\ncxyd"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "ab\ncd"));
    size_t count = 0;
    const cui_edit_selection *restored = cui_edit_get_selections(edit, &count);
    assert(count == 2 && restored[0].id == 11 && restored[0].cursor == 1 && restored[1].id == 22 &&
           restored[1].cursor == 4);
    assert(cui_edit_redo(edit) == CUI_EDIT_OK && !strcmp(buffer, "axyb\ncxyd"));
    cui_edit_destroy(edit);
}
static void test_typing_history_batch_limit(void) {
    char buffer[1200] = "";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = sizeof buffer * 4,
                                                       .max_history_steps = 3});
    assert(edit);
    for (size_t i = 0; i < 1001; ++i)
        assert(cui_edit_type(edit, cui_string("x")) == CUI_EDIT_OK);
    assert(strlen(buffer) == 1001);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && strlen(buffer) == 1000);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !buffer[0]);
    cui_edit_destroy(edit);
}
static void test_column_anchor_history(void) {
    char buffer[64] = "wide\nx\nlonger";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 3});
    assert(edit);
    cui_edit_selection selections[3];
    cui_edit_selection_snapshot history[4];
    cui_edit_column_anchor columns[3];
    cui_edit_column_snapshot column_history[4];
    assert(cui_edit_attach_selections(edit, selections, 3, history, 4) == CUI_EDIT_OK);
    assert(cui_edit_attach_columns(edit, columns, 3, column_history, 4) == CUI_EDIT_OK);
    cui_edit_selection cursors[] = {{7, 3, 3}, {8, 6, 6}};
    assert(cui_edit_select_many(edit, cursors, 2) == CUI_EDIT_OK);
    cui_edit_column_anchor preferred[] = {
        {.id = 7, .cursor = 3, .column = 3, .x = 30, .valid = true},
        {.id = 8, .cursor = 6, .column = 3, .x = 30, .valid = true}};
    assert(cui_edit_set_columns(edit, preferred, 2) == CUI_EDIT_OK);
    preferred[1].cursor = 5;
    assert(cui_edit_set_columns(edit, preferred, 2) == CUI_EDIT_INVALID_ARGUMENT);
    preferred[1].cursor = 6;
    assert(cui_edit_replace_selections(edit, cui_string("!")) == CUI_EDIT_OK);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    size_t count = 0;
    const cui_edit_column_anchor *restored = cui_edit_get_columns(edit, &count);
    assert(count == 2 && restored[0].id == 7 && restored[0].cursor == 3 && restored[0].valid &&
           restored[0].column == 3 && restored[0].x == 30 && restored[1].id == 8 &&
           restored[1].valid && restored[1].column == 3);
    uint64_t revision = cui_edit_get_state(edit).revision;
    assert(cui_edit_insert(edit, cui_string("this exceeds the available buffer by a wide margin; "
                                            "this exceeds the available buffer again")) ==
           CUI_EDIT_CAPACITY);
    restored = cui_edit_get_columns(edit, &count);
    assert(cui_edit_get_state(edit).revision == revision && count == 2 && restored[0].valid &&
           restored[0].column == 3);
    assert(cui_edit_redo(edit) == CUI_EDIT_OK);
    assert(cui_edit_get_columns(edit, &count) && count == 2);
    for (size_t step = 0; step < 4; ++step) {
        const cui_edit_selection *current = cui_edit_get_selections(edit, &count);
        cui_edit_column_anchor next[2] = {{.id = current[0].id,
                                           .cursor = current[0].cursor,
                                           .column = step,
                                           .x = 20.0f + (float)step,
                                           .valid = true},
                                          {.id = current[1].id,
                                           .cursor = current[1].cursor,
                                           .column = step,
                                           .x = 20.0f + (float)step,
                                           .valid = true}};
        assert(cui_edit_set_columns(edit, next, 2) == CUI_EDIT_OK);
        assert(cui_edit_replace_selections(edit, cui_string("q")) == CUI_EDIT_OK);
    }
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK);
    restored = cui_edit_get_columns(edit, &count);
    assert(count == 2 && restored[0].valid && restored[0].x == 21 && restored[1].valid &&
           restored[1].x == 21);
    cui_edit_detach_selections(edit);
    assert(!cui_edit_get_columns(edit, &count) && count == 0);
    cui_edit_destroy(edit);
}
static void test_directional_delete_history(void) {
    char buffer[128] = "abcdef";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 7});
    assert(edit);
    assert(cui_edit_backspace_grapheme(edit) == CUI_EDIT_OK);
    assert(cui_edit_backspace_grapheme(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcd"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcdef"));
    assert(cui_edit_redo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcd"));
    assert(cui_edit_select(edit, 1, 1) == CUI_EDIT_OK);
    assert(cui_edit_delete_grapheme(edit) == CUI_EDIT_OK);
    assert(cui_edit_delete_grapheme(edit) == CUI_EDIT_OK && !strcmp(buffer, "ad"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcd"));
    assert(cui_edit_backspace_grapheme(edit) == CUI_EDIT_OK && !strcmp(buffer, "bcd"));
    assert(cui_edit_delete_grapheme(edit) == CUI_EDIT_OK && !strcmp(buffer, "cd"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "bcd"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcd"));
    assert(cui_edit_select(edit, 4, 4) == CUI_EDIT_OK);
    cui_event key = {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_BACKSPACE};
    assert(cui_edit_apply_event(edit, key) == CUI_EDIT_OK);
    assert(cui_edit_apply_event(edit, key) == CUI_EDIT_OK && !strcmp(buffer, "ab"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcd"));
    cui_edit_destroy(edit);
}
static void test_multi_delete_history(void) {
    char buffer[128] = "abcd\nefgh";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 7});
    assert(edit);
    cui_edit_selection selections[3];
    cui_edit_selection_snapshot history[8];
    assert(cui_edit_attach_selections(edit, selections, 3, history, 8) == CUI_EDIT_OK);
    cui_edit_selection cursors[] = {{4, 4, 4}, {5, 9, 9}};
    assert(cui_edit_select_many(edit, cursors, 2) == CUI_EDIT_OK);
    assert(cui_edit_erase_selections_interactive(edit, true, false) == CUI_EDIT_OK);
    assert(cui_edit_erase_selections_interactive(edit, true, false) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "ab\nef"));
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "abcd\nefgh"));
    size_t count = 0;
    const cui_edit_selection *restored = cui_edit_get_selections(edit, &count);
    assert(count == 2 && restored[0].id == 4 && restored[0].cursor == 4 && restored[1].id == 5 &&
           restored[1].cursor == 9);
    cui_edit_destroy(edit);
}
static void test_multi_typing_change_limit(void) {
    char buffer[1300] = "a\nb";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .multiline = true,
                                                       .history_bytes = sizeof buffer * 4,
                                                       .max_history_steps = 3});
    assert(edit);
    cui_edit_selection selections[2];
    cui_edit_selection_snapshot history[4];
    assert(cui_edit_attach_selections(edit, selections, 2, history, 4) == CUI_EDIT_OK);
    cui_edit_selection cursors[] = {{1, 1, 1}, {2, 3, 3}};
    assert(cui_edit_select_many(edit, cursors, 2) == CUI_EDIT_OK);
    for (size_t i = 0; i < 501; ++i)
        assert(cui_edit_type_selections(edit, cui_string("x")) == CUI_EDIT_OK);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && strlen(buffer) == 1003);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(buffer, "a\nb"));
    cui_edit_destroy(edit);
}
static void test_ime_session(void) {
    char document[1024] = "before AFTER", preedit[512], view[1536];
    cui_edit *e = make(document, sizeof(document), false, 10);
    assert(cui_edit_select(e, 7, 12) == CUI_EDIT_OK);
    cui_edit_composition c = {.buffer = preedit,
                              .capacity = sizeof(preedit),
                              .view_buffer = view,
                              .view_capacity = sizeof(view)};
    const char *emoji = "\xf0\x9f\x98\x80\xc3\xa9";
    cui_event update = {.type = CUI_EVENT_COMPOSITION_UPDATE,
                        .text_data = emoji,
                        .text_length = 6,
                        .composition_anchor = 4,
                        .composition_cursor = 6};
    assert(cui_edit_composition_event(e, &c, update, true) == CUI_EDIT_OK);
    assert(c.active && c.start == 7 && c.end == 12);
    assert(!strcmp(document, "before AFTER") && !cui_edit_get_state(e).can_undo);
    cui_str rendered = cui_edit_composition_view(e, &c);
    assert(rendered.length == 13 && !memcmp(rendered.data + 7, emoji, 6));
    assert(cui_edit_composition_selection(e, &c).cursor == 13);
    update.composition_cursor = 1;
    assert(cui_edit_composition_event(e, &c, update, true) == CUI_EDIT_BOUNDARY);
    assert(c.cursor == 6);
    update.composition_cursor = 6;
    update.text_length = sizeof(preedit);
    assert(cui_edit_composition_event(e, &c,
                                      (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                                  .text_data = "\xff",
                                                  .text_length = 1},
                                      true) == CUI_EDIT_INVALID_UTF8);
    char large[600];
    memset(large, 'x', sizeof(large));
    assert(cui_edit_composition_event(e, &c,
                                      (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                                  .text_data = large,
                                                  .text_length = sizeof(large)},
                                      true) == CUI_EDIT_CAPACITY);
    assert(c.length == 6);
    assert(
        cui_edit_composition_event(
            e, &c,
            (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT, .text_data = emoji, .text_length = 6},
            true) == CUI_EDIT_OK);
    assert(!c.active && !strcmp(document, "before \xf0\x9f\x98\x80\xc3\xa9"));
    assert(cui_edit_undo(e) == CUI_EDIT_OK && !strcmp(document, "before AFTER"));
    assert(cui_edit_get_state(e).anchor == 7 && cui_edit_get_state(e).cursor == 12);
    assert(cui_edit_composition_event(e, &c,
                                      (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                                  .text_data = large,
                                                  .text_length = 300},
                                      true) == CUI_EDIT_OK);
    assert(c.length == 300 && !strcmp(document, "before AFTER"));
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_CANCEL}, true) ==
           CUI_EDIT_OK);
    assert(!c.active && !strcmp(document, "before AFTER"));
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_START},
                                      true) == CUI_EDIT_OK);
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE},
                                      false) == CUI_EDIT_OK);
    assert(!c.active);
    c.view_capacity = 8;
    assert(
        cui_edit_composition_event(
            e, &c,
            (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE, .text_data = "new", .text_length = 3},
            true) == CUI_EDIT_CAPACITY);
    assert(!c.active && !strcmp(document, "before AFTER"));
    c.view_capacity = sizeof(view);
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_START},
                                      true) == CUI_EDIT_OK);
    assert(cui_edit_select(e, 0, 0) == CUI_EDIT_OK);
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE},
                                      true) == CUI_EDIT_OK);
    assert(!c.active); /* External selection drift invalidates old replacement range. */
    cui_edit_destroy(e);
    char large_document[2048] = "", large_preedit[2048], large_view[6144], payload[1024];
    memset(payload, 'x', sizeof(payload));
    e = make(large_document, sizeof(large_document), false, 10);
    cui_edit_composition large_c = {.buffer = large_preedit,
                                    .capacity = sizeof(large_preedit),
                                    .view_buffer = large_view,
                                    .view_capacity = sizeof(large_view)};
    cui_event big = {.type = CUI_EVENT_COMPOSITION_UPDATE,
                     .text_data = payload,
                     .text_length = sizeof(payload),
                     .composition_anchor = sizeof(payload),
                     .composition_cursor = sizeof(payload)};
    assert(cui_edit_composition_event(e, &large_c, big, true) == CUI_EDIT_OK);
    payload[0] = 'y';
    assert(cui_edit_composition_view(e, &large_c).length == 1024 && large_preedit[0] == 'x');
    assert(large_document[0] == 0);
    big.type = CUI_EVENT_COMPOSITION_COMMIT;
    assert(cui_edit_composition_event(e, &large_c, big, true) == CUI_EDIT_OK);
    assert(strlen(large_document) == 1024 && large_document[0] == 'y');
    assert(cui_edit_undo(e) == CUI_EDIT_OK && large_document[0] == 0);
    cui_edit_destroy(e);
    char multiline[64] = "abcdef";
    e = make(multiline, sizeof(multiline), true, 10);
    cui_edit_selection selections[4];
    cui_edit_selection_snapshot history[32];
    assert(cui_edit_attach_selections(e, selections, 4, history, 32) == CUI_EDIT_OK);
    cui_edit_selection two[] = {{1, 0, 0}, {2, 4, 4}};
    assert(cui_edit_select_many(e, two, 2) == CUI_EDIT_OK);
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_START},
                                      true) == CUI_EDIT_INVALID_ARGUMENT);
    assert(!c.active && !strcmp(multiline, "abcdef"));
    cui_edit_destroy(e);
}
static void test_ime_owned_view_growth(void) {
    size_t capacity = 400001;
    char *document = malloc(capacity), *insert = malloc(100001);
    assert(document && insert);
    memset(document, 'a', 200000);
    document[200000] = 0;
    memset(insert, 'b', 100000);
    insert[100000] = 0;
    cui_edit *e = cui_edit_create((cui_edit_config){.buffer = document,
                                                    .capacity = capacity,
                                                    .history_bytes = capacity * 5,
                                                    .max_history_steps = 4});
    assert(e);
    assert(cui_edit_select(e, 100000, 100000) == CUI_EDIT_OK);
    char preedit[4096];
    cui_edit_composition c = {.buffer = preedit, .capacity = sizeof(preedit), .grow_view = true};
    cui_event update = {.type = CUI_EVENT_COMPOSITION_UPDATE,
                        .text_data = "\xf0\x9f\x98\x80",
                        .text_length = 4,
                        .composition_anchor = 4,
                        .composition_cursor = 4};
    assert(cui_edit_composition_event(e, &c, update, true) == CUI_EDIT_OK);
    cui_str snapshot = cui_edit_composition_view(e, &c);
    assert(snapshot.length == 200004 && c.view_buffer == NULL);
    assert(!memcmp(snapshot.data + 100000, update.text_data, 4) && strlen(document) == 200000);
    char *first = (char *)snapshot.data;
    /* An idle view read must neither allocate nor rebuild/copy a document. */
    first[0] = 'z';
    assert(cui_edit_composition_view(e, &c).data == first && first[0] == 'z');
    first[0] = 'a';
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL},
                                      true) == CUI_EDIT_OK);
    assert(cui_edit_composition_view(e, &c).data == document);
    assert(cui_edit_composition_event(e, &c, update, true) == CUI_EDIT_OK);
    assert(cui_edit_composition_view(e, &c).data == first); /* Reuse after cancellation. */
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL},
                                      true) == CUI_EDIT_OK);
    assert(cui_edit_insert(e, (cui_str){insert, 100000}) == CUI_EDIT_OK);
    assert(cui_edit_composition_event(e, &c, update, true) == CUI_EDIT_OK);
    assert(cui_edit_composition_view(e, &c).data != first);
    snapshot = cui_edit_composition_view(e, &c);
    assert(snapshot.length == 300004 && strlen(document) == 300000);
    update.type = CUI_EVENT_COMPOSITION_COMMIT;
    assert(cui_edit_composition_event(e, &c, update, true) == CUI_EDIT_OK);
    assert(strlen(document) == 300004 && !c.active);
    assert(cui_edit_undo(e) == CUI_EDIT_OK && strlen(document) == 300000);
    assert(cui_edit_undo(e) == CUI_EDIT_OK && strlen(document) == 200000);
    update.type = CUI_EVENT_COMPOSITION_UPDATE;
    assert(cui_edit_composition_event(e, &c, update, true) == CUI_EDIT_OK);
    cui_str previous = cui_edit_composition_view(e, &c);
    char oversized[4096];
    memset(oversized, 'q', sizeof(oversized));
    assert(cui_edit_composition_event(e, &c,
                                      (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                                  .text_data = oversized,
                                                  .text_length = sizeof(oversized)},
                                      true) == CUI_EDIT_CAPACITY);
    assert(c.active && c.length == 4 && cui_edit_composition_view(e, &c).data == previous.data &&
           cui_edit_composition_view(e, &c).length == previous.length);
    cui_edit_destroy(e);
    assert(c.view_buffer == NULL); /* Owned rendering never installs a dangling session pointer. */
    char rebound[64] = "replacement";
    e = make(rebound, sizeof(rebound), false, 4);
    assert(cui_edit_composition_view(e, &c).data == rebound);
    assert(cui_edit_composition_selection(e, &c).cursor == cui_edit_get_state(e).cursor);
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_GESTURE}, true) ==
           CUI_EDIT_OK);
    assert(!c.active && !strcmp(rebound, "replacement"));
    assert(cui_edit_composition_event(e, &c, update, true) == CUI_EDIT_OK);
    assert(cui_edit_composition_view(e, &c).length == 15);
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL},
                                      true) == CUI_EDIT_OK);
    assert(!c.active && !strcmp(rebound, "replacement"));
    cui_edit_destroy(e);
    free(insert);
    free(document);
}
int main(void) {
    test_multi_typing_change_limit();
    test_directional_delete_history();
    test_multi_delete_history();
    test_column_anchor_history();
    test_typing_history_batch_limit();
    test_ime_owned_view_growth();
    test_ime_session();
    test_multicursor_typing_history();
    test_typing_history_coalescing();
    test_change_span_metadata();
    test_multi_indent_and_clipboard();
    test_multi_motion_and_erasure();
    test_multiple_selections();
    test_token_replacement();
    test_inline_content();
    test_unicode_word_events();
    test_word_conformance();
    test_grapheme_conformance();
    test_grapheme_editing();
    test_composition_keys();
    test_word_and_vertical_navigation();
    test_atomic_range();
    test_atomic_indent();
    test_unicode();
    test_rejection();
    test_history_lines();
    test_random_operations();
    test_events();
    test_revision_and_max_scalar();
    puts("edit tests passed");
    return 0;
}
