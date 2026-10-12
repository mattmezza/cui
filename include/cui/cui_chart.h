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
typedef enum { CUI_CHART_SANKEY_LINEAR, CUI_CHART_SANKEY_SQRT } cui_chart_sankey_value_scale;
typedef struct {
    size_t depth, height, layer;
    double throughput;
    float x0, y0, x1, y1;
} cui_chart_sankey_node;
typedef struct {
    float source_y, target_y, source_width, target_width;
} cui_chart_sankey_link_geometry;
typedef struct {
    float left, right, top, bottom;
} cui_chart_sankey_margins;
typedef struct {
    cui_str text;
    cui_color color; /* alpha zero uses chart foreground */
    float font_size; /* zero uses source 10px */
} cui_chart_sankey_label;
/* Styled Radar dimension content. Measure before placement; paint within that box. */
typedef cui_size (*cui_chart_radar_label_measure_fn)(cui_context *, const cui_chart_datum *,
                                                     size_t index, void *user);
typedef void (*cui_chart_radar_label_paint_fn)(cui_context *, cui_id, const cui_chart_datum *,
                                               size_t index, void *user);
/* At most 16 lines per node; return a count no larger than capacity. */
typedef size_t (*cui_chart_sankey_labels_fn)(const cui_chart_datum *, size_t index,
                                             double raw_throughput, cui_chart_sankey_label *out,
                                             size_t capacity, void *user);
/* Caller-defined content removes the fixed line count. Set both callbacks; size is in chart
 * logical pixels and must be finite, nonnegative, and no larger than 16384 per dimension. */
typedef cui_size (*cui_chart_sankey_label_measure_fn)(cui_context *, const cui_chart_datum *,
                                                      size_t index, double raw_throughput,
                                                      void *user);
typedef void (*cui_chart_sankey_label_paint_fn)(cui_context *, cui_id, const cui_chart_datum *,
                                                size_t index, double raw_throughput, void *user);
typedef struct {
    cui_str text;
    float edge_x, edge_y, anchor_x, anchor_y, text_x, text_y;
    bool visible, right;
} cui_chart_pie_label;
typedef struct {
    cui_plot_rect wick, body;
    bool bullish;
} cui_chart_candle_geometry;
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
    cui_color fill; /* area/radar fill; alpha zero derives from stroke */
    bool custom_curve;
    cui_chart_curve curve;
} cui_chart_series;
typedef size_t (*cui_chart_format_fn)(double value, char *buffer, size_t capacity, void *user);
typedef cui_color (*cui_chart_bar_fill_fn)(const cui_chart_datum *, size_t index, cui_plot_rect bar,
                                           cui_plot_rect chart, float x, float y, void *user);
typedef double (*cui_chart_pie_radius_fn)(const cui_chart_datum *, size_t index,
                                          double default_radius, void *user);
typedef struct {
    const cui_chart_datum *datum;
    size_t index;
    double value, start_angle, end_angle, pad_angle; /* source arc angles, radians from zero */
} cui_chart_pie_arc;
typedef double (*cui_chart_pie_arc_radius_fn)(const cui_chart_pie_arc *, double default_radius,
                                              void *user);
typedef cui_color (*cui_chart_pie_line_color_fn)(const cui_chart_datum *, size_t index, void *user);
typedef cui_color (*cui_chart_bar_label_color_fn)(const cui_chart_datum *, size_t index,
                                                  void *user);
typedef cui_color (*cui_chart_region_fill_fn)(size_t series, float x, float y, cui_color fallback,
                                              void *user);
typedef void (*cui_chart_tooltip_fn)(cui_context *, cui_id, size_t index, const cui_chart_datum *,
                                     void *user);
/* Content size excludes 8px padding per side. Finite dimensions in [0, 16368] are accepted;
 * invalid or larger dimensions fall back to the legacy 150x64 portal. */
typedef cui_size (*cui_chart_tooltip_measure_fn)(cui_context *, size_t index,
                                                 const cui_chart_datum *, void *user);
typedef cui_str (*cui_chart_tooltip_title_fn)(const cui_chart_datum *, size_t index, void *user);
typedef cui_str (*cui_chart_tooltip_label_fn)(const cui_chart_datum *, size_t index, size_t series,
                                              void *user);
typedef size_t (*cui_chart_tooltip_value_fn)(const cui_chart_datum *, size_t index, size_t series,
                                             double value, char *buffer, size_t capacity,
                                             void *user);
typedef size_t (*cui_chart_pie_tooltip_value_fn)(const cui_chart_datum *, size_t index,
                                                 double value, double share_percent, char *buffer,
                                                 size_t capacity, void *user);
