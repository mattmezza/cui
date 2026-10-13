/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <string.h>
typedef struct {
    unsigned calls;
    bool reject, reenter;
    cui_edit *edit;
    char last[64];
} check;
static bool validate(void *user, cui_str value) {
    check *c = user;
    c->calls++;
    assert(value.length < sizeof c->last);
    memcpy(c->last, value.data, value.length);
    c->last[value.length] = 0;
    assert(!memchr(value.data, '\r', value.length) && !memchr(value.data, '\n', value.length));
    if (c->reenter)
        cui_edit_destroy(c->edit);
    return !c->reject || !memchr(value.data, 'X', value.length);
}
static cui_input_result frame(cui_context *ui, cui_edit *edit, cui_input_state *state,
                              cui_input_options options) {
    assert(cui_begin_frame(ui, 480, 120, .016f));
    cui_theme theme = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 12});
    cui_input_result result =
        cui_input_ex(ui, 10, &theme, cui_string("Value"), edit, state, options);
    cui_end_box(ui);
    assert(cui_end_frame(ui).count && cui_last_error(ui) == CUI_OK);
    return result;
}
static void send(cui_context *ui, cui_event_type type, const char *s, size_t anchor,
                 size_t cursor) {
    assert(cui_queue_event(ui, (cui_event){.type = type,
                                           .text_data = s,
                                           .text_length = strlen(s),
                                           .composition_anchor = anchor,
                                           .composition_cursor = cursor}));
}
static void key(cui_context *ui, cui_key k) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k, .modifiers = CUI_MOD_CTRL}));
}
static cui_str clipboard(void *user) {
    return cui_string(user);
}
static void helper(void) {
    char out[24] = "unchanged";
    cui_str result = {"sentinel", 8};
    assert(cui_edit_normalize_text(cui_string("a\r\n😀\r\né"), CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
                                   out, sizeof out, &result) == CUI_EDIT_OK);
    assert(result.data == out && !strcmp(out, "a😀é") && result.length == 7);
    assert(cui_edit_normalize_text(cui_string("\r\n"), CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF, out, 1,
                                   &result) == CUI_EDIT_OK &&
           result.length == 0 && !out[0]);
    strcpy(out, "unchanged");
    result = (cui_str){"sentinel", 8};
    assert(cui_edit_normalize_text(cui_string("abcd"), CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF, out, 4,
                                   &result) == CUI_EDIT_CAPACITY);
    assert(!strcmp(out, "unchanged") && result.length == 8 && !strcmp(result.data, "sentinel"));
    assert(cui_edit_normalize_text((cui_str){"\xf0\x80", 2}, CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
                                   out, sizeof out, &result) == CUI_EDIT_INVALID_UTF8 &&
           !strcmp(out, "unchanged"));
    assert(cui_edit_normalize_text(cui_string(out), CUI_EDIT_NORMALIZE_NONE, out, sizeof out,
                                   &result) == CUI_EDIT_INVALID_ARGUMENT &&
           !strcmp(out, "unchanged"));
    union {
        cui_str value;
        char bytes[48];
    } alias;
    memset(&alias, 0x33, sizeof alias);
    unsigned char saved[sizeof alias];
    memcpy(saved, &alias, sizeof alias);
    assert(cui_edit_normalize_text(cui_string("a"), CUI_EDIT_NORMALIZE_NONE, alias.bytes,
                                   sizeof alias, &alias.value) == CUI_EDIT_INVALID_ARGUMENT &&
           !memcmp(&alias, saved, sizeof alias));
    assert(cui_edit_normalize_text((cui_str){alias.bytes, 1}, CUI_EDIT_NORMALIZE_NONE, out,
                                   sizeof out, &alias.value) == CUI_EDIT_INVALID_ARGUMENT &&
           !strcmp(out, "unchanged"));
    assert(cui_edit_normalize_text(cui_string("a\r\nb"), CUI_EDIT_NORMALIZE_NONE, out, sizeof out,
                                   &result) == CUI_EDIT_OK &&
           !strcmp(out, "a\r\nb"));
    assert(cui_edit_normalize_text(cui_string("a"), (cui_edit_normalization)99, out, sizeof out,
                                   &result) == CUI_EDIT_INVALID_ARGUMENT &&
           !strcmp(out, "a\r\nb"));
}
static void composed(void) {
    char buffer[24] = "";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .history_bytes = 2048,
                                                       .max_history_steps = 16});
    assert(edit);
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_input_state state = {0};
    check c = {.edit = edit};
    cui_input_options options = {.normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
                                 .validate = validate,
                                 .validate_user = &c};
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    frame(ui, edit, &state, options);
    send(ui, CUI_EVENT_TEXT, "a\r\nb", 0, 0);
    assert(frame(ui, edit, &state, options).changed);
    assert(!strcmp(buffer, "ab") && !strcmp(c.last, "ab"));
    send(ui, CUI_EVENT_TEXT, "\r\nc", 0, 0);
    assert(frame(ui, edit, &state, options).changed);
    key(ui, CUI_KEY_Z);
    frame(ui, edit, &state, options);
    assert(!buffer[0]);
    unsigned calls = c.calls;
    key(ui, CUI_KEY_Y);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "abc") && c.calls == calls);
    char paste[] = "é\r\n😀";
    cui_platform platform = {
        .capabilities = CUI_PLATFORM_CLIPBOARD, .user = paste, .clipboard_get = clipboard};
    options.clipboard = &platform;
    assert(cui_edit_select(edit, 1, 2) == CUI_EDIT_OK);
    key(ui, CUI_KEY_V);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "aé😀c"));
    key(ui, CUI_KEY_Z);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "abc"));
    c.reject = true;
    assert(cui_edit_select(edit, 1, 2) == CUI_EDIT_OK);
    cui_edit_state before = cui_edit_get_state(edit);
    send(ui, CUI_EVENT_TEXT, "X\r\n", 0, 0);
    cui_input_result r = frame(ui, edit, &state, options);
    cui_edit_state after = cui_edit_get_state(edit);
    assert(r.rejected && r.error == CUI_EDIT_OK && !strcmp(buffer, "abc") &&
           before.revision == after.revision && before.anchor == after.anchor &&
           before.cursor == after.cursor);
    options.readonly = true;
    calls = c.calls;
    send(ui, CUI_EVENT_TEXT, "q\r\n", 0, 0);
    frame(ui, edit, &state, options);
    assert(c.calls == calls && !strcmp(buffer, "abc"));
    options.readonly = false;
    options.disabled = true;
    send(ui, CUI_EVENT_TEXT, "q\r\n", 0, 0);
    frame(ui, edit, &state, options);
    assert(c.calls == calls && !strcmp(buffer, "abc"));
    options.disabled = false;
    cui_focus(ui, 10);
    frame(ui, edit, &state, options);
    options.normalization = CUI_EDIT_NORMALIZE_NONE;
    send(ui, CUI_EVENT_TEXT, "q\r\n", 0, 0);
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_LINE_BREAK);
    assert(!strcmp(buffer, "abc"));
    options.normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF;
    assert(cui_edit_select(edit, 0, 3) == CUI_EDIT_OK);
    send(ui, CUI_EVENT_TEXT, "\r\n", 0, 0);
    frame(ui, edit, &state, options);
    assert(!buffer[0]);
    send(ui, CUI_EVENT_TEXT, "\xe2\x80\xa8", 0, 0);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "\xe2\x80\xa8"));
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void ime(bool grow) {
    char buffer[16] = "ab", preedit[32] = {0}, view[32] = {0};
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                       .capacity = sizeof buffer,
                                                       .history_bytes = 1024,
                                                       .max_history_steps = 8});
    assert(edit);
    cui_edit_composition composition = {.buffer = preedit,
                                        .capacity = sizeof preedit,
                                        .view_buffer = view,
                                        .view_capacity = sizeof view,
                                        .grow_view = grow};
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_input_state state = {0};
    state.behavior.composition = &composition;
    check c = {.edit = edit, .reject = true};
    cui_input_options options = {.normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
                                 .validate = validate,
                                 .validate_user = &c};
    frame(ui, edit, &state, options);
    cui_focus(ui, 10);
    frame(ui, edit, &state, options);
    assert(cui_edit_select(edit, 0, 2) == CUI_EDIT_OK);
    send(ui, CUI_EVENT_COMPOSITION_START, "", 0, 0);
    frame(ui, edit, &state, options);
    const char *s = "é\r\n😀\nq";
    send(ui, CUI_EVENT_COMPOSITION_UPDATE, s, 3, 9);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "ab") && !strcmp(preedit, "é😀q") && composition.anchor == 2 &&
           composition.cursor == 6);
    cui_str visible = cui_edit_composition_view(edit, &composition);
    assert(visible.length == 7 && !memcmp(visible.data, "é😀q", 7));
    send(ui, CUI_EVENT_COMPOSITION_UPDATE, "X\r\n", 1, 3);
    assert(frame(ui, edit, &state, options).rejected && !strcmp(preedit, "é😀q"));
    visible = cui_edit_composition_view(edit, &composition);
    assert(visible.length == 7 && !memcmp(visible.data, "é😀q", 7));
    send(ui, CUI_EVENT_COMPOSITION_UPDATE, s, 1, 9);
    assert(frame(ui, edit, &state, options).error == CUI_EDIT_BOUNDARY);
    assert(!strcmp(preedit, "é😀q"));
    send(ui, CUI_EVENT_COMPOSITION_COMMIT, s, 3, 9);
    frame(ui, edit, &state, options);
    assert(!composition.active && !strcmp(buffer, "é😀q"));
    key(ui, CUI_KEY_Z);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "ab"));
    send(ui, CUI_EVENT_COMPOSITION_START, "", 0, 0);
    frame(ui, edit, &state, options);
    send(ui, CUI_EVENT_COMPOSITION_UPDATE, "\r\n", 1, 2);
    frame(ui, edit, &state, options);
    assert(composition.active && !preedit[0] && composition.anchor == 0 && composition.cursor == 0);
    send(ui, CUI_EVENT_COMPOSITION_CANCEL, "", 0, 0);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "ab"));
    send(ui, CUI_EVENT_COMPOSITION_START, "", 0, 0);
    frame(ui, edit, &state, options);
    send(ui, CUI_EVENT_COMPOSITION_UPDATE, "q\r\n", 1, 3);
    frame(ui, edit, &state, options);
    cui_edit_state before = cui_edit_get_state(edit);
    send(ui, CUI_EVENT_COMPOSITION_COMMIT, "X\r\n", 1, 3);
    assert(frame(ui, edit, &state, options).rejected && !composition.active);
    cui_edit_state after = cui_edit_get_state(edit);
    assert(!strcmp(buffer, "ab") && before.revision == after.revision &&
           before.anchor == after.anchor && before.cursor == after.cursor);
    send(ui, CUI_EVENT_TEXT, "c\r\n", 0, 0);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "c"));
    key(ui, CUI_KEY_Z);
    frame(ui, edit, &state, options);
    assert(!strcmp(buffer, "ab"));
    cui_destroy(ui);
    cui_edit_destroy(edit);
}
static void bounds(void) {
    char buffer[4] = "a";
    cui_edit *edit = cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = 4});
    assert(edit);
    cui_edit_request request = {.kind = CUI_EDIT_REQUEST_INSERT,
                                .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
                                .text = {"\r\n\r\n\r\nb", 7}};
    assert(cui_edit_apply_checked(edit, request, (cui_edit_acceptance){0}).error == CUI_EDIT_OK &&
           !strcmp(buffer, "ab"));
    request.text = cui_string("cd\r\n");
    assert(cui_edit_apply_checked(edit, request, (cui_edit_acceptance){0}).error ==
               CUI_EDIT_CAPACITY &&
           !strcmp(buffer, "ab"));
    check c = {.edit = edit, .reenter = true};
    request.text = cui_string("\r\nc");
    assert(cui_edit_apply_checked(edit, request, (cui_edit_acceptance){validate, &c}).error ==
               CUI_EDIT_INVALID_ARGUMENT &&
           !strcmp(buffer, "ab"));
    cui_edit_destroy(edit);
    char multiline[16] = "";
    edit =
        cui_edit_create((cui_edit_config){.buffer = multiline, .capacity = 16, .multiline = true});
    request.text = cui_string("a\r\nb");
    assert(cui_edit_apply_checked(edit, request, (cui_edit_acceptance){0}).error == CUI_EDIT_OK &&
           !strcmp(multiline, "a\r\nb"));
    cui_edit_destroy(edit);
}
static void composition_bounds(void) {
    char buffer[8] = "ab", preedit[8] = {0}, view[3] = {0};
    cui_edit *edit =
        cui_edit_create((cui_edit_config){.buffer = buffer, .capacity = sizeof buffer});
    assert(edit && cui_edit_select(edit, 0, 2) == CUI_EDIT_OK);
    cui_edit_composition c = {.buffer = preedit,
                              .capacity = sizeof preedit,
                              .view_buffer = view,
                              .view_capacity = sizeof view};
    cui_edit_request r = {.kind = CUI_EDIT_REQUEST_COMPOSITION,
                          .composition = &c,
                          .editable = true,
                          .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF,
                          .event = {.type = CUI_EVENT_COMPOSITION_START}};
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK &&
           c.active);
    r.event =
        (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE, .text_data = "abc\r\n", .text_length = 5};
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){0}).error == CUI_EDIT_CAPACITY);
    assert(c.active && !c.length && !preedit[0] && !view[0] && !strcmp(buffer, "ab"));
    r.event.text_data = "\r\n\r\n\r\n\r\n";
    r.event.text_length = 8;
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){0}).error == CUI_EDIT_CAPACITY);
    assert(!c.length && !preedit[0] && !view[0]);
    r.event.text_data = "\xff\r";
    r.event.text_length = 2;
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_UTF8);
    assert(!c.length && !preedit[0] && !view[0]);
    r.event.text_data = "a\r\nb";
    r.event.text_length = 4;
    r.event.composition_anchor = 1;
    r.event.composition_cursor = 3;
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK);
    assert(!strcmp(preedit, "ab") && !strcmp(view, "ab") && c.anchor == 1 && c.cursor == 1);
    r.event.type = CUI_EVENT_COMPOSITION_CANCEL;
    assert(cui_edit_apply_checked(edit, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK &&
           !c.active);
    cui_edit_destroy(edit);
}
int main(void) {
    helper();
    composition_bounds();
    composed();
    ime(false);
    ime(true);
    bounds();
    return 0;
}
