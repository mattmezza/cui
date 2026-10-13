/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static bool differs(const unsigned char *pixels, size_t count, cui_color background) {
    for (size_t i = 0; i < count; i++)
        if (pixels[i * 4] != background.r || pixels[i * 4 + 1] != background.g ||
            pixels[i * 4 + 2] != background.b)
            return true;
    return false;
}
static bool green_pixel(const unsigned char *pixels, size_t count) {
    for (size_t i = 0; i < count; i++)
        if (pixels[i * 4 + 1] > 150 && pixels[i * 4] < 100)
            return true;
    return false;
}
static unsigned first_red_row(const unsigned char *pixels, unsigned width, unsigned height) {
    for (unsigned y = 0; y < height; y++)
        for (unsigned x = 0; x < width; x++) {
            size_t i = ((size_t)y * width + x) * 4;
            if (pixels[i] > 200 && pixels[i + 1] < 50 && pixels[i + 2] < 50)
                return y;
        }
    return height;
}
static cui_color chart_bar_pattern(const cui_chart_datum *datum, size_t index, cui_plot_rect bar,
                                   cui_plot_rect chart, float x, float y, void *user) {
    size_t *calls = user;
    assert(datum && index < 3 && x >= bar.x_min && x < bar.x_max && y >= bar.y_min &&
           y < bar.y_max && chart.x_max == 128 && chart.y_max == 96);
    (*calls)++;
    return ((int)x & 1) ? (cui_color){0, 240, 0, 255} : (cui_color){0, 0, 0, 0};
}
static cui_color chart_region_pattern(size_t series, float x, float y, cui_color fallback,
                                      void *user) {
    size_t *calls = user;
    assert(series < 8 && x >= 0 && x < 128 && y >= 0 && y < 96 && fallback.a);
    (*calls)++;
    return ((int)x & 1) ? (cui_color){0, 230, 0, 255} : (cui_color){0, 0, 0, 0};
}
static double pie_outer_for_index(const cui_chart_datum *datum, size_t index, double default_radius,
                                  void *user) {
    (void)datum;
    (void)user;
    return index == 0 ? default_radius / 2 : default_radius;
}
static double pie_inner_for_index(const cui_chart_datum *datum, size_t index, double default_radius,
                                  void *user) {
    (void)datum;
    (void)user;
    return index == 0 ? 8 : default_radius;
}
static double pie_bad_radius(const cui_chart_datum *datum, size_t index, double default_radius,
                             void *user) {
    (void)datum;
    (void)index;
    (void)default_radius;
    (void)user;
    return NAN;
}
typedef struct {
    unsigned seen[3];
    double start[3], end[3], pad[3];
    bool invalid;
} pie_arc_calls;
static double pie_outer_for_arc(const cui_chart_pie_arc *arc, double default_radius, void *user) {
    pie_arc_calls *calls = user;
    assert(arc && arc->datum && arc->index < 3 && arc->value > 0);
    calls->seen[arc->index]++;
    calls->start[arc->index] = arc->start_angle;
    calls->end[arc->index] = arc->end_angle;
    calls->pad[arc->index] = arc->pad_angle;
    return calls->invalid && arc->index == 1 ? NAN
           : arc->index == 0                 ? default_radius / 2
                                             : default_radius;
}
static cui_color pie_green_line(const cui_chart_datum *datum, size_t index, void *user) {
    (void)datum;
    (void)user;
    return index == 0 ? (cui_color){0, 230, 0, 255} : (cui_color){0, 0, 0, 0};
}
#ifndef CUI_CHART_BASE_ONLY
typedef struct {
    unsigned measured, painted;
    bool invalid;
} radar_label_calls;
static cui_size measure_radar_label(cui_context *ui, const cui_chart_datum *datum, size_t index,
                                    void *user) {
    (void)ui;
    assert(datum && index < 3);
    radar_label_calls *calls = user;
    calls->measured++;
    return calls->invalid ? (cui_size){NAN, 11} : (cui_size){19, 11};
}
static void paint_radar_label(cui_context *ui, cui_id id, const cui_chart_datum *datum,
                              size_t index, void *user) {
    assert(ui && id && datum && index < 3);
    ((radar_label_calls *)user)->painted++;
    cui_text(ui, (cui_str){"R", 1}, 9, (cui_color){7, 199, 8, 255});
}
static size_t format_q(double value, char *buffer, size_t capacity, void *user) {
    (void)value;
    (void)user;
    if (capacity)
        buffer[0] = 'Q';
    return capacity ? 1 : 0;
}
typedef struct {
    size_t count;
    double first, last;
} axis_tick_capture;
static size_t capture_axis_tick(double value, char *buffer, size_t capacity, void *user) {
    axis_tick_capture *capture = user;
    assert(isfinite(value));
    if (!capture->count)
        capture->first = value;
    capture->last = value;
    capture->count++;
    if (capacity)
        buffer[0] = 'T';
    return capacity ? 1 : 0;
}
static size_t format_wide_tick(double value, char *buffer, size_t capacity, void *user) {
    (void)value;
    (void)user;
    const char text[] = "123456789";
    size_t count = capacity < sizeof text ? capacity : sizeof text - 1;
    memcpy(buffer, text, count);
    return count;
}
static cui_color bar_label_by_index(const cui_chart_datum *datum, size_t index, void *user) {
    (void)datum;
    (void)user;
    return index == 1 ? (cui_color){4, 211, 5, 255} : (cui_color){100, 20, 30, 255};
}
static size_t custom_sankey_lines(const cui_chart_datum *datum, size_t index, double raw,
                                  cui_chart_sankey_label *out, size_t capacity, void *user) {
    (void)datum;
    assert(index < 3 && raw >= 0 && capacity == 16);
    if (*(bool *)user)
        return capacity + 1;
    out[0] = (cui_chart_sankey_label){{"X", 1}, {0, 215, 0, 255}, 14};
    out[1] = (cui_chart_sankey_label){{"Y", 1}, {0, 0, 215, 255}, 9};
    return 2;
}
static cui_str tooltip_title_override(const cui_chart_datum *datum, size_t index, void *user) {
    (void)user;
    assert(datum && index == 0);
    return (cui_str){"Price", 5};
}
static size_t tooltip_value_override(const cui_chart_datum *datum, size_t index, size_t series,
                                     double value, char *buffer, size_t capacity, void *user) {
    (void)user;
    assert(datum && index == 0 && series < 4 && isfinite(value));
    if (capacity)
        buffer[0] = 'V';
    return capacity ? 1 : 0;
}
static cui_color tooltip_color_override(const cui_chart_datum *datum, size_t index, size_t series,
                                        double value, void *user) {
    (void)user;
    assert(datum && index == 0 && series < 4 && isfinite(value));
    return (cui_color){3, 210, 4, 255};
}
static cui_str tooltip_label_override(const cui_chart_datum *datum, size_t index, size_t series,
                                      void *user) {
    (void)user;
    assert(datum && index == 0 && series == 0);
    return (cui_str){"CustomName", 10};
}
static size_t pie_value_override(const cui_chart_datum *datum, size_t index, double value,
                                 double share_percent, char *buffer, size_t capacity, void *user) {
    (void)user;
    assert(datum && index == 0 && value == 20 && fabs(share_percent - 2000. / 90.) < .001 &&
           capacity >= 6);
    memcpy(buffer, "Share!", 6);
    return 6;
}
static void custom_tooltip(cui_context *ui, cui_id id, size_t index, const cui_chart_datum *datum,
                           void *user) {
    size_t *calls = user;
    assert(ui && id == 91 && index == 0 && datum->label.length == 1);
    (*calls)++;
    cui_begin_absolute_box(
        ui, id ^ UINT64_C(0xabcdef),
        (cui_box){.width = 72, .height = 16, .fixed_width = true, .fixed_height = true},
        (cui_rect){0, 0, 72, 16});
    cui_text(ui, (cui_str){"Custom", 6}, 11, (cui_color){10, 20, 30, 255});
    cui_end_box(ui);
}
static cui_size custom_tooltip_size = {220, 90};
static cui_size custom_tooltip_measure(cui_context *ui, size_t index, const cui_chart_datum *datum,
                                       void *user) {
    (void)user;
    assert(ui && index == 0 && datum->label.length == 1);
    return custom_tooltip_size;
}
typedef struct {
    size_t measured, painted;
    bool invalid, empty;
} sankey_content_calls;
static cui_size sankey_content_measure(cui_context *ui, const cui_chart_datum *datum, size_t index,
                                       double throughput, void *user) {
    sankey_content_calls *calls = user;
    assert(ui && datum && index < 3 && throughput >= 0);
    calls->measured++;
    return calls->invalid ? (cui_size){NAN, 40}
           : calls->empty ? (cui_size){0, 0}
                          : (cui_size){40, 40};
}
static void sankey_content_paint(cui_context *ui, cui_id id, const cui_chart_datum *datum,
                                 size_t index, double throughput, void *user) {
    sankey_content_calls *calls = user;
    assert(ui && id && datum && index < 3 && throughput >= 0);
    calls->painted++;
    for (size_t line = 0; line < 20; line++) {
        cui_begin_absolute_box(ui, id ^ (UINT64_C(0x349ace170000) + line),
                               (cui_box){.width = 3,
                                         .height = 2,
                                         .fixed_width = true,
                                         .fixed_height = true,
                                         .background = {40, 120, 200, 255}},
                               (cui_rect){0, (float)line * 2, 3, 2});
        cui_end_box(ui);
    }
}
#endif
static void continuous_curve_stroke(void) {
    cui_chart_datum data[] = {{.value = 20}, {.value = 40}, {.value = 30}};
    unsigned char pixels[128 * 96 * 4];
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels};
    cui_chart_options options = {.kind = CUI_CHART_LINE,
                                 .data = data,
                                 .count = 3,
                                 .width = 128,
                                 .height = 96,
                                 .hide_grid = true,
                                 .hide_axes = true,
                                 .hide_x_axis = true,
                                 .hide_legend = true,
                                 .background = {0, 0, 0, 255},
                                 .series = {{255, 0, 0, 255}},
                                 .data_revision = 1};
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    for (unsigned x = 4; x < 124; ++x) {
        unsigned ink = 0;
        for (unsigned y = 0; y < 96; ++y)
            ink += pixels[((size_t)y * 128 + x) * 4] > 80;
        assert(ink >= 2);
    }
    unsigned char default_pixels[sizeof pixels];
    memcpy(default_pixels, pixels, sizeof pixels);
    uint64_t revision = state.revision;
    options.line_width = 1;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK && state.revision == revision + 1);
    unsigned narrow = 0, wide = 0;
    for (size_t i = 0; i < sizeof pixels; i += 4)
        narrow += pixels[i] > 80;
    options.line_width = 4;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    for (size_t i = 0; i < sizeof pixels; i += 4)
        wide += pixels[i] > 80;
    assert(wide > narrow * 2);
    options.line_width = NAN;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(default_pixels, pixels, sizeof pixels) == 0);
    options.line_width = 16;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    memcpy(default_pixels, pixels, sizeof pixels);
    options.line_width = 100;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(default_pixels, pixels, sizeof pixels) == 0);
}
static void shared_sankey_adapter(void) {
    cui_chart_datum data[3] = {0};
    cui_chart_link input[] = {{.source = 0, .target = 1, .value = 9},
                              {.source = 1, .target = 2, .value = 4}};
    cui_chart_options o = {.kind = CUI_CHART_SANKEY,
                           .data = data,
                           .count = 3,
                           .links = input,
                           .link_count = 2,
                           .width = 100,
                           .height = 100};
    cui_chart_sankey_node nodes[3], defaults[3], saved[3];
    assert(cui_chart_sankey_layout(&o, nodes, 3) == CUI_PLOT_OK);
    assert(nodes[0].x0 == 0 && nodes[0].x1 == 10 && nodes[1].x0 == 45 && nodes[2].x0 == 90);
    assert(nodes[0].throughput == 9 && nodes[1].throughput == 9 && nodes[2].throughput == 4);
    memcpy(defaults, nodes, sizeof nodes);
    o.node_width = 10;
    o.node_padding = 16;
    o.sankey_iterations = 6;
    assert(cui_chart_sankey_layout(&o, nodes, 3) == CUI_PLOT_OK);
    for (size_t i = 0; i < 3; ++i) {
        assert(nodes[i].x0 == defaults[i].x0 && nodes[i].x1 == defaults[i].x1);
        assert(nodes[i].y0 == defaults[i].y0 && nodes[i].y1 == defaults[i].y1);
    }
    o.sankey_value_scale = CUI_CHART_SANKEY_SQRT;
    assert(cui_chart_sankey_layout(&o, nodes, 3) == CUI_PLOT_OK);
    assert(nodes[0].throughput == 9 && nodes[1].throughput == 9 && nodes[2].throughput == 4);
    assert(fabs((nodes[2].y1 - nodes[2].y0) / 100.0 - 2.0 / 3) < 1e-5);
    o.sankey_margins = (cui_chart_sankey_margins){5, 7, 3, 9};
    assert(cui_chart_sankey_layout(&o, nodes, 3) == CUI_PLOT_OK);
    assert(nodes[0].x0 == 5 && nodes[2].x1 == 93 && fabsf(nodes[0].y0 - 3) < .001f &&
           fabsf(nodes[0].y1 - 91) < .001f);
    memcpy(saved, nodes, sizeof nodes);
    input[0].value = DBL_MAX;
    input[1].value = DBL_MAX;
    cui_chart_link overflow[] = {{.source = 0, .target = 1, .value = DBL_MAX},
                                 {.source = 0, .target = 2, .value = DBL_MAX}};
    o.links = overflow;
    assert(cui_chart_sankey_layout(&o, nodes, 3) == CUI_PLOT_OVERFLOW);
    assert(memcmp(saved, nodes, sizeof nodes) == 0);

    cui_chart_link zero_edge[] = {{.source = 0, .target = 1, .value = 0}};
    o.links = zero_edge;
    o.link_count = 1;
    o.sankey_value_scale = CUI_CHART_SANKEY_LINEAR;
    assert(cui_chart_sankey_layout(&o, nodes, 3) == CUI_PLOT_OK);
    assert(nodes[0].layer == 1 && nodes[1].layer == 1 && nodes[2].layer == 1);
    /* Caller geometry carries no valid topology ranks and may have a cycle;
     * link-bounds only distributes widths on these existing node sides. */
    cui_chart_sankey_node arbitrary[] = {
        {.y0 = 10, .y1 = 110}, {.y0 = 20, .y1 = 100}, {.y0 = 200, .y1 = 240}};
    cui_chart_link positioned[] = {{.source = 0, .target = 2, .value = 1},
                                   {.source = 0, .target = 1, .value = 3},
                                   {.source = 1, .target = 0, .value = 2}};
    o.links = positioned;
    o.link_count = 3;
    o.sankey_value_scale = CUI_CHART_SANKEY_LINEAR;
    cui_chart_sankey_link_geometry geometry[3], old[3];
    assert(cui_chart_sankey_link_bounds(&o, arbitrary, geometry, 3) == CUI_PLOT_OK);
    assert(geometry[0].source_width == 25 && geometry[1].source_width == 75);
    assert(geometry[0].source_y == 97.5f && geometry[1].source_y == 47.5f);
    assert(geometry[0].target_width == 40 && geometry[0].target_y == 220);
    assert(geometry[1].target_width == 80 && geometry[1].target_y == 60);
    assert(geometry[2].source_width == 80 && geometry[2].target_width == 100);
    memcpy(old, geometry, sizeof old);
    arbitrary[1].y1 = 19;
    assert(cui_chart_sankey_link_bounds(&o, arbitrary, geometry, 3) == CUI_PLOT_INVALID);
    assert(memcmp(old, geometry, sizeof old) == 0);
    arbitrary[1].y1 = 100;
    assert(cui_chart_sankey_link_bounds(&o, arbitrary, geometry, 2) == CUI_PLOT_CAPACITY);
    assert(memcmp(old, geometry, sizeof old) == 0);
}
#ifndef CUI_CHART_BASE_ONLY
typedef struct {
    size_t values, labels, colors, content, titles;
} empty_tooltip_calls;
static size_t empty_tooltip_value(const cui_chart_datum *datum, size_t index, size_t series,
                                  double value, char *buffer, size_t capacity, void *user) {
    empty_tooltip_calls *calls = user;
    assert(datum && index == 0 && series < 2 && isfinite(value) && buffer && capacity);
    calls->values++;
    return 0;
}
static cui_str empty_tooltip_label(const cui_chart_datum *datum, size_t index, size_t series,
                                   void *user) {
    empty_tooltip_calls *calls = user;
    assert(datum && index == 0 && series < 2);
    calls->labels++;
    return (cui_str){"Still shown", 11};
}
static cui_color empty_tooltip_color(const cui_chart_datum *datum, size_t index, size_t series,
                                     double value, void *user) {
    empty_tooltip_calls *calls = user;
    assert(datum && index == 0 && series < 2 && isfinite(value));
    calls->colors++;
    return (cui_color){7, 209, 33, 255};
}
static cui_str empty_tooltip_title(const cui_chart_datum *datum, size_t index, void *user) {
    empty_tooltip_calls *calls = user;
    assert(datum && index == 0);
    calls->titles++;
    return (cui_str){"Unexpected", 10};
}
static void override_empty_tooltip(cui_context *ui, cui_id id, size_t index,
                                   const cui_chart_datum *datum, void *user) {
    empty_tooltip_calls *calls = user;
    assert(ui && id == 922 && datum && index == 0);
    calls->content++;
    cui_text(ui, (cui_str){"Override", 8}, 11, (cui_color){240, 240, 240, 255});
}
static size_t tooltip_value_with_unit(const cui_chart_datum *datum, size_t index, size_t series,
                                      double value, char *buffer, size_t capacity, void *user) {
    (void)datum;
    (void)index;
    (void)series;
    const char *unit = user;
    int written = snprintf(buffer, capacity, "%.2f%s", value, unit);
    return written > 0 ? (size_t)written : 0;
}
typedef struct {
    size_t calls;
    double last;
} tooltip_value_capture;
static size_t capture_tooltip_value(const cui_chart_datum *datum, size_t index, size_t series,
                                    double value, char *buffer, size_t capacity, void *user) {
    tooltip_value_capture *capture = user;
    assert(datum && index == 0 && series == 0 && isfinite(value) && capacity);
    capture->calls++;
    capture->last = value;
    buffer[0] = 'H';
    return 1;
}
#endif
static void test_reusable_chart_hover(void) {
    cui_chart_datum values[] = {{.label = {"Right", 5}, .value = DBL_MAX / 2},
                                {.label = {"Left", 4}, .value = DBL_MAX / 2}};
    cui_chart_options o = {.kind = CUI_CHART_PIE,
                           .data = values,
                           .count = 2,
                           .width = 128,
                           .height = 96,
                           .logical_width = 64,
                           .logical_height = 48,
                           .pie_outer_radius = 32,
                           .inner_radius = 8,
                           .pad_angle = .2};
    cui_chart_state hover = {0};
    /* A valid extreme Pie has two half-circle angular intervals. Source selects
     * original intervals even where pad_angle removes painted edge pixels. */
    cui_chart_hit hit = cui_chart_update_hover(&hover, &o, 44, 24);
    assert(hit.valid && hit.hovered == 0 && hit.value == DBL_MAX / 2);
    hit = cui_chart_update_hover(&hover, &o, 20, 24);
    assert(hit.valid && hit.hovered == 1 && hover.hovered == 1);
    hit = cui_chart_update_hover(&hover, &o, 32, 12);
    assert(hit.valid && hit.hovered == 0);
    hit = cui_chart_update_hover(&hover, &o, 32, 36);
    assert(hit.valid && hit.hovered == 1);
    assert(!cui_chart_update_hover(&hover, &o, 32, 24).valid); /* Donut hole. */
    assert(!cui_chart_update_hover(&hover, &o, 50, 24).valid); /* Beyond radius. */
    assert(!cui_chart_update_hover(&hover, &o, NAN, 24).valid);
    assert(!cui_chart_update_hover(NULL, &o, 44, 24).valid);
    assert(cui_chart_update_hover(&hover, &o, 44, 24).valid);
    hover.tooltip_visual_index = 0;
    hover.tooltip_focus = .5f;
    assert(!cui_chart_update_hover(&hover, &o, -1, 24).valid);
    assert(hover.hovered == SIZE_MAX && hover.hovered_value == 0 &&
           hover.tooltip_visual_index == 0 && hover.tooltip_focus == .5f &&
           hover.tooltip_cursor_valid && hover.tooltip_cursor_x == 44);
    o.noninteractive = true;
    assert(!cui_chart_update_hover(&hover, &o, 44, 24).valid);
    assert(hover.hovered == SIZE_MAX && !hover.tooltip_cursor_valid);
    o.noninteractive = false;
    o.kind = CUI_CHART_LINE;
    o.has_domain = true;
    o.domain_min = -DBL_MAX;
    o.domain_max = DBL_MAX;
    values[0].value = -DBL_MAX;
    values[1].value = DBL_MAX;
    o.show_y_axis = true;
    o.value_axis_gutter = 32;
    assert(!cui_chart_update_hover(&hover, &o, 15, 20).valid);
    assert(cui_chart_update_hover(&hover, &o, 16, 20).hovered == 0);
    assert(cui_chart_update_hover(&hover, &o, 63, 20).hovered == 1);
    assert(!cui_chart_update_hover(&hover, &o, 32, 39).valid); /* Category gutter. */
}
#ifndef CUI_CHART_BASE_ONLY
static bool hover_gate_popup(cui_draw_list draw) {
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT &&
            draw.commands[i].text_style.weight == CUI_TEXT_WEIGHT_SEMIBOLD &&
            draw.commands[i].text.length == 8 &&
            memcmp(draw.commands[i].text.data, "Gate row", 8) == 0)
            return true;
    return false;
}
static void test_appearance_hover_gate(void) {
    unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.label = {"Gate row", 8}, .value = 20},
                              {.label = {"Second", 6}, .value = 40}};
    cui_chart_options o = cui_chart_theme((cui_chart_options){.kind = CUI_CHART_LINE,
                                                              .data = data,
                                                              .count = 2,
                                                              .width = 128,
                                                              .height = 96,
                                                              .appear_key = 1},
                                          &(cui_theme){.background = {255, 255, 255, 255},
                                                       .text = {0, 0, 0, 255},
                                                       .chart_2 = {0, 100, 240, 255}});
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    cui_context *ui = cui_create((cui_config){.width = 256, .height = 256});
    assert(ui && cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    (void)cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = state.bounds.x + 30,
                                           .y = state.bounds.y + 30}));
    assert(cui_begin_frame(ui, 256, 256, .5f));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    cui_draw_list draw = cui_end_frame(ui);
    assert(state.rendered_reveal_progress > 0 && state.rendered_reveal_progress < 1);
    assert(state.hovered == SIZE_MAX && !hover_gate_popup(draw));
    /* The quartic float sample can round to1 before LINEAR time completes. */
    assert(cui_begin_frame(ui, 256, 256, .499f));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    draw = cui_end_frame(ui);
    assert(state.rendered_reveal_progress == 1 && state.hovered == SIZE_MAX &&
           !hover_gate_popup(draw));
    assert(state.appear_hover_pending && cui_needs_redraw(ui));
    /* A stationary pointer becomes eligible when appearance completes. */
    assert(cui_begin_frame(ui, 256, 256, .002f));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    draw = cui_end_frame(ui);
    assert(state.rendered_reveal_progress == 1 && state.hovered == 0 && hover_gate_popup(draw));
    /* Restart feeds no live datum, while the existing tooltip fades out. */
    o.appear_key++;
    float focus = state.tooltip_focus;
    assert(cui_begin_frame(ui, 256, 256, .01f));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    draw = cui_end_frame(ui);
    assert(state.hovered == SIZE_MAX && state.appear_hover_pending && state.tooltip_focus > 0 &&
           state.tooltip_focus < focus && hover_gate_popup(draw));
    /* Moving outside cancels pending hover; completion must not resurrect it. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 240, .y = 240}));
    assert(cui_begin_frame(ui, 256, 256, .5f));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    draw = cui_end_frame(ui);
    assert(!state.appear_hover_pending && !hover_gate_popup(draw));
    assert(cui_begin_frame(ui, 256, 256, .5f));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    draw = cui_end_frame(ui);
    assert(state.hovered == SIZE_MAX && !hover_gate_popup(draw));
    assert(!cui_needs_redraw(ui));
    /* Noninteractive still animates, but neither resolves nor retains pending input. */
    o.appear_key++;
    o.noninteractive = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 30, .y = 30}));
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    (void)cui_end_frame(ui);
    assert(state.rendered_reveal_progress == 0 && !state.appear_hover_pending);
    assert(cui_begin_frame(ui, 256, 256, .5f));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    (void)cui_end_frame(ui);
    assert(state.rendered_reveal_progress > 0 && state.rendered_reveal_progress < 1);
    /* Instant/reduced restart is immediately eligible, including owner option. */
    o.noninteractive = false;
    for (int instant = 0; instant < 3; instant++) {
        o.appear_key++;
        o.disable_appear = instant == 0;
        o.reduced_motion = instant == 1;
        cui_set_reduced_motion(ui, instant == 2);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 30, .y = 30}));
        assert(cui_begin_frame(ui, 256, 256, .12f));
        assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
        draw = cui_end_frame(ui);
        assert(state.rendered_reveal_progress == 1 && state.hovered == 0 &&
               !state.appear_hover_pending && hover_gate_popup(draw));
    }
    /* Explicit progress retains its existing caller-controlled completion contract. */
    cui_set_reduced_motion(ui, false);
    o.disable_appear = false;
    o.reduced_motion = false;
    o.reveal_enabled = true;
    o.reveal_progress = .4f;
    assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 256, 256, .01f));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    (void)cui_end_frame(ui);
    assert(state.hovered == SIZE_MAX && state.appear_hover_pending);
    o.reveal_progress = 1;
    assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 256, 256, .12f));
    assert(cui_chart(ui, 991, &state, &o, (cui_str){"Gate chart", 10}));
    draw = cui_end_frame(ui);
    assert(state.hovered == 0 && hover_gate_popup(draw));
    cui_destroy(ui);
}

