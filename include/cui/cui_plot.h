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
/* Pure geometry: coordinates are already projected by the application.
 * Output/input/count pointers must not overlap. Two-pass builders never write
 * partial output on error; required is published only for OK/CAPACITY.
 * At most CUI_PLOT_CATEGORY_LIMIT samples/stack cells are accepted. */
typedef struct {
    double x, y0, y1;
    bool defined;
} cui_plot_sample;
typedef enum {
    CUI_PLOT_CURVE_LINEAR,
    CUI_PLOT_CURVE_NATURAL,
    CUI_PLOT_CURVE_STEP_AFTER
} cui_plot_curve;
typedef enum { CUI_PLOT_GAPS_SKIP, CUI_PLOT_GAPS_SPLIT } cui_plot_gap_policy;
typedef enum {
    CUI_PLOT_PATH_MOVE,
    CUI_PLOT_PATH_LINE,
    CUI_PLOT_PATH_CUBIC,
    CUI_PLOT_PATH_CLOSE,
    CUI_PLOT_PATH_ARC
} cui_plot_path_kind;
typedef struct {
    cui_plot_path_kind kind;
    cui_plot_point point, control1, control2;
    double radius; /* ARC: circular endpoint arc, no axis rotation */
    bool large_arc, clockwise;
} cui_plot_path_command;
/* Line uses y1. Area follows y1 then reverses y0 and closes each run.
 * Undefined samples' numbers are ignored. SKIP bridges them (frozen GPUI
 * behavior); SPLIT starts a new run. StepAfter omits the final vertical leg,
 * matching the source line. Area always closes even single-point runs. */
cui_plot_result cui_plot_path(const cui_plot_sample *, size_t count, cui_plot_curve,
                              cui_plot_gap_policy, bool area, cui_plot_path_command *,
                              size_t capacity, size_t *required);
/* Frozen Area's independent accessor missingness and optional constant
 * baseline. This is distinct from the extended closed per-point area path. */
typedef struct {
    double x, y;
    bool has_x, has_y;
} cui_plot_area_sample;
typedef struct {
    cui_plot_path_command *fill, *stroke;
    size_t fill_capacity, stroke_capacity, fill_count, stroke_count;
} cui_plot_area_buffers;
/* SKIP missing x/y pairs. Fill and stroke share the upper curve. Fill closes
 * only when >1 pair remains, baseline is present, and original first/last x
 * exist; closure uses those original x values even if their y is absent.
 * Singleton emits Move only, ignoring baseline (source early return).
 * Without closure the fill path is open; renderer fill semantics decide its
 * implicit closure. INVALID/OVERFLOW preserve counts and both command buffers;
 * CAPACITY publishes both counts without partial output. All buffers disjoint
 * and stable; count <=4096. Baseline is ignored when has_baseline=false. */
cui_plot_result cui_plot_area_paths(const cui_plot_area_sample *, size_t count, cui_plot_curve,
                                    bool has_baseline, double baseline, cui_plot_area_buffers *);
typedef struct {
    double value;
    bool defined;
} cui_plot_value;
typedef struct {
    double y0, y1;
} cui_plot_stack_point;
/* Series-major cells: [series * point_count + point]. Natural series order,
 * shared signed cumulative baseline; missing values are zero (not divergent).
 * cells = series_count * point_count, with no data copies or allocations. */
cui_plot_result cui_plot_stack(const cui_plot_value *, size_t series_count, size_t point_count,
                               cui_plot_stack_point *, size_t capacity, size_t *required);
typedef struct {
    size_t index;
    double value, start_angle, end_angle, pad_angle;
} cui_plot_arc;
/* Positive defined values only, original indices retained; no sorting or pad
 * redistribution. Angles may reverse; finite nonnegative pad is metadata.
 * radians: zero at 12 o'clock, clockwise positive. */
cui_plot_result cui_plot_pie(const cui_plot_value *, size_t count, double start_angle,
                             double end_angle, double pad_angle, cui_plot_arc *, size_t capacity,
                             size_t *required);
/* Radii must be finite, 0 <= inner <= outer. Centroid is relative to center.
 * contains uses normalized [0, 2pi) screen angle and the source half-open
 * [start_angle,end_angle) test, deliberately ignoring pad_angle. Consequently
 * reversed and unnormalized angle intervals do not wrap automatically. */
cui_plot_result cui_plot_arc_centroid(const cui_plot_arc *, double inner_radius,
                                      double outer_radius, cui_plot_point *);
cui_plot_result cui_plot_arc_contains(const cui_plot_arc *, double inner_radius,
                                      double outer_radius, cui_plot_point relative_position,
                                      bool *contains);
/* Closed annular sector, centered at (0,0). Positive spans up to 2pi only;
 * reversed/zero/fully padded spans produce an empty successful path.
 * Full turns use two semicircles without the source's artificial seam.
 * Smaller sectors use source pad-width policy; pad is disabled for full turns.
 * Geometry errors/capacity are transactional; buffers must be disjoint. */
cui_plot_result cui_plot_arc_path(const cui_plot_arc *, double inner_radius, double outer_radius,
                                  cui_plot_path_command *, size_t capacity, size_t *required);
