/* SPDX-License-Identifier: MIT */
#include "cui/cui_edit.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_edit *edit;
    unsigned starts, matches, ends, predicates, prepares;
    cui_text_match_result begin_error, match_error, end_error;
    unsigned mutate;
    char order[128];
    size_t order_length;
} spy;
static void mark(spy *s, char c) {
    s->order[s->order_length++] = c;
}
static void mutate(spy *s, unsigned phase) {
    if (s->mutate == phase) {
        cui_edit_destroy(s->edit);
        assert(cui_edit_insert(s->edit, cui_string("!")) == CUI_EDIT_INVALID_ARGUMENT);
    }
}
static cui_text_match_result begin(void *p, uint64_t generation, void **lease) {
    spy *s = p;
    s->starts++;
    mark(s, 'B');
    mutate(s, 1);
    if (generation != 7)
        return CUI_TEXT_MATCH_STALE;
    if (s->begin_error)
        return s->begin_error;
    *lease = s;
    return CUI_TEXT_MATCH_OK;
}
static cui_text_match_result match(void *p, cui_str text, cui_text_match_buffer scratch,
                                   bool *out) {
    spy *s = p;
    (void)scratch;
    s->matches++;
    mark(s, 'R');
    mutate(s, 2);
    if (s->match_error)
        return s->match_error;
    *out = !memchr(text.data, 'x', text.length);
    return CUI_TEXT_MATCH_OK;
}
static cui_text_match_result end(void *p, void *lease) {
    spy *s = p;
    assert(lease == s);
    s->ends++;
    mark(s, 'E');
    mutate(s, 3);
    return s->end_error;
}
static bool custom(void *p, cui_str text) {
    spy *s = p;
    s->predicates++;
    mark(s, 'V');
    return !memchr(text.data, 'z', text.length);
}
static cui_edit_result prepare(void *p, cui_str text, bool provisional) {
    spy *s = p;
    (void)text;
    (void)provisional;
    s->prepares++;
    mark(s, 'P');
    return CUI_EDIT_OK;
}
static cui_edit_request request(spy *s, const char *text) {
    return (cui_edit_request){.kind = CUI_EDIT_REQUEST_INSERT,
                              .text = cui_string(text),
                              .matcher = {{begin, match, end}, s, sizeof *s, 7, {0}}};
}
static cui_edit_checked_result insert(spy *s, const char *t) {
    return cui_edit_apply_checked(s->edit, request(s, t), (cui_edit_acceptance){custom, s});
}
static void unchanged(cui_edit *e, cui_edit_state before, const char *value) {
    cui_edit_state after = cui_edit_get_state(e);
    assert(after.revision == before.revision && after.cursor == before.cursor &&
           after.anchor == before.anchor);
    assert(!strcmp(cui_edit_get_text(e).data, value));
}
int main(void) {
    char buffer[128] = "a";
    cui_edit *e = cui_edit_create((cui_edit_config){buffer, sizeof buffer, false, 4096, 32});
    assert(e);
    spy s = {.edit = e};
    cui_edit_select(e, 1, 1);
    cui_edit_state before = cui_edit_get_state(e);
    cui_edit_checked_result r = insert(&s, "x");
    assert(r.error == CUI_EDIT_OK && r.rejected);
    unchanged(e, before, "a");
    assert(s.starts == 1 && s.ends == 1 && s.matches == 2 && s.predicates == 2);
    assert(!memcmp(s.order, "BVRVRE", 6));
    s = (spy){.edit = e, .match_error = CUI_TEXT_MATCH_FAILED};
    r = insert(&s, "x");
    assert(r.error == CUI_EDIT_MATCHER && !r.rejected && s.matches == 1 && s.ends == 1);
    unchanged(e, before, "a");
    s = (spy){.edit = e, .end_error = CUI_TEXT_MATCH_CAPACITY};
    r = insert(&s, "b");
    assert(r.error == CUI_EDIT_CAPACITY && !r.rejected && s.ends == 1);
    unchanged(e, before, "a");
    for (unsigned phase = 1; phase <= 3; phase++) {
        s = (spy){.edit = e, .mutate = phase};
        r = insert(&s, "b");
        assert(r.error == CUI_EDIT_INVALID_ARGUMENT && !r.rejected);
        unchanged(e, before, "a");
        assert(s.ends == 1);
    }
    for (int error = CUI_TEXT_MATCH_INVALID; error <= CUI_TEXT_MATCH_FAILED; error++) {
        s = (spy){.edit = e, .begin_error = (cui_text_match_result)error};
        r = insert(&s, "b");
        assert(r.error != CUI_EDIT_OK && !r.rejected && s.ends == 0 && s.predicates == 0);
        unchanged(e, before, "a");
    }
    s = (spy){.edit = e};
    cui_edit_request q = request(&s, "b");
    q.matcher.generation = 6;
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){0});
    assert(r.error == CUI_EDIT_MATCHER_STALE);
    unchanged(e, before, "a");
    q = request(&s, "b");
    q.matcher.scratch = (cui_text_match_buffer){buffer, sizeof buffer};
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){0});
    assert(r.error == CUI_EDIT_INVALID_ARGUMENT);
    unchanged(e, before, "a");
    q = request(&s, "b");
    q.matcher.ops.end = NULL;
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){0});
    assert(r.error == CUI_EDIT_INVALID_ARGUMENT);
    /* Hard presentation precedes lease/custom; source custom short-circuit still holds. */
    spy presentation = {0};
    s = (spy){.edit = e};
    q = request(&s, "z");
    q.presentation = (cui_edit_presentation){
        .prepare = prepare, .user = &presentation, .user_bytes = sizeof presentation};
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){custom, &s});
    assert(r.rejected && presentation.prepares == 1 && s.starts == 1 && s.matches == 1 &&
           s.ends == 1);
    unchanged(e, before, "a");
    /* Empty is valid without invoking any semantic callback. */
    cui_edit_select(e, 0, cui_edit_get_state(e).length);
    s = (spy){.edit = e};
    r = insert(&s, "");
    assert(r.error == CUI_EDIT_OK && !r.rejected && s.starts == 0 && s.predicates == 0 &&
           !buffer[0]);
    /* Invalid-old healing, then replay bypass of matcher. */
    assert(cui_edit_insert(e, cui_string("x")) == CUI_EDIT_OK);
    s = (spy){.edit = e};
    r = insert(&s, "b");
    assert(r.error == CUI_EDIT_OK && !r.rejected && !strcmp(buffer, "xb"));
    assert(s.matches == 2 && s.ends == 1);
    s = (spy){.edit = e, .begin_error = CUI_TEXT_MATCH_FAILED};
    q = request(&s, "");
    q.kind = CUI_EDIT_REQUEST_EVENT;
    q.event = (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL};
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){custom, &s});
    assert(r.error == CUI_EDIT_OK && s.starts == 0);
    /* IME candidate/old view and commit lease each exactly once; rejection preserves view. */
    cui_edit_select(e, 0, cui_edit_get_state(e).length);
    assert(cui_edit_insert(e, cui_string("a")) == CUI_EDIT_OK);
    cui_edit_select(e, 1, 1);
    char pre[64] = {0}, view[256] = {0};
    cui_edit_composition c = {
        .buffer = pre, .capacity = sizeof pre, .view_buffer = view, .view_capacity = sizeof view};
    s = (spy){.edit = e};
    q = request(&s, "");
    q.kind = CUI_EDIT_REQUEST_COMPOSITION;
    q.composition = &c;
    q.editable = true;
    q.event = (cui_event){.type = CUI_EVENT_COMPOSITION_START};
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){custom, &s});
    assert(r.error == CUI_EDIT_OK && c.active && s.starts == 0);
    q.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                          .text_data = "b",
                          .text_length = 1,
                          .composition_anchor = 1,
                          .composition_cursor = 1};
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){custom, &s});
    assert(r.error == CUI_EDIT_OK && !strcmp(view, "ab") && s.starts == 1 && s.ends == 1);
    before = cui_edit_get_state(e);
    q.event.text_data = "x";
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){custom, &s});
    assert(r.rejected && !strcmp(view, "ab") && !strcmp(pre, "b"));
    unchanged(e, before, "a");
    q.event = (cui_event){.type = CUI_EVENT_COMPOSITION_COMMIT,
                          .text_data = "b",
                          .text_length = 1,
                          .composition_anchor = 1,
                          .composition_cursor = 1};
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){custom, &s});
    assert(r.error == CUI_EDIT_OK && !c.active && !strcmp(buffer, "ab"));
    assert(cui_edit_undo(e) == CUI_EDIT_OK && !strcmp(buffer, "a"));
    /* Final resources are prepared from formatted text, semantics see raw text.
     * A mask-invalid candidate short-circuits regex but still checks old validity. */
    cui_edit_select(e, 0, cui_edit_get_state(e).length);
    assert(cui_edit_insert(e, cui_string("12")) == CUI_EDIT_OK);
    char formatted[128];
    s = (spy){.edit = e};
    q = request(&s, "34");
    q.format = (cui_edit_format){.kind = CUI_EDIT_FORMAT_PATTERN, .pattern = {"99-99", 5}};
    q.format_workspace = (cui_edit_workspace){formatted, sizeof formatted};
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){custom, &s});
    assert(r.error == CUI_EDIT_OK && !r.rejected && !strcmp(buffer, "12-34") && s.starts == 1 &&
           s.matches == 1 && s.ends == 1);
    assert(cui_edit_undo(e) == CUI_EDIT_OK && !strcmp(buffer, "12"));
    before = cui_edit_get_state(e);
    s = (spy){.edit = e};
    q.matcher.object = &s;
    q.text = cui_string("x");
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){custom, &s});
    assert(r.rejected && s.matches == 1 && s.ends == 1);
    unchanged(e, before, "12");
    /* No provider callbacks on malformed/overlapping descriptors. */
    s = (spy){.edit = e};
    q = request(&s, "b");
    q.matcher.scratch = (cui_text_match_buffer){&s, sizeof s};
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){0});
    assert(r.error == CUI_EDIT_INVALID_ARGUMENT && s.starts == 0);
    q = request(&s, "b");
    q.matcher.scratch = (cui_text_match_buffer){(void *)q.text.data, q.text.length};
    r = cui_edit_apply_checked(e, q, (cui_edit_acceptance){0});
    assert(r.error == CUI_EDIT_INVALID_ARGUMENT && s.starts == 0);
    s = (spy){.edit = e, .match_error = (cui_text_match_result)999};
    r = insert(&s, "b");
    assert(r.error == CUI_EDIT_INVALID_ARGUMENT && !r.rejected && s.ends == 1);
    unchanged(e, before, "12");
    cui_edit_destroy(e);
    return 0;
}