#endif
int main(void) {
#ifndef CUI_CHART_BASE_ONLY
    test_appearance_hover_gate();
#endif
    test_reusable_chart_hover();
    shared_sankey_adapter();
    continuous_curve_stroke();
    cui_chart_datum points[] = {
        {.label = {"A", 1}, .value = 20, .open = 15, .high = 25, .low = 10, .close = 20},
        {.label = {"B", 1}, .value = 40, .open = 20, .high = 45, .low = 15, .close = 30},
        {.label = {"C", 1}, .value = 30, .open = 30, .high = 40, .low = 25, .close = 28},
    };
    cui_chart_link links[] = {{0, 1, 20, {0}}, {0, 2, 10, {0}}};
    static unsigned char pixels[128 * 96 * 4];
    static unsigned char saved[sizeof pixels];
    uint64_t old_revision;
    memset(pixels, 77, sizeof pixels);
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels};
    cui_chart_options options = {.data = points,
                                 .count = 3,
                                 .links = links,
                                 .link_count = 2,
                                 .width = 128,
                                 .height = 96,
                                 .logical_width = 64,
                                 .logical_height = 48,
                                 .background = {255, 255, 255, 255},
                                 .series = {{240, 0, 0, 255}},
                                 .data_revision = 1};
    for (int kind = CUI_CHART_LINE; kind <= CUI_CHART_SANKEY; kind++) {
        options.kind = (cui_chart_kind)kind;
        state.cache_key = 0;
        assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
        assert(differs(pixels, options.width * options.height, options.background));
        uint64_t revision = state.revision;
        assert(cui_chart_render(&state, &options) == CUI_PLOT_OK && state.revision == revision);
        options.data_revision++;
        assert(cui_chart_render(&state, &options) == CUI_PLOT_OK && state.revision == revision + 1);
    }
    options.data_revision = 0;
    options.kind = CUI_CHART_LINE;
    for (int kind = CUI_CHART_LINE; kind <= CUI_CHART_BAR; kind++) {
        cui_chart_options tick_options = options;
        tick_options.kind = (cui_chart_kind)kind;
        tick_options.tick_count = 2;
        assert(cui_chart_render(&state, &tick_options) == CUI_PLOT_OK);
        memcpy(saved, pixels, sizeof pixels);
        tick_options.tick_count = 1;
        assert(cui_chart_render(&state, &tick_options) == CUI_PLOT_OK);
        assert(memcmp(saved, pixels, sizeof pixels) == 0);
    }
    cui_chart_options radar_ticks = options;
    radar_ticks.kind = CUI_CHART_RADAR;
    radar_ticks.tick_count = 1;
    assert(cui_chart_render(&state, &radar_ticks) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    radar_ticks.tick_count = 2;
    assert(cui_chart_render(&state, &radar_ticks) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    radar_ticks.tick_count = 4;
    assert(cui_chart_render(&state, &radar_ticks) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    radar_ticks.tick_count = 0;
    assert(cui_chart_render(&state, &radar_ticks) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) == 0);
    options.kind = CUI_CHART_LINE;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 1;
    state.hovered_value = points[1].value;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(state.hovered == 1 && state.hovered_value == points[1].value);
    options.kind = CUI_CHART_AREA;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(state.hovered == SIZE_MAX);
    options.kind = CUI_CHART_LINE;
    float point_x = -1, point_y = -1;
    assert(cui_chart_point_coordinates(&options, 0, 0, &point_x, &point_y) == CUI_PLOT_OK);
    assert(point_x == 0 && point_y >= 10 && point_y <= 76);
    assert(cui_chart_point_coordinates(&options, 2, 0, &point_x, &point_y) == CUI_PLOT_OK);
    assert(point_x == 128);
    assert(cui_chart_hit_test(&options, 0, 20).valid &&
           cui_chart_hit_test(&options, 0, 20).hovered == 0);
    assert(cui_chart_hit_test(&options, 63, 20).valid &&
           cui_chart_hit_test(&options, 63, 20).hovered == 2);
    options.show_y_axis = true;
    assert(cui_chart_point_coordinates(&options, 0, 0, &point_x, &point_y) == CUI_PLOT_OK &&
           point_x == 32);
    assert(cui_chart_point_coordinates(&options, 2, 0, &point_x, &point_y) == CUI_PLOT_OK &&
           point_x == 128);
    options.point_count = 5;
    assert(cui_chart_point_coordinates(&options, 2, 0, &point_x, &point_y) == CUI_PLOT_OK &&
           point_x == 80);
    options.point_count = 0;
    assert(!cui_chart_hit_test(&options, 0, 20).valid);
    assert(cui_chart_hit_test(&options, 16, 20).valid &&
           cui_chart_hit_test(&options, 16, 20).hovered == 0);
    options.show_y_axis = false;
    assert(cui_chart_hit_test(&options, 16, 1).valid);
    options.custom_y_padding = true;
    options.y_padding_bottom = 20;
    assert(cui_chart_hit_test(&options, 16, 37).valid);
    cui_chart_options source_point_plot = options;
    source_point_plot.grid = (cui_color){9, 21, 233, 255};
    source_point_plot.grid_solid = true;
    source_point_plot.hide_grid = false;
    source_point_plot.hide_axes = true;
    assert(cui_chart_render(&state, &source_point_plot) == CUI_PLOT_OK);
    size_t point_top_grid = ((size_t)0 * source_point_plot.width + 127) * 4;
    assert(pixels[point_top_grid] == 9 && pixels[point_top_grid + 2] == 233);
    source_point_plot.hide_grid = true;
    source_point_plot.hide_axes = false;
    assert(cui_chart_render(&state, &source_point_plot) == CUI_PLOT_OK);
    size_t point_axis = ((size_t)76 * source_point_plot.width + 127) * 4;
    assert(pixels[point_axis] == 9 && pixels[point_axis + 2] == 233);
    options.y_padding_bottom = 0;
    options.hide_grid = true;
    options.hide_axes = true;
    options.custom_y_padding = true;
    options.y_padding_top = 0;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    unsigned row_without_padding = first_red_row(pixels, options.width, options.height);
    options.y_padding_top = 24;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(first_red_row(pixels, options.width, options.height) > row_without_padding + 15);
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    options.y_padding_top = NAN;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(saved, pixels, sizeof pixels) == 0);
    options.y_padding_top = 0;
    options.custom_y_padding = false;
    size_t region_calls = 0;
    options.area_radar_fill_pixel = chart_region_pattern;
    options.area_radar_fill_user = &region_calls;
    options.kind = CUI_CHART_AREA;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(region_calls > 0 && green_pixel(pixels, options.width * options.height));
    region_calls = 0;
    options.kind = CUI_CHART_RADAR;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(region_calls > 0 && green_pixel(pixels, options.width * options.height));
    options.area_radar_fill_pixel = NULL;
    options.area_radar_fill_user = NULL;
    options.reveal_enabled = true;
    options.hide_grid = true;
    options.hide_axes = true;
    for (int kind = CUI_CHART_LINE; kind <= CUI_CHART_SANKEY; kind++) {
        options.kind = (cui_chart_kind)kind;
        options.reveal_progress = 0;
        assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
        assert(!differs(pixels, options.width * options.height, options.background));
        options.reveal_progress = .5f;
        assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
        assert(differs(pixels, options.width * options.height, options.background));
    }
    options.reveal_enabled = false;
    options.reveal_progress = 0;
    options.hide_grid = false;
    options.hide_axes = false;
    cui_chart_datum flow[] = {
        {.label = {"A", 1}}, {.label = {"B", 1}}, {.label = {"C", 1}}, {.label = {"D", 1}}};
    cui_chart_link flow_links[] = {{0, 1, 30, {0}}, {1, 2, 20, {0}}, {1, 3, 10, {0}}};
    cui_chart_options flow_options = options;
    flow_options.kind = CUI_CHART_SANKEY;
    flow_options.data = flow;
    flow_options.count = 4;
    flow_options.links = flow_links;
    flow_options.link_count = 3;
    flow_options.data_revision = 7;
    cui_chart_state emphasis_state = {.pixels = pixels, .byte_capacity = sizeof pixels};
    assert(cui_chart_render(&emphasis_state, &flow_options) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    uint64_t flow_revision = emphasis_state.revision;
    emphasis_state.hovered = 2;
    emphasis_state.emphasis_visual_index = 2;
    emphasis_state.emphasis_focus = 1;
    assert(cui_chart_render(&emphasis_state, &flow_options) == CUI_PLOT_OK);
    assert(emphasis_state.revision == flow_revision + 1 &&
           memcmp(saved, pixels, sizeof pixels) != 0);
    cui_chart_sankey_node hover_nodes[4];
    cui_chart_sankey_link_geometry hover_links[3];
    assert(cui_chart_sankey_layout(&flow_options, hover_nodes, 4) == CUI_PLOT_OK);
    assert(cui_chart_sankey_link_bounds(&flow_options, hover_nodes, hover_links, 3) == CUI_PLOT_OK);
    size_t unrelated_x = (size_t)((hover_nodes[0].x1 + hover_nodes[1].x0) / 2);
    size_t unrelated_y = (size_t)((hover_links[0].source_y + hover_links[0].target_y) / 2);
    size_t unrelated_pixel = (unrelated_y * flow_options.width + unrelated_x) * 4;
    assert(pixels[unrelated_pixel] != saved[unrelated_pixel] ||
           pixels[unrelated_pixel + 1] != saved[unrelated_pixel + 1]);
    size_t related_x = (size_t)((hover_nodes[1].x1 + hover_nodes[2].x0) / 2);
    size_t related_y = (size_t)((hover_links[1].source_y + hover_links[1].target_y) / 2);
    size_t related_pixel = (related_y * flow_options.width + related_x) * 4;
    assert(memcmp(pixels + related_pixel, saved + related_pixel, 4) == 0);
    flow_revision = emphasis_state.revision;
    assert(cui_chart_render(&emphasis_state, &flow_options) == CUI_PLOT_OK);
    assert(emphasis_state.revision == flow_revision);
    flow_options.data_revision = 0;
    cui_chart_sankey_node nodes[4];
    assert(cui_chart_sankey_layout(&flow_options, nodes, 3) == CUI_PLOT_CAPACITY);
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    assert(fabsf(nodes[0].x0) < .01f && fabsf(nodes[3].x1 - flow_options.width) < .01f);
    double first_center = (nodes[0].y0 + nodes[0].y1) / 2;
    double second_center = (nodes[1].y0 + nodes[1].y1) / 2;
    assert(fabs(first_center - 48) < .01 && second_center > first_center + .5);
    double last_column_center =
        (fmin(nodes[2].y0, nodes[3].y0) + fmax(nodes[2].y1, nodes[3].y1)) / 2;
    assert(fabs(last_column_center - 48) < .01);
    float no_margin_left = nodes[0].x0, no_margin_right = nodes[3].x0;
    flow_options.sankey_margins = (cui_chart_sankey_margins){20, 20, 40, 4};
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    assert(fabsf(nodes[0].x0 - 20) < .01f &&
           fabsf(nodes[3].x1 - (flow_options.width - 20)) < .01f &&
           nodes[0].x0 >= no_margin_left + 19 && nodes[3].x0 <= no_margin_right - 19);
    for (size_t i = 0; i < 4; i++)
        assert(nodes[i].y0 >= 40 && nodes[i].y1 <= 92.01);
    cui_chart_sankey_node margin_nodes[4];
    memcpy(margin_nodes, nodes, sizeof nodes);
    flow_options.sankey_margins.left = NAN;
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_INVALID);
    assert(memcmp(margin_nodes, nodes, sizeof nodes) == 0);
    flow_options.sankey_margins = (cui_chart_sankey_margins){0};
    cui_chart_link skewed[] = {{0, 1, 100, {0}}, {0, 2, 1, {0}}};
    cui_chart_options scaled_flow = flow_options;
    scaled_flow.count = 3;
    scaled_flow.links = skewed;
    scaled_flow.link_count = 2;
    cui_chart_sankey_link_geometry scaled_ribbons[2];
    assert(cui_chart_sankey_layout(&scaled_flow, nodes, 4) == CUI_PLOT_OK);
    assert(nodes[0].throughput == 101);
    assert(cui_chart_sankey_link_bounds(&scaled_flow, nodes, scaled_ribbons, 2) == CUI_PLOT_OK);
    double linear_ratio = scaled_ribbons[0].source_width / scaled_ribbons[1].source_width;
    scaled_flow.sankey_value_scale = CUI_CHART_SANKEY_SQRT;
    assert(cui_chart_sankey_layout(&scaled_flow, nodes, 4) == CUI_PLOT_OK);
    assert(nodes[0].throughput == 101);
    assert(cui_chart_sankey_link_bounds(&scaled_flow, nodes, scaled_ribbons, 2) == CUI_PLOT_OK);
    double sqrt_ratio = scaled_ribbons[0].source_width / scaled_ribbons[1].source_width;
    assert(linear_ratio > 90 && sqrt_ratio > 9 && sqrt_ratio < 11);
    scaled_flow.custom_sankey_link_opacity = true;
    scaled_flow.sankey_link_opacity = 0;
    assert(cui_chart_render(&state, &scaled_flow) == CUI_PLOT_OK);
    static unsigned char transparent_links[sizeof pixels];
    memcpy(transparent_links, pixels, sizeof pixels);
    scaled_flow.sankey_link_opacity = 1;
    assert(cui_chart_render(&state, &scaled_flow) == CUI_PLOT_OK);
    assert(memcmp(transparent_links, pixels, sizeof pixels) != 0);
    flow[0].color = (cui_color){250, 0, 0, 255};
    flow[1].color = (cui_color){0, 0, 250, 255};
    assert(cui_chart_render(&state, &scaled_flow) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    skewed[0].color = (cui_color){250, 0, 0, 255};
    assert(cui_chart_render(&state, &scaled_flow) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    skewed[0].color = (cui_color){0};
    assert(cui_chart_render(&state, &scaled_flow) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    scaled_flow.sankey_node_corner_radius = 4;
    assert(cui_chart_render(&state, &scaled_flow) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    flow[0].color = (cui_color){0};
    flow[1].color = (cui_color){0};
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    assert(nodes[0].depth == 0 && nodes[1].depth == 1 && nodes[2].depth == 2 &&
           nodes[3].depth == 2);
    assert(nodes[0].throughput == 30 && nodes[1].throughput == 30 && nodes[2].throughput == 20 &&
           nodes[3].throughput == 10);
    assert(nodes[0].x1 < nodes[1].x0 && nodes[1].x1 < nodes[2].x0);
    assert(nodes[2].y1 <= nodes[3].y0);
    cui_chart_link reversed_links[] = {{0, 1, 40, {0}}, {1, 3, 10, {0}}, {1, 2, 20, {0}}};
    cui_chart_options reordered = flow_options;
    reordered.links = reversed_links;
    assert(cui_chart_sankey_layout(&reordered, nodes, 4) == CUI_PLOT_OK);
    cui_chart_sankey_link_geometry ribbons[3] = {{0}};
    assert(cui_chart_sankey_link_bounds(&reordered, nodes, ribbons, 2) == CUI_PLOT_CAPACITY);
    assert(ribbons[0].source_width == 0);
    assert(cui_chart_sankey_link_bounds(&reordered, nodes, ribbons, 3) == CUI_PLOT_OK);
    assert(ribbons[2].source_y < ribbons[1].source_y);
    assert(fabs((ribbons[1].source_width + ribbons[2].source_width) - (nodes[1].y1 - nodes[1].y0)) <
           .01);
    assert(fabs(ribbons[0].target_width - (nodes[1].y1 - nodes[1].y0)) < .01);
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    cui_chart_hit node_hit = cui_chart_hit_test(&flow_options, (nodes[1].x0 + nodes[1].x1) / 4,
                                                (nodes[1].y0 + nodes[1].y1) / 4);
    assert(node_hit.valid && node_hit.hovered == 1 && node_hit.value == 30);
    flow_options.sankey_align = CUI_CHART_SANKEY_LEFT;
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK && nodes[3].layer == 2);
    cui_chart_link crossing[] = {{0, 3, 15, {0}}, {1, 2, 15, {0}}};
    flow_options.links = crossing;
    flow_options.link_count = 2;
    flow_options.sankey_iterations = 1;
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    assert(fabs((nodes[0].y0 + nodes[0].y1) - (nodes[3].y0 + nodes[3].y1)) < 2);
    assert(fabs((nodes[1].y0 + nodes[1].y1) - (nodes[2].y0 + nodes[2].y1)) < 2);
    flow_options.sankey_iterations = 6;
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    for (size_t i = 0; i < 4; i++) {
        assert(nodes[i].y0 >= 0 && nodes[i].y1 <= flow_options.height);
        for (size_t j = i + 1; j < 4; j++)
            if (nodes[i].layer == nodes[j].layer)
                assert(nodes[i].y1 + 15 <= nodes[j].y0 || nodes[j].y1 + 15 <= nodes[i].y0);
    }
    cui_chart_link reordered_crossing[] = {
        {0, 3, 30, {0}}, {0, 2, 1, {0}}, {1, 2, 30, {0}}, {1, 3, 1, {0}}};
    cui_chart_options slot_flow = flow_options;
    slot_flow.links = reordered_crossing;
    slot_flow.link_count = 4;
    slot_flow.width = 200;
    slot_flow.height = 120;
    slot_flow.sankey_iterations = 1;
    assert(cui_chart_sankey_layout(&slot_flow, nodes, 4) == CUI_PLOT_OK);
    assert(nodes[1].y1 < nodes[0].y0 && nodes[2].y1 < nodes[3].y0);
    cui_chart_sankey_link_geometry slot_ribbons[4];
    assert(cui_chart_sankey_link_bounds(&slot_flow, nodes, slot_ribbons, 4) == CUI_PLOT_OK);
    assert(fabs(slot_ribbons[0].source_y - slot_ribbons[0].target_y) < 1);
    assert(fabs(slot_ribbons[2].source_y - slot_ribbons[2].target_y) < 1);
    flow_options.sankey_iterations = 101;
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_INVALID);
    flow_options.sankey_iterations = 0;
    flow_options.links = flow_links;
    flow_options.link_count = 3;
    flow_links[2].target = 0;
    memset(pixels, 77, sizeof pixels);
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    assert(cui_chart_render(&state, &flow_options) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(pixels, saved, sizeof pixels) == 0);
    flow_links[2].target = 3;
    options.kind = CUI_CHART_LINE;
    options.data_revision = 0;
    options.curve = CUI_CHART_CURVE_NATURAL;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    static unsigned char natural[sizeof pixels];
    memcpy(natural, pixels, sizeof pixels);
    options.curve = CUI_CHART_CURVE_LINEAR;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(natural, pixels, sizeof pixels) != 0);
    options.curve = CUI_CHART_CURVE_STEP_AFTER;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(natural, pixels, sizeof pixels) != 0);
    double values[] = {40, 15, 55};
    cui_chart_series extra = {
        .values = values, .count = 3, .color = {0, 210, 0, 255}, .name = {"Growth", 6}};
    options.extra_series = &extra;
    options.extra_series_count = 1;
    options.curve = CUI_CHART_CURVE_LINEAR;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(green_pixel(pixels, options.width * options.height));
    options.kind = CUI_CHART_AREA;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    extra.fill = (cui_color){0, 0, 240, 255};
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    memcpy(saved, pixels, sizeof pixels);
    options.curve = CUI_CHART_CURVE_NATURAL;
    extra.custom_curve = true;
    extra.curve = CUI_CHART_CURVE_STEP_AFTER;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    extra.custom_curve = false;
    extra.fill = (cui_color){0};
    options.primary_fill = (cui_color){230, 0, 0, 180};
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    options.primary_fill = (cui_color){0};
    options.kind = CUI_CHART_LINE;
    double lo, hi;
    assert(cui_chart_value_domain(&options, &lo, &hi) && lo == 0 && hi == 55);
    extra.count = 2;
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(saved, pixels, sizeof pixels) == 0);
    extra.count = 3;
    float x_with_axis, y_with_axis, x_without_axis, y_without_axis;
    assert(cui_chart_point_coordinates(&options, 1, 0, &x_with_axis, &y_with_axis) == CUI_PLOT_OK);
    assert(!cui_chart_hit_test(&options, x_with_axis * options.logical_width / options.width,
                               options.logical_height - 1)
                .valid);
    options.hide_x_axis = true;
    assert(cui_chart_point_coordinates(&options, 1, 0, &x_without_axis, &y_without_axis) ==
           CUI_PLOT_OK);
    assert(fabsf(x_with_axis - x_without_axis) < .01f && y_without_axis > y_with_axis);
    assert(cui_chart_hit_test(&options, x_without_axis * options.logical_width / options.width,
                              options.logical_height - 1)
               .valid);
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    options.hide_x_axis = false;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    options.kind = CUI_CHART_AREA;
    options.hide_x_axis = true;
    assert(cui_chart_point_coordinates(&options, 1, 0, &x_without_axis, &y_without_axis) ==
           CUI_PLOT_OK);
    assert(y_without_axis > y_with_axis);
    options.hide_x_axis = false;
    options.kind = CUI_CHART_RADAR;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    float radar_x = -1, radar_y = -1;
    assert(cui_chart_radar_vertex(&options, 0, 0, &radar_x, &radar_y) == CUI_PLOT_OK);
    assert(fabsf(radar_x - 64) < .01f && radar_y > 0 && radar_y < 48);
    assert(cui_chart_radar_vertex(&options, 0, 1, &radar_x, &radar_y) == CUI_PLOT_OK);
    assert(fabsf(radar_x - 64) < .01f && radar_y > 0 && radar_y < 48);
    float full_radar_y = radar_y;
    options.reveal_enabled = true;
    options.reveal_progress = .5;
    assert(cui_chart_radar_vertex(&options, 0, 1, &radar_x, &radar_y) == CUI_PLOT_OK);
    assert(fabsf(radar_y - (48 + full_radar_y) / 2) < .01f);
    options.reduced_motion = true;
    assert(cui_chart_radar_vertex(&options, 0, 1, &radar_x, &radar_y) == CUI_PLOT_OK);
    assert(fabsf(radar_y - full_radar_y) < .01f);
    options.reduced_motion = false;
    options.reveal_enabled = false;
    assert(cui_chart_radar_vertex(&options, 0, 9, &radar_x, &radar_y) == CUI_PLOT_INVALID);
    assert(cui_chart_hit_test(&options, 32, 1).valid);
    assert(!cui_chart_hit_test(&options, 1, 1).valid);
    assert(green_pixel(pixels, options.width * options.height));
    memcpy(saved, pixels, sizeof pixels);
    extra.fill = (cui_color){0, 0, 240, 255};
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    extra.fill = (cui_color){0};
    memcpy(saved, pixels, sizeof pixels);
    options.radar_max_value = 100;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    memcpy(saved, pixels, sizeof pixels);
    options.show_dots = true;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    options.show_dots = false;
    options.radar_max_value = 0;
    options.extra_series = NULL;
    options.extra_series_count = 0;
    options.kind = CUI_CHART_BAR;
    options.bar_alignment = CUI_CHART_BAR_BOTTOM;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    state.hovered = 0;
    state.emphasis_focus = 1;
    assert(cui_chart_bar_band_center(&options, 0, &state.emphasis_center) == CUI_PLOT_OK);
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    cui_plot_rect dimmed_bar;
    assert(cui_chart_bar_bounds(&options, 1, &dimmed_bar) == CUI_PLOT_OK);
    size_t dimmed_pixel = ((size_t)((dimmed_bar.y_min + dimmed_bar.y_max) / 2) * options.width +
                           (size_t)((dimmed_bar.x_min + dimmed_bar.x_max) / 2)) *
                          4;
    assert(pixels[dimmed_pixel] != saved[dimmed_pixel] ||
           pixels[dimmed_pixel + 1] != saved[dimmed_pixel + 1]);
    cui_plot_rect focused_bar;
    assert(cui_chart_bar_bounds(&options, 0, &focused_bar) == CUI_PLOT_OK);
    size_t focused_pixel = ((size_t)((focused_bar.y_min + focused_bar.y_max) / 2) * options.width +
                            (size_t)((focused_bar.x_min + focused_bar.x_max) / 2)) *
                           4;
    assert(memcmp(pixels + focused_pixel, saved + focused_pixel, 4) == 0);
    state.emphasis_focus = 0;
    state.hovered = SIZE_MAX;
    cui_plot_rect first_bar, second_bar;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
    assert(cui_chart_bar_bounds(&options, 1, &second_bar) == CUI_PLOT_OK);
    float first_band_center = 0, empty_band_center = 0;
    assert(cui_chart_bar_band_center(&options, 0, &first_band_center) == CUI_PLOT_OK);
    assert(fabs(first_band_center - (first_bar.x_min + first_bar.x_max) / 2) < .01);
    points[0].value = 0;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_EMPTY);
    assert(cui_chart_bar_band_center(&options, 0, &empty_band_center) == CUI_PLOT_OK);
    assert(fabs(first_band_center - empty_band_center) < .01);
    points[0].value = 20;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
    assert(fabs(first_bar.x_min - 128. / 3 * .2) < .01 && first_bar.x_max < second_bar.x_min);
    assert(fabs((first_bar.x_max - first_bar.x_min) - 25.6) < .01);
    options.point_count = 5;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
    assert(fabs((first_bar.x_max - first_bar.x_min) - 15.36) < .01);
    options.point_count = 0;
    options.custom_bar_padding = true;
    options.bar_padding_inner = 0;
    options.bar_padding_outer = 0;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
    assert(first_bar.x_min == 0 && first_bar.x_max - first_bar.x_min == 30);
    options.max_bar_width = 4;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
    assert(first_bar.x_max - first_bar.x_min == 4);
    options.max_bar_width = 0;
    options.custom_bar_padding = false;
    points[0].value = 0;
    options.min_bar_length = 3;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
    assert(first_bar.y_max - first_bar.y_min >= 3);
    options.min_bar_length = 0;
    points[0].value = 20;
    cui_plot_rect sentinel = {-1, -1, -1, -1};
    options.bar_padding_inner = NAN;
    assert(cui_chart_bar_bounds(&options, 0, &sentinel) == CUI_PLOT_INVALID);
    assert(sentinel.x_min == -1 && sentinel.y_min == -1);
    options.bar_padding_inner = 0;
    options.has_domain = true;
    options.domain_min = -50;
    options.domain_max = 50;
    points[0].value = -.001;
    options.min_bar_length = 3;
    const cui_chart_bar_alignment alignments[] = {CUI_CHART_BAR_BOTTOM, CUI_CHART_BAR_TOP,
                                                  CUI_CHART_BAR_LEFT, CUI_CHART_BAR_RIGHT};
    for (size_t i = 0; i < 4; i++) {
        options.bar_alignment = alignments[i];
        assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
        if (i < 2)
            assert(first_bar.y_max - first_bar.y_min >= 3);
        else
            assert(first_bar.x_max - first_bar.x_min >= 3);
    }
    options.bar_alignment = CUI_CHART_BAR_BOTTOM;
    options.min_bar_length = 0;
    options.has_domain = false;
    points[0].value = 20;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    static unsigned char bottom_bars[sizeof pixels];
    memcpy(bottom_bars, pixels, sizeof pixels);
    options.bar_alignment = CUI_CHART_BAR_TOP;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(bottom_bars, pixels, sizeof pixels) != 0);
    options.bar_alignment = CUI_CHART_BAR_LEFT;
    options.show_y_axis = false;
    cui_plot_rect no_axis_bar;
    assert(cui_chart_bar_bounds(&options, 2, &no_axis_bar) == CUI_PLOT_OK);
    options.show_y_axis = true;
    cui_plot_rect outside_axis_bar;
    assert(cui_chart_bar_bounds(&options, 2, &outside_axis_bar) == CUI_PLOT_OK);
    assert(outside_axis_bar.y_max < no_axis_bar.y_max);
    options.show_y_axis = false;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(bottom_bars, pixels, sizeof pixels) != 0);
    assert(cui_chart_hit_test(&options, 20, 15).valid);
    options.bar_alignment = CUI_CHART_BAR_RIGHT;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(bottom_bars, pixels, sizeof pixels) != 0);
    cui_plot_rect right_labeled, right_unlabeled, left_labeled, left_unlabeled;
    assert(cui_chart_bar_bounds(&options, 0, &right_labeled) == CUI_PLOT_OK);
    options.hide_x_axis = true;
    assert(cui_chart_bar_bounds(&options, 0, &right_unlabeled) == CUI_PLOT_OK);
    assert(right_unlabeled.x_max > right_labeled.x_max + 20);
    assert(cui_chart_hit_test(&options, options.logical_width - 1, 20).valid);
    options.bar_alignment = CUI_CHART_BAR_LEFT;
    options.hide_x_axis = false;
    assert(cui_chart_bar_bounds(&options, 0, &left_labeled) == CUI_PLOT_OK);
    options.hide_x_axis = true;
    assert(cui_chart_bar_bounds(&options, 0, &left_unlabeled) == CUI_PLOT_OK);
    assert(left_unlabeled.x_min < left_labeled.x_min - 20);
    options.hide_x_axis = false;
    options.bar_alignment = CUI_CHART_BAR_BOTTOM;
    cui_chart_options source_bar_extent = options;
    source_bar_extent.show_y_axis = false;
    source_bar_extent.show_bar_values = false;
    source_bar_extent.hide_x_axis = false;
    cui_plot_rect source_bar;
    assert(cui_chart_bar_bounds(&source_bar_extent, 1, &source_bar) == CUI_PLOT_OK);
    assert(fabs(source_bar.y_min - 10) < .01 && fabs(source_bar.y_max - 76) < .01);
    source_bar_extent.show_bar_values = true;
    assert(cui_chart_bar_bounds(&source_bar_extent, 1, &source_bar) == CUI_PLOT_OK);
    assert(fabs(source_bar.y_min - 12) < .01);
    source_bar_extent.show_bar_values = false;
    source_bar_extent.bar_alignment = CUI_CHART_BAR_TOP;
    assert(cui_chart_bar_bounds(&source_bar_extent, 1, &source_bar) == CUI_PLOT_OK);
    assert(fabs(source_bar.y_min - 20) < .01 && fabs(source_bar.y_max - 86) < .01);
    source_bar_extent.hide_x_axis = true;
    assert(cui_chart_bar_bounds(&source_bar_extent, 1, &source_bar) == CUI_PLOT_OK);
    assert(fabs(source_bar.y_min) < .01);
    source_bar_extent.bar_alignment = CUI_CHART_BAR_LEFT;
    source_bar_extent.show_y_axis = true;
    source_bar_extent.y_axis_inside = false;
    assert(cui_chart_bar_bounds(&source_bar_extent, 0, &source_bar) == CUI_PLOT_OK);
    float compressed_band_end = source_bar.y_max;
    source_bar_extent.show_y_axis = false;
    assert(cui_chart_bar_bounds(&source_bar_extent, 0, &source_bar) == CUI_PLOT_OK);
    assert(source_bar.y_max > compressed_band_end + 5);
    source_bar_extent.hide_grid = true;
    source_bar_extent.hide_axes = false;
    source_bar_extent.hide_x_axis = false;
    source_bar_extent.grid = (cui_color){9, 21, 233, 255};
    source_bar_extent.bar_alignment = CUI_CHART_BAR_TOP;
    assert(cui_chart_render(&state, &source_bar_extent) == CUI_PLOT_OK);
    size_t top_axis_pixel = ((size_t)20 * source_bar_extent.width + 127) * 4;
    assert(pixels[top_axis_pixel] == 9 && pixels[top_axis_pixel + 2] == 233);
    source_bar_extent.bar_alignment = CUI_CHART_BAR_BOTTOM;
    assert(cui_chart_render(&state, &source_bar_extent) == CUI_PLOT_OK);
    size_t bottom_axis_pixel = ((size_t)76 * source_bar_extent.width + 127) * 4;
    assert(pixels[bottom_axis_pixel] == 9 && pixels[bottom_axis_pixel + 2] == 233);
    source_bar_extent.data_revision = 91;
    assert(cui_chart_render(&state, &source_bar_extent) == CUI_PLOT_OK);
    old_revision = state.revision;
    source_bar_extent.show_bar_values = true;
    assert(cui_chart_render(&state, &source_bar_extent) == CUI_PLOT_OK);
    assert(state.revision == old_revision + 1);
    source_bar_extent.show_bar_values = false;
    source_bar_extent.bar_alignment = CUI_CHART_BAR_RIGHT;
    assert(cui_chart_render(&state, &source_bar_extent) == CUI_PLOT_OK);
    size_t right_axis_pixel = ((size_t)95 * source_bar_extent.width + 104) * 4;
    assert(pixels[right_axis_pixel] == 9 && pixels[right_axis_pixel + 2] == 233);
    source_bar_extent.hide_axes = true;
    source_bar_extent.hide_grid = false;
    source_bar_extent.grid_solid = true;
    source_bar_extent.bar_alignment = CUI_CHART_BAR_TOP;
    assert(cui_chart_render(&state, &source_bar_extent) == CUI_PLOT_OK);
    size_t top_far_grid = ((size_t)86 * source_bar_extent.width + 127) * 4;
    size_t top_baseline_grid = ((size_t)20 * source_bar_extent.width + 127) * 4;
    assert(pixels[top_far_grid] == 9 && pixels[top_far_grid + 2] == 233);
    assert(pixels[top_baseline_grid] != 9);
    source_bar_extent.bar_alignment = CUI_CHART_BAR_LEFT;
    assert(cui_chart_render(&state, &source_bar_extent) == CUI_PLOT_OK);
    size_t left_far_grid = ((size_t)95 * source_bar_extent.width + 116) * 4;
    size_t left_baseline_grid = ((size_t)95 * source_bar_extent.width + 24) * 4;
    assert(pixels[left_far_grid] == 9 && pixels[left_far_grid + 2] == 233);
    assert(pixels[left_baseline_grid] != 9);
    options.grid_columns = 3;
    options.grid_solid = false;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    options.grid_solid = true;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    memcpy(saved, pixels, sizeof pixels);
    double reference = 25;
    options.reference_values = &reference;
    options.reference_count = 1;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    options.reference_values = NULL;
    options.reference_count = 0;
    options.grid_columns = 0;
    options.hide_grid = true;
    options.hide_axes = true;
    options.bar_gradient_start = (cui_color){255, 0, 0, 255};
    options.bar_gradient_end = (cui_color){0, 0, 255, 255};
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    bool red_end = false, blue_end = false;
    for (size_t i = 0; i < options.width * options.height; i++) {
        if (pixels[4 * i] > 180 && pixels[4 * i + 2] < 70)
            red_end = true;
        if (pixels[4 * i + 2] > 180 && pixels[4 * i] < 70)
            blue_end = true;
    }
    assert(red_end && blue_end);
    size_t fill_calls = 0;
    options.bar_fill_pixel = chart_bar_pattern;
    options.bar_fill_user = &fill_calls;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(fill_calls > 10 && green_pixel(pixels, options.width * options.height));
    options.bar_fill_pixel = NULL;
    options.bar_fill_user = NULL;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    options.bar_corner_radius = 6;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    memcpy(saved, pixels, sizeof pixels);
    options.custom_bar_corners = true;
    options.bar_corner_radii[0] = 6;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(memcmp(saved, pixels, sizeof pixels) != 0);
    options.custom_bar_corners = false;
    options.reveal_enabled = true;
    options.reveal_progress = 0;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(!differs(pixels, options.width * options.height, options.background));
    options.reduced_motion = true;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(differs(pixels, options.width * options.height, options.background));
    options.reveal_enabled = false;
    options.reduced_motion = false;
    options.bar_corner_radius = 0;
    options.bar_gradient_start = (cui_color){0};
    options.bar_gradient_end = (cui_color){0};
    options.bar_corner_radius = NAN;
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(saved, pixels, sizeof pixels) == 0);
    options.bar_corner_radius = 0;
    options.bar_corner_radii[0] = -1;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    options.bar_corner_radii[0] = 0;
    options.hide_grid = false;
    options.hide_axes = false;
    options.kind = CUI_CHART_CANDLESTICK;
    points[0].open = 100;
    points[0].low = 95;
    points[0].high = 110;
    points[0].close = 105;
    points[1].open = 108;
    points[1].low = 90;
    points[1].high = 112;
    points[1].close = 94;
    points[2].open = 102;
    points[2].low = 99;
    points[2].high = 106;
    points[2].close = 102;
    cui_chart_candle_geometry first_candle, second_candle;
    assert(cui_chart_value_domain(&options, &lo, &hi) && lo == 90 && hi == 112);
    assert(cui_chart_candle_bounds(&options, 0, &first_candle) == CUI_PLOT_OK);
    assert(cui_chart_candle_bounds(&options, 1, &second_candle) == CUI_PLOT_OK);
    double candle_average = options.width / 3.0;
    double candle_outer = candle_average * .2;
    double candle_band = fmin(30, candle_average * .6);
    double candle_step = (options.width - 2 * candle_outer) / 3 * 1.2;
    assert(fabs(first_candle.wick.x_min - (candle_outer + candle_band / 2)) < .01 &&
           fabs(second_candle.wick.x_min - (candle_outer + candle_band / 2 + candle_step)) < .01);
    assert(cui_chart_hit_test(&options, 1, 1).valid &&
           !cui_chart_hit_test(&options, 1, options.logical_height - 1).valid);
    options.hide_x_axis = true;
    assert(cui_chart_hit_test(&options, 1, options.logical_height - 1).valid);
    options.hide_x_axis = false;
    assert(first_candle.bullish && !second_candle.bullish);
    assert(first_candle.wick.y_min > 12 && first_candle.wick.y_max < 76);
    assert(first_candle.body.x_max - first_candle.body.x_min > 14);
    assert(first_candle.wick.x_min < second_candle.wick.x_min);
    options.max_bar_width = 5;
    assert(cui_chart_candle_bounds(&options, 0, &second_candle) == CUI_PLOT_OK);
    assert(second_candle.body.x_max - second_candle.body.x_min <= 4.01);
    options.max_bar_width = 0;
    options.candle_body_ratio = .5f;
    cui_chart_candle_geometry narrow_candle;
    assert(cui_chart_candle_bounds(&options, 0, &narrow_candle) == CUI_PLOT_OK);
    assert(narrow_candle.body.x_max - narrow_candle.body.x_min <
           first_candle.body.x_max - first_candle.body.x_min);
    options.candle_body_ratio = NAN;
    assert(cui_chart_candle_bounds(&options, 0, &narrow_candle) == CUI_PLOT_INVALID);
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    options.candle_body_ratio = 0;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    size_t candle_grid_pixel = (size_t)0 * options.width + 36;
    size_t candle_left_pixel = (size_t)5 * options.width;
    assert(pixels[4 * candle_grid_pixel] != options.background.r ||
           pixels[4 * candle_grid_pixel + 1] != options.background.g);
    assert(pixels[4 * candle_left_pixel] == options.background.r &&
           pixels[4 * candle_left_pixel + 1] == options.background.g);
    cui_chart_hit candle_hit =
        cui_chart_hit_test(&options, (float)(first_candle.wick.x_min / 2), 18);
    assert(candle_hit.valid && candle_hit.hovered == 0 && candle_hit.value == 105);
    options.kind = CUI_CHART_PIE;
    options.pie_outer_radius = 12;
    options.inner_radius = 0;
    options.data_revision = 77;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(!cui_chart_hit_test(&options, 34, 6).valid);
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    options.pie_outer_radius = 0;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(state.revision == old_revision + 1 && memcmp(saved, pixels, sizeof pixels) != 0);
    memcpy(saved, pixels, sizeof pixels);
    state.hovered = 1;
    state.emphasis_focus = 1;
    state.pie_lift[1] = 1;
    old_revision = state.revision;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(state.revision == old_revision + 1);
    size_t lifted_pixel = ((size_t)87 * options.width + 78) * 4;
    assert(saved[lifted_pixel] == options.background.r &&
           (pixels[lifted_pixel] != saved[lifted_pixel] ||
            pixels[lifted_pixel + 1] != saved[lifted_pixel + 1]));
    size_t faded_pixel = ((size_t)20 * options.width + 64) * 4;
    assert(pixels[faded_pixel] != saved[faded_pixel] ||
           pixels[faded_pixel + 1] != saved[faded_pixel + 1]);
    state.pie_lift[1] = 0;
    state.emphasis_focus = 0;
    state.hovered = SIZE_MAX;
    options.data_revision = 0;
    options.pie_show_labels = true;
    options.pie_label_line_color_fn = pie_green_line;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(green_pixel(pixels, options.width * options.height));
    options.pie_label_line_color_fn = NULL;
    cui_chart_pie_label pie_labels[3];
    memset(pie_labels, 0, sizeof pie_labels);
    assert(cui_chart_pie_labels(&options, pie_labels, 2) == CUI_PLOT_CAPACITY);
    assert(!pie_labels[0].visible);
    assert(cui_chart_pie_labels(&options, pie_labels, 3) == CUI_PLOT_OK);
    assert(pie_labels[0].visible && pie_labels[1].visible && pie_labels[2].visible);
    assert(hypot(pie_labels[0].edge_x - 64, pie_labels[0].edge_y - 48) > 35);
    options.pie_outer_radius_fn = pie_outer_for_index;
    assert(cui_chart_pie_labels(&options, pie_labels, 3) == CUI_PLOT_OK);
    assert(hypot(pie_labels[0].edge_x - 64, pie_labels[0].edge_y - 48) < 21);
    assert(hypot(pie_labels[1].edge_x - 64, pie_labels[1].edge_y - 48) > 35);
    float outside_x = (float)((64 + (pie_labels[0].edge_x - 64) * 1.5) / 2);
    float outside_y = (float)((48 + (pie_labels[0].edge_y - 48) * 1.5) / 2);
    assert(!cui_chart_hit_test(&options, outside_x, outside_y).valid);
    options.pie_outer_radius_fn = NULL;
    assert(cui_chart_hit_test(&options, outside_x, outside_y).valid);
    options.pie_inner_radius_fn = pie_inner_for_index;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    options.pie_inner_radius_fn = pie_bad_radius;
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(pixels, saved, sizeof pixels) == 0);
    options.pie_inner_radius_fn = NULL;
    cui_chart_datum arc_data[3];
    memcpy(arc_data, points, sizeof arc_data);
    arc_data[2].value = 0;
    pie_arc_calls arc_calls = {0};
    cui_chart_options arc_options = options;
    arc_options.data = arc_data;
    arc_options.pie_outer_arc_radius_fn = pie_outer_for_arc;
    arc_options.pie_radius_user = &arc_calls;
    arc_options.pad_angle = .02;
    assert(cui_chart_render(&state, &arc_options) == CUI_PLOT_OK);
    assert(arc_calls.seen[0] && arc_calls.seen[1] && !arc_calls.seen[2]);
    assert(fabs(arc_calls.start[0]) < .0001 &&
           fabs(arc_calls.end[0] - 2 * 3.14159265358979323846 / 3) < .0001 &&
           fabs(arc_calls.start[1] - arc_calls.end[0]) < .0001 &&
           fabs(arc_calls.end[1] - 2 * 3.14159265358979323846) < .0001 &&
           fabs(arc_calls.pad[1] - .02) < .0001);
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    arc_calls.invalid = true;
    assert(cui_chart_render(&state, &arc_options) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(saved, pixels, sizeof pixels) == 0);
    options.pie_label_gap = NAN;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    options.pie_label_gap = 0;
    options.inner_radius = 12;
    options.pad_angle = .02;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    cui_chart_hit hit = cui_chart_hit_test(&options, 34, 6);
    assert(hit.valid && hit.hovered < options.count && hit.label.length == 1);
    assert(!cui_chart_hit_test(&options, 32, 24).valid); /* donut hole */
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    options.inner_radius = NAN;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(pixels, saved, sizeof pixels) == 0);
    options.inner_radius = 0;
    options.tick_count = 1;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    old_revision = state.revision;
    memcpy(saved, pixels, sizeof pixels);
    options.tick_count = 0;
    state.byte_capacity = sizeof pixels - 1;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_CAPACITY);
    assert(state.revision == old_revision && memcmp(pixels, saved, sizeof pixels) == 0);
    state.byte_capacity = sizeof pixels;
