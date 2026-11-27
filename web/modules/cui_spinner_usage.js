/* SPDX-License-Identifier: MIT */
export function spinnerUsage(size, icon, color, easing, sizing, rem) {
 const curves=['phase','phase','.25f','-.75f','1.25f','phase + .25f','NAN','INFINITY','-INFINITY'];
 const names=['CUI_SPINNER_CUSTOM','CUI_SPINNER_XSMALL','CUI_SPINNER_SMALL','CUI_SPINNER_MEDIUM','CUI_SPINNER_LARGE','CUI_SPINNER_CUSTOM'];
 const artworkState=icon===3?`typedef struct {
    cui_icon_state icon;
    bool ready;
} spinner_artwork;

static void spinner_artwork_collect(void *resource) {
    cui_icon_state_collect(&((spinner_artwork *)resource)->icon);
}

static void spinner_artwork_destroy(void *resource) {
    cui_icon_state_destroy(&((spinner_artwork *)resource)->icon);
}

`:'';
 const paint=icon===3?`    (void)user;
    /* The context owns the SVG and keeps its raster alive through rendering. */
    spinner_artwork *artwork = cui_resource_get(
        ui, id, UINT64_C(0x6d5c60713bb44f7a), sizeof(*artwork),
        spinner_artwork_collect, spinner_artwork_destroy);
    if (!artwork)
        return false;
    if (!artwork->ready) {
        artwork->ready = cui_icon_set_data(
            &artwork->icon, cui_string("<svg viewBox='0 0 24 24'>"
                                      "<path d='M12 2 L20 14 L12 20 L6 13 L11 13 Z'/></svg>"));
        if (!artwork->ready)
            return false;
    }
    return cui_icon(ui, id, theme, &artwork->icon, options) == CUI_ICON_OK;`
  :`    return cui_icon_named(ui, id, theme, cui_string(user), options) == CUI_ICON_OK;`;
 return `#include "cui/cui_content.h"
#include "cui/cui_icon.h"
#include <math.h>

${artworkState}static float spinner_ease(float phase, void *user) {
    (void)user; (void)phase;
    return ${curves[easing]};
}
static bool spinner_icon(cui_context *ui, cui_id id, const cui_theme *theme,
                         float size, cui_color tint, float radians, void *user) {
    cui_icon_options options = {.size = CUI_ICON_CUSTOM, .custom_size = size,
        .custom_color = true, .color = tint, .custom_transform = true,
        .transform = cui_icon_rotation(radians)};
${paint}
}
/* Retain a separate zero-initialized motion for each Spinner.
 * Call inside a frame; now_seconds and returned wake share one clock domain.
 * The local sizing and callbacks are borrowed only through this submission. */
bool draw_spinner(cui_context *ui, cui_id id, const cui_theme *theme,
                   cui_loading_motion *motion, double now_seconds, bool reduced_motion,
                   cui_loading_motion_result *result) {
    const int custom_easing = ${easing};
    const int custom_icon = ${icon};
    const bool use_sizing = ${sizing ? 'true' : 'false'};
    cui_spinner_sizing sizing = {.size = ${names[sizing]},
                                .pixels = ${size}, .rem_size = ${rem}};
    cui_spinner_options options = cui_spinner_options_default();
    options.size = ${size};
    options.sizing = use_sizing ? &sizing : NULL;
    options.now_seconds = now_seconds;
    options.reduced_motion = reduced_motion;
    options.custom_color = ${color ? 'true' : 'false'};
    options.color = (cui_color){220, 40, 90, ${color===2 ? 0 : 255}};
    options.ease = custom_easing ? spinner_ease : NULL;
    options.paint = custom_icon ? spinner_icon : NULL;
    options.paint_user = ${icon===3?'NULL':'custom_icon == 1 ? "loader-circle" : "arrow-right"'};
    /* Finite turns may be negative or exceed one; reduced motion evaluates
     * ease(0). NaN/Infinity reject without committing motion/result or paint.
     * On success schedule result->next_wake_seconds; zero means idle. */
    return cui_spinner_ex(ui, id, theme, motion, options, result);
}
`;
}
