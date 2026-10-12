/* SPDX-License-Identifier: MIT */
#ifndef CUI_THEME_H
#define CUI_THEME_H
#include "cui.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    CUI_THEME_BUTTON_DEFAULT,
    CUI_THEME_BUTTON_PRIMARY,
    CUI_THEME_BUTTON_SECONDARY,
    CUI_THEME_BUTTON_DANGER,
    CUI_THEME_BUTTON_WARNING,
    CUI_THEME_BUTTON_SUCCESS,
    CUI_THEME_BUTTON_INFO,
    CUI_THEME_BUTTON_COUNT
} cui_theme_button_kind;
typedef struct {
    cui_color normal, hover, active, foreground;
    cui_color outline_normal, outline_hover, outline_active, border, outline_border,
        outline_foreground;
} cui_theme_button_colors;
typedef struct {
    float response, damping, epsilon;
    bool travel;
} cui_spring_settings;
typedef struct {
    cui_color background, surface, text, muted, primary, on_primary, border, danger, focus;
    float font_size, padding, gap, radius, control_height;
    /* Semantic intent surfaces and their readable foregrounds. */
    cui_color warning, on_warning, success, on_success, info, on_info, on_danger;
    cui_theme_button_colors buttons[CUI_THEME_BUTTON_COUNT];
    cui_color link, link_hover, link_active, input, input_background;
    cui_color secondary_foreground, accent, accent_foreground;
    cui_spring_settings spring_control;
} cui_theme;
cui_theme cui_theme_light(void);
cui_theme cui_theme_dark(void);
cui_theme cui_theme_high_contrast(void);
/* Independent, square-cornered amber/ink design system using the same base. */
cui_theme cui_theme_terminal(void);
#ifdef __cplusplus
}
#endif
#endif
