/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <string.h>
static cui_context *ui;
static cui_draw_list draw;
static cui_chart_state chart;
static unsigned char pixels[128 * 128 * 4];
static cui_chart_datum data[] = {{.label = {"Top", 3}, .value = 20},
                                 {.label = {"Side", 4}, .value = 40},
                                 {.label = {"End", 3}, .value = 30}};
static cui_size measure(void *u, cui_str s, float f, cui_text_style style) {
    (void)u;
    (void)style;
    return (cui_size){s.length * f * .5f, f};
}
static cui_size custom_measure(cui_context *c, const cui_chart_datum *d, size_t i, void *u) {
    (void)c;
    (void)d;
    (void)i;
    (void)u;
    return (cui_size){20, 10};
}
static void custom_paint(cui_context *c, cui_id id, const cui_chart_datum *d, size_t i, void *u) {
    (void)id;
    (void)d;
    (void)i;
    (void)u;
    cui_text(c, cui_string("Label"), 10, (cui_color){0, 0, 0, 255});
}
static cui_str override_title(const cui_chart_datum *d, size_t i, void *u) {
    (void)d;
    (void)i;
    (void)u;
    return cui_string("Override");
}
int probe_init(void) {
    ui = cui_create((cui_config){.max_elements = 128,
                                 .max_commands = 1024,
                                 .frame_bytes = 65536,
                                 .measure_styled = measure});
    chart =
        (cui_chart_state){.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    if (!ui)
        return 1;
    return !cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 64, .y = 20});
}
int probe_frame(int source, int override) {
    cui_chart_options o = {.kind = CUI_CHART_RADAR,
                           .data = data,
                           .count = 3,
                           .width = 128,
                           .height = 128,
                           .disable_appear = true,
                           .reduced_motion = true,
                           .interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME,
                           .background = {255, 255, 255, 255},
                           .foreground = {0, 0, 0, 255},
                           .grid = {120, 120, 120, 255},
                           .series = {{0, 120, 220, 255}},
                           .radar_label_measure = custom_measure,
                           .radar_label_paint = custom_paint,
                           .radar_tooltip_title_mode =
                               source ? CUI_CHART_RADAR_TOOLTIP_TITLE_SOURCE_LABEL
                                      : CUI_CHART_RADAR_TOOLTIP_TITLE_LEGACY,
                           .tooltip_title = override ? override_title : NULL};
    if (!cui_begin_frame(ui, 240, 220, .016f))
        return 1;
    if (!cui_chart(ui, 1, &chart, &o, cui_string("Radar")))
        return 2;
    draw = cui_end_frame(ui);
    return cui_last_error(ui);
}
unsigned probe_count(void) {
    return (unsigned)draw.count;
}
double probe_command(unsigned index, unsigned field) {
    if (index >= draw.count)
        return -1;
    const cui_draw_command *c = &draw.commands[index];
    switch (field) {
    case 0:
        return c->type;
    case 9:
        return c->font_size;
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
    default:
        return -1;
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
int probe_hover(void) {
    return chart.hovered == SIZE_MAX ? -1 : (int)chart.hovered;
}
