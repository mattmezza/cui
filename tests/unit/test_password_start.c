/* SPDX-License-Identifier: MIT */
#include "cui/cui_edit.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_edit *edit;
    unsigned calls, predicates;
    bool guard;
    cui_edit_result error;
    char candidate[64];
} spy;
static cui_edit_result prepare(void *p, cui_str text, bool provisional) {
    spy *s = p;
    s->calls++;
    assert(provisional && text.length < sizeof s->candidate);
    memcpy(s->candidate, text.data, text.length);
    s->candidate[text.length] = 0;
    if (s->guard)
        cui_edit_break_coalescing(s->edit);
    return s->error;
}
static bool predicate(void *p, cui_str text) {
    (void)text;
    ((spy *)p)->predicates++;
    return false;
}
static void unchanged(cui_edit *e, cui_edit_state before, cui_edit_composition *c,
                      const char *preedit, const char *view) {
    cui_edit_state now = cui_edit_get_state(e);
    assert(now.length == before.length && now.revision == before.revision &&
           now.anchor == before.anchor && now.cursor == before.cursor);
    assert(!strcmp(cui_edit_get_text(e).data, "aé😀z") && !c->active && !c->length &&
           !c->view_valid);
    assert(!strcmp(c->buffer, preedit));
    if (c->view_buffer)
        assert(!strcmp(c->view_buffer, view));
}
static void run(bool grow) {
    char value[64] = "aé😀z", preedit[32] = "caller preedit", view[96] = "caller view";
    cui_edit *e = cui_edit_create((cui_edit_config){
        .buffer = value, .capacity = sizeof value, .history_bytes = 2048, .max_history_steps = 16});
    assert(e);
    assert(cui_edit_select(e, 1, 7) == CUI_EDIT_OK);
    cui_edit_state before = cui_edit_get_state(e);
    cui_edit_composition c = {.buffer = preedit,
                              .capacity = sizeof preedit,
                              .view_buffer = grow ? NULL : view,
                              .view_capacity = grow ? 0 : sizeof view,
                              .grow_view = grow};
    spy s = {.edit = e, .error = CUI_EDIT_CAPACITY};
    cui_edit_request r = {.kind = CUI_EDIT_REQUEST_COMPOSITION,
                          .composition = &c,
                          .editable = true,
                          .event = {.type = CUI_EVENT_COMPOSITION_START},
                          .presentation = {.prepare = prepare, .user = &s, .user_bytes = sizeof s},
                          .normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF};
    cui_edit_checked_result result =
        cui_edit_apply_checked(e, r, (cui_edit_acceptance){predicate, &s});
    assert(result.error == CUI_EDIT_CAPACITY && !result.presentation_applied && s.calls == 1 &&
           !s.predicates);
    unchanged(e, before, &c, "caller preedit", "caller view");
    s.error = CUI_EDIT_PRESENTATION;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){predicate, &s}).error ==
           CUI_EDIT_PRESENTATION);
    unchanged(e, before, &c, "caller preedit", "caller view");
    s.error = CUI_EDIT_OK;
    s.guard = true;
    result = cui_edit_apply_checked(e, r, (cui_edit_acceptance){predicate, &s});
    assert(result.error == CUI_EDIT_INVALID_ARGUMENT && !result.presentation_applied);
    unchanged(e, before, &c, "caller preedit", "caller view");
    s.guard = false;
    result = cui_edit_apply_checked(e, r, (cui_edit_acceptance){predicate, &s});
    assert(result.error == CUI_EDIT_OK && result.presentation_prepared &&
           result.presentation_applied && s.calls == 4 && !s.predicates &&
           !strcmp(s.candidate, "az"));
    assert(c.active && c.length == 0 && c.start == 1 && c.end == 7 && c.view_valid &&
           !strcmp(cui_edit_composition_view(e, &c).data, "az") && !strcmp(value, "aé😀z"));
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL},
                                      true) == CUI_EDIT_OK);
    assert(cui_edit_select(e, 0, 0) == CUI_EDIT_OK);
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){predicate, &s}).error ==
               CUI_EDIT_OK &&
           !strcmp(s.candidate, value));
    assert(cui_edit_composition_event(e, &c, (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL},
                                      true) == CUI_EDIT_OK);
    assert(cui_edit_select(e, 8, 8) == CUI_EDIT_OK);
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){predicate, &s}).error ==
               CUI_EDIT_OK &&
           !strcmp(s.candidate, value));
    assert(!s.predicates);
    cui_edit_destroy(e);
}
int main(void) {
    run(false);
    run(true);
    return 0;
}
