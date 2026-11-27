/* SPDX-License-Identifier: MIT */
/* Configured whole-header consumer: ordinary V1 drawing, no font adapter. */
#define CUI_IMPLEMENTATION_ALL
#include "cui_all.h"
#include <assert.h>
static unsigned calls[CUI_DIALOG_SLOT_COUNT];
static void dialog_consumer_emit(cui_context *ui, const cui_dialog_frame *frame, void *user) {
    (void)frame;
    unsigned kind = *(const unsigned *)user;
    ++calls[kind];
    cui_begin_box(
        ui, 100 + kind,
        (cui_box){.width = -1, .height = kind == CUI_DIALOG_BODY ? 500 : 20, .fixed_height = true});
    cui_end_box(ui);
}
static void app_content(cui_context *ui, void *user) {
    (void)ui;
    (void)user;
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(ui);
    cui_surface_state surface;
    cui_dialog_layout_v1 layout;
    cui_surface_init(&surface, 50, CUI_SURFACE_DIALOG);
    assert(cui_dialog_layout_init_v1(&layout, &surface));
    assert(cui_dialog_open_v1(ui, &layout, 0));
    cui_surface_options options = {.viewport = {0, 0, 400, 300}, .margin = 16, .hide_close = true};
    cui_dialog_sizing_v1 sizing;
    assert(cui_dialog_sizing_default_v1(options, &sizing));
    cui_dialog_composition_v1 composition = {
        .struct_size = sizeof composition, .version = 1, .present = 63, .name = {"Dialog", 6}};
    unsigned kinds[CUI_DIALOG_SLOT_COUNT];
    for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i) {
        kinds[i] = i;
        composition.slots[i] = (cui_dialog_slot){dialog_consumer_emit, &kinds[i]};
    }
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_dialog_submit_result result =
        cui_base_dialog_compose_v1(ui, &layout, options, &sizing, &composition);
    cui_draw_list draw = cui_end_frame(ui);
    assert(result.valid && result.submitted && cui_last_error(ui) == CUI_OK && draw.count > 0);
    assert(layout.completed[0].h > 0 && layout.completed[0].h <= 268);
    for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i)
        assert(calls[i] == 1);
    cui_surface_request_close(ui, &surface, CUI_SURFACE_CLOSED);
    assert(cui_begin_frame(ui, 400, 300, .016f));
    result = cui_base_dialog_compose_v1(ui, &layout, options, &sizing, &composition);
    (void)cui_end_frame(ui);
    assert(result.valid && !result.submitted && !surface.overlay.open &&
           cui_last_error(ui) == CUI_OK);
    cui_root root = {0};
    cui_root_surface entries[1];
    assert(cui_root_init(&root, NULL, 0, NULL, NULL));
    assert(cui_root_bind_facilities(&root, entries, 1, NULL));
    cui_root_composed_dialog_v1 entry = {
        .layout = &layout, .options = options, .composition = &composition};
    cui_theme theme = cui_theme_light();
    assert(cui_root_dialog_open_composed_v1(&root, ui, &entry, 0));
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_root_render(ui, 908, &root, &theme, options.viewport, NULL, app_content, NULL, 0));
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && layout.completed[0].w == 368);
    for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i)
        assert(calls[i] == 2);
    assert(cui_root_dialog_close_top(&root, ui, CUI_SURFACE_CLOSED));
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_root_render(ui, 908, &root, &theme, options.viewport, NULL, app_content, NULL, 0));
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && !root.dialog_count);
    cui_root_dispose(&root);
    cui_destroy(ui);
    return 0;
}
