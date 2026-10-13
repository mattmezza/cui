/* SPDX-License-Identifier: MIT */
#include "cui/cui_edit.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    cui_edit *edit;
    char *stage;
    size_t capacity;
    unsigned prepares, predicates, allocations;
    bool allocate, fail_allocation;
    char order[64];
    size_t order_length;
    cui_edit_result failure;
    bool reenter, reject_x, provisional;
} spy;
static void record(spy *s, char c) {
    assert(s->order_length + 1 < sizeof s->order);
    s->order[s->order_length++] = c;
    s->order[s->order_length] = 0;
}
static void *allocate_stage(spy *s) {
    s->allocations++;
    return s->fail_allocation ? NULL : malloc(s->capacity);
}
static cui_edit_result prepare(void *user, cui_str value, bool provisional) {
    spy *s = user;
    s->prepares++;
    s->provisional = provisional;
    record(s, 'P');
    if (s->reenter)
        cui_edit_destroy(s->edit);
    if (s->failure != CUI_EDIT_OK)
        return s->failure;
    char *storage = s->allocate ? allocate_stage(s) : s->stage;
    if (!storage)
        return CUI_EDIT_CAPACITY;
    cui_str display;
    cui_edit_result result = cui_edit_mask_text(value, storage, s->capacity, &display);
    if (s->allocate) {
        if (result == CUI_EDIT_OK)
            memcpy(s->stage, storage, display.length + 1);
        free(storage);
    }
    return result;
}
static bool validate(void *user, cui_str value) {
    spy *s = user;
    s->predicates++;
    record(s, 'V');
    return !s->reject_x || !memchr(value.data, 'X', value.length);
}
static cui_edit_request request(spy *s, cui_str text) {
    return (cui_edit_request){.kind = CUI_EDIT_REQUEST_INSERT,
                              .text = text,
                              .presentation = {.prepare = prepare,
                                               .user = s,
                                               .user_bytes = sizeof *s,
                                               .caller_workspaces = {{s->stage, s->capacity}},
                                               .caller_workspace_count = 1}};
}
static cui_edit *model(char *buffer, size_t capacity) {
    cui_edit *e = cui_edit_create((cui_edit_config){
        .buffer = buffer, .capacity = capacity, .history_bytes = 2048, .max_history_steps = 16});
    assert(e);
    return e;
}
static void same(cui_edit *e, cui_edit_state before, const char *text) {
    cui_edit_state after = cui_edit_get_state(e);
    assert(after.length == before.length && after.anchor == before.anchor &&
           after.cursor == before.cursor && after.revision == before.revision &&
           after.can_undo == before.can_undo && after.can_redo == before.can_redo &&
           !strcmp(cui_edit_get_text(e).data, text));
}
static void helpers(void) {
    cui_str source = cui_string("aé😀e\xcc\x81🇺🇸"); /* 7 scalars, 5 grapheme clusters. */
    char display[32] = "old";
    cui_str output = {"unchanged", 9};
    assert(cui_edit_mask_text(source, display, sizeof display, &output) == CUI_EDIT_OK &&
           output.length == 21);
    for (size_t i = 0; i < 7; i++)
        assert(!memcmp(display + i * 3, "\xe2\x80\xa2", 3));
    assert(!display[21]);
    size_t source_offsets[] = {0, 1, 3, 7, 8, 10, 14, 18};
    for (size_t i = 0; i < 8; i++) {
        size_t out = 999, back = 999;
        assert(cui_edit_mask_offset(source, source_offsets[i], &out) == CUI_EDIT_OK &&
               out == i * 3);
        assert(cui_edit_unmask_offset(source, i * 3, &back) == CUI_EDIT_OK &&
               back == source_offsets[i]);
    }
    size_t mapped = 999;
    assert(cui_edit_mask_offset(source, 2, &mapped) == CUI_EDIT_BOUNDARY && mapped == 999);
    assert(cui_edit_unmask_offset(source, 2, &mapped) == CUI_EDIT_BOUNDARY && mapped == 999);
    assert(cui_edit_unmask_offset(source, 24, &mapped) == CUI_EDIT_BOUNDARY && mapped == 999);
    strcpy(display, "old");
    output = (cui_str){"unchanged", 9};
    assert(cui_edit_mask_text(source, display, 21, &output) == CUI_EDIT_CAPACITY &&
           !strcmp(display, "old") && output.length == 9);
    assert(cui_edit_mask_text((cui_str){"\xff", 1}, display, sizeof display, &output) ==
               CUI_EDIT_INVALID_UTF8 &&
           !strcmp(display, "old"));
    assert(cui_edit_mask_text(cui_string(display), display, sizeof display, &output) ==
               CUI_EDIT_INVALID_ARGUMENT &&
           !strcmp(display, "old"));
    union {
        cui_str value;
        size_t offset;
        char bytes[48];
    } aliased;
    memset(&aliased, 0x33, sizeof aliased);
    unsigned char saved[sizeof aliased];
    memcpy(saved, &aliased, sizeof saved);
    assert(cui_edit_mask_text(cui_string("a"), aliased.bytes, sizeof aliased, &aliased.value) ==
               CUI_EDIT_INVALID_ARGUMENT &&
           !memcmp(saved, &aliased, sizeof saved));
    assert(cui_edit_mask_offset((cui_str){aliased.bytes, 1}, 0, &aliased.offset) ==
               CUI_EDIT_INVALID_ARGUMENT &&
           !memcmp(saved, &aliased, sizeof saved));
    assert(cui_edit_unmask_offset((cui_str){aliased.bytes, 1}, 0, &aliased.offset) ==
               CUI_EDIT_INVALID_ARGUMENT &&
           !memcmp(saved, &aliased, sizeof saved));
    assert(cui_edit_mask_text((cui_str){0}, display, 1, &output) == CUI_EDIT_OK && !output.length &&
           !display[0]);
}
static void replacement(void) {
    char buffer[64] = "a", stage[64] = {0};
    cui_edit *e = model(buffer, sizeof buffer);
    spy s = {.edit = e, .stage = stage, .capacity = 7, .reject_x = true};
    assert(cui_edit_select(e, 1, 1) == CUI_EDIT_OK);
    cui_edit_state before = cui_edit_get_state(e);
    cui_edit_request r = request(&s, cui_string("bc"));
    cui_edit_checked_result out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.error == CUI_EDIT_CAPACITY && !out.rejected && !out.presentation_prepared &&
           s.prepares == 1 && !s.predicates);
    same(e, before, "a");
    s.capacity = sizeof stage;
    r = request(&s, cui_string("X"));
    s.order_length = 0;
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.error == CUI_EDIT_OK && out.rejected && out.presentation_prepared &&
           !out.presentation_applied && !strcmp(s.order, "PVV"));
    same(e, before, "a");
    assert(cui_edit_insert(e, cui_string("X")) == CUI_EDIT_OK);
    s.order_length = 0;
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.error == CUI_EDIT_OK && !out.rejected && out.presentation_applied &&
           !strcmp(s.order, "PVV") && !strcmp(buffer, "aXX"));
    r = request(&s, cui_string("b\r\n"));
    r.normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF;
    s.order_length = 0;
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){0});
    assert(out.error == CUI_EDIT_OK && out.presentation_applied && !strcmp(s.order, "P") &&
           !strcmp(buffer, "aXXb"));
    assert(!memcmp(stage, "••••", 12) && !stage[12]);
    before = cui_edit_get_state(e);
    s.failure = CUI_EDIT_PRESENTATION;
    r = request(&s, cui_string("q"));
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.error == CUI_EDIT_PRESENTATION && !out.presentation_prepared);
    same(e, before, "aXXb");
    s.allocate = true;
    s.fail_allocation = true;
    s.failure = CUI_EDIT_OK;
    unsigned old_predicates = s.predicates;
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.error == CUI_EDIT_CAPACITY && !out.presentation_prepared && s.allocations == 1 &&
           s.predicates == old_predicates);
    same(e, before, "aXXb");
    s.fail_allocation = false;
    s.failure = CUI_EDIT_BOUNDARY;
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){0});
    assert(out.error == CUI_EDIT_INVALID_ARGUMENT);
    same(e, before, "aXXb");
    s.failure = CUI_EDIT_OK;
    s.reenter = true;
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.error == CUI_EDIT_INVALID_ARGUMENT && out.presentation_prepared &&
           !out.presentation_applied);
    same(e, before, "aXXb");
    cui_edit_destroy(e);
}
static void replay_and_grouping(void) {
    char buffer[64] = "", stage[64];
    cui_edit *e = model(buffer, sizeof buffer);
    spy s = {.edit = e, .stage = stage, .capacity = sizeof stage};
    cui_edit_request r = request(&s, (cui_str){0});
    r.kind = CUI_EDIT_REQUEST_EVENT;
    r.event = (cui_event){.type = CUI_EVENT_TEXT, .text_data = "a", .text_length = 1};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s}).presentation_applied);
    s.failure = CUI_EDIT_PRESENTATION;
    cui_edit_state before = cui_edit_get_state(e);
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s}).error ==
           CUI_EDIT_PRESENTATION);
    same(e, before, "a");
    s.failure = CUI_EDIT_OK;
    r.event.text_data = "b";
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s}).presentation_applied);
    r.event = (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL};
    unsigned predicates = s.predicates, prepares = s.prepares;
    s.failure = CUI_EDIT_CAPACITY;
    before = cui_edit_get_state(e);
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s}).error ==
           CUI_EDIT_CAPACITY);
    same(e, before, "ab");
    s.failure = CUI_EDIT_OK;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s}).presentation_applied &&
           !buffer[0] && s.predicates == predicates && s.prepares == prepares + 2);
    r.event.key = CUI_KEY_Y;
    s.capacity = 4;
    r.presentation.caller_workspaces[0].bytes = 4;
    before = cui_edit_get_state(e);
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s}).error ==
           CUI_EDIT_CAPACITY);
    same(e, before, "");
    s.capacity = sizeof stage;
    r.presentation.caller_workspaces[0].bytes = sizeof stage;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s}).presentation_applied &&
           !strcmp(buffer, "ab") && s.predicates == predicates);
    cui_edit_destroy(e);
}
static void aliases(void) {
    char buffer[32] = "ab", stage[32] = {0};
    cui_edit *e = model(buffer, sizeof buffer);
    spy s = {.edit = e, .stage = stage, .capacity = sizeof stage};
    cui_edit_state before = cui_edit_get_state(e);
    cui_edit_request valid = request(&s, cui_string("q")), r = valid;
    r.presentation.caller_workspaces[0] = (cui_edit_workspace){buffer, sizeof buffer};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    r = valid;
    r.presentation.user = e;
    r.presentation.user_bytes = 1;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    r = valid;
    r.presentation.caller_descriptor = (cui_edit_workspace){buffer, 1};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    r = valid;
    r.presentation.caller_workspace_count = 4;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    r = valid;
    r.presentation.caller_workspaces[1] = (cui_edit_workspace){stage, 1};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    r = valid;
    r.presentation.caller_workspace_count = 2;
    r.presentation.caller_workspaces[1] = (cui_edit_workspace){stage + 2, 1};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    r = valid;
    r.text = (cui_str){stage, 1};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    r = valid;
    r.presentation.user_bytes = 0;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    r = valid;
    r.presentation.prepare = NULL;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    assert(!s.prepares);
    same(e, before, "ab");
    char other[32], third[32];
    unsigned descriptor = 123;
    r = valid;
    r.presentation.caller_workspace_count = 3;
    r.presentation.caller_workspaces[1] = (cui_edit_workspace){other, sizeof other};
    r.presentation.caller_workspaces[2] = (cui_edit_workspace){third, sizeof third};
    r.presentation.caller_descriptor = (cui_edit_workspace){&descriptor, sizeof descriptor};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).presentation_applied &&
           s.prepares == 1);
    cui_edit_destroy(e);
    strcpy(buffer, "ab");
    e = cui_edit_create(
        (cui_edit_config){.buffer = buffer, .capacity = sizeof buffer, .multiline = true});
    assert(e);
    s.edit = e;
    r = request(&s, cui_string("q"));
    before = cui_edit_get_state(e);
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error ==
           CUI_EDIT_INVALID_ARGUMENT);
    same(e, before, "ab");
    cui_edit_destroy(e);
}
static void composition(bool grow) {
    char buffer[32] = "ab", stage[64] = {0}, preedit[32] = {0}, view[64] = {0};
    cui_edit *e = model(buffer, sizeof buffer);
    cui_edit_composition c = {.buffer = preedit,
                              .capacity = sizeof preedit,
                              .view_buffer = view,
                              .view_capacity = sizeof view,
                              .grow_view = grow};
    spy s = {.edit = e, .stage = stage, .capacity = sizeof stage, .reject_x = true};
    assert(cui_edit_select(e, 0, 2) == CUI_EDIT_OK);
    cui_edit_request r = request(&s, (cui_str){0});
    r.kind = CUI_EDIT_REQUEST_COMPOSITION;
    r.composition = &c;
    r.editable = true;
    r.normalization = CUI_EDIT_NORMALIZE_SINGLE_LINE_CRLF;
    r.event = (cui_event){.type = CUI_EVENT_COMPOSITION_START};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s}).error == CUI_EDIT_OK &&
           !s.prepares);
    r.event = (cui_event){.type = CUI_EVENT_COMPOSITION_UPDATE,
                          .text_data = "é\r\n😀",
                          .text_length = 8,
                          .composition_anchor = 2,
                          .composition_cursor = 8};
    cui_edit_checked_result out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.error == CUI_EDIT_OK && out.presentation_applied && s.provisional &&
           s.prepares == 1 && c.anchor == 2 && c.cursor == 6);
    char old_view[64];
    cui_str visible = cui_edit_composition_view(e, &c);
    memcpy(old_view, visible.data, visible.length + 1);
    size_t old_length = visible.length;
    cui_edit_state before = cui_edit_get_state(e);
    s.failure = CUI_EDIT_PRESENTATION;
    r.event.text_data = "q";
    r.event.text_length = 1;
    r.event.composition_anchor = r.event.composition_cursor = 1;
    unsigned predicates = s.predicates;
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.error == CUI_EDIT_PRESENTATION && !out.presentation_prepared &&
           s.predicates == predicates);
    same(e, before, "ab");
    visible = cui_edit_composition_view(e, &c);
    assert(c.active && visible.length == old_length &&
           !memcmp(old_view, visible.data, old_length + 1));
    s.failure = CUI_EDIT_OK;
    r.event.text_data = "X";
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.rejected && out.presentation_prepared && !out.presentation_applied);
    visible = cui_edit_composition_view(e, &c);
    assert(!memcmp(old_view, visible.data, old_length + 1));
    r.event.type = CUI_EVENT_COMPOSITION_COMMIT;
    r.event.text_data = "q\r\n";
    r.event.text_length = 3;
    r.event.composition_cursor = 3;
    unsigned prepares = s.prepares;
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){validate, &s});
    assert(out.presentation_applied && !s.provisional && s.prepares == prepares + 1 && !c.active &&
           !strcmp(buffer, "q"));
    assert(cui_edit_undo(e) == CUI_EDIT_OK && !strcmp(buffer, "ab"));
    r.event.type = CUI_EVENT_COMPOSITION_START;
    r.event.text_data = "";
    r.event.text_length = 0;
    r.event.composition_anchor = r.event.composition_cursor = 0;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK);
    s.capacity = 4;
    r.presentation.caller_workspaces[0].bytes = 4;
    r.event.type = CUI_EVENT_COMPOSITION_UPDATE;
    r.event.text_data = "cd";
    r.event.text_length = 2;
    before = cui_edit_get_state(e);
    out = cui_edit_apply_checked(e, r, (cui_edit_acceptance){0});
    assert(out.error == CUI_EDIT_CAPACITY);
    same(e, before, "ab");
    assert(c.active && !c.length);
    r.event.type = CUI_EVENT_COMPOSITION_CANCEL;
    prepares = s.prepares;
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK &&
           s.prepares == prepares && !c.active);
    cui_edit_destroy(e);
}
static void growing_failure(void) {
    char buffer[32] = "a", stage[64];
    char *preedit = calloc(5000, 1), *payload = malloc(4501);
    assert(preedit && payload);
    memset(payload, 'q', 4500);
    payload[4500] = 0;
    cui_edit *e = model(buffer, sizeof buffer);
    cui_edit_composition c = {.buffer = preedit, .capacity = 5000, .grow_view = true};
    spy s = {.edit = e, .stage = stage, .capacity = sizeof stage};
    cui_edit_request r = request(&s, (cui_str){0});
    r.kind = CUI_EDIT_REQUEST_COMPOSITION;
    r.composition = &c;
    r.editable = true;
    r.event = (cui_event){.type = CUI_EVENT_COMPOSITION_START};
    assert(cui_edit_apply_checked(e, r, (cui_edit_acceptance){0}).error == CUI_EDIT_OK);
    cui_str old = cui_edit_composition_view(e, &c);
    const char *pointer = old.data;
    cui_edit_state before = cui_edit_get_state(e);
    r.event = (cui_event){
        .type = CUI_EVENT_COMPOSITION_UPDATE, .text_data = payload, .text_length = 4500};
    cui_edit_checked_result result = cui_edit_apply_checked(e, r, (cui_edit_acceptance){0});
    assert(result.error == CUI_EDIT_CAPACITY && s.prepares == 1 && !result.presentation_prepared);
    cui_str retained = cui_edit_composition_view(e, &c);
    assert(retained.data == pointer && retained.length == old.length &&
           !strcmp(retained.data, "a") && !c.length);
    same(e, before, "a");
    cui_edit_destroy(e);
    free(payload);
    free(preedit);
}
int main(void) {
    helpers();
    growing_failure();
    replacement();
    replay_and_grouping();
    aliases();
    composition(false);
    composition(true);
    return 0;
}
