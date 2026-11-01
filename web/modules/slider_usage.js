/* SPDX-License-Identifier: MIT */
// Complete callback + initialization + composition; no borrowed request retained.
export function sliderUsage(kind=0,vertical=0,reverse=0,logarithmic=0,accept=1,disabled=false,shadows=false) {
 return `#include "cui/cui_styled_slider_model.h"
#include <math.h>

typedef struct {
    cui_slider_model_value_v1 confirmed;
    cui_slider_model_state_v1 model;
    cui_styled_slider_model_state_v1 presentation;
    cui_slider_model_request_v1 last_request;
    unsigned accepted;
    bool accept, has_request;
    double next_wake_seconds;
} slider_application;

static void slider_request(void *user, const cui_slider_model_request_v1 *request) {
    slider_application *app = user;
    app->last_request = *request; /* Owned copy; callback argument is borrowed. */
    app->has_request = true;
    if (app->accept && request->phase == CUI_SLIDER_MODEL_UPDATE &&
        cui_slider_model_value_valid_v1(request->domain, request->requested)) {
        app->confirmed = request->requested; /* Explicit complete-pair acceptance. */
        app->accepted++;
    }
    /* Reject by doing nothing. CANCEL may carry an old kind/domain. */
}

bool slider_initialize(slider_application *app) {
    if (!app) return false;
    *app = (slider_application){.confirmed = {${kind?'CUI_SLIDER_MODEL_RANGE':'CUI_SLIDER_MODEL_SINGLE'}, 0, ${kind?25:0}, ${kind?75:40}},
                                .accept = ${accept?'true':'false'}};
    return cui_slider_model_state_init_v1(sizeof app->model, &app->model) &&
        cui_styled_slider_model_state_init_v1(sizeof app->presentation, &app->presentation);
}

/* Call inside an admitted ordinary frame. Render after end_frame, then wait
 * until next_wake_seconds in absolute cui_frame_time units (zero means idle).
 * Changing mode/domain should explicitly synchronize the owned value/state.
 * This V1 example rejects actual visible gradient paint; no V2 host is claimed. */
bool slider_compose(cui_context *ui, const cui_theme *theme, slider_application *app,
                    float width, bool reduced_motion) {
    cui_styled_slider_model_options_v1 paint;
    if (!cui_styled_slider_model_options_from_theme_v1(theme, 16,
        ${vertical?'CUI_SLIDER_MODEL_VERTICAL':'CUI_SLIDER_MODEL_HORIZONTAL'}, &paint)) return false;
    paint.style.layout = CUI_SLIDER_STYLE_PARENT;
    if (reduced_motion) paint.flags |= CUI_STYLED_SLIDER_REDUCED_MOTION;
    cui_slider_model_options_v1 options = {
        .version = CUI_SLIDER_MODEL_V1, .struct_size = sizeof options,
        .flags = ${disabled?'CUI_SLIDER_MODEL_DISABLED_BIT':'0'}${reverse?' | CUI_SLIDER_MODEL_REVERSE_FILL_BIT':''},
        .orientation = ${vertical?'CUI_SLIDER_MODEL_VERTICAL':'CUI_SLIDER_MODEL_HORIZONTAL'},
        .domain = {${logarithmic?1:0}, 100, 1, ${logarithmic?'CUI_SLIDER_MODEL_LOGARITHMIC':'CUI_SLIDER_MODEL_LINEAR'}, 0},
        .name = cui_string("Selection"), .start_name = cui_string("Selection start"),
        .end_name = cui_string("Selection end")};
    cui_id owner = cui_id_from(ui, "controlled-slider");
    cui_begin_box(ui, cui_id_from(ui, "slider-area"),
        (cui_box){.width = ${vertical?'48':'fmaxf(1, width)'}, .height = ${vertical?'180':'40'},
                  .fixed_width = true, .fixed_height = true});
    (void)width;
    cui_styled_slider_model_result_v1 result = cui_styled_slider_model_events_v1(
        ui, owner, app->confirmed, &app->model, &options, &app->presentation, &paint,
        (cui_slider_model_handler_v1){slider_request, app});
${shadows?`    if (result.render_status == CUI_STYLED_SLIDER_RENDER_OK &&
        !(options.flags & CUI_SLIDER_MODEL_DISABLED_BIT)) {
        cui_slider_style_plan_v1 plan;
        if (cui_slider_style_resolve_v1(&paint.style, &plan)) {
            cui_box_shadow shadow = {.kind = CUI_BOX_SHADOW_INSET,
                .layer = {.offset_y = 1, .sigma = 1, .color = {20, 40, 80, 150}}};
            float radius = fminf(plan.full_radius, plan.track_extent / 2);
            bool applied = cui_element_set_box_shadows(ui,
                cui_base_slider_model_part_id_v1(owner, CUI_SLIDER_MODEL_TRACK), &shadow, 1,
                (cui_corner_radii){radius, radius, radius, radius});
            (void)applied; /* ID getter alone never proves painted presence. */
        }
    }
`:''}    cui_end_box(ui);
    app->next_wake_seconds = result.next_wake_seconds;
    return result.render_status == CUI_STYLED_SLIDER_RENDER_OK;
}
`;
}
