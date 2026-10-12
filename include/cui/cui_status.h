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
typedef enum { CUI_MARKER_PLAIN, CUI_MARKER_SEPARATOR, CUI_MARKER_BORDER } cui_marker_variant;
typedef enum {
    CUI_MARKER_ALIGN_DEFAULT,
    CUI_MARKER_ALIGN_START,
    CUI_MARKER_ALIGN_CENTER,
    CUI_MARKER_ALIGN_END
} cui_marker_alignment;
typedef enum { CUI_MARKER_SPINNER, CUI_MARKER_SHIMMER } cui_marker_loading;
typedef enum {
    CUI_MARKER_ICON,
    CUI_MARKER_TEXT,
    CUI_MARKER_CONTENT,
    CUI_MARKER_DIRECT
} cui_marker_child_kind;
/* Callback receives resolved text style and animated opacity. It owns children
 * and must multiply paint alpha by opacity for rich-only shimmer. */
typedef void (*cui_status_render_fn)(cui_context *, cui_id, void *, float font_size,
                                     cui_color color, float opacity);
typedef struct {
    cui_marker_child_kind kind;
    cui_str text;
    float width;
    cui_status_render_fn render;
    void *user;
} cui_marker_child;
#define CUI_MARKER_MAX_CHILDREN 32
typedef struct {
    cui_rect text_bounds[CUI_MARKER_MAX_CHILDREN];
    float last_phase;
    bool initialized;
} cui_marker_state;
typedef struct {
    cui_marker_variant variant;
    cui_marker_alignment alignment;
    cui_marker_loading loading_style;
    bool loading, reduced_motion, reverse, once, custom_highlight, absolute_spread;
    cui_role role;
    cui_str name;
    float width, shimmer_spread;
    double now_seconds, duration;
    cui_color highlight;
} cui_marker_options;
typedef struct {
    cui_marker_alignment alignment;
    bool spinner, animated;
    float phase, rich_opacity;
    double next_wake_seconds;
} cui_marker_plan;
cui_marker_options cui_marker_options_default(void);
bool cui_base_marker(cui_marker_options options, const cui_marker_child *children, size_t count,
                     cui_marker_plan *plan);
bool cui_marker(cui_context *ui, cui_id id, const cui_theme *theme, cui_marker_state *state,
                const cui_marker_child *children, size_t count, cui_marker_options options);
#ifdef __cplusplus
}
#endif
#endif
