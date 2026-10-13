/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include "cui/cui_plot_view.h"
#include <math.h>
static cui_context *ui;
static cui_draw_list draw;
static cui_theme theme;
static unsigned char chart_pixels[2][200 * 100 * 4], plot_pixels[64 * 64 * 4];
static cui_chart_state charts[2];
static cui_chart_options chart_options[2];
static cui_plot_element_state plots[2];
static cui_plot_motion_pointer_state pointers[2];
static cui_plot_element_options plot_options[2];
static cui_plot_view_options view;
static float pointer_x, pointer_y;
static unsigned serial;
static cui_chart_datum data[] = {{.label = {"One", 3}, .value = 20},
                                 {.label = {"Two", 3}, .value = 40}};
static cui_plot_result resolve(cui_plot_point p, cui_plot_rect b, cui_plot_tooltip_state *out,
                               void *user) {
    (void)p;
    (void)b;
    (void)user;
    *out = (cui_plot_tooltip_state){.index = 0};
    return CUI_PLOT_OK;
}
static void marks(cui_context *context, cui_id owner, const cui_theme *t,
                  const cui_plot_hover_memory *hover, cui_plot_rect bounds, void *user) {
    (void)t;
    (void)bounds;
    unsigned i = (unsigned)(uintptr_t)user;
    if (!hover || !hover->has_state)
        return;
    cui_plot_spring_value value;
    double target = pointer_x - 220 * i > 45 ? 50 : 10;
    if (cui_plot_motion_follow(context, owner, &pointers[i], plot_options[i].motion.pointer, target,
                               hover->entering, &value) != CUI_PLOT_OK)
        return;
    cui_begin_absolute_box(
        context, owner + 100,
        (cui_box){.background = {255, 0, 0, (unsigned char)lround(255 * hover->progress)}},
        (cui_rect){(float)value.value, 10, 6, 10});
    cui_end_box(context);
}
int probe_init(void) {
    ui = cui_create((cui_config){.width = 460, .height = 220});
    if (!ui)
        return 0;
    theme = cui_theme_light();
    cui_theme_motion_tokens tokens = cui_theme_motion_default();
    cui_plot_motion_policy policy;
    if (cui_plot_motion_from_theme(&tokens, &policy) != CUI_PLOT_OK)
        return 0;
    view = (cui_plot_view_options){.bitmap = {plot_pixels, 64, 64, 256, 1}, .interactive = true};
    for (unsigned i = 0; i < 2; i++) {
        plot_options[i] = (cui_plot_element_options){.bounds = {10 + 220 * i, 10, 74 + 220 * i, 74},
                                                     .resolve = resolve,
                                                     .marks = marks,
                                                     .user = (void *)(uintptr_t)i,
                                                     .custom_motion = true,
                                                     .motion = policy};
        charts[i] = (cui_chart_state){.pixels = chart_pixels[i],
                                      .byte_capacity = sizeof chart_pixels[i],
                                      .hovered = SIZE_MAX};
        chart_options[i] = cui_chart_theme((cui_chart_options){.kind = CUI_CHART_LINE,
                                                               .data = data,
                                                               .count = 2,
                                                               .width = 200,
                                                               .height = 100,
                                                               .hide_x_axis = true,
                                                               .custom_motion = true,
                                                               .motion = policy},
                                           &theme);
    }
    plot_options[1].motion.enter.duration = .8;
    plot_options[1].motion.enter.easing = CUI_PLOT_MOTION_LINEAR;
    plot_options[1].motion.pointer.response = .8;
    chart_options[1].motion.appear.duration = 2;
    return 1;
}
void probe_input(int type, float x, float y) {
    pointer_x = x;
    pointer_y = y;
    (void)cui_queue_event(ui, (cui_event){.type = (cui_event_type)type, .x = x, .y = y});
}
int probe_frame(float dt, int reduced) {
    cui_set_reduced_motion(ui, reduced != 0);
    if (!cui_begin_frame(ui, 460, 220, dt))
        return 0;
    cui_begin_box(
        ui, 1, (cui_box){.width = 460, .height = 220, .fixed_width = true, .fixed_height = true});
    for (unsigned i = 0; i < 2; i++) {
        cui_begin_absolute_box(ui, 20 + i, (cui_box){0}, (cui_rect){10 + 220 * i, 10, 64, 64});
        plot_options[i].pointer = (cui_plot_point){pointer_x, pointer_y};
        /* This fixture owns fixed bounds and the live pointer gate; it does
         * not claim automatic prepaint. */
        plot_options[i].hovered = pointer_y < 80;
        plot_options[i].frame_serial = serial;
        if (cui_plot_element(ui, 40 + i, &theme, &view, &plots[i], &plot_options[i]) != CUI_PLOT_OK)
            return 0;
        cui_end_box(ui);
        cui_begin_absolute_box(ui, 60 + i, (cui_box){0}, (cui_rect){10 + 220 * i, 100, 200, 100});
        if (!cui_chart(ui, 80 + i, &charts[i], &chart_options[i], cui_string("Owner chart")))
            return 0;
        cui_end_box(ui);
    }
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    serial++;
    return cui_last_error(ui) == CUI_OK;
}
unsigned probe_count(void) {
    return (unsigned)draw.count;
}
float probe_command(unsigned i, unsigned field) {
    if (i >= draw.count)
        return 0;
    const cui_draw_command *c = &draw.commands[i];
    switch (field) {
    case 0:
        return c->type;
    case 1:
        return c->bounds.x;
    case 2:
        return c->bounds.y;
    case 3:
        return c->bounds.w;
    case 4:
        return c->bounds.h;
    case 5:
        return c->color.r;
    case 6:
        return c->color.g;
    case 7:
        return c->color.b;
    case 8:
        return c->color.a;
    case 9:
        return c->font_size;
    default:
        return 0;
    }
}
uintptr_t probe_text(unsigned i) {
    return i < draw.count ? (uintptr_t)draw.commands[i].text.data : 0;
}
unsigned probe_length(unsigned i) {
    return i < draw.count ? (unsigned)draw.commands[i].text.length : 0;
}
uintptr_t probe_pixels(unsigned i) {
    return i < draw.count ? (uintptr_t)draw.commands[i].bitmap.bitmap.pixels : 0;
}
unsigned probe_bitmap(unsigned i, unsigned field) {
    if (i >= draw.count)
        return 0;
    const cui_bitmap *b = &draw.commands[i].bitmap.bitmap;
    return field == 0 ? b->width : field == 1 ? b->height : (unsigned)b->stride;
}
float probe_appearance(unsigned i) {
    return i < 2 ? charts[i].rendered_reveal_progress : 0;
}
