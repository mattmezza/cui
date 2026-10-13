/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
static unsigned char pixels[128 * 96 * 4];
static cui_chart_state state;
static cui_chart_datum data[] = {{.value = 0}, {.value = 10}, {.value = 0}};
int probe_render(int source, int count) {
    state = (cui_chart_state){.pixels = pixels, .byte_capacity = sizeof pixels};
    cui_chart_options o = {.kind = CUI_CHART_LINE,
                           .data = data,
                           .count = 3,
                           .point_count = (size_t)count,
                           .width = 128,
                           .height = 96,
                           .layout_mode =
                               source ? CUI_CHART_LAYOUT_SOURCE_POINT : CUI_CHART_LAYOUT_LEGACY,
                           .has_domain = true,
                           .domain_min = 0,
                           .domain_max = 10,
                           .series = {{255, 0, 0, 255}},
                           .background = {255, 255, 255, 255},
                           .line_width = 2,
                           .hide_axes = true};
    return cui_chart_render(&state, &o);
}
unsigned char *probe_pixels(void) {
    return pixels;
}
