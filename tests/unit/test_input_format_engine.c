/* SPDX-License-Identifier: MIT */
#include "cui/cui_edit.h"
#include <assert.h>
#include <string.h>

static cui_edit_format pattern(const char *p) {
    return (cui_edit_format){.kind = CUI_EDIT_FORMAT_PATTERN, .pattern = cui_string(p)};
}
static void formatted(const char *p, const char *value, const char *expected, bool valid) {
    char output[128], original[128];
    cui_str result;
    bool actual = !valid;
    assert(cui_edit_format_valid(pattern(p), cui_string(value), &actual) == CUI_EDIT_OK);
    assert(actual == valid);
    assert(cui_edit_format_text(pattern(p), cui_string(value), output, sizeof output, &result) ==
           CUI_EDIT_OK);
    assert(result.length == strlen(expected) && !strcmp(output, expected));
    assert(cui_edit_unformat_text(pattern(p), result, original, sizeof original, &result) ==
           CUI_EDIT_OK);
}
typedef struct {
    cui_edit *edit;
    unsigned prepared, predicates;
    bool fail, mutate;
    char presented[128], seen[128];
} spy;
static cui_edit_result prepare(void *user, cui_str value, bool provisional) {
    spy *s = user;
    (void)provisional;
    s->prepared++;
    memcpy(s->presented, value.data, value.length);
    s->presented[value.length] = 0;
    if (s->mutate)
        cui_edit_destroy(s->edit);
    return s->fail ? CUI_EDIT_PRESENTATION : CUI_EDIT_OK;
}
static bool predicate(void *user, cui_str value) {
    spy *s = user;
    s->predicates++;
    memcpy(s->seen, value.data, value.length);
    s->seen[value.length] = 0;
    return !memchr(value.data, 'x', value.length);
}
static cui_edit_checked_result insert(cui_edit *edit, cui_edit_request request, const char *text,
                                      spy *s) {
    request.kind = CUI_EDIT_REQUEST_INSERT;
    request.text = cui_string(text);
    return cui_edit_apply_checked(edit, request, (cui_edit_acceptance){predicate, s});
}
int main(void) {
    formatted("(AA)999-999", "AB123456", "(AB)123-456", true);
    formatted("(AA)999-999", "(AB123456", "(AB)123-456", true);
    formatted("(AA)999-999", "AB123--", "(AB)123-", false);
    formatted("999-999-******", "123456A(111)", "123-456-A(111)", true);
    formatted("9999年99月", "2024年12月", "2024年12月", true);
    formatted("*999", "😀123", "😀123", true);
    formatted("**", "é", "é", true);
    formatted("A9", "1", "", true); /* Source skips mismatched tokens in validity. */
    formatted("99-99", "", "", true);
    formatted("", "😀", "😀", true);
    char output[128] = "unchanged";
    cui_str result = {output, 9};
    assert(cui_edit_format_placeholder(pattern("9999年99月"), output, sizeof output, &result) ==
           CUI_EDIT_OK);
    assert(!strcmp(output, "____年__月"));
    char before[128];
    memcpy(before, output, sizeof before);
    cui_str saved = result;
    assert(cui_edit_format_text(pattern("99-99"), cui_string("1234"), output, 5, &result) ==
           CUI_EDIT_CAPACITY);
    assert(!memcmp(before, output, sizeof before) && result.data == saved.data &&
           result.length == saved.length);
    assert(cui_edit_format_text(pattern("99"), (cui_str){"\xff", 1}, output, sizeof output,
                                &result) == CUI_EDIT_INVALID_UTF8);
    assert(cui_edit_format_text(pattern("99"), (cui_str){output, 2}, output, sizeof output,
                                &result) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_format_text(pattern("99"), cui_string("12"), output, sizeof output,
                                (cui_str *)output) == CUI_EDIT_INVALID_ARGUMENT);

    char document[128] = "", work[128];
    cui_edit *edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 12});
    assert(edit);
    spy s = {.edit = edit};
    cui_edit_request r = {.format = pattern("99-99"),
                          .format_workspace = {work, sizeof work},
                          .presentation = {.prepare = prepare, .user = &s, .user_bytes = sizeof s}};
    cui_edit_checked_result checked = insert(edit, r, "1234", &s);
    assert(checked.error == CUI_EDIT_OK && !checked.rejected && checked.presentation_applied);
    assert(!strcmp(document, "12-34") && !strcmp(s.presented, "12-34") && s.prepared == 1 &&
           s.predicates == 1 && !strcmp(s.seen, "1234"));
    assert(cui_edit_get_state(edit).cursor == 5);
    assert(cui_edit_select(edit, 3, 3) == CUI_EDIT_OK);
    uint64_t revision = cui_edit_get_state(edit).revision;
    cui_edit_request erase = r;
    erase.kind = CUI_EDIT_REQUEST_EVENT;
    erase.event = (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_BACKSPACE};
    assert(cui_edit_apply_checked(edit, erase, (cui_edit_acceptance){0}).error == CUI_EDIT_OK);
    assert(!strcmp(document, "12-34") && cui_edit_get_state(edit).revision == revision &&
           cui_edit_get_state(edit).cursor == 3);
    assert(cui_edit_select(edit, 5, 5) == CUI_EDIT_OK);
    checked = insert(edit, r, "5x", &s);
    assert(checked.rejected && !strcmp(document, "12-34") && s.predicates == 3);
    r.kind = CUI_EDIT_REQUEST_EVENT;
    r.event = (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL};
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_OK);
    assert(!document[0] && s.predicates == 3);
    r.event.key = CUI_KEY_Y;
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_OK);
    assert(!strcmp(document, "12-34") && cui_edit_get_state(edit).cursor == 5 && s.predicates == 3);
    assert(cui_edit_select(edit, 0, 5) == CUI_EDIT_OK);
    cui_edit_state state = cui_edit_get_state(edit);
    r.format_workspace.bytes = 5;
    checked = insert(edit, r, "5678", &s);
    assert(checked.error == CUI_EDIT_CAPACITY && !strcmp(document, "12-34"));
    assert(cui_edit_get_state(edit).anchor == state.anchor &&
           cui_edit_get_state(edit).cursor == state.cursor);
    r.format_workspace.bytes = sizeof work;
    s.fail = true;
    checked = insert(edit, r, "5678", &s);
    assert(checked.error == CUI_EDIT_PRESENTATION && !strcmp(document, "12-34"));
    s.fail = false;
    s.mutate = true;
    checked = insert(edit, r, "5678", &s);
    assert(checked.error == CUI_EDIT_INVALID_ARGUMENT && !strcmp(document, "12-34"));
    s.mutate = false;
    r.format_workspace.data = document;
    unsigned calls = s.prepared;
    assert(insert(edit, r, "5678", &s).error == CUI_EDIT_INVALID_ARGUMENT && s.prepared == calls);
    r.format_workspace.data = work;

    char preedit[32] = "", view[128] = "";
    cui_edit_composition c = {.buffer = preedit,
                              .capacity = sizeof preedit,
                              .view_buffer = view,
                              .view_capacity = sizeof view};
    r.kind = CUI_EDIT_REQUEST_COMPOSITION;
    r.composition = &c;
    r.editable = true;
    r.text = (cui_str){0};
    r.event = (cui_event){.type = CUI_EVENT_COMPOSITION_START};
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_OK);
    r.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                          .text_data = "5678",
                          .text_length = 4,
                          .composition_anchor = 4,
                          .composition_cursor = 4};
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_OK);
    assert(!strcmp(document, "12-34") && !strcmp(view, "5678") && !strcmp(s.presented, "5678"));
    cui_edit_composition prior = c;
    char previous_view[128];
    memcpy(previous_view, view, sizeof view);
    r.event.text_data = "56x8";
    checked = cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s});
    assert(checked.rejected && !strcmp(document, "12-34") &&
           !memcmp(previous_view, view, prior.view_length + 1));
    assert(c.active == prior.active && c.length == prior.length && c.cursor == prior.cursor);
    r.event.text_data = "5678";
    c.revision++;
    unsigned untouched = s.prepared;
    checked = cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s});
    assert(checked.error == CUI_EDIT_INVALID_ARGUMENT && s.prepared == untouched && c.active &&
           !strcmp(view, "5678") && !strcmp(preedit, "5678"));
    c.revision--;
    calls = s.prepared;
    r.event.type = CUI_EVENT_COMPOSITION_COMMIT;
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_OK);
    assert(!strcmp(document, "56-78") && !strcmp(s.presented, "56-78") && s.prepared == calls + 1 &&
           !c.active);
    assert(cui_edit_undo(edit) == CUI_EDIT_OK && !strcmp(document, "12-34"));
    cui_edit_destroy(edit);

    strcpy(document, "bad");
    edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 12});
    s.edit = edit;
    r = (cui_edit_request){.format = pattern("99-99"), .format_workspace = {work, sizeof work}};
    checked = insert(edit, r, "", &s);
    assert(checked.error == CUI_EDIT_OK && !checked.rejected && !document[0]);
    cui_edit_destroy(edit);

    strcpy(document, "bad");
    edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 12});
    s.edit = edit;
    r.composition = NULL;
    r.presentation = (cui_edit_presentation){0};
    r.kind = CUI_EDIT_REQUEST_INSERT;
    checked = insert(edit, r, "x", &s);
    assert(!checked.rejected && checked.error == CUI_EDIT_OK &&
           !document[0]); /* invalid old healing formats accepted invalid candidate */
    cui_edit_destroy(edit);
    strcpy(document, "");
    edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 12});
    s.edit = edit;
    r = (cui_edit_request){.format = pattern("9999年99月"),
                           .format_workspace = {work, sizeof work}};
    checked = insert(edit, r, "202412", &s);
    assert(checked.error == CUI_EDIT_OK && !strcmp(document, "2024年12") &&
           cui_edit_get_state(edit).cursor == 9);
    r.format = pattern("AA");
    assert(cui_edit_select(edit, 0, cui_edit_get_state(edit).length) == CUI_EDIT_OK);
    checked = insert(edit, r, "AB", &s); /* runtime policy uses invalid-old healing */
    assert(checked.error == CUI_EDIT_OK && !strcmp(document, "AB"));
    cui_edit_destroy(edit);
    edit = cui_edit_create((cui_edit_config){document, sizeof document, true, 2048, 12});
    s.edit = edit;
    cui_edit_selection selections[2];
    cui_edit_selection_snapshot snapshots[13];
    assert(cui_edit_attach_selections(edit, selections, 2, snapshots, 13) == CUI_EDIT_OK);
    assert(cui_edit_select_many(edit, (cui_edit_selection[]){{1, 0, 0}, {2, 2, 2}}, 2) ==
           CUI_EDIT_OK);
    calls = s.predicates;
    assert(insert(edit, r, "C", &s).error == CUI_EDIT_INVALID_ARGUMENT && !strcmp(document, "AB") &&
           calls == s.predicates);
    cui_edit_destroy(edit);
    strcpy(document, "abc");
    edit = cui_edit_create((cui_edit_config){document, sizeof document, true, 2048, 12});
    s.edit = edit;
    assert(insert(edit, r, "C", &s).error == CUI_EDIT_INVALID_ARGUMENT && !strcmp(document, "abc"));
    cui_edit_destroy(edit);
    return 0;
}
