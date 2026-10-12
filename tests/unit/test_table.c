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
    cui_destroy(ui);
    cui_table_destroy(m);
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
#ifndef CUI_TABLE_BASE_ONLY
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
    columns_and_loading();
    events();
    multiple_selection();
#ifndef CUI_TABLE_BASE_ONLY
    virtual_scene();
    pointer_resize();
    horizontal_virtualization();
#else
    (void)rendered;
    (void)label;
#endif
    return 0;
}
