/* SPDX-License-Identifier: MIT */
#include "cui/cui_edit.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_edit *e;
    unsigned calls;
    bool provisional, guard;
    char value[64];
    cui_edit_result error;
} spy;
static cui_edit_result prepare(void *p, cui_str s, bool provisional) {
    spy *v = p;
    v->calls++;
    v->provisional = provisional;
    assert(s.length < sizeof v->value);
    memcpy(v->value, s.data, s.length);
    v->value[s.length] = 0;
    if (v->guard)
        assert(cui_edit_insert(v->e, cui_string("bad")) == CUI_EDIT_INVALID_ARGUMENT);
    return v->error;
}
int main(void) {
    char buffer[64] = "one two", preedit[32] = {0}, view[96] = {0};
    cui_edit *e = cui_edit_create((cui_edit_config){.buffer = buffer,
                                                    .capacity = sizeof buffer,
                                                    .history_bytes = 2048,
                                                    .max_history_steps = 16});
    assert(e);
    spy s = {.e = e};
    cui_edit_request r = {.kind = CUI_EDIT_REQUEST_PRESENT,
                          .presentation = {.prepare = prepare, .user = &s, .user_bytes = sizeof s}};
    cui_edit_checked_result result = cui_edit_apply_checked(e, r, (cui_edit_acceptance){0});
    assert(result.error == CUI_EDIT_OK && result.presentation_prepared &&
           result.presentation_applied && s.calls == 1 && !strcmp(s.value, "one two"));
    s.guard = true;
    result = cui_edit_apply_checked(e, r, (cui_edit_acceptance){0});
    assert(result.error == CUI_EDIT_INVALID_ARGUMENT && !strcmp(buffer, "one two"));
    s.guard = false;
    cui_edit_composition c = {.buffer = preedit,
                              .capacity = sizeof preedit,
                              .view_buffer = view,
                              .view_capacity = sizeof view};
    r.composition = &c;
    unsigned alias_calls = s.calls;
    c.buffer = buffer;
    c.capacity = sizeof buffer;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
               CUI_EDIT_INVALID_ARGUMENT &&
           s.calls == alias_calls);
    c.buffer = preedit;
    c.capacity = sizeof preedit;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK &&
           !s.provisional);
    assert(cui_edit_select(e, 7, 7) == CUI_EDIT_OK);
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_START},
                                      true) == CUI_EDIT_OK);
    assert(cui_edit_composition_event(e, &c,
                                      (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                                                  .text_data = "é",
                                                  .text_length = 2,
                                                  .composition_cursor = 2,
                                                  .composition_anchor = 0},
                                      true) == CUI_EDIT_OK);
    (void)cui_edit_composition_view(e, &c);
    result = cui_edit_apply_checked(e, r, (cui_edit_acceptance){0});
    assert(result.error == CUI_EDIT_OK && s.provisional && !strcmp(s.value, "one twoé") &&
           c.active);
    unsigned calls = s.calls;
    c.model_identity++;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
               CUI_EDIT_INVALID_ARGUMENT &&
           s.calls == calls);
    c.model_identity--;
    s.error = CUI_EDIT_CAPACITY;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error == CUI_EDIT_CAPACITY &&
           c.active && !strcmp(buffer, "one two"));
    s.error = CUI_EDIT_INVALID_UTF8;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
               CUI_EDIT_INVALID_ARGUMENT &&
           c.active && !strcmp(buffer, "one two"));
    s.error = CUI_EDIT_OK;
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL},
                                      true) == CUI_EDIT_OK);
    r = (cui_edit_request){
        .kind = CUI_EDIT_REQUEST_EVENT,
        .event = {.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT, .modifiers = CUI_MOD_CTRL},
        .masked_words = true};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK &&
           cui_edit_get_state(e).cursor == 0);
    r.event.key = CUI_KEY_RIGHT;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK &&
           cui_edit_get_state(e).cursor == 7);
    r.event.key = CUI_KEY_BACKSPACE;
    r.presentation =
        (cui_edit_presentation){.prepare = prepare, .user = &s, .user_bytes = sizeof s};
    s.error = CUI_EDIT_CAPACITY;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error == CUI_EDIT_CAPACITY &&
           cui_edit_get_state(e).cursor == 7 && !strcmp(buffer, "one two"));
    s.error = CUI_EDIT_OK;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK &&
           !buffer[0]);
    cui_edit_destroy(e);
    return 0;
}
