/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <stdio.h>
#include <string.h>
static cui_context *ui;
static cui_draw_list draw;
static cui_chart_state chart;
static unsigned char pixels[128 * 96 * 4];
static cui_chart_datum data[] = {{.label = {"Default", 7}, .value = 20},
                                 {.label = {"Next", 4}, .value = 40}};
static unsigned mode, calls;
static cui_size measure(void *u, cui_str text, float font, cui_text_style style) {
    (void)u;
    (void)style;
    return (cui_size){text.length * font * .5f, font};
}
static bool build(const cui_chart_tooltip_info *info, cui_chart_tooltip_builder *b, void *u) {
    (void)u;
    calls++;
    if (info->index != 0 || info->values[0] != 20)
        return false;
    if (mode == 3) {
        for (unsigned i = 0; i < 33; i++)
            (void)cui_chart_tooltip_add_row(b, &(cui_chart_tooltip_row){0});
        return true;
    }
    if (!cui_chart_tooltip_set_title(b, cui_string("Totals")))
        return false;
    for (unsigned i = 0; i < 2; i++) {
        char label[16], value[16];
        int ln = snprintf(label, sizeof label, "%s", i ? "Total" : "Series");
        int vn = snprintf(value, sizeof value, "%s", i ? "1000" : "20");
        cui_chart_tooltip_row row = {.label = {label, (size_t)ln},
                                     .value = {value, (size_t)vn},
                                     .has_swatch = mode != 0 && i == 0,
                                     .swatch = {220, 10, 20, mode == 2 ? 0 : 255},
                                     .has_value_color = i == 1,
                                     .value_color = {0, 180, 0, 255}};
        if (!cui_chart_tooltip_add_row(b, &row))
            return false;
        memset(label, 'X', sizeof label);
        memset(value, 'Y', sizeof value);
    }
    return true;
}
int probe_init(void) {
    ui = cui_create((cui_config){.max_elements = 256,
                                 .max_commands = 1024,
                                 .frame_bytes = 65536,
                                 .measure_styled = measure});
    chart =
        (cui_chart_state){.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    if (!ui)
        return 1;
    return !cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 20, .y = 20});
}
int probe_frame(unsigned selected, int legacy, float left, float width) {
    mode = selected;
    cui_clear_error(ui);
    cui_chart_options o = {.kind = CUI_CHART_LINE,
                           .data = data,
                           .count = 2,
                           .width = 128,
                           .height = 96,
                           .logical_width = width,
                           .disable_appear = true,
                           .reduced_motion = true,
                           .hide_axes = true,
                           .hide_legend = true,
                           .interaction_mode =
                               legacy ? CUI_PLOT_INTERACTION_LEGACY : CUI_PLOT_INTERACTION_AUTO,
                           .tooltip_build = build,
                           .background = {255, 255, 255, 255},
                           .foreground = {0, 0, 0, 255},
                           .grid = {120, 120, 120, 255},
                           .series = {{0, 120, 220, 255}}};
    if (!cui_begin_frame(ui, 400, 260, .016f))
        return 1;
    cui_begin_box(ui, 3, (cui_box){.width = 400, .height = 260});
    cui_begin_absolute_box(ui, 2, (cui_box){0}, (cui_rect){left, 0, width, 96});
    bool ok = cui_chart(ui, 1, &chart, &o, cui_string("Chart"));
    cui_end_box(ui);
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    if (mode == 3)
        return legacy                                   ? !ok && cui_last_error(ui) == 0 ? 0 : 2
               : cui_last_error(ui) == CUI_ERROR_LAYOUT ? 0
                                                        : 3;
    return ok ? cui_last_error(ui) : 4;
}
unsigned probe_calls(void) {
    return calls;
}
float probe_bounds(void) {
    return chart.bounds.x;
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
