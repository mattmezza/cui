/* SPDX-License-Identifier: MIT */
export function stepperUsage(visual=0, vertical=1, density=0, icons=0, size=0, disabled=false) {
 const densities=['MD','XS','SM','LG'];
 const sizes=['INHERIT','XS','SM','BASE','MD','LG','XL','2XL','3XL','4XL','5XL','6XL','7XL','8XL','9XL'];
 return `#include "cui/cui_stepper_${visual?'visual':'navigation'}.h"

/* Initialize owner with {0} and retain it across frames.
 * Called once per accepted activation.
 * Selection changes are visible on the next submission. */
typedef struct { size_t selected; bool confirmed, rejected; } step_owner;
static void request_step(void *user, const cui_stepper_navigation_request *request) {
    step_owner *owner = user;
    owner->rejected = request->requested > 0 && !owner->confirmed;
    if (!owner->rejected) owner->selected = request->requested;
}
void draw_steps(cui_context *ui, const cui_theme *theme, step_owner *owner) {
    const cui_step steps[] = {
        {.title = CUI_STR("Details"), .description = CUI_STR("Confirm before continuing"),
         .status = owner->rejected ? CUI_STEP_ERROR :
                   owner->selected > 0 ? CUI_STEP_COMPLETE : CUI_STEP_ACTIVE},
        {.title = CUI_STR("Review"), .description = CUI_STR("Review application"),
         .status = owner->selected > 1 ? CUI_STEP_COMPLETE : CUI_STEP_PENDING},
        {.title = CUI_STR("Finish"), .description = CUI_STR("Ready to submit")}
    };
${visual&&icons?`    /* Actual RGBA8 alpha mask. Pixel storage lives through renderer completion.
     * Replace with your own bitmap or an existing Icon API raster. */
    static const unsigned char mask[] = {
        255,255,255,0, 255,255,255,255, 255,255,255,0,
        255,255,255,255, 255,255,255,255, 255,255,255,255,
        255,255,255,0, 255,255,255,255, 255,255,255,0
    };
    const cui_stepper_icon icons[3] = {
        {.present=true, .bitmap={mask,3,3,12,1}},
        {.present=true, .bitmap={mask,3,3,12,1}},
        {.present=true, .bitmap={mask,3,3,12,1}}
    };
`:''}${visual?`    const cui_stepper_visual_options options = {
        .vertical = ${!!vertical}, .disabled = ${!!disabled},
        .density = CUI_STEPPER_DENSITY_${densities[density]},
        .design_size = CUI_DESIGN_${sizes[size]}${icons?', .icons = icons, .icon_count = 3':''}
    };
    (void)cui_stepper_visual_navigation(ui, 100, theme, steps, 3, owner->selected,
        options, (cui_stepper_navigation_handler){request_step, owner});
    /* Non-centered layout; responsive/centered source styling is partial. */`: `    (void)cui_stepper_navigation(ui, 100, theme, steps, 3, owner->selected,
        ${!!vertical}, ${!!disabled}, (cui_stepper_navigation_handler){request_step, owner});`}
}
/* Set owner->confirmed from your form/checkbox before submitting this control.
 * Do not mutate borrowed step/icon descriptors or input routing in callbacks. */`;
}