#ifndef CUI_CHART_BASE_ONLY
    cui_theme source_light;
    assert(cui_theme_preset_count() == 38 && cui_theme_preset_get(0, &source_light));
    cui_chart_options source_line_theme =
        cui_chart_theme((cui_chart_options){.kind = CUI_CHART_LINE}, &source_light);
    assert(source_line_theme.series[0].r == 59 && source_line_theme.series[0].g == 130 &&
           source_line_theme.series[0].b == 246);
    cui_chart_options source_radar_theme =
        cui_chart_theme((cui_chart_options){.kind = CUI_CHART_RADAR}, &source_light);
    assert(source_radar_theme.series[0].r == 147 && source_radar_theme.series[0].g == 197 &&
           source_radar_theme.series[0].b == 253 && source_radar_theme.series[1].r == 59);
    assert(source_light.chart_grid.r == 229 && source_light.chart_grid.a == 153 &&
           source_light.chart_bullish.g == 163 && source_light.chart_bearish.r == 220);
    assert(source_line_theme.grid.r == source_light.chart_grid.r &&
           source_line_theme.grid.a == source_light.chart_grid.a &&
           source_line_theme.bullish.g == source_light.chart_bullish.g);
    cui_chart_options source_area_theme =
        cui_chart_theme((cui_chart_options){.kind = CUI_CHART_AREA}, &source_light);
    assert(source_area_theme.series[0].r == 59 && source_area_theme.series[1].r == 59);
    cui_chart_options source_sankey_theme =
        cui_chart_theme((cui_chart_options){.kind = CUI_CHART_SANKEY}, &source_light);
    assert(source_sankey_theme.series[0].r == 147 && source_sankey_theme.series[5].r == 147);
    cui_chart_options custom_theme = cui_chart_theme(
        (cui_chart_options){
            .kind = CUI_CHART_LINE, .grid = {1, 2, 3, 255}, .series = {{4, 5, 6, 255}}},
        &source_light);
    assert(custom_theme.grid.r == 1 && custom_theme.series[0].r == 4);
    for (size_t preset = 0; preset < cui_theme_preset_count(); preset++) {
        cui_theme chart_preset;
        assert(cui_theme_preset_get(preset, &chart_preset));
        assert(chart_preset.chart_1.a && chart_preset.chart_2.a && chart_preset.chart_3.a &&
               chart_preset.chart_4.a && chart_preset.chart_5.a && chart_preset.chart_bullish.a &&
               chart_preset.chart_bearish.a && chart_preset.chart_grid.a);
    }
    cui_theme tokyo_theme;
    assert(cui_theme_preset_get(35, &tokyo_theme));
    assert(tokyo_theme.chart_1.r == 255 && tokyo_theme.chart_2.r == 192 &&
           tokyo_theme.chart_grid.r == 34 && tokyo_theme.chart_grid.g == 35 &&
           tokyo_theme.chart_grid.b == 50);
    for (int t = 0; t < 4; t++) {
        cui_theme theme = t == 0   ? cui_theme_light()
                          : t == 1 ? cui_theme_dark()
                          : t == 2 ? cui_theme_high_contrast()
                                   : cui_theme_terminal();
        cui_chart_options themed = cui_chart_theme((cui_chart_options){0}, &theme);
        assert(themed.background.a && themed.foreground.a && themed.series[0].a &&
               themed.bullish.a && themed.bearish.a);
    }
    cui_context *ui = cui_create((cui_config){.width = 256, .height = 256});
    assert(ui);
    cui_str saved_category = points[1].label;
    points[1].label = (cui_str){"An exceptionally long category", 30};
    cui_chart_options measured_bar = {.kind = CUI_CHART_BAR,
                                      .data = points,
                                      .count = 3,
                                      .width = 512,
                                      .height = 96,
                                      .bar_alignment = CUI_CHART_BAR_LEFT};
    float category_gap = -1, value_end_gap = -1;
    assert(cui_chart_measure_horizontal_bar_gutters(ui, &measured_bar, &category_gap,
                                                    &value_end_gap) == CUI_PLOT_OK);
    assert(category_gap > 24 && value_end_gap == 8);
    measured_bar.custom_horizontal_bar_gutters = true;
    measured_bar.bar_category_gutter = category_gap;
    measured_bar.bar_value_end_gutter = value_end_gap;
    cui_plot_rect measured_left;
    assert(cui_chart_bar_bounds(&measured_bar, 0, &measured_left) == CUI_PLOT_OK);
    assert(fabs(measured_left.x_min - category_gap) < .01);
    measured_bar.bar_alignment = CUI_CHART_BAR_RIGHT;
    cui_plot_rect measured_right;
    assert(cui_chart_bar_bounds(&measured_bar, 0, &measured_right) == CUI_PLOT_OK);
    assert(fabs(measured_right.x_max - (512 - category_gap)) < .01);
    measured_bar.hide_x_axis = true;
    assert(cui_chart_measure_horizontal_bar_gutters(ui, &measured_bar, &category_gap,
                                                    &value_end_gap) == CUI_PLOT_OK);
    assert(category_gap == 0);
    measured_bar.hide_x_axis = false;
    measured_bar.show_bar_values = true;
    measured_bar.bar_label_format = format_wide_tick;
    assert(cui_chart_measure_horizontal_bar_gutters(ui, &measured_bar, &category_gap,
                                                    &value_end_gap) == CUI_PLOT_OK);
    assert(value_end_gap > 8);
    cui_chart_options styled_auto = measured_bar, styled_manual = measured_bar;
    styled_auto.custom_horizontal_bar_gutters = false;
    styled_auto.disable_appear = true;
    styled_manual.bar_category_gutter = category_gap;
    styled_manual.bar_value_end_gutter = value_end_gap;
    static unsigned char auto_pixels[512 * 96 * 4], manual_pixels[sizeof auto_pixels];
    cui_chart_state auto_chart = {
        .pixels = auto_pixels, .byte_capacity = sizeof auto_pixels, .hovered = SIZE_MAX};
    cui_chart_state manual_chart = {
        .pixels = manual_pixels, .byte_capacity = sizeof manual_pixels, .hovered = SIZE_MAX};
    assert(cui_chart_render(&manual_chart, &styled_manual) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 640, 160, 0));
    assert(cui_chart(ui, 901, &auto_chart, &styled_auto, (cui_str){"Measured bar", 12}));
    (void)cui_end_frame(ui);
    assert(auto_chart.revision == 1);
    assert(memcmp(auto_pixels, manual_pixels, sizeof auto_pixels) == 0);
    cui_chart_state animated_chart = {
        .pixels = auto_pixels, .byte_capacity = sizeof auto_pixels, .hovered = SIZE_MAX};
    styled_auto.disable_appear = true;
    assert(cui_chart_render_animated(ui, &animated_chart, &styled_auto) == CUI_PLOT_OK);
    assert(memcmp(auto_pixels, manual_pixels, sizeof auto_pixels) == 0);
    float old_category = category_gap, old_end = value_end_gap;
    points[1].value = NAN;
    assert(cui_chart_measure_horizontal_bar_gutters(ui, &measured_bar, &category_gap,
                                                    &value_end_gap) == CUI_PLOT_INVALID);
    assert(category_gap == old_category && value_end_gap == old_end);
    points[1].value = 40;
    measured_bar.width = 32;
    assert(cui_chart_measure_horizontal_bar_gutters(ui, &measured_bar, &category_gap,
                                                    &value_end_gap) == CUI_PLOT_EMPTY);
    assert(category_gap == old_category && value_end_gap == old_end);
    points[1].label = saved_category;
    measured_bar.width = 128;
    measured_bar.show_bar_values = false;
    measured_bar.bar_label_format = NULL;
    assert(cui_chart_measure_horizontal_bar_gutters(ui, &measured_bar, &category_gap,
                                                    &value_end_gap) == CUI_PLOT_OK);
    measured_bar.bar_category_gutter = category_gap;
    measured_bar.bar_value_end_gutter = value_end_gap;
    measured_bar.data_revision = 77;
    assert(cui_chart_render(&state, &measured_bar) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    measured_bar.bar_category_gutter += 10;
    assert(cui_chart_render(&state, &measured_bar) == CUI_PLOT_OK);
    assert(state.revision == old_revision + 1 && memcmp(saved, pixels, sizeof pixels) != 0);
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    measured_bar.bar_category_gutter = NAN;
    assert(cui_chart_render(&state, &measured_bar) == CUI_PLOT_INVALID);
    cui_plot_rect invalid_gutter_bar;
    assert(cui_chart_bar_bounds(&measured_bar, 0, &invalid_gutter_bar) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(saved, pixels, sizeof pixels) == 0);
    options.kind = CUI_CHART_LINE;
    options.show_y_axis = true;
    options.x_tick_count = 2;
    options.y_tick_format = format_q;
    options.tooltip_format = format_q;
    float short_gutter = cui_chart_measure_value_axis_gutter(ui, &options);
    options.y_tick_format = format_wide_tick;
    float wide_gutter = cui_chart_measure_value_axis_gutter(ui, &options);
    assert(short_gutter >= 32 && wide_gutter > short_gutter);
    cui_chart_options no_candle_axis = options;
    no_candle_axis.kind = CUI_CHART_CANDLESTICK;
    assert(cui_chart_measure_value_axis_gutter(ui, &no_candle_axis) == 0);
    cui_chart_datum fractional_data[] = {{.label = {"A", 1}, .value = 1},
                                         {.label = {"B", 1}, .value = 2},
                                         {.label = {"C", 1}, .value = 3}};
    cui_chart_options fractional_axis = options;
    fractional_axis.kind = CUI_CHART_BAR;
    fractional_axis.data = fractional_data;
    fractional_axis.y_tick_format = NULL;
    fractional_axis.tooltip_format = NULL;
    fractional_axis.has_domain = true;
    fractional_axis.domain_min = 0;
    fractional_axis.domain_max = 3.14159;
    fractional_axis.y_axis_inside = true;
    fractional_axis.disable_appear = true;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 909, &state, &fractional_axis, (cui_str){"Fraction ticks", 14}));
    cui_draw_list tick_draw = cui_end_frame(ui);
    bool source_fraction_tick = false, old_fraction_tick = false;
    for (size_t i = 0; i < tick_draw.count; i++)
        if (tick_draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_str item = tick_draw.commands[i].text;
            source_fraction_tick |= item.length == 3 && memcmp(item.data, "3.1", 3) == 0;
            old_fraction_tick |= item.length == 4 && memcmp(item.data, "3.14", 4) == 0;
        }
    assert(source_fraction_tick && !old_fraction_tick);
    /* Source does not subsample value labels at 32 ticks. Explicit category count
     * zero suppresses labels while leaving grid and category line available. */
    axis_tick_capture capture = {0};
    cui_chart_options many_ticks = fractional_axis;
    many_ticks.bar_alignment = CUI_CHART_BAR_LEFT;
    many_ticks.custom_horizontal_bar_gutters = true;
    many_ticks.bar_category_gutter = 20;
    many_ticks.bar_value_end_gutter = 8;
    many_ticks.domain_min = -DBL_MAX;
    many_ticks.domain_max = DBL_MAX;
    many_ticks.tick_count = 65;
    many_ticks.y_tick_format = capture_axis_tick;
    many_ticks.format_user = &capture;
    many_ticks.custom_x_tick_count = true;
    many_ticks.x_tick_count = 0;
    many_ticks.hide_axes = false;
    many_ticks.noninteractive = true;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 910, &state, &many_ticks, (cui_str){"All ticks", 9}));
    tick_draw = cui_end_frame(ui);
    assert(capture.count == 65 && capture.first == -DBL_MAX && capture.last == DBL_MAX);
    size_t painted_ticks = 0;
    for (size_t i = 0; i < tick_draw.count; i++)
        if (tick_draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_str item = tick_draw.commands[i].text;
            painted_ticks += item.length == 1 && item.data[0] == 'T';
            assert(!(item.length == 1 && item.data[0] >= 'A' && item.data[0] <= 'C'));
        }
    assert(painted_ticks > 32); /* Endpoint text may clip at the chart edge. */
    capture = (axis_tick_capture){0};
    many_ticks.hide_axes = true;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 910, &state, &many_ticks, (cui_str){"Hidden axes", 11}));
    (void)cui_end_frame(ui);
    assert(capture.count == 0);
    /* Point labels align inward at both endpoints, including a scaled bitmap. */
    cui_chart_options endpoint_axis = fractional_axis;
    endpoint_axis.kind = CUI_CHART_LINE;
    endpoint_axis.show_y_axis = false;
    endpoint_axis.hide_axes = false;
    endpoint_axis.logical_width = 128;
    endpoint_axis.logical_height = 96;
    endpoint_axis.x_tick_count = 2;
    endpoint_axis.noninteractive = true;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 911, &state, &endpoint_axis, (cui_str){"Endpoints", 9}));
    tick_draw = cui_end_frame(ui);
    bool left_endpoint = false, right_endpoint = false;
    for (size_t i = 0; i < tick_draw.count; i++)
        if (tick_draw.commands[i].type == CUI_DRAW_TEXT) {
            const cui_draw_command *command = &tick_draw.commands[i];
            if (command->text.length == 1 && command->text.data[0] == 'A') {
                assert(fabsf(command->bounds.x - state.bounds.x) < .01f);
                left_endpoint = true;
            }
            if (command->text.length == 1 && command->text.data[0] == 'C') {
                assert(command->bounds.x + command->bounds.w <=
                       state.bounds.x + state.bounds.w + .01f);
                right_endpoint = true;
            }
        }
    assert(left_endpoint && right_endpoint);
    endpoint_axis.show_y_axis = true;
    endpoint_axis.y_axis_inside = true;
    endpoint_axis.tick_count = 3;
    endpoint_axis.y_tick_format = format_q;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 912, &state, &endpoint_axis, (cui_str){"Inside ticks", 12}));
    tick_draw = cui_end_frame(ui);
    size_t inside_ticks = 0;
    for (size_t i = 0; i < tick_draw.count; i++)
        if (tick_draw.commands[i].type == CUI_DRAW_TEXT && tick_draw.commands[i].text.length == 1 &&
            tick_draw.commands[i].text.data[0] == 'Q') {
            assert(fabsf(tick_draw.commands[i].bounds.x - state.bounds.x - 2) < .01f);
            assert(tick_draw.commands[i].bounds.y >= state.bounds.y &&
                   tick_draw.commands[i].bounds.y + 10 <= state.bounds.y + 76);
            inside_ticks++;
        }
    assert(inside_ticks == 3);
    endpoint_axis.y_axis_inside = false;
    endpoint_axis.value_axis_gutter = 32;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 913, &state, &endpoint_axis, (cui_str){"Outside ticks", 13}));
    tick_draw = cui_end_frame(ui);
    size_t outside_ticks = 0;
    for (size_t i = 0; i < tick_draw.count; i++)
        if (tick_draw.commands[i].type == CUI_DRAW_TEXT && tick_draw.commands[i].text.length == 1 &&
            tick_draw.commands[i].text.data[0] == 'Q') {
            assert(fabsf(tick_draw.commands[i].bounds.x + tick_draw.commands[i].bounds.w -
                         state.bounds.x - 28) < .01f);
            assert(tick_draw.commands[i].bounds.y >= state.bounds.y &&
                   tick_draw.commands[i].bounds.y + 10 <= state.bounds.y + 76);
            outside_ticks++;
        }
    assert(outside_ticks == 3);

    cui_chart_datum raw_data[] = {{.label = {"A", 1}, .value = 12.3456},
                                  {.label = {"B", 1}, .value = 2}};
    cui_chart_options raw_tooltip = options;
    raw_tooltip.data = raw_data;
    raw_tooltip.count = 2;
    raw_tooltip.show_y_axis = false;
    raw_tooltip.tooltip_format = NULL;
    raw_tooltip.y_tick_format = NULL;
    raw_tooltip.disable_appear = true;
    unsigned char raw_pixels[sizeof pixels] = {0};
    cui_chart_state raw_state = {.pixels = raw_pixels, .byte_capacity = sizeof raw_pixels};
    assert(cui_chart_render(&raw_state, &raw_tooltip) == CUI_PLOT_OK);
    raw_state.hovered = 0;
    raw_state.tooltip_cursor_valid = true;
    raw_state.tooltip_cursor_x = 16;
    raw_state.tooltip_cursor_y = 20;
    cui_set_reduced_motion(ui, true);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 908, &raw_state, &raw_tooltip, (cui_str){"Raw tooltip", 11}));
    cui_draw_list raw_draw = cui_end_frame(ui);
    cui_set_reduced_motion(ui, false);
    bool raw_source_value = false, raw_short_value = false;
    for (size_t i = 0; i < raw_draw.count; i++)
        if (raw_draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_str item = raw_draw.commands[i].text;
            raw_source_value |= item.length == 7 && memcmp(item.data, "12.3456", 7) == 0;
            raw_short_value |= item.length == 4 && memcmp(item.data, "12.3", 4) == 0;
        }
    assert(raw_source_value && !raw_short_value);
    raw_data[0].value = 1000;
    assert(cui_begin_frame(ui, 256, 256, .01f));
    assert(cui_chart(ui, 908, &raw_state, &raw_tooltip, (cui_str){"Raw tooltip", 11}));
    raw_draw = cui_end_frame(ui);
    bool raw_integer = false, scientific_integer = false;
    for (size_t i = 0; i < raw_draw.count; i++)
        if (raw_draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_str item = raw_draw.commands[i].text;
            raw_integer |= item.length == 4 && memcmp(item.data, "1000", 4) == 0;
            scientific_integer |= item.length == 5 && memcmp(item.data, "1e+03", 5) == 0;
        }
    assert(raw_integer && !scientific_integer);
    cui_chart_options styled_axis_auto = options, styled_axis_manual = options;
    styled_axis_auto.value_axis_gutter = 0;
    styled_axis_auto.disable_appear = true;
    styled_axis_manual.value_axis_gutter = wide_gutter;
    static unsigned char axis_auto_pixels[sizeof pixels], axis_manual_pixels[sizeof pixels];
    cui_chart_state axis_auto_chart = {
        .pixels = axis_auto_pixels, .byte_capacity = sizeof axis_auto_pixels, .hovered = SIZE_MAX};
    cui_chart_state axis_manual_chart = {.pixels = axis_manual_pixels,
                                         .byte_capacity = sizeof axis_manual_pixels,
                                         .hovered = SIZE_MAX};
    assert(cui_chart_render(&axis_manual_chart, &styled_axis_manual) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 902, &axis_auto_chart, &styled_axis_auto, (cui_str){"Measured axis", 13}));
    (void)cui_end_frame(ui);
    assert(axis_auto_chart.revision == 1 &&
           memcmp(axis_auto_pixels, axis_manual_pixels, sizeof pixels) == 0);
    styled_axis_auto.disable_appear = true;
    cui_chart_state axis_animated_chart = {
        .pixels = axis_auto_pixels, .byte_capacity = sizeof axis_auto_pixels, .hovered = SIZE_MAX};
    assert(cui_chart_render_animated(ui, &axis_animated_chart, &styled_axis_auto) == CUI_PLOT_OK);
    assert(memcmp(axis_auto_pixels, axis_manual_pixels, sizeof pixels) == 0);
    cui_chart_options unfinished_labels = {.kind = CUI_CHART_LINE,
                                           .data = points,
                                           .count = 3,
                                           .width = 128,
                                           .height = 96,
                                           .point_count = 5,
                                           .x_tick_count = 2,
                                           .noninteractive = true,
                                           .background = {255, 255, 255, 255},
                                           .series = {{240, 0, 0, 255}}};
    cui_chart_state label_chart = {
        .pixels = axis_auto_pixels, .byte_capacity = sizeof axis_auto_pixels, .hovered = SIZE_MAX};
    assert(cui_chart_render(&label_chart, &unfinished_labels) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 903, &label_chart, &unfinished_labels, (cui_str){"Slots", 5}));
    cui_draw_list unfinished_draw = cui_end_frame(ui);
    unsigned first_label = 0, last_label = 0;
    for (size_t i = 0; i < unfinished_draw.count; i++)
        if (unfinished_draw.commands[i].type == CUI_DRAW_TEXT &&
            unfinished_draw.commands[i].text.length == 1) {
            first_label += unfinished_draw.commands[i].text.data[0] == 'A';
            last_label += unfinished_draw.commands[i].text.data[0] == 'C';
        }
    assert(first_label == 1 && last_label == 0);
    options.value_axis_gutter = wide_gutter;
    options.kind = CUI_CHART_BAR;
    cui_plot_rect shifted_bar;
    assert(cui_chart_bar_bounds(&options, 0, &shifted_bar) == CUI_PLOT_OK);
    options.value_axis_gutter = short_gutter;
    cui_plot_rect narrow_bar;
    assert(cui_chart_bar_bounds(&options, 0, &narrow_bar) == CUI_PLOT_OK);
    assert(shifted_bar.x_min > narrow_bar.x_min);
    options.value_axis_gutter = wide_gutter;
    options.kind = CUI_CHART_LINE;
    options.y_tick_format = format_q;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    memcpy(saved, pixels, sizeof pixels);
    old_revision = state.revision;
    options.value_axis_gutter = NAN;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(saved, pixels, sizeof pixels) == 0);
    options.value_axis_gutter = wide_gutter;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 0;
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    cui_draw_list draw = cui_end_frame(ui);
    bool image = false, text = false, formatted = false, hover_crosshair = false,
         hover_halo = false, hover_dot = false, hover_stroke = false, eased_crosshair = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE) {
            if (draw.commands[i].bitmap.bitmap.pixels == pixels) {
                image = true;
                assert(draw.commands[i].bounds.w == 64 && draw.commands[i].bounds.h == 48);
            } else if (draw.commands[i].bitmap.bitmap.width == 1 &&
                       draw.commands[i].bitmap.bitmap.height == 38) {
                hover_crosshair = draw.commands[i].bounds.w == 1 &&
                                  fabsf(draw.commands[i].bounds.y) < .01f &&
                                  fabsf(draw.commands[i].bounds.h - 38) < .01f;
                const unsigned char *guide = draw.commands[i].bitmap.bitmap.pixels;
                eased_crosshair = guide[3] > 0 && guide[3 + 4 * 4] == 0 &&
                                  fabsf(guide[3] - (options.grid.a ? options.grid.a : 150) *
                                                       state.tooltip_focus) <= 1;
            }
        } else if (draw.commands[i].type == CUI_DRAW_TEXT) {
            text = true;
            if (draw.commands[i].text.length == 1 && draw.commands[i].text.data[0] == 'Q')
                formatted = true;
        } else if (draw.commands[i].type == CUI_DRAW_RECT) {
            hover_halo |= fabsf(draw.commands[i].bounds.w - 20 * state.tooltip_focus) < .01f &&
                          draw.commands[i].color.r == options.series[0].r &&
                          draw.commands[i].color.a > 0;
            hover_dot |= draw.commands[i].bounds.w == 8 && draw.commands[i].bounds.h == 8 &&
                         draw.commands[i].color.r == options.series[0].r;
        } else if (draw.commands[i].type == CUI_DRAW_BORDER)
            hover_stroke |= draw.commands[i].color.r == options.background.r;
    assert(image && text && formatted && hover_crosshair && hover_halo && hover_dot &&
           hover_stroke && eased_crosshair);
    assert(state.hover_progress > 0 && state.hover_progress < 1);
    static unsigned char guide_pixels[48 * 4] = {0};
    state.hover_guide_pixels = guide_pixels;
    state.hover_guide_capacity = sizeof guide_pixels;
    assert(cui_begin_frame(ui, 256, 256, .01));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    bool dashed_guide = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE &&
            draw.commands[i].bitmap.bitmap.pixels == guide_pixels &&
            draw.commands[i].bitmap.bitmap.width == 1 &&
            draw.commands[i].bitmap.bitmap.height == 38 && fabsf(draw.commands[i].bounds.y) < .01f)
            dashed_guide = true;
    assert(dashed_guide && guide_pixels[3] > 0 && guide_pixels[3 + 3 * 4] > 0 &&
           guide_pixels[3 + 4 * 4] == 0 && guide_pixels[3 + 6 * 4] > 0);
    state.hover_guide_pixels = NULL;
    state.hover_guide_capacity = 0;
    options.reduced_motion = true;
    assert(cui_begin_frame(ui, 256, 256, .04));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    (void)cui_end_frame(ui);
    assert(state.hover_progress == 1);
    state.hovered = SIZE_MAX;
    assert(cui_begin_frame(ui, 256, 256, .05));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    (void)cui_end_frame(ui);
    assert(state.hover_progress == 0 && !state.hover_visual_started);
    options.reduced_motion = false;
    options.x_tick_count = 0;
    options.x_tick_margin = 2;
    state.hovered = SIZE_MAX;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    bool category_a = false, category_b = false, category_c = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1) {
            char c = draw.commands[i].text.data[0];
            category_a |= c == 'A';
            category_b |= c == 'B';
            category_c |= c == 'C';
        }
    assert(!category_a && category_b && !category_c);
    options.x_tick_margin = 0;
    options.x_tick_count = 2;
    size_t count;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    bool named = false;
    for (size_t i = 0; i < count; i++)
        if (semantics[i].id == 91 && semantics[i].role == CUI_ROLE_IMAGE &&
            semantics[i].name.length == 7)
            named = true;
    assert(named);
    assert(state.bounds.w == 64 && state.bounds.h == 48);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 34, .y = 6}));
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    (void)cui_end_frame(ui);
    assert(state.hovered < options.count);
    options.kind = CUI_CHART_BAR;
    options.show_bar_values = true;
    options.show_y_axis = false;
    options.bar_label_format = format_q;
    options.bar_label_color_fn = bar_label_by_index;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    formatted = false;
    bool per_bar_color = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            draw.commands[i].text.data[0] == 'Q') {
            formatted = true;
            per_bar_color |= draw.commands[i].color.g == 211;
        }
    assert(formatted && per_bar_color);
    cui_plot_rect hovered_bar;
    assert(cui_chart_bar_bounds(&options, 1, &hovered_bar) == CUI_PLOT_OK);
    options.reduced_motion = true;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                        .x = state.bounds.x + (hovered_bar.x_min + hovered_bar.x_max) / 2 *
                                                  options.logical_width / options.width,
                        .y = state.bounds.y + (hovered_bar.y_min + hovered_bar.y_max) / 2 *
                                                  options.logical_height / options.height}));
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    assert(state.hovered == 1 && state.emphasis_focus == 1);
    bool hover_band = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].bounds.y == 0 &&
            draw.commands[i].bounds.h == 38 && draw.commands[i].color.a > 0 &&
            draw.commands[i].color.a <= 25)
            hover_band = true;
    assert(hover_band);
    options.bar_alignment = CUI_CHART_BAR_TOP;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 1;
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    bool top_hover_band = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].bounds.y == 10 &&
            draw.commands[i].bounds.h == 38 && draw.commands[i].color.a > 0 &&
            draw.commands[i].color.a <= 25)
            top_hover_band = true;
    assert(top_hover_band);
    options.bar_alignment = CUI_CHART_BAR_BOTTOM;
    options.reduced_motion = false;
    float old_band_center = state.emphasis_center;
    assert(cui_chart_bar_bounds(&options, 0, &hovered_bar) == CUI_PLOT_OK);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                        .x = state.bounds.x + (hovered_bar.x_min + hovered_bar.x_max) / 2 *
                                                  options.logical_width / options.width,
                        .y = state.bounds.y + (hovered_bar.y_min + hovered_bar.y_max) / 2 *
                                                  options.logical_height / options.height}));
    assert(cui_begin_frame(ui, 256, 256, .016f));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    float target_band_center;
    assert(cui_chart_bar_band_center(&options, 0, &target_band_center) == CUI_PLOT_OK);
    assert(state.hovered == 0 && state.emphasis_center > target_band_center &&
           state.emphasis_center < old_band_center && cui_needs_redraw(ui));
    (void)cui_end_frame(ui);
    options.bar_label_format = format_wide_tick;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    cui_str wide_label = {"123456789", 9};
    float expected_width = cui_measure_text(ui, wide_label, 10).width;
    bool measured_bar_label = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 9 &&
            memcmp(draw.commands[i].text.data, wide_label.data, 9) == 0)
            measured_bar_label |= fabsf(draw.commands[i].bounds.w - expected_width) < 1;
    assert(measured_bar_label);
    options.bar_label_format = format_q;
    options.bar_label_color_fn = NULL;
    options.bar_alignment = CUI_CHART_BAR_LEFT;
    options.show_y_axis = true;
    state.hovered = SIZE_MAX;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    float first_tick_x = INFINITY, last_tick_x = -INFINITY;
    unsigned horizontal_ticks = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            draw.commands[i].text.data[0] == 'Q' && draw.commands[i].bounds.y >= 21 &&
            draw.commands[i].bounds.y <= 23) {
            horizontal_ticks++;
            first_tick_x = fminf(first_tick_x, draw.commands[i].bounds.x);
            last_tick_x = fmaxf(last_tick_x, draw.commands[i].bounds.x);
        }
    assert(horizontal_ticks >= 4 && last_tick_x - first_tick_x > 30);
    float left_category_x = NAN;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            draw.commands[i].text.data[0] == 'A')
            left_category_x = draw.commands[i].bounds.x;
    assert(isfinite(left_category_x));
    options.bar_alignment = CUI_CHART_BAR_RIGHT;
    assert(cui_begin_frame(ui, 256, 256, .055));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    float right_category_x = NAN;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            draw.commands[i].text.data[0] == 'A')
            right_category_x = draw.commands[i].bounds.x;
    assert(isfinite(right_category_x) && right_category_x > left_category_x + 30);
    options.bar_alignment = CUI_CHART_BAR_BOTTOM;
    options.show_y_axis = false;
    options.x_tick_count = 0;
    options.x_tick_margin = 1;
    points[0].value = -20;
    cui_plot_rect negative_bar;
    assert(cui_chart_bar_bounds(&options, 0, &negative_bar) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 256, 256, .06));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    float negative_label_y = NAN, positive_label_y = NAN;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1) {
            if (draw.commands[i].text.data[0] == 'A')
                negative_label_y = draw.commands[i].bounds.y;
            if (draw.commands[i].text.data[0] == 'B')
                positive_label_y = draw.commands[i].bounds.y;
        }
    assert(isfinite(negative_label_y) && isfinite(positive_label_y) &&
           negative_label_y < positive_label_y);
    points[0].value = 0;
    assert(cui_begin_frame(ui, 256, 256, .07));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    bool zero_category = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            draw.commands[i].text.data[0] == 'A')
            zero_category = true;
    assert(zero_category);
    points[0].value = 20;
    options.kind = CUI_CHART_CANDLESTICK;
    options.foreground = (cui_color){20, 30, 40, 255};
    options.bullish = (cui_color){7, 180, 8, 255};
    options.tooltip_format = NULL;
    static unsigned char fresh_candle_pixels[sizeof pixels] = {0};
    cui_chart_state fresh_candle = {.pixels = fresh_candle_pixels,
                                    .byte_capacity = sizeof fresh_candle_pixels};
    assert(cui_begin_frame(ui, 256, 256, .069));
    assert(cui_chart(ui, 92, &fresh_candle, &options, (cui_str){"Fresh candle", 12}));
    draw = cui_end_frame(ui);
    assert(fresh_candle.revision == 1 && fresh_candle.hovered == SIZE_MAX &&
           differs(fresh_candle_pixels, options.width * options.height, options.background));
    memcpy(saved, fresh_candle_pixels, sizeof saved);
    float original_open = points[0].open;
    points[0].open = points[0].high - 1;
    cui_chart_options changed_candle = options;
    changed_candle.data_revision++;
    assert(cui_begin_frame(ui, 256, 256, .0695));
    assert(cui_chart(ui, 92, &fresh_candle, &changed_candle, (cui_str){"Fresh candle", 12}));
    draw = cui_end_frame(ui);
    assert(fresh_candle.revision == 2 && memcmp(saved, fresh_candle_pixels, sizeof saved) != 0);
    points[0].open = original_open;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 0;
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
    draw = cui_end_frame(ui);
    bool ohlc[4] = {0};
    bool candle_swatch = false, neutral_value = false;
    static const char *ohlc_labels[] = {"Open", "High", "Low", "Close"};
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            const char *label = draw.commands[i].text.data;
            for (size_t row = 0; row < 4; row++)
                if (draw.commands[i].text.length == strlen(ohlc_labels[row]) &&
                    memcmp(label, ohlc_labels[row], draw.commands[i].text.length) == 0)
                    ohlc[row] = true;
            if (draw.commands[i].text.length > 0 && label[0] >= '0' && label[0] <= '9' &&
                draw.commands[i].color.r == options.foreground.r)
                neutral_value = true;
        } else if (draw.commands[i].type == CUI_DRAW_RECT &&
                   fabsf(draw.commands[i].bounds.w - 8) < .01f &&
                   draw.commands[i].color.g == options.bullish.g) {
            candle_swatch = true;
        }
    assert(ohlc[0] && ohlc[1] && ohlc[2] && ohlc[3] && candle_swatch && neutral_value);
    cui_chart_candle_geometry first_hover_candle, next_hover_candle;
    assert(cui_chart_candle_bounds(&options, 0, &first_hover_candle) == CUI_PLOT_OK);
    assert(cui_chart_candle_bounds(&options, 1, &next_hover_candle) == CUI_PLOT_OK);
    bool candle_hover_band = false;
    float expected_band = (float)((first_hover_candle.body.x_max - first_hover_candle.body.x_min) /
                                  .8f * options.logical_width / options.width);
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT &&
            fabsf(draw.commands[i].bounds.w - expected_band) < .01f &&
            fabsf(draw.commands[i].bounds.h -
                  (options.logical_height - 20 * options.logical_height / options.height)) < .01f &&
            draw.commands[i].color.r == options.foreground.r && draw.commands[i].color.a > 0 &&
            draw.commands[i].color.a < 30)
            candle_hover_band = true;
    assert(candle_hover_band && fabs(state.emphasis_center - first_hover_candle.wick.x_min) < .01);
    state.hovered = 1;
    assert(cui_begin_frame(ui, 256, 256, .01f));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
    (void)cui_end_frame(ui);
    assert(state.emphasis_center > first_hover_candle.wick.x_min &&
           state.emphasis_center < next_hover_candle.wick.x_min);
    options.reduced_motion = true;
    state.hovered = 2;
    cui_chart_candle_geometry last_hover_candle;
    assert(cui_chart_candle_bounds(&options, 2, &last_hover_candle) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
    (void)cui_end_frame(ui);
    assert(fabs(state.emphasis_center - last_hover_candle.wick.x_min) < .01 &&
           state.emphasis_focus == 1);
    options.reduced_motion = false;
    state.hovered = 0;
    size_t tooltip_calls = 0;
    options.tooltip_content = custom_tooltip;
    options.tooltip_user = &tooltip_calls;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 0;
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
    draw = cui_end_frame(ui);
    bool custom = false, default_ohlc = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_str label = draw.commands[i].text;
            if (label.length == 6 && memcmp(label.data, "Custom", 6) == 0)
                custom = true;
            if (label.length == 4 && memcmp(label.data, "Open", 4) == 0)
                default_ohlc = true;
        }
    assert(tooltip_calls == 1 && custom && !default_ohlc);
    options.tooltip_measure = custom_tooltip_measure;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
    draw = cui_end_frame(ui);
    bool measured_custom = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT &&
            fabsf(draw.commands[i].bounds.w - 236) < .01f &&
            fabsf(draw.commands[i].bounds.h - 106) < .01f)
            measured_custom = true;
    assert(measured_custom && tooltip_calls == 2);
    cui_rect custom_portal = {0}, custom_content_bounds = {0};
    for (size_t i = 0; i < draw.count; i++) {
        if (draw.commands[i].type == CUI_DRAW_RECT &&
            fabsf(draw.commands[i].bounds.w - 236) < .01f &&
            fabsf(draw.commands[i].bounds.h - 106) < .01f)
            custom_portal = draw.commands[i].bounds;
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 6 &&
            memcmp(draw.commands[i].text.data, "Custom", 6) == 0)
            custom_content_bounds = draw.commands[i].bounds;
    }
    assert(fabsf(custom_content_bounds.x - custom_portal.x - 8) < .01f &&
           fabsf(custom_content_bounds.y - custom_portal.y - 8) < .01f);

    const cui_size bad_sizes[] = {{NAN, 20}, {-1, 20}, {16369, 20}, {20, INFINITY}};
    for (size_t bad = 0; bad < sizeof bad_sizes / sizeof bad_sizes[0]; bad++) {
        custom_tooltip_size = bad_sizes[bad];
        assert(cui_begin_frame(ui, 256, 256, 0));
        assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
        draw = cui_end_frame(ui);
        bool fallback = false;
        for (size_t i = 0; i < draw.count; i++)
            if (draw.commands[i].type == CUI_DRAW_RECT &&
                fabsf(draw.commands[i].bounds.w - 150) < .01f &&
                fabsf(draw.commands[i].bounds.h - 64) < .01f)
                fallback = true;
        assert(fallback);
    }
    custom_tooltip_size = (cui_size){0, 0};
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
    draw = cui_end_frame(ui);
    bool zero_content = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT &&
            fabsf(draw.commands[i].bounds.w - 150) < .01f &&
            fabsf(draw.commands[i].bounds.h - 16) < .01f)
            zero_content = true;
    assert(zero_content);
    custom_tooltip_size = (cui_size){220, 90};
    options.tooltip_measure = NULL;
    options.tooltip_content = NULL;
    options.tooltip_title = tooltip_title_override;
    options.tooltip_value = tooltip_value_override;
    options.tooltip_value_color = tooltip_color_override;
    options.foreground = (cui_color){20, 30, 40, 255};
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
    draw = cui_end_frame(ui);
    bool overridden_title = false, overridden_value = false;
    float open_x = NAN, value_x = NAN;
    bool row_swatch = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_str label = draw.commands[i].text;
            overridden_title |= label.length == 5 && memcmp(label.data, "Price", 5) == 0 &&
                                draw.commands[i].text_style.weight == CUI_TEXT_WEIGHT_SEMIBOLD;
            overridden_value |= label.length == 1 && memcmp(label.data, "V", 1) == 0 &&
                                draw.commands[i].color.g == 210;
            if (label.length == 4 && memcmp(label.data, "Open", 4) == 0)
                open_x = draw.commands[i].bounds.x;
            if (label.length == 1 && label.data[0] == 'V')
                value_x = draw.commands[i].bounds.x;
        } else if (draw.commands[i].type == CUI_DRAW_RECT &&
                   fabsf(draw.commands[i].bounds.w - 8) < .01f &&
                   fabsf(draw.commands[i].bounds.h - 8) < .01f)
            row_swatch = true;
    assert(overridden_title && overridden_value && row_swatch && open_x < value_x);
    options.tooltip_title = NULL;
    options.tooltip_value = NULL;
    options.tooltip_value_color = NULL;
    options.kind = CUI_CHART_BAR;
    options.bar_gradient_start = (cui_color){211, 22, 33, 255};
    options.bar_gradient_end = (cui_color){4, 155, 6, 255};
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 0;
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    bool gradient_swatch = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && fabsf(draw.commands[i].bounds.w - 8) < .01f &&
            draw.commands[i].color.r == options.bar_gradient_start.r &&
            draw.commands[i].color.g == options.bar_gradient_start.g)
            gradient_swatch = true;
    assert(gradient_swatch);
    options.bar_gradient_start = options.bar_gradient_end = (cui_color){0};
    options.pie_show_labels = true;
    options.pie_outer_radius = 20;
    options.pie_label_gap = 5;
    options.logical_width = 128;
    options.logical_height = 96;
    options.kind = CUI_CHART_PIE;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 0;
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Shares", 6}));
    draw = cui_end_frame(ui);
    bool pie_named_row = false, pie_share = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_str label = draw.commands[i].text;
            if (label.length == 6 && memcmp(label.data, "Shares", 6) == 0)
                pie_named_row = true;
            if (label.length >= 2 && label.data[label.length - 2] == '%' &&
                label.data[label.length - 1] == ')')
                pie_share = true;
        }
    assert(pie_named_row && pie_share);
    options.tooltip_label = tooltip_label_override;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Shares", 6}));
    draw = cui_end_frame(ui);
    bool pie_custom_name = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 10 &&
            memcmp(draw.commands[i].text.data, "CustomName", 10) == 0)
            pie_custom_name = true;
    assert(pie_custom_name);
    options.tooltip_label = NULL;
    options.pie_tooltip_value = pie_value_override;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Shares", 6}));
    draw = cui_end_frame(ui);
    bool pie_custom_value = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 6 &&
            memcmp(draw.commands[i].text.data, "Share!", 6) == 0)
            pie_custom_value = true;
    assert(pie_custom_value);
    options.pie_tooltip_value = NULL;
    bool outside_label = false;
    cui_chart_pie_label styled_pie_labels[3];
    assert(cui_chart_pie_labels(&options, styled_pie_labels, 3) == CUI_PLOT_OK);
    bool measured_pie_labels = false, left_pie_label = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            (draw.commands[i].text.data[0] == 'A' || draw.commands[i].text.data[0] == 'B')) {
            outside_label = true;
            measured_pie_labels |= fabsf(draw.commands[i].bounds.w -
                                         cui_measure_text(ui, draw.commands[i].text, 10).width) < 1;
        } else if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
                   draw.commands[i].text.data[0] == 'C')
            left_pie_label = draw.commands[i].bounds.x <= styled_pie_labels[2].anchor_x;
    assert(outside_label);
    assert(measured_pie_labels && left_pie_label);
    options.reduced_motion = true;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = state.bounds.x + 80,
                                           .y = state.bounds.y + 55}));
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Shares", 6}));
    (void)cui_end_frame(ui);
    assert(state.hovered == 1 && state.emphasis_focus == 1 && state.pie_lift[1] == 1 &&
           state.pie_lift[0] == 0);
    options.reduced_motion = false;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = state.bounds.x + 2,
                                           .y = state.bounds.y + 2}));
    assert(cui_begin_frame(ui, 256, 256, .06f));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Shares", 6}));
    (void)cui_end_frame(ui);
    assert(state.hovered == SIZE_MAX && state.emphasis_focus > 0 && state.emphasis_focus < 1 &&
           state.pie_lift[1] > 0 && state.pie_lift[1] < 1);
    assert(state.tooltip_visual_index == 1 && state.tooltip_focus > .5f && state.tooltip_focus < 1);
    assert(fabsf(state.emphasis_focus - state.tooltip_focus) < .0001f);
    options.pie_show_labels = false;
    options.pie_outer_radius = 0;
    options.pie_label_gap = 0;
    options.kind = CUI_CHART_RADAR;
    options.radar_label_gap = 18;
    options.radar_label_color = (cui_color){220, 10, 20, 255};
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 0;
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Radar", 5}));
    draw = cui_end_frame(ui);
    bool radar_colored_label = false, radar_hover_dot = false, radar_hover_halo = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            draw.commands[i].color.r == 220 && draw.commands[i].color.g == 10)
            radar_colored_label = true;
        else if (draw.commands[i].type == CUI_DRAW_RECT &&
                 fabsf(draw.commands[i].bounds.w - 8) < .01f &&
                 fabsf(draw.commands[i].bounds.h - 8) < .01f &&
                 draw.commands[i].color.r == options.series[0].r)
            radar_hover_dot = true;
        else if (draw.commands[i].type == CUI_DRAW_RECT &&
                 fabsf(draw.commands[i].bounds.w - 20 * state.tooltip_focus) < .01f &&
                 draw.commands[i].color.r == options.series[0].r)
            radar_hover_halo = true;
    assert(radar_colored_label);
    assert(radar_hover_dot && radar_hover_halo);
    radar_label_calls custom_labels = {0};
    options.radar_label_measure = measure_radar_label;
    options.radar_label_paint = paint_radar_label;
    options.radar_label_user = &custom_labels;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Radar", 5}));
    draw = cui_end_frame(ui);
    assert(custom_labels.measured == options.count && custom_labels.painted == options.count);
    bool custom_radar_content = false, old_radar_label = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1) {
            custom_radar_content |= draw.commands[i].text.data[0] == 'R' &&
                                    draw.commands[i].color.g == 199 &&
                                    draw.commands[i].bounds.x >= 0 &&
                                    draw.commands[i].bounds.x < options.logical_width &&
                                    draw.commands[i].bounds.y >= 0 &&
                                    draw.commands[i].bounds.y < options.logical_height;
            old_radar_label |= draw.commands[i].color.r == 220 && draw.commands[i].color.g == 10;
        }
    assert(custom_radar_content && !old_radar_label);
    custom_labels.invalid = true;
    custom_labels.measured = custom_labels.painted = 0;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Radar", 5}));
    (void)cui_end_frame(ui);
    assert(custom_labels.measured == options.count && custom_labels.painted == 0);
    options.radar_label_measure = NULL;
    options.radar_label_paint = NULL;
    options.radar_label_user = NULL;
    options.radar_label_gap = 0;
    options.radar_label_color = (cui_color){0};
    options.kind = CUI_CHART_SANKEY;
    options.sankey_show_labels = true;
    options.sankey_show_values = true;
    options.sankey_value_format = format_q;
    cui_chart_sankey_margins measured_margins = {0};
    assert(cui_chart_measure_sankey_margins(ui, &options, &measured_margins) == CUI_PLOT_OK);
    assert(measured_margins.left > 0 && measured_margins.right > 0 && measured_margins.bottom > 0);
    static unsigned char expected_sankey_pixels[sizeof pixels];
    static unsigned char automatic_sankey_pixels[sizeof pixels] = {0};
    cui_chart_state expected_sankey = {.pixels = expected_sankey_pixels,
                                       .byte_capacity = sizeof expected_sankey_pixels};
    cui_chart_state automatic_sankey = {.pixels = automatic_sankey_pixels,
                                        .byte_capacity = sizeof automatic_sankey_pixels};
    cui_chart_options explicit_sankey = options;
    explicit_sankey.sankey_margins = measured_margins;
    cui_chart_options automatic_sankey_options = options;
    automatic_sankey_options.disable_appear = true;
    assert(cui_chart_render(&expected_sankey, &explicit_sankey) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(
        cui_chart(ui, 92, &automatic_sankey, &automatic_sankey_options, (cui_str){"Auto flow", 9}));
    (void)cui_end_frame(ui);
    assert(memcmp(expected_sankey_pixels, automatic_sankey_pixels, sizeof expected_sankey_pixels) ==
           0);
    static unsigned char animated_sankey_pixels[sizeof pixels] = {0};
    cui_chart_state animated_sankey = {.pixels = animated_sankey_pixels,
                                       .byte_capacity = sizeof animated_sankey_pixels};
    cui_chart_options immediate_sankey = options;
    immediate_sankey.disable_appear = true;
    assert(cui_begin_frame(ui, 256, 256, .001));
    assert(cui_chart_render_animated(ui, &animated_sankey, &immediate_sankey) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(memcmp(expected_sankey_pixels, animated_sankey_pixels, sizeof expected_sankey_pixels) ==
           0);
    cui_chart_options compact_sankey = options;
    compact_sankey.custom_sankey_margins = true;
    compact_sankey.disable_appear = true;
    assert(cui_chart_render(&expected_sankey, &compact_sankey) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 256, 256, .002));
    assert(cui_chart(ui, 92, &automatic_sankey, &compact_sankey, (cui_str){"Compact", 7}));
    (void)cui_end_frame(ui);
    assert(memcmp(expected_sankey_pixels, automatic_sankey_pixels, sizeof expected_sankey_pixels) ==
           0);
    options.sankey_margins = measured_margins;
    state.hovered = SIZE_MAX;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Flow", 4}));
    draw = cui_end_frame(ui);
    bool sankey_label = false, sankey_value = false;
    cui_chart_sankey_node label_nodes[3];
    assert(cui_chart_sankey_layout(&options, label_nodes, 3) == CUI_PLOT_OK);
    bool first_label_left = false, terminal_label_right = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1) {
            sankey_label |= draw.commands[i].text.data[0] == 'A';
            sankey_value |= draw.commands[i].text.data[0] == 'Q';
            if (draw.commands[i].text.data[0] == 'A')
                first_label_left = draw.commands[i].bounds.x <= label_nodes[0].x0;
            if (draw.commands[i].text.data[0] == 'B')
                terminal_label_right = draw.commands[i].bounds.x >= label_nodes[1].x1;
        }
    assert(sankey_label && sankey_value && first_label_left && terminal_label_right);
    bool overflow_labels = false;
    options.sankey_labels = custom_sankey_lines;
    options.sankey_labels_user = &overflow_labels;
    cui_chart_sankey_margins custom_margins = {0};
    assert(cui_chart_measure_sankey_margins(ui, &options, &custom_margins) == CUI_PLOT_OK);
    assert(custom_margins.left > 0 && custom_margins.right > 0);
    options.sankey_margins = custom_margins;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Flow", 4}));
    draw = cui_end_frame(ui);
    bool first_custom = false, second_custom = false, old_default = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1) {
            char ch = draw.commands[i].text.data[0];
            first_custom |= ch == 'X' && draw.commands[i].color.g == 215;
            second_custom |= ch == 'Y' && draw.commands[i].color.b == 215;
            old_default |= ch == 'A' || ch == 'Q';
        }
    assert(first_custom && second_custom && !old_default);
    overflow_labels = true;
    cui_chart_sankey_margins unchanged = custom_margins;
    assert(cui_chart_measure_sankey_margins(ui, &options, &custom_margins) == CUI_PLOT_INVALID);
    assert(memcmp(&unchanged, &custom_margins, sizeof unchanged) == 0);
    sankey_content_calls content_calls = {0};
    options.sankey_label_measure = sankey_content_measure;
    options.sankey_label_paint = sankey_content_paint;
    options.sankey_label_user = &content_calls;
    assert(cui_chart_measure_sankey_margins(ui, &options, &custom_margins) == CUI_PLOT_OK);
    assert(content_calls.measured == options.count && custom_margins.left > 0 &&
           custom_margins.right > 0);
    options.sankey_margins = custom_margins;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Flow", 4}));
    draw = cui_end_frame(ui);
    size_t custom_pieces = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.g == 120 &&
            fabsf(draw.commands[i].bounds.h - 2) < .01f)
            custom_pieces++;
    assert(content_calls.painted == options.count && custom_pieces >= 20);
    content_calls.invalid = true;
    unchanged = custom_margins;
    assert(cui_chart_measure_sankey_margins(ui, &options, &custom_margins) == CUI_PLOT_INVALID);
    assert(memcmp(&unchanged, &custom_margins, sizeof unchanged) == 0);
    content_calls.invalid = false;
    content_calls.empty = true;
    assert(cui_chart_measure_sankey_margins(ui, &options, &custom_margins) == CUI_PLOT_OK);
    assert(custom_margins.left == 0 && custom_margins.right == 0 && custom_margins.top == 0 &&
           custom_margins.bottom == 0);
    content_calls.empty = false;
    options.sankey_label_paint = NULL;
    assert(cui_chart_measure_sankey_margins(ui, &options, &custom_margins) == CUI_PLOT_INVALID);
    options.sankey_label_measure = NULL;
    options.sankey_label_user = NULL;
    options.sankey_labels = NULL;
    options.sankey_labels_user = NULL;
    options.sankey_margins = (cui_chart_sankey_margins){0};
    options.sankey_show_labels = false;
    options.sankey_show_values = false;
    state.hovered = 0;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 0;
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Flow", 4}));
    draw = cui_end_frame(ui);
    bool tooltip_value_fallback = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            draw.commands[i].text.data[0] == 'Q')
            tooltip_value_fallback = true;
    assert(tooltip_value_fallback);
    options.sankey_value_format = NULL;
    options.logical_width = 64;
    options.logical_height = 48;
    options.kind = CUI_CHART_BAR;
    options.hide_axes = true;
    options.hide_grid = true;
    options.data_revision = 102;
    options.appear_key = 7;
    static unsigned char default_appear_pixels[sizeof pixels] = {0};
    cui_chart_state default_appear = {.pixels = default_appear_pixels,
                                      .byte_capacity = sizeof default_appear_pixels,
                                      .hovered = SIZE_MAX};
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 93, &default_appear, &options, (cui_str){"Auto appear", 11}));
    (void)cui_end_frame(ui);
    assert(default_appear.revision == 1 && default_appear.appear_started &&
           default_appear.rendered_reveal_progress == 0 &&
           !differs(default_appear_pixels, options.width * options.height, options.background));
    assert(cui_begin_frame(ui, 256, 256, .5f));
    assert(cui_chart(ui, 93, &default_appear, &options, (cui_str){"Auto appear", 11}));
    (void)cui_end_frame(ui);
    assert(default_appear.revision == 2 && default_appear.rendered_reveal_progress > 0 &&
           default_appear.rendered_reveal_progress < 1 &&
           differs(default_appear_pixels, options.width * options.height, options.background));
    cui_set_reduced_motion(ui, true);
    assert(cui_begin_frame(ui, 256, 256, .1f));
    assert(cui_chart(ui, 93, &default_appear, &options, (cui_str){"Auto appear", 11}));
    (void)cui_end_frame(ui);
    assert(default_appear.rendered_reveal_progress == 1);
    cui_set_reduced_motion(ui, false);
    cui_chart_state disabled_appear = {.pixels = default_appear_pixels,
                                       .byte_capacity = sizeof default_appear_pixels,
                                       .hovered = SIZE_MAX};
    cui_chart_options no_auto_appear = options;
    no_auto_appear.disable_appear = true;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 93, &disabled_appear, &no_auto_appear, (cui_str){"No appear", 9}));
    (void)cui_end_frame(ui);
    assert(disabled_appear.rendered_reveal_progress == 1 &&
           differs(default_appear_pixels, options.width * options.height, options.background));
    cui_chart_options explicit_clock = options;
    explicit_clock.data_revision = 0;
    cui_chart_state explicit_clock_state = {.pixels = default_appear_pixels,
                                            .byte_capacity = sizeof default_appear_pixels,
                                            .hovered = SIZE_MAX};
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render_animated(ui, &explicit_clock_state, &explicit_clock) == CUI_PLOT_OK);
    uint64_t explicit_revision = explicit_clock_state.revision;
    assert(
        cui_chart(ui, 94, &explicit_clock_state, &explicit_clock, (cui_str){"Explicit clock", 14}));
    (void)cui_end_frame(ui);
    assert(explicit_clock_state.revision == explicit_revision);
    explicit_clock.disable_appear = true;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render_animated(ui, &explicit_clock_state, &explicit_clock) == CUI_PLOT_OK);
    explicit_revision = explicit_clock_state.revision;
    memcpy(saved, default_appear_pixels, sizeof saved);
    explicit_clock.width -= 16;
    assert(
        cui_chart(ui, 94, &explicit_clock_state, &explicit_clock, (cui_str){"Explicit clock", 14}));
    (void)cui_end_frame(ui);
    assert(explicit_clock_state.revision > explicit_revision &&
           memcmp(saved, default_appear_pixels, sizeof saved) != 0);
    explicit_clock.width += 16;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render_animated(ui, &explicit_clock_state, &explicit_clock) == CUI_PLOT_OK);
    explicit_revision = explicit_clock_state.revision;
    memcpy(saved, default_appear_pixels, sizeof saved);
    double original_chart_value = points[0].value;
    points[0].value = original_chart_value + 45;
    assert(
        cui_chart(ui, 94, &explicit_clock_state, &explicit_clock, (cui_str){"Explicit clock", 14}));
    (void)cui_end_frame(ui);
    assert(explicit_clock_state.revision > explicit_revision &&
           memcmp(saved, default_appear_pixels, sizeof saved) != 0);
    points[0].value = original_chart_value;
    explicit_clock.disable_appear = false;
    explicit_clock.appear_key++;
    assert(cui_begin_frame(ui, 256, 256, .5f));
    assert(
        cui_chart(ui, 94, &explicit_clock_state, &explicit_clock, (cui_str){"Explicit clock", 14}));
    (void)cui_end_frame(ui);
    assert(explicit_clock_state.active_appear_key == explicit_clock.appear_key &&
           explicit_clock_state.rendered_reveal_progress == 0);
    cui_chart_options manual_progress = options;
    manual_progress.reveal_enabled = true;
    manual_progress.reveal_progress = .3f;
    cui_chart_state manual_progress_state = {.pixels = default_appear_pixels,
                                             .byte_capacity = sizeof default_appear_pixels,
                                             .hovered = SIZE_MAX};
    assert(cui_chart_render(&manual_progress_state, &manual_progress) == CUI_PLOT_OK);
    assert(cui_begin_frame(ui, 256, 256, .1f));
    assert(cui_chart(ui, 95, &manual_progress_state, &manual_progress,
                     (cui_str){"Manual progress", 15}));
    (void)cui_end_frame(ui);
    assert(!manual_progress_state.appear_started &&
           manual_progress_state.rendered_reveal_progress == .3f);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render_animated(ui, &state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    assert(!differs(pixels, options.width * options.height, options.background));
    (void)cui_end_frame(ui);
    assert(cui_begin_frame(ui, 256, 256, .5f));
    assert(cui_chart_render_animated(ui, &state, &options) == CUI_PLOT_OK);
    float reveal_before_hover = state.rendered_reveal_progress;
    assert(reveal_before_hover > 0 && reveal_before_hover < 1);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    assert(state.rendered_reveal_progress == reveal_before_hover);
    assert(differs(pixels, options.width * options.height, options.background));
    (void)cui_end_frame(ui);
    assert(cui_begin_frame(ui, 256, 256, .5f));
    assert(cui_chart_render_animated(ui, &state, &options) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    options.appear_key++;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render_animated(ui, &state, &options) == CUI_PLOT_OK);
    assert(!differs(pixels, options.width * options.height, options.background));
    (void)cui_end_frame(ui);
    cui_set_reduced_motion(ui, true);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render_animated(ui, &state, &options) == CUI_PLOT_OK);
    assert(differs(pixels, options.width * options.height, options.background));
    (void)cui_end_frame(ui);
    options.logical_width = 200;
    options.logical_height = 100;
    options.data_revision = 0;
    options.show_bar_values = false;
    state.hovered = SIZE_MAX;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    (void)cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = state.bounds.x + 60,
                                           .y = state.bounds.y + 25}));
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    float left_popup_x = NAN;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].bounds.w >= 150 &&
            draw.commands[i].bounds.h >= 48)
            left_popup_x = draw.commands[i].bounds.x;
    assert(state.hovered < options.count && left_popup_x > state.bounds.x + 60 &&
           left_popup_x + 150 > state.bounds.x + state.bounds.w);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = state.bounds.x + 180,
                                           .y = state.bounds.y + 70}));
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    float right_popup_x = NAN, bottom_popup_y = NAN;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].bounds.w >= 150 &&
            draw.commands[i].bounds.h >= 48)
            right_popup_x = draw.commands[i].bounds.x, bottom_popup_y = draw.commands[i].bounds.y;
    assert(state.hovered < options.count && right_popup_x < state.bounds.x + 180 &&
           bottom_popup_y < state.bounds.y + 70);
    cui_set_reduced_motion(ui, false);
    options.disable_appear = true; /* This section isolates hover easing after reveal. */
    state.tooltip_phase = state.tooltip_focus = 0;
    state.tooltip_active = state.tooltip_started = false;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = state.bounds.x + 60,
                                           .y = state.bounds.y + 25}));
    assert(cui_begin_frame(ui, 256, 256, .06f));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    assert(state.tooltip_phase > .49f && state.tooltip_phase < .51f &&
           state.tooltip_focus > state.tooltip_phase && state.tooltip_focus < 1);
    assert(fabsf(state.emphasis_focus - state.tooltip_focus) < .0001f);
    bool faded_tooltip = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].bounds.w >= 150 &&
            draw.commands[i].bounds.h >= 48 && draw.commands[i].color.a > 0 &&
            draw.commands[i].color.a < options.background.a)
            faded_tooltip = true;
    assert(faded_tooltip);
    float entering_focus = state.tooltip_focus;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = state.bounds.x + 2,
                                           .y = state.bounds.y + 2}));
    assert(cui_begin_frame(ui, 256, 256, .01f));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    (void)cui_end_frame(ui);
    assert(state.hovered == SIZE_MAX && state.tooltip_visual_index < options.count &&
           state.tooltip_focus > 0 && state.tooltip_focus < entering_focus &&
           state.tooltip_focus > entering_focus * .8f);
    options.logical_width = 64;
    options.logical_height = 48;
    /* Empty formatter text retains the source row, its label and swatch. */
    cui_chart_datum empty_row_data[] = {{.label = {"Category", 8}, .value = 12.3456},
                                        {.label = {"Other", 5}, .value = 20}};
    double second_row_values[] = {9, 17};
    cui_chart_series empty_row_series = {
        .values = second_row_values, .count = 2, .name = {"Second", 6}, .color = {1, 211, 39, 255}};
    empty_tooltip_calls empty_calls = {0};
    cui_chart_options empty_rows = {.kind = CUI_CHART_LINE,
                                    .data = empty_row_data,
                                    .count = 2,
                                    .width = 128,
                                    .height = 96,
                                    .hide_axes = true,
                                    .hide_grid = true,
                                    .hide_legend = true,
                                    .hide_x_axis = true,
                                    .extra_series = &empty_row_series,
                                    .extra_series_count = 1,
                                    .disable_appear = true,
                                    .reduced_motion = true,
                                    .tooltip_value = empty_tooltip_value,
                                    .tooltip_label = empty_tooltip_label,
                                    .tooltip_value_color = empty_tooltip_color,
                                    .tooltip_user = &empty_calls,
                                    .background = {19, 23, 31, 255},
                                    .foreground = {240, 240, 240, 255},
                                    .series = {{201, 7, 33, 255}, {3, 209, 47, 255}}};
    static unsigned char empty_row_pixels[128 * 96 * 4];
    cui_chart_state empty_state = {.pixels = empty_row_pixels,
                                   .byte_capacity = sizeof empty_row_pixels};
    assert(cui_chart_render(&empty_state, &empty_rows) == CUI_PLOT_OK);
    assert(cui_chart_update_hover(&empty_state, &empty_rows, 0, 20).valid);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 922, &empty_state, &empty_rows, (cui_str){"Rows", 4}));
    draw = cui_end_frame(ui);
    size_t retained_empty_labels = 0;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 11 &&
            memcmp(draw.commands[i].text.data, "Still shown", 11) == 0)
            retained_empty_labels++;
    assert(empty_calls.values == 2 && empty_calls.labels == 2 && empty_calls.colors == 2 &&
           retained_empty_labels == 2);
    empty_rows.tooltip_content = override_empty_tooltip;
    empty_rows.tooltip_title = empty_tooltip_title;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 922, &empty_state, &empty_rows, (cui_str){"Rows", 4}));
    draw = cui_end_frame(ui);
    bool custom_only = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_str text = draw.commands[i].text;
            custom_only |= text.length == 8 && memcmp(text.data, "Override", 8) == 0;
            assert(!(text.length == 11 && memcmp(text.data, "Still shown", 11) == 0));
        }
    assert(custom_only && empty_calls.content == 1 && empty_calls.titles == 0 &&
           empty_calls.values == 2 && empty_calls.labels == 2 && empty_calls.colors == 2);
    empty_rows.tooltip_content = NULL;
    empty_rows.tooltip_title = NULL;
    empty_rows.tooltip_value = tooltip_value_with_unit;
    empty_rows.tooltip_label = NULL;
    empty_rows.tooltip_value_color = NULL;
    empty_rows.tooltip_user = " USD";
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 922, &empty_state, &empty_rows, (cui_str){"Rows", 4}));
    draw = cui_end_frame(ui);
    bool formatted_unit = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 9 &&
            memcmp(draw.commands[i].text.data, "12.35 USD", 9) == 0)
            formatted_unit = true;
    assert(formatted_unit);
    empty_rows.noninteractive = true;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 922, &empty_state, &empty_rows, (cui_str){"Rows", 4}));
    draw = cui_end_frame(ui);
    assert(empty_state.hovered == SIZE_MAX && !empty_state.tooltip_cursor_valid &&
           empty_calls.values == 2);
    empty_rows.noninteractive = false;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 922, &empty_state, &empty_rows, (cui_str){"Rows", 4}));
    (void)cui_end_frame(ui);
    assert(empty_state.hovered == SIZE_MAX && empty_calls.values == 2);
    empty_rows.kind = CUI_CHART_PIE;
    empty_rows.extra_series = NULL;
    empty_rows.extra_series_count = 0;
    empty_rows.tooltip_value = NULL;
    empty_rows.tooltip_label = NULL;
    empty_rows.tooltip_value_color = NULL;
    empty_rows.pie_outer_radius = 30;
    assert(cui_chart_render(&empty_state, &empty_rows) == CUI_PLOT_OK);
    assert(cui_chart_update_hover(&empty_state, &empty_rows, 84, 48).hovered == 0);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 923, &empty_state, &empty_rows, (cui_str){"Shares", 6}));
    draw = cui_end_frame(ui);
    bool exact_pie_value = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length >= 8 &&
            memcmp(draw.commands[i].text.data, "12.3456 ", 8) == 0)
            exact_pie_value = true;
    assert(exact_pie_value);
    assert(cui_chart_update_hover(&empty_state, &empty_rows, 44, 48).hovered == 1);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 923, &empty_state, &empty_rows, (cui_str){"Shares", 6}));
    draw = cui_end_frame(ui);
    bool matching_pie_swatch = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_RECT && fabsf(draw.commands[i].bounds.w - 8) < .01f &&
            draw.commands[i].color.r == 3 && draw.commands[i].color.g == 209 &&
            draw.commands[i].color.b == 47)
            matching_pie_swatch = true;
    assert(matching_pie_swatch);
    /* Source builds Sankey tooltip throughput from current data, including when
     * the pointer stays still between revisions. */
    cui_chart_link changing_link = {.source = 0, .target = 1, .value = 10};
    tooltip_value_capture throughput_capture = {0};
    empty_rows.kind = CUI_CHART_SANKEY;
    empty_rows.links = &changing_link;
    empty_rows.link_count = 1;
    empty_rows.custom_sankey_margins = true;
    empty_rows.tooltip_value = capture_tooltip_value;
    empty_rows.tooltip_user = &throughput_capture;
    cui_chart_sankey_node changing_nodes[2];
    assert(cui_chart_render(&empty_state, &empty_rows) == CUI_PLOT_OK);
    assert(cui_chart_sankey_layout(&empty_rows, changing_nodes, 2) == CUI_PLOT_OK);
    float node_x = (changing_nodes[0].x0 + changing_nodes[0].x1) / 2;
    float node_y = (changing_nodes[0].y0 + changing_nodes[0].y1) / 2;
    assert(cui_chart_update_hover(&empty_state, &empty_rows, node_x, node_y).valid);
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 924, &empty_state, &empty_rows, (cui_str){"Flow", 4}));
    (void)cui_end_frame(ui);
    assert(throughput_capture.calls == 1 && throughput_capture.last == 10);
    changing_link.value = 20;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 924, &empty_state, &empty_rows, (cui_str){"Flow", 4}));
    (void)cui_end_frame(ui);
    assert(throughput_capture.calls == 2 && throughput_capture.last == 20 &&
           empty_state.hovered_value == 20);

    cui_destroy(ui);
