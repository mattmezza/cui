/* SPDX-License-Identifier: MIT */
#include "cui/cui_table.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
static void models(void) {
    assert(!cui_table_create(SIZE_MAX));
    cui_table_model *m = cui_table_create(10000);
    assert(m);
    cui_table_row *rows = calloc(10000, sizeof(*rows));
    assert(rows);
    for (size_t i = 0; i < 10000; i++)
        rows[i] = (cui_table_row){.key = i + 1, .label = {"Value", 5}};
    cui_table_column cols[] = {
        {.key = 10,
         .label = {"Name", 4},
         .width = 120,
         .minimum = 40,
         .maximum = 200,
         .resizable = true,
         .movable = true},
        {.key = 20, .label = {"Value", 5}, .width = 100, .movable = true, .sortable = true}};
    assert(cui_table_bind(m, rows, 10000, cols, 2) == CUI_TABLE_OK);
    size_t ix;
    assert(cui_table_row_index(m, 9999, &ix) && ix == 9998);
    cui_table_view v = cui_table_viewport(m, 160005, 240, 32, 2);
    assert(v.begin == 4998 && v.end == 5010 && v.before == 159936 && v.scroll == 160005);
    v = cui_table_viewport(m, NAN, INFINITY, NAN, SIZE_MAX);
    assert(v.begin == 0 && v.end == 10000 && v.scroll == 0);
    assert(cui_table_resize(m, 10, 999) && cols[0].width == 200);
    assert(cui_table_resize(m, 10, -99) && cols[0].width == 40);
    assert(!cui_table_resize(m, 10, NAN));
    assert(cui_table_move_column(m, 10, 1) && cols[1].key == 10);
    cols[0].movable = false;
    assert(!cui_table_move_column(m, 10, 0));
    cui_table_row duplicate[] = {{.key = 1}, {.key = 1}};
    assert(cui_table_bind(m, duplicate, 2, cols, 2) == CUI_TABLE_DUPLICATE_KEY);
    assert(cui_table_rows(m) == 10000 && cui_table_row_index(m, 9999, &ix));
    cols[0].width = NAN;
    assert(cui_table_bind(m, rows, 2, cols, 2) == CUI_TABLE_INVALID);
    cols[0].width = 100;
    cui_table_destroy(m);
    free(rows);
}
static void columns_and_loading(void) {
    cui_table_model *m = cui_table_create(20);
    cui_table_row rows[20];
    cui_table_column cols[100];
    for (size_t r = 0; r < 20; r++)
        rows[r] = (cui_table_row){.key = r + 1};
    for (size_t c = 0; c < 100; c++)
        cols[c] = (cui_table_column){.key = c + 1, .width = 100};
    assert(cui_table_bind(m, rows, 10, cols, 100) == CUI_TABLE_OK);
    cui_table_view v = cui_table_column_viewport(m, 4505, 200, 1);
    assert(v.begin == 44 && v.end == 49 && v.before == 4400 && v.after == 5100 &&
           v.total == 10000 && v.scroll == 4505);
    v = cui_table_column_viewport(m, INFINITY, NAN, SIZE_MAX);
    assert(v.begin == 0 && v.end == 100);
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    cui_table_state state = {0};
    cui_table_options o = {
        .width = 200, .height = 64, .row_height = 32, .load_more = true, .load_more_threshold = 1};
    assert(cui_begin_frame(ui, 700, 500, .016f));
    cui_table_result result = cui_table_reduce(ui, 10, m, &state, o);
    assert(result.visible_rows_changed && result.visible_columns_changed && !result.load_more &&
           result.rows.end == 2);
    cui_end_frame(ui);
    state.scroll_y = 256;
    assert(cui_begin_frame(ui, 700, 500, .016f));
    result = cui_table_reduce(ui, 10, m, &state, o);
    assert(result.load_more && result.visible_rows_changed && result.rows.begin == 8);
    cui_end_frame(ui);
    assert(cui_begin_frame(ui, 700, 500, .016f));
    result = cui_table_reduce(ui, 10, m, &state, o);
    assert(!result.load_more && !result.visible_rows_changed);
    cui_end_frame(ui);
    assert(cui_table_bind(m, rows, 12, cols, 100) == CUI_TABLE_OK);
    state.scroll_y = 320;
    assert(cui_begin_frame(ui, 700, 500, .016f));
    result = cui_table_reduce(ui, 10, m, &state, o);
    assert(result.load_more);
    cui_end_frame(ui);
    o.selection_mode = (cui_table_selection_mode)99;
    assert(cui_begin_frame(ui, 700, 500, .016f));
    assert(cui_table_reduce(ui, 10, m, &state, o).error == CUI_TABLE_INVALID);
    cui_end_frame(ui);
    o.selection_mode = CUI_TABLE_SELECT_ROW;
    state.sort = (cui_table_sort)99;
    assert(cui_begin_frame(ui, 700, 500, .016f));
    assert(cui_table_reduce(ui, 10, m, &state, o).error == CUI_TABLE_INVALID);
    cui_end_frame(ui);
    cui_destroy(ui);
    cui_table_destroy(m);
}
static void spans(void) {
    float widths[] = {80, 120, 40}, out = 17;
    assert(cui_table_span_width(widths, 3, 0, 2, &out) && out == 200);
    out = 17;
    assert(!cui_table_span_width(widths, 3, SIZE_MAX, 2, &out) && out == 17);
    assert(!cui_table_span_width(widths, 3, 2, SIZE_MAX, &out) && out == 17);
    widths[1] = NAN;
    assert(!cui_table_span_width(widths, 3, 0, 2, &out) && out == 17);
    widths[1] = INFINITY;
    assert(!cui_table_span_width(widths, 3, 0, 2, &out));
}
static size_t rendered;
static cui_str label(void *data, size_t row, cui_id column) {
    (void)data;
    (void)row;
    (void)column;
    return cui_string("Accessible value");
}
#ifndef CUI_TABLE_BASE_ONLY
static void cell(cui_context *ui, const cui_theme *t, void *data, size_t row, cui_id column) {
    rendered++;
    cui_text(ui, label(data, row, column), t->font_size, t->text);
}
#endif
static cui_table_result scene(cui_context *ui, cui_table_model *m, cui_table_state *s,
                              cui_table_options o) {
    assert(cui_begin_frame(ui, 700, 500, .016f));
#ifdef CUI_TABLE_BASE_ONLY
    cui_table_result result = cui_base_data_table(ui, 10, m, s, o);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++) {
        cui_begin_box(ui, sem[i].id, (cui_box){.width = 100, .height = 20});
        cui_end_box(ui);
    }
    assert(cui_end_frame(ui).count == 0);
