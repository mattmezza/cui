/* SPDX-License-Identifier: MIT */
#ifndef CUI_STATUS_H
#define CUI_STATUS_H
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    CUI_TAG_SECONDARY,
    CUI_TAG_PRIMARY,
    CUI_TAG_DANGER,
    CUI_TAG_SUCCESS,
    CUI_TAG_WARNING,
    CUI_TAG_INFO,
    CUI_TAG_CUSTOM,
    CUI_TAG_PALETTE
} cui_tag_variant;
typedef enum {
    CUI_STATUS_MEDIUM,
    CUI_STATUS_XSMALL,
    CUI_STATUS_SMALL,
    CUI_STATUS_LARGE
} cui_status_size;
typedef enum {
    CUI_STATUS_COLOR_WHITE,
    CUI_STATUS_COLOR_BLACK,
    CUI_STATUS_COLOR_NEUTRAL,
    CUI_STATUS_COLOR_GRAY,
    CUI_STATUS_COLOR_RED,
    CUI_STATUS_COLOR_ORANGE,
    CUI_STATUS_COLOR_AMBER,
    CUI_STATUS_COLOR_YELLOW,
    CUI_STATUS_COLOR_LIME,
    CUI_STATUS_COLOR_GREEN,
    CUI_STATUS_COLOR_EMERALD,
    CUI_STATUS_COLOR_TEAL,
    CUI_STATUS_COLOR_CYAN,
    CUI_STATUS_COLOR_SKY,
    CUI_STATUS_COLOR_BLUE,
    CUI_STATUS_COLOR_INDIGO,
    CUI_STATUS_COLOR_VIOLET,
    CUI_STATUS_COLOR_PURPLE,
    CUI_STATUS_COLOR_FUCHSIA,
    CUI_STATUS_COLOR_PINK,
    CUI_STATUS_COLOR_ROSE
} cui_status_color;
/* Frozen default-colors palette; false leaves output untouched. */
/* Palette scales are supplied by the application, preserving its named palette.
 * These are the exact scales the reference uses for light/dark tags. */
typedef struct {
    cui_color scale50, scale200, scale300, scale600, scale800, scale950;
    bool dark;
} cui_tag_palette;
bool cui_tag_named_palette(cui_status_color name, bool dark, cui_tag_palette *palette);
typedef struct {
    cui_tag_variant variant;
    cui_status_size size;
    bool outline, custom_radius, pill;
    float radius;
    cui_color background, foreground, border;
    cui_tag_palette palette;
} cui_tag_options;
typedef struct {
    cui_color background, foreground, border;
    float font_size, padding_x, padding_y, radius;
} cui_tag_style;
cui_tag_options cui_tag_options_default(void);
/* Pure style resolution; malformed enum/geometry returns false without writing. */
bool cui_tag_resolve(const cui_theme *theme, cui_tag_options options, bool hovered,
                     cui_tag_style *style);
/* Pair end only after a successful begin. Children are application submitted;
 * text children should use the returned foreground/font size. */
bool cui_begin_tag(cui_context *ui, cui_id id, const cui_theme *theme, cui_tag_options options,
                   cui_tag_style *style);
void cui_end_tag(cui_context *ui, cui_id id, const cui_tag_style *style);
bool cui_tag(cui_context *ui, cui_id id, const cui_theme *theme, cui_str text,
             cui_tag_options options);
#ifdef __cplusplus
}
#endif
#endif
