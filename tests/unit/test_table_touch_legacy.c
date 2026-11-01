/* SPDX-License-Identifier: MIT */
#include "cui/cui_table_events.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
cui_table_event_result dialog_frozen_cui_table_reduce_events(cui_context *, cui_id,
                                                             cui_table_model *, cui_table_state *,
                                                             cui_table_options,
                                                             cui_table_event_handler);
void dialog_frozen_cui_table_submit_root(cui_context *, cui_id, cui_table_model *,
                                         cui_table_state *, cui_table_options);
void dialog_frozen_cui_table_submit_header(cui_context *, cui_id, cui_table_model *,
                                           cui_table_state *, cui_table_options);
typedef struct {
    cui_context *ui;
    cui_table_model *model;
    cui_table_state table;
    cui_table_column columns[2];
    cui_table_event_result result;
    unsigned count;
    struct {
        unsigned kind;
        cui_id key;
        double before, after;
        cui_component_event_metadata event;
    } trace[128];
} retained_driver;
static void width(void *user, const cui_table_column_width_change *e) {
    retained_driver *d = user;
    assert(d->count < 128);
    size_t count;
    cui_semantics(d->ui, &count);
    assert(count == 0); /* synchronous reduction, before current submission */
    bool found = false;
    for (size_t i = 0; i < 2; ++i)
        if (d->columns[i].key == e->column) {
            assert(d->columns[i].width == e->width);
            found = true;
        }
    assert(found);
    d->trace[d->count].kind = 1;
    d->trace[d->count].key = e->column;
    d->trace[d->count].before = e->previous_width;
    d->trace[d->count].after = e->width;
    d->trace[d->count++].event = e->event;
}
static void move(void *user, const cui_table_column_move_change *e) {
    retained_driver *d = user;
    assert(d->count < 128 && e->index < 2 && d->columns[e->index].key == e->column);
    size_t count;
    cui_semantics(d->ui, &count);
    assert(count == 0);
    d->trace[d->count].kind = 2;
    d->trace[d->count].key = e->column;
    d->trace[d->count].before = (double)e->previous_index;
    d->trace[d->count].after = (double)e->index;
    d->trace[d->count++].event = e->event;
}
static void init(retained_driver *d) {
    *d = (retained_driver){0};
    d->ui = cui_create((cui_config){.width = 300, .height = 200});
    d->model = cui_table_create(1);
    assert(d->ui && d->model);
    for (size_t i = 0; i < 2; ++i) {
        d->columns[i] = cui_table_column_default(i + 1, cui_string("Column"));
        d->columns[i].width = 100;
        d->columns[i].minimum = 20;
        d->columns[i].maximum = 300;
        d->columns[i].resizable = d->columns[i].movable = true;
    }
    assert(cui_table_bind(d->model, NULL, 0, d->columns, 2) == CUI_TABLE_OK);
}
static void frame(retained_driver *d, bool frozen, bool consumed, unsigned blocked) {
    assert(cui_begin_frame(d->ui, 300, 200, .016f));
    cui_begin_box(d->ui, 1, (cui_box){.width = 300, .height = 200});
    assert(cui_begin_focus_scope(d->ui, 100, false));
    size_t n;
    cui_frame_events(d->ui, &n);
    if (consumed)
        for (size_t i = 0; i < n; ++i)
            cui_consume_event(d->ui, i);
    cui_table_options o = cui_table_options_default();
    o.width = 208;
    o.height = 60;
    o.header_height = 40;
    o.disabled = blocked == 1;
    o.loading = blocked == 2;
    cui_table_event_handler h = {.on_column_width = width, .on_column_move = move, .user = d};
    d->result = frozen
                    ? dialog_frozen_cui_table_reduce_events(d->ui, 220, d->model, &d->table, o, h)
                    : cui_table_reduce_events(d->ui, 220, d->model, &d->table, o, h);
    assert(d->result.valid && d->result.control.error == CUI_TABLE_OK);
    cui_begin_box(d->ui, 220, (cui_box){.width = 208, .height = 60, .row = true});
    for (size_t i = 0; i < 2; ++i) {
        cui_begin_box(d->ui, cui_table_header_id(220, d->columns[i].key),
                      (cui_box){.width = 100, .height = 40});
        cui_end_box(d->ui);
        if (!i) {
            cui_begin_box(d->ui, cui_table_resize_id(220, d->columns[i].key),
                          (cui_box){.width = 8, .height = 40});
            cui_end_box(d->ui);
        }
    }
    if (frozen) {
        dialog_frozen_cui_table_submit_root(d->ui, 220, d->model, &d->table, o);
        dialog_frozen_cui_table_submit_header(d->ui, 220, d->model, &d->table, o);
    } else {
        cui_table_submit_root(d->ui, 220, d->model, &d->table, o);
        cui_table_submit_header(d->ui, 220, d->model, &d->table, o);
    }
    cui_end_box(d->ui);
    cui_end_focus_scope(d->ui);
    cui_end_box(d->ui);
    (void)cui_end_frame(d->ui);
    assert(cui_last_error(d->ui) == CUI_OK);
}
static void compare(const retained_driver *a, const retained_driver *b) {
#define SAME(m) assert(a->m == b->m)
    SAME(count);
    SAME(table.row);
    SAME(table.column);
    SAME(table.cursor_row);
    SAME(table.cursor_column);
    SAME(table.header_cursor);
    SAME(table.sort_column);
    SAME(table.sort);
    SAME(table.scroll_x);
    SAME(table.scroll_y);
    SAME(table.dragging);
    SAME(table.drag_x);
    SAME(table.drag_active);
    SAME(table.resizing);
    SAME(table.resize_x);
    SAME(table.resize_width);
    SAME(table.scrollbar.x);
    SAME(table.scrollbar.y);
    SAME(table.scrollbar.grab_x);
    SAME(table.scrollbar.grab_y);
    SAME(table.scrollbar.dragging_x);
    SAME(table.scrollbar.dragging_y);
    SAME(table.visible_begin);
    SAME(table.visible_end);
    SAME(table.column_begin);
    SAME(table.column_end);
    SAME(table.previous_row_count);
    SAME(table.viewport_initialized);
    SAME(table.load_pending);
    assert(!a->table.selection && !b->table.selection);
    SAME(table.selection_count);
    SAME(table.selection_capacity);
    SAME(table.anchor_row);
    SAME(table.anchor_column);
    SAME(result.valid);
    SAME(result.submitted);
    SAME(result.count_overflow);
    SAME(result.column_width_count);
    SAME(result.column_move_count);
    SAME(result.selection_count);
    SAME(result.sort_count);
    SAME(result.activation_count);
    SAME(result.context_request_count);
    SAME(result.mode_request_count);
    SAME(result.control.error);
    SAME(result.control.columns_changed);
    SAME(result.control.selection_changed);
    SAME(result.control.sort_changed);
    SAME(result.control.visible_rows_changed);
    SAME(result.control.visible_columns_changed);
    SAME(result.control.load_more);
    SAME(result.control.context_requested);
    SAME(result.control.context_row);
    SAME(result.control.context_column);
    SAME(result.control.selection_mode_requested);
    SAME(result.control.requested_selection_mode);
    SAME(result.control.activated_row);
    SAME(result.control.activated_column);
    SAME(result.control.rows.begin);
    SAME(result.control.rows.end);
    SAME(result.control.rows.before);
    SAME(result.control.rows.after);
    SAME(result.control.rows.total);
    SAME(result.control.rows.scroll);
    SAME(result.control.columns.begin);
    SAME(result.control.columns.end);
    SAME(result.control.columns.before);
    SAME(result.control.columns.after);
    SAME(result.control.columns.total);
    SAME(result.control.columns.scroll);

    for (size_t i = 0; i < 2; ++i) {
        SAME(columns[i].key);
        SAME(columns[i].width);
        SAME(columns[i].minimum);
        SAME(columns[i].maximum);
        SAME(columns[i].resizable);
        SAME(columns[i].movable);
        SAME(columns[i].disabled);
        SAME(columns[i].fixed_left);
        SAME(columns[i].sortable);
        SAME(columns[i].nonselectable);
        SAME(columns[i].align);
        SAME(columns[i].label.length);
        assert(memcmp(a->columns[i].label.data, b->columns[i].label.data,
                      a->columns[i].label.length) == 0);
    }
    for (size_t i = 0; i < a->count; ++i) {
        SAME(trace[i].kind);
        SAME(trace[i].key);
        SAME(trace[i].before);
        SAME(trace[i].after);
        SAME(trace[i].event.kind);
        SAME(trace[i].event.target);
        SAME(trace[i].event.flags);
        SAME(trace[i].event.frame_sequence);
        SAME(trace[i].event.event_index);
        SAME(trace[i].event.source);
        assert(!(a->trace[i].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    }
#undef SAME
}
static void queue(retained_driver *d, cui_id id, cui_event_type type, cui_key key, float dx) {
    cui_rect b = cui_bounds(d->ui, id);
    assert(b.w > 0 && b.h > 0);
    if (type == CUI_EVENT_KEY_DOWN)
        cui_focus(d->ui, id);
    assert(
        cui_queue_event(d->ui, (cui_event){.type = type, .key = key, .x = b.x + dx, .y = b.y + 2}));
}
int main(void) {
    retained_driver a, b;
    init(&a);
    init(&b);
    frame(&a, false, false, 0);
    frame(&b, true, false, 0);
    compare(&a, &b);
    const cui_event_type types[] = {CUI_EVENT_POINTER_DOWN, CUI_EVENT_POINTER_MOVE,
                                    CUI_EVENT_POINTER_UP,   CUI_EVENT_KEY_DOWN,
                                    CUI_EVENT_KEY_DOWN,     CUI_EVENT_CANCEL};
    const cui_key keys[] = {0, 0, 0, CUI_KEY_RIGHT, CUI_KEY_HOME, 0};
    for (unsigned target = 0; target < 2; ++target)
        for (unsigned consumed = 0; consumed < 2; ++consumed)
            for (size_t i = 0; i < sizeof types / sizeof types[0]; ++i) {
                /* Only the first current column authors a resize leaf. */
                assert(a.columns[0].key == b.columns[0].key);
                cui_id id = target == 0 ? cui_table_resize_id(220, a.columns[0].key)
                                        : cui_table_header_id(220, 1);
                queue(&a, id, types[i], keys[i], i == 0 ? 2 : target == 1 ? 180 : 40);
                queue(&b, id, types[i], keys[i], i == 0 ? 2 : target == 1 ? 180 : 40);
                frame(&a, false, consumed, 0);
                frame(&b, true, consumed, 0);
                compare(&a, &b);
            }
    for (unsigned blocked = 1; blocked <= 2; ++blocked) {
        /* Publish enabled input semantics after the preceding blocked frame. */
        frame(&a, false, false, 0);
        frame(&b, true, false, 0);
        compare(&a, &b);
        assert(a.columns[0].key == b.columns[0].key);
        queue(&a, cui_table_resize_id(220, a.columns[0].key), CUI_EVENT_POINTER_DOWN, 0, 2);
        queue(&b, cui_table_resize_id(220, a.columns[0].key), CUI_EVENT_POINTER_DOWN, 0, 2);
        frame(&a, false, false, 0);
        frame(&b, true, false, 0);
        compare(&a, &b);
        assert(a.table.resizing);
        frame(&a, false, false, blocked);
        frame(&b, true, false, blocked);
        compare(&a, &b);
    }
    assert(a.count > 0);
    cui_table_destroy(a.model);
    cui_table_destroy(b.model);
    cui_destroy(a.ui);
    cui_destroy(b.ui);
    return 0;
}
