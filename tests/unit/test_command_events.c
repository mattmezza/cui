/* SPDX-License-Identifier: MIT */
#include "cui/cui_command_events.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    cui_command_state state;
    cui_command_row rows[8];
    cui_command_entry entries[3];
    cui_command_options options;
    cui_command_event_handler handler;
    cui_command_selection_change selection;
    cui_command_activation activation;
    size_t selections, queries, activations;
    char trace[64], copied[32];
    size_t n;
    cui_str retained;
    char *buffer;
    cui_edit *edit;
    bool redirect, replace_select, replace_query, teardown;
} fixture;
static void metadata(const cui_component_event_metadata *e, cui_component_event_kind kind) {
    assert(e->target == 90 && e->kind == kind);
    assert(e->source == CUI_COMPONENT_EVENT_SOURCE_PROGRAMMATIC);
    assert(e->flags == CUI_COMPONENT_EVENT_HAS_SOURCE && !e->frame_sequence);
}
static void replace_editor(fixture *f) {
    /* The editor attaches caller-owned storage: detach before actual realloc. */
    cui_edit_destroy(f->edit);
    uintptr_t previous = (uintptr_t)f->buffer;
    f->buffer = realloc(f->buffer, 1024 * 1024);
    assert(f->buffer);
#if defined(__SANITIZE_ADDRESS__)
    assert((uintptr_t)f->buffer != previous); /* ASan must exercise a moved allocation. */
#else
    (void)previous;
#endif
    strcpy(f->buffer, "replacement");
    f->edit = cui_edit_create((cui_edit_config){.buffer = f->buffer, .capacity = 1024 * 1024});
    assert(f->edit);
    f->state.query = f->edit;
}
static void old_select(cui_command_index index, void *user) {
    fixture *f = user;
    assert(index.section == 0);
    f->trace[f->n++] = 'S';
    if (f->redirect)
        assert(cui_command_select(&f->state, (cui_command_index){0, 2}, false,
                                  (cui_command_options){0}));
    if (f->replace_select)
        replace_editor(f);
}
static void old_query(cui_str value, void *user) {
    fixture *f = user;
    assert(value.length == 4 && !memcmp(value.data, " Two", 4));
    f->trace[f->n++] = 'Q';
    if (f->replace_query)
        replace_editor(f);
}
static void old_confirm(cui_command_index index, void *user) {
    fixture *f = user;
    assert(index.row == 1);
    f->trace[f->n++] = 'C';
}
static void action(void *user) {
    fixture *f = user;
    f->trace[f->n++] = 'A';
    /* Same legacy mutation as before: captured confirm identity remains row 1. */
    assert(
        cui_command_select(&f->state, (cui_command_index){0, 2}, false, (cui_command_options){0}));
}
static void selection(void *user, const cui_command_selection_change *event) {
    fixture *f = user;
    metadata(&event->event, CUI_COMPONENT_EVENT_CHANGE);
    f->selection = *event;
    f->selections++;
    f->trace[f->n++] = 's';
    f->teardown = true;
}
static void query(void *user, const cui_command_query_change *event) {
    fixture *f = user;
    metadata(&event->event, CUI_COMPONENT_EVENT_CHANGE);
    assert(cui_component_event_copy_utf8(event->value, 31, f->copied, sizeof f->copied,
                                         &f->retained) == CUI_COMPONENT_EVENT_COPY_OK);
    f->queries++;
    f->trace[f->n++] = 'q';
}
static void activation(void *user, const cui_command_activation *event) {
    fixture *f = user;
    metadata(&event->event, CUI_COMPONENT_EVENT_ACTIVATE);
    f->activation = *event;
    f->activations++;
    f->trace[f->n++] = 'a';
    f->teardown = true;
}
static void init(fixture *f) {
    memset(f, 0, sizeof *f);
    assert(cui_command_init(&f->state, f->rows, 8, NULL));
    f->entries[0] = (cui_command_entry){.kind = CUI_COMMAND_ITEM, .item = {.label = {"One", 3}}};
    f->entries[1] = (cui_command_entry){.kind = CUI_COMMAND_ITEM, .item = {.label = {"Two", 3}}};
    f->entries[2] = (cui_command_entry){.kind = CUI_COMMAND_ITEM, .item = {.label = {"Three", 5}}};
    f->options = cui_command_default();
    f->options.filterable = false;
    f->handler = (cui_command_event_handler){
        .on_selection = selection, .on_query = query, .on_activate = activation, .user = f};
}
static void hooks(fixture *f) {
    f->options.on_select = old_select;
    f->options.on_query = old_query;
    f->options.on_confirm = old_confirm;
    f->options.user = f;
}
static void compare(fixture *a, fixture *b) {
    assert(a->state.row_count == b->state.row_count &&
           a->state.matched_count == b->state.matched_count);
    assert(a->state.selected == b->state.selected &&
           a->state.pending_scroll == b->state.pending_scroll &&
           a->state.preserve_no_selection == b->state.preserve_no_selection);
    for (size_t i = 0; i < a->state.row_count; i++)
        assert(cui_command_index_equal(a->state.rows[i].index, b->state.rows[i].index) &&
               a->state.rows[i].kind == b->state.rows[i].kind);
}
static void predicates(void) {
    fixture a, b;
    init(&a);
    init(&b);
    bool old = cui_command_rebuild(&a.state, a.entries, 3, cui_string(""), false, a.options);
    cui_command_event_result r = cui_command_rebuild_events(
        &b.state, 90, b.entries, 3, cui_string(""), false, b.options, b.handler);
    assert(r.control == old && !r.selection_count && !r.query_count);
    compare(&a, &b);
    for (size_t i = 0; i < 4; i++) {
        old = cui_command_move(&a.state, true, a.options);
        r = cui_command_move_events(&b.state, 90, true, b.options, b.handler);
        assert(r.control == old && r.selection_count == 1);
        compare(&a, &b);
    }
    r = cui_command_select_events(&b.state, 90, (cui_command_index){0, 1}, true, b.options,
                                  b.handler);
    assert(!r.control && !r.selection_count);
    b.options.disabled = true;
    old = cui_command_move(&a.state, false, a.options);
    r = cui_command_move_events(&b.state, 90, false, b.options, b.handler);
    assert(r.control == old && r.selection_count == 1);
    compare(&a, &b);
    r = cui_command_confirm_events(&b.state, 90, b.options, b.handler);
    assert(!r.control && !r.activation_count);
    b.options.disabled = false;
    b.state.loading = true;
    r = cui_command_confirm_events(&b.state, 90, b.options, b.handler);
    assert(r.control && r.activation_count == 1);
    r = cui_command_select_events(&b.state, 90, (cui_command_index){9, 9}, true, b.options,
                                  b.handler);
    assert(!r.control && !r.selection_count);
    cui_command_clear_selection(&b.state);
    r = cui_command_confirm_events(&b.state, 90, b.options, b.handler);
    assert(!r.control);
    r = cui_command_move_events(&b.state, 90, true, b.options, b.handler);
    assert(r.control && !b.selection.has_previous);
    /* Explicit changed=true initialization, unlike Styled's initial false flag. */
    init(&b);
    r = cui_command_rebuild_events(&b.state, 90, b.entries, 3, cui_string(" Two"), true, b.options,
                                   b.handler);
    assert(r.control && r.selection_count == 1 && r.query_count == 1 && !b.selection.has_previous);
    r = cui_command_rebuild_events(&b.state, 90, b.entries, 3, cui_string(" Two"), true, b.options,
                                   b.handler);
    assert(r.control && !r.selection_count && r.query_count == 1);
    b.entries[0].item.disabled = true;
    r = cui_command_rebuild_events(&b.state, 90, b.entries, 3, cui_string(""), false, b.options,
                                   b.handler);
    assert(r.control && !r.selection_count && !r.query_count && b.state.selected == 1);
    b.entries[1].item.disabled = b.entries[2].item.disabled = true;
    r = cui_command_rebuild_events(&b.state, 90, b.entries, 3, cui_string(""), true, b.options,
                                   b.handler);
    assert(r.control && !r.selection_count && r.query_count == 1 && b.state.selected == SIZE_MAX);
    b.options.searchable = false;
    r = cui_command_rebuild_events(&b.state, 90, b.entries, 3, cui_string(""), true, b.options,
                                   b.handler);
    assert(r.control && !r.query_count);
}
static void ordering_and_realloc(void) {
    for (unsigned replace = 0; replace < 3; replace++) {
        fixture f;
        init(&f);
        hooks(&f);
        f.buffer = malloc(16);
        assert(f.buffer);
        strcpy(f.buffer, " Two");
        f.edit = cui_edit_create((cui_edit_config){.buffer = f.buffer, .capacity = 16});
        assert(f.edit);
        f.state.query = f.edit;
        f.replace_select = replace == 1;
        f.replace_query = replace == 2;
        if (f.replace_select)
            f.options.on_query = NULL;
        f.redirect = true;
        cui_command_event_result r = cui_command_rebuild_events(
            &f.state, 90, f.entries, 3, cui_edit_get_text(f.edit), true, f.options, f.handler);
        assert(r.control && r.query_count == 1 && r.selection_count == 1);
        assert(f.n == (replace == 1 ? 3u : 4u) &&
               !memcmp(f.trace, replace == 1 ? "qSs" : "qSsQ", f.n));
        assert(f.state.selected == 2 && f.selection.value.row == 0 && !f.selection.has_previous);
        assert(f.retained.length == 4 && !memcmp(f.retained.data, " Two", 4));
        if (replace)
            assert(!strcmp(cui_edit_get_text(f.edit).data, "replacement"));
        cui_edit_destroy(f.edit);
        free(f.buffer);
        assert(!memcmp(f.retained.data, " Two", 4));
    }
    fixture f;
    init(&f);
    hooks(&f);
    f.options.on_query = NULL;
    assert(cui_command_rebuild(&f.state, f.entries, 3, cui_string(""), false, f.options));
    f.entries[1].item.action = action;
    f.entries[1].item.action_user = &f;
    assert(cui_command_select(&f.state, (cui_command_index){0, 1}, true, (cui_command_options){0}));
    cui_command_event_result r = cui_command_confirm_events(&f.state, 90, f.options, f.handler);
    assert(r.control && r.activation_count == 1 && f.activation.value.row == 1 &&
           f.state.selected == 2);
    assert(f.n == 3 && !memcmp(f.trace, "ACa", 3));
}
static void legacy_pairs(void) {
    fixture a, b;
    init(&a);
    init(&b);
    hooks(&a);
    hooks(&b);
    a.redirect = b.redirect = true;
    bool old = cui_command_rebuild(&a.state, a.entries, 3, cui_string(" Two"), true, a.options);
    cui_command_event_result r = cui_command_rebuild_events(
        &b.state, 90, b.entries, 3, cui_string(" Two"), true, b.options, b.handler);
    assert(old == r.control);
    compare(&a, &b);
    assert(a.n == 2 && !memcmp(a.trace, "SQ", 2));
    assert(b.n == 4 && !memcmp(b.trace, "qSsQ", 4));
    assert(b.selection.value.row == 0 && b.state.selected == 2);
    a.n = b.n = 0;
    a.redirect = b.redirect = false;
    old = cui_command_select(&a.state, (cui_command_index){0, 1}, true, a.options);
    r = cui_command_select_events(&b.state, 90, (cui_command_index){0, 1}, true, b.options,
                                  b.handler);
    assert(old == r.control && r.selection_count == 1);
    compare(&a, &b);
    assert(a.n == 1 && a.trace[0] == 'S' && b.n == 2 && !memcmp(b.trace, "Ss", 2));
    a.n = b.n = 0;
    a.entries[1].item.action = b.entries[1].item.action = action;
    a.entries[1].item.action_user = &a;
    b.entries[1].item.action_user = &b;
    old = cui_command_confirm(&a.state, a.options);
    r = cui_command_confirm_events(&b.state, 90, b.options, b.handler);
    assert(old == r.control && r.activation_count == 1);
    compare(&a, &b);
    assert(a.n == 2 && !memcmp(a.trace, "AC", 2) && b.n == 3 && !memcmp(b.trace, "ACa", 3));
    assert(b.activation.value.row == 1 && b.state.selected == 2 && b.teardown);
}
static void failures_and_copy(void) {
    fixture f;
    init(&f);
    cui_command_state before = f.state;
    cui_command_event_result r = cui_command_rebuild_events(
        &f.state, 0, f.entries, 3, cui_string(" Two"), true, f.options, f.handler);
    assert(!r.control && !memcmp(&before, &f.state, sizeof before) && !f.queries);
    f.state.capacity = 1;
    before = f.state;
    r = cui_command_rebuild_events(&f.state, 90, f.entries, 3, cui_string(" Two"), true, f.options,
                                   f.handler);
    assert(!r.control && !memcmp(&before, &f.state, sizeof before) && !f.queries);
    assert(!cui_command_move_events(NULL, 90, true, f.options, f.handler).control);
    init(&f);
    cui_command_event_handler none = {.user = &f};
    r = cui_command_rebuild_events(&f.state, 90, f.entries, 3, cui_string(" Two"), true, f.options,
                                   none);
    assert(r.control && !r.query_count && !r.selection_count);
    r = cui_command_confirm_events(&f.state, 90, f.options, none);
    assert(r.control && !r.activation_count);
    char storage[8] = "safe";
    cui_str out = {0};
    assert(cui_component_event_copy_utf8(cui_string("é"), 1, storage, sizeof storage, &out) ==
               CUI_COMPONENT_EVENT_COPY_LIMIT &&
           !strcmp(storage, "safe"));
    assert(cui_component_event_copy_utf8(cui_string("é"), 7, storage, 2, &out) ==
           CUI_COMPONENT_EVENT_COPY_CAPACITY);
    assert(cui_component_event_copy_utf8(cui_string("é"), 7, storage, sizeof storage, &out) ==
               CUI_COMPONENT_EVENT_COPY_OK &&
           out.length == 2);
}
int main(void) {
    predicates();
    legacy_pairs();
    ordering_and_realloc();
    failures_and_copy();
    return 0;
}
