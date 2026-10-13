/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_table.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <string.h>
static unsigned label_calls, cell_calls, actions;
static cui_draw_list last_table_draw;
static cui_str label(void *user, size_t row, cui_id column) {
    (void)user;
    (void)column;
    label_calls++;
    return cui_string(row ? "Alpha" : "Zulu");
}
static void child(cui_context *ui, const cui_theme *theme, void *user, size_t row, cui_id column) {
    (void)user;
    cell_calls++;
    if (column == 11 && cui_button(ui, 900 + row, theme, cui_string("Inspect"), false))
        actions++;
}
static cui_table_result frame(cui_context *ui, cui_table_model *model, cui_table_state *state,
                              cui_table_options options, bool custom) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_table_result result =
        cui_data_table(ui, 20, &theme, model, state, options, label, custom ? child : NULL, NULL);
    last_table_draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void key(cui_context *ui, cui_key code) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code}));
}
static void tap(cui_context *ui, cui_id id) {
    cui_rect r = cui_bounds(ui, id);
    assert(r.w > 0 && r.h > 0);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + r.w * .5f, .y = r.y + r.h * .5f}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + r.w * .5f, .y = r.y + r.h * .5f}));
}
static void selection_and_sort(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_table_model *model = cui_table_create(2);
    assert(model);
    cui_table_row rows[] = {{.key = 1, .label = {"Zulu", 4}}, {.key = 2, .label = {"Alpha", 5}}};
    cui_table_column columns[] = {{.key = 10, .label = {"Name", 4}, .width = 140, .sortable = true},
                                  {.key = 11,
                                   .label = {"Actions", 7},
                                   .width = 140,
                                   .sortable = true,
                                   .resizable = true,
                                   .nonselectable = true}};
    assert(cui_table_bind(model, rows, 2, columns, 2) == CUI_TABLE_OK);
    cui_table_options o = {.width = 280,
                           .height = 160,
                           .row_height = 48,
                           .header_height = 32,
                           .selection_mode = CUI_TABLE_SELECT_CELL,
                           .sort_cycle = CUI_TABLE_SORT_CYCLE_SOURCE,
                           .accessible_name = {"Files", 5}};
    cui_table_state s = {.row = 1, .column = 10};
    label_calls = 0;
    frame(ui, model, &s, o, false);
    assert(label_calls == 4);
    tap(ui, cui_table_cell_id(20, 2, 11));
    cui_table_result result = frame(ui, model, &s, o, false);
    assert(!result.selection_changed && s.row == 1 && s.column == 10);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool found = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == cui_table_cell_id(20, 2, 11)) {
            assert(!sem[i].disabled && !sem[i].details.selected && sem[i].details.readonly);
            found = true;
        }
    assert(found && cui_focused(ui) == cui_table_cell_id(20, 2, 11));
    s.row = 1;
    s.column = 11;
    frame(ui, model, &s, o, false);
    assert(s.row == 1 && s.column == 11);
    columns[1].nonselectable = false;
    frame(ui, model, &s, o, false);
    sem = cui_semantics(ui, &n);
    found = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == cui_table_cell_id(20, 1, 11)) {
            assert(sem[i].details.selected && !sem[i].details.readonly);
            found = true;
        }
    assert(found);
    columns[1].nonselectable = true;
    s.row = 1;
    s.column = 10;

    cui_focus(ui, cui_table_header_id(20, 11));
    const cui_table_sort expected[] = {CUI_TABLE_SORT_DESCENDING, CUI_TABLE_SORT_ASCENDING,
                                       CUI_TABLE_SORT_DEFAULT};
    for (unsigned cycle = 0; cycle < 3; cycle++) {
        key(ui, CUI_KEY_ENTER);
        result = frame(ui, model, &s, o, false);
        assert(result.sort_changed && s.sort_column == 11 && s.sort == expected[cycle]);
        assert(rows[0].key == 1 && rows[1].key == 2);
        assert(!frame(ui, model, &s, o, false).sort_changed); /* No repeated host request. */
    }
    cui_focus(ui, cui_table_resize_id(20, 11));
    key(ui, CUI_KEY_RIGHT);
    assert(frame(ui, model, &s, o, false).columns_changed && columns[1].width == 148);
    o.sort_cycle = CUI_TABLE_SORT_CYCLE_LEGACY;
    cui_focus(ui, cui_table_header_id(20, 10));
    key(ui, CUI_KEY_ENTER);
    assert(frame(ui, model, &s, o, false).sort_changed && s.sort == CUI_TABLE_SORT_ASCENDING);
    o.selection_mode = CUI_TABLE_SELECT_COLUMN;
    s.row = 0;
    s.column = 10;
    cui_focus(ui, cui_table_header_id(20, 11));
    key(ui, CUI_KEY_ENTER);
    assert(!frame(ui, model, &s, o, false).selection_changed && s.column == 10);
    o.selection_mode = CUI_TABLE_SELECT_ROW;
    s.row = 1;
    s.column = 0;
    o.row_selection_disabled = true;
    frame(ui, model, &s, o, true);
    cell_calls = 0;
    tap(ui, 901);
    result = frame(ui, model, &s, o, true);
    assert(actions == 1 && cell_calls == 4 && !result.selection_changed && s.row == 1);
    cui_focus(ui, 901);
    key(ui, CUI_KEY_ENTER);
    frame(ui, model, &s, o, true);
    assert(actions == 2);
    o.row_selection_disabled = false;
    frame(ui, model, &s, o, false);
    /* Nonselectable action columns still participate in whole-row highlighting. */
    sem = cui_semantics(ui, &n);
    found = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == cui_table_cell_id(20, 1, 11)) {
            assert(sem[i].details.selected && !sem[i].details.readonly);
            found = true;
        }
    assert(found);
    cui_rect highlighted = cui_bounds(ui, cui_table_cell_id(20, 1, 11));
    cui_theme theme = cui_theme_light();
    bool painted = false;
    for (size_t i = 0; i < last_table_draw.count; i++) {
        const cui_draw_command *command = &last_table_draw.commands[i];
        if (command->type == CUI_DRAW_RECT && command->bounds.x == highlighted.x &&
            command->bounds.y == highlighted.y && command->bounds.w == highlighted.w &&
            !memcmp(&command->color, &theme.primary, sizeof theme.primary))
            painted = true;
    }
    assert(painted);
    cui_table_row reordered[] = {rows[1], rows[0]};
    assert(cui_table_bind(model, reordered, 2, columns, 2) == CUI_TABLE_OK);
    frame(ui, model, &s, o, false);
    assert(s.row == 1);
    assert(cui_bounds(ui, cui_table_cell_id(20, 1, 10)).y == 80);
    o.sort_cycle = (cui_table_sort_cycle)99;
    assert(frame(ui, model, &s, o, false).error == CUI_TABLE_INVALID);
    cui_table_destroy(model);
    cui_destroy(ui);
}
static void selection_mode_policies(void) {
    cui_context *ui = cui_create((cui_config){0});
    cui_table_model *model = cui_table_create(2);
    cui_table_row rows[] = {{.key = 1}, {.key = 2}};
    cui_table_column columns[] = {{.key = 10, .width = 100, .sortable = true},
                                  {.key = 11, .width = 100, .nonselectable = true},
                                  {.key = 12, .width = 100}};
    assert(ui && model && cui_table_bind(model, rows, 2, columns, 3) == CUI_TABLE_OK);
    cui_table_options o = {.width = 300,
                           .height = 160,
                           .row_height = 32,
                           .header_height = 32,
                           .selection_mode = CUI_TABLE_SELECT_CELL,
                           .cell_selection_disabled = true};
    cui_table_state state = {.row = 1, .column = 10};
    frame(ui, model, &state, o, false);
    tap(ui, cui_table_cell_id(20, 2, 12));
    assert(!frame(ui, model, &state, o, false).selection_changed && state.row == 1 &&
           state.column == 10);
    o.selection_mode = CUI_TABLE_SELECT_COLUMN;
    o.column_selection_disabled = true;
    cui_focus(ui, cui_table_header_id(20, 10));
    key(ui, CUI_KEY_ENTER);
    assert(frame(ui, model, &state, o, false).sort_changed && state.row == 1 && state.column == 10);
    o.cell_selection_disabled = o.column_selection_disabled = false;
    o.selection_mode = CUI_TABLE_SELECT_CELL;
    o.multiple_selection = true;
    cui_table_selection selected[4];
    state = (cui_table_state){.selection = selected, .selection_capacity = 4};
    frame(ui, model, &state, o, false);
    tap(ui, cui_table_cell_id(20, 1, 10));
    assert(frame(ui, model, &state, o, false).selection_changed && state.selection_count == 1);
    cui_rect target = cui_bounds(ui, cui_table_cell_id(20, 2, 12));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = target.x + 20,
                                           .y = target.y + 12,
                                           .modifiers = CUI_MOD_SHIFT}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = target.x + 20,
                                           .y = target.y + 12,
                                           .modifiers = CUI_MOD_SHIFT}));
    cui_table_result result = frame(ui, model, &state, o, false);
    assert(result.error == CUI_TABLE_OK && result.selection_changed && state.selection_count == 4);
    for (size_t i = 0; i < state.selection_count; i++)
        assert(selected[i].column != 11);
    cui_table_destroy(model);
    cui_destroy(ui);
}
static void metadata_and_design(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    for (unsigned density = 0; density < 2; density++)
        for (int size = CUI_DESIGN_XS; size <= CUI_DESIGN_9XL; size++) {
            cui_theme t = cui_theme_light();
            t.font_size = 16 * (density ? .875f : 1);
            cui_design_metrics m;
            assert(cui_design_measure((cui_design_size)size, t.font_size / 16, &m));
            assert(cui_begin_frame(ui, 800, 500, .016f));
            assert(cui_table_begin_ex(ui, 1, &t,
                                      (cui_table_basic_options){.width = 700,
                                                                .accessible_name = {"Invoices", 8},
                                                                .row_count = 1,
                                                                .column_count = 1}));
            assert(cui_table_begin_row_checked(
                ui, 2, &t,
                (cui_table_row_options){.name = {"Invoice row", 11},
                                        .parent = 1,
                                        .row_index = 1,
                                        .design_size = (cui_design_size)size}));
            assert(cui_table_begin_cell_ex(
                ui, 3, &t,
                (cui_table_cell_options){.width = 700,
                                         .name = {"Paid", 4},
                                         .parent = 2,
                                         .row_index = 1,
                                         .column_index = 1,
                                         .design_size = (cui_design_size)size}));
            cui_text_with_line_height(ui, cui_string("Paid"), m.font_size, t.text, m.line_height);
            cui_table_end(ui);
            cui_table_end(ui);
            cui_table_end(ui);
            cui_draw_list draw = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            cui_rect bounds = cui_bounds(ui, 3);
            assert(bounds.h == m.control_height);
            bool text = false;
            for (size_t i = 0; i < draw.count; i++)
                if (draw.commands[i].type == CUI_DRAW_TEXT) {
                    cui_rect b = draw.commands[i].bounds;
                    assert(b.x >= bounds.x && b.y >= bounds.y &&
                           b.x + b.w <= bounds.x + bounds.w + .1f &&
                           b.y + b.h <= bounds.y + bounds.h + .1f);
                    text = true;
                }
            assert(text);
            size_t n;
            const cui_semantic *sem = cui_semantics(ui, &n);
            assert(n == 3);
            assert(sem[0].name.length == 8 && sem[0].details.row_count == 1 &&
                   sem[0].details.column_count == 1);
            assert(sem[1].details.parent == 1 && sem[1].details.row_index == 1);
            assert(sem[2].role == CUI_ROLE_CELL && sem[2].name.length == 4 &&
                   sem[2].details.parent == 2 && sem[2].details.column_index == 1);
        }
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 800, 500, .016f));
    assert(!cui_table_begin_ex(ui, 1, &t, (cui_table_basic_options){.width = NAN}));
    assert(!cui_table_begin_ex(ui, 1, &t, (cui_table_basic_options){.row_count = UINT_MAX}));
    assert(!cui_table_begin_row_checked(ui, 2, &t, (cui_table_row_options){.height = -1}));
    assert(!cui_table_begin_row_checked(ui, 2, &t, (cui_table_row_options){.row_index = UINT_MAX}));
    assert(!cui_table_begin_row_checked(
        ui, 2, &t, (cui_table_row_options){.design_size = (cui_design_size)99}));
    assert(!cui_table_begin_row_checked(ui, 2, &t, (cui_table_row_options){.parent = 2}));
    assert(!cui_table_begin_cell_ex(ui, 2, &t, (cui_table_cell_options){.parent = 2}));
    assert(!cui_table_begin_cell_ex(ui, 2, &t, (cui_table_cell_options){.width = -1}));
    assert(!cui_table_begin_cell_ex(ui, 2, &t, (cui_table_cell_options){.height = INFINITY}));
    assert(!cui_table_begin_cell_ex(ui, 2, &t, (cui_table_cell_options){.padding = -1}));
    assert(!cui_table_begin_cell_ex(ui, 2, &t,
                                    (cui_table_cell_options){.design_size = (cui_design_size)99}));
    assert(!cui_table_begin_cell_ex(ui, 2, &t,
                                    (cui_table_cell_options){.align = (cui_table_align)99}));
    assert(!cui_table_begin_cell_ex(ui, 2, &t, (cui_table_cell_options){.column_index = UINT_MAX}));
    assert(!cui_table_begin_cell_ex(ui, 2, &t, (cui_table_cell_options){.name = {NULL, 1}}));
    assert(cui_table_begin_cell_ex(ui, 3, &t,
                                   (cui_table_cell_options){.width = 100,
                                                            .height = 40,
                                                            .padding = 0,
                                                            .custom_padding = true,
                                                            .header = true,
                                                            .name = {"Header", 6}}));
    cui_table_end(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 1 && sem[0].role == CUI_ROLE_COLUMNHEADER && sem[0].details.readonly);
    float widths[] = {80, 120, 40}, span = 0;
    assert(cui_table_span_width(widths, 3, 0, 2, &span) && span == 200);
    cui_destroy(ui);
}
int main(void) {
    selection_and_sort();
    metadata_and_design();
    selection_mode_policies();
    return 0;
}
