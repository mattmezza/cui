/* SPDX-License-Identifier: MIT */
#include "cui/cui_stepper_navigation.h"
#include <stdlib.h>
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition))                                                                          \
            abort();                                                                               \
    } while (0)
typedef struct {
    size_t selected, requests;
    bool payment_valid, validation_failed;
} application;
static void navigate(void *user, const cui_stepper_navigation_request *request) {
    application *app = user;
    app->requests++;
    /* This validation belongs to the application, not to Stepper. */
    if (request->requested == 2 && !app->payment_valid) {
        app->validation_failed = true;
        return;
    }
    app->validation_failed = false;
    app->selected = request->requested;
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 800, .height = 300});
    CHECK(ui);
    cui_theme theme = cui_theme_light();
    application app = {0};
    for (unsigned frame = 0; frame < 4; ++frame) {
        if (frame == 2)
            app.payment_valid = true;
        if (frame == 1 || frame == 2)
            CHECK(cui_queue_event(ui,
                                  (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                              .accessibility_target = cui_numeric_part_id(10, 3),
                                              .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
        /* Descriptors reflect owner errors from a previous call. The callback
         * changes owner fields only, never this still-borrowed array. */
        const cui_step steps[] = {
            {.title = {"Account", 7}},
            {.title = {"Payment", 7},
             .status = app.validation_failed ? CUI_STEP_ERROR : CUI_STEP_PENDING},
            {.title = {"Confirm", 7}}};
        CHECK(cui_begin_frame(ui, 800, 300, .016f));
        cui_begin_box(ui, 1, (cui_box){.width = 800, .height = 300});
        cui_stepper_navigation_result result =
            cui_stepper_navigation(ui, 10, &theme, steps, 3, app.selected, false, false,
                                   (cui_stepper_navigation_handler){navigate, &app});
        CHECK(result.valid && result.request_count == (frame == 1 || frame == 2));
        cui_end_box(ui);
        (void)cui_end_frame(ui);
        CHECK(cui_last_error(ui) == CUI_OK);
        CHECK(app.selected == (frame < 2 ? 0 : 2));
        CHECK(app.validation_failed == (frame == 1));
    }
    CHECK(app.requests == 2);
    cui_destroy(ui);
    return 0;
}
