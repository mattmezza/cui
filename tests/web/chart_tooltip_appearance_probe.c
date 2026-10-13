/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include "cui/cui_effects.h"
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
    if (!cui_chart_tooltip_set_title(b, cui_string("Totals")))
        return false;
    for (unsigned i = 0; i < 2; i++) {
        char label[16], value[16];
        int ln = snprintf(label, sizeof label, "%s", i ? "Total" : "Series");
        int vn = snprintf(value, sizeof value, "%s", i ? "1000" : "20");
        cui_chart_tooltip_row row = {.label = {label, (size_t)ln},
                                     .value = {value, (size_t)vn},
                                     .has_swatch = i == 0,
                                     .swatch = {220, 10, 20, 255},
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
    cui_theme theme = {.radius = 10,
                       .popover = {240, 230, 210, 255},
                       .popover_foreground = {10, 30, 70, 255},
                       .muted = {100, 40, 120, 255},
                       .text = {220, 20, 50, 255}};
    cui_chart_tooltip_style style;
    if (!cui_chart_tooltip_style_project(&theme, 16, 20, &style))
        return 8;
    style.appearance = (mode & 1) ? CUI_CHART_TOOLTIP_BARE : CUI_CHART_TOOLTIP_BOXED;
    if (mode & 8)
        style.background.a = 128;
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
                           .interaction_mode = legacy ? CUI_PLOT_INTERACTION_LEGACY
                                                      : CUI_PLOT_INTERACTION_CURRENT_FRAME,
                           .tooltip_build = (mode & 2) ? NULL : build,
                           .tooltip_style = &style,
                           .background = {255, 255, 255, 255},
                           .foreground = {0, 0, 0, 255},
                           .grid = {120, 120, 120, 255},
                           .series = {{0, 120, 220, 255}}};
    if (mode & 16) {
        o.reduced_motion = false;
        o.hover_duration = 1000000;
        chart.tooltip_focus = .5f;
        chart.tooltip_from = .5f;
        chart.tooltip_phase = 0;
        chart.tooltip_active = false;
        chart.tooltip_started = true;
    }
    if (!cui_begin_frame(ui, 400, 260, .016f))
        return 1;
    cui_begin_box(ui, 3, (cui_box){.width = 400, .height = 260});
    if (mode & 4)
        cui_begin_box(ui, 4,
                      (cui_box){.width = 50,
                                .height = 50,
                                .fixed_width = true,
                                .fixed_height = true,
                                .clip = true,
                                .custom_opacity = true,
                                .opacity = .5f});
    cui_begin_absolute_box(ui, 2, (cui_box){0}, (cui_rect){left, 0, width, 96});
    bool ok = cui_chart(ui, 1, &chart, &o, cui_string("Chart"));
    memset(&style, 0xff, sizeof style);
    memset(&o, 0, sizeof o);
    cui_end_box(ui);
    if (mode & 4)
        cui_end_box(ui);
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    return ok ? cui_last_error(ui) : 4;
}
float probe_focus(void) {
    return chart.tooltip_focus;
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
    case 10:
        return c->custom_corners ? c->corner_radii.top_left : c->radius;
    case 11:
        return c->stroke;
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

double probe_shadow(unsigned index, unsigned field) {
    if (index >= draw.count || draw.commands[index].type != CUI_DRAW_DROP_SHADOW)
        return 0;
    cui_drop_shadow_paint s = draw.commands[index].shadow;
    const float v[] = {
        s.caster.x,       s.caster.y,        s.caster.w,          s.caster.h,
        s.radii.top_left, s.radii.top_right, s.radii.bottom_left, s.radii.bottom_right,
        s.layer.offset_x, s.layer.offset_y,  s.layer.sigma,       s.layer.spread,
        s.layer.color.r,  s.layer.color.g,   s.layer.color.b,     s.layer.color.a};
    return field < 16 ? v[field] : 0;
}

float probe_shadow_coverage(unsigned index, float x, float y) {
    if (index >= draw.count || draw.commands[index].type != CUI_DRAW_DROP_SHADOW)
        return 0;
    float alpha = 0;
    cui_effect_drop_shadow_sample(draw.commands[index].shadow, 1, 0, 0, x, y, &alpha);
    return alpha;
}
