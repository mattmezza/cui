/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <float.h>
#include <math.h>
#include <string.h>
#define CUI_CHART_MAX_ITEMS 4096
#define CUI_CHART_PI 3.14159265358979323846
static cui_color cui_chart_palette(const cui_chart_options *o, size_t i) {
    cui_color c = o->data[i].color;
    if (c.a)
        return c;
    c = o->series[i % 8];
    return c.a ? c : (cui_color){64, 120, 220, 255};
}
static bool cui_chart_valid_str(cui_str s) {
    return !s.length || s.data;
}
static double cui_chart_sankey_scaled(const cui_chart_options *o, double value) {
    return o->sankey_value_scale == CUI_CHART_SANKEY_SQRT ? sqrt(value) : value;
}
static bool cui_chart_pie_radii(const cui_chart_options *o, size_t i, double *inner,
                                double *outer) {
    double base_outer = o->pie_outer_radius > 0 ? o->pie_outer_radius : o->height * .4;
    *inner = o->pie_inner_radius_fn
                 ? o->pie_inner_radius_fn(&o->data[i], i, o->inner_radius, o->pie_radius_user)
                 : o->inner_radius;
    *outer = o->pie_outer_radius_fn
                 ? o->pie_outer_radius_fn(&o->data[i], i, base_outer, o->pie_radius_user)
                 : base_outer;
    return isfinite(*inner) && isfinite(*outer) && *inner >= 0 && *outer >= *inner &&
           *outer <= 8192;
}
static bool cui_chart_sankey_margins_valid(const cui_chart_options *o) {
    cui_chart_sankey_margins m = o->sankey_margins;
    double width = o->node_width > 0 ? o->node_width : 10;
    return isfinite(m.left) && isfinite(m.right) && isfinite(m.top) && isfinite(m.bottom) &&
           m.left >= 0 && m.right >= 0 && m.top >= 0 && m.bottom >= 0 &&
           (double)m.left + m.right + width + 16 < o->width &&
           (double)m.top + m.bottom + 16 < o->height;
}
static cui_plot_result cui_chart_validate(const cui_chart_state *s, const cui_chart_options *o) {
    if (!s || !o || !s->pixels || !o->data || !o->count || o->count > CUI_CHART_MAX_ITEMS ||
        o->kind < CUI_CHART_LINE || o->kind > CUI_CHART_SANKEY || !o->width || !o->height ||
        o->curve < CUI_CHART_CURVE_NATURAL || o->curve > CUI_CHART_CURVE_STEP_AFTER ||
        o->extra_series_count > 7 || (o->extra_series_count && !o->extra_series) ||
        (o->extra_series_count && o->kind != CUI_CHART_LINE && o->kind != CUI_CHART_AREA &&
         o->kind != CUI_CHART_RADAR) ||
        o->width > 4096 || o->height > 4096 || o->width > SIZE_MAX / 4 / o->height ||
        ((o->kind == CUI_CHART_PIE || o->kind == CUI_CHART_RADAR || o->kind == CUI_CHART_SANKEY) &&
         o->count > 256) ||
        !isfinite(o->inner_radius) || o->inner_radius < 0 || !isfinite(o->pie_outer_radius) ||
        o->pie_outer_radius < 0 || !isfinite(o->pie_label_gap) || o->pie_label_gap < 0 ||
        !isfinite(o->candle_body_ratio) || o->candle_body_ratio < 0 || o->candle_body_ratio > 1 ||
        !isfinite(o->pad_angle) || o->pad_angle < 0 || o->pad_angle > 2 * CUI_CHART_PI ||
        !isfinite(o->bar_ratio) || o->bar_ratio < 0 || o->bar_ratio > 1 || o->tick_count == 1 ||
        o->tick_count > 1000 || o->bar_alignment < CUI_CHART_BAR_BOTTOM ||
        o->bar_alignment > CUI_CHART_BAR_RIGHT || !isfinite(o->bar_padding_inner) ||
        !isfinite(o->bar_padding_outer) || !isfinite(o->max_bar_width) ||
        !isfinite(o->min_bar_length) || o->bar_padding_inner < 0 || o->bar_padding_inner > 1 ||
        o->bar_padding_outer < 0 || o->bar_padding_outer > 1 || o->max_bar_width < 0 ||
        o->min_bar_length < 0 || !isfinite(o->radar_max_value) || !isfinite(o->bar_corner_radius) ||
        o->bar_corner_radius < 0 || !isfinite(o->reveal_progress) || o->reveal_progress < 0 ||
        o->reveal_progress > 1 || !isfinite(o->appear_duration) || o->appear_duration < 0 ||
        (o->bar_gradient_start.a == 0) != (o->bar_gradient_end.a == 0) || o->radar_max_value < 0 ||
        !isfinite(o->radar_outer_radius) || o->radar_outer_radius < 0 || o->x_tick_count > 1000 ||
        o->grid_columns > 1000 || o->reference_count > 16 || o->sankey_iterations > 100 ||
        o->sankey_value_scale < CUI_CHART_SANKEY_LINEAR ||
        o->sankey_value_scale > CUI_CHART_SANKEY_SQRT || !isfinite(o->sankey_min_link_width) ||
        o->sankey_min_link_width < 0 || !isfinite(o->sankey_link_opacity) ||
        o->sankey_link_opacity < 0 || o->sankey_link_opacity > 1 ||
        !isfinite(o->sankey_node_corner_radius) || o->sankey_node_corner_radius < 0 ||
        !isfinite(o->sankey_label_gap) || o->sankey_label_gap < 0 ||
        !isfinite(o->sankey_font_size) || o->sankey_font_size < 0 ||
        !isfinite(o->radar_label_gap) || o->radar_label_gap < 0 ||
        (o->reference_count && !o->reference_values) ||
        (o->point_count && (o->point_count < o->count || o->point_count > 1000000)) ||
        !isfinite(o->logical_width) || !isfinite(o->logical_height) || o->logical_width < 0 ||
        o->logical_height < 0 || !isfinite(o->value_axis_gutter) || o->value_axis_gutter < 0 ||
        o->value_axis_gutter >= o->width || !isfinite(o->y_padding_top) ||
        !isfinite(o->y_padding_bottom) || o->y_padding_top < 0 || o->y_padding_bottom < 0 ||
        (o->custom_y_padding && (o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA) &&
         (double)o->y_padding_top + o->y_padding_bottom >= o->height - 20) ||
        (o->has_domain &&
         (!isfinite(o->domain_min) || !isfinite(o->domain_max) || o->domain_min >= o->domain_max)))
        return CUI_PLOT_INVALID;
    if (s->byte_capacity < (size_t)o->width * o->height * 4)
        return CUI_PLOT_CAPACITY;
    if (o->kind == CUI_CHART_SANKEY && !cui_chart_sankey_margins_valid(o))
        return CUI_PLOT_INVALID;
    for (size_t i = 0; i < 4; i++)
        if (!isfinite(o->bar_corner_radii[i]) || o->bar_corner_radii[i] < 0)
            return CUI_PLOT_INVALID;
    for (size_t i = 0; i < o->count; i++) {
        const cui_chart_datum *d = &o->data[i];
        if (!cui_chart_valid_str(d->label) || !isfinite(d->value))
            return CUI_PLOT_INVALID;
        if (o->kind == CUI_CHART_CANDLESTICK &&
            (!isfinite(d->open) || !isfinite(d->high) || !isfinite(d->low) || !isfinite(d->close) ||
             d->low > d->high || d->open < d->low || d->open > d->high || d->close < d->low ||
             d->close > d->high))
            return CUI_PLOT_INVALID;
        if ((o->kind == CUI_CHART_PIE || o->kind == CUI_CHART_SANKEY) && d->value < 0)
            return CUI_PLOT_INVALID;
        if (o->kind == CUI_CHART_PIE) {
            double inner, outer;
            if (!cui_chart_pie_radii(o, i, &inner, &outer))
                return CUI_PLOT_INVALID;
        }
    }
    for (size_t series = 0; series < o->extra_series_count; series++) {
        const cui_chart_series *extra = &o->extra_series[series];
        if (!extra->values || extra->count != o->count || !cui_chart_valid_str(extra->name) ||
            (extra->custom_curve &&
             (extra->curve < CUI_CHART_CURVE_NATURAL || extra->curve > CUI_CHART_CURVE_STEP_AFTER)))
            return CUI_PLOT_INVALID;
        for (size_t i = 0; i < o->count; i++)
            if (!isfinite(extra->values[i]))
                return CUI_PLOT_INVALID;
    }
    for (size_t i = 0; i < o->reference_count; i++)
        if (!isfinite(o->reference_values[i]))
            return CUI_PLOT_INVALID;
    if (o->kind == CUI_CHART_SANKEY) {
        if (o->link_count > 1024 || (o->link_count && !o->links))
            return CUI_PLOT_INVALID;
        for (size_t i = 0; i < o->link_count; i++)
            if (o->links[i].source >= o->count || o->links[i].target >= o->count ||
                o->links[i].source == o->links[i].target || !isfinite(o->links[i].value) ||
                o->links[i].value < 0)
                return CUI_PLOT_INVALID;
    }
    if (o->kind == CUI_CHART_PIE) {
        long double total = 0;
        for (size_t i = 0; i < o->count; i++)
            total += o->data[i].value;
        if (total > DBL_MAX)
            return CUI_PLOT_OVERFLOW;
    }
    return CUI_PLOT_OK;
}
static void cui_chart_pixel(cui_chart_state *s, const cui_chart_options *o, int x, int y,
                            cui_color c) {
    if (x < 0 || y < 0 || (unsigned)x >= o->width || (unsigned)y >= o->height || !c.a ||
        (s->mark_clip_active && (x + .5 < s->mark_clip_left || x + .5 > s->mark_clip_x)))
        return;
    unsigned char *p = s->pixels + ((size_t)y * o->width + (unsigned)x) * 4;
    unsigned a = c.a, inv = 255 - a, back = p[3];
    unsigned out = a + (back * inv + 127) / 255;
    if (!out)
        return;
    p[0] = (unsigned char)((c.r * a + (p[0] * back * inv + 127) / 255 + out / 2) / out);
    p[1] = (unsigned char)((c.g * a + (p[1] * back * inv + 127) / 255 + out / 2) / out);
    p[2] = (unsigned char)((c.b * a + (p[2] * back * inv + 127) / 255 + out / 2) / out);
    p[3] = (unsigned char)out;
}
static void cui_chart_rect(cui_chart_state *s, const cui_chart_options *o, int x0, int y0, int x1,
                           int y1, cui_color c) {
    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 > (int)o->width)
        x1 = (int)o->width;
    if (y1 > (int)o->height)
        y1 = (int)o->height;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            cui_chart_pixel(s, o, x, y, c);
}
static void cui_chart_rounded_rect(cui_chart_state *s, const cui_chart_options *o,
                                   cui_plot_rect bounds, double requested_radius, cui_color color) {
    double radius = fmin(requested_radius, fmin((bounds.x_max - bounds.x_min) / 2,
                                                (bounds.y_max - bounds.y_min) / 2));
    int x0 = (int)fmax(0, floor(bounds.x_min)), x1 = (int)fmin(o->width, ceil(bounds.x_max));
    int y0 = (int)fmax(0, floor(bounds.y_min)), y1 = (int)fmin(o->height, ceil(bounds.y_max));
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            double px = x + .5, py = y + .5;
            if (px < bounds.x_min || px >= bounds.x_max || py < bounds.y_min || py >= bounds.y_max)
                continue;
            double cx = fmax(bounds.x_min + radius, fmin(bounds.x_max - radius, px));
            double cy = fmax(bounds.y_min + radius, fmin(bounds.y_max - radius, py));
            if (radius > 0 && (px - cx) * (px - cx) + (py - cy) * (py - cy) > radius * radius)
                continue;
            cui_chart_pixel(s, o, x, y, color);
        }
}
static void cui_chart_line(cui_chart_state *s, const cui_chart_options *o, double ax, double ay,
                           double bx, double by, cui_color c) {
    double dx = bx - ax, dy = by - ay, steps = fmax(fabs(dx), fabs(dy));
    if (!isfinite(steps) || steps > 20000)
        return;
    if (steps < 1) {
        cui_chart_pixel(s, o, (int)lround(ax), (int)lround(ay), c);
        return;
    }
    for (int i = 0; i <= (int)ceil(steps); i++) {
        double t = i / ceil(steps);
        cui_chart_pixel(s, o, (int)lround(ax + dx * t), (int)lround(ay + dy * t), c);
    }
}
static void cui_chart_guide(cui_chart_state *s, const cui_chart_options *o, double ax, double ay,
                            double bx, double by, cui_color c, bool solid) {
    if (solid) {
        cui_chart_line(s, o, ax, ay, bx, by, c);
        return;
    }
    double distance = hypot(bx - ax, by - ay);
    if (!isfinite(distance) || distance <= 0)
        return;
    for (double start = 0; start < distance; start += 6) {
        double end = fmin(distance, start + 4);
        cui_chart_line(s, o, ax + (bx - ax) * start / distance, ay + (by - ay) * start / distance,
                       ax + (bx - ax) * end / distance, ay + (by - ay) * end / distance, c);
    }
}
static double cui_chart_plot_left(const cui_chart_options *o) {
    if (o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA)
        return o->show_y_axis && !o->y_axis_inside
                   ? (o->value_axis_gutter > 0 ? o->value_axis_gutter : 32)
                   : 0;
    if (o->kind == CUI_CHART_BAR &&
        (o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT))
        return 24;
    return o->show_y_axis && !o->y_axis_inside
               ? (o->value_axis_gutter > 0 ? o->value_axis_gutter : 50)
               : 24;
}
static double cui_chart_plot_right(const cui_chart_options *o, double left) {
    return o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA
               ? o->width - left
               : (o->width > 36 ? o->width - 12 : left);
}
static double cui_chart_plot_bottom(const cui_chart_options *o) {
    bool horizontal_bar = o->kind == CUI_CHART_BAR && (o->bar_alignment == CUI_CHART_BAR_LEFT ||
                                                       o->bar_alignment == CUI_CHART_BAR_RIGHT);
    double gap = horizontal_bar && o->show_y_axis && !o->y_axis_inside ? 32 : 20;
    return o->height > gap + 12 ? o->height - gap : 12;
}
static double cui_chart_x_at(const cui_chart_options *o, size_t i, double left, double right) {
    size_t n = o->point_count ? o->point_count : o->count;
    return n > 1 ? left + (right - left) * i / (n - 1) : (left + right) / 2;
}
static double cui_chart_map_y(double v, double lo, double hi, double top, double bottom) {
    cui_plot_scale scale = {CUI_PLOT_LINEAR, lo, hi, bottom, top, true};
    double mapped = bottom;
    (void)cui_plot_map(&scale, v, &mapped);
    return mapped;
}
static void cui_chart_domain(const cui_chart_options *o, double *lo, double *hi) {
    if (o->has_domain) {
        *lo = o->domain_min;
        *hi = o->domain_max;
        return;
    }
    *lo = o->kind == CUI_CHART_CANDLESTICK ? o->data[0].low : 0;
    *hi = o->kind == CUI_CHART_CANDLESTICK ? o->data[0].high : 0;
    for (size_t i = 0; i < o->count; i++) {
        double a = o->kind == CUI_CHART_CANDLESTICK ? o->data[i].low : o->data[i].value;
        double b = o->kind == CUI_CHART_CANDLESTICK ? o->data[i].high : o->data[i].value;
        if (a < *lo)
            *lo = a;
        if (b > *hi)
            *hi = b;
    }
    for (size_t series = 0; series < o->extra_series_count; series++)
        for (size_t i = 0; i < o->count; i++) {
            double value = o->extra_series[series].values[i];
            if (value < *lo)
                *lo = value;
            if (value > *hi)
                *hi = value;
        }
    if (*lo == *hi) {
        if (*hi == 0)
            *hi = 1;
        else {
            double delta = fmax(1, fabs(*lo) * .01);
            *lo = fmax(-DBL_MAX, *lo - delta);
            *hi = fmin(DBL_MAX, *hi + delta);
        }
    }
}
bool cui_chart_value_domain(const cui_chart_options *o, double *minimum, double *maximum) {
    if (!o || !minimum || !maximum || !o->data || !o->count || o->count > CUI_CHART_MAX_ITEMS ||
        o->extra_series_count > 7 || (o->extra_series_count && !o->extra_series))
        return false;
    for (size_t i = 0; i < o->count; i++)
        if (!isfinite(o->data[i].value) ||
            (o->kind == CUI_CHART_CANDLESTICK &&
             (!isfinite(o->data[i].low) || !isfinite(o->data[i].high) ||
              o->data[i].low > o->data[i].high)))
            return false;
    for (size_t series = 0; series < o->extra_series_count; series++) {
        const cui_chart_series *extra = &o->extra_series[series];
        if (!extra->values || extra->count != o->count)
            return false;
        for (size_t i = 0; i < o->count; i++)
            if (!isfinite(extra->values[i]))
                return false;
    }
    cui_chart_domain(o, minimum, maximum);
    return isfinite(*minimum) && isfinite(*maximum) && *minimum < *maximum;
}
static cui_plot_result cui_chart_bar_bounds_domain(const cui_chart_options *o, size_t index,
                                                   double lo, double hi, cui_plot_rect *output) {
    double left = cui_chart_plot_left(o), right = cui_chart_plot_right(o, left);
    double top = 12, bottom = cui_chart_plot_bottom(o);
    if (right <= left || bottom <= top)
        return CUI_PLOT_EMPTY;
    bool horizontal =
        o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT;
    size_t slots = o->point_count ? o->point_count : o->count;
    double start = horizontal ? top : left, extent = horizontal ? bottom - top : right - left;
    double inner = o->custom_bar_padding ? o->bar_padding_inner : .4;
    double outer = o->custom_bar_padding ? o->bar_padding_outer : .2;
    double avg = extent / slots, outer_width = avg * outer;
    double width = avg * (o->bar_ratio ? o->bar_ratio : 1 - inner);
    width = fmin(width, o->max_bar_width > 0 ? o->max_bar_width : 30);
    double display = (extent - 2 * outer_width) / slots;
    double step = slots > 1 ? display * (1 + inner / (slots - 1)) : extent;
    double band_start =
        slots > 1 ? start + index * step + outer_width : start + (extent - width) / 2;
    cui_plot_scale scale = {CUI_PLOT_LINEAR, lo, hi, 0, 0, true};
    if (horizontal) {
        scale.range_start = o->bar_alignment == CUI_CHART_BAR_LEFT ? left : right;
        scale.range_end = o->bar_alignment == CUI_CHART_BAR_LEFT ? right : left;
    } else {
        scale.range_start = o->bar_alignment == CUI_CHART_BAR_TOP ? top : bottom;
        scale.range_end = o->bar_alignment == CUI_CHART_BAR_TOP ? bottom : top;
    }
    double at, zero;
    if (cui_plot_map(&scale, o->data[index].value, &at) != CUI_PLOT_OK ||
        cui_plot_map(&scale, 0, &zero) != CUI_PLOT_OK)
        return CUI_PLOT_OVERFLOW;
    if (fabs(at - zero) < o->min_bar_length) {
        bool negative = o->data[index].value < 0;
        bool toward_origin =
            o->bar_alignment == CUI_CHART_BAR_BOTTOM || o->bar_alignment == CUI_CHART_BAR_RIGHT;
        at = zero + (toward_origin != negative ? -o->min_bar_length : o->min_bar_length);
    }
    cui_plot_rect r;
    if (horizontal)
        r = (cui_plot_rect){fmax(left, fmin(at, zero)), fmax(top, band_start),
                            fmin(right, fmax(at, zero)), fmin(bottom, band_start + width)};
    else
        r = (cui_plot_rect){fmax(left, band_start), fmax(top, fmin(at, zero)),
                            fmin(right, band_start + width), fmin(bottom, fmax(at, zero))};
    if (r.x_min >= r.x_max || r.y_min >= r.y_max)
        return CUI_PLOT_EMPTY;
    *output = r;
    return CUI_PLOT_OK;
}
cui_plot_result cui_chart_bar_bounds(const cui_chart_options *o, size_t index, cui_plot_rect *out) {
    if (!o || !out || o->kind != CUI_CHART_BAR || !o->data || !o->count ||
        o->count > CUI_CHART_MAX_ITEMS || index >= o->count ||
        o->bar_alignment < CUI_CHART_BAR_BOTTOM || o->bar_alignment > CUI_CHART_BAR_RIGHT ||
        (o->point_count && (o->point_count < o->count || o->point_count > 1000000)) ||
        !isfinite(o->bar_padding_inner) || !isfinite(o->bar_padding_outer) ||
        !isfinite(o->max_bar_width) || !isfinite(o->min_bar_length) || o->bar_padding_inner < 0 ||
        o->bar_padding_inner > 1 || o->bar_padding_outer < 0 || o->bar_padding_outer > 1 ||
        o->max_bar_width < 0 || o->min_bar_length < 0 || !isfinite(o->value_axis_gutter) ||
        o->value_axis_gutter < 0 || o->value_axis_gutter >= o->width)
        return CUI_PLOT_INVALID;
    double lo, hi;
    if (!cui_chart_value_domain(o, &lo, &hi))
        return CUI_PLOT_INVALID;
    return cui_chart_bar_bounds_domain(o, index, lo, hi, out);
}
cui_plot_result cui_chart_candle_bounds(const cui_chart_options *o, size_t index,
                                        cui_chart_candle_geometry *out) {
    if (!o || !out || o->kind != CUI_CHART_CANDLESTICK || !o->data || !o->count ||
        o->count > CUI_CHART_MAX_ITEMS || index >= o->count || !o->width || !o->height ||
        o->width > 4096 || o->height > 4096 ||
        (o->point_count && (o->point_count < o->count || o->point_count > 1000000)) ||
        (o->has_domain && (!isfinite(o->domain_min) || !isfinite(o->domain_max) ||
                           o->domain_min >= o->domain_max)) ||
        !isfinite(o->max_bar_width) || o->max_bar_width < 0 || !isfinite(o->candle_body_ratio) ||
        o->candle_body_ratio < 0 || o->candle_body_ratio > 1 || !isfinite(o->value_axis_gutter) ||
        o->value_axis_gutter < 0 || o->value_axis_gutter >= o->width)
        return CUI_PLOT_INVALID;
    for (size_t i = 0; i < o->count; i++) {
        const cui_chart_datum *d = &o->data[i];
        if (!isfinite(d->open) || !isfinite(d->high) || !isfinite(d->low) || !isfinite(d->close) ||
            d->low > d->high || d->open < d->low || d->open > d->high || d->close < d->low ||
            d->close > d->high)
            return CUI_PLOT_INVALID;
    }
    double lo, hi;
    cui_chart_domain(o, &lo, &hi);
    if (!isfinite(lo) || !isfinite(hi) || lo >= hi)
        return CUI_PLOT_OVERFLOW;
    double left = cui_chart_plot_left(o), right = cui_chart_plot_right(o, left);
    double top = 12, bottom = o->height > 32 ? o->height - 20 : 12;
    if (right <= left || bottom <= top)
        return CUI_PLOT_EMPTY;
    size_t slots = o->point_count ? o->point_count : o->count;
    double extent = right - left, average = extent / slots;
    double outer = average * .2;
    double display = (extent - 2 * outer) / slots;
    double step = slots > 1 ? display * (1 + .4 / (slots - 1)) : extent;
    double band = fmin(o->max_bar_width > 0 ? o->max_bar_width : 30, average * .6);
    double center = slots > 1 ? left + outer + index * step + band / 2 : (left + right) / 2;
    double width = band * (o->candle_body_ratio ? o->candle_body_ratio : .8);
    const cui_chart_datum *d = &o->data[index];
    double y_low = cui_chart_map_y(d->low, lo, hi, top, bottom);
    double y_high = cui_chart_map_y(d->high, lo, hi, top, bottom);
    double y_open = cui_chart_map_y(d->open, lo, hi, top, bottom);
    double y_close = cui_chart_map_y(d->close, lo, hi, top, bottom);
    cui_chart_candle_geometry result = {.wick = {center, y_high, center, y_low},
                                        .body = {center - width / 2, fmin(y_open, y_close),
                                                 center + width / 2, fmax(y_open, y_close)},
                                        .bullish = d->close > d->open};
    *out = result;
    return CUI_PLOT_OK;
}
static double cui_chart_series_value(const cui_chart_options *o, size_t series, size_t point) {
    return series ? o->extra_series[series - 1].values[point] : o->data[point].value;
}
static void cui_chart_series_point(const cui_chart_options *o, size_t series, size_t point,
                                   double left, double right, double lo, double hi, double top,
                                   double bottom, double *x, double *y) {
    *x = cui_chart_x_at(o, point, left, right);
    *y = cui_chart_map_y(cui_chart_series_value(o, series, point), lo, hi, top, bottom);
    if (*y < top)
        *y = top;
    if (*y > bottom)
        *y = bottom;
}
static void cui_chart_draw_series(cui_chart_state *s, const cui_chart_options *o, size_t series,
                                  double left, double right, double top, double bottom, double lo,
                                  double hi, double baseline) {
    cui_color color = series ? o->extra_series[series - 1].color : o->series[0];
    if (!color.a)
        color = o->series[series].a ? o->series[series] : cui_chart_palette(o, 0);
    cui_color fill = color;
    fill.a = (unsigned char)(fill.a / 3);
    cui_color explicit_fill = series ? o->extra_series[series - 1].fill : o->primary_fill;
    if (explicit_fill.a)
        fill = explicit_fill;
    cui_chart_curve curve = series && o->extra_series[series - 1].custom_curve
                                ? o->extra_series[series - 1].curve
                                : o->curve;
    double first_x, first_y;
    cui_chart_series_point(o, series, 0, left, right, lo, hi, top, bottom, &first_x, &first_y);
    if (o->kind == CUI_CHART_AREA)
        cui_chart_rect(s, o, (int)floor(first_x), (int)floor(fmin(first_y, baseline)),
                       (int)ceil(first_x + 1), (int)ceil(fmax(first_y, baseline)), fill);
    for (size_t i = 1; i < o->count; i++) {
        double x0, y0, x1, y1;
        cui_chart_series_point(o, series, i - 1, left, right, lo, hi, top, bottom, &x0, &y0);
        cui_chart_series_point(o, series, i, left, right, lo, hi, top, bottom, &x1, &y1);
        if (curve == CUI_CHART_CURVE_STEP_AFTER) {
            cui_chart_line(s, o, x0, y0, x1, y0, color);
            if (i + 1 < o->count)
                cui_chart_line(s, o, x1, y0, x1, y1, color);
            if (o->kind == CUI_CHART_AREA)
                cui_chart_rect(s, o, (int)ceil(x0), (int)floor(fmin(y0, baseline)), (int)ceil(x1),
                               (int)ceil(fmax(y0, baseline)), fill);
        } else if (curve == CUI_CHART_CURVE_LINEAR) {
            cui_chart_line(s, o, x0, y0, x1, y1, color);
            if (o->kind == CUI_CHART_AREA)
                for (int x = (int)ceil(x0); x <= (int)floor(x1); x++) {
                    double y = y0 + (y1 - y0) * (x - x0) / (x1 - x0);
                    cui_chart_rect(s, o, x, (int)floor(fmin(y, baseline)), x + 1,
                                   (int)ceil(fmax(y, baseline)), fill);
                }
        } else {
            double prev_y = y0, prev_x = x0, p0y = y0, p3y = y1;
            if (i > 1) {
                double unused;
                cui_chart_series_point(o, series, i - 2, left, right, lo, hi, top, bottom, &unused,
                                       &p0y);
            }
            if (i + 1 < o->count) {
                double unused;
                cui_chart_series_point(o, series, i + 1, left, right, lo, hi, top, bottom, &unused,
                                       &p3y);
            }
            double c1 = y0 + (y1 - p0y) / 6, c2 = y1 - (p3y - y0) / 6;
            int steps = (int)fmax(4, ceil(x1 - x0) * 2);
            for (int step = 1; step <= steps; step++) {
                double t = (double)step / steps, u = 1 - t;
                double x = x0 + (x1 - x0) * t;
                double y =
                    u * u * u * y0 + 3 * u * u * t * c1 + 3 * u * t * t * c2 + t * t * t * y1;
                if (y < top)
                    y = top;
                if (y > bottom)
                    y = bottom;
                cui_chart_line(s, o, prev_x, prev_y, x, y, color);
                if (o->kind == CUI_CHART_AREA)
                    cui_chart_rect(s, o, (int)floor(x), (int)floor(fmin(y, baseline)),
                                   (int)ceil(x + 1), (int)ceil(fmax(y, baseline)), fill);
                prev_x = x;
                prev_y = y;
            }
        }
    }
    for (size_t i = 0; i < o->count; i++) {
        double x, y;
        cui_chart_series_point(o, series, i, left, right, lo, hi, top, bottom, &x, &y);
        cui_chart_rect(s, o, (int)lround(x) - 1, (int)lround(y) - 1, (int)lround(x) + 2,
                       (int)lround(y) + 2, color);
    }
}
static cui_color cui_chart_blend_color(cui_color a, cui_color b, double t) {
    t = fmax(0, fmin(1, t));
    return (cui_color){
        (unsigned char)lround(a.r + (b.r - a.r) * t), (unsigned char)lround(a.g + (b.g - a.g) * t),
        (unsigned char)lround(a.b + (b.b - a.b) * t), (unsigned char)lround(a.a + (b.a - a.a) * t)};
}
static void cui_chart_draw_bar(cui_chart_state *s, const cui_chart_options *o, size_t index,
                               cui_plot_rect bounds, cui_color solid) {
    bool horizontal =
        o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT;
    bool positive = o->data[index].value >= 0;
    bool forward = (o->bar_alignment == CUI_CHART_BAR_BOTTOM ||
                    o->bar_alignment == CUI_CHART_BAR_RIGHT) == positive;
    double start = horizontal ? (forward ? bounds.x_min : bounds.x_max)
                              : (forward ? bounds.y_max : bounds.y_min);
    double end = horizontal ? (forward ? bounds.x_max : bounds.x_min)
                            : (forward ? bounds.y_min : bounds.y_max);
    double progress = o->reveal_enabled && !o->reduced_motion ? o->reveal_progress : 1;
    double reveal_end = start + (end - start) * progress;
    if (horizontal) {
        bounds.x_min = fmin(start, reveal_end);
        bounds.x_max = fmax(start, reveal_end);
    } else {
        bounds.y_min = fmin(start, reveal_end);
        bounds.y_max = fmax(start, reveal_end);
    }
    double max_radius = fmin((bounds.x_max - bounds.x_min) / 2, (bounds.y_max - bounds.y_min) / 2);
    int x0 = (int)fmax(0, floor(bounds.x_min)), x1 = (int)fmin(o->width, ceil(bounds.x_max));
    int y0 = (int)fmax(0, floor(bounds.y_min)), y1 = (int)fmin(o->height, ceil(bounds.y_max));
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            double px = x + .5, py = y + .5;
            if (px < bounds.x_min || px >= bounds.x_max || py < bounds.y_min || py >= bounds.y_max)
                continue;
            size_t corner = py < (bounds.y_min + bounds.y_max) / 2
                                ? (px < (bounds.x_min + bounds.x_max) / 2 ? 0 : 1)
                                : (px < (bounds.x_min + bounds.x_max) / 2 ? 3 : 2);
            double radius = fmin(max_radius, o->custom_bar_corners ? o->bar_corner_radii[corner]
                                                                   : o->bar_corner_radius);
            if (radius > 0) {
                double cx = fmax(bounds.x_min + radius, fmin(bounds.x_max - radius, px));
                double cy = fmax(bounds.y_min + radius, fmin(bounds.y_max - radius, py));
                if ((px - cx) * (px - cx) + (py - cy) * (py - cy) > radius * radius)
                    continue;
            }
            cui_color color = solid;
            if (o->bar_gradient_start.a && o->bar_gradient_end.a) {
                double coordinate = horizontal ? px : py;
                double t = end != start ? (coordinate - start) / (end - start) : 0;
                color = cui_chart_blend_color(o->bar_gradient_start, o->bar_gradient_end, t);
            }
            if (o->bar_fill_pixel)
                color = o->bar_fill_pixel(&o->data[index], index, bounds,
                                          (cui_plot_rect){0, 0, o->width, o->height}, (float)px,
                                          (float)py, o->bar_fill_user);
            cui_chart_pixel(s, o, x, y, color);
        }
}
static void cui_chart_cartesian(cui_chart_state *s, const cui_chart_options *o) {
    double left = cui_chart_plot_left(o), right = cui_chart_plot_right(o, left), top = 12,
           bottom = cui_chart_plot_bottom(o);
    if (o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA) {
        top = o->custom_y_padding ? o->y_padding_top : 10;
        bottom = o->height - 20 - (o->custom_y_padding ? o->y_padding_bottom : 0);
    }
    if (right <= left || bottom <= top)
        return;
    double lo, hi;
    cui_chart_domain(o, &lo, &hi);
    bool horizontal = o->kind == CUI_CHART_BAR && (o->bar_alignment == CUI_CHART_BAR_LEFT ||
                                                   o->bar_alignment == CUI_CHART_BAR_RIGHT);
    cui_color grid = o->grid.a ? o->grid : (cui_color){120, 120, 120, 80};
    if (!o->hide_grid) {
        unsigned n = o->tick_count ? o->tick_count : 5;
        for (unsigned i = 0; i + 1 < n; i++) {
            if (horizontal) {
                double x = left + (right - left) * i / (n - 1);
                cui_chart_guide(s, o, x, top, x, bottom, grid, o->grid_solid);
            } else {
                double y = top + (bottom - top) * i / (n - 1);
                cui_chart_guide(s, o, left, y, right, y, grid, o->grid_solid);
            }
        }
        for (unsigned i = 0; i < o->grid_columns; i++) {
            double x = left + (right - left) * i / o->grid_columns;
            cui_chart_guide(s, o, x, top, x, bottom, grid, o->grid_solid);
        }
    }
    if (!o->hide_axes) {
        cui_chart_line(s, o, left, top, left, bottom, grid);
        if (!o->hide_x_axis)
            cui_chart_line(s, o, left, bottom, right, bottom, grid);
    }
    for (size_t i = 0; i < o->reference_count; i++) {
        double value = o->reference_values[i];
        if (value >= lo && value <= hi) {
            double y = cui_chart_map_y(value, lo, hi, top, bottom);
            cui_chart_guide(s, o, left, y, right, y, o->foreground.a ? o->foreground : grid, false);
        }
    }
    double baseline = cui_chart_map_y(0, lo, hi, top, bottom);
    if (baseline < top)
        baseline = top;
    if (baseline > bottom)
        baseline = bottom;
    if (o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA) {
        s->mark_clip_active = o->reveal_enabled && !o->reduced_motion && o->reveal_progress < 1;
        s->mark_clip_left = left;
        s->mark_clip_x = left + (right - left) * o->reveal_progress;
        for (size_t series = 0; series <= o->extra_series_count; series++)
            cui_chart_draw_series(s, o, series, left, right, top, bottom, lo, hi, baseline);
        s->mark_clip_active = false;
        return;
    }
    s->mark_clip_active = o->kind == CUI_CHART_CANDLESTICK && o->reveal_enabled &&
                          !o->reduced_motion && o->reveal_progress < 1;
    s->mark_clip_left = 0;
    s->mark_clip_x = o->width * o->reveal_progress;
    for (size_t i = 0; i < o->count; i++) {
        const cui_chart_datum *d = &o->data[i];
        cui_color c = cui_chart_palette(o, i);
        double y = cui_chart_map_y(d->value, lo, hi, top, bottom);
        if (y < top)
            y = top;
        if (y > bottom)
            y = bottom;
        if (o->kind == CUI_CHART_BAR) {
            cui_plot_rect bounds;
            if (cui_chart_bar_bounds_domain(o, i, lo, hi, &bounds) == CUI_PLOT_OK)
                cui_chart_draw_bar(s, o, i, bounds, c);
        } else if (o->kind == CUI_CHART_CANDLESTICK) {
            cui_chart_candle_geometry candle;
            if (cui_chart_candle_bounds(o, i, &candle) != CUI_PLOT_OK)
                continue;
            c = candle.bullish ? (o->bullish.a ? o->bullish : c) : (o->bearish.a ? o->bearish : c);
            cui_chart_line(s, o, candle.wick.x_min, candle.wick.y_min, candle.wick.x_max,
                           candle.wick.y_max, c);
            cui_chart_rect(s, o, (int)floor(candle.body.x_min), (int)floor(candle.body.y_min),
                           (int)ceil(candle.body.x_max), (int)ceil(candle.body.y_max + 1), c);
        }
    }
    s->mark_clip_active = false;
}
cui_plot_result cui_chart_pie_labels(const cui_chart_options *o, cui_chart_pie_label *out,
                                     size_t capacity) {
    if (!o || o->kind != CUI_CHART_PIE || !o->data || !out || !o->count || o->count > 256 ||
        !o->width || !o->height || o->width > 4096 || o->height > 4096 ||
        !isfinite(o->pie_outer_radius) || o->pie_outer_radius < 0 || !isfinite(o->pie_label_gap) ||
        o->pie_label_gap < 0)
        return CUI_PLOT_INVALID;
    if (capacity < o->count)
        return CUI_PLOT_CAPACITY;
    long double total = 0;
    for (size_t i = 0; i < o->count; i++) {
        if (!isfinite(o->data[i].value) || o->data[i].value < 0 ||
            !cui_chart_valid_str(o->data[i].label))
            return CUI_PLOT_INVALID;
        double inner, outer;
        if (!cui_chart_pie_radii(o, i, &inner, &outer))
            return CUI_PLOT_INVALID;
        total += o->data[i].value;
    }
    if (total <= 0 || total > DBL_MAX)
        return CUI_PLOT_EMPTY;
    cui_chart_pie_label labels[256];
    memset(labels, 0, sizeof labels);
    double radius = o->pie_outer_radius > 0 ? o->pie_outer_radius : o->height * .4;
    double gap = o->pie_label_gap > 0 ? o->pie_label_gap : 15;
    double cx = o->width / 2.0, cy = o->height / 2.0, cumulative = 0;
    for (size_t i = 0; i < o->count; i++) {
        double sweep = 2 * CUI_CHART_PI * (double)(o->data[i].value / total);
        double midpoint = cumulative + sweep / 2;
        cumulative += sweep;
        if (sweep < CUI_CHART_PI / 360 || !o->data[i].label.length)
            continue;
        cui_chart_pie_label *label = &labels[i];
        label->text = o->data[i].label;
        label->visible = true;
        label->right = sin(midpoint) >= 0;
        double slice_inner, slice_outer;
        (void)cui_chart_pie_radii(o, i, &slice_inner, &slice_outer);
        label->edge_x = (float)(cx + sin(midpoint) * slice_outer);
        label->edge_y = (float)(cy - cos(midpoint) * slice_outer);
        label->anchor_x = (float)(cx + sin(midpoint) * (radius + gap));
        label->anchor_y = (float)(cy - cos(midpoint) * (radius + gap));
    }
    for (int side = 0; side < 2; side++) {
        size_t indices[256], count = 0;
        for (size_t i = 0; i < o->count; i++)
            if (labels[i].visible && labels[i].right == (side == 1))
                indices[count++] = i;
        for (size_t i = 1; i < count; i++) {
            size_t item = indices[i], j = i;
            while (j && labels[indices[j - 1]].anchor_y > labels[item].anchor_y) {
                indices[j] = indices[j - 1];
                j--;
            }
            indices[j] = item;
        }
        double spacing = fmin(12, o->height / (double)(count + 1));
        double previous = -DBL_MAX;
        for (size_t i = 0; i < count; i++) {
            cui_chart_pie_label *label = &labels[indices[i]];
            double y = fmax(6, fmin(o->height - 6, label->anchor_y));
            if (i && y < previous + spacing)
                y = previous + spacing;
            label->anchor_y = (float)y;
            previous = y;
        }
        double next = o->height - 6;
        for (size_t i = count; i > 0; i--) {
            cui_chart_pie_label *label = &labels[indices[i - 1]];
            if (label->anchor_y > next)
                label->anchor_y = (float)next;
            next = label->anchor_y - spacing;
            label->text_x = label->anchor_x + (label->right ? 4 : -68);
            label->text_y = label->anchor_y - 6;
        }
    }
    memcpy(out, labels, o->count * sizeof(*out));
    return CUI_PLOT_OK;
}
static void cui_chart_pie(cui_chart_state *s, const cui_chart_options *o) {
    long double sum = 0;
    for (size_t i = 0; i < o->count; i++)
        sum += o->data[i].value;
    if (sum <= 0)
        return;
    double cx = o->width / 2.0, cy = o->height / 2.0;
    double r = o->pie_outer_radius > 0 ? o->pie_outer_radius : o->height * .4;
    if (r <= 0)
        return;
    double edges[257] = {0}, inners[256], outers[256];
    for (size_t i = 0; i < o->count; i++) {
        (void)cui_chart_pie_radii(o, i, &inners[i], &outers[i]);
        r = fmax(r, outers[i]);
    }
    double progress = o->reveal_enabled && !o->reduced_motion ? o->reveal_progress : 1;
    for (size_t i = 0; i < o->count; i++)
        edges[i + 1] = edges[i] + 2 * CUI_CHART_PI * progress * (double)(o->data[i].value / sum);
    edges[o->count] = 2 * CUI_CHART_PI * progress;
    int y0 = (int)fmax(0, floor(cy - r)), y1 = (int)fmin(o->height - 1, ceil(cy + r));
    int x0 = (int)fmax(0, floor(cx - r)), x1 = (int)fmin(o->width - 1, ceil(cx + r));
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            double dx = x + .5 - cx, dy = y + .5 - cy, rr = dx * dx + dy * dy;
            if (rr > r * r)
                continue;
            double angle = atan2(dy, dx) + CUI_CHART_PI / 2;
            if (angle < 0)
                angle += 2 * CUI_CHART_PI;
            size_t low = 0, high = o->count;
            while (low < high) {
                size_t mid = low + (high - low) / 2;
                if (angle >= edges[mid + 1])
                    low = mid + 1;
                else
                    high = mid;
            }
            if (low == o->count || angle < edges[low] + o->pad_angle / 2 ||
                angle >= edges[low + 1] - o->pad_angle / 2)
                continue;
            if (rr < inners[low] * inners[low] || rr > outers[low] * outers[low])
                continue;
            cui_chart_pixel(s, o, x, y, cui_chart_palette(o, low));
        }
    if (o->pie_show_labels) {
        cui_chart_pie_label labels[256];
        if (cui_chart_pie_labels(o, labels, 256) == CUI_PLOT_OK) {
            double opacity = fmax(0, fmin(1, (progress - .7) / .3));
            cui_color line = o->pie_label_line_color.a
                                 ? o->pie_label_line_color
                                 : (o->grid.a ? o->grid : (cui_color){120, 120, 120, 255});
            for (size_t i = 0; i < o->count; i++)
                if (labels[i].visible) {
                    cui_color slice_line = o->pie_label_line_color_fn
                                               ? o->pie_label_line_color_fn(
                                                     &o->data[i], i, o->pie_label_line_color_user)
                                               : line;
                    slice_line.a = (unsigned char)lround(slice_line.a * opacity);
                    cui_chart_line(s, o, labels[i].edge_x, labels[i].edge_y, labels[i].anchor_x,
                                   labels[i].anchor_y, slice_line);
                    cui_chart_line(s, o, labels[i].anchor_x, labels[i].anchor_y,
                                   labels[i].anchor_x + (labels[i].right ? 4 : -4),
                                   labels[i].anchor_y, slice_line);
                }
        }
    }
}
static void cui_chart_triangle(cui_chart_state *s, const cui_chart_options *o, double ax, double ay,
                               double bx, double by, double cx, double cy, cui_color color) {
    double left = fmax(0, floor(fmin(ax, fmin(bx, cx))));
    double right = fmin(o->width, ceil(fmax(ax, fmax(bx, cx))));
    double top = fmax(0, floor(fmin(ay, fmin(by, cy))));
    double bottom = fmin(o->height, ceil(fmax(ay, fmax(by, cy))));
    double area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    if (fabs(area) < 1e-9)
        return;
    for (int y = (int)top; y < (int)bottom; y++)
        for (int x = (int)left; x < (int)right; x++) {
            double px = x + .5, py = y + .5;
            double a = (bx - ax) * (py - ay) - (by - ay) * (px - ax);
            double b = (cx - bx) * (py - by) - (cy - by) * (px - bx);
            double c = (ax - cx) * (py - cy) - (ay - cy) * (px - cx);
            if ((area > 0 && a >= 0 && b >= 0 && c >= 0) ||
                (area < 0 && a <= 0 && b <= 0 && c <= 0))
                cui_chart_pixel(s, o, x, y, color);
        }
}
static double cui_chart_radar_radius(const cui_chart_options *o) {
    return o->radar_outer_radius > 0 ? o->radar_outer_radius : o->height * .4;
}
static void cui_chart_radar(cui_chart_state *s, const cui_chart_options *o) {
    if (o->count < 3)
        return;
    double cx = o->width / 2.0, cy = o->height / 2.0, radius = cui_chart_radar_radius(o),
           max = o->radar_max_value;
    if (max == 0)
        for (size_t series = 0; series <= o->extra_series_count; series++)
            for (size_t i = 0; i < o->count; i++) {
                double value = cui_chart_series_value(o, series, i);
                if (value > max)
                    max = value;
            }
    if (max <= 0 || radius <= 0)
        return;
    cui_color grid = o->grid.a ? o->grid : (cui_color){120, 120, 120, 80};
    unsigned levels = o->tick_count ? o->tick_count : 5;
    if (!o->hide_grid) {
        for (unsigned level = 1; level <= levels; level++) {
            double r = radius * level / levels;
            for (size_t i = 0; i < o->count; i++) {
                double a = -CUI_CHART_PI / 2 + 2 * CUI_CHART_PI * i / o->count;
                double b = -CUI_CHART_PI / 2 + 2 * CUI_CHART_PI * ((i + 1) % o->count) / o->count;
                cui_chart_line(s, o, cx + r * cos(a), cy + r * sin(a), cx + r * cos(b),
                               cy + r * sin(b), grid);
            }
        }
        for (size_t i = 0; i < o->count; i++) {
            double a = -CUI_CHART_PI / 2 + 2 * CUI_CHART_PI * i / o->count;
            cui_chart_line(s, o, cx, cy, cx + radius * cos(a), cy + radius * sin(a), grid);
        }
    }
    if (o->reveal_enabled && !o->reduced_motion && o->reveal_progress <= 0)
        return;
    for (size_t series = 0; series <= o->extra_series_count; series++) {
        cui_color color = series ? o->extra_series[series - 1].color : o->series[0];
        if (!color.a)
            color = o->series[series].a ? o->series[series] : cui_chart_palette(o, 0);
        cui_color fill = color;
        fill.a = (unsigned char)(color.a * 3 / 10);
        cui_color explicit_fill = series ? o->extra_series[series - 1].fill : o->primary_fill;
        if (explicit_fill.a)
            fill = explicit_fill;
        double xs[256], ys[256];
        for (size_t i = 0; i < o->count; i++) {
            double a = -CUI_CHART_PI / 2 + 2 * CUI_CHART_PI * i / o->count;
            double ratio = fmax(0, cui_chart_series_value(o, series, i)) / max;
            double progress = o->reveal_enabled && !o->reduced_motion ? o->reveal_progress : 1;
            double r = fmin(8192, radius * ratio * progress);
            xs[i] = cx + r * cos(a);
            ys[i] = cy + r * sin(a);
        }
        for (size_t i = 0; i < o->count; i++) {
            size_t j = (i + 1) % o->count;
            cui_chart_triangle(s, o, cx, cy, xs[i], ys[i], xs[j], ys[j], fill);
        }
        for (size_t i = 0; i < o->count; i++) {
            size_t j = (i + 1) % o->count;
            cui_chart_line(s, o, xs[i], ys[i], xs[j], ys[j], color);
            if (o->show_dots)
                cui_chart_rect(s, o, (int)lround(xs[i]) - 3, (int)lround(ys[i]) - 3,
                               (int)lround(xs[i]) + 4, (int)lround(ys[i]) + 4, color);
        }
    }
}
static void cui_chart_sankey_relax(const cui_chart_options *o, cui_chart_sankey_node *nodes,
                                   size_t layers, double padding);
