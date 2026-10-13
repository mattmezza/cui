/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    cui_edit *edit;
    unsigned calls, minimum, reenter;
    size_t lengths[32];
    char values[32][64];
    const char *borrowed;
} predicate;
static bool digits(void *user, cui_str value) {
    predicate *p = user;
    assert(p->calls < 32 && cui_edit_valid_utf8(value));
    unsigned at = p->calls++;
    p->lengths[at] = value.length;
    size_t n = value.length < 63 ? value.length : 63;
    memcpy(p->values[at], value.data, n);
    p->values[at][n] = 0;
    p->borrowed = value.data; /* Only used to test forbidden workspace aliases afterwards. */
    assert(cui_edit_get_text(p->edit).data && cui_edit_get_state(p->edit).length < 128);
    switch (p->reenter) {
    case 1:
        assert(cui_edit_select(p->edit, 0, 0) == CUI_EDIT_INVALID_ARGUMENT);
        break;
    case 2:
        cui_edit_destroy(p->edit);
        break;
    case 3:
        cui_edit_break_coalescing(p->edit);
        break;
    case 4:
        cui_edit_detach_selections(p->edit);
        break;
    case 5:
        cui_edit_detach_columns(p->edit);
        break;
    case 6:
        assert(cui_edit_insert(p->edit, cui_string("9")) == CUI_EDIT_INVALID_ARGUMENT);
        break;
    case 7:
        assert(cui_edit_undo(p->edit) == CUI_EDIT_INVALID_ARGUMENT);
        break;
    case 8:
        assert(cui_edit_apply_checked(
                   p->edit, (cui_edit_request){.kind = CUI_EDIT_REQUEST_INSERT, .text = {"9", 1}},
                   (cui_edit_acceptance){0})
                   .error == CUI_EDIT_INVALID_ARGUMENT);
        break;
    case 9:
        assert(cui_edit_composition_view(p->edit, NULL).data == NULL);
        break;
    }
    if (value.length < p->minimum)
        return false;
    for (size_t i = 0; i < value.length; i++)
        if (value.data[i] < '0' || value.data[i] > '9')
            return false;
    return true;
}
static cui_draw_list painted;
static cui_input_result frame(cui_context *ui, cui_edit *e, cui_input_state *state,
                              cui_input_options options) {
    assert(cui_begin_frame(ui, 600, 180, .016f));
    cui_theme t = cui_theme_light();
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .padding = 16});
    cui_input_result result =
        cui_input_ex(ui, 10, &t, cui_string("Accepted Input"), e, state, options);
    cui_button(ui, 20, &t, cui_string("Other focus"), false);
    cui_end_box(ui);
    painted = cui_end_frame(ui);
    assert(painted.count && cui_last_error(ui) == CUI_OK);
    return result;
}
static void text(cui_context *ui, const char *s) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_TEXT, .text_data = s, .text_length = strlen(s)}));
}
static void key(cui_context *ui, cui_key k, unsigned mods) {
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k, .modifiers = mods}));
}
static char clipboard[64];
static unsigned copies;
static bool fail_copy;
static bool copy(void *user, cui_str s) {
    (void)user;
    copies++;
    if (fail_copy)
        return false;
    assert(s.length < sizeof clipboard);
    memcpy(clipboard, s.data, s.length);
    clipboard[s.length] = 0;
    return true;
}
static cui_str paste(void *user) {
    (void)user;
    return cui_string(clipboard);
}
static void widget_dispatch(void) {
    char buffer[64] = "";
    cui_edit *e = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                    .capacity = sizeof buffer,
                                                    .history_bytes = 2048,
                                                    .max_history_steps = 16});
    cui_context *ui = cui_create((cui_config){0});
    cui_input_state state = {0};
    predicate p = {.edit = e};
    cui_platform platform = {
        .capabilities = CUI_PLATFORM_CLIPBOARD, .clipboard_get = paste, .clipboard_set = copy};
    cui_input_options options = {.validate = digits, .validate_user = &p, .clipboard = &platform};
    assert(e && ui);
    frame(ui, e, &state, options);
    cui_focus(ui, 10);
    text(ui, "1");
    assert(frame(ui, e, &state, options).changed);
    text(ui, "2");
    assert(frame(ui, e, &state, options).changed && !strcmp(buffer, "12"));
    unsigned calls = p.calls;
    cui_edit_state before = cui_edit_get_state(e);
    text(ui, "X");
    cui_input_result r = frame(ui, e, &state, options);
    assert(r.rejected && !r.changed && r.error == CUI_EDIT_OK && !strcmp(buffer, "12"));
    assert(p.calls == calls + 2 && !strcmp(p.values[calls], "12X") &&
           !strcmp(p.values[calls + 1], "12"));
    assert(cui_edit_get_state(e).revision == before.revision &&
           cui_edit_get_state(e).cursor == before.cursor);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (sem[i].id == 10)
            assert(!sem[i].details.invalid);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, e, &state, options);
    assert(!buffer[0]);
    text(ui, "X");
    assert(frame(ui, e, &state, options).rejected && cui_edit_get_state(e).can_redo);
    calls = p.calls;
    key(ui, CUI_KEY_Y, CUI_MOD_CTRL);
    frame(ui, e, &state, options);
    assert(!strcmp(buffer, "12") && p.calls == calls); /* Replay bypasses acceptance. */
    strcpy(clipboard, "3");
    key(ui, CUI_KEY_V, CUI_MOD_CTRL);
    frame(ui, e, &state, options);
    assert(!strcmp(buffer, "123"));
    strcpy(clipboard, "X");
    key(ui, CUI_KEY_V, CUI_MOD_CTRL);
    assert(frame(ui, e, &state, options).rejected && !strcmp(buffer, "123"));
    assert(cui_edit_select(e, 0, 3) == CUI_EDIT_OK);
    p.minimum = 3;
    assert(cui_edit_select(e, 0, 1) == CUI_EDIT_OK);
    copies = 0;
    key(ui, CUI_KEY_X, CUI_MOD_CTRL);
    r = frame(ui, e, &state, options);
    assert(r.rejected && !strcmp(buffer, "123") && copies == 1 && !strcmp(clipboard, "1"));
    fail_copy = true;
    calls = p.calls;
    key(ui, CUI_KEY_X, CUI_MOD_CTRL);
    r = frame(ui, e, &state, options);
    assert(r.error == CUI_EDIT_CLIPBOARD_UNAVAILABLE && p.calls == calls && !strcmp(buffer, "123"));
    fail_copy = false;
    p.minimum = 0;
    key(ui, CUI_KEY_BACKSPACE, 0);
    assert(frame(ui, e, &state, options).changed && !strcmp(buffer, "23"));
    assert(cui_edit_select(e, 0, 0) == CUI_EDIT_OK);
    key(ui, CUI_KEY_DELETE, 0);
    assert(frame(ui, e, &state, options).changed && !strcmp(buffer, "3"));
    key(ui, CUI_KEY_END, 0);
    key(ui, CUI_KEY_BACKSPACE, CUI_MOD_CTRL);
    frame(ui, e, &state, options);
    assert(!buffer[0]);
    assert(cui_edit_insert(e, cui_string("bad")) ==
           CUI_EDIT_OK); /* Programmatic invalid defaults are allowed. */
    text(ui, "X");
    r = frame(ui, e, &state, options);
    assert(r.changed && !r.rejected && !strcmp(buffer, "badX"));
    key(ui, CUI_KEY_A, CUI_MOD_CTRL);
    key(ui, CUI_KEY_BACKSPACE, 0);
    calls = p.calls;
    frame(ui, e, &state, options);
    assert(!buffer[0] && p.calls == calls); /* Empty candidate bypasses predicate. */
    options.readonly = true;
    text(ui, "1");
    calls = p.calls;
    frame(ui, e, &state, options);
    assert(!buffer[0] && p.calls == calls);
    options.readonly = false;
    options.disabled = true;
    text(ui, "1");
    frame(ui, e, &state, options);
    assert(!buffer[0] && p.calls == calls);
    options.disabled = false;
    options.invalid = true;
    cui_focus(ui, 10);
    text(ui, "1");
    frame(ui, e, &state, options);
    assert(!strcmp(buffer, "1"));
    cui_edit_destroy(e);
    cui_destroy(ui);
}
static cui_edit_checked_result checked(cui_edit *e, predicate *p, cui_edit_request request) {
    return cui_edit_apply_checked(e, request, (cui_edit_acceptance){digits, p});
}
static void guards_and_bounds(void) {
    char buffer[8] = "1";
    cui_edit *e = cui_edit_create((cui_edit_config){
        .buffer = buffer, .capacity = sizeof buffer, .history_bytes = 512, .max_history_steps = 8});
    predicate p = {.edit = e};
    cui_edit_request insert = {.kind = CUI_EDIT_REQUEST_INSERT, .text = {"2", 1}};
    for (unsigned mode = 1; mode <= 9; mode++) {
        p.reenter = mode;
        p.calls = 0;
        cui_edit_state before = cui_edit_get_state(e);
        cui_edit_checked_result r = checked(e, &p, insert);
        assert(r.error == CUI_EDIT_INVALID_ARGUMENT && !r.rejected && !strcmp(buffer, "1") &&
               cui_edit_get_state(e).revision == before.revision &&
               cui_edit_get_state(e).cursor == before.cursor);
    }
    p.reenter = 0;
    p.calls = 0;
    assert(checked(e, &p, insert).error == CUI_EDIT_OK && !strcmp(buffer, "12"));
    const char *borrowed = p.borrowed;
    insert.text = (cui_str){borrowed, 2};
    assert(checked(e, &p, insert).error == CUI_EDIT_INVALID_ARGUMENT && !strcmp(buffer, "12"));
    p.calls = 0;
    insert.text = cui_string("1234567");
    assert(checked(e, &p, insert).error == CUI_EDIT_CAPACITY && p.calls == 0);
    insert.text = (cui_str){"\xc3", 1};
    assert(checked(e, &p, insert).error == CUI_EDIT_INVALID_UTF8 && p.calls == 0);
    insert.text = cui_string("\n");
    assert(checked(e, &p, insert).error == CUI_EDIT_LINE_BREAK && p.calls == 0);
    insert.kind = (cui_edit_request_kind)99;
    assert(checked(e, &p, insert).error == CUI_EDIT_INVALID_ARGUMENT && p.calls == 0);
    cui_edit_composition bad = {.buffer = buffer, .capacity = sizeof buffer, .grow_view = true};
    assert(checked(e, &p,
                   (cui_edit_request){.kind = CUI_EDIT_REQUEST_COMPOSITION,
                                      .composition = &bad,
                                      .editable = true,
                                      .event = {.type = CUI_EVENT_COMPOSITION_START}})
                   .error == CUI_EDIT_INVALID_ARGUMENT &&
           !strcmp(buffer, "12"));
    cui_edit_destroy(e);
    char multi[32] = "a\nb";
    e = cui_edit_create(
        (cui_edit_config){.buffer = multi, .capacity = sizeof multi, .multiline = true});
    p = (predicate){.edit = e};
    insert = (cui_edit_request){.kind = CUI_EDIT_REQUEST_INSERT, .text = {"X", 1}};
    assert(checked(e, &p, insert).error == CUI_EDIT_OK && p.calls == 0 && !strcmp(multi, "a\nbX"));
    cui_edit_destroy(e);
}
static void composition(bool grow) {
    char buffer[8] = "12", preedit[64] = {0}, view[80] = {0};
    cui_edit *e = cui_edit_create((cui_edit_config){
        .buffer = buffer, .capacity = sizeof buffer, .history_bytes = 512, .max_history_steps = 8});
    predicate p = {.edit = e};
    cui_edit_composition c = {.buffer = preedit,
                              .capacity = sizeof preedit,
                              .view_buffer = grow ? NULL : view,
                              .view_capacity = grow ? 0 : sizeof view,
                              .grow_view = grow};
    assert(cui_edit_select(e, 0, 2) == CUI_EDIT_OK);
    cui_edit_request req = {.kind = CUI_EDIT_REQUEST_COMPOSITION,
                            .composition = &c,
                            .editable = true,
                            .event = {.type = CUI_EVENT_COMPOSITION_START}};
    assert(checked(e, &p, req).error == CUI_EDIT_OK && p.calls == 0);
    req.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                            .text_data = "34567890123456789012",
                            .text_length = 20,
                            .composition_cursor = 20};
    assert(checked(e, &p, req).error == CUI_EDIT_OK && c.active && !strcmp(buffer, "12"));
    cui_str visible = cui_edit_composition_view(e, &c);
    assert(visible.length == 20);
    char previous[64];
    memcpy(previous, visible.data, visible.length + 1);
    unsigned calls = p.calls;
    req.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                            .text_data = "X",
                            .text_length = 1,
                            .composition_cursor = 1};
    cui_edit_checked_result result = checked(e, &p, req);
    visible = cui_edit_composition_view(e, &c);
    assert(result.rejected && result.error == CUI_EDIT_OK && c.active && c.length == 20 &&
           visible.length == 20 && !memcmp(previous, visible.data, 21) && !strcmp(buffer, "12") &&
           p.calls == calls + 2);
    req.event.type = CUI_EVENT_COMPOSITION_COMMIT;
    result = checked(e, &p, req);
    assert(result.rejected && c.active && !strcmp(buffer, "12"));
    req.event = (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                            .text_data = "34567890123456789012",
                            .text_length = 20,
                            .composition_cursor = 20};
    calls = p.calls;
    assert(checked(e, &p, req).error == CUI_EDIT_CAPACITY && p.calls == calls && c.active);
    req.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                            .text_data = visible.data,
                            .text_length = visible.length};
    assert(checked(e, &p, req).error == CUI_EDIT_INVALID_ARGUMENT && c.active &&
           !memcmp(previous, cui_edit_composition_view(e, &c).data, 21));
    p.reenter = 2;
    req.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                            .text_data = "56",
                            .text_length = 2,
                            .composition_cursor = 2};
    assert(checked(e, &p, req).error == CUI_EDIT_INVALID_ARGUMENT && c.active && c.length == 20 &&
           !memcmp(previous, cui_edit_composition_view(e, &c).data, 21));
    p.reenter = 0;
    req.event = (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                            .text_data = "34",
                            .text_length = 2,
                            .composition_cursor = 2};
    assert(checked(e, &p, req).error == CUI_EDIT_OK && !c.active && !strcmp(buffer, "34"));
    assert(cui_edit_undo(e) == CUI_EDIT_OK && !strcmp(buffer, "12"));
    cui_edit_destroy(e);
}
static void host_composition(void) {
    char buffer[64] = "12";
    cui_edit *e = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                    .capacity = sizeof buffer,
                                                    .history_bytes = 2048,
                                                    .max_history_steps = 16});
    cui_context *ui = cui_create((cui_config){0});
    cui_input_state state = {0};
    predicate p = {.edit = e};
    cui_input_options options = {.validate = digits, .validate_user = &p};
    frame(ui, e, &state, options);
    cui_focus(ui, 10);
    assert(cui_edit_select(e, 0, 2) == CUI_EDIT_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .text_data = "34",
                                           .text_length = 2,
                                           .composition_cursor = 2}));
    assert(!frame(ui, e, &state, options).changed && !strcmp(buffer, "12"));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .text_data = "X",
                                           .text_length = 1,
                                           .composition_cursor = 1}));
    assert(frame(ui, e, &state, options).rejected && state.behavior.composition->active &&
           !strcmp(cui_edit_composition_view(e, state.behavior.composition).data, "34"));
    cui_edit_state before = cui_edit_get_state(e);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                                           .text_data = "X",
                                           .text_length = 1,
                                           .composition_cursor = 1}));
    cui_input_result r = frame(ui, e, &state, options);
    assert(r.rejected && !r.changed && !state.behavior.composition->active &&
           !strcmp(buffer, "12") && cui_edit_get_state(e).revision == before.revision &&
           cui_edit_get_state(e).cursor == before.cursor &&
           cui_edit_get_state(e).anchor == before.anchor);
    text(ui, "56");
    assert(frame(ui, e, &state, options).changed && !strcmp(buffer, "56"));
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, e, &state, options);
    assert(!strcmp(buffer, "12"));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                           .text_data = "78",
                                           .text_length = 2,
                                           .composition_cursor = 2}));
    frame(ui, e, &state, options);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                                           .text_data = "78",
                                           .text_length = 2,
                                           .composition_cursor = 2}));
    assert(frame(ui, e, &state, options).changed && !strcmp(buffer, "78"));
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, e, &state, options);
    assert(!strcmp(buffer, "12"));
    cui_edit_destroy(e);
    cui_destroy(ui);
}
static void grapheme_and_fixed_capacity(void) {
    char buffer[32] = "1\xcc\x81"
                      "2";
    cui_edit *e = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                    .capacity = sizeof buffer,
                                                    .history_bytes = 1024,
                                                    .max_history_steps = 8});
    predicate p = {.edit = e};
    cui_context *ui = cui_create((cui_config){0});
    cui_input_state state = {0};
    cui_input_options options = {.validate = digits, .validate_user = &p};
    frame(ui, e, &state, options);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_HOME, 0);
    key(ui, CUI_KEY_RIGHT, 0);
    key(ui, CUI_KEY_BACKSPACE, 0);
    assert(frame(ui, e, &state, options).changed && !strcmp(buffer, "2") && p.calls == 1);
    key(ui, CUI_KEY_Z, CUI_MOD_CTRL);
    frame(ui, e, &state, options);
    assert(strlen(buffer) == 4 && p.calls == 1);
    cui_edit_destroy(e);
    cui_destroy(ui);
    char document[8] = "12", preedit[8] = {0}, view[2] = {0};
    e = cui_edit_create((cui_edit_config){.buffer = document, .capacity = sizeof document});
    p = (predicate){.edit = e};
    assert(cui_edit_select(e, 0, 2) == CUI_EDIT_OK);
    cui_edit_composition c = {.buffer = preedit,
                              .capacity = sizeof preedit,
                              .view_buffer = view,
                              .view_capacity = sizeof view};
    cui_edit_request req = {.kind = CUI_EDIT_REQUEST_COMPOSITION,
                            .composition = &c,
                            .editable = true,
                            .event = {.type = CUI_EVENT_COMPOSITION_START}};
    assert(checked(e, &p, req).error == CUI_EDIT_OK && c.active);
    req.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                            .text_data = "345",
                            .text_length = 3,
                            .composition_cursor = 3};
    assert(checked(e, &p, req).error == CUI_EDIT_CAPACITY && !p.calls && c.active && !c.length &&
           !preedit[0] && !view[0] && !strcmp(document, "12"));
    req.event =
        (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE, .text_data = "\xc3", .text_length = 1};
    assert(checked(e, &p, req).error == CUI_EDIT_INVALID_UTF8 && !p.calls && c.active && !c.length);
    req.event = (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL};
    assert(checked(e, &p, req).error == CUI_EDIT_OK && !c.active && !p.calls);
    cui_edit_destroy(e);
}
typedef struct {
    unsigned calls;
    size_t length;
} large_predicate;
static bool last_not_x(void *user, cui_str value) {
    large_predicate *p = user;
    p->calls++;
    p->length = value.length;
    return value.data[value.length - 1] != 'X';
}
static void large_workspace(void) {
    char *document = malloc(200010), *saved = malloc(200001);
    assert(document && saved);
    memset(document, '1', 200000);
    document[200000] = 0;
    memcpy(saved, document, 200001);
    cui_edit *e = cui_edit_create((cui_edit_config){.buffer = document, .capacity = 200010});
    large_predicate p = {0};
    cui_edit_acceptance acceptance = {last_not_x, &p};
    cui_edit_request request = {.kind = CUI_EDIT_REQUEST_INSERT, .text = {"X", 1}};
    cui_edit_checked_result r = cui_edit_apply_checked(e, request, acceptance);
    assert(r.rejected && r.error == CUI_EDIT_OK && p.calls == 2 && p.length == 200000 &&
           !memcmp(document, saved, 200001));
    cui_edit_destroy(e);
    free(document);
    free(saved);
    document = calloc(5000, 1);
    char *preedit = calloc(9001, 1), *payload = malloc(8021), *previous = malloc(8001);
    assert(document && preedit && payload && previous);
    memset(document, '1', 4000);
    e = cui_edit_create((cui_edit_config){.buffer = document, .capacity = 5000});
    cui_edit_composition c = {.buffer = preedit, .capacity = 9001, .grow_view = true};
    assert(cui_edit_select(e, 0, 4000) == CUI_EDIT_OK);
    request = (cui_edit_request){.kind = CUI_EDIT_REQUEST_COMPOSITION,
                                 .composition = &c,
                                 .editable = true,
                                 .event = {.type = CUI_EVENT_COMPOSITION_START}};
    p = (large_predicate){0};
    assert(cui_edit_apply_checked(e, request, acceptance).error == CUI_EDIT_OK);
    uintptr_t original = (uintptr_t)cui_edit_composition_view(e, &c).data;
    memset(payload, '1', 8000);
    payload[7999] = 'X';
    payload[8000] = 0;
    request.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                .text_data = payload,
                                .text_length = 8000,
                                .composition_cursor = 8000};
    r = cui_edit_apply_checked(e, request, acceptance);
    assert(r.rejected && p.calls == 1 && c.active && !c.length &&
           (uintptr_t)cui_edit_composition_view(e, &c).data == original &&
           !cui_edit_composition_view(e, &c).length);
    payload[7999] = '2';
    assert(cui_edit_apply_checked(e, request, acceptance).error == CUI_EDIT_OK && c.length == 8000);
    cui_str view = cui_edit_composition_view(e, &c);
    assert(view.length == 8000);
    memcpy(previous, view.data, 8001);
    memset(payload, '1', 8020);
    payload[8019] = 'X';
    payload[8020] = 0;
    request.event.text_length = request.event.composition_cursor = 8020;
    r = cui_edit_apply_checked(e, request, acceptance);
    view = cui_edit_composition_view(e, &c);
    assert(r.rejected && c.length == 8000 && view.length == 8000 &&
           !memcmp(previous, view.data, 8001) && strlen(document) == 4000);
    unsigned calls = p.calls;
    request =
        (cui_edit_request){.kind = CUI_EDIT_REQUEST_EVENT, .event = {.type = CUI_EVENT_GESTURE}};
    for (unsigned i = 0; i < 10; i++)
        assert(cui_edit_apply_checked(e, request, acceptance).error == CUI_EDIT_OK);
    assert(p.calls == calls);
    cui_edit_destroy(e);
    free(document);
    free(preedit);
    free(payload);
    free(previous);
}
int main(void) {
    widget_dispatch();
    large_workspace();
    grapheme_and_fixed_capacity();
    host_composition();
    guards_and_bounds();
    composition(false);
    composition(true);
    return 0;
}
