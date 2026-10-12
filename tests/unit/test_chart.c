/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <assert.h>
#include <float.h>
#include <math.h>
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
#endif
int main(void) {
    cui_chart_datum points[] = {
        {.label = {"A", 1}, .value = 20, .open = 15, .high = 25, .low = 10, .close = 20},
        {.label = {"B", 1}, .value = 40, .open = 20, .high = 45, .low = 15, .close = 30},
        {.label = {"C", 1}, .value = 30, .open = 30, .high = 40, .low = 25, .close = 28},
    };
    cui_chart_link links[] = {{0, 1, 20, {0}}, {0, 2, 10, {0}}};
    unsigned char pixels[128 * 96 * 4];
    unsigned char saved[sizeof pixels];
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
    assert(!cui_chart_hit_test(&options, 0, 20).valid);
    assert(cui_chart_hit_test(&options, 16, 20).valid &&
           cui_chart_hit_test(&options, 16, 20).hovered == 0);
    options.show_y_axis = false;
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
    double first_center = (nodes[0].y0 + nodes[0].y1) / 2;
    double second_center = (nodes[1].y0 + nodes[1].y1) / 2;
    assert(fabs(first_center - 48) < .01 && second_center > first_center + .5);
    double last_column_center =
        (fmin(nodes[2].y0, nodes[3].y0) + fmax(nodes[2].y1, nodes[3].y1)) / 2;
    assert(fabs(last_column_center - 48) < .01);
    float no_margin_left = nodes[0].x0, no_margin_right = nodes[3].x0;
    flow_options.sankey_margins = (cui_chart_sankey_margins){20, 20, 40, 4};
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    assert(nodes[0].x0 >= no_margin_left + 19 && nodes[3].x0 <= no_margin_right - 19);
    for (size_t i = 0; i < 4; i++)
        assert(nodes[i].y0 >= 47 && nodes[i].y1 <= 84.01);
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
    unsigned char transparent_links[sizeof pixels];
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
        assert(nodes[i].y0 >= 8 && nodes[i].y1 <= flow_options.height - 8);
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
    unsigned char natural[sizeof pixels];
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
    assert(first_bar.x_min > 24 && first_bar.x_max < second_bar.x_min);
    assert(fabs((first_bar.x_max - first_bar.x_min) - 18.4) < .01);
    options.point_count = 5;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
    assert(fabs((first_bar.x_max - first_bar.x_min) - 11.04) < .01);
    options.point_count = 0;
    options.custom_bar_padding = true;
    options.bar_padding_inner = 0;
    options.bar_padding_outer = 0;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
    assert(first_bar.x_min == 24 && first_bar.x_max - first_bar.x_min == 30);
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
    unsigned char bottom_bars[sizeof pixels];
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
    options.bar_alignment = CUI_CHART_BAR_BOTTOM;
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
    assert(cui_chart_render(&state, &options) == CUI_PLOT_INVALID);
    assert(state.revision == old_revision && memcmp(pixels, saved, sizeof pixels) == 0);
    options.tick_count = 0;
    state.byte_capacity = sizeof pixels - 1;
    assert(cui_chart_render(&state, &options) == CUI_PLOT_CAPACITY);
    assert(state.revision == old_revision && memcmp(pixels, saved, sizeof pixels) == 0);
    state.byte_capacity = sizeof pixels;
#ifndef CUI_CHART_BASE_ONLY
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
    options.kind = CUI_CHART_LINE;
    options.show_y_axis = true;
    options.x_tick_count = 2;
    options.y_tick_format = format_q;
    options.tooltip_format = format_q;
    float short_gutter = cui_chart_measure_value_axis_gutter(ui, &options);
    options.y_tick_format = format_wide_tick;
    float wide_gutter = cui_chart_measure_value_axis_gutter(ui, &options);
    assert(short_gutter >= 32 && wide_gutter > short_gutter);
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
         hover_halo = false, eased_crosshair = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE) {
            image = true;
            assert(draw.commands[i].bitmap.bitmap.pixels == pixels);
            assert(draw.commands[i].bounds.w == 64 && draw.commands[i].bounds.h == 48);
        } else if (draw.commands[i].type == CUI_DRAW_TEXT) {
            text = true;
            if (draw.commands[i].text.length == 1 && draw.commands[i].text.data[0] == 'Q')
                formatted = true;
        } else if (draw.commands[i].type == CUI_DRAW_RECT) {
            hover_crosshair |= draw.commands[i].bounds.w == 1 && draw.commands[i].bounds.h > 20;
            if (draw.commands[i].bounds.w == 1 && draw.commands[i].bounds.h > 20)
                eased_crosshair |=
                    fabsf(draw.commands[i].color.a -
                          (options.grid.a ? options.grid.a : 150) * state.tooltip_focus) <= 1;
            hover_halo |= draw.commands[i].bounds.w == 10 && draw.commands[i].bounds.h == 10;
        }
    assert(image && text && formatted && hover_crosshair && hover_halo && eased_crosshair);
    assert(state.hover_progress > 0 && state.hover_progress < 1);
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
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].bounds.h == 32 &&
            draw.commands[i].color.a > 0 && draw.commands[i].color.a <= 25)
            hover_band = true;
    assert(hover_band);
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
            draw.commands[i].text.data[0] == 'Q' && draw.commands[i].bounds.y >= 13 &&
            draw.commands[i].bounds.y <= 17) {
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
    options.tooltip_format = NULL;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    state.hovered = 0;
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
    draw = cui_end_frame(ui);
    bool ohlc[4] = {0};
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length >= 3) {
            const char *label = draw.commands[i].text.data;
            for (size_t row = 0; row < 4; row++)
                if (label[0] == "OHLC"[row] && label[1] == ':' && label[2] == ' ')
                    ohlc[row] = true;
        }
    assert(ohlc[0] && ohlc[1] && ohlc[2] && ohlc[3]);
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
            if (label.length >= 3 && label.data[0] == 'O' && label.data[1] == ':')
                default_ohlc = true;
        }
    assert(tooltip_calls == 1 && custom && !default_ohlc);
    options.tooltip_content = NULL;
    options.tooltip_title = tooltip_title_override;
    options.tooltip_value = tooltip_value_override;
    options.tooltip_value_color = tooltip_color_override;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Price", 5}));
    draw = cui_end_frame(ui);
    bool overridden_title = false, overridden_value = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            cui_str label = draw.commands[i].text;
            overridden_title |= label.length == 5 && memcmp(label.data, "Price", 5) == 0 &&
                                draw.commands[i].text_style.weight == CUI_TEXT_WEIGHT_SEMIBOLD;
            overridden_value |= label.length == 4 && memcmp(label.data, "O: V", 4) == 0 &&
                                draw.commands[i].color.g == 210;
        }
    assert(overridden_title && overridden_value);
    options.tooltip_title = NULL;
    options.tooltip_value = NULL;
    options.tooltip_value_color = NULL;
    options.pie_show_labels = true;
    options.pie_outer_radius = 20;
    options.pie_label_gap = 5;
    options.logical_width = 128;
    options.logical_height = 96;
    options.kind = CUI_CHART_PIE;
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Shares", 6}));
    draw = cui_end_frame(ui);
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
    bool radar_colored_label = false, radar_hover_dot = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            draw.commands[i].color.r == 220 && draw.commands[i].color.g == 10)
            radar_colored_label = true;
        else if (draw.commands[i].type == CUI_DRAW_RECT &&
                 fabsf(draw.commands[i].bounds.w - 6) < .01f &&
                 fabsf(draw.commands[i].bounds.h - 6) < .01f &&
                 draw.commands[i].color.r == options.series[0].r)
            radar_hover_dot = true;
    assert(radar_colored_label);
    assert(radar_hover_dot);
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
    options.sankey_labels = NULL;
    options.sankey_labels_user = NULL;
    options.sankey_margins = (cui_chart_sankey_margins){0};
    options.sankey_show_labels = false;
    options.sankey_show_values = false;
    options.sankey_value_format = NULL;
    options.logical_width = 64;
    options.logical_height = 48;
    options.kind = CUI_CHART_BAR;
    options.hide_axes = true;
    options.hide_grid = true;
    options.data_revision = 102;
    options.appear_key = 7;
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
    flow_links[0].value = DBL_MAX;
    flow_links[1].value = DBL_MAX;
    flow_links[2].value = 0;
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    return 0;
}