#endif
    points[0].value = -DBL_MAX;
    points[1].value = DBL_MAX;
    points[2].value = 0;
    options.has_domain = false;
    options.kind = CUI_CHART_BAR;
    options.bar_alignment = CUI_CHART_BAR_TOP;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    options.kind = CUI_CHART_LINE;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    /* Large geometry must not require an extended long-double exponent range. */
    flow_links[0].value = 1;
    flow_links[1].value = 1;
    flow_links[2].value = 0;
    cui_chart_sankey_node unit_nodes[4];
    cui_chart_sankey_link_geometry unit_links[3], extreme_links[3];
    assert(cui_chart_sankey_layout(&flow_options, unit_nodes, 4) == CUI_PLOT_OK);
    assert(cui_chart_sankey_link_bounds(&flow_options, unit_nodes, unit_links, 3) == CUI_PLOT_OK);
    flow_links[0].value = DBL_MAX;
    flow_links[1].value = DBL_MAX;
    flow_links[2].value = 0;
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    assert(cui_chart_sankey_link_bounds(&flow_options, nodes, extreme_links, 3) == CUI_PLOT_OK);
    for (size_t i = 0; i < 4; ++i) {
        assert(nodes[i].x0 == unit_nodes[i].x0 && nodes[i].x1 == unit_nodes[i].x1);
        assert(nodes[i].y0 == unit_nodes[i].y0 && nodes[i].y1 == unit_nodes[i].y1);
        assert(nodes[i].throughput == (i == 3 ? 0 : DBL_MAX));
    }
    for (size_t i = 0; i < 3; ++i) {
        assert(extreme_links[i].source_y == unit_links[i].source_y);
        assert(extreme_links[i].target_y == unit_links[i].target_y);
        assert(extreme_links[i].source_width == unit_links[i].source_width);
        assert(extreme_links[i].target_width == unit_links[i].target_width);
    }
    return 0;
}
