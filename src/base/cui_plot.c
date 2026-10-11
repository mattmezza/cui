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
    long double step = power * (normalized <= 1   ? 1
                                : normalized <= 2 ? 2
                                : normalized <= 5 ? 5
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
    long double first = ceill(a / step);
    double values[CUI_PLOT_MAX_TICKS];
    size_t n = 0;
    for (size_t i = 0; i < CUI_PLOT_MAX_TICKS; ++i) {
        long double tick = (first + i) * step;
        if (tick > b)
            break;
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
