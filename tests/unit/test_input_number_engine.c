/* SPDX-License-Identifier: MIT */
#include "cui/cui_edit.h"
#include <assert.h>
#include <string.h>
static cui_edit_format number(bool grouping, uint32_t separator, bool limited, size_t digits) {
    return (cui_edit_format){.kind = CUI_EDIT_FORMAT_NUMBER,
                             .has_separator = grouping,
                             .separator = separator,
                             .fraction_limited = limited,
                             .fraction_digits = digits};
}
static void format(cui_edit_format f, const char *value, const char *expected, bool valid) {
    bool actual = !valid;
    cui_str out;
    char buffer[128];
    assert(cui_edit_format_valid(f, cui_string(value), &actual) == CUI_EDIT_OK && actual == valid);
    assert(cui_edit_format_text(f, cui_string(value), buffer, sizeof buffer, &out) == CUI_EDIT_OK);
    assert(out.length == strlen(expected) && !memcmp(buffer, expected, out.length + 1));
}
typedef struct {
    cui_edit *edit;
    unsigned calls, predicates;
    bool fail, mutate;
    char prepared[128], seen[128];
} spy;
static cui_edit_result prepare(void *user, cui_str value, bool provisional) {
    spy *s = user;
    (void)provisional;
    s->calls++;
    memcpy(s->prepared, value.data, value.length);
    s->prepared[value.length] = 0;
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
static cui_edit_checked_result insert(cui_edit *edit, cui_edit_request request, const char *value,
                                      spy *s) {
    request.kind = CUI_EDIT_REQUEST_INSERT;
    request.text = cui_string(value);
    return cui_edit_apply_checked(edit, request, (cui_edit_acceptance){predicate, s});
}
int main(void) {
    cui_edit_format none = number(false, 0, false, 0), grouped = number(true, ',', false, 0),
                    limited = number(true, ',', true, 2);
    format(grouped, "1234567.89", "1,234,567.89", true);
    format(grouped, "-123", "-123", true);
    format(grouped, "+1234567", "+1,234,567", true);
    format(grouped, "-1234567.", "-1,234,567.", true);
    format(none, ".", ".", true);
    format(none, "-.5", "-.5", true);
    format(grouped, "+.5", "+.5", true);
    format(grouped, "-", "-", true);
    format(grouped, "+", "+", true);
    format(grouped, "", "", true);
    format(limited, "1234.5678", "1,234.56", true);
    format(limited, "1234.5", "1,234.5", true);
    format(number(true, ',', true, 0), "1234.5678", "1,234", true);
    format(number(false, 0, true, 0), "1234.5678", "1234.5678", true);
    format(number(false, 0, true, 2), "1234.5678", "1234.5678", true);
    format(limited, "1,23,4.5,6", "1,234.56", true);
    format(grouped, "1.2.3", "1.2", false);
    format(grouped, "1..", "1.", false);
    format(grouped, "1,-234", "-1,234", false);
    format(grouped, "+-12", "-+12", false);
    format(grouped, "+123.-", "+123.-", false);
    format(number(true, 0x2009, false, 0), "1234567.5", "1 234 567.5", true);
    format(number(true, '.', false, 0), "1.23", "123", true);
    format(number(true, '+', false, 0), "1.+", "1.", true);
    char buffer[128] = "untouched";
    cui_str out = {buffer, 9};
    assert(cui_edit_unformat_text(grouped, cui_string("1,234.00"), buffer, sizeof buffer, &out) ==
               CUI_EDIT_OK &&
           !strcmp(buffer, "1234."));
    assert(cui_edit_unformat_text(none, cui_string("1.00"), buffer, sizeof buffer, &out) ==
               CUI_EDIT_OK &&
           !strcmp(buffer, "1.00"));
    assert(cui_edit_format_placeholder(grouped, buffer, sizeof buffer, &out) == CUI_EDIT_OK &&
           !out.length);
    strcpy(buffer, "untouched");
    out = (cui_str){buffer, 9};
    assert(cui_edit_format_text(number(true, 0, false, 0), cui_string("1234"), buffer,
                                sizeof buffer, &out) == CUI_EDIT_INVALID_ARGUMENT);
    assert(!strcmp(buffer, "untouched") && out.data == buffer && out.length == 9);

    bool nul_valid = true;
    assert(cui_edit_format_valid(number(true, 0, false, 0), cui_string("1234"), &nul_valid) ==
               CUI_EDIT_INVALID_ARGUMENT &&
           nul_valid);
    assert(cui_edit_unformat_text(number(true, 0, false, 0), cui_string("1234"), buffer,
                                  sizeof buffer, &out) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_format_placeholder(number(true, 0, false, 0), buffer, sizeof buffer, &out) ==
           CUI_EDIT_INVALID_ARGUMENT);
    assert(!strcmp(buffer, "untouched") && out.data == buffer && out.length == 9);
    assert(cui_edit_format_text(grouped, cui_string("1234"), buffer, 5, &out) == CUI_EDIT_CAPACITY);
    assert(!strcmp(buffer, "untouched") && out.data == buffer && out.length == 9);
    assert(cui_edit_unformat_text(grouped, cui_string("1.00000"), buffer, 3, &out) == CUI_EDIT_OK &&
           !strcmp(buffer, "1."));

    const char *fullwidth = "１２。５\r\n−＋－．，";
    assert(cui_edit_normalize_number_text(cui_string(fullwidth),
                                          CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF, buffer,
                                          sizeof buffer, &out) == CUI_EDIT_OK);
    assert(!strcmp(buffer, "12.5-+-.,"));
    size_t offset = 99;
    assert(cui_edit_number_offset(cui_string("１😀２"), CUI_EDIT_NORMALIZE_NONE, 7, &offset) ==
               CUI_EDIT_OK &&
           offset == 5);
    assert(cui_edit_number_offset(cui_string("１😀２"), CUI_EDIT_NORMALIZE_NONE, 4, &offset) ==
               CUI_EDIT_BOUNDARY &&
           offset == 5);
    assert(cui_edit_number_offset(cui_string("１\r\n２"), CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF, 5,
                                  &offset) == CUI_EDIT_OK &&
           offset == 1);
    char saved[128];
    memcpy(saved, buffer, sizeof saved);
    cui_str previous = out;
    assert(cui_edit_normalize_number_text(cui_string("１２"), CUI_EDIT_NORMALIZE_NONE, buffer, 2,
                                          &out) == CUI_EDIT_CAPACITY);
    assert(!memcmp(saved, buffer, sizeof saved) && out.data == previous.data &&
           out.length == previous.length);
    assert(cui_edit_normalize_number_text((cui_str){buffer, 3}, CUI_EDIT_NORMALIZE_NONE, buffer,
                                          sizeof buffer, &out) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_normalize_number_text(cui_string("１"), CUI_EDIT_NORMALIZE_NONE, buffer,
                                          sizeof buffer,
                                          (cui_str *)buffer) == CUI_EDIT_INVALID_ARGUMENT);
    assert(cui_edit_normalize_number_text((cui_str){"\xff", 1}, CUI_EDIT_NORMALIZE_NONE, buffer,
                                          sizeof buffer, &out) == CUI_EDIT_INVALID_UTF8);
    bool validity = true;
    cui_edit_format invalid = grouped;
    invalid.separator = 0xd800;
    assert(cui_edit_format_valid(invalid, cui_string("12"), &validity) ==
               CUI_EDIT_INVALID_ARGUMENT &&
           validity);
    invalid = none;
    invalid.separator = ',';
    assert(cui_edit_format_valid(invalid, cui_string("12"), &validity) ==
           CUI_EDIT_INVALID_ARGUMENT);

    char document[128] = "", work[128];
    cui_edit *edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 12});
    assert(edit);
    spy s = {.edit = edit};
    cui_edit_request r = {.format = limited,
                          .format_workspace = {work, sizeof work},
                          .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
                          .presentation = {.prepare = prepare, .user = &s, .user_bytes = sizeof s}};
    memset(work, 0x5a, sizeof work);
    cui_edit_request bad = r;
    bad.format.separator = 0;
    assert(insert(edit, bad, "1234", &s).error == CUI_EDIT_INVALID_ARGUMENT && !document[0] &&
           !s.calls && !s.predicates);
    for (size_t i = 0; i < sizeof work; i++)
        assert(work[i] == 0x5a);
    cui_edit_checked_result checked = insert(edit, r, "１２３４。５６７", &s);
    assert(checked.error == CUI_EDIT_OK && !checked.rejected && !strcmp(document, "1,234.56"));
    assert(!strcmp(s.seen, "1234.567") && !strcmp(s.prepared, "1,234.56") && s.calls == 1 &&
           s.predicates == 1);
    assert(cui_edit_select(edit, 0, 8) == CUI_EDIT_OK);
    checked = insert(edit, r, "１２x", &s);
    assert(checked.rejected && !strcmp(document, "1,234.56"));
    unsigned calls = s.calls, predicates = s.predicates;
    r.format_workspace.bytes = 4;
    checked = insert(edit, r, "5678", &s);
    assert(checked.error == CUI_EDIT_CAPACITY && s.calls == calls && s.predicates == predicates);
    assert(cui_edit_get_state(edit).anchor == 0 && cui_edit_get_state(edit).cursor == 8);
    r.format_workspace.bytes = sizeof work;
    s.fail = true;
    assert(insert(edit, r, "5678", &s).error == CUI_EDIT_PRESENTATION &&
           !strcmp(document, "1,234.56"));
    s.fail = false;
    s.mutate = true;
    assert(insert(edit, r, "5678", &s).error == CUI_EDIT_INVALID_ARGUMENT &&
           !strcmp(document, "1,234.56"));
    s.mutate = false;
    r.format_workspace.data = document;
    calls = s.calls;
    assert(insert(edit, r, "5678", &s).error == CUI_EDIT_INVALID_ARGUMENT && s.calls == calls);
    r.format_workspace.data = work;

    char preedit[64] = "", view[128] = "";
    cui_edit_composition c = {.buffer = preedit,
                              .capacity = sizeof preedit,
                              .view_buffer = view,
                              .view_capacity = sizeof view};
    r.kind = CUI_EDIT_REQUEST_COMPOSITION;
    r.composition = &c;
    r.editable = true;
    r.event = (cui_event){.type = CUI_EVENT_COMPOSITION_START};
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_OK);
    r.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                          .text_data = "５\r\n６７８",
                          .text_length = 14,
                          .composition_anchor = 3,
                          .composition_cursor = 14};
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_OK);
    assert(!strcmp(document, "1,234.56") && !strcmp(preedit, "5678") && !strcmp(view, "5678") &&
           c.anchor == 1 && c.cursor == 4);
    s.fail = true;
    r.event.text_data = "９９９９";
    r.event.text_length = 12;
    r.event.composition_anchor = 0;
    r.event.composition_cursor = 12;
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_PRESENTATION);
    assert(!strcmp(document, "1,234.56") && !strcmp(preedit, "5678") && !strcmp(view, "5678") &&
           c.anchor == 1 && c.cursor == 4 && c.active);
    s.fail = false;
    r.event.text_data = "５\r\n６７８";
    r.event.text_length = 14;
    r.event.composition_anchor = 3;
    r.event.composition_cursor = 14;
    r.event.type = CUI_EVENT_COMPOSITION_COMMIT;
    calls = s.calls;
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_OK);
    assert(!strcmp(document, "5,678") && !strcmp(s.prepared, "5,678") && s.calls == calls + 1 &&
           !c.active);
    r = (cui_edit_request){
        .kind = CUI_EDIT_REQUEST_EVENT,
        .format = none,
        .format_workspace = {work, sizeof work},
        .event = {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL}};
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
               CUI_EDIT_OK &&
           !strcmp(document, "1,234.56"));
    r.event.key = CUI_KEY_Y;
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){predicate, &s}).error ==
               CUI_EDIT_OK &&
           !strcmp(document, "5,678"));
    cui_edit_destroy(edit);
    strcpy(document, "bad");
    edit = cui_edit_create((cui_edit_config){document, sizeof document, false, 2048, 12});
    s.edit = edit;
    r = (cui_edit_request){.format = grouped, .format_workspace = {work, sizeof work}};
    checked = insert(edit, r, "x", &s);
    assert(checked.error == CUI_EDIT_OK && !checked.rejected && !strcmp(document, "b,adx"));
    cui_edit_destroy(edit);
    return 0;
}
