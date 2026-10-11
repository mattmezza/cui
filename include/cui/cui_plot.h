/* SPDX-License-Identifier: MIT */
#ifndef CUI_PLOT_H
#define CUI_PLOT_H
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum { CUI_PLOT_LINEAR, CUI_PLOT_LOG10 } cui_plot_scale_kind;
typedef enum {
    CUI_PLOT_OK,
    CUI_PLOT_INVALID,
    CUI_PLOT_DEGENERATE,
    CUI_PLOT_OVERFLOW,
    CUI_PLOT_CAPACITY,
    CUI_PLOT_EMPTY
} cui_plot_result;
typedef struct {
    cui_plot_scale_kind kind;
    double domain_min, domain_max, range_start, range_end;
    bool clamp;
} cui_plot_scale;
typedef struct {
    double x, y;
} cui_plot_point;
typedef struct {
    double x_min, y_min, x_max, y_max;
} cui_plot_rect;
typedef struct {
    cui_plot_point start, end;
} cui_plot_line;
/* Domain must ascend; pixel ranges may reverse. All endpoints finite. */
cui_plot_result cui_plot_validate(const cui_plot_scale *);
cui_plot_result cui_plot_map(const cui_plot_scale *, double value, double *pixel);
cui_plot_result cui_plot_inverse(const cui_plot_scale *, double pixel, double *value);
/* Desired count is advisory, 2..1000. Count is required capacity; no partial
 * output on capacity failure. NULL/zero output performs a size query. */
cui_plot_result cui_plot_ticks(const cui_plot_scale *, size_t desired_count, double *ticks,
                               size_t capacity, size_t *count);
cui_plot_result cui_plot_nice(cui_plot_scale *, size_t desired_count);
/* Positive pixel delta shifts plotted data toward increasing pixels.
 * Factor >1 zooms in, keeping the data at anchor_pixel fixed. */
cui_plot_result cui_plot_pan(cui_plot_scale *, double delta_pixels);
cui_plot_result cui_plot_zoom(cui_plot_scale *, double factor, double anchor_pixel);
cui_plot_result cui_plot_clip_line(cui_plot_line, cui_plot_rect viewport, cui_plot_line *out);
cui_plot_result cui_plot_project_line(const cui_plot_scale *x, const cui_plot_scale *y,
                                      cui_plot_point a, cui_plot_point b, cui_plot_rect viewport,
                                      cui_plot_line *out);
cui_plot_result cui_plot_project_bar(const cui_plot_scale *x, const cui_plot_scale *y,
                                     double center, double width_pixels, double value,
                                     double baseline, cui_plot_rect viewport, cui_plot_rect *out);
#ifdef __cplusplus
}
#endif
#endif
