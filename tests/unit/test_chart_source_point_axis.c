/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static void geometry(void) {
    cui_chart_datum data[] = {{.label = {"A", 1}, .value = 0},
                              {.label = {"B", 1}, .value = 10},
                              {.label = {"C", 1}, .value = 0}};
    unsigned char pixels[128 * 96 * 4];
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels};
    cui_chart_options o = {.kind = CUI_CHART_LINE,
                           .data = data,
                           .count = 3,
                           .width = 128,
                           .height = 96,
                           .has_domain = true,
                           .domain_min = 0,
                           .domain_max = 10,
                           .series = {{255, 0, 0, 255}},
                           .background = {255, 255, 255, 255},
                           .data_revision = 1};
    size_t slots = 99;
    float gutter = 99, x = -1, y = -1;
    assert(cui_chart_point_axis_layout(&o, &slots, &gutter) == CUI_PLOT_OK);
    assert(slots == 3 && gutter == 20);
    assert(cui_chart_point_coordinates(&o, 2, 0, &x, &y) == CUI_PLOT_OK);
    assert(x == 128 && y == 76);
    assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
    uint64_t legacy_key = state.cache_key;
    o.layout_mode = CUI_CHART_LAYOUT_SOURCE_POINT;
    o.point_count = 1;
    assert(cui_chart_point_axis_layout(&o, &slots, &gutter) == CUI_PLOT_OK);
    assert(slots == 3 && gutter == 18);
    assert(cui_chart_point_coordinates(&o, 2, 0, &x, &y) == CUI_PLOT_OK);
    assert(x == 128 && y == 78);
    assert(cui_chart_hit_test(&o, 127, 77).hovered == 2);
    assert(cui_chart_render(&state, &o) == CUI_PLOT_OK && state.cache_key != legacy_key);
    uint64_t source_key = state.cache_key, revision = state.revision;
    o.point_count = 2;
    assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
    assert(state.cache_key == source_key && state.revision == revision);
    o.point_count = 5;
    assert(cui_chart_point_coordinates(&o, 2, 0, &x, &y) == CUI_PLOT_OK && x == 64);
    assert(cui_chart_hit_test(&o, 64, 78).hovered == 2);
    o.hide_x_axis = true;
    assert(cui_chart_point_axis_layout(&o, &slots, &gutter) == CUI_PLOT_OK && gutter == 0);
    assert(cui_chart_point_coordinates(&o, 2, 0, &x, &y) == CUI_PLOT_OK && y == 96);
    o.layout_mode = CUI_CHART_LAYOUT_LEGACY;
    o.point_count = 1;
    slots = 99;
    gutter = 99;
    x = -1;
    y = -1;
    assert(cui_chart_point_axis_layout(&o, &slots, &gutter) == CUI_PLOT_INVALID);
    assert(slots == 99 && gutter == 99);
    assert(cui_chart_point_coordinates(&o, 0, 0, &x, &y) == CUI_PLOT_INVALID && x == -1 && y == -1);
    assert(cui_chart_render(&state, &o) == CUI_PLOT_INVALID);
    /* Raw legacy hit geometry intentionally keeps its historical fallback. */
    assert(cui_chart_hit_test(&o, 64, 50).hovered == 0);
    o.layout_mode = (cui_chart_layout_mode)99;
    assert(cui_chart_point_axis_layout(&o, &slots, &gutter) == CUI_PLOT_INVALID);
    assert(cui_chart_render(&state, &o) == CUI_PLOT_INVALID);
    o.layout_mode = CUI_CHART_LAYOUT_SOURCE_POINT;
    o.kind = CUI_CHART_BAR;
    assert(cui_chart_point_axis_layout(&o, &slots, &gutter) == CUI_PLOT_INVALID);
    assert(cui_chart_render(&state, &o) == CUI_PLOT_INVALID);
    o.kind = CUI_CHART_AREA;
    o.point_count = 1000001;
    assert(cui_chart_point_axis_layout(&o, &slots, &gutter) == CUI_PLOT_INVALID);
    assert(slots == 99 && gutter == 99);
    o.point_count = 0;
    o.count = 0;
    assert(cui_chart_point_axis_layout(&o, &slots, &gutter) == CUI_PLOT_INVALID);
    o.count = 4097;
    assert(cui_chart_point_axis_layout(&o, &slots, &gutter) == CUI_PLOT_INVALID);
    o.count = 3;
    o.hide_x_axis = false;
    o.custom_y_padding = true;
    o.y_padding_top = NAN;
    unsigned char before[sizeof pixels];
    memcpy(before, pixels, sizeof pixels);
    revision = state.revision;
    source_key = state.cache_key;
    assert(cui_chart_render(&state, &o) == CUI_PLOT_INVALID);
    assert(!memcmp(before, pixels, sizeof pixels));
    assert(state.revision == revision && state.cache_key == source_key);
    o.y_padding_top = 0;
    o.y_padding_bottom = 78;
    assert(cui_chart_render(&state, &o) == CUI_PLOT_INVALID);
    assert(!memcmp(before, pixels, sizeof pixels));
}
static cui_size measure(void *user, cui_str text, float font, cui_text_style style) {
    (void)user;
    (void)style;
    return (cui_size){text.length * font * .5f, font};
}
static void styled(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 1024,
                                              .frame_bytes = 65536,
                                              .measure_styled = measure});
    assert(ui);
    unsigned char pixels[128 * 100 * 4];
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    cui_chart_datum data[] = {{.label = {"A", 1}, .value = 0},
                              {.label = {"B", 1}, .value = 10},
                              {.label = {"C", 1}, .value = 0}};
    cui_chart_options o = {.kind = CUI_CHART_LINE,
                           .data = data,
                           .count = 3,
                           .point_count = 1,
                           .width = 128,
                           .height = 100,
                           .disable_appear = true,
                           .reduced_motion = true,
                           .layout_mode = CUI_CHART_LAYOUT_SOURCE_POINT,
                           .interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME};
    for (unsigned mode = 0; mode < 2; mode++) {
        o.layout_mode = mode ? CUI_CHART_LAYOUT_LEGACY : CUI_CHART_LAYOUT_SOURCE_POINT;
        if (mode)
            o.point_count = 3;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 20, .y = 20}));
        assert(cui_begin_frame(ui, 200, 150, .016f));
        assert(cui_chart(ui, 1, &state, &o, cui_string("Chart")));
        cui_draw_list draw = cui_end_frame(ui);
        bool first = false, last = false, crosshair = false;
        for (size_t i = 0; i < draw.count; i++) {
            const cui_draw_command *c = &draw.commands[i];
            if (c->type == CUI_DRAW_RECT && c->bounds.w == 1 && c->bounds.y == 78) {
                assert(c->bounds.h == (mode ? 2 : 4));
                crosshair = true;
            }
            if (c->type != CUI_DRAW_TEXT || c->text.length != 1 ||
                c->text_style.weight == CUI_TEXT_WEIGHT_SEMIBOLD)
                continue;
            if (c->text.data[0] == 'A') {
                assert(c->bounds.x == 0);
                first = true;
            } else if (c->text.data[0] == 'C') {
                assert(c->bounds.x == 123);
                last = true;
            } else
                continue;
            assert(c->bounds.y == (mode ? 86 : 88));
        }
        assert(first && last && crosshair && cui_last_error(ui) == 0);
    }
    assert(cui_begin_frame(ui, 200, 150, .016f));
    uint64_t revision = state.revision;
    o.layout_mode = (cui_chart_layout_mode)99;
    assert(!cui_chart(ui, 1, &state, &o, cui_string("Chart")));
    assert(state.revision == revision);
    assert(cui_end_frame(ui).count == 0);
    cui_destroy(ui);
}
int main(void) {
    geometry();
    styled();
    return 0;
}
