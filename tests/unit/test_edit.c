#include "cui/cui_edit.h"
#include <assert.h>
#include <stdio.h>
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
int main(void) {
    test_atomic_range();
    test_unicode();
    test_rejection();
    test_history_lines();
    test_random_operations();
    test_events();
    test_revision_and_max_scalar();
    puts("edit tests passed");
    return 0;
}
