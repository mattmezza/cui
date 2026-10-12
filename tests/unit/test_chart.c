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
#ifndef CUI_CHART_BASE_ONLY
static size_t format_q(double value, char *buffer, size_t capacity, void *user) {
    (void)value;
    (void)user;
    if (capacity)
        buffer[0] = 'Q';
    return capacity ? 1 : 0;
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
    unsigned char saved[sizeof pixels];
    uint64_t old_revision;
    cui_chart_datum flow[] = {
        {.label = {"A", 1}}, {.label = {"B", 1}}, {.label = {"C", 1}}, {.label = {"D", 1}}};
    cui_chart_link flow_links[] = {{0, 1, 30, {0}}, {1, 2, 20, {0}}, {1, 3, 10, {0}}};
    cui_chart_options flow_options = options;
    flow_options.kind = CUI_CHART_SANKEY;
    flow_options.data = flow;
    flow_options.count = 4;
    flow_options.links = flow_links;
    flow_options.link_count = 3;
    cui_chart_sankey_node nodes[4];
    assert(cui_chart_sankey_layout(&flow_options, nodes, 3) == CUI_PLOT_CAPACITY);
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK);
    assert(nodes[0].depth == 0 && nodes[1].depth == 1 && nodes[2].depth == 2 &&
           nodes[3].depth == 2);
    assert(nodes[0].throughput == 30 && nodes[1].throughput == 30 && nodes[2].throughput == 20 &&
           nodes[3].throughput == 10);
    assert(nodes[0].x1 < nodes[1].x0 && nodes[1].x1 < nodes[2].x0);
    assert(nodes[2].y1 <= nodes[3].y0);
    cui_chart_hit node_hit = cui_chart_hit_test(&flow_options, (nodes[1].x0 + nodes[1].x1) / 4,
                                                (nodes[1].y0 + nodes[1].y1) / 4);
    assert(node_hit.valid && node_hit.hovered == 1 && node_hit.value == 30);
    flow_options.sankey_align = CUI_CHART_SANKEY_LEFT;
    assert(cui_chart_sankey_layout(&flow_options, nodes, 4) == CUI_PLOT_OK && nodes[3].layer == 2);
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
    assert(green_pixel(pixels, options.width * options.height));
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
    cui_plot_rect first_bar, second_bar;
    assert(cui_chart_bar_bounds(&options, 0, &first_bar) == CUI_PLOT_OK);
    assert(cui_chart_bar_bounds(&options, 1, &second_bar) == CUI_PLOT_OK);
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
    options.kind = CUI_CHART_PIE;
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
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    cui_draw_list draw = cui_end_frame(ui);
    bool image = false, text = false, formatted = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_IMAGE) {
            image = true;
            assert(draw.commands[i].bitmap.bitmap.pixels == pixels);
            assert(draw.commands[i].bounds.w == 64 && draw.commands[i].bounds.h == 48);
        } else if (draw.commands[i].type == CUI_DRAW_TEXT) {
            text = true;
            if (draw.commands[i].text.length == 1 && draw.commands[i].text.data[0] == 'Q')
                formatted = true;
        }
    assert(image && text && formatted);
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
    assert(cui_begin_frame(ui, 256, 256, 0));
    assert(cui_chart_render(&state, &options) == CUI_PLOT_OK);
    assert(cui_chart(ui, 91, &state, &options, (cui_str){"Revenue", 7}));
    draw = cui_end_frame(ui);
    formatted = false;
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT && draw.commands[i].text.length == 1 &&
            draw.commands[i].text.data[0] == 'Q')
            formatted = true;
    assert(formatted);
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
