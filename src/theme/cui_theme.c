/* SPDX-License-Identifier: MIT */
#include "cui/cui_theme.h"
#define CUI_THEME_RGB(r, g, b) {r, g, b, 255}
static cui_color cui_theme_scale(cui_color c, float factor) {
    c.r = (uint8_t)(c.r * factor);
    c.g = (uint8_t)(c.g * factor);
    c.b = (uint8_t)(c.b * factor);
    return c;
}
static cui_color cui_theme_opacity(cui_color c, float opacity) {
    c.a = (uint8_t)(c.a * opacity);
    return c;
}
static cui_theme cui_theme_control_defaults(cui_theme t) {
    cui_color secondary = {(uint8_t)((3 * t.surface.r + t.border.r) / 4),
                           (uint8_t)((3 * t.surface.g + t.border.g) / 4),
                           (uint8_t)((3 * t.surface.b + t.border.b) / 4), 255};
    cui_color backgrounds[] = {t.surface, t.primary, secondary, t.danger,
                               t.warning, t.success, t.info};
    cui_color foregrounds[] = {t.text,       t.on_primary, t.text,   t.on_danger,
                               t.on_warning, t.on_success, t.on_info};
    for (size_t i = 0; i < CUI_THEME_BUTTON_COUNT; i++) {
        cui_color normal = backgrounds[i], hover = cui_theme_scale(normal, .9f),
                  active = cui_theme_scale(normal, .75f);
        t.buttons[i] = (cui_theme_button_colors){
            .normal = normal,
            .hover = hover,
            .active = active,
            .foreground = foregrounds[i],
            .outline_normal = cui_theme_opacity(normal, .1f),
            .outline_hover = cui_theme_opacity(hover, .2f),
            .outline_active = cui_theme_opacity(active, .4f),
            .border = normal,
            .outline_border = normal,
            .outline_foreground =
                i == CUI_THEME_BUTTON_DEFAULT || i == CUI_THEME_BUTTON_SECONDARY ? t.text : normal};
    }
    t.input = t.border;
    t.input_background = t.surface;
    t.buttons[CUI_THEME_BUTTON_DEFAULT].border = t.input;
    t.buttons[CUI_THEME_BUTTON_DEFAULT].outline_border = t.input;
    t.buttons[CUI_THEME_BUTTON_DEFAULT].outline_normal = t.input_background;
    t.buttons[CUI_THEME_BUTTON_DEFAULT].outline_hover = cui_theme_opacity(t.input, .5f);
    t.buttons[CUI_THEME_BUTTON_DEFAULT].outline_active = cui_theme_opacity(t.input, .7f);
    t.link = t.primary;
    t.link_hover = cui_theme_scale(t.primary, .9f);
    t.link_active = cui_theme_scale(t.primary, .75f);
    t.secondary_foreground = t.text;
    t.accent = t.border;
    t.accent_foreground = t.text;
    t.spring_control =
        (cui_spring_settings){.response = .18f, .damping = 1, .epsilon = .001f, .travel = true};
    t.popover = t.surface;
    t.radius_lg = t.radius == 0 ? 0 : t.radius + 2;
    return t;
}
cui_theme cui_theme_light(void) {
    return cui_theme_control_defaults((cui_theme){CUI_THEME_RGB(246, 248, 250),
                                                  CUI_THEME_RGB(255, 255, 255),
                                                  CUI_THEME_RGB(24, 32, 42),
                                                  CUI_THEME_RGB(87, 99, 112),
                                                  CUI_THEME_RGB(23, 91, 169),
                                                  CUI_THEME_RGB(255, 255, 255),
                                                  CUI_THEME_RGB(181, 191, 204),
                                                  CUI_THEME_RGB(179, 35, 39),
                                                  CUI_THEME_RGB(23, 91, 169),
                                                  16,
                                                  10,
                                                  8,
                                                  6,
                                                  42,
                                                  CUI_THEME_RGB(133, 77, 14),
                                                  CUI_THEME_RGB(255, 255, 255),
                                                  CUI_THEME_RGB(22, 101, 52),
                                                  CUI_THEME_RGB(255, 255, 255),
                                                  CUI_THEME_RGB(7, 89, 133),
                                                  CUI_THEME_RGB(255, 255, 255),
                                                  CUI_THEME_RGB(255, 255, 255),
                                                  .spring_control = {0}});
}
cui_theme cui_theme_dark(void) {
    return cui_theme_control_defaults((cui_theme){CUI_THEME_RGB(18, 23, 30),
                                                  CUI_THEME_RGB(30, 38, 48),
                                                  CUI_THEME_RGB(235, 240, 246),
                                                  CUI_THEME_RGB(164, 176, 190),
                                                  CUI_THEME_RGB(116, 178, 250),
                                                  CUI_THEME_RGB(14, 24, 38),
                                                  CUI_THEME_RGB(102, 118, 137),
                                                  CUI_THEME_RGB(255, 137, 142),
                                                  CUI_THEME_RGB(164, 209, 255),
                                                  16,
                                                  10,
                                                  8,
                                                  6,
                                                  42,
                                                  CUI_THEME_RGB(250, 204, 21),
                                                  CUI_THEME_RGB(14, 24, 38),
                                                  CUI_THEME_RGB(74, 222, 128),
                                                  CUI_THEME_RGB(14, 24, 38),
                                                  CUI_THEME_RGB(56, 189, 248),
                                                  CUI_THEME_RGB(14, 24, 38),
                                                  CUI_THEME_RGB(14, 24, 38),
                                                  .spring_control = {0}});
}
cui_theme cui_theme_high_contrast(void) {
    return cui_theme_control_defaults((cui_theme){CUI_THEME_RGB(0, 0, 0),
                                                  CUI_THEME_RGB(0, 0, 0),
                                                  CUI_THEME_RGB(255, 255, 255),
                                                  CUI_THEME_RGB(255, 255, 255),
                                                  CUI_THEME_RGB(255, 255, 0),
                                                  CUI_THEME_RGB(0, 0, 0),
                                                  CUI_THEME_RGB(255, 255, 255),
                                                  CUI_THEME_RGB(255, 120, 120),
                                                  CUI_THEME_RGB(0, 255, 255),
                                                  18,
                                                  10,
                                                  10,
                                                  0,
                                                  46,
                                                  CUI_THEME_RGB(255, 255, 0),
                                                  CUI_THEME_RGB(0, 0, 0),
                                                  CUI_THEME_RGB(0, 255, 128),
                                                  CUI_THEME_RGB(0, 0, 0),
                                                  CUI_THEME_RGB(0, 255, 255),
                                                  CUI_THEME_RGB(0, 0, 0),
                                                  CUI_THEME_RGB(0, 0, 0),
                                                  .spring_control = {0}});
}
cui_theme cui_theme_terminal(void) {
    return cui_theme_control_defaults((cui_theme){CUI_THEME_RGB(31, 29, 23),
                                                  CUI_THEME_RGB(45, 42, 33),
                                                  CUI_THEME_RGB(255, 227, 164),
                                                  CUI_THEME_RGB(205, 182, 140),
                                                  CUI_THEME_RGB(255, 192, 77),
                                                  CUI_THEME_RGB(31, 29, 23),
                                                  CUI_THEME_RGB(148, 126, 84),
                                                  CUI_THEME_RGB(255, 137, 103),
                                                  CUI_THEME_RGB(255, 227, 164),
                                                  16,
                                                  12,
                                                  6,
                                                  0,
                                                  44,
                                                  CUI_THEME_RGB(255, 192, 77),
                                                  CUI_THEME_RGB(31, 29, 23),
                                                  CUI_THEME_RGB(168, 212, 128),
                                                  CUI_THEME_RGB(31, 29, 23),
                                                  CUI_THEME_RGB(145, 205, 232),
                                                  CUI_THEME_RGB(31, 29, 23),
                                                  CUI_THEME_RGB(31, 29, 23),
                                                  .spring_control = {0}});
}

#undef CUI_THEME_RGB
