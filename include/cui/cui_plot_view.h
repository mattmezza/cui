/* SPDX-License-Identifier: MIT */
#ifndef CUI_PLOT_VIEW_H
#define CUI_PLOT_VIEW_H
#include "cui.h"
#include "cui_plot.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CUI_PLOT_AXIS_LIMIT 256u
/* Borrowed labels; font size zero selects 10px. START/CENTER/END alignment is
 * applied about tick (source left/center/right). Alpha zero uses theme muted. */
typedef struct {
    cui_str text;
    float tick, font_size;
    cui_align align;
    cui_color color;
} cui_plot_axis_text;
typedef enum { CUI_PLOT_AXIS_END, CUI_PLOT_AXIS_START } cui_plot_axis_side;
typedef struct {
    float width, height, x, y;
    bool has_x, has_y, x_line, y_line;
    cui_plot_axis_side x_side, y_side;
    const cui_plot_axis_text *x_labels, *y_labels;
    size_t x_count, y_count;
    cui_color stroke;
} cui_plot_axis_options;
typedef struct {
    cui_rect bounds;
    size_t index;
    bool horizontal;
} cui_plot_axis_label_geometry;
/* Default x line shown, y line hidden; axes absent until has_x/has_y is set. */
cui_plot_axis_options cui_plot_axis_defaults(float width, float height);
float cui_plot_axis_gutter(float font_size);
/* At default 10px, source offsets match; larger fonts scale the upper gap
 * and vertical centering: x end +6/start -(font+4), y end +2/start -2, vertically
 * centered by font/2. Geometry uses Core measurement and is transactional. */
cui_plot_result cui_plot_axis_layout(cui_context *, const cui_plot_axis_options *,
                                     cui_plot_axis_label_geometry *, size_t capacity,
                                     size_t *required);
/* Submits a fixed-size axis surface with theme-resolved lines/text. */
cui_plot_result cui_plot_axis(cui_context *, cui_id, const cui_theme *,
                              const cui_plot_axis_options *);
/* Caller storage for scale-generated numeric tick labels; remains live through
 * end_frame. No formatter callback, locales, category labels or auto-gutters. */
typedef struct {
    char text[32];
    double value;
    cui_plot_axis_text label;
} cui_plot_numeric_tick;
cui_plot_result cui_plot_axis_ticks(const cui_plot_scale *, size_t desired, cui_plot_numeric_tick *,
                                    size_t capacity, size_t *required);
typedef struct {
    const cui_plot_path_command *commands;
    size_t count;
    cui_color fill, stroke;
    float stroke_width;
} cui_plot_shape;
/* Caller RGBA8 storage, straight alpha. Rasterizer accepts Move/Line/Cubic/Close;
 * use arc_flatten for ARC commands. Cubics use 16 uniform parameter chords.
 * Each shape <=4096 flattened segments, <=64 shapes, dimensions1..1024,
 * storage requires stride*height bytes; total pixel*segment work <=64 million. Even-odd fill of
 * closed runs, round segment strokes, pixel-center sampling, no anti-aliasing. Disjoint stable
 * buffers required; invalid/capacity errors preserve all pixels. Coordinates finite and within
 * +/-16384; shape colors are explicit. */
cui_plot_result cui_plot_raster(unsigned char *pixels, unsigned width, unsigned height,
                                size_t stride, size_t capacity_bytes, const cui_plot_shape *,
                                size_t count, cui_color clear);
typedef void (*cui_plot_activate_fn)(cui_context *, cui_id, cui_plot_point bitmap_position,
                                     void *user);
typedef struct {
    cui_bitmap bitmap;
    const cui_plot_axis_options *axis; /* same pixel extent as bitmap */
    cui_str name;
    bool disabled, interactive;
    cui_plot_activate_fn on_activate;
    void *user;
} cui_plot_view_options;
/* Immediate fixed-size bitmap + axis composition. Bitmap/labels borrowed
 * through renderer completion. Validated primary-pointer release invokes the
 * callback synchronously with bitmap coordinates; no keyboard activation,
 * implicit zoom/pan or per-shape hit testing. */
cui_plot_result cui_plot_view(cui_context *, cui_id, const cui_theme *,
                              const cui_plot_view_options *);
#ifdef __cplusplus
}
#endif
#endif
