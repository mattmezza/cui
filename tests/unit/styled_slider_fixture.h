/* SPDX-License-Identifier: MIT */
#ifndef CUI_STYLED_SLIDER_TEST_FIXTURE_H
#define CUI_STYLED_SLIDER_TEST_FIXTURE_H
/* SPDX-License-Identifier: MIT */
#ifdef CUI_STYLED_SLIDER_DISTRIBUTION
#define CUI_IMPLEMENTATION
#define CUI_STYLED_SLIDER_MODEL_IMPLEMENTATION
#ifdef CUI_STYLED_SLIDER_UMBRELLA
#include "cui_all.h"
#else
#include "cui_styled_slider_model.h"
#endif
#else
#include "cui/cui_styled_slider_model.h"
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_slider_model_state_v1 model;
    cui_styled_slider_model_state_v1 motion;
    cui_styled_slider_model_options_v1 style;
    cui_slider_model_options_v1 options;
    cui_slider_model_value_v1 value;
    cui_slider_model_request_v1 requests[64];
    size_t count;
    bool accept, invalidate, handler;
    float length, opacity;
    bool clip, inert, layout_only;
} machine;
static inline cui_id part(unsigned p) {
    return cui_base_slider_model_part_id_v1(100, p);
}
static inline cui_id decor(unsigned t, unsigned p) {
    return cui_styled_slider_model_part_id_v1(100, t, p);
}
static inline void request(void *user, const cui_slider_model_request_v1 *r) {
    machine *m = user;
    assert(m->count < 64);
    m->requests[m->count++] = *r;
    if (m->accept && r->phase == CUI_SLIDER_MODEL_UPDATE)
        m->value = r->requested;
    if (m->invalidate) {
        cui_report_diagnostic(m->ui, &(cui_diagnostic){.error = CUI_ERROR_LAYOUT});
        cui_clear_error(m->ui);
    }
}
static inline machine *create(void) {
    machine *m = calloc(1, sizeof *m);
    assert(m);
    m->ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    assert(m->ui);
    assert(cui_slider_model_state_init_v1(sizeof m->model, &m->model));
    assert(cui_styled_slider_model_state_init_v1(sizeof m->motion, &m->motion));
    cui_theme t = cui_theme_light();
    assert(cui_styled_slider_model_options_from_theme_v1(&t, 16, CUI_SLIDER_MODEL_HORIZONTAL,
                                                         &m->style));
    m->options = (cui_slider_model_options_v1){.version = 1,
                                               .struct_size = sizeof m->options,
                                               .domain = {0, 1, .125, CUI_SLIDER_MODEL_LINEAR, 0},
                                               .name = CUI_STR("Selection"),
                                               .start_name = CUI_STR("Start"),
                                               .end_name = CUI_STR("End")};
    m->value =
        (cui_slider_model_value_v1){.kind = CUI_SLIDER_MODEL_RANGE, .start = .25, .end = .75};
    m->handler = true;
    m->length = 200;
    m->opacity = 1;
    return m;
}
static inline void destroy(machine *m) {
    cui_destroy(m->ui);
    free(m);
}
static inline cui_styled_slider_model_result_v1 submit(machine *m) {
    float cross = m->style.style.rem * 1.5f;
    bool vertical = m->options.orientation == CUI_SLIDER_MODEL_VERTICAL;
    cui_begin_box(m->ui, 900,
                  (cui_box){.width = vertical ? cross : m->length,
                            .height = vertical ? m->length : cross,
                            .fixed_width = true,
                            .fixed_height = true,
                            .clip = m->clip,
                            .custom_opacity = true,
                            .opacity = m->opacity,
                            .inert = m->inert,
                            .layout_only = m->layout_only});
    cui_styled_slider_model_result_v1 r = cui_styled_slider_model_events_v1(
        m->ui, 100, m->value, &m->model, &m->options, &m->motion, &m->style,
        (cui_slider_model_handler_v1){m->handler ? request : NULL, m});
    cui_end_box(m->ui);
    return r;
}
static inline cui_styled_slider_model_result_v1 frame(machine *m, float dt, cui_draw_list *paint) {
    assert(cui_begin_frame(m->ui, 400, 300, dt));
    cui_styled_slider_model_result_v1 r = submit(m);
    cui_draw_list p = cui_end_frame(m->ui);
    if (paint)
        *paint = p;
    return r;
}
static inline void pointer(machine *m, cui_event_type type, float x, float y, bool touch) {
    assert(cui_queue_event(
        m->ui, (cui_event){.type = type,
                           .x = x,
                           .y = y,
                           .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                           .pointer_id = touch ? 7 : 0,
                           .pointer_device = touch ? 3 : 0}));
}
static inline void over(machine *m, unsigned thumb) {
    cui_rect b = cui_bounds(m->ui, part(CUI_SLIDER_MODEL_START_THUMB + thumb));
    bool vertical = m->options.orientation == CUI_SLIDER_MODEL_VERTICAL;
    float xmax = m->clip ? (vertical ? 1.5f * m->style.style.rem : m->length) : 400;
    float ymax = m->clip ? (vertical ? m->length : 1.5f * m->style.style.rem) : 300;
    pointer(m, CUI_EVENT_POINTER_MOVE, fminf(xmax - .5f, fmaxf(.5f, b.x + b.w * .5f)),
            fminf(ymax - .5f, fmaxf(.5f, b.y + b.h * .5f)), false);
}
static inline void key(machine *m, cui_key k) {
    assert(cui_queue_event(m->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
}
static inline void okay(cui_styled_slider_model_result_v1 r) {
    assert(r.render_status == CUI_STYLED_SLIDER_RENDER_OK && r.base.status == CUI_SLIDER_MODEL_OK);
}
#endif
