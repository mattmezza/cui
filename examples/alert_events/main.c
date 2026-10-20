/* SPDX-License-Identifier: MIT */
/* Headless authoring example: the application owns visibility and a copied
 * aggregate dismissal record. A host can queue ordinary input between frames. */
#include "cui/cui_alert_events.h"
#include <stdlib.h>
typedef struct {
    bool visible;
    unsigned dismissed;
    cui_alert_dismissal last;
} model;
static void dismissed(void *user, const cui_alert_dismissal *event) {
    model *state = user;
    state->last = *event;
    ++state->dismissed;
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 480, .height = 240});
    if (!ui)
        return EXIT_FAILURE;
    model state = {.visible = true};
    cui_theme theme = cui_theme_light();
    if (!cui_begin_frame(ui, 480, 240, .016f)) {
        cui_destroy(ui);
        return EXIT_FAILURE;
    }
    cui_alert_dismiss_result result = cui_alert_events(
        ui, 100, &theme,
        (cui_alert_options){.kind = CUI_ALERT_INFO,
                            .message = {"Your changes were saved.", 24},
                            .width = 400, .visible = &state.visible, .dismissible = true},
        (cui_alert_dismiss_handler){dismissed, &state});
    (void)cui_end_frame(ui);
    /* Choose the callback or dismissal_count for each side effect; do not
     * perform the same effect through both. Neither reading result nor copying
     * it dispatches a second callback. This frame has no queued dismissal. */
    bool ok = result.valid && result.submitted && !result.legacy_dismissed &&
              result.dismissal_count == 0 && state.dismissed == 0 &&
              cui_last_error(ui) == CUI_OK;
    cui_destroy(ui);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
