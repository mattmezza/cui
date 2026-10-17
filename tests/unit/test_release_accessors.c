/* SPDX-License-Identifier: MIT */
#include "cui/cui_dock.h"
#include "cui/cui_radio_group.h"
#include "cui/cui_table.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #x);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#ifdef CUI_TEST_ALLOCATOR
void *__real_malloc(size_t);
static bool fail_malloc;
void *__wrap_malloc(size_t size) {
    if (fail_malloc) {
        fail_malloc = false;
        return NULL;
    }
    return __real_malloc(size);
}
#endif
static bool remove_during_visibility(void *user, const cui_dock *model, uint64_t panel) {
    (void)model;
    (void)panel;
    CHECK(cui_dock_remove(user, 12) == CUI_DOCK_OK);
    return true;
}
static bool remove_active(void *user, const cui_dock *model, uint64_t panel) {
    (void)model;
    CHECK(cui_dock_remove(user, panel) == CUI_DOCK_OK);
    return true;
}
static void dock_outputs_and_capture(void) {
    cui_dock *model = cui_dock_create((cui_dock_config){16, 16, 8, 4096});
    CHECK(model);
    cui_dock_node root = cui_dock_root(model), child = root;
    cui_dock_node_info info, before;
    CHECK(cui_dock_node_get(model, root, &info));
    before = info;
    CHECK(!cui_dock_node_get(model, UINT64_MAX, &info));
    CHECK(memcmp(&info, &before, sizeof info) == 0);
    CHECK(!cui_dock_child_at(model, root, 0, &child, NULL) && child == root);
    CHECK(cui_dock_insert(model, root, 11, SIZE_MAX) == CUI_DOCK_OK);
    CHECK(cui_dock_insert(model, root, 12, SIZE_MAX) == CUI_DOCK_OK);
    uint64_t key = 99;
    CHECK(!cui_dock_panel_at(model, root, 2, &key) && key == 99);
    cui_dock_lifecycle_snapshot current[4] = {0}, pending[4] = {0}, saved[4];
    cui_dock_lifecycle state;
    CHECK(cui_dock_lifecycle_init(&state, current, pending, 4));
    cui_dock_lifecycle_region region = {.model = model};
    CHECK(cui_dock_lifecycle_capture(&state, &region, 1, NULL, 0) == CUI_DOCK_OK);
    CHECK(state.pending_count == 2 && state.has_pending);
    memcpy(saved, pending, sizeof saved);
#ifdef CUI_TEST_ALLOCATOR
    fail_malloc = true;
    CHECK(cui_dock_lifecycle_capture(&state, &region, 1, NULL, 0) == CUI_DOCK_CAPACITY);
    CHECK(!fail_malloc && memcmp(saved, pending, sizeof saved) == 0);
    CHECK(state.pending_count == 2 && state.has_pending && !state.dispatching);
#endif
    region.panel_visible = remove_during_visibility;
    region.visibility_user = model;
    CHECK(cui_dock_lifecycle_capture(&state, &region, 1, NULL, 0) == CUI_DOCK_INVALID);
    CHECK(state.pending_count == 2 && state.has_pending && state.current_count == 0);
    CHECK(!state.dispatching);
    CHECK(memcmp(saved, pending, sizeof saved) == 0);
    CHECK(cui_dock_lifecycle_dispatch(&state) && state.current_count == 2);
    region.panel_visible = NULL;
    CHECK(cui_dock_lifecycle_capture(&state, &region, 1, NULL, 0) == CUI_DOCK_OK);
    CHECK(cui_dock_lifecycle_dispatch(&state) && state.current_count == 1);
    key = 99;
    CHECK(!cui_dock_displayed_panel(model, root, remove_active, model, &key) && key == 99);
    CHECK(!cui_dock_panel_find(model, 11, NULL, NULL));
    cui_dock_destroy(model);
}
typedef struct {
    cui_table_model *model;
    cui_table_column *column;
    unsigned calls;
} table_mutation;
static void shrink_header(cui_context *ui, const cui_theme *theme, void *user, cui_id key) {
    (void)ui;
    (void)theme;
    (void)key;
    table_mutation *mutation = user;
    mutation->calls++;
    CHECK(cui_table_bind(mutation->model, NULL, 0, mutation->column, 1) == CUI_TABLE_OK);
}
static cui_str shrink_label(void *user, size_t row, cui_id column) {
    (void)row;
    (void)column;
    table_mutation *mutation = user;
    mutation->calls++;
    CHECK(cui_table_bind(mutation->model, NULL, 0, mutation->column, 1) == CUI_TABLE_OK);
    return cui_string("snapshot");
}
static void table_callbacks(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    cui_table_model *model = cui_table_create(4);
    CHECK(ui && model);
    cui_table_row rows[] = {{.key = 1}, {.key = 2}};
    cui_table_column columns[] = {cui_table_column_default(10, cui_string("A")),
                                  cui_table_column_default(20, cui_string("B"))};
    cui_table_state state = {0};
    cui_theme theme = cui_theme_light();
    cui_table_options options = cui_table_options_default();
    options.width = 500;
    options.height = 300;
    table_mutation mutation = {model, columns, 0};
    options.render_header = shrink_header;
    options.render_userdata = &mutation;
    CHECK(cui_table_bind(model, rows, 2, columns, 2) == CUI_TABLE_OK);
    cui_table_row row_output = {.key = 123};
    cui_table_column col_output = cui_table_column_default(456, cui_string("sentinel"));
    CHECK(!cui_table_get_row(model, 2, &row_output) && row_output.key == 123);
    CHECK(!cui_table_get_column(model, 2, &col_output) && col_output.key == 456);
    CHECK(cui_begin_frame(ui, 600, 400, .016f));
    CHECK(cui_data_table(ui, 100, &theme, model, &state, options, NULL, NULL, NULL).error ==
          CUI_TABLE_OK);
    cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK && mutation.calls == 1 && cui_table_rows(model) == 0);
    options.render_header = NULL;
    mutation.calls = 0;
    CHECK(cui_table_bind(model, rows, 2, columns, 2) == CUI_TABLE_OK);
    CHECK(cui_begin_frame(ui, 600, 400, .016f));
    CHECK(cui_data_table(ui, 100, &theme, model, &state, options, shrink_label, NULL, &mutation)
              .error == CUI_TABLE_OK);
    cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK && mutation.calls == 1);
    cui_table_destroy(model);
    cui_destroy(ui);
}
static void radio_metrics(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    CHECK(ui);
    cui_theme theme = cui_theme_light();
    cui_radio_group_item item = {.id = 42, .label = cui_string("Choice")};
    cui_radio_group_state state = {0};
    cui_id selected = 42;
    CHECK(cui_begin_frame(ui, 600, 400, .016f));
    CHECK(!cui_radio_group(ui, 100, &theme, &item, 1, &selected, &state,
                           (cui_radio_group_options){.design_size = (cui_design_size)999})
               .valid);
    CHECK(selected == 42 && !state.had_focus);
    CHECK(cui_radio_group(ui, 100, &theme, &item, 1, &selected, &state,
                          (cui_radio_group_options){.design_size = CUI_DESIGN_XS})
              .valid);
    cui_end_frame(ui);
    CHECK(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
int main(void) {
    dock_outputs_and_capture();
    table_callbacks();
    radio_metrics();
    return 0;
}
