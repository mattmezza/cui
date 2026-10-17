/* SPDX-License-Identifier: MIT */
#define main icon_style_contract
#include "../unit/test_icon_style_source.c"
#undef main
static cui_context *probe_ui;
static cui_software *probe_renderer;
static cui_icon_state probe_icon;
static cui_rect probe_bounds;
static size_t probe_semantics;
int icon_style_init(void) {
    if (icon_style_contract())
        return 0;
    probe_ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    probe_renderer = cui_software_create(160, 100, NULL);
    return probe_ui && probe_renderer && cui_icon_set_data(&probe_icon, cui_string(svg));
}
int icon_style_frame(unsigned mode) {
    cui_theme theme = cui_theme_light();
    cui_box style = {.padding = mode == 1 ? 24 : 4, .max_width = mode == 2 ? 8 : 0};
    if (mode == 2)
        style.padding = 0;
    if (!cui_begin_frame(probe_ui, 160, 100, 0))
        return 0;
    cui_begin_box(probe_ui, 1,
                  (cui_box){.width = mode == 4 ? 8 : 160,
                            .height = 100,
                            .row = true,
                            .clip = mode == 4,
                            .custom_opacity = true,
                            .opacity = mode == 4 ? .5f : 1});
    for (unsigned i = 0; i < (mode == 3 ? 3u : 1u); i++) {
        cui_icon_options o = {.size = CUI_ICON_CUSTOM,
                              .custom_size = mode == 4 ? 64 : 16 + 8 * i,
                              .style = &style,
                              .custom_color = true,
                              .color = {i == 0 ? 255 : 0, i == 1 ? 255 : 0, i == 2 ? 255 : 0, 255},
                              .custom_transform = true,
                              .transform = cui_icon_rotation(i * 1.57079632679f),
                              .accessible_name = {"Visible icon", 12}};
        if (cui_icon(probe_ui, 10 + i, &theme, &probe_icon, o) != CUI_ICON_OK)
            return 0;
    }
    cui_end_box(probe_ui);
    cui_draw_list list = cui_end_frame(probe_ui);
    probe_bounds = cui_bounds(probe_ui, 10);
    cui_semantics(probe_ui, &probe_semantics);
    return cui_last_error(probe_ui) == CUI_OK &&
           cui_software_render(probe_renderer, &list, (cui_color){0, 0, 0, 255});
}
const unsigned char *icon_style_pixels(void) {
    unsigned w, h;
    return cui_software_pixels(probe_renderer, &w, &h);
}
float icon_style_stat(unsigned n) {
    return n == 0 ? probe_bounds.w : n == 1 ? probe_bounds.h : (float)probe_semantics;
}
