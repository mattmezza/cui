/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include "cui/cui_plot_view.h"
#include <math.h>
static cui_context *ui;
static cui_draw_list draw;
static cui_prepaint_info snapshot;
static unsigned frames, callbacks, wheel_events;
static float scroll;
static bool overlay, modal;
static unsigned char chart_pixels[128 * 96 * 4], plot_pixels[64 * 32 * 4];
static cui_chart_state chart;
static cui_plot_element_state plot;
static cui_chart_datum chart_data[] = {{.label = {"One", 3}, .value = 20},
                                       {.label = {"Two", 3}, .value = 40}};
static cui_chart_options options;
static cui_theme theme;
static cui_plot_result resolve(cui_plot_point point, cui_plot_rect bounds,
                               cui_plot_tooltip_state *out, void *user) {
    (void)bounds;
    (void)user;
    *out = (cui_plot_tooltip_state){.cross_line = point};
    return CUI_PLOT_OK;
}
static bool paint(const cui_prepaint_info *info, cui_prepaint_writer *writer, const cui_theme *t,
                  const cui_plot_hover_memory *hover, void *user) {
    (void)t;
    (void)user;
    snapshot = *info;
    callbacks++;
    cui_draw_command c = {.type = CUI_DRAW_RECT,
                          .bounds = info->bounds,
                          .color =
                              hover ? (cui_color){0, 180, 0, 255} : (cui_color){0, 0, 180, 255}};
    return cui_prepaint_emit(writer, &c, false);
}
static bool tip(const cui_prepaint_info *info, cui_prepaint_writer *writer, const cui_theme *t,
                const cui_plot_hover_memory *hover, void *user) {
    (void)t;
    (void)hover;
    (void)user;
    cui_draw_command c = {
        .type = CUI_DRAW_RECT, .bounds = {150, 120, 40, 20}, .color = {240, 200, 0, 255}};
    (void)info;
    return cui_prepaint_emit(writer, &c, true);
}
int probe_init(void) {
    ui = cui_create((cui_config){.max_elements = 128, .max_commands = 1024, .frame_bytes = 65536});
    if (!ui)
        return 1;
    theme = cui_theme_light();
    chart = (cui_chart_state){
        .pixels = chart_pixels, .byte_capacity = sizeof chart_pixels, .hovered = SIZE_MAX};
    options =
        cui_chart_theme((cui_chart_options){.kind = CUI_CHART_LINE,
                                            .data = chart_data,
                                            .count = 2,
                                            .width = 128,
                                            .height = 96,
                                            .hide_axes = true,
                                            .disable_appear = true,
                                            .reduced_motion = true,
                                            .interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME},
                        &theme);
    cui_set_reduced_motion(ui, true);
    return 0;
}
int probe_event(int wheel, float x, float y, float delta) {
    return cui_queue_event(ui,
                           (cui_event){.type = wheel ? CUI_EVENT_SCROLL : CUI_EVENT_POINTER_MOVE,
                                       .x = x,
                                       .y = y,
                                       .dy = -delta,
                                       .scroll_precise = true});
}
void probe_options(int passively_occluded, int modal_occluded) {
    overlay = passively_occluded != 0;
    modal = modal_occluded != 0;
}
int probe_frame(float width, float height) {
    if (!cui_begin_frame(ui, width, height, .016f))
        return 1;
    size_t count = 0;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (events[i].type == CUI_EVENT_SCROLL && cui_event_target(ui, i) == 10) {
            scroll = fminf(100, fmaxf(0, scroll - events[i].dy));
            wheel_events++;
            cui_consume_event(ui, i);
        }
    cui_begin_box(
        ui, 1,
        (cui_box){
            .width = width, .height = height, .background = {255, 255, 255, 255}, .padding = 10});
    cui_begin_box(
        ui, 10,
        (cui_box){
            .width = width / 2, .height = 100, .clip = true, .background = {220, 220, 220, 255}});
    cui_interact(ui, 10, CUI_ROLE_GROUP, cui_string("Scroll viewport"), false, false, 0);
    cui_set_semantic_details(
        ui, 10,
        (cui_semantic_details){.scrollable = true, .scroll_y = scroll, .scroll_max_y = 100});
    cui_begin_absolute_box(ui, 20, (cui_box){0}, (cui_rect){0, 10 - scroll, 128, 96});
    if (!cui_chart(ui, 21, &chart, &options, cui_string("Chart")))
        return 2;
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_absolute_box(ui, 50, (cui_box){.clip = true}, (cui_rect){10, 120, 100, 32});
    cui_plot_view_options view = {.bitmap = {plot_pixels, 64, 32, 256, 1}, .interactive = true};
    cui_plot_element_options element = {.interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME,
                                        .resolve = resolve,
                                        .paint_marks = paint,
                                        .paint_tooltip = tip,
                                        .paint_command_budget = 2};
    if (cui_plot_element(ui, 51, &theme, &view, &plot, &element) != CUI_PLOT_OK)
        return 3;
    cui_end_box(ui);
    cui_end_box(ui);
    if (overlay) {
        cui_begin_portal(ui, 30, (cui_box){.background = {200, 0, 0, 255}},
                         (cui_rect){20, 20, 40, 40}, 10);
        cui_set_hover_hitbox(ui, true);
        cui_end_box(ui);
    }
    if (modal) {
        cui_begin_focus_scope(ui, 40, true);
        cui_begin_portal(ui, 41, (cui_box){.background = {180, 0, 180, 255}},
                         (cui_rect){180, 100, 40, 40}, 20);
        cui_set_hover_hitbox(ui, true);
        cui_end_box(ui);
        cui_end_focus_scope(ui);
    }
    draw = cui_end_frame(ui);
    frames++;
    return cui_last_error(ui);
}
double probe_stat(unsigned field) {
    switch (field) {
    case 0:
        return frames;
    case 1:
        return callbacks;
    case 2:
        return snapshot.pointer_valid;
    case 3:
        return snapshot.pointer_visible;
    case 4:
        return snapshot.pointer_hit;
    case 5:
        return snapshot.bounds.y;
    case 6:
        return snapshot.visible_bounds.w;
    case 7:
        return snapshot.modal_allowed;
    case 8:
        return wheel_events;
    case 9:
        return scroll;
    case 10:
        return chart.hovered == SIZE_MAX ? -1 : (double)chart.hovered;
    case 11:
        return chart.bounds.x;
    case 12:
        return chart.bounds.y;
    case 13:
        return chart.tooltip_cursor_y;
    default:
        return -1;
    }
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