typedef cui_color (*cui_chart_tooltip_value_color_fn)(const cui_chart_datum *, size_t index,
                                                      size_t series, double value, void *user);
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
    unsigned
        tick_count; /* zero: 5 Cartesian ticks, 4 Radar rings; one clamps to two for Cartesian */
    unsigned x_tick_count, grid_columns; /* zero: source stride/all, no columns */
    unsigned x_tick_margin; /* zero selects source stride one; ignored when count is set */
    bool hide_grid, hide_axes, hide_legend, noninteractive;
    bool hide_x_axis, show_y_axis, y_axis_inside, grid_solid;
    float value_axis_gutter; /* measured outside y-axis width; zero uses 32px */
    bool custom_horizontal_bar_gutters;
    float bar_category_gutter, bar_value_end_gutter; /* measured logical pixels */
    bool custom_y_padding; /* line/area value scale; defaults source 10px top, 0 bottom */
    float y_padding_top, y_padding_bottom;
    const double *reference_values;
    size_t reference_count; /* at most 16 */
    cui_chart_format_fn y_tick_format, tooltip_format, bar_label_format;
    void *format_user;
    cui_chart_tooltip_fn tooltip_content;         /* replaces default hover title and rows */
    cui_chart_tooltip_measure_fn tooltip_measure; /* optional intrinsic custom content size */
    void *tooltip_user;
    cui_chart_tooltip_title_fn tooltip_title;
    cui_chart_tooltip_label_fn tooltip_label; /* per-row label; pie/sankey tooltip_name */
    cui_chart_tooltip_value_fn tooltip_value;
    cui_chart_pie_tooltip_value_fn pie_tooltip_value; /* receives source share percentage */
    cui_chart_tooltip_value_color_fn tooltip_value_color;
    bool has_domain;
    double domain_min, domain_max;
    double inner_radius;     /* pie/donut pixels */
    double pie_outer_radius; /* zero selects 40% of pixel height */
    cui_chart_pie_radius_fn pie_inner_radius_fn, pie_outer_radius_fn;
    cui_chart_pie_arc_radius_fn pie_inner_arc_radius_fn, pie_outer_arc_radius_fn;
    void *pie_radius_user; /* arc callbacks take precedence; stable during a render */
    bool pie_show_labels;  /* outside labels and leader lines */
    float pie_label_gap;   /* zero selects source 15px */
    cui_color pie_label_color, pie_label_line_color;
    cui_chart_pie_line_color_fn pie_label_line_color_fn;
    void *pie_label_line_color_user;
    double pad_angle; /* pie radians */
    double bar_ratio; /* zero means 0.6 */
    cui_chart_bar_alignment bar_alignment;
    bool custom_bar_padding;
    float bar_padding_inner, bar_padding_outer;     /* shares of a band; defaults .4/.2 */
    float max_bar_width, min_bar_length;            /* zero means source 30px/no stub */
    float candle_body_ratio;                        /* zero selects source 0.8 */
    cui_color bar_gradient_start, bar_gradient_end; /* both set for base-to-tip fill */
    cui_chart_bar_fill_fn bar_fill_pixel; /* optional chart-aware pattern/gradient override */
    void *bar_fill_user;
    float bar_corner_radius; /* uniform pixel radius */
    bool custom_bar_corners;
    float bar_corner_radii[4]; /* top-left, top-right, bottom-right, bottom-left */
    bool reveal_enabled, reduced_motion;
    float reveal_progress; /* host animation progress 0..1; reduced motion paints fully */
    bool disable_appear;   /* styled animated render skips first-paint/replay motion */
    uint64_t appear_key;   /* changing this replays styled appear */
    float appear_duration; /* seconds; zero selects styled source default 1s */
    float hover_duration;  /* seconds; zero selects styled source fast tier 0.12s */
    bool show_bar_values;
    cui_color bar_label_color; /* alpha zero uses foreground */
    cui_chart_bar_label_color_fn bar_label_color_fn;
    void *bar_label_color_user;
    cui_chart_curve curve; /* natural is source default */
    bool show_dots;
    double radar_max_value, radar_outer_radius; /* zero selects source defaults */
    float radar_label_gap;                      /* zero selects source 10px */
    cui_color radar_label_color;
    cui_chart_radar_label_measure_fn radar_label_measure;
    cui_chart_radar_label_paint_fn radar_label_paint;
    void *radar_label_user; /* both callbacks set to override text labels */
    cui_chart_sankey_align sankey_align;
    cui_chart_sankey_value_scale sankey_value_scale;
    unsigned sankey_iterations;  /* zero selects six relaxation passes; maximum 100 */
    float sankey_min_link_width; /* zero selects source 1px */
    bool custom_sankey_link_opacity;
    float sankey_link_opacity;       /* default 0.3 */
    float sankey_node_corner_radius; /* zero selects square nodes */
    bool sankey_show_labels, sankey_show_values;
    float sankey_label_gap;                   /* zero selects source 6px */
    float sankey_font_size;                   /* zero selects source 10px */
    cui_chart_sankey_labels_fn sankey_labels; /* overrides simple value/name lines */
    void *sankey_labels_user;
    cui_chart_sankey_label_measure_fn sankey_label_measure;
    cui_chart_sankey_label_paint_fn sankey_label_paint;
    void *sankey_label_user; /* measured/painted CUI content takes precedence over lines */
    cui_chart_sankey_margins sankey_margins; /* measured before layout; zero preserves legacy */
    cui_chart_format_fn sankey_value_format;
    float node_width, node_padding; /* zero means 10 and 16 */
    cui_color background, foreground, grid, series[8], bullish, bearish;
    cui_color primary_fill;                         /* area/radar fill for the first series */
    cui_chart_region_fill_fn area_radar_fill_pixel; /* chart-space gradients/patterns */
    void *area_radar_fill_user;
} cui_chart_options;
typedef struct {
    unsigned char *pixels; /* caller-owned RGBA8, width*height*4 bytes */
    size_t byte_capacity;
    uint64_t revision;
    size_t hovered; /* SIZE_MAX when no point is hovered */
    double hovered_value;
    uint64_t cache_key; /* private; zero disables the cache */
    uint64_t active_appear_key;
    double appear_started_at;
    bool appear_started;
    double mark_clip_left, mark_clip_x; /* private reveal mask bounds */
    bool mark_clip_active;
    float rendered_reveal_progress;
    const cui_chart_datum *rendered_data; /* private hover identity */
    size_t rendered_count;
    cui_chart_kind rendered_kind;
    float hover_progress;                             /* private styled hover fade 0..1 */
    float hover_x, hover_y[8], hover_vx, hover_vy[8]; /* private pointer spring */
    unsigned char *hover_guide_pixels; /* optional caller-owned RGBA scratch for dashed guide */
    size_t hover_guide_capacity;       /* at least ceil(logical plot height)*4 bytes */
    uint64_t hover_guide_revision;     /* private bitmap revision */
    size_t hover_visual_index;
    bool hover_visual_started;
    float emphasis_focus, emphasis_center, emphasis_velocity;
    bool emphasis_started;
    size_t emphasis_visual_index;                /* last hovered mark during exit fade */
    float pie_lift[256], pie_lift_velocity[256]; /* bounded source per-slice spring */
    float tooltip_cursor_x, tooltip_cursor_y;    /* last valid plot-relative pointer */
    bool tooltip_cursor_valid;
    float tooltip_phase, tooltip_focus; /* source 120ms enter/exit easing */
    float tooltip_from;
    bool tooltip_active, tooltip_started;
    size_t tooltip_visual_index; /* retained while fading out */
    double tooltip_visual_value;
    cui_rect bounds; /* retained logical bounds for pointer interaction */
} cui_chart_state;
typedef struct {
    bool valid;
    size_t hovered;
    cui_str label;
    double value;
} cui_chart_hit;
/* Validates all data and storage before mutating any pixel or state. */
cui_plot_result cui_chart_render(cui_chart_state *, const cui_chart_options *);
/* Styled one-second ease-out-quart appear using core frame time and reduced motion. */
cui_plot_result cui_chart_render_animated(cui_context *, cui_chart_state *,
                                          const cui_chart_options *);
