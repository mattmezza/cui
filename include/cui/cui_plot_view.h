/* SPDX-License-Identifier: MIT */
#ifndef CUI_PLOT_VIEW_H
#define CUI_PLOT_VIEW_H
#include "cui.h"
#include "cui_plot.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Source Text/PlotLabel: caller-owned labels relative to this fixed surface.
 * Size zero selects10px; explicit colors (including transparent) are preserved.
 * Source font_weight is ignored by PlotLabel.paint, so this contract uses normal.
 * Borrowed text remains valid through renderer completion. <=256 labels. */
typedef struct {
    cui_str text;
    cui_plot_point origin;
    float font_size;
    cui_color color;
    cui_align align;
} cui_plot_label;
cui_plot_result cui_plot_labels_layout(cui_context *, const cui_plot_label *, size_t count,
                                       cui_rect *, size_t capacity, size_t *required);
cui_plot_result cui_plot_labels(cui_context *, cui_id, cui_size extent, const cui_plot_label *,
                                size_t count);
/* Copy source-style measured UTF-8 truncation into caller storage, NUL terminated.
 * <=4096 input bytes, strict UTF-8, zero size defaults10. max_width<=0 keeps text;
 * when truncated, returns at least ellipsis even if that exceeds the budget.
 * required includes NUL; errors preserve output, CAPACITY sets required.
 * Input may alias output. No grapheme clustering/locale or automatic layout. */
cui_plot_result cui_plot_truncate_text(cui_context *, cui_str, float font_size, float max_width,
                                       char *output, size_t capacity, size_t *required);
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
/* Fixed-size themed Grid, borrowing pure geometry options for this call.
 * Alpha-zero stroke selects theme border. Preflight failures submit nothing. */
cui_plot_result cui_plot_grid(cui_context *, cui_id, const cui_theme *,
                              const cui_plot_grid_options *, cui_color stroke);
typedef struct {
    double size;
    cui_color fill, stroke;
    bool use_theme_fill, custom_stroke;
} cui_plot_dot_options;
/* Styled default size4, theme primary fill; source base itself defaults to
 * transparent fill. Without custom_stroke the border uses resolved solid fill. */
cui_plot_dot_options cui_plot_dot_defaults(void);
/* Fixed extent; skips undefined samples and paints markers after preflight.
 * Cartesian uses x/y1. Radial uses angle/radius1 around extent center.
 * <=4096 samples, size0..1024, coordinates within +/-16384. No dot semantics
 * or hit model; stable borrowed arrays. Pure validation errors submit nothing. */
cui_plot_result cui_plot_dots(cui_context *, cui_id, const cui_theme *, cui_size extent,
                              const cui_plot_sample *, size_t count, cui_plot_dot_options);
cui_plot_result cui_plot_radial_dots(cui_context *, cui_id, const cui_theme *, cui_size extent,
                                     const cui_plot_radial_sample *, size_t count,
                                     cui_plot_dot_options);
typedef struct {
    double cross, baseline, value;
    bool has_cross, has_value;
} cui_plot_bar_sample;
typedef cui_color (*cui_plot_bar_fill_fn)(const cui_plot_bar_sample *, size_t index,
                                          cui_plot_rect frame, cui_plot_bar_alignment, void *user);
typedef struct {
    cui_plot_bar_alignment alignment;
    double band_width;
    cui_color fill;
    bool use_theme_fill;
    cui_corner_radii corners;
    cui_plot_bar_fill_fn fill_for;
    void *user;
} cui_plot_bar_options;
/* Styled bottom/zero-width/themed solid fill defaults; caller sets width. */
cui_plot_bar_options cui_plot_bar_defaults(void);
/* Callback resolves a solid fill synchronously once per eligible datum, with
 * source-relative frame and alignment. It must not submit UI or mutate borrowed
 * samples/options; called during frame composition, not renderer execution.
 * Per-corner rounding uses existing Core renderer clamping. No gradient or
 * generic label-content driver. Missing cross/value skip callback and geometry.
 * Fixed extent, <=4096 samples, band width0..16384, finite coordinates within
 * +/-16384, corner sizes0..1024; preflight failures submit nothing/call nobody. */
cui_plot_result cui_plot_bars(cui_context *, cui_id, const cui_theme *, cui_size extent,
                              const cui_plot_bar_sample *, size_t count,
                              const cui_plot_bar_options *);
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
