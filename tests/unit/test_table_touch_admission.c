/* SPDX-License-Identifier: MIT */
#ifndef CUI_TABLE_TOUCH_DISTRIBUTION
#include "cui/cui_table_events.h"
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_table_model *model;
    cui_table_state state;
    cui_table_column columns[2];
    cui_table_options options;
    unsigned moves, sorts, cancels, raw_moves, raw_ups, scrolls;
    float outer_scroll;
} fixture;
static void moved(void *user, const cui_table_column_move_change *e) {
    fixture *f = user;
    assert(e->index < 2 && f->columns[e->index].key == e->column);
    ++f->moves;
}
static void sorted(void *user, const cui_table_sort_change *e) {
    fixture *f = user;
    assert(f->state.sort_column == e->column && f->state.sort == e->value);
    ++f->sorts;
}
static void frame(fixture *f) {
    assert(cui_begin_frame(f->ui, 300, 200, .016f));
    cui_begin_box(f->ui, 501, (cui_box){.width = 300, .height = 200, .clip = true});
    assert(cui_begin_focus_scope(f->ui, 100, false));
    size_t count;
    const cui_event *events = cui_frame_events(f->ui, &count);
    for (size_t i = 0; i < count; ++i) {
        f->cancels += events[i].type == CUI_EVENT_CANCEL;
        f->raw_moves += events[i].type == CUI_EVENT_POINTER_MOVE;
        f->raw_ups += events[i].type == CUI_EVENT_POINTER_UP;
        if (events[i].type == CUI_EVENT_SCROLL) {
            ++f->scrolls;
            if (cui_event_target(f->ui, i) == 501 && !cui_event_consumed(f->ui, i)) {
                f->outer_scroll = fmaxf(0, f->outer_scroll - events[i].dy);
                assert(cui_consume_event(f->ui, i));
            }
        }
    }
    cui_table_event_result r = cui_table_reduce_events(
        f->ui, 220, f->model, &f->state, f->options,
        (cui_table_event_handler){.on_column_move = moved, .on_sort = sorted, .user = f});
    assert(r.valid && r.control.error == CUI_TABLE_OK);
    cui_begin_box(f->ui, 220, (cui_box){.width = 208, .height = 100});
    cui_begin_box(f->ui, 221, (cui_box){.width = 208, .height = 40, .row = true});
    for (size_t i = 0; i < 2; ++i) {
        cui_begin_box(f->ui, cui_table_header_id(220, f->columns[i].key),
                      (cui_box){.width = 100, .height = 40});
        cui_end_box(f->ui);
    }
    cui_end_box(f->ui);
    cui_begin_box(f->ui, 222, (cui_box){.width = 208, .height = 60});
    (void)cui_interact(f->ui, 222, CUI_ROLE_GROUP, cui_string("Body"), false, false, 0);
    cui_set_semantic_details(f->ui, 222, (cui_semantic_details){.parent = 220});
    cui_end_box(f->ui);
    cui_table_submit_root(f->ui, 220, f->model, &f->state, f->options);
    cui_table_submit_header(f->ui, 220, f->model, &f->state, f->options);
    cui_end_box(f->ui);
    (void)cui_interact(f->ui, 501, CUI_ROLE_GROUP, cui_string("Outer scroll"), false, false, 0);
    cui_set_semantic_details(f->ui, 501,
                             (cui_semantic_details){.scrollable = true,
                                                    .scroll_max_y = 200,
                                                    .scroll_y = f->outer_scroll});
    cui_end_focus_scope(f->ui);
    cui_end_box(f->ui);
    (void)cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
}
static void init(fixture *f, bool movable, unsigned blocked, bool pinned) {
    *f = (fixture){0};
    f->ui = cui_create((cui_config){.width = 300, .height = 200});
    f->model = cui_table_create(1);
    assert(f->ui && f->model);
    for (size_t i = 0; i < 2; ++i) {
        f->columns[i] = cui_table_column_default(i + 1, cui_string("Column"));
        f->columns[i].width = 100;
        f->columns[i].movable = movable;
        f->columns[i].sortable = true;
        f->columns[i].disabled = blocked == 3;
    }
    f->columns[0].fixed_left = pinned;
    assert(cui_table_bind(f->model, NULL, 0, f->columns, 2) == CUI_TABLE_OK);
    f->options = cui_table_options_default();
    f->options.width = 208;
    f->options.height = 100;
    f->options.header_height = 40;
    f->options.disabled = blocked == 1;
    f->options.loading = blocked == 2;
    frame(f);
    frame(f);
}
static void done(fixture *f) {
    cui_table_destroy(f->model);
    cui_destroy(f->ui);
}
static void event(fixture *f, cui_event_type type, float x, float y, uint64_t contact) {
    assert(cui_queue_event(f->ui, (cui_event){.type = type,
                                              .pointer_type = CUI_POINTER_TOUCH,
                                              .pointer_id = contact,
                                              .pointer_device = 17,
                                              .x = x,
                                              .y = y}));
}
static void gesture(fixture *f, cui_id key, float x, float y, bool later_mouse) {
    cui_rect b = cui_bounds(f->ui, cui_table_header_id(220, key));
    event(f, CUI_EVENT_POINTER_DOWN, b.x + 2, b.y + 2, 7);
    event(f, CUI_EVENT_POINTER_MOVE, x, y, 7);
    event(f, CUI_EVENT_POINTER_UP, x, y, 7);
    if (later_mouse)
        assert(cui_queue_event(f->ui,
                               (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 290, .y = 190}));
    frame(f);
}
static void reorder(bool later_mouse) {
    fixture f;
    init(&f, true, 0, false);
    cui_rect second = cui_bounds(f.ui, cui_table_header_id(220, 2));
    gesture(&f, 1, second.x + 80, second.y + 2, later_mouse);
    fprintf(stderr, "touch reorder later_mouse=%d moves=%u cancel=%u move/up=%u/%u first=%llu\n",
            later_mouse, f.moves, f.cancels, f.raw_moves, f.raw_ups,
            (unsigned long long)f.columns[0].key);
    assert(f.moves == 1 && f.columns[0].key == 2 && !f.cancels && f.raw_ups == 1);
    cui_rect first = cui_bounds(f.ui, cui_table_header_id(220, 2));
    gesture(&f, 1, first.x + 2, first.y + 2, later_mouse);
    assert(f.moves == 2 && f.columns[0].key == 1 && !f.state.dragging);
    done(&f);
}
static void tap(void) {
    fixture f;
    init(&f, true, 0, false);
    cui_rect b = cui_bounds(f.ui, cui_table_header_id(220, 1));
    gesture(&f, 1, b.x + 3, b.y + 2, false);
    assert(!f.moves && !f.state.dragging && f.sorts == 1 && f.state.sort_column == 1);
    done(&f);
}
static void blocked(void) {
    for (unsigned blocked = 1; blocked <= 3; ++blocked) {
        fixture f;
        init(&f, true, blocked, false);
        cui_rect b = cui_bounds(f.ui, cui_table_header_id(220, 2));
        gesture(&f, 1, b.x + 80, b.y + 2, false);
        assert(!f.moves && !f.sorts && !f.state.dragging && f.columns[0].key == 1);
        done(&f);
    }
}
static void pinned(void) {
    fixture f;
    init(&f, true, 0, true);
    cui_rect b = cui_bounds(f.ui, cui_table_header_id(220, 2));
    gesture(&f, 1, b.x + 80, b.y + 2, false);
    assert(!f.moves && f.columns[0].key == 1);
    done(&f);
}
static void pan(bool body) {
    fixture f;
    init(&f, false, 0, false);
    cui_rect b = cui_bounds(f.ui, body ? 222 : cui_table_header_id(220, 1));
    event(&f, CUI_EVENT_POINTER_DOWN, b.x + 10, b.y + 25, 7);
    event(&f, CUI_EVENT_POINTER_MOVE, b.x + 10, b.y + 5, 7);
    event(&f, CUI_EVENT_POINTER_UP, b.x + 10, b.y + 5, 7);
    frame(&f);
    fprintf(stderr, "pan body=%d scrolls=%u offset=%g cancel=%u\n", body, f.scrolls,
            (double)f.outer_scroll, f.cancels);
    assert(f.scrolls > 0 && f.outer_scroll > 0 && !f.moves && !f.state.dragging);
    done(&f);
}
static void multiple(void) {
    fixture f;
    init(&f, true, 0, false);
    cui_rect b = cui_bounds(f.ui, cui_table_header_id(220, 1));
    event(&f, CUI_EVENT_POINTER_DOWN, b.x + 2, b.y + 2, 7);
    frame(&f);
    assert(f.state.dragging == 1 && cui_pressed(f.ui, cui_table_header_id(220, 1)));
    event(&f, CUI_EVENT_POINTER_DOWN, b.x + 20, b.y + 2, 8);
    event(&f, CUI_EVENT_POINTER_MOVE, b.x + 80, b.y + 2, 7);
    event(&f, CUI_EVENT_POINTER_UP, b.x + 80, b.y + 2, 7);
    event(&f, CUI_EVENT_POINTER_UP, b.x + 20, b.y + 2, 8);
    frame(&f);
    assert(!f.moves && !f.state.dragging && !f.state.drag_active && f.cancels > 0);
    b = cui_bounds(f.ui, cui_table_header_id(220, 2));
    gesture(&f, 1, b.x + 80, b.y + 2, false);
    assert(f.moves == 1 && f.columns[0].key == 2);
    done(&f);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    if (!strcmp(argv[1], "--reorder"))
        reorder(false);
    else if (!strcmp(argv[1], "--later-mouse"))
        reorder(true);
    else if (!strcmp(argv[1], "--tap"))
        tap();
    else if (!strcmp(argv[1], "--blocked"))
        blocked();
    else if (!strcmp(argv[1], "--pinned"))
        pinned();
    else if (!strcmp(argv[1], "--body-pan"))
        pan(true);
    else if (!strcmp(argv[1], "--header-pan"))
        pan(false);
    else if (!strcmp(argv[1], "--multiple"))
        multiple();
    else
        assert(false);
    return 0;
}
