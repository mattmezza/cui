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
typedef struct {
    fixture base;
    cui_context *ui;
    char buffer[64];
    cui_command_event_result result;
    cui_component_event_metadata events[32];
    cui_command_index confirmed;
    size_t calls, old_confirm_calls, old_cancel_calls;
    bool legacy, redirect, consume_content, fail_measure, no_handler;
    cui_draw_list draw;
} styled_fixture;
static void styled_selection(void *user, const cui_command_selection_change *event) {
    styled_fixture *f = user;
    assert(f->calls < 32);
    f->events[f->calls++] = event->event;
}
static void styled_query(void *user, const cui_command_query_change *event) {
    styled_fixture *f = user;
    assert(f->calls < 32);
    f->events[f->calls++] = event->event;
    assert(cui_component_event_copy_utf8(event->value, 31, f->base.copied, sizeof f->base.copied,
                                         &f->base.retained) == CUI_COMPONENT_EVENT_COPY_OK);
}
static void styled_activation(void *user, const cui_command_activation *event) {
    styled_fixture *f = user;
    assert(f->calls < 32);
    f->events[f->calls++] = event->event;
    f->confirmed = event->value;
    assert(!f->base.options.on_confirm || f->old_confirm_calls);
    f->base.teardown = true;
}
static void styled_cancel(void *user, const cui_command_cancel_request *event) {
    styled_fixture *f = user;
    assert(f->calls < 32 && event->event.kind == CUI_COMPONENT_EVENT_REQUEST);
    f->events[f->calls++] = event->event;
}
static void styled_old_select(cui_command_index index, void *user) {
    styled_fixture *f = user;
    if (f->redirect && index.row == 1)
        assert(cui_command_select(&f->base.state, (cui_command_index){0, 2}, false,
                                  (cui_command_options){0}));
}
static void styled_old_confirm(cui_command_index index, void *user) {
    styled_fixture *f = user;
    (void)index;
    f->old_confirm_calls++;
    cui_focus(f->ui, cui_command_input_id(90));
}
static void styled_old_cancel(void *user) {
    styled_fixture *f = user;
    f->old_cancel_calls++;
}
static void styled_content(cui_context *ui, const cui_theme *theme, const cui_command_state *state,
                           void *user) {
    styled_fixture *f = user;
    (void)state;
    if (f->consume_content) {
        size_t count = 0;
        (void)cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; i++)
            (void)cui_consume_event(ui, i);
    }
    cui_text(ui, cui_string("Two"), 14, theme->text);
}
static float styled_measure(float width, float font, void *user) {
    styled_fixture *f = user;
    (void)width;
    (void)font;
    return f->fail_measure ? -1 : 32;
}
static void styled_init(styled_fixture *f, bool legacy) {
    memset(f, 0, sizeof *f);
    init(&f->base);
    f->legacy = legacy;
    f->ui = cui_create(
        (cui_config){.width = 500, .height = 400, .max_elements = 256, .max_commands = 1024});
    assert(f->ui);
    f->base.edit =
        cui_edit_create((cui_edit_config){.buffer = f->buffer, .capacity = sizeof f->buffer});
    assert(f->base.edit);
    f->base.state.query = f->base.edit;
    f->base.options.on_select = styled_old_select;
    f->base.options.on_confirm = styled_old_confirm;
    f->base.options.user = f;
}
static void styled_finish(styled_fixture *f) {
    cui_edit_destroy(f->base.edit);
    cui_destroy(f->ui);
}
static void styled_frame(styled_fixture *f, unsigned passive, bool success) {
    assert(cui_begin_frame(f->ui, 500, 400, .016f));
    cui_begin_box(f->ui, 800, (cui_box){.inert = passive == 1, .layout_only = passive == 2});
    cui_theme theme = cui_theme_light();
    if (f->legacy)
        f->result.control =
            cui_command(f->ui, 90, &theme, &f->base.state, f->base.entries, 3, f->base.options);
    else
        f->result = cui_command_events(
            f->ui, 90, &theme, &f->base.state, f->base.entries, 3, f->base.options,
            f->no_handler ? (cui_command_event_handler){.user = f}
                          : (cui_command_event_handler){.on_selection = styled_selection,
                                                        .on_query = styled_query,
                                                        .on_activate = styled_activation,
                                                        .on_cancel = styled_cancel,
                                                        .user = f});
    assert(f->result.control == success);
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
}
static void styled_key(styled_fixture *f, cui_key key) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void styled_compare(styled_fixture *a, styled_fixture *b) {
    compare(&a->base, &b->base);
    assert(a->old_confirm_calls == b->old_confirm_calls &&
           cui_focused(a->ui) == cui_focused(b->ui));
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; i++) {
        const cui_draw_command *x = &a->draw.commands[i], *y = &b->draw.commands[i];
        assert(x->type == y->type && !memcmp(&x->bounds, &y->bounds, sizeof x->bounds) &&
               !memcmp(&x->color, &y->color, sizeof x->color));
    }
    size_t an = 0, bn = 0;
    const cui_semantic *as = cui_semantics(a->ui, &an), *bs = cui_semantics(b->ui, &bn);
    assert(an == bn);
    for (size_t i = 0; i < an; i++)
        assert(as[i].id == bs[i].id && as[i].role == bs[i].role &&
               as[i].disabled == bs[i].disabled);
}
static void styled_old_measure_failure(cui_str text, void *user) {
    styled_fixture *f = user;
    (void)text;
    f->fail_measure = true;
}
static void styled_cases(void) {
    styled_fixture a, b;
    styled_init(&a, true);
    styled_init(&b, false);
    styled_frame(&a, 0, true);
    styled_frame(&b, 0, true);
    assert(!b.calls);
    styled_compare(&a, &b);
    cui_command_focus(a.ui, 90, a.base.options);
    cui_command_focus(b.ui, 90, b.base.options);
    styled_key(&a, CUI_KEY_DOWN);
    styled_key(&b, CUI_KEY_DOWN);
    styled_frame(&a, 0, true);
    styled_frame(&b, 0, true);
    styled_compare(&a, &b);
    assert(b.result.selection_count == 1 &&
           b.events[0].source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD &&
           (b.events[0].flags & CUI_COMPONENT_EVENT_HAS_INDEX));
    for (size_t i = 0; i < 2; i++) {
        styled_key(&a, CUI_KEY_ENTER);
        styled_key(&b, CUI_KEY_ENTER);
    }
    styled_frame(&a, 0, true);
    styled_frame(&b, 0, true);
    styled_compare(&a, &b);
    assert(b.result.activation_count == 2 && b.confirmed.row == 1);
    assert(!(b.events[1].flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "T"}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "T"}));
    styled_frame(&a, 0, true);
    styled_frame(&b, 0, true);
    styled_compare(&a, &b);
    assert(b.result.query_count == 1);
    assert(b.events[3].source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN &&
           b.events[3].flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    styled_key(&a, CUI_KEY_ESCAPE);
    styled_key(&b, CUI_KEY_ESCAPE);
    styled_frame(&a, 0, true);
    styled_frame(&b, 0, true);
    styled_compare(&a, &b);
    assert(b.result.query_count == 1 && !b.result.cancel_request_count &&
           cui_event_consumed(b.ui, 0));
    styled_key(&b, CUI_KEY_ESCAPE);
    styled_frame(&b, 0, true);
    assert(b.result.cancel_request_count == 1 && !cui_event_consumed(b.ui, 0));
    b.base.options.on_cancel = styled_old_cancel;
    styled_key(&b, CUI_KEY_ESCAPE);
    styled_frame(&b, 0, true);
    assert(b.old_cancel_calls == 1 && b.result.cancel_request_count == 1 &&
           !cui_event_consumed(b.ui, 0));
    styled_finish(&a);
    styled_finish(&b);
    for (unsigned redirect = 0; redirect < 2; redirect++) {
        styled_init(&a, true);
        styled_init(&b, false);
        a.redirect = b.redirect = redirect != 0;
        a.consume_content = b.consume_content = true;
        a.base.entries[1].item.content = b.base.entries[1].item.content = styled_content;
        a.base.entries[1].item.content_user = &a;
        b.base.entries[1].item.content_user = &b;
        styled_frame(&a, 0, true);
        styled_frame(&b, 0, true);
        cui_rect bounds = cui_bounds(b.ui, cui_command_row_id(90, (cui_command_index){0, 1}));
        for (unsigned up = 0; up < 2; up++) {
            cui_event e = {.type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
                           .x = bounds.x + 10,
                           .y = bounds.y + 10,
                           .pointer_button = CUI_POINTER_PRIMARY};
            assert(cui_queue_event(a.ui, e));
            assert(cui_queue_event(b.ui, e));
        }
        styled_frame(&a, 0, true);
        styled_frame(&b, 0, true);
        styled_compare(&a, &b);
        assert(b.result.activation_count == 1 && b.confirmed.row == (redirect ? 2u : 1u));
        cui_component_event_metadata copied = b.events[b.calls - 1];
        assert(copied.source == CUI_COMPONENT_EVENT_SOURCE_POINTER &&
               (copied.flags & CUI_COMPONENT_EVENT_HAS_INDEX));
        assert(!!(copied.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION) == !redirect);
        styled_finish(&a);
        styled_finish(&b);
        assert(copied.target == 90);
    }
    /* Two accepted pointer activations coalesce; no arbitrary input wins. */
    styled_init(&b, false);
    styled_frame(&b, 0, true);
    cui_rect twice = cui_bounds(b.ui, cui_command_row_id(90, (cui_command_index){0, 1}));
    for (unsigned i = 0; i < 4; i++)
        assert(cui_queue_event(
            b.ui, (cui_event){.type = i % 2 ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
                              .x = twice.x + 10,
                              .y = twice.y + 10,
                              .pointer_button = CUI_POINTER_PRIMARY}));
    styled_frame(&b, 0, true);
    assert(b.result.activation_count == 1);
    assert(b.events[b.calls - 1].flags == CUI_COMPONENT_EVENT_HAS_FRAME &&
           b.events[b.calls - 1].source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    /* Existing pointer_activatable OPTION supports both actions without role remap. */
    cui_id option = cui_command_row_id(90, (cui_command_index){0, 1});
    assert(cui_accessibility_action_supported(b.ui, option, CUI_ACCESSIBILITY_FOCUS));
    assert(cui_accessibility_action_supported(b.ui, option, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_accessibility_action(b.ui, option, CUI_ACCESSIBILITY_FOCUS));
    styled_frame(&b, 0, true);
    assert(!b.result.activation_count);
    assert(cui_queue_accessibility_action(b.ui, option, CUI_ACCESSIBILITY_ACTIVATE));
    styled_frame(&b, 0, true);
    assert(b.result.activation_count == 1);
    assert(b.events[b.calls - 1].source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY &&
           (b.events[b.calls - 1].flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    styled_finish(&b);
    for (unsigned passive = 0; passive < 3; passive++) {
        styled_init(&b, false);
        styled_frame(&b, 0, true);
        b.base.options.disabled = passive == 0;
        assert(cui_edit_insert(b.base.edit, cui_string("T")) == CUI_EDIT_OK);
        styled_frame(&b, passive, true);
        assert(b.result.query_count == 1 &&
               b.events[0].source == CUI_COMPONENT_EVENT_SOURCE_PROGRAMMATIC);
        size_t before = b.calls;
        cui_command_focus(b.ui, 90, b.base.options);
        styled_key(&b, CUI_KEY_DOWN);
        styled_key(&b, CUI_KEY_ENTER);
        styled_frame(&b, passive, true);
        assert(b.calls == before);
        styled_finish(&b);
    }
    styled_init(&b, false);
    styled_frame(&b, 0, true);
    b.base.state.loading = true;
    cui_command_focus(b.ui, 90, b.base.options);
    styled_key(&b, CUI_KEY_ENTER);
    styled_frame(&b, 0, true);
    assert(b.result.activation_count == 1);
    styled_finish(&b);
    styled_init(&b, false);
    b.base.options.on_select = NULL;
    b.base.options.on_confirm = NULL;
    styled_frame(&b, 0, true);
    cui_command_focus(b.ui, 90, b.base.options);
    styled_key(&b, CUI_KEY_ENTER);
    styled_frame(&b, 0, true);
    assert(b.result.activation_count == 1 && !b.old_confirm_calls);
    styled_finish(&b);
    styled_init(&b, false);
    b.no_handler = true;
    styled_frame(&b, 0, true);
    cui_command_focus(b.ui, 90, b.base.options);
    styled_key(&b, CUI_KEY_ENTER);
    styled_frame(&b, 0, true);
    assert(!b.calls && !b.result.activation_count && b.old_confirm_calls == 1);
    styled_finish(&b);
    /* Query observer progress survives a legacy query hook invalidating geometry. */
    styled_init(&b, false);
    b.base.entries[0].item.measure = styled_measure;
    b.base.entries[0].item.content_user = &b;
    styled_frame(&b, 0, true);
    b.base.options.on_query = styled_old_measure_failure;
    cui_command_focus(b.ui, 90, b.base.options);
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_TEXT, .text = "T"}));
    styled_frame(&b, 0, false);
    assert(b.result.query_count == 1 && b.events[0].source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    styled_finish(&b);
    /* Programmatic leading rebuild progress survives later invalid geometry. */
    styled_init(&b, false);
    styled_frame(&b, 0, true);
    b.base.entries[0].item.measure = styled_measure;
    b.base.entries[0].item.content_user = &b;
    b.fail_measure = true;
    assert(cui_edit_insert(b.base.edit, cui_string("T")) == CUI_EDIT_OK);
    styled_frame(&b, 0, false);
    assert(b.result.query_count == 1);
    styled_finish(&b);
}
int main(void) {
    styled_cases();
    predicates();
    legacy_pairs();
    ordering_and_realloc();
    failures_and_copy();
    return 0;
}
