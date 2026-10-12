/* SPDX-License-Identifier: MIT */
#ifndef CUI_CHART_H
#define CUI_CHART_H
#include "cui.h"
#include "cui_plot.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    CUI_CHART_LINE,
    CUI_CHART_AREA,
    CUI_CHART_BAR,
    CUI_CHART_PIE,
    CUI_CHART_RADAR,
    CUI_CHART_CANDLESTICK,
    CUI_CHART_SANKEY
} cui_chart_kind;
typedef struct {
    cui_str label;
    double value, open, high, low, close;
    cui_color color; /* alpha zero selects the theme series color */
} cui_chart_datum;
typedef struct {
    size_t source, target;
    double value;
    cui_color color;
} cui_chart_link;
typedef enum {
    CUI_CHART_SANKEY_JUSTIFY,
    CUI_CHART_SANKEY_LEFT,
    CUI_CHART_SANKEY_RIGHT,
    CUI_CHART_SANKEY_CENTER
} cui_chart_sankey_align;
typedef struct {
    size_t depth, height, layer;
    double throughput;
    float x0, y0, x1, y1;
} cui_chart_sankey_node;
typedef enum {
    CUI_CHART_CURVE_NATURAL,
    CUI_CHART_CURVE_LINEAR,
    CUI_CHART_CURVE_STEP_AFTER
} cui_chart_curve;
typedef enum {
    CUI_CHART_BAR_BOTTOM,
    CUI_CHART_BAR_TOP,
    CUI_CHART_BAR_LEFT,
    CUI_CHART_BAR_RIGHT
} cui_chart_bar_alignment;
typedef struct {
    const double *values; /* one value for each datum */
    size_t count;
    cui_str name;
    cui_color color;
} cui_chart_series;
typedef size_t (*cui_chart_format_fn)(double value, char *buffer, size_t capacity, void *user);
typedef struct {
    cui_chart_kind kind;
    const cui_chart_datum *data;
    size_t count;
    const cui_chart_link *links;
    size_t link_count;
    const cui_chart_series *extra_series; /* additional line/area/radar series, at most 7 */
    size_t extra_series_count;
    unsigned width, height;
    float logical_width, logical_height; /* zero selects pixel dimensions */
    uint64_t data_revision;              /* nonzero enables a cache with stable input pointers */
    size_t point_count;                  /* zero means count */
    unsigned tick_count;                 /* zero means 5 */
    unsigned x_tick_count, grid_columns; /* zero: source stride/all, no columns */
    bool hide_grid, hide_axes, hide_legend, noninteractive;
    bool hide_x_axis, show_y_axis, y_axis_inside, grid_solid;
    const double *reference_values;
    size_t reference_count; /* at most 16 */
    cui_chart_format_fn y_tick_format, tooltip_format, bar_label_format;
    void *format_user;
    bool has_domain;
    double domain_min, domain_max;
    double inner_radius; /* pie/donut pixels */
    double pad_angle;    /* pie radians */
    double bar_ratio;    /* zero means 0.6 */
    cui_chart_bar_alignment bar_alignment;
    bool custom_bar_padding;
    float bar_padding_inner, bar_padding_outer; /* shares of a band; defaults .4/.2 */
    float max_bar_width, min_bar_length;        /* zero means source 30px/no stub */
    bool show_bar_values;
    cui_color bar_label_color; /* alpha zero uses foreground */
    cui_chart_curve curve;     /* natural is source default */
    bool show_dots;
    double radar_max_value, radar_outer_radius; /* zero selects source defaults */
    cui_chart_sankey_align sankey_align;
    float node_width, node_padding; /* zero means 10 and 16 */
    cui_color background, foreground, grid, series[8], bullish, bearish;
} cui_chart_options;
typedef struct {
    unsigned char *pixels; /* caller-owned RGBA8, width*height*4 bytes */
    size_t byte_capacity;
    uint64_t revision;
    size_t hovered; /* SIZE_MAX when no point is hovered */
    double hovered_value;
    uint64_t cache_key; /* private; zero disables the cache */
    cui_rect bounds;    /* retained logical bounds for pointer interaction */
} cui_chart_state;
typedef struct {
    bool valid;
    size_t hovered;
    cui_str label;
    double value;
} cui_chart_hit;
/* Validates all data and storage before mutating any pixel or state. */
cui_plot_result cui_chart_render(cui_chart_state *, const cui_chart_options *);
bool cui_chart_value_domain(const cui_chart_options *, double *minimum, double *maximum);
cui_plot_result cui_chart_bar_bounds(const cui_chart_options *, size_t index, cui_plot_rect *);
/* Caller-owned output; computes acyclic topology and proportional node bounds atomically. */
cui_plot_result cui_chart_sankey_layout(const cui_chart_options *, cui_chart_sankey_node *,
                                        size_t capacity);
/* Read-only hit geometry over a successfully rendered chart. */
cui_chart_hit cui_chart_hit_test(const cui_chart_options *, float x, float y);
/* Draws the caller-owned bitmap, labels and a named image semantic. Render first. */
bool cui_chart(cui_context *, cui_id, cui_chart_state *, const cui_chart_options *, cui_str name);
/* Colors from the active theme; caller options override individual fields. */
cui_chart_options cui_chart_theme(cui_chart_options, const cui_theme *);
#ifdef __cplusplus
}
#endif
#endif