#else
    cui_theme t = cui_theme_dark();
    cui_table_result result = cui_data_table(ui, 10, &t, m, s, o, label, cell, NULL);
    assert(cui_end_frame(ui).count > 0);
#endif
    assert(result.error == CUI_TABLE_OK && cui_last_error(ui) == CUI_OK);
    return result;
}
static void key(cui_context *ui, cui_key k, unsigned mods) {
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k, .modifiers = mods}));
}
static void events(void) {
    cui_table_model *m = cui_table_create(10);
    cui_table_row rows[] = {{1, {"One", 3}, false},
                            {2, {"Two", 3}, true},
                            {3, {"Three", 5}, false},
                            {4, {"Four", 4}, false}};
    cui_table_column cols[] = {
        {.key = 10,
         .label = {"Name", 4},
         .width = 120,
         .resizable = true,
         .movable = true,
         .sortable = true},
        {.key = 20, .label = {"Disabled", 8}, .width = 100, .disabled = true, .movable = true},
        {.key = 30, .label = {"Value", 5}, .width = 100, .movable = true, .sortable = true}};
    assert(cui_table_bind(m, rows, 4, cols, 3) == CUI_TABLE_OK);
    cui_table_state state = {0};
    cui_table_options o = {.width = 360, .height = 160, .selection_mode = CUI_TABLE_SELECT_CELL};
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    scene(ui, m, &state, o);
    assert(state.cursor_row == 1 && state.cursor_column == 10);
    cui_focus(ui, cui_table_cell_id(10, 1, 10));
    key(ui, CUI_KEY_DOWN, 0);
    key(ui, CUI_KEY_DOWN, 0);
    key(ui, CUI_KEY_RIGHT, 0);
    assert(scene(ui, m, &state, o).selection_changed && state.row == 4 && state.column == 30);
    key(ui, CUI_KEY_HOME, 0);
    scene(ui, m, &state, o);
    assert(state.row == 1 && state.column == 30);
    key(ui, CUI_KEY_PAGEDOWN, 0);
    scene(ui, m, &state, o);
    assert(state.row == 4);
    key(ui, CUI_KEY_PAGEUP, 0);
    scene(ui, m, &state, o);
    assert(state.row == 1);
    o.manual_selection = true;
    key(ui, CUI_KEY_DOWN, 0);
    scene(ui, m, &state, o);
    assert(state.cursor_row == 3 && state.row == 1);
    o.manual_selection = false;
    o.disabled = true;
    key(ui, CUI_KEY_END, 0);
    scene(ui, m, &state, o);
    assert(state.cursor_row == 3);
    o.disabled = false;
    cui_focus(ui, cui_table_header_id(10, 10));
    key(ui, CUI_KEY_ENTER, 0);
    assert(scene(ui, m, &state, o).sort_changed && state.sort == CUI_TABLE_SORT_ASCENDING);
    key(ui, CUI_KEY_ENTER, 0);
    scene(ui, m, &state, o);
    assert(state.sort == CUI_TABLE_SORT_DESCENDING);
    key(ui, CUI_KEY_ENTER, 0);
    scene(ui, m, &state, o);
    assert(state.sort == CUI_TABLE_SORT_DEFAULT);
    cols[2].sortable = true;
    key(ui, CUI_KEY_RIGHT, 0);
    key(ui, CUI_KEY_ENTER, 0);
    assert(scene(ui, m, &state, o).sort_changed && state.header_cursor == 30 &&
           state.sort_column == 30);
    cui_focus(ui, cui_table_header_id(10, 10));
    key(ui, CUI_KEY_RIGHT, CUI_MOD_CTRL);
    assert(scene(ui, m, &state, o).columns_changed && cols[1].key == 10);
    cui_focus(ui, cui_table_resize_id(10, 10));
    key(ui, CUI_KEY_RIGHT, 0);
    assert(scene(ui, m, &state, o).columns_changed && cols[1].width == 128);
    cui_rect target = cui_bounds(ui, cui_table_cell_id(10, 1, 10));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = target.x + 8,
                                           .y = target.y + 8,
                                           .pointer_button = CUI_POINTER_SECONDARY}));
    cui_table_result request = scene(ui, m, &state, o);
    assert(request.context_requested && request.context_row == 1 && request.context_column == 10 &&
           !request.selection_changed);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool found = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == cui_table_cell_id(10, 3, 10)) {
            found = true;
            assert(sem[i].role == CUI_ROLE_GRIDCELL && sem[i].details.row_index == 3 &&
                   sem[i].details.column_index == 2 &&
                   sem[i].details.parent == cui_table_row_id(10, 3));
        }
    assert(found);
    cui_destroy(ui);
    cui_table_destroy(m);
}
#ifndef CUI_TABLE_BASE_ONLY
static void custom_header(cui_context *ui, const cui_theme *t, void *data, cui_id column) {
    (void)column;
    ((size_t *)data)[0]++;
    cui_text(ui, cui_string("Custom head"), t->font_size, t->text);
}
static void custom_row_header(cui_context *ui, const cui_theme *t, void *data, size_t row) {
    (void)row;
    ((size_t *)data)[1]++;
    cui_text(ui, cui_string("R"), t->font_size, t->text);
}
static void custom_placeholder(cui_context *ui, const cui_theme *t, void *data) {
    ((size_t *)data)[2]++;
    cui_text(ui, cui_string("Custom status"), t->font_size, t->muted);
}
static void custom_group(cui_context *ui, const cui_theme *t, void *data, size_t level,
                         size_t group, cui_str label, size_t span, float width) {
    size_t *counts = data;
    assert(level < 2 && group < 3 && label.length && span && width > 0);
    counts[0]++;
    cui_text(ui, label, t->font_size, t->text);
}
static void custom_whole_row(cui_context *ui, const cui_theme *t, void *data, size_t row) {
    size_t *counts = data;
    assert(row < 4);
    counts[1]++;
    cui_text(ui, cui_string("row"), t->font_size, t->muted);
}
static void custom_trailing(cui_context *ui, const cui_theme *t, void *data, bool header,
                            size_t row) {
    size_t *counts = data;
    assert(header || row < 4);
    counts[2]++;
    cui_text(ui, cui_string("+"), t->font_size, t->muted);
}
#endif
static void placeholders(void) {
    cui_table_model *m = cui_table_create(2);
    cui_table_row row = {.key = 1};
    cui_table_column cols[] = {{.key = 10, .label = {"First", 5}, .width = 100, .sortable = true},
                               {.key = 20, .label = {"Second", 6}, .width = 100}};
    assert(cui_table_bind(m, NULL, 0, cols, 2) == CUI_TABLE_OK);
    cui_table_state state = {0};
    cui_table_options o = {.width = 240, .height = 96, .row_header = true, .load_more = true};
    size_t counters[3] = {0};
#ifndef CUI_TABLE_BASE_ONLY
    o.render_header = custom_header;
    o.render_row_header = custom_row_header;
    o.render_empty = custom_placeholder;
    o.render_loading = custom_placeholder;
    o.render_userdata = counters;
#endif
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    rendered = 0;
    assert(scene(ui, m, &state, o).load_more);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool status = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].role == CUI_ROLE_STATUS)
            status = !sem[i].details.busy;
    assert(status);
