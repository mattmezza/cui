/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include "cui/cui_plot_view.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_plot_motion_policy source_policy(void) {
    cui_theme_motion_tokens t = cui_theme_motion_default();
    cui_plot_motion_policy p;
    assert(cui_plot_motion_from_theme(&t, &p) == CUI_PLOT_OK);
    return p;
}
static void factory(void) {
    cui_plot_motion_policy p = source_policy(), copy = p;
    assert(fabs(p.enter.duration - .12) < 1e-7 && p.exit.duration == p.enter.duration);
    assert(p.pointer.response == p.enter.duration && p.pointer.damping == 1 &&
           p.pointer.epsilon == .1 && p.pointer.travel);
    assert(p.appear.duration == 1 && p.appear.easing == CUI_PLOT_MOTION_QUARTIC_OUT);
    double value;
    assert(cui_plot_motion_sample(&copy.appear, .5, &value) == CUI_PLOT_OK && value == .9375);
    assert(cui_plot_motion_sample(&p.enter, .5, &value) == CUI_PLOT_OK && value > .9);
    assert(cui_plot_motion_sample(&p.exit, .5, &value) == CUI_PLOT_OK && value < .5);
    cui_theme_motion_tokens t = cui_theme_motion_default();
    t.duration_fast = .4f;
    t.spring_move.response = .9f;
    t.easing_move.x1 = .7f;
    assert(cui_plot_motion_from_theme(&t, &p) == CUI_PLOT_OK);
    assert(p.appear.duration == 1 && fabs(p.pointer.response - .4) < 1e-7);
    t.duration_fast = NAN;
    assert(cui_plot_motion_from_theme(&t, &p) == CUI_PLOT_INVALID);
    assert(fabs(p.pointer.response - .4) < 1e-7 && p.appear.duration == 1);
    p.enter.easing = (cui_plot_motion_easing)99;
    value = 42;
    assert(!cui_plot_motion_valid(&p));
    assert(cui_plot_motion_sample(&p.enter, .5, &value) == CUI_PLOT_INVALID && value == 42);
}
typedef struct {
    cui_plot_motion_pointer_state pointer;
    cui_plot_motion_policy policy;
    double target, painted;
    unsigned calls;
} marks_probe;
static cui_plot_result resolve(cui_plot_point p, cui_plot_rect b, cui_plot_tooltip_state *out,
                               void *u) {
    (void)p;
    (void)b;
    (void)u;
    *out = (cui_plot_tooltip_state){.index = 0};
    return CUI_PLOT_OK;
}
static void marks(cui_context *ui, cui_id owner, const cui_theme *theme,
                  const cui_plot_hover_memory *hover, cui_plot_rect bounds, void *u) {
    (void)theme;
    (void)bounds;
    marks_probe *p = u;
    if (!hover || !hover->has_state)
        return;
    cui_plot_spring_value value;
    assert(cui_plot_motion_follow(ui, owner, &p->pointer, p->policy.pointer, p->target,
                                  hover->entering, &value) == CUI_PLOT_OK);
    p->painted = value.value;
    p->calls++;
    cui_begin_absolute_box(
        ui, owner + 100,
        (cui_box){.background = {230, 0, 0, (unsigned char)lround(255 * hover->progress)}},
        (cui_rect){(float)value.value, 5, 4, 4});
    cui_end_box(ui);
}
static void appear_callback(cui_id owner, const cui_plot_appear_value *value, void *user) {
    (void)owner;
    (void)user;
    double sample;
    assert(cui_plot_appear_value_stagger(value, 0, 1, .25, &sample) == CUI_PLOT_OK);
    assert(fabs(sample - value->progress) < 1e-6);
}
static void plots(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    assert(ui);
    unsigned char pixels[64 * 64 * 4] = {0};
    cui_plot_view_options view = {.bitmap = {pixels, 64, 64, 256, 1}, .interactive = true};
    cui_theme theme = cui_theme_light();
    marks_probe probes[2] = {{.policy = source_policy(), .target = 10},
                             {.policy = source_policy(), .target = 10}};
    probes[1].policy.enter.duration = .8;
    probes[1].policy.enter.easing = CUI_PLOT_MOTION_LINEAR;
    probes[1].policy.exit.duration = .8;
    probes[1].policy.pointer.response = .8;
    probes[1].policy.appear.duration = 2;
    cui_plot_element_state states[2] = {0};
    cui_plot_element_options o[2] = {0};
    for (int i = 0; i < 2; i++)
        o[i] = (cui_plot_element_options){.bounds = {0, 0, 64, 64},
                                          .pointer = {20, 20},
                                          .resolve = resolve,
                                          .marks = marks,
                                          .user = &probes[i],
                                          .custom_motion = true,
                                          .motion = probes[i].policy,
                                          .has_appearance = true,
                                          .on_appear = appear_callback};
    for (unsigned frame = 0; frame < 3; frame++) {
        assert(cui_begin_frame(ui, 200, 100, frame ? .5f : 0));
        for (int i = 0; i < 2; i++) {
            o[i].frame_serial = frame;
            assert(cui_plot_element(ui, 10 + i, &theme, &view, &states[i], &o[i]) == CUI_PLOT_OK);
        }
        (void)cui_end_frame(ui);
    }
    assert(states[0].appearance.time == 1 && states[1].appearance.time == .5);
    assert(states[1].appearance.progress == .9375);
    assert(states[1].appearance.easing == NULL && states[1].appearance.user == NULL);
    cui_plot_element_state copied_state = states[1];
    cui_plot_element_options copied_options = o[1];
    double stagger;
    assert(cui_plot_motion_appear_stagger(&copied_options.motion.appear, &copied_state.appearance,
                                          0, 1, .25, &stagger) == CUI_PLOT_OK &&
           stagger == .9375);
    o[1] = (cui_plot_element_options){0};
    states[1] = (cui_plot_element_state){0};
    assert(cui_begin_frame(ui, 200, 100, .1f));
    copied_options.frame_serial++;
    assert(cui_plot_element(ui, 11, &theme, &view, &copied_state, &copied_options) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(copied_state.appearance.easing == NULL && copied_state.appearance.user == NULL);
    o[1] = copied_options;
    states[1] = copied_state;
    /* Disable appearance for hover channels; initial target is inactive. */
    for (int i = 0; i < 2; i++)
        o[i].has_appearance = false;
    assert(cui_begin_frame(ui, 200, 100, 0));
    for (int i = 0; i < 2; i++)
        assert(cui_plot_element(ui, 10 + i, &theme, &view, &states[i], &o[i]) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    for (int i = 0; i < 2; i++)
        o[i].hovered = true;
    assert(cui_begin_frame(ui, 200, 100, 0));
    for (int i = 0; i < 2; i++)
        assert(cui_plot_element(ui, 10 + i, &theme, &view, &states[i], &o[i]) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(probes[0].painted == 10 && probes[1].painted == 10);
    assert(cui_begin_frame(ui, 200, 100, .06f));
    for (int i = 0; i < 2; i++)
        assert(cui_plot_element(ui, 10 + i, &theme, &view, &states[i], &o[i]) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(states[0].hover.progress > .9 && states[1].hover.progress < .1);
    for (int i = 0; i < 2; i++)
        probes[i].target = 50;
    for (unsigned frame = 0; frame < 3; frame++) {
        assert(cui_begin_frame(ui, 200, 100, .06f));
        for (int i = 0; i < 2; i++)
            assert(cui_plot_element(ui, 10 + i, &theme, &view, &states[i], &o[i]) == CUI_PLOT_OK);
        (void)cui_end_frame(ui);
    }
    assert(probes[0].painted > probes[1].painted && probes[1].painted > 10);
    assert(probes[0].calls && probes[1].calls);
    double before = states[1].hover.progress;
    o[1].hovered = false;
    assert(cui_begin_frame(ui, 200, 100, .02f));
    assert(cui_plot_element(ui, 11, &theme, &view, &states[1], &o[1]) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(states[1].hover.has_state && states[1].hover.progress <= before);
    cui_plot_element_state saved = states[0];
    o[0].motion.pointer.response = NAN;
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(cui_plot_element(ui, 10, &theme, &view, &states[0], &o[0]) == CUI_PLOT_INVALID);
    assert(states[0].transition.started_at == saved.transition.started_at &&
           states[0].hover.progress == saved.hover.progress);
    (void)cui_end_frame(ui);
    o[0].motion = probes[0].policy;
    cui_plot_motion_pointer_state saved_pointer = probes[0].pointer;
    cui_plot_spring_value output = {.value = 99, .velocity = 88};
    cui_plot_spring_policy invalid = probes[0].policy.pointer;
    invalid.response = NAN;
    assert(cui_plot_motion_follow(ui, 10, &probes[0].pointer, invalid, 50, false, &output) ==
           CUI_PLOT_INVALID);
    assert(probes[0].pointer.clock.position == saved_pointer.clock.position && output.value == 99);
    cui_set_reduced_motion(ui, true);
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(cui_plot_element(ui, 10, &theme, &view, &states[0], &o[0]) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(states[0].hover.progress == 1 && probes[0].painted == 50 && !cui_needs_redraw(ui));
    cui_destroy(ui);
}
static void charts(void) {
    unsigned char pixels[2][128 * 96 * 4];
    cui_chart_datum data[] = {{.label = {"One", 3}, .value = 20},
                              {.label = {"Two", 3}, .value = 40}};
    cui_chart_state states[2] = {
        {.pixels = pixels[0], .byte_capacity = sizeof pixels[0], .hovered = SIZE_MAX},
        {.pixels = pixels[1], .byte_capacity = sizeof pixels[1], .hovered = SIZE_MAX}};
    cui_chart_options o[2];
    cui_theme theme = cui_theme_light();
    for (int i = 0; i < 2; i++)
        o[i] = cui_chart_theme((cui_chart_options){.kind = CUI_CHART_LINE,
                                                   .data = data,
                                                   .count = 2,
                                                   .width = 128,
                                                   .height = 96,
                                                   .custom_motion = true,
                                                   .motion = source_policy()},
                               &theme);
    o[1].motion.appear.duration = 2;
    o[1].motion.enter.duration = .8;
    o[1].motion.enter.easing = CUI_PLOT_MOTION_LINEAR;
    o[1].motion.pointer.response = .8;
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 200});
    assert(ui);
    for (int frame = 0; frame < 3; frame++) {
        assert(cui_begin_frame(ui, 300, 200, frame ? .5f : 0));
        for (int i = 0; i < 2; i++)
            assert(cui_chart(ui, 40 + i, &states[i], &o[i], cui_string("Motion")));
        (void)cui_end_frame(ui);
    }
    assert(states[0].rendered_reveal_progress == 1 && states[1].rendered_reveal_progress == .9375f);
    for (int kind = 0; kind < 3; kind++) {
        for (int i = 0; i < 2; i++) {
            o[i].kind = kind == 0 ? CUI_CHART_LINE : kind == 1 ? CUI_CHART_BAR : CUI_CHART_PIE;
            o[i].disable_appear = true;
        }
        assert(cui_begin_frame(ui, 300, 200, 0));
        for (int i = 0; i < 2; i++)
            assert(cui_chart(ui, 40 + i, &states[i], &o[i], cui_string("Motion")));
        (void)cui_end_frame(ui);
        for (int i = 0; i < 2; i++)
            assert(
                cui_chart_update_hover(&states[i], &o[i], kind == 2 ? 90 : 30, kind == 2 ? 35 : 30)
                    .valid);
        assert(cui_begin_frame(ui, 300, 200, 0));
        for (int i = 0; i < 2; i++)
            assert(cui_chart(ui, 40 + i, &states[i], &o[i], cui_string("Motion")));
        (void)cui_end_frame(ui);
        assert(cui_begin_frame(ui, 300, 200, .06f));
        for (int i = 0; i < 2; i++)
            assert(cui_chart(ui, 40 + i, &states[i], &o[i], cui_string("Motion")));
        (void)cui_end_frame(ui);
        assert(states[0].tooltip_focus > states[1].tooltip_focus);
        for (int i = 0; i < 2; i++)
            assert(
                cui_chart_update_hover(&states[i], &o[i], kind == 2 ? 42 : 95, kind == 2 ? 60 : 30)
                    .valid);
        cui_draw_list moving_draw = {0};
        for (int f = 0; f < 3; f++) {
            assert(cui_begin_frame(ui, 300, 200, .06f));
            for (int i = 0; i < 2; i++)
                assert(cui_chart(ui, 40 + i, &states[i], &o[i], cui_string("Motion")));
            moving_draw = cui_end_frame(ui);
        }
        if (kind == 0)
            assert(states[0].hover_x > states[1].hover_x);
        if (kind == 1)
            assert(states[0].emphasis_center > states[1].emphasis_center);
        if (kind == 2)
            assert(states[0].pie_lift[states[0].hovered] > states[1].pie_lift[states[1].hovered]);
        if (kind < 2) {
            float painted_x[2] = {NAN, NAN};
            for (size_t command = 0; command < moving_draw.count; command++) {
                const cui_draw_command *c = &moving_draw.commands[command];
                bool guide = kind == 0 ? c->type == CUI_DRAW_IMAGE && c->bitmap.bitmap.width == 1
                                       : c->type == CUI_DRAW_RECT && c->color.a > 0 &&
                                             c->color.a <= 25 && fabsf(c->bounds.h - 76) < .01f;
                if (!guide)
                    continue;
                for (int i = 0; i < 2; i++)
                    if (fabsf(c->bounds.y - states[i].bounds.y) < .01f)
                        painted_x[i] = c->bounds.x - states[i].bounds.x;
            }
            assert(isfinite(painted_x[0]) && isfinite(painted_x[1]) && painted_x[0] > painted_x[1]);
        } else {
            /* The actual pie rasters differ while the two owner lifts travel. */
            assert(memcmp(pixels[0], pixels[1], sizeof pixels[0]) != 0);
        }
        for (int f = 0; f < 40; f++) {
            assert(cui_begin_frame(ui, 300, 200, .1f));
            for (int i = 0; i < 2; i++)
                assert(cui_chart(ui, 40 + i, &states[i], &o[i], cui_string("Motion")));
            (void)cui_end_frame(ui);
        }
        assert(!cui_needs_redraw(ui));
    }
    uint64_t revision = states[0].revision;
    float focus = states[0].tooltip_focus;
    unsigned char saved[sizeof pixels[0]];
    memcpy(saved, pixels[0], sizeof saved);
    o[0].motion.appear.duration = NAN;
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(!cui_chart(ui, 40, &states[0], &o[0], cui_string("Motion")));
    assert(cui_chart_render_animated(ui, &states[0], &o[0]) == CUI_PLOT_INVALID);
    (void)cui_end_frame(ui);
    assert(states[0].revision == revision && states[0].tooltip_focus == focus &&
           memcmp(saved, pixels[0], sizeof saved) == 0);
    o[0].motion = source_policy();
    o[0].motion.enter.duration = 0;
    o[0].motion.pointer.response = 0;
    assert(cui_chart_update_hover(&states[0], &o[0], 90, 35).valid);
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(cui_chart(ui, 40, &states[0], &o[0], cui_string("Motion")));
    (void)cui_end_frame(ui);
    assert(states[0].tooltip_focus == 1);
    o[0].motion = source_policy();
    o[0].motion.appear.duration = 2;
    o[0].appear_duration = .2f;
    o[0].disable_appear = false;
    o[0].appear_key++;
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(cui_chart_render_animated(ui, &states[0], &o[0]) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(cui_begin_frame(ui, 300, 200, .1f));
    assert(cui_chart_render_animated(ui, &states[0], &o[0]) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(fabs(states[0].rendered_reveal_progress - .9375) < 1e-6);
    o[0].appear_duration = 0;
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(cui_chart_render_animated(ui, &states[0], &o[0]) == CUI_PLOT_OK);
    float prior_reveal = states[0].rendered_reveal_progress;
    o[0].motion.appear.duration = 4;
    assert(cui_chart(ui, 40, &states[0], &o[0], cui_string("Motion")));
    (void)cui_end_frame(ui);
    assert(states[0].rendered_reveal_progress < prior_reveal);
    o[0].reduced_motion = true;
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(cui_chart(ui, 40, &states[0], &o[0], cui_string("Motion")));
    (void)cui_end_frame(ui);
    assert(states[0].rendered_reveal_progress == 1);
    assert(cui_chart_update_hover(&states[0], &o[0], 90, 35).valid);
    assert(cui_begin_frame(ui, 300, 200, 0));
    assert(cui_chart(ui, 40, &states[0], &o[0], cui_string("Motion")));
    (void)cui_end_frame(ui);
    assert(states[0].tooltip_focus == 1 && states[0].pie_lift[states[0].hovered] == 1);
    cui_destroy(ui);
}
int main(void) {
    factory();
    plots();
    charts();
    return 0;
}