/* Polygonal tessellation of the same sector, with outer+reversed inner rim.
 * segments_per_turn: 4..1024; each rim uses ceil(span*segments/2pi) chords.
 * Returns Move/Line/Close only. Maximum sagitta is r*(1-cos(pi/segments)). */
cui_plot_result cui_plot_arc_flatten(const cui_plot_arc *, double inner_radius, double outer_radius,
                                     size_t segments_per_turn, cui_plot_path_command *,
                                     size_t capacity, size_t *required);
typedef struct {
    double angle, radius0, radius1;
    bool defined;
} cui_plot_radial_sample;
/* Linear radial polygon relative to (0,0). Undefined samples are skipped or
 * split. closed stroke requires >=3 points; fill requires >=3 and always
 * closes (frozen RadialLine). area follows radius1 then reverse radius0;
 * radial area is a CUI extension, not a frozen GPUI RadialArea API.
 * Radii must be finite/nonnegative. area ignores closed/fill. */
cui_plot_result cui_plot_radial_path(const cui_plot_radial_sample *, size_t count,
                                     cui_plot_gap_policy, bool closed, bool fill, bool area,
                                     cui_plot_path_command *, size_t capacity, size_t *required);
typedef struct {
    cui_plot_rect frame;
    double radius, border_width;
} cui_plot_dot_geometry;
/* Source Line/RadialLine marker: square centered at point, radius=size/2,
 * one-pixel border. Finite nonnegative size; zero produces a valid degenerate
 * marker. Errors preserve output; source unchecked negative size is rejected. */
cui_plot_result cui_plot_dot_layout(cui_plot_point center, double size, cui_plot_dot_geometry *);
typedef enum {
    CUI_PLOT_BAR_BOTTOM,
    CUI_PLOT_BAR_TOP,
    CUI_PLOT_BAR_LEFT,
    CUI_PLOT_BAR_RIGHT
} cui_plot_bar_alignment;
typedef struct {
    cui_plot_rect frame;
    cui_plot_point label_origin;
    double gradient_angle;
} cui_plot_bar_geometry;
/* Already projected coordinates: cross is the leading band edge, not center.
 * Source value-end label origin uses 10px text/2px gap; no clipping or text
 * measurement. Zero width/height still returns valid degenerate geometry.
 * All orientations support values on either side of baseline. */
cui_plot_result cui_plot_bar_layout(cui_plot_bar_alignment, double cross, double band_width,
                                    double baseline, double value, cui_plot_bar_geometry *);
typedef struct {
    double width, height;
    const double *x, *y, *dashes;
    size_t x_count, y_count, dash_count;
} cui_plot_grid_options;
/* Source 1px centered grid quads, vertical lines then horizontal lines.
 * SVG alternating dash/gap; odd arrays repeat twice, negative lengths clamp
 * to zero, all-nonpositive/empty patterns draw solid. Extents/positions finite
 * within +/-16384 (positive extents); positions are not silently clipped.
 * At most256 lines per axis/64 dash entries/4096 output quads. If the bounded
 * generator exceeds4096 quads CAPACITY reports4097 as a lower bound, even for
 * a size query. Other capacity errors report exact count. Tiny nonadvancing
 * dash steps return DEGENERATE. All errors leave geometry unchanged; only
 * OK/CAPACITY publish required. Buffers must be disjoint/stable. */
cui_plot_result cui_plot_grid_layout(const cui_plot_grid_options *, cui_plot_rect *,
                                     size_t capacity, size_t *required);
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
/* Caller-owned hover lifecycle. Dots are borrowed until replaced or exit completes;
 * live requires cursor. Progress is supplied by the caller's motion policy in [0,1].
 * Zero initialize. Invalid input leaves memory unchanged. */
typedef struct {
    size_t index;
    cui_plot_point cross_line;
    const cui_plot_point *dots;
    size_t dot_count;
} cui_plot_tooltip_state;
typedef struct {
    cui_plot_tooltip_state state;
    cui_plot_point cursor;
    double progress;
    bool has_state, hovered, entering;
} cui_plot_hover_memory;
cui_plot_result cui_plot_hover_track(cui_plot_hover_memory *, const cui_plot_tooltip_state *live,
                                     const cui_plot_point *cursor, double progress);
/* Explicit scope entry: one slot per stable application identity. Only completed
 * generations persist across disappearance; unfinished appearances restart.
 * An identity change clears the slot. No allocation or implicit frame tracking. */
typedef struct {
    uint64_t identity, completed_generation;
    bool has_completed;
} cui_plot_appear_scope;
bool cui_plot_appear_complete(cui_plot_appear_scope *, uint64_t identity, uint64_t generation);
cui_plot_result cui_plot_appear_record(cui_plot_appear_scope *, uint64_t identity,
                                       uint64_t generation, double linear_progress);
/* Stagger the linear clock before applying the caller's existing easing policy.
 * Spread clamps to [0,.95], index clamps to count-1. count<=1 is unstaggered. */
cui_plot_result cui_plot_appear_stagger(double linear_progress, size_t index, size_t count,
                                        double spread, double *local_progress);
#ifdef __cplusplus
}
#endif
#endif