#ifndef CUI_TABLE_BASE_ONLY
    assert(counters[0] == 2 && counters[2] == 1 && rendered == 0);
#endif
    assert(cui_table_bind(m, &row, 1, cols, 2) == CUI_TABLE_OK);
    o.loading = true;
    cui_focus(ui, cui_table_header_id(10, 10));
    key(ui, CUI_KEY_ENTER, 0);
    cui_table_result result = scene(ui, m, &state, o);
    assert(!result.sort_changed && !result.load_more);
    sem = cui_semantics(ui, &n);
    assert(sem[0].details.busy);
    for (size_t i = 0; i < n; i++)
        assert(sem[i].role != CUI_ROLE_GRIDCELL && sem[i].role != CUI_ROLE_ROW);
    o.loading = false;
    scene(ui, m, &state, o);
#ifndef CUI_TABLE_BASE_ONLY
    assert(counters[0] == 6 && counters[1] == 1 && counters[2] == 2 && rendered == 2);
#else
    (void)counters;
#endif
    cui_destroy(ui);
    cui_table_destroy(m);
}
static void multiple_selection(void) {
    cui_table_model *m = cui_table_create(4);
    cui_table_row rows[] = {{.key = 1}, {.key = 2, .disabled = true}, {.key = 3}, {.key = 4}};
    cui_table_column cols[] = {{.key = 10, .width = 100}, {.key = 20, .width = 100}};
    assert(cui_table_bind(m, rows, 4, cols, 2) == CUI_TABLE_OK);
    cui_table_selection buffer[8] = {0};
    cui_table_state state = {.selection = buffer, .selection_capacity = 8};
    cui_table_options o = {.width = 200,
                           .height = 160,
                           .selection_mode = CUI_TABLE_SELECT_CELL,
                           .multiple_selection = true};
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    scene(ui, m, &state, o);
    cui_focus(ui, cui_table_cell_id(10, 1, 10));
    key(ui, CUI_KEY_ENTER, 0);
    scene(ui, m, &state, o);
    assert(state.selection_count == 1 && cui_table_is_selected(&state, o, 1, 10));
    key(ui, CUI_KEY_DOWN, CUI_MOD_SHIFT);
    key(ui, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    scene(ui, m, &state, o);
    assert(state.selection_count == 4 && cui_table_is_selected(&state, o, 3, 20) &&
           !cui_table_is_selected(&state, o, 2, 10));
    key(ui, CUI_KEY_ENTER, CUI_MOD_CTRL);
    scene(ui, m, &state, o);
    assert(state.selection_count == 3 && !cui_table_is_selected(&state, o, 3, 20));
    key(ui, CUI_KEY_ENTER, CUI_MOD_CTRL);
    scene(ui, m, &state, o);
    assert(state.selection_count == 4 && cui_table_is_selected(&state, o, 3, 20));
    state.selection_capacity = 4;
    key(ui, CUI_KEY_DOWN, CUI_MOD_SHIFT);
    assert(cui_begin_frame(ui, 700, 500, .016f));
    cui_table_result result;
#ifdef CUI_TABLE_BASE_ONLY
    result = cui_base_data_table(ui, 10, m, &state, o);
#else
    cui_theme t = cui_theme_dark();
    result = cui_data_table(ui, 10, &t, m, &state, o, label, cell, NULL);
#endif
    cui_end_frame(ui);
    assert(result.error == CUI_TABLE_OK); /* Anchor changed by Ctrl toggle: two cells. */
    assert(state.selection_count == 2 && cui_table_is_selected(&state, o, 4, 20));
    state.selection_capacity = 1;
    state.selection_count = 1;
    state.anchor_row = 1;
    state.anchor_column = 10;
    cui_focus(ui, cui_table_cell_id(10, 4, 20));
    key(ui, CUI_KEY_HOME, CUI_MOD_SHIFT);
    assert(cui_begin_frame(ui, 700, 500, .016f));
#ifdef CUI_TABLE_BASE_ONLY
    result = cui_base_data_table(ui, 10, m, &state, o);
#else
    result = cui_data_table(ui, 10, &t, m, &state, o, label, cell, NULL);
#endif
    cui_end_frame(ui);
    assert(result.error == CUI_TABLE_CAPACITY && state.selection_count == 1);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n > 0 && sem[0].details.multiselectable);
    state.selection_capacity = 8;
    state.selection_count = 0;
    state.anchor_row = state.anchor_column = 0;
    o.selection_mode = CUI_TABLE_SELECT_ROW;
    scene(ui, m, &state, o);
    cui_focus(ui, cui_table_cell_id(10, 1, 10));
    key(ui, CUI_KEY_ENTER, 0);
    key(ui, CUI_KEY_END, CUI_MOD_SHIFT);
    scene(ui, m, &state, o);
    assert(state.selection_count == 3 && cui_table_is_selected(&state, o, 4, 20));
    state.anchor_row = 999;
    cui_focus(ui, cui_table_cell_id(10, 4, 20));
    key(ui, CUI_KEY_HOME, CUI_MOD_SHIFT);
    scene(ui, m, &state, o);
    assert(state.selection_count == 1 && cui_table_is_selected(&state, o, 1, 10));

    state.selection_count = 0;
    state.anchor_row = state.anchor_column = 0;
    o.selection_mode = CUI_TABLE_SELECT_COLUMN;
    scene(ui, m, &state, o);
    cui_focus(ui, cui_table_header_id(10, 10));
    key(ui, CUI_KEY_ENTER, 0);
    key(ui, CUI_KEY_RIGHT, CUI_MOD_SHIFT);
    scene(ui, m, &state, o);
    assert(state.selection_count == 2 && cui_table_is_selected(&state, o, 4, 10) &&
           cui_table_is_selected(&state, o, 1, 20));
    cui_destroy(ui);
    cui_table_destroy(m);
}
static void row_headers(void) {
    cui_table_model *m = cui_table_create(4);
    cui_table_row rows[] = {{.key = 1}, {.key = 2, .disabled = true}, {.key = 3}, {.key = 4}};
    cui_table_column cols[] = {{.key = 10, .width = 100, .disabled = true},
                               {.key = 20, .width = 100, .disabled = true}};
    assert(cui_table_bind(m, rows, 4, cols, 2) == CUI_TABLE_OK);
    cui_table_state state = {0};
    cui_table_options o = {.width = 240,
                           .height = 160,
                           .row_height = 32,
                           .selection_mode = CUI_TABLE_SELECT_CELL,
                           .row_header = true};
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    scene(ui, m, &state, o);
    cui_focus(ui, cui_table_row_header_id(10, 1));
    key(ui, CUI_KEY_ENTER, 0);
    cui_table_result result = scene(ui, m, &state, o);
    assert(result.selection_mode_requested &&
           result.requested_selection_mode == CUI_TABLE_SELECT_ROW && state.row == 1 &&
           state.column == 0);
    o.selection_mode = result.requested_selection_mode;
    key(ui, CUI_KEY_DOWN, 0);
    key(ui, CUI_KEY_DOWN, 0);
    result = scene(ui, m, &state, o);
    assert(result.selection_changed && state.row == 4 &&
           cui_focused(ui) == cui_table_row_header_id(10, 4));
#ifndef CUI_TABLE_BASE_ONLY
    assert(cui_bounds(ui, cui_table_row_header_id(10, 1)).x == 0 &&
           cui_bounds(ui, cui_table_cell_id(10, 1, 10)).x == 40);
#endif
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    bool disabled = false;
    for (size_t i = 0; i < count; i++)
        if (sem[i].id == cui_table_row_header_id(10, 2))
            disabled = sem[i].disabled && sem[i].role == CUI_ROLE_ROWHEADER &&
                       sem[i].details.parent == cui_table_row_id(10, 2);
    assert(disabled);
    cui_destroy(ui);
    cui_table_destroy(m);
}
static void scrollbars(void) {
    cui_table_model *m = cui_table_create(100);
    cui_table_row rows[100];
    for (size_t r = 0; r < 100; r++)
        rows[r] = (cui_table_row){.key = r + 1};
    cui_table_column cols[] = {
        {.key = 10, .width = 100}, {.key = 20, .width = 100}, {.key = 30, .width = 100}};
    assert(cui_table_bind(m, rows, 100, cols, 3) == CUI_TABLE_OK);
    cui_table_state state = {0};
    cui_table_options o = {.width = 200, .height = 100, .row_height = 32, .scrollbars = true};
    cui_scroll_geometry geometry = cui_table_scroll_layout(m, o);
    assert(geometry.viewport.width == 190 && geometry.viewport.height == 90 &&
           geometry.maximum_x == 110 && geometry.maximum_y == 3110);
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    scene(ui, m, &state, o);
    cui_focus(ui, cui_scrollbar_id(10, true));
    key(ui, CUI_KEY_END, 0);
    scene(ui, m, &state, o);
    assert(state.scroll_y == 3110);
    key(ui, CUI_KEY_HOME, 0);
    scene(ui, m, &state, o);
    assert(state.scroll_y == 0);
    cui_focus(ui, cui_scrollbar_id(10, false));
    key(ui, CUI_KEY_END, 0);
    scene(ui, m, &state, o);
    assert(state.scroll_x == 110);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool found = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == cui_scrollbar_id(10, true))
            found = sem[i].role == CUI_ROLE_SCROLLBAR && sem[i].details.maximum == 3110 &&
                    sem[i].details.controls == 10;
    assert(found);
