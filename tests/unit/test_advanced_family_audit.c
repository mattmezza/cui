/* SPDX-License-Identifier: MIT */
#include "cui/cui_plot_view.h"
#include "cui/cui_table.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_str labels(void *user, size_t row, cui_id column) {
    (void)user;
    (void)column;
    return cui_string(row ? "Alpha" : "Zulu");
}
static unsigned callback_cells;
static void cells(cui_context *ui, const cui_theme *theme, void *user, size_t row, cui_id column) {
    callback_cells++;
    cui_text(ui, labels(user, row, column), theme->font_size, theme->text);
}
static cui_table_result table_frame(cui_context *ui, cui_table_model *model,
                                    cui_table_state *state) {
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 240, .016f));
    cui_table_result result =
        cui_data_table(ui, 20, &t, model, state,
                       (cui_table_options){.width = 220,
                                           .height = 160,
                                           .row_height = 32,
                                           .header_height = 32,
                                           .manual_selection = true,
                                           .selection_mode = CUI_TABLE_SELECT_CELL},
                       labels, cells, NULL);
    cui_end_frame(ui);
    assert(result.error == CUI_TABLE_OK && cui_last_error(ui) == CUI_OK);
    return result;
}
static void controlled_sort_and_selection(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_table_model *model = cui_table_create(2);
    assert(model);
    cui_table_row rows[] = {{.key = 1, .label = {"Zulu", 4}}, {.key = 2, .label = {"Alpha", 5}}};
    cui_table_column columns[] = {
        {.key = 10, .label = {"Pinned", 6}, .width = 80, .fixed_left = true, .sortable = true},
        {.key = 11, .label = {"Value", 5}, .width = 200}};
    assert(cui_table_bind(model, rows, 2, columns, 2) == CUI_TABLE_OK);
    cui_table_state state = {.row = 1, .column = 10, .scroll_x = 60};
    callback_cells = 0;
    table_frame(ui, model, &state);
    assert(callback_cells == 4);
    assert(cui_bounds(ui, cui_table_cell_id(20, 1, 10)).x == 0);
    cui_focus(ui, cui_table_header_id(20, 10));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    cui_table_result request = table_frame(ui, model, &state);
    assert(request.sort_changed && state.sort == CUI_TABLE_SORT_ASCENDING);
    /* Current ascending-first policy differs from upstream descending-first.
     * Sorting emits a request: application data stays unchanged until rebind. */
    assert(rows[0].key == 1 && rows[1].key == 2);
    cui_table_row reordered[] = {rows[1], rows[0]};
    assert(cui_table_bind(model, reordered, 2, columns, 2) == CUI_TABLE_OK);
    table_frame(ui, model, &state);
    assert(state.row == 1 && state.column == 10);
    size_t index;
    assert(cui_table_row_index(model, 1, &index) && index == 1);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    bool selected = false;
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == cui_table_cell_id(20, 1, 10)) {
            assert(sem[i].details.row_index == 2 && sem[i].details.selected);
            selected = true;
        }
    assert(selected);
    cui_focus(ui, cui_table_cell_id(20, 1, 10));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    table_frame(ui, model, &state);
    assert(state.cursor_row == 2 && state.row == 1); /* Host rejects selection proposal. */
    cui_table_destroy(model);
    cui_destroy(ui);
}
static void owner_theme_isolation(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    cui_theme first = cui_theme_light(), second = cui_theme_dark();
    first.surface = (cui_color){12, 34, 56, 255};
    second.surface = (cui_color){78, 90, 123, 255};
    cui_color first_before = first.surface;
    assert(cui_begin_frame(ui, 400, 240, .016f));
    cui_begin_box(ui, 100, (cui_box){.row = true});
    cui_table_begin(ui, 101, &first, 180);
    cui_table_caption(ui, 102, &first, cui_string("Invoices"));
    cui_table_end(ui);
    cui_table_begin(ui, 103, &second, 180);
    cui_table_caption(ui, 104, &second, cui_string("Orders"));
    cui_table_end(ui);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    bool a = false, b = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT) {
            if (!memcmp(&draw.commands[i].color, &first.surface, sizeof(cui_color)))
                a = true;
            if (!memcmp(&draw.commands[i].color, &second.surface, sizeof(cui_color)))
                b = true;
        }
    assert(a && b && !memcmp(&first.surface, &first_before, sizeof first_before));
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; i++)
        if (sem[i].id == 101 || sem[i].id == 103)
            assert(sem[i].role == CUI_ROLE_TABLE && sem[i].name.length == 5 &&
                   !memcmp(sem[i].name.data, "Table", 5)); /* Captions do not name tables. */
    cui_destroy(ui);
}
static bool same_scale(cui_plot_scale a, cui_plot_scale b) {
    return a.kind == b.kind && a.domain_min == b.domain_min && a.domain_max == b.domain_max &&
           a.range_start == b.range_start && a.range_end == b.range_end && a.clamp == b.clamp;
}
static void axis_opt_in(void) {
    cui_plot_viewport viewport;
    assert(cui_plot_viewport_init(
               &viewport, (cui_plot_scale){CUI_PLOT_LINEAR, 0, 100, 0, 200, false},
               (cui_plot_scale){CUI_PLOT_LINEAR, -10, 10, 100, 0, false}) == CUI_PLOT_OK);
    cui_plot_viewport_options options = cui_plot_viewport_defaults();
    options.pan_y = false;
    options.zoom_y = false;
    cui_plot_scale before_y = viewport.y;
    assert(cui_plot_viewport_transform(&viewport, options, 40, 70, 2, (cui_plot_point){50, 30}) ==
           CUI_PLOT_OK);
    assert(viewport.x.domain_max - viewport.x.domain_min == 50);
    assert(same_scale(viewport.y, before_y));
    double value;
    assert(cui_plot_inverse(&viewport.x, 50, &value) == CUI_PLOT_OK);
    assert(fabs(value - 5) < 1e-8); /* Pan then anchored zoom preserves intended data. */
    cui_plot_scale before_x = viewport.x;
    options.enabled = false;
    assert(cui_plot_viewport_transform(&viewport, options, 100, 100, 4, (cui_plot_point){0, 0}) ==
           CUI_PLOT_OK);
    assert(same_scale(viewport.x, before_x));
}
static void reduced_motion_appearance(void) {
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    unsigned char pixels[8 * 8 * 4] = {0};
    cui_theme t = cui_theme_light();
    cui_plot_element_state animated = {0}, reduced = {0};
    cui_plot_view_options view = {.bitmap = {pixels, 8, 8, 32, 1}};
    cui_plot_element_options options = {.bounds = {0, 0, 8, 8},
                                        .has_appearance = true,
                                        .appear = {.duration = 1},
                                        .frame_serial = 1};
    assert(cui_begin_frame(ui, 100, 100, 0));
    assert(cui_plot_element(ui, 200, &t, &view, &animated, &options) == CUI_PLOT_OK);
    cui_end_frame(ui);
    assert(animated.appearance.time == 0 && animated.request_frame);
    cui_set_reduced_motion(ui, true);
    assert(cui_begin_frame(ui, 100, 100, 0));
    assert(cui_plot_element(ui, 201, &t, &view, &reduced, &options) == CUI_PLOT_OK);
    cui_end_frame(ui);
    assert(reduced.appearance.time == 1 && !reduced.request_frame);
    assert(animated.appearance.time == 0); /* Owner state remains independent. */
    cui_destroy(ui);
}
int main(void) {
    controlled_sort_and_selection();
    owner_theme_isolation();
    axis_opt_in();
    reduced_motion_appearance();
    return 0;
}
