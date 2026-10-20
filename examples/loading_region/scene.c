/* SPDX-License-Identifier: MIT */
#include "scene.h"
#include "cui/cui_components.h"
static void details(cui_context *ui, const cui_theme *theme, cui_color fg, void *user) {
    (void)user;
    cui_text(ui, cui_string("The server could not load your records."), theme->font_size, fg);
}
static void retry(cui_context *ui, const cui_theme *theme, cui_color fg, void *user) {
    (void)fg;
    loading_scene *scene = user;
    if (cui_button(ui, 31, theme, cui_string("Retry records"), false))
        scene->retries++;
}
void loading_scene_frame(cui_context *ui, const cui_theme *theme, loading_scene *scene,
                         double now) {
    cui_begin_box(ui, 10, (cui_box){.width = 360, .gap = 8});
    cui_interact(ui, 10, CUI_ROLE_REGION, cui_string("Records"), false, false, 0);
    cui_set_tab_stop(ui, 10, false);
    cui_set_semantic_details(
        ui, 10, (cui_semantic_details){.busy = scene->phase == LOADING_REGION_LOADING});
    if (scene->phase != LOADING_REGION_FAILED) {
        cui_begin_box(ui, 11, (cui_box){.width = 350, .height = 24});
        cui_interact(ui, 11, CUI_ROLE_STATUS,
                     cui_string(scene->phase == LOADING_REGION_LOADING ? "Loading records"
                                                                       : "Records loaded"),
                     false, false, 0);
        cui_set_tab_stop(ui, 11, false);
        cui_end_box(ui);
    }
    if (scene->phase == LOADING_REGION_LOADING) {
        for (unsigned i = 0; i < 3; i++) {
            cui_skeleton_options options = cui_skeleton_options_default();
            options.width = 320 - i * 40;
            options.now_seconds = now;
            cui_loading_motion_result result;
            (void)cui_skeleton_ex(ui, 20 + i, theme, &scene->placeholders[i], options, &result);
            /* A visual host schedules the earliest result.next_wake_seconds.
             * It does not create semantic messages for animation ticks. */
        }
    } else if (scene->phase == LOADING_REGION_READY) {
        cui_text(ui, cui_string("Three records are ready."), theme->font_size, theme->text);
    } else {
        (void)cui_alert(ui, 30, theme,
                        (cui_alert_options){.kind = CUI_ALERT_ERROR,
                                            .width = 350,
                                            .visible = &scene->alert_visible,
                                            .dismissible = true,
                                            .content = details,
                                            .action = retry,
                                            .user = scene,
                                            .accessible_name = cui_string(scene->alert_name)});
    }
    cui_end_box(ui);
}
