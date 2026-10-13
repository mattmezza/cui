/* SPDX-License-Identifier: MIT */
#include "cui/cui_plot.h"
#include <float.h>
#include <math.h>
#include <string.h>
#define CUI_PLOT_MAX_TICKS 1024
static long double cui_plot_transform(cui_plot_scale_kind kind, double value) {
    return kind == CUI_PLOT_LOG10 ? log10l((long double)value) : (long double)value;
}
static cui_plot_result cui_plot_number(long double value, double *out) {
    if (!isfinite(value) || value > DBL_MAX || value < -DBL_MAX)
        return CUI_PLOT_OVERFLOW;
    double result = (double)value;
    if (!isfinite(result))
        return CUI_PLOT_OVERFLOW;
    *out = result;
    return CUI_PLOT_OK;
}
static cui_plot_result cui_plot_untransform(cui_plot_scale_kind kind, long double value,
                                            double *out) {
    if (kind == CUI_PLOT_LOG10) {
        long double result = powl(10, value);
        if (result <= 0 || (double)result == 0)
            return CUI_PLOT_OVERFLOW;
        return cui_plot_number(result, out);
    }
    return cui_plot_number(value, out);
}
static long double cui_plot_fraction(long double value, long double a, long double b) {
    long double span = b - a;
    if (!isfinite(span) || !isfinite(value - a))
        return (value / 2 - a / 2) / (b / 2 - a / 2);
    return (value - a) / span;
}
static long double cui_plot_lerp(long double a, long double b, long double t) {
    if (t == 0)
        return a;
    if (t == 1)
        return b;
    return (1 - t) * a + t * b;
}
static long double cui_plot_clamp_fraction(long double t, bool clamp) {
    return clamp ? fmaxl(0, fminl(1, t)) : t;
}
cui_plot_result cui_plot_validate(const cui_plot_scale *s) {
    if (!s || (s->kind != CUI_PLOT_LINEAR && s->kind != CUI_PLOT_LOG10) ||
        !isfinite(s->domain_min) || !isfinite(s->domain_max) || !isfinite(s->range_start) ||
        !isfinite(s->range_end) || s->domain_min > s->domain_max ||
        (s->kind == CUI_PLOT_LOG10 && s->domain_min <= 0))
        return CUI_PLOT_INVALID;
    if (s->domain_min == s->domain_max || s->range_start == s->range_end ||
        cui_plot_transform(s->kind, s->domain_min) == cui_plot_transform(s->kind, s->domain_max))
        return CUI_PLOT_DEGENERATE;
    return CUI_PLOT_OK;
}
cui_plot_result cui_plot_map(const cui_plot_scale *s, double value, double *pixel) {
    cui_plot_result status = cui_plot_validate(s);
    if (status != CUI_PLOT_OK)
        return status;
    if (!pixel || !isfinite(value) || (s->kind == CUI_PLOT_LOG10 && value <= 0))
        return CUI_PLOT_INVALID;
    long double t = cui_plot_fraction(cui_plot_transform(s->kind, value),
                                      cui_plot_transform(s->kind, s->domain_min),
                                      cui_plot_transform(s->kind, s->domain_max));
    t = cui_plot_clamp_fraction(t, s->clamp);
    return cui_plot_number(cui_plot_lerp(s->range_start, s->range_end, t), pixel);
}
cui_plot_result cui_plot_inverse(const cui_plot_scale *s, double pixel, double *value) {
    cui_plot_result status = cui_plot_validate(s);
    if (status != CUI_PLOT_OK)
        return status;
    if (!value || !isfinite(pixel))
        return CUI_PLOT_INVALID;
    long double t =
        cui_plot_clamp_fraction(cui_plot_fraction(pixel, s->range_start, s->range_end), s->clamp);
    if (t == 0) {
        *value = s->domain_min;
        return CUI_PLOT_OK;
    }
    if (t == 1) {
        *value = s->domain_max;
        return CUI_PLOT_OK;
    }
    return cui_plot_untransform(s->kind,
                                cui_plot_lerp(cui_plot_transform(s->kind, s->domain_min),
                                              cui_plot_transform(s->kind, s->domain_max), t),
                                value);
}
static long double cui_plot_tick_step(const cui_plot_scale *s, size_t desired) {
    long double a = cui_plot_transform(s->kind, s->domain_min);
    long double b = cui_plot_transform(s->kind, s->domain_max);
    long double raw = (b - a) / (desired - 1);
    if (!isfinite(raw))
        raw = (b / (desired - 1) - a / (desired - 1));
    if (raw <= 0)
        raw = DBL_TRUE_MIN;
    long double power = powl(10, floorl(log10l(raw)));
    if (power == 0)
        power = DBL_TRUE_MIN;
    long double normalized = raw / power;
    /* log10l/powl and the span division can straddle an exact nice-step
       threshold by a few arithmetic ulps. Do not double a nominal unit step
       merely because one libm returns log10l(1000) just above 3. */
    long double rounding = 1 + 8 * LDBL_EPSILON;
    long double step = power * (normalized <= rounding       ? 1
                                : normalized <= 2 * rounding ? 2
                                : normalized <= 5 * rounding ? 5
                                                             : 10);
    if (s->kind == CUI_PLOT_LINEAR) {
        double magnitude = fmax(fabs(s->domain_min), fabs(s->domain_max));
        double ulp = magnitude - nextafter(magnitude, 0);
        step = fmaxl(step, fmaxl(ulp, DBL_TRUE_MIN));
        step = fminl(step, DBL_MAX);
    }
    return step;
}
cui_plot_result cui_plot_ticks(const cui_plot_scale *s, size_t desired, double *ticks,
                               size_t capacity, size_t *count) {
    if (!count)
        return CUI_PLOT_INVALID;
    *count = 0;
    cui_plot_result status = cui_plot_validate(s);
    if (status != CUI_PLOT_OK)
        return status;
    if (desired < 2 || desired > 1000 || (!ticks && capacity))
        return CUI_PLOT_INVALID;
    long double a = cui_plot_transform(s->kind, s->domain_min);
    long double b = cui_plot_transform(s->kind, s->domain_max);
    long double step = cui_plot_tick_step(s, desired);
    /* Inspect the adjacent grid index on each side: a transformed endpoint
       can round across its exact index. Inclusion is decided below in the
       original double domain, never by a tolerant out-of-domain comparison. */
    long double first = ceill(a / step) - 1;
    long double last = floorl(b / step) + 1;
    double values[CUI_PLOT_MAX_TICKS];
    size_t n = 0;
    for (size_t i = 0; i < CUI_PLOT_MAX_TICKS; ++i) {
        long double index = first + i;
        if (index > last)
            break;
        long double tick = index * step;
        double value;
        status = cui_plot_untransform(s->kind, tick, &value);
        if (status != CUI_PLOT_OK)
            continue;
        if (value < s->domain_min || value > s->domain_max || (n && value <= values[n - 1]))
            continue;
        values[n++] = value == 0 ? 0 : value;
    }
    *count = n;
    if (capacity < n)
        return CUI_PLOT_CAPACITY;
    if (n)
        memcpy(ticks, values, n * sizeof(*ticks));
    return CUI_PLOT_OK;
}
cui_plot_result cui_plot_nice(cui_plot_scale *s, size_t desired) {
    cui_plot_result status = cui_plot_validate(s);
    if (status != CUI_PLOT_OK)
        return status;
    if (desired < 2 || desired > 1000)
        return CUI_PLOT_INVALID;
    long double step = cui_plot_tick_step(s, desired);
    long double lo = floorl(cui_plot_transform(s->kind, s->domain_min) / step) * step;
    long double hi = ceill(cui_plot_transform(s->kind, s->domain_max) / step) * step;
    cui_plot_scale next = *s;
    if (cui_plot_untransform(s->kind, lo, &next.domain_min) != CUI_PLOT_OK)
        next.domain_min = s->kind == CUI_PLOT_LOG10 ? DBL_TRUE_MIN : -DBL_MAX;
    if (cui_plot_untransform(s->kind, hi, &next.domain_max) != CUI_PLOT_OK)
        next.domain_max = DBL_MAX;
    next.domain_min = fmin(next.domain_min, s->domain_min);
    next.domain_max = fmax(next.domain_max, s->domain_max);
    status = cui_plot_validate(&next);
    if (status == CUI_PLOT_OK)
        *s = next;
    return status;
}
static cui_plot_result cui_plot_set_transformed(cui_plot_scale *s, long double a, long double b) {
    cui_plot_scale next = *s;
    cui_plot_result status = cui_plot_untransform(s->kind, a, &next.domain_min);
    if (status != CUI_PLOT_OK)
        return status;
    status = cui_plot_untransform(s->kind, b, &next.domain_max);
    if (status != CUI_PLOT_OK)
        return status;
    status = cui_plot_validate(&next);
    if (status == CUI_PLOT_OK)
        *s = next;
    return status;
}
cui_plot_result cui_plot_pan(cui_plot_scale *s, double delta_pixels) {
    cui_plot_result status = cui_plot_validate(s);
    if (status != CUI_PLOT_OK)
        return status;
    if (!isfinite(delta_pixels))
        return CUI_PLOT_INVALID;
    if (delta_pixels == 0)
        return CUI_PLOT_OK;
    long double a = cui_plot_transform(s->kind, s->domain_min);
    long double b = cui_plot_transform(s->kind, s->domain_max);
    long double span = (long double)s->range_end - s->range_start;
    long double ratio = isfinite(span)
                            ? delta_pixels / span
                            : ((long double)delta_pixels / 2) /
                                  ((long double)s->range_end / 2 - (long double)s->range_start / 2);
    long double shift = -ratio * b + ratio * a;
    return cui_plot_set_transformed(s, a + shift, b + shift);
}
cui_plot_result cui_plot_zoom(cui_plot_scale *s, double factor, double anchor) {
    cui_plot_result status = cui_plot_validate(s);
    if (status != CUI_PLOT_OK)
        return status;
    if (!isfinite(factor) || factor <= 0 || !isfinite(anchor))
        return CUI_PLOT_INVALID;
    if (factor == 1)
        return CUI_PLOT_OK;
    long double a = cui_plot_transform(s->kind, s->domain_min);
    long double b = cui_plot_transform(s->kind, s->domain_max);
    long double t = cui_plot_fraction(anchor, s->range_start, s->range_end);
    long double fixed = cui_plot_lerp(a, b, t);
    long double da = a - fixed, db = b - fixed;
    long double offset_a = isfinite(da) ? da / factor : a / factor - fixed / factor;
    long double offset_b = isfinite(db) ? db / factor : b / factor - fixed / factor;
    return cui_plot_set_transformed(s, fixed + offset_a, fixed + offset_b);
}
static bool cui_plot_rect_valid(cui_plot_rect r) {
    return isfinite(r.x_min) && isfinite(r.y_min) && isfinite(r.x_max) && isfinite(r.y_max) &&
           r.x_min < r.x_max && r.y_min < r.y_max;
}
static unsigned cui_plot_outcode(cui_plot_point p, cui_plot_rect r) {
    return (p.x < r.x_min   ? 1u
            : p.x > r.x_max ? 2u
                            : 0u) |
           (p.y < r.y_min   ? 4u
            : p.y > r.y_max ? 8u
                            : 0u);
}
cui_plot_result cui_plot_clip_line(cui_plot_line line, cui_plot_rect r, cui_plot_line *out) {
    if (!out || !cui_plot_rect_valid(r) || !isfinite(line.start.x) || !isfinite(line.start.y) ||
        !isfinite(line.end.x) || !isfinite(line.end.y))
        return CUI_PLOT_INVALID;
    for (unsigned iteration = 0; iteration < 16; ++iteration) {
        unsigned a = cui_plot_outcode(line.start, r), b = cui_plot_outcode(line.end, r);
        if (!(a | b)) {
            *out = line;
            return CUI_PLOT_OK;
        }
        if (a & b)
            return CUI_PLOT_EMPTY;
        unsigned code = a ? a : b;
        cui_plot_point p;
        if (code & (4u | 8u)) {
            p.y = code & 8u ? r.y_max : r.y_min;
            long double t = cui_plot_fraction(p.y, line.start.y, line.end.y);
            cui_plot_result status =
                cui_plot_number(cui_plot_lerp(line.start.x, line.end.x, t), &p.x);
            if (status != CUI_PLOT_OK)
                return status;
        } else {
            p.x = code & 2u ? r.x_max : r.x_min;
            long double t = cui_plot_fraction(p.x, line.start.x, line.end.x);
            cui_plot_result status =
                cui_plot_number(cui_plot_lerp(line.start.y, line.end.y, t), &p.y);
            if (status != CUI_PLOT_OK)
                return status;
        }
        if (a)
            line.start = p;
        else
            line.end = p;
    }
    return CUI_PLOT_OVERFLOW;
}
cui_plot_result cui_plot_project_line(const cui_plot_scale *x, const cui_plot_scale *y,
                                      cui_plot_point a, cui_plot_point b, cui_plot_rect r,
                                      cui_plot_line *out) {
    cui_plot_line line;
    cui_plot_result status;
    if ((status = cui_plot_map(x, a.x, &line.start.x)) != CUI_PLOT_OK ||
        (status = cui_plot_map(y, a.y, &line.start.y)) != CUI_PLOT_OK ||
        (status = cui_plot_map(x, b.x, &line.end.x)) != CUI_PLOT_OK ||
        (status = cui_plot_map(y, b.y, &line.end.y)) != CUI_PLOT_OK)
        return status;
    return cui_plot_clip_line(line, r, out);
}
cui_plot_result cui_plot_project_bar(const cui_plot_scale *x, const cui_plot_scale *y,
                                     double center, double width, double value, double baseline,
                                     cui_plot_rect viewport, cui_plot_rect *out) {
    if (!out || !cui_plot_rect_valid(viewport) || !isfinite(width) || width <= 0)
        return CUI_PLOT_INVALID;
    double px, py, base;
    cui_plot_result status;
    if ((status = cui_plot_map(x, center, &px)) != CUI_PLOT_OK ||
        (status = cui_plot_map(y, value, &py)) != CUI_PLOT_OK ||
        (status = cui_plot_map(y, baseline, &base)) != CUI_PLOT_OK)
        return status;
    cui_plot_rect r = {fmax(viewport.x_min, px - width / 2), fmax(viewport.y_min, fmin(py, base)),
                       fmin(viewport.x_max, px + width / 2), fmin(viewport.y_max, fmax(py, base))};
    if (r.x_min >= r.x_max || r.y_min >= r.y_max)
        return CUI_PLOT_EMPTY;
    *out = r;
    return CUI_PLOT_OK;
}

