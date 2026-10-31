/* SPDX-License-Identifier: MIT */
/* Complete headless driver; reuse application/compose in a real host frame loop.
 * Link CUI as usual. No SDL/window, font or platform dependency is required. */
#include "cui/cui_base_slider_model.h"
#include <stdio.h>
#include <stdlib.h>
typedef struct {
    cui_slider_model_value_v1 confirmed;
    cui_slider_model_state_v1 slider;
    unsigned updates;
    double last_current;
} application;
static void on_request(void *user, const cui_slider_model_request_v1 *request) {
    application *app = user;
    if (request->phase == CUI_SLIDER_MODEL_UPDATE) {
        app->confirmed = request->requested; /* Explicit acceptance, external storage only. */
        app->last_current = request->current.end;
        ++app->updates;
    }
    /* CANCEL can carry an old kind/domain; do not blindly accept it. */
}
static bool compose(cui_context *ui, application *app) {
    cui_slider_model_options_v1 options = {.version = CUI_SLIDER_MODEL_V1,
                                           .struct_size = sizeof options,
                                           .domain = {0, 1, .125, CUI_SLIDER_MODEL_LINEAR, 0},
                                           .name = cui_string("Selection"),
                                           .start_name = cui_string("Selection start"),
                                           .end_name = cui_string("Selection end")};
    cui_base_slider_model_layout_v1 layout = {
        .version = CUI_SLIDER_MODEL_V1,
        .struct_size = sizeof layout,
        .length = 240,
        .cross_extent = 24,
        .track_extent = 6,
        .thumb_extent = 16,
        .axis_inset = 8,
        .track = {.background = {80, 80, 80, 255}},
        .fill = {.background = {60, 120, 240, 255}},
        .start_thumb = {.background = {240, 240, 240, 255}, .radius = 8},
        .end_thumb = {.background = {240, 240, 240, 255}, .radius = 8}};
    cui_slider_model_result_v1 result =
        cui_base_slider_model_events_v1(ui, 100, app->confirmed, &app->slider, &options, &layout,
                                        (cui_slider_model_handler_v1){on_request, app});
    return result.status == CUI_SLIDER_MODEL_OK;
}
static bool frame(cui_context *ui, application *app) {
    if (!cui_begin_frame(ui, 300, 80, 0))
        return false;
    bool composed = compose(ui, app);
    cui_draw_list draw = cui_end_frame(ui);
    /* A real host renders draw now, before its next begin invalidates the list. */
    return composed && draw.count && cui_last_error(ui) == CUI_OK;
}
int main(void) {
    application app = {.confirmed = {CUI_SLIDER_MODEL_RANGE, 0, .25, .75}};
    if (!cui_slider_model_state_init_v1(sizeof app.slider, &app.slider))
        return EXIT_FAILURE;
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    if (!ui)
        return EXIT_FAILURE;
    bool ok = frame(ui, &app); /* First frame establishes real geometry, no input. */
    cui_focus(ui, cui_base_slider_model_part_id_v1(100, CUI_SLIDER_MODEL_END_THUMB));
    for (unsigned i = 0; ok && i < 2; ++i)
        ok = cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT});
    if (ok)
        ok = frame(ui, &app);
    /* Both requests had immutable current=.75; intention progressed .625,.5.
     * Accepting an external owner does not change that entry snapshot. */
    ok = ok && app.updates == 2 && app.last_current == .75 && app.confirmed.start == .25 &&
         app.confirmed.end == .5;
    if (ok)
        printf("Selection [%.3f, %.3f]; accepted %u requests\n", app.confirmed.start,
               app.confirmed.end, app.updates);
    cui_destroy(ui);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