#ifndef CUI_TABLE_BASE_ONLY
    cui_rect track = cui_bounds(ui, cui_scrollbar_id(10, true));
    assert(track.x == 190 && track.y == 32 && track.w == 10 && track.h == 90);
    float x = track.x + 5, y = track.y + 3;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x, .y = y + 39}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = y + 39}));
    scene(ui, m, &state, o);
    assert(fabsf(state.scroll_y - 1555) < .01f && !state.scrollbar.dragging_y);
#endif
    cui_destroy(ui);
    cui_table_destroy(m);
}
#ifndef CUI_TABLE_BASE_ONLY
static void composed_data_table(void) {
    cui_table_model *m = cui_table_create(4);
    cui_table_row rows[] = {{.key = 1}, {.key = 2}, {.key = 3}, {.key = 4}};
    cui_table_column cols[] = {{.key = 10, .label = {"A", 1}, .width = 80},
                               {.key = 20, .label = {"B", 1}, .width = 90},
                               {.key = 30, .label = {"C", 1}, .width = 100}};
    assert(cui_table_bind(m, rows, 4, cols, 3) == CUI_TABLE_OK);
    const cui_table_column_group level0[] = {{{"Identity", 8}, 1}, {{"Metrics", 7}, 2}};
    const cui_table_column_group level1[] = {
        {{"First", 5}, 1}, {{"Second", 6}, 1}, {{"Third", 5}, 1}};
    const cui_table_group_row groups[] = {{level0, 2}, {level1, 3}};
    size_t counts[3] = {0};
    cui_table_options o = {.width = 220,
                           .height = 64,
                           .row_height = 32,
                           .header_height = 24,
                           .group_headers = groups,
                           .group_header_count = 2,
                           .render_group_header = custom_group,
                           .render_row = custom_whole_row,
                           .render_trailing = custom_trailing,
                           .trailing_width = 18,
                           .render_userdata = counts};
    assert(cui_table_group_headers_valid(m, o));
    cui_table_state state = {.scroll_x = 68};
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    scene(ui, m, &state, o);
    assert(counts[0] == 5 && counts[1] == 2 && counts[2] == 5);
    assert(cui_bounds(ui, cui_table_group_header_id(10, 0, 1)).y == 0);
    assert(cui_bounds(ui, cui_table_header_id(10, 10)).y == 48);
    assert(cui_bounds(ui, cui_table_trailing_id(10, true, 0)).w == 18);
    assert(cui_bounds(ui, cui_table_trailing_id(10, false, 1)).w == 18);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool group_seen = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == cui_table_group_header_id(10, 0, 1))
            group_seen = sem[i].role == CUI_ROLE_COLUMNHEADER && sem[i].details.readonly &&
                         sem[i].details.column_index == 2;
    assert(group_seen);
    cui_table_column_group crossing[] = {{{"All", 3}, 3}};
    cui_table_group_row invalid[] = {{crossing, 1}};
    cols[0].fixed_left = true;
    assert(cui_table_bind(m, rows, 4, cols, 3) == CUI_TABLE_OK);
    o.group_headers = invalid;
    o.group_header_count = 1;
    assert(!cui_table_group_headers_valid(m, o));
    cui_destroy(ui);
    cui_table_destroy(m);
}
static void static_tables(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t i = 0; i < 4; i++) {
        cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
        assert(ui);
        assert(cui_begin_frame(ui, 700, 500, .016f));
        float widths[] = {80, 120};
        cui_table_begin(ui, 1, &themes[i], 200);
        cui_table_begin_section(ui, 2, &themes[i], CUI_TABLE_SECTION_HEADER);
        cui_table_begin_row(ui, 3, &themes[i], 32, true);
        cui_table_begin_head_cell(ui, 4, 80, 32, CUI_TABLE_ALIGN_LEFT, cui_string("First"));
        cui_text(ui, cui_string("First"), themes[i].font_size, themes[i].text);
        cui_table_end(ui);
        cui_table_begin_head_cell(ui, 5, 120, 32, CUI_TABLE_ALIGN_RIGHT, cui_string("Second"));
        cui_text(ui, cui_string("Second"), themes[i].font_size, themes[i].text);
        cui_table_end(ui);
        cui_table_end(ui);
        cui_table_end(ui);
        cui_table_begin_section(ui, 6, &themes[i], CUI_TABLE_SECTION_BODY);
        cui_table_begin_row(ui, 7, &themes[i], 32, false);
        assert(cui_table_begin_cell_span(ui, 8, widths, 2, 0, 2, 32, CUI_TABLE_ALIGN_CENTER));
        cui_text(ui, cui_string("Spanning"), themes[i].font_size, themes[i].text);
        cui_table_end(ui);
        assert(!cui_table_begin_cell_span(ui, 9, widths, 2, 1, 2, 32, CUI_TABLE_ALIGN_LEFT));
        cui_table_end(ui);
        cui_table_end(ui);
        cui_table_begin_section(ui, 10, &themes[i], CUI_TABLE_SECTION_FOOTER);
        cui_table_begin_row(ui, 11, &themes[i], 32, false);
        cui_table_begin_cell(ui, 12, 200, 32, CUI_TABLE_ALIGN_RIGHT);
        cui_text(ui, cui_string("Total"), themes[i].font_size, themes[i].text);
        cui_table_end(ui);
        cui_table_end(ui);
        cui_table_end(ui);
        cui_table_caption(ui, 13, &themes[i], cui_string("Caption"));
        cui_table_end(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(list.count && cui_last_error(ui) == CUI_OK);
        assert(cui_bounds(ui, 8).w == 200 && cui_bounds(ui, 8).y == 32 &&
               cui_bounds(ui, 13).y == 96);
        size_t n;
        const cui_semantic *sem = cui_semantics(ui, &n);
        for (size_t j = 0; j < n; j++)
            if (sem[j].id == 2)
                assert(sem[j].role == CUI_ROLE_ROWGROUP && sem[j].tab_index == -1);
            else if (sem[j].id == 4)
                assert(sem[j].role == CUI_ROLE_COLUMNHEADER && sem[j].details.readonly &&
                       sem[j].tab_index == -1);
        bool header = false;
        for (size_t j = 0; j < list.count; j++)
            if (list.commands[j].type == CUI_DRAW_RECT && list.commands[j].bounds.y == 0 &&
                list.commands[j].color.r == themes[i].background.r &&
                list.commands[j].color.g == themes[i].background.g)
                header = true;
        assert(header);
        cui_destroy(ui);
    }
}
static void pointer_resize(void) {
    cui_table_model *m = cui_table_create(1);
    cui_table_row row = {1, {"One", 3}, false};
    cui_table_column col = {.key = 10,
                            .label = {"Name", 4},
                            .width = 120,
                            .minimum = 80,
                            .maximum = 150,
                            .resizable = true,
                            .sortable = true};
    assert(cui_table_bind(m, &row, 1, &col, 1) == CUI_TABLE_OK);
    cui_table_state state = {0};
    cui_table_options o = {.width = 200, .height = 100};
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    scene(ui, m, &state, o);
    cui_rect grip = cui_bounds(ui, cui_table_resize_id(10, 10));
    float x = grip.x + 4, y = grip.y + 4;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = y}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x + 80, .y = y}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x + 80, .y = y}));
    assert(scene(ui, m, &state, o).columns_changed && col.width == 150 && !state.resizing);
    scene(ui, m, &state, o);
    grip = cui_bounds(ui, cui_table_resize_id(10, 10));
    x = grip.x + 4;
    y = grip.y + 4;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = x,
                                           .y = y,
                                           .pointer_button = CUI_POINTER_SECONDARY}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x - 80, .y = y}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = x - 80,
                                           .y = y,
                                           .pointer_button = CUI_POINTER_SECONDARY}));
    assert(!scene(ui, m, &state, o).columns_changed && col.width == 150);
    cui_destroy(ui);
    cui_table_destroy(m);
}
static void drag_columns(void) {
    cui_table_model *m = cui_table_create(1);
    cui_table_row row = {.key = 1};
    cui_table_column cols[] = {
        {.key = 10, .label = {"First", 5}, .width = 100, .movable = true, .sortable = true},
        {.key = 20, .label = {"Middle", 6}, .width = 100, .movable = true},
        {.key = 30, .label = {"Last", 4}, .width = 100, .movable = true}};
    assert(cui_table_bind(m, &row, 1, cols, 3) == CUI_TABLE_OK);
    cui_table_state state = {0};
    cui_table_options o = {.width = 300, .height = 64};
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    scene(ui, m, &state, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 40, .y = 16}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 280, .y = 16}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 280, .y = 16}));
    cui_table_result result = scene(ui, m, &state, o);
    assert(result.columns_changed && !result.sort_changed && cols[2].key == 10 && !state.dragging);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 40, .y = 16}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 280, .y = 16}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    assert(!scene(ui, m, &state, o).columns_changed && !state.dragging);
    cols[0].fixed_left = true;
    assert(cui_table_bind(m, &row, 1, cols, 3) == CUI_TABLE_OK);
    scene(ui, m, &state, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 40, .y = 16}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 280, .y = 16}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 280, .y = 16}));
    assert(!scene(ui, m, &state, o).columns_changed && cols[0].key == 20);
    cui_destroy(ui);
    cui_table_destroy(m);
}
static void pinned_geometry(void) {
    cui_table_model *m = cui_table_create(1);
    cui_table_row row = {.key = 1, .label = {"Row", 3}};
    cui_table_column cols[] = {
        {.key = 10, .label = {"Pinned", 6}, .width = 100, .fixed_left = true, .movable = true},
        {.key = 20, .label = {"Scroll", 6}, .width = 100, .movable = true},
        {.key = 30, .label = {"Last", 4}, .width = 100, .movable = true}};
    assert(cui_table_bind(m, &row, 1, cols, 3) == CUI_TABLE_OK);
    assert(cui_table_fixed_columns(m) == 1 && cui_table_fixed_width(m) == 100);
    assert(!cui_table_move_column(m, 10, 1) && !cui_table_move_column(m, 20, 0));
    cui_table_state state = {.scroll_x = 50};
    cui_table_options o = {
        .width = 250, .height = 64, .row_height = 32, .selection_mode = CUI_TABLE_SELECT_CELL};
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    scene(ui, m, &state, o);
    assert(cui_bounds(ui, cui_table_cell_id(10, 1, 10)).x == 0);
    assert(cui_bounds(ui, cui_table_cell_id(10, 1, 20)).x == 50);
    assert(cui_bounds(ui, cui_table_header_id(10, 10)).x == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 75, .y = 40}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 75, .y = 40}));
    cui_table_result result = scene(ui, m, &state, o);
    assert(result.activated_column == 10 && state.column == 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 125, .y = 40}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 125, .y = 40}));
    result = scene(ui, m, &state, o);
    assert(result.activated_column == 20 && state.column == 20);
    state.scroll_x = 0;
    scene(ui, m, &state, o);
    cui_focus(ui, cui_table_cell_id(10, 1, 10));
    key(ui, CUI_KEY_RIGHT, 0);
    key(ui, CUI_KEY_RIGHT, 0);
    scene(ui, m, &state, o);
    assert(state.cursor_column == 30 && state.scroll_x == 50 &&
           cui_bounds(ui, cui_table_cell_id(10, 1, 10)).x == 0);
    cols[0].fixed_left = false;
    cols[1].fixed_left = true;
    assert(cui_table_bind(m, &row, 1, cols, 3) == CUI_TABLE_INVALID);
    cols[0].fixed_left = true;
    cols[1].fixed_left = false;
    cui_destroy(ui);
    cui_table_destroy(m);
}
static void horizontal_virtualization(void) {
    cui_table_model *m = cui_table_create(3);
    cui_table_row rows[] = {{.key = 1}, {.key = 2}, {.key = 3}};
    cui_table_column cols[100];
    for (size_t c = 0; c < 100; c++)
        cols[c] = (cui_table_column){.key = c + 1, .width = 100, .label = {"Column", 6}};
    assert(cui_table_bind(m, rows, 3, cols, 100) == CUI_TABLE_OK);
    cui_table_state state = {.scroll_x = 4505};
    cui_table_options o = {.width = 200, .height = 96, .row_height = 32, .column_overscan = 1};
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    rendered = 0;
    scene(ui, m, &state, o);
    assert(rendered == 15);
    cui_rect rect = cui_bounds(ui, cui_table_cell_id(10, 1, 46));
    assert(rect.x == -5);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 25);
    assert(sem[0].details.scrollable && sem[0].details.column_count == 100 &&
           sem[0].details.scroll_max_x == 9800);
    cui_focus(ui, cui_table_cell_id(10, 1, 46));
    key(ui, CUI_KEY_RIGHT, 0);
    key(ui, CUI_KEY_RIGHT, 0);
    scene(ui, m, &state, o);
    assert(state.cursor_column == 48 && state.scroll_x == 4600);
    cui_destroy(ui);
    cui_table_destroy(m);
}
static void virtual_scene(void) {
    cui_table_model *m = cui_table_create(10000);
    cui_table_row *rows = calloc(10000, sizeof(*rows));
    assert(rows);
    for (size_t i = 0; i < 10000; i++)
        rows[i] = (cui_table_row){.key = i + 1, .label = {"Value", 5}};
    cui_table_column cols[] = {{.key = 10, .label = {"Header", 6}, .width = 100},
                               {.key = 20, .label = {"Other", 5}, .width = 100}};
    assert(cui_table_bind(m, rows, 10000, cols, 2) == CUI_TABLE_OK);
    cui_table_state s = {.scroll_y = 160005};
    cui_table_options o = {.width = 200, .height = 240, .row_height = 32, .overscan = 2};
    cui_context *ui = cui_create((cui_config){.width = 700, .height = 500});
    assert(ui);
    rendered = 0;
    scene(ui, m, &s, o);
    assert(rendered == 24);
    cui_rect first = cui_bounds(ui, cui_table_cell_id(10, 5001, 10));
    assert(fabsf(first.y - 27) < .01f);
    cui_focus(ui, cui_table_cell_id(10, 5001, 10));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 10, .y = 100}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 10, .y = 100, .dy = -100}));
    scene(ui, m, &s, o);
    assert(s.scroll_y == 160105);
    s.scroll_y = 160005;
    scene(ui, m, &s, o);
    cui_rect header = cui_bounds(ui, cui_table_header_id(10, 10));
    assert(header.y == 0);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 40);
    bool label_seen = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].role == CUI_ROLE_GRIDCELL) {
            assert(sem[i].name.length == 16);
            label_seen = true;
        }
    assert(label_seen);
    cui_rect clipped = cui_bounds(ui, cui_table_cell_id(10, 4999, 10));
    assert(clipped.y < 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = clipped.x + 8, .y = clipped.y + 8}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = clipped.x + 8, .y = clipped.y + 8}));
    assert(!scene(ui, m, &s, o).selection_changed);
    cui_destroy(ui);
    cui_table_destroy(m);
    free(rows);
}
#endif
int main(void) {
    models();
    spans();
    columns_and_loading();
    events();
    multiple_selection();
    placeholders();
    row_headers();
    scrollbars();
#ifndef CUI_TABLE_BASE_ONLY
    composed_data_table();
    static_tables();
    virtual_scene();
    pointer_resize();
    horizontal_virtualization();
    pinned_geometry();
    drag_columns();
#else
    (void)rendered;
    (void)label;
#endif
    return 0;
}