static cui_plot_result cui_plot_categories(const uint64_t *keys, size_t count) {
    if (count > CUI_PLOT_CATEGORY_LIMIT)
        return CUI_PLOT_CAPACITY;
    return count && !keys ? CUI_PLOT_INVALID : CUI_PLOT_OK;
}
static cui_plot_result cui_plot_band_geometry_for(size_t count, cui_plot_band_options o,
                                                  cui_plot_band_geometry *out) {
    if (!isfinite(o.range_start) || !isfinite(o.range_end) || !isfinite(o.padding_inner) ||
        o.padding_inner < 0 || o.padding_inner > 1 || !isfinite(o.padding_outer) ||
        o.padding_outer < 0 ||
        (o.has_max_band_width && (!isfinite(o.max_band_width) || o.max_band_width < 0)))
        return CUI_PLOT_INVALID;
    size_t bands = count > o.band_count ? count : o.band_count;
    if (bands > CUI_PLOT_CATEGORY_LIMIT)
        return CUI_PLOT_CAPACITY;
    if (bands && o.padding_outer > (double)bands / 2)
        return CUI_PLOT_INVALID;
    long double low = fmin(o.range_start, o.range_end);
    long double high = fmax(o.range_start, o.range_end);
    long double span = high - low;
    long double average = bands ? span / bands : 0;
    long double width = average * (1 - o.padding_inner);
    if (o.has_max_band_width)
        width = fminl(width, o.max_band_width);
    long double step = bands <= 1 ? span
                                  : average * (1 - 2 * (long double)o.padding_outer / bands) *
                                        (1 + (long double)o.padding_inner / (bands - 1));
    long double start =
        bands == 1 ? cui_plot_lerp(low, high, .5L) - width / 2 : low + average * o.padding_outer;
    cui_plot_band_geometry result = {.band_count = bands};
    cui_plot_result status = cui_plot_number(start, &result.start);
    if (status == CUI_PLOT_OK)
        status = cui_plot_number(step, &result.step);
    if (status == CUI_PLOT_OK)
        status = cui_plot_number(width, &result.band_width);
    if (status == CUI_PLOT_OK)
        *out = result;
    return status;
}
cui_plot_result cui_plot_band_layout(const cui_plot_band_scale *s, cui_plot_band_geometry *out) {
    if (!s || !out || s->count > s->capacity)
        return CUI_PLOT_INVALID;
    cui_plot_result status = cui_plot_categories(s->domain, s->count);
    return status == CUI_PLOT_OK ? cui_plot_band_geometry_for(s->count, s->options, out) : status;
}
cui_plot_result cui_plot_band_init(cui_plot_band_scale *s, uint64_t *storage, size_t capacity,
                                   const uint64_t *domain, size_t count,
                                   cui_plot_band_options options, size_t *required) {
    if (!s || !required || (capacity && !storage))
        return CUI_PLOT_INVALID;
    cui_plot_result status = cui_plot_categories(domain, count);
    if (status != CUI_PLOT_OK)
        return status;
    uint64_t distinct[CUI_PLOT_CATEGORY_LIMIT];
    size_t unique = 0;
    for (size_t i = 0; i < count; ++i) {
        size_t at = 0;
        while (at < unique && distinct[at] != domain[i])
            ++at;
        if (at == unique)
            distinct[unique++] = domain[i];
    }
    cui_plot_band_geometry geometry;
    status = cui_plot_band_geometry_for(unique, options, &geometry);
    if (status != CUI_PLOT_OK)
        return status;
    *required = unique;
    if (capacity < unique)
        return CUI_PLOT_CAPACITY;
    if (unique)
        memmove(storage, distinct, unique * sizeof *storage);
    *s = (cui_plot_band_scale){storage, unique, capacity, options};
    return CUI_PLOT_OK;
}
cui_plot_result cui_plot_band_at(const cui_plot_band_scale *s, size_t index, double *pixel) {
    if (!pixel)
        return CUI_PLOT_INVALID;
    cui_plot_band_geometry geometry;
    cui_plot_result status = cui_plot_band_layout(s, &geometry);
    if (status != CUI_PLOT_OK)
        return status;
    if (index >= s->count)
        return CUI_PLOT_EMPTY;
    return cui_plot_number((long double)geometry.start + index * (long double)geometry.step, pixel);
}
cui_plot_result cui_plot_band_map(const cui_plot_band_scale *s, uint64_t key, double *pixel) {
    if (!pixel)
        return CUI_PLOT_INVALID;
    cui_plot_band_geometry geometry;
    cui_plot_result status = cui_plot_band_layout(s, &geometry);
    if (status != CUI_PLOT_OK)
        return status;
    for (size_t i = 0; i < s->count; ++i)
        if (s->domain[i] == key)
            return cui_plot_band_at(s, i, pixel);
    return CUI_PLOT_EMPTY;
}
static size_t cui_plot_nearest_category(long double index, size_t count) {
    if (!count || index <= 0)
        return 0;
    if (!isfinite(index) || index >= (long double)(count - 1))
        return count - 1;
    return (size_t)floorl(index + .5L);
}
cui_plot_result cui_plot_band_nearest(const cui_plot_band_scale *s, double pixel, size_t *index) {
    if (!index || !isfinite(pixel))
        return CUI_PLOT_INVALID;
    cui_plot_band_geometry geometry;
    cui_plot_result status = cui_plot_band_layout(s, &geometry);
    if (status != CUI_PLOT_OK)
        return status;
    size_t result = 0;
    if (geometry.band_count > 1) {
        if (geometry.step == 0)
            result = pixel > geometry.start ? geometry.band_count - 1 : 0;
        else {
            long double normalized = ((long double)pixel - geometry.start) / geometry.step;
            result = cui_plot_nearest_category(normalized, geometry.band_count);
        }
    }
    *index = result;
    return CUI_PLOT_OK;
}
static cui_plot_result cui_plot_point_valid(const cui_plot_point_scale *s) {
    if (!s || !isfinite(s->range_start) || !isfinite(s->range_end))
        return CUI_PLOT_INVALID;
    return cui_plot_categories(s->domain, s->count);
}
cui_plot_result cui_plot_point_init(cui_plot_point_scale *s, const uint64_t *domain, size_t count,
                                    double start, double end) {
    if (!s)
        return CUI_PLOT_INVALID;
    cui_plot_point_scale candidate = {domain, count, start, end};
    cui_plot_result status = cui_plot_point_valid(&candidate);
    if (status == CUI_PLOT_OK)
        *s = candidate;
    return status;
}
cui_plot_result cui_plot_point_at(const cui_plot_point_scale *s, size_t index, double *pixel) {
    if (!pixel)
        return CUI_PLOT_INVALID;
    cui_plot_result status = cui_plot_point_valid(s);
    if (status != CUI_PLOT_OK)
        return status;
    if (index >= s->count)
        return CUI_PLOT_EMPTY;
    long double fraction = s->count == 1 ? .5L : (long double)index / (s->count - 1);
    return cui_plot_number(cui_plot_lerp(s->range_start, s->range_end, fraction), pixel);
}
cui_plot_result cui_plot_point_map(const cui_plot_point_scale *s, uint64_t key, double *pixel) {
    if (!pixel)
        return CUI_PLOT_INVALID;
    cui_plot_result status = cui_plot_point_valid(s);
    if (status != CUI_PLOT_OK)
        return status;
    for (size_t i = 0; i < s->count; ++i)
        if (s->domain[i] == key)
            return cui_plot_point_at(s, i, pixel);
    return CUI_PLOT_EMPTY;
}
cui_plot_result cui_plot_point_nearest(const cui_plot_point_scale *s, double pixel, size_t *index) {
    if (!index || !isfinite(pixel))
        return CUI_PLOT_INVALID;
    cui_plot_result status = cui_plot_point_valid(s);
    if (status != CUI_PLOT_OK)
        return status;
    size_t result = 0;
    if (s->count > 1 && s->range_start != s->range_end) {
        long double fraction = cui_plot_fraction(pixel, s->range_start, s->range_end);
        result = cui_plot_nearest_category(fraction * (s->count - 1), s->count);
    }
    *index = result;
    return CUI_PLOT_OK;
}
static cui_plot_result cui_plot_ordinal_valid(const cui_plot_ordinal_scale *s) {
    if (!s || !s->element_size || (s->range_count && !s->range) || (s->has_unknown && !s->unknown))
        return CUI_PLOT_INVALID;
    cui_plot_result status = cui_plot_categories(s->domain, s->count);
    if (status != CUI_PLOT_OK)
        return status;
    if (s->range_count > CUI_PLOT_CATEGORY_LIMIT || s->element_size > CUI_PLOT_ORDINAL_BYTE_LIMIT ||
        s->range_count > CUI_PLOT_ORDINAL_BYTE_LIMIT / s->element_size)
        return CUI_PLOT_CAPACITY;
    return CUI_PLOT_OK;
}
cui_plot_result cui_plot_ordinal_init(cui_plot_ordinal_scale *s, const uint64_t *domain,
                                      size_t count, const void *range, size_t range_count,
                                      size_t element_size, const void *unknown) {
    if (!s)
        return CUI_PLOT_INVALID;
    cui_plot_ordinal_scale candidate = {domain,      count,        range,          unknown,
                                        range_count, element_size, unknown != NULL};
    cui_plot_result status = cui_plot_ordinal_valid(&candidate);
    if (status == CUI_PLOT_OK)
        *s = candidate;
    return status;
}
cui_plot_result cui_plot_ordinal_map(const cui_plot_ordinal_scale *s, uint64_t key, void *output,
                                     size_t capacity) {
    cui_plot_result status = cui_plot_ordinal_valid(s);
    if (status != CUI_PLOT_OK)
        return status;
    size_t at = 0;
    while (at < s->count && s->domain[at] != key)
        ++at;
    const void *value = NULL;
    if (at < s->count) {
        if (!s->range_count)
            return CUI_PLOT_EMPTY;
        value = (const unsigned char *)s->range + (at % s->range_count) * s->element_size;
    } else if (s->has_unknown)
        value = s->unknown;
    else
        return CUI_PLOT_EMPTY;
    if (capacity < s->element_size)
        return CUI_PLOT_CAPACITY;
    if (!output)
        return CUI_PLOT_INVALID;
    memmove(output, value, s->element_size);
    return CUI_PLOT_OK;
}
