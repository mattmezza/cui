/* SPDX-License-Identifier: MIT */
export function stepperUsage(mode = 2, orientation = 2, density = 0, icons = 0, size = 0, disabled = false) {
 const visual = mode !== 0;
 const wrapped = mode === 3 || mode === 4;
 const layout = mode === 4 ? 'CENTERED_FILL' : wrapped ? 'WRAPPED_FILL' : 'FILL';
 const fill = mode === 2 || wrapped;
 const automatic = orientation === 2;
 const sizeToken = ['INHERIT','XS','SM','BASE','MD','LG','XL','2XL','3XL','4XL','5XL','6XL','7XL','8XL','9XL'][size];
 const densityToken = ['MD','XS','SM','LG'][density];
 const header = fill ? 'presentation' : visual ? 'visual' : 'navigation';
 const orientationCode = automatic ? `    float layout_width = content_width;
${visual?`    cui_design_metrics metrics;
    if (cui_design_measure(CUI_DESIGN_${sizeToken}, theme->font_size / 16, &metrics))
        layout_width /= fmaxf(1, metrics.font_size / theme->font_size);
`:''}    bool vertical = !cui_responsive_at_least(cui_responsive_query(layout_width, NULL),
                                             CUI_BREAKPOINT_SM);` : `    bool vertical = ${orientation === 1};`;
 return `#include "cui/cui_stepper_${header}.h"
${automatic?'#include "cui/cui_responsive.h"\n':''}
/* Initialize owner with {0} and retain it across frames. */
typedef struct { size_t selected; bool confirmed, rejected; } step_owner;
static void request_step(void *user, const cui_stepper_navigation_request *request) {
    step_owner *owner = user;
    owner->rejected = request->requested > 0 && !owner->confirmed;
    if (!owner->rejected) owner->selected = request->requested;
}
void draw_steps(cui_context *ui, const cui_theme *theme, step_owner *owner${automatic?', float content_width':''}${fill&&!wrapped?', float vertical_extent':''}) {
    cui_id id = cui_id_from(ui, "setup-steps");
    const cui_step steps[] = {
        {.title = CUI_STR("Details"), .description = CUI_STR("Confirm before continuing"),
         .status = owner->rejected ? CUI_STEP_ERROR :
                   owner->selected > 0 ? CUI_STEP_COMPLETE : CUI_STEP_ACTIVE},
        {.title = CUI_STR("Review"), .description = CUI_STR("Review application"),
         .status = owner->selected > 1 ? CUI_STEP_COMPLETE : CUI_STEP_PENDING},
        {.title = CUI_STR("Finish"), .description = CUI_STR("Ready to submit")}
    };
${orientationCode}
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
`:''}    cui_stepper_navigation_handler handler = {request_step, owner};
${visual?`    const cui_stepper_visual_options options = {
        .vertical = vertical, .disabled = ${!!disabled},
        .density = CUI_STEPPER_DENSITY_${densityToken},
        .design_size = CUI_DESIGN_${sizeToken}${icons?', .icons = icons, .icon_count = 3':''}
    };
${fill?`    /* ${wrapped?'The containing box supplies the width; rows fit their wrapped labels.':'Give the containing box a width. A positive vertical extent spaces the steps.'} */
    const cui_stepper_presentation_options presentation = {
        .layout = CUI_STEPPER_LAYOUT_${layout},
${wrapped?'':'        .height = vertical ? vertical_extent : 0,\n'}        .custom_palette = true,
        .palette = {
            .normal = theme->secondary,
            .hover = cui_color_mix_oklab(theme->secondary, theme->secondary_foreground, .12f),
            .pressed = cui_color_mix_oklab(theme->secondary, theme->secondary_foreground, .22f),
            .foreground = theme->secondary_foreground
        }
    };
    cui_stepper_navigation_result result =
        cui_stepper_present(ui, id, theme, steps, 3, owner->selected,
                            options, presentation, handler);`:`    cui_stepper_navigation_result result =
        cui_stepper_visual_navigation(ui, id, theme, steps, 3, owner->selected,
                                      options, handler);`}`:`    cui_stepper_navigation_result result =
        cui_stepper_navigation(ui, id, theme, steps, 3, owner->selected,
                               vertical, ${!!disabled}, handler);`}
    if (result.request_count) cui_invalidate(ui);
}
/* Set owner->confirmed from your form before submitting the steps.
 * Selection changes appear on the next submission. Keep borrowed descriptors stable. */`;
}
