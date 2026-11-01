/* SPDX-License-Identifier: MIT */
#include "cui/cui_skeleton_presentation.h"
#include <math.h>
cui_skeleton_presentation cui_skeleton_presentation_default(void) {
    return (cui_skeleton_presentation){.root = {.width = 1, .height = 16, .width_percent = true},
                                        .duration = 2};
}
bool cui_skeleton_present(cui_context *ui, cui_id id, const cui_skeleton_palette *palette,
                          cui_loading_motion *state, cui_skeleton_presentation presentation,
                          cui_loading_motion_result *out) {
    if (!ui || !id || !palette || !state || !out || !isfinite(presentation.duration) ||
        presentation.duration < 0 || presentation.duration > 60)
        return false;
    /* Entry snapshots: easing may mutate external application style/palette. */
    cui_box root = presentation.root;
    cui_color background = palette->background;
    if (presentation.secondary)
        background.a = (uint8_t)(background.a * .5f);
    if (presentation.custom_background)
        background = root.background;
    cui_loading_motion next = *state;
    cui_loading_motion_result result;
    if (!cui_loading_motion_sample(&next, presentation.now_seconds,
                                   presentation.duration ? presentation.duration : 2,
                                   presentation.reduced_motion || cui_reduced_motion(ui), true,
                                   presentation.ease, presentation.ease_user, &result))
        return false;
    root.background = background;
    /* Source order: style refinement first, animation opacity last. This also
     * fades own-root border; ancestor opacity remains Core's independent layer. */
    root.custom_opacity = true;
    root.opacity = result.opacity;
    cui_begin_box(ui, id, root);
    cui_end_box(ui);
    *state = next;
    *out = result;
    return true;
}
