/* SPDX-License-Identifier: MIT */
#ifndef CUI_PLOT_H
#define CUI_PLOT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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
/* Allocation-free categorical scale models, using application-defined keys.
 * Point/Ordinal borrow arrays; Band copies a first-occurrence deduplicated
 * domain into caller storage. All buffers must outlive their models. */
#define CUI_PLOT_CATEGORY_LIMIT 4096u
#define CUI_PLOT_ORDINAL_BYTE_LIMIT 1048576u
typedef struct {
    double range_start, range_end, padding_inner, padding_outer, max_band_width;
    size_t band_count;
    bool has_max_band_width;
} cui_plot_band_options;
typedef struct {
    uint64_t *domain;
    size_t count, capacity;
    cui_plot_band_options options;
} cui_plot_band_scale;
typedef struct {
    double start, step, band_width;
    size_t band_count;
} cui_plot_band_geometry;
/* On CAPACITY, required reports distinct key count; scale/storage are untouched.
 * NULL storage with zero capacity queries required storage. Initialization is
 * transactional, including when input keys alias destination storage. */
cui_plot_result cui_plot_band_init(cui_plot_band_scale *, uint64_t *storage, size_t capacity,
                                   const uint64_t *domain, size_t count, cui_plot_band_options,
                                   size_t *required);
/* Band order always starts at the lower endpoint, including reversed ranges.
 * Inner padding is 0..1; outer padding is nonnegative and at most half the
 * effective band count. Width cap affects widths, not ticks/steps. */
cui_plot_result cui_plot_band_layout(const cui_plot_band_scale *, cui_plot_band_geometry *);
cui_plot_result cui_plot_band_map(const cui_plot_band_scale *, uint64_t key, double *pixel);
cui_plot_result cui_plot_band_at(const cui_plot_band_scale *, size_t index, double *pixel);
/* May identify reserved empty bands beyond domain.count. Empty/zero-step scales
 * return defined clamped indices; see PLOT_MODEL.md. */
cui_plot_result cui_plot_band_nearest(const cui_plot_band_scale *, double pixel, size_t *index);
typedef struct {
    const uint64_t *domain;
    size_t count;
    double range_start, range_end;
} cui_plot_point_scale;
/* Point preserves duplicate keys and range direction. Key lookup finds the
 * first occurrence; at() maps each entry, including later duplicates. */
cui_plot_result cui_plot_point_init(cui_plot_point_scale *, const uint64_t *domain, size_t count,
                                    double range_start, double range_end);
cui_plot_result cui_plot_point_map(const cui_plot_point_scale *, uint64_t key, double *pixel);
cui_plot_result cui_plot_point_at(const cui_plot_point_scale *, size_t index, double *pixel);
cui_plot_result cui_plot_point_nearest(const cui_plot_point_scale *, double pixel, size_t *index);
typedef struct {
    const uint64_t *domain;
    size_t count;
    const void *range, *unknown;
    size_t range_count, element_size;
    bool has_unknown;
} cui_plot_ordinal_scale;
/* Generic range elements are copied with memmove, preserving their C type's
 * bytes. Range and optional unknown value are borrowed. No implicit domain
 * insertion. Known keys use their first index modulo range_count; a known key
 * with empty range stays EMPTY even when unknown is configured. */
cui_plot_result cui_plot_ordinal_init(cui_plot_ordinal_scale *, const uint64_t *domain,
                                      size_t count, const void *range, size_t range_count,
                                      size_t element_size, const void *unknown);
/* EMPTY/INVALID/OVERFLOW/CAPACITY leave output unchanged. NULL output/zero
 * capacity queries required element capacity for a resolvable key. */
cui_plot_result cui_plot_ordinal_map(const cui_plot_ordinal_scale *, uint64_t key, void *output,
                                     size_t output_capacity);
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