cui_plot_result cui_chart_sankey_layout(const cui_chart_options *o, cui_chart_sankey_node *out,
                                        size_t capacity) {
    if (!o || o->kind != CUI_CHART_SANKEY || !o->data || !o->count || o->count > 256 ||
        o->link_count > 1024 || (o->link_count && !o->links) || !out ||
        o->sankey_align < CUI_CHART_SANKEY_JUSTIFY || o->sankey_align > CUI_CHART_SANKEY_CENTER ||
        !o->width || !o->height || o->width > 4096 || o->height > 4096 ||
        !isfinite(o->node_width) || !isfinite(o->node_padding) || o->node_width < 0 ||
        o->node_padding < 0 || o->sankey_iterations > 100 || !isfinite(o->sankey_min_link_width) ||
        o->sankey_value_scale < CUI_CHART_SANKEY_LINEAR ||
        o->sankey_value_scale > CUI_CHART_SANKEY_SQRT || o->sankey_min_link_width < 0)
        return CUI_PLOT_INVALID;
    if (!cui_chart_sankey_margins_valid(o))
        return CUI_PLOT_INVALID;
    if (capacity < o->count)
        return CUI_PLOT_CAPACITY;
    cui_chart_sankey_node nodes[256] = {{0}};
    size_t indegree[256] = {0}, order[256] = {0}, order_count = 0;
    long double incoming[256] = {0}, outgoing[256] = {0};
    long double scaled_in[256] = {0}, scaled_out[256] = {0};
    for (size_t i = 0; i < o->link_count; i++) {
        const cui_chart_link *link = &o->links[i];
        if (link->source >= o->count || link->target >= o->count || link->source == link->target ||
            !isfinite(link->value) || link->value < 0)
            return CUI_PLOT_INVALID;
        indegree[link->target]++;
        outgoing[link->source] += link->value;
        incoming[link->target] += link->value;
        double scaled = cui_chart_sankey_scaled(o, link->value);
        scaled_out[link->source] += scaled;
        scaled_in[link->target] += scaled;
    }
    for (size_t i = 0; i < o->count; i++) {
        if (!cui_chart_valid_str(o->data[i].label) || !isfinite(o->data[i].value) ||
            incoming[i] > DBL_MAX || outgoing[i] > DBL_MAX)
            return CUI_PLOT_OVERFLOW;
        nodes[i].throughput = (double)fmaxl(incoming[i], outgoing[i]);
        if (!indegree[i])
            order[order_count++] = i;
    }
    for (size_t pos = 0; pos < order_count; pos++) {
        size_t source = order[pos];
        for (size_t i = 0; i < o->link_count; i++)
            if (o->links[i].source == source) {
                size_t target = o->links[i].target;
                if (nodes[target].depth < nodes[source].depth + 1)
                    nodes[target].depth = nodes[source].depth + 1;
                if (!--indegree[target])
                    order[order_count++] = target;
            }
    }
    if (order_count != o->count)
        return CUI_PLOT_INVALID; /* cyclic */
    size_t layers = 1;
    for (size_t i = 0; i < o->count; i++)
        if (layers < nodes[i].depth + 1)
            layers = nodes[i].depth + 1;
    for (size_t pos = order_count; pos > 0; pos--) {
        size_t source = order[pos - 1];
        for (size_t i = 0; i < o->link_count; i++)
            if (o->links[i].source == source) {
                size_t target = o->links[i].target;
                if (nodes[source].height < nodes[target].height + 1)
                    nodes[source].height = nodes[target].height + 1;
            }
    }
    size_t counts[256] = {0};
    long double sums[256] = {0};
    for (size_t i = 0; i < o->count; i++) {
        size_t layer = nodes[i].depth;
        if (o->sankey_align == CUI_CHART_SANKEY_RIGHT)
            layer = layers - 1 - nodes[i].height;
        else if (o->sankey_align == CUI_CHART_SANKEY_JUSTIFY && !outgoing[i])
            layer = layers - 1;
        else if (o->sankey_align == CUI_CHART_SANKEY_CENTER && !incoming[i] && outgoing[i]) {
            size_t nearest = layers;
            for (size_t j = 0; j < o->link_count; j++)
                if (o->links[j].source == i && nodes[o->links[j].target].depth < nearest)
                    nearest = nodes[o->links[j].target].depth;
            layer = nearest ? nearest - 1 : 0;
        }
        nodes[i].layer = layer;
        counts[layer]++;
        sums[layer] += fmaxl(scaled_in[i], scaled_out[i]);
    }
    double width = o->node_width ? o->node_width : 10;
    double padding = o->node_padding ? o->node_padding : 16;
    double plot_left = 8 + o->sankey_margins.left;
    double plot_right = o->width - 8 - o->sankey_margins.right;
    double plot_top = 8 + o->sankey_margins.top;
    double plot_bottom = o->height - 8 - o->sankey_margins.bottom;
    double plot_height = plot_bottom - plot_top;
    if (width >= plot_right - plot_left || plot_height <= 0)
        return CUI_PLOT_INVALID;
    size_t max_count = 1;
    for (size_t l = 0; l < layers; l++)
        if (counts[l] > max_count)
            max_count = counts[l];
    padding = fmin(padding, plot_height / (max_count > 1 ? max_count - 1 : 1));
    long double ky = LDBL_MAX;
    for (size_t l = 0; l < layers; l++)
        if (sums[l] > 0) {
            long double available = plot_height - padding * (counts[l] - 1);
            long double candidate = available / sums[l];
            if (candidate < ky)
                ky = candidate;
        }
    if (ky == LDBL_MAX)
        ky = 0;
    if (!isfinite(ky) || ky < 0)
        return CUI_PLOT_OVERFLOW;
    for (size_t l = 0; l < layers; l++) {
        long double total = sums[l] * ky + padding * (counts[l] ? counts[l] - 1 : 0);
        long double y = plot_top + (plot_height - total) / 2;
        if (!isfinite(y) || y < -4096 || y > 4096)
            return CUI_PLOT_OVERFLOW;
        for (size_t i = 0; i < o->count; i++)
            if (nodes[i].layer == l) {
                double x = plot_left +
                           (layers > 1 ? (plot_right - plot_left - width) * l / (layers - 1) : 0);
                nodes[i].x0 = (float)x;
                nodes[i].x1 = (float)(x + width);
                nodes[i].y0 = (float)y;
                y += fmaxl(scaled_in[i], scaled_out[i]) * ky;
                if (!isfinite(y) || y < -4096 || y > 8192)
                    return CUI_PLOT_OVERFLOW;
                nodes[i].y1 = (float)y;
                y += padding;
            }
    }
    cui_chart_sankey_relax(o, nodes, layers, padding);
    memcpy(out, nodes, o->count * sizeof(*out));
    return CUI_PLOT_OK;
}
static void cui_chart_sankey_resolve(cui_chart_sankey_node *nodes, size_t count, size_t layer,
                                     double padding, double top, double bottom) {
    size_t indices[256], n = 0;
    for (size_t i = 0; i < count; i++)
        if (nodes[i].layer == layer)
            indices[n++] = i;
    for (size_t i = 1; i < n; i++) {
        size_t item = indices[i], j = i;
        while (j && nodes[indices[j - 1]].y0 > nodes[item].y0) {
            indices[j] = indices[j - 1];
            j--;
        }
        indices[j] = item;
    }
    double cursor = top;
    for (size_t i = 0; i < n; i++) {
        cui_chart_sankey_node *node = &nodes[indices[i]];
        double height = node->y1 - node->y0;
        double y = fmax(cursor, node->y0);
        node->y0 = (float)y;
        node->y1 = (float)(y + height);
        cursor = node->y1 + padding;
    }
    cursor = bottom;
    for (size_t i = n; i > 0; i--) {
        cui_chart_sankey_node *node = &nodes[indices[i - 1]];
        double height = node->y1 - node->y0;
        double y = fmin(cursor - height, node->y0);
        node->y0 = (float)y;
        node->y1 = (float)(y + height);
        cursor = y - padding;
    }
}
static void cui_chart_sankey_relax(const cui_chart_options *o, cui_chart_sankey_node *nodes,
                                   size_t layers, double padding) {
    unsigned iterations = o->sankey_iterations ? o->sankey_iterations : 6;
    for (unsigned pass = 0; pass < iterations; pass++) {
        double alpha = pow(.99, pass);
        for (int direction = 0; direction < 2; direction++)
            for (size_t step = 0; step < layers; step++) {
                size_t layer = direction == 0 ? layers - 1 - step : step;
                for (size_t node_index = 0; node_index < o->count; node_index++) {
                    cui_chart_sankey_node *node = &nodes[node_index];
                    if (node->layer != layer)
                        continue;
                    long double total = 0, weighted = 0;
                    for (size_t link_index = 0; link_index < o->link_count; link_index++) {
                        const cui_chart_link *link = &o->links[link_index];
                        size_t other;
                        if (direction == 0 && link->source == node_index)
                            other = link->target;
                        else if (direction == 1 && link->target == node_index)
                            other = link->source;
                        else
                            continue;
                        size_t distance = nodes[other].layer > layer ? nodes[other].layer - layer
                                                                     : layer - nodes[other].layer;
                        long double weight = cui_chart_sankey_scaled(o, link->value) * distance;
                        weighted += weight * (nodes[other].y0 + nodes[other].y1) / 2;
                        total += weight;
                    }
                    if (total > 0) {
                        double center = (node->y0 + node->y1) / 2;
                        double delta = ((double)(weighted / total) - center) * alpha;
                        node->y0 += (float)delta;
                        node->y1 += (float)delta;
                    }
                }
                cui_chart_sankey_resolve(nodes, o->count, layer, padding, 8 + o->sankey_margins.top,
                                         o->height - 8 - o->sankey_margins.bottom);
            }
    }
}
cui_plot_result cui_chart_sankey_link_bounds(const cui_chart_options *o,
                                             const cui_chart_sankey_node *nodes,
                                             cui_chart_sankey_link_geometry *out, size_t capacity) {
    if (!o || o->kind != CUI_CHART_SANKEY || !o->data || !nodes || !out || !o->count ||
        o->count > 256 || o->link_count > 1024 || (o->link_count && !o->links))
        return CUI_PLOT_INVALID;
    if (capacity < o->link_count)
        return CUI_PLOT_CAPACITY;
    long double outgoing[256] = {0}, incoming[256] = {0};
    for (size_t i = 0; i < o->count; i++)
        if (!isfinite(nodes[i].y0) || !isfinite(nodes[i].y1) || nodes[i].y0 > nodes[i].y1)
            return CUI_PLOT_INVALID;
    for (size_t i = 0; i < o->link_count; i++) {
        const cui_chart_link *link = &o->links[i];
        if (link->source >= o->count || link->target >= o->count || !isfinite(link->value) ||
            link->value < 0)
            return CUI_PLOT_INVALID;
        double scaled = cui_chart_sankey_scaled(o, link->value);
        outgoing[link->source] += scaled;
        incoming[link->target] += scaled;
    }
    size_t source_order[1024], target_order[1024];
    cui_chart_sankey_link_geometry geometry[1024] = {{0}};
    for (size_t i = 0; i < o->link_count; i++) {
        size_t j = i;
        while (j &&
               (o->links[source_order[j - 1]].source > o->links[i].source ||
                (o->links[source_order[j - 1]].source == o->links[i].source &&
                 nodes[o->links[source_order[j - 1]].target].y0 > nodes[o->links[i].target].y0))) {
            source_order[j] = source_order[j - 1];
            j--;
        }
        source_order[j] = i;
        j = i;
        while (j &&
               (o->links[target_order[j - 1]].target > o->links[i].target ||
                (o->links[target_order[j - 1]].target == o->links[i].target &&
                 nodes[o->links[target_order[j - 1]].source].y0 > nodes[o->links[i].source].y0))) {
            target_order[j] = target_order[j - 1];
            j--;
        }
        target_order[j] = i;
    }
    double used[256] = {0};
    for (size_t order = 0; order < o->link_count; order++) {
        size_t i = source_order[order], source = o->links[i].source;
        double height = nodes[source].y1 - nodes[source].y0;
        double width =
            outgoing[source] > 0
                ? (double)(cui_chart_sankey_scaled(o, o->links[i].value) / outgoing[source]) *
                      height
                : 0;
        geometry[i].source_width = (float)width;
        geometry[i].source_y = (float)(nodes[source].y0 + used[source] + width / 2);
        used[source] += width;
    }
    memset(used, 0, sizeof used);
    for (size_t order = 0; order < o->link_count; order++) {
        size_t i = target_order[order], target = o->links[i].target;
        double height = nodes[target].y1 - nodes[target].y0;
        double width =
            incoming[target] > 0
                ? (double)(cui_chart_sankey_scaled(o, o->links[i].value) / incoming[target]) *
                      height
                : 0;
        geometry[i].target_width = (float)width;
        geometry[i].target_y = (float)(nodes[target].y0 + used[target] + width / 2);
        used[target] += width;
    }
    memcpy(out, geometry, o->link_count * sizeof(*out));
    return CUI_PLOT_OK;
}
static void cui_chart_sankey(cui_chart_state *s, const cui_chart_options *o,
                             const cui_chart_sankey_node *nodes,
                             const cui_chart_sankey_link_geometry *geometry) {
    for (size_t i = 0; i < o->link_count; i++) {
        const cui_chart_link *link = &o->links[i];
        size_t source = link->source, target = link->target;
        double start_y = geometry[i].source_y, end_y = geometry[i].target_y;
        double min_width = o->sankey_min_link_width > 0 ? o->sankey_min_link_width : 1;
        double start_w = fmax(geometry[i].source_width, min_width);
        double end_w = fmax(geometry[i].target_width, min_width);
        double x0 = nodes[source].x1, x1 = nodes[target].x0;
        if (x1 <= x0)
            continue;
        cui_color source_color = cui_chart_palette(o, source);
        cui_color target_color = cui_chart_palette(o, target);
        double opacity = o->custom_sankey_link_opacity ? o->sankey_link_opacity : .3;
        for (int x = (int)ceil(x0); x < (int)x1; x++) {
            double t = (x - x0) / (x1 - x0), smooth = t * t * (3 - 2 * t),
                   center = (1 - smooth) * start_y + smooth * end_y;
            double half = ((1 - t) * start_w + t * end_w) / 2;
            cui_color color =
                link->color.a ? link->color : cui_chart_blend_color(source_color, target_color, t);
            color.a = (unsigned char)lround(color.a * opacity);
            cui_chart_rect(s, o, x, (int)floor(center - half), x + 1, (int)ceil(center + half),
                           color);
        }
    }
    for (size_t i = 0; i < o->count; i++)
        cui_chart_rounded_rect(s, o,
                               (cui_plot_rect){nodes[i].x0, nodes[i].y0, nodes[i].x1,
                                               fmax(nodes[i].y1, nodes[i].y0 + 1)},
                               o->sankey_node_corner_radius, cui_chart_palette(o, i));
}
static uint64_t cui_chart_mix(uint64_t h, uint64_t v) {
    return (h ^ v) * UINT64_C(1099511628211);
}
static uint64_t cui_chart_bits(double v) {
    uint64_t u;
    memcpy(&u, &v, sizeof u);
    return u;
}
static uint64_t cui_chart_cache_key(const cui_chart_options *o, const cui_chart_state *s) {
    if (!o->data_revision)
        return 0;
    uint64_t h = UINT64_C(1469598103934665603);
    h = cui_chart_mix(h, o->data_revision);
    h = cui_chart_mix(h, (uintptr_t)s->pixels);
    h = cui_chart_mix(h, (uintptr_t)o->data);
    h = cui_chart_mix(h, (uintptr_t)o->links);
    h = cui_chart_mix(h, (uintptr_t)o->extra_series);
    h = cui_chart_mix(h, o->count);
    h = cui_chart_mix(h, o->link_count);
    h = cui_chart_mix(h, o->extra_series_count);
    h = cui_chart_mix(h, o->kind);
    h = cui_chart_mix(h, o->width);
    h = cui_chart_mix(h, o->height);
    h = cui_chart_mix(h, o->point_count);
    h = cui_chart_mix(h, o->tick_count);
    h = cui_chart_mix(h, o->grid_columns);
    h = cui_chart_mix(h, o->curve);
    h = cui_chart_mix(h, o->bar_alignment);
    h = cui_chart_mix(h, o->custom_bar_padding);
    h = cui_chart_mix(h, o->show_dots);
    h = cui_chart_mix(h, cui_chart_bits(o->value_axis_gutter));
    h = cui_chart_mix(h, o->custom_y_padding);
    h = cui_chart_mix(h, cui_chart_bits(o->y_padding_top));
    h = cui_chart_mix(h, cui_chart_bits(o->y_padding_bottom));
    h = cui_chart_mix(h, o->sankey_align);
    h = cui_chart_mix(h, o->sankey_value_scale);
    h = cui_chart_mix(h, o->sankey_iterations);
    h = cui_chart_mix(h, o->custom_sankey_link_opacity);
    h = cui_chart_mix(h, cui_chart_bits(o->sankey_link_opacity));
    h = cui_chart_mix(h, cui_chart_bits(o->sankey_node_corner_radius));
    h = cui_chart_mix(h, cui_chart_bits(o->sankey_min_link_width));
    h = cui_chart_mix(h, o->hide_grid);
    h = cui_chart_mix(h, o->hide_axes);
    h = cui_chart_mix(h, o->hide_x_axis);
    h = cui_chart_mix(h, o->show_y_axis);
    h = cui_chart_mix(h, o->y_axis_inside);
    h = cui_chart_mix(h, o->grid_solid);
    h = cui_chart_mix(h, o->has_domain);
    h = cui_chart_mix(h, cui_chart_bits(o->domain_min));
    h = cui_chart_mix(h, cui_chart_bits(o->domain_max));
    h = cui_chart_mix(h, cui_chart_bits(o->inner_radius));
    h = cui_chart_mix(h, cui_chart_bits(o->pie_outer_radius));
    const unsigned char *inner_callback = (const unsigned char *)&o->pie_inner_radius_fn;
    const unsigned char *outer_callback = (const unsigned char *)&o->pie_outer_radius_fn;
    for (size_t i = 0; i < sizeof(o->pie_inner_radius_fn); i++)
        h = cui_chart_mix(h, inner_callback[i]);
    for (size_t i = 0; i < sizeof(o->pie_outer_radius_fn); i++)
        h = cui_chart_mix(h, outer_callback[i]);
    h = cui_chart_mix(h, (uintptr_t)o->pie_radius_user);
    h = cui_chart_mix(h, o->pie_show_labels);
    h = cui_chart_mix(h, cui_chart_bits(o->pie_label_gap));
    const unsigned char *pie_label_colors = (const unsigned char *)&o->pie_label_color;
    for (size_t i = 0; i < 2 * sizeof(cui_color); i++)
        h = cui_chart_mix(h, pie_label_colors[i]);
    const unsigned char *pie_line_callback = (const unsigned char *)&o->pie_label_line_color_fn;
    for (size_t i = 0; i < sizeof(o->pie_label_line_color_fn); i++)
        h = cui_chart_mix(h, pie_line_callback[i]);
    h = cui_chart_mix(h, (uintptr_t)o->pie_label_line_color_user);
    h = cui_chart_mix(h, cui_chart_bits(o->pad_angle));
    h = cui_chart_mix(h, cui_chart_bits(o->bar_ratio));
    h = cui_chart_mix(h, cui_chart_bits(o->candle_body_ratio));
    h = cui_chart_mix(h, cui_chart_bits(o->bar_corner_radius));
    h = cui_chart_mix(h, o->custom_bar_corners);
    for (size_t i = 0; i < 4; i++)
        h = cui_chart_mix(h, cui_chart_bits(o->bar_corner_radii[i]));
    h = cui_chart_mix(h, o->reveal_enabled);
    h = cui_chart_mix(h, o->appear_key);
    h = cui_chart_mix(h, o->disable_appear);
    h = cui_chart_mix(h, cui_chart_bits(o->appear_duration));
    h = cui_chart_mix(h, o->reduced_motion);
    h = cui_chart_mix(h, cui_chart_bits(o->reveal_progress));
    const unsigned char *gradient = (const unsigned char *)&o->bar_gradient_start;
    for (size_t i = 0; i < 2 * sizeof(cui_color); i++)
        h = cui_chart_mix(h, gradient[i]);
    const unsigned char *fill_callback = (const unsigned char *)&o->bar_fill_pixel;
    for (size_t i = 0; i < sizeof(o->bar_fill_pixel); i++)
        h = cui_chart_mix(h, fill_callback[i]);
    h = cui_chart_mix(h, (uintptr_t)o->bar_fill_user);
    h = cui_chart_mix(h, cui_chart_bits(o->bar_padding_inner));
    h = cui_chart_mix(h, cui_chart_bits(o->bar_padding_outer));
    h = cui_chart_mix(h, cui_chart_bits(o->max_bar_width));
    h = cui_chart_mix(h, cui_chart_bits(o->min_bar_length));
    h = cui_chart_mix(h, cui_chart_bits(o->radar_max_value));
    h = cui_chart_mix(h, cui_chart_bits(o->radar_outer_radius));
    h = cui_chart_mix(h, cui_chart_bits(o->node_width));
    h = cui_chart_mix(h, cui_chart_bits(o->node_padding));
    h = cui_chart_mix(h, cui_chart_bits(o->sankey_margins.left));
    h = cui_chart_mix(h, cui_chart_bits(o->sankey_margins.right));
    h = cui_chart_mix(h, cui_chart_bits(o->sankey_margins.top));
    h = cui_chart_mix(h, cui_chart_bits(o->sankey_margins.bottom));
    h = cui_chart_mix(h, o->reference_count);
    for (size_t i = 0; i < o->reference_count; i++)
        h = cui_chart_mix(h, cui_chart_bits(o->reference_values[i]));
    const cui_color *colors = &o->background;
    for (size_t i = 0; i < sizeof(cui_color) * 13; i++)
        h = cui_chart_mix(h, ((const unsigned char *)colors)[i]);
    const unsigned char *primary_fill = (const unsigned char *)&o->primary_fill;
    for (size_t i = 0; i < sizeof(o->primary_fill); i++)
        h = cui_chart_mix(h, primary_fill[i]);
    return h ? h : 1;
}
cui_plot_result cui_chart_render(cui_chart_state *s, const cui_chart_options *o) {
    cui_plot_result status = cui_chart_validate(s, o);
    if (status != CUI_PLOT_OK)
        return status;
    cui_chart_sankey_node nodes[256];
    cui_chart_sankey_link_geometry link_geometry[1024];
    if (o->kind == CUI_CHART_SANKEY) {
        status = cui_chart_sankey_layout(o, nodes, 256);
        if (status != CUI_PLOT_OK)
            return status;
        status = cui_chart_sankey_link_bounds(o, nodes, link_geometry, 1024);
        if (status != CUI_PLOT_OK)
            return status;
    }
    uint64_t key = cui_chart_cache_key(o, s);
    if (key && key == s->cache_key)
        return CUI_PLOT_OK;
    for (size_t i = 0; i < (size_t)o->width * o->height; i++) {
        s->pixels[4 * i] = o->background.r;
        s->pixels[4 * i + 1] = o->background.g;
        s->pixels[4 * i + 2] = o->background.b;
        s->pixels[4 * i + 3] = o->background.a;
    }
    if (o->kind == CUI_CHART_PIE)
        cui_chart_pie(s, o);
    else if (o->kind <= CUI_CHART_BAR || o->kind == CUI_CHART_CANDLESTICK)
        cui_chart_cartesian(s, o);
    else if (o->kind == CUI_CHART_RADAR)
        cui_chart_radar(s, o);
    else if (o->kind == CUI_CHART_SANKEY) {
        s->mark_clip_active = o->reveal_enabled && !o->reduced_motion && o->reveal_progress < 1;
        s->mark_clip_left = 0;
        s->mark_clip_x = o->width * o->reveal_progress;
        cui_chart_sankey(s, o, nodes, link_geometry);
        s->mark_clip_active = false;
    }
    s->hovered = SIZE_MAX;
    s->hovered_value = 0;
    s->revision++;
    s->cache_key = key;
    s->rendered_reveal_progress = o->reveal_enabled && !o->reduced_motion ? o->reveal_progress : 1;
    return CUI_PLOT_OK;
}
static size_t cui_chart_band_nearest(const cui_chart_options *o, double position, double start,
                                     double extent) {
    size_t slots = o->point_count ? o->point_count : o->count;
    if (slots <= 1)
        return 0;
    double inner = o->custom_bar_padding ? o->bar_padding_inner : .4;
    double outer = o->custom_bar_padding ? o->bar_padding_outer : .2;
    double avg = extent / slots, outer_width = avg * outer;
    double display = (extent - 2 * outer_width) / slots;
    double step = display * (1 + inner / (slots - 1));
    if (step <= 0)
        return SIZE_MAX;
    double candidate = round((position - start - outer_width) / step);
    if (candidate < 0)
        candidate = 0;
    if (candidate >= slots)
        candidate = slots - 1;
    return (size_t)candidate;
}
cui_chart_hit cui_chart_hit_test(const cui_chart_options *o, float x, float y) {
    cui_chart_hit hit = {0};
    if (!o || !o->data || !o->count || o->count > CUI_CHART_MAX_ITEMS || o->point_count > 1000000 ||
        o->noninteractive || !isfinite(x) || !isfinite(y) || x < 0 || y < 0 ||
        x >= (o->logical_width > 0 ? o->logical_width : o->width) ||
        y >= (o->logical_height > 0 ? o->logical_height : o->height))
        return hit;
    if (o->logical_width > 0)
        x = (float)(x * o->width / o->logical_width);
    if (o->logical_height > 0)
        y = (float)(y * o->height / o->logical_height);
    size_t index = 0;
    if (o->kind == CUI_CHART_PIE) {
        double cx = o->width / 2.0, cy = o->height / 2.0;
        double dx = x - cx, dy = y - cy;
        double sum = 0;
        for (size_t i = 0; i < o->count; i++)
            sum += o->data[i].value;
        if (sum <= 0)
            return hit;
        double a = atan2(dy, dx) + CUI_CHART_PI / 2;
        if (a < 0)
            a += 2 * CUI_CHART_PI;
        double cumulative = 0;
        for (index = 0; index < o->count; index++) {
            double start = cumulative;
            cumulative += 2 * CUI_CHART_PI * o->data[index].value / sum;
            if (a < cumulative) {
                if (a < start + o->pad_angle / 2 || a >= cumulative - o->pad_angle / 2)
                    return hit;
                break;
            }
        }
        if (index == o->count)
            return hit;
        double slice_inner, slice_outer;
        if (!cui_chart_pie_radii(o, index, &slice_inner, &slice_outer) ||
            dx * dx + dy * dy < slice_inner * slice_inner ||
            dx * dx + dy * dy > slice_outer * slice_outer)
            return hit;
    } else if (o->kind == CUI_CHART_RADAR) {
        double dx = x - o->width / 2.0, dy = y - o->height / 2.0;
        double radius = fmin(o->width / 2.0, o->height / 2.0) - 12;
        if (dx * dx + dy * dy > radius * radius || radius <= 0)
            return hit;
        double angle = atan2(dy, dx) + CUI_CHART_PI / 2;
        if (angle < 0)
            angle += 2 * CUI_CHART_PI;
        index = (size_t)llround(angle * o->count / (2 * CUI_CHART_PI)) % o->count;
    } else if (o->kind == CUI_CHART_SANKEY) {
        cui_chart_sankey_node nodes[256];
        if (cui_chart_sankey_layout(o, nodes, 256) != CUI_PLOT_OK)
            return hit;
        for (index = 0; index < o->count; index++)
            if (x >= nodes[index].x0 && x <= nodes[index].x1 && y >= nodes[index].y0 &&
                y <= fmax(nodes[index].y1, nodes[index].y0 + 1))
                break;
        if (index == o->count)
            return hit;
        hit.valid = true;
        hit.hovered = index;
        hit.label = o->data[index].label;
        hit.value = nodes[index].throughput;
        return hit;
    } else {
        double left = cui_chart_plot_left(o), right = cui_chart_plot_right(o, left);
        double top = 12, bottom = cui_chart_plot_bottom(o);
        if (x < left || x > right || y < top || y > bottom || right <= left)
            return hit;
        size_t n = o->point_count ? o->point_count : o->count;
        if (o->kind == CUI_CHART_BAR &&
            (o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT)) {
            index = cui_chart_band_nearest(o, y, top, bottom - top);
            if (index >= o->count)
                return hit;
        } else if (o->kind == CUI_CHART_BAR || o->kind == CUI_CHART_CANDLESTICK) {
            index = cui_chart_band_nearest(o, x, left, right - left);
            if (index >= o->count)
                return hit;
        } else {
            double pos = (x - left) / (right - left) * (n > 1 ? n - 1 : 0);
            index = (size_t)llround(pos);
            if (index >= o->count)
                return hit;
        }
    }
    hit.valid = true;
    hit.hovered = index;
    hit.label = o->data[index].label;
    hit.value = o->kind == CUI_CHART_CANDLESTICK ? o->data[index].close : o->data[index].value;
    return hit;
}
