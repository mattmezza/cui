/* SPDX-License-Identifier: MIT */
/* Complete headless driver; reuse application/compose in a real host frame loop.
 * Link CUI as usual. No SDL/window, font or platform dependency is required. */
#include "cui/cui_styled_slider_model.h"
#include <stdio.h>
#include <stdlib.h>
typedef struct {
    cui_slider_model_value_v1 confirmed;
    cui_slider_model_state_v1 slider;
    cui_styled_slider_model_state_v1 presentation;
    cui_styled_slider_model_options_v1 style;
    double next_wake_seconds;
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
    cui_begin_box(ui, 900,
                  (cui_box){.width = 240, .height = 24, .fixed_width = true, .fixed_height = true});
    cui_styled_slider_model_result_v1 result = cui_styled_slider_model_events_v1(
        ui, 100, app->confirmed, &app->slider, &options, &app->presentation, &app->style,
        (cui_slider_model_handler_v1){on_request, app});
    cui_end_box(ui);
    app->next_wake_seconds = result.next_wake_seconds;
    return result.render_status == CUI_STYLED_SLIDER_RENDER_OK;
}
static bool frame(cui_context *ui, application *app, float elapsed) {
    if (!cui_begin_frame(ui, 300, 80, elapsed))
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
    cui_theme theme = cui_theme_light();
    if (!cui_styled_slider_model_state_init_v1(sizeof app.presentation, &app.presentation) ||
        !cui_styled_slider_model_options_from_theme_v1(&theme, 16, CUI_SLIDER_MODEL_HORIZONTAL,
                                                       &app.style))
        return EXIT_FAILURE;
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    if (!ui)
        return EXIT_FAILURE;
    bool ok = frame(ui, &app, 0); /* First frame establishes real geometry, no input. */
    cui_focus(ui, cui_base_slider_model_part_id_v1(100, CUI_SLIDER_MODEL_END_THUMB));
    for (unsigned i = 0; ok && i < 2; ++i)
        ok = cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT});
    if (ok)
        ok = frame(ui, &app, 0);
    /* Both requests had immutable current=.75; intention progressed .625,.5.
     * Accepting an external owner does not change that entry snapshot. */
    ok = ok && app.updates == 2 && app.last_current == .75 && app.confirmed.start == .25 &&
         app.confirmed.end == .5;
    /* Publish acceptance, then demonstrate real hover and passive deadlines.
     * A real host waits until next_wake_seconds in Core clock units and supplies
     * actual elapsed dt; this windowless driver advances that clock explicitly. */
    if (ok)
        ok = frame(ui, &app, 0);
    cui_rect end =
        cui_bounds(ui, cui_base_slider_model_part_id_v1(100, CUI_SLIDER_MODEL_END_THUMB));
    if (ok)
        ok = cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                             .x = end.x + end.w * .5f,
                                             .y = end.y + end.h * .5f});
    if (ok)
        ok = frame(ui, &app, 0) && app.next_wake_seconds > cui_frame_time(ui);
    for (unsigned i = 0; ok && i < 10; ++i)
        ok = frame(ui, &app, 1.0f / 60);
    if (ok)
        ok = cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 280, .y = 60});
    if (ok)
        ok = frame(ui, &app, 0);
    for (unsigned i = 0; ok && app.next_wake_seconds && i < 300; ++i)
        ok = frame(ui, &app, 1.0f / 60);
    ok = ok && app.next_wake_seconds == 0;
    if (ok)
        printf("Selection [%.3f, %.3f]; accepted %u requests\n", app.confirmed.start,
               app.confirmed.end, app.updates);
    cui_destroy(ui);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