/* Measure source's max(32px, widest visible tick + 4px) before render; copy
 * result to options.value_axis_gutter for consistent paint and hit geometry. */
float cui_chart_measure_value_axis_gutter(cui_context *, const cui_chart_options *);
/* Measure horizontal Bar category/value-end label gutters before rasterization.
 * Copy the result to options and set custom_horizontal_bar_gutters. */
cui_plot_result cui_chart_measure_horizontal_bar_gutters(cui_context *, const cui_chart_options *,
                                                         float *category, float *value_end);
/* Measures source-style label margins using the host's text metrics. Call before
 * render, assign the result to options.sankey_margins, then reuse those options
 * for layout, hit testing, and styled painting. Output is unchanged on failure. */
cui_plot_result cui_chart_measure_sankey_margins(cui_context *, const cui_chart_options *,
                                                 cui_chart_sankey_margins *);
bool cui_chart_value_domain(const cui_chart_options *, double *minimum, double *maximum);
/* Pixel-space point location shared by line/area raster and hover decoration. */
cui_plot_result cui_chart_point_coordinates(const cui_chart_options *, size_t index, size_t series,
                                            float *x, float *y);
/* Radar value vertex in physical pixels, shared with hovered series marks. */
cui_plot_result cui_chart_radar_vertex(const cui_chart_options *, size_t dimension, size_t series,
                                       float *x, float *y);
cui_plot_result cui_chart_bar_bounds(const cui_chart_options *, size_t index, cui_plot_rect *);
/* Band-axis center in pixels, including a zero-value bar with empty bounds. */
cui_plot_result cui_chart_bar_band_center(const cui_chart_options *, size_t index, float *center);
cui_plot_result cui_chart_candle_bounds(const cui_chart_options *, size_t index,
                                        cui_chart_candle_geometry *);
/* Caller-owned output; computes acyclic topology and proportional node bounds atomically. */
cui_plot_result cui_chart_sankey_layout(const cui_chart_options *, cui_chart_sankey_node *,
                                        size_t capacity);
cui_plot_result cui_chart_sankey_link_bounds(const cui_chart_options *,
                                             const cui_chart_sankey_node *nodes,
                                             cui_chart_sankey_link_geometry *, size_t capacity);
cui_plot_result cui_chart_pie_labels(const cui_chart_options *, cui_chart_pie_label *,
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
