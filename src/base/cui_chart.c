/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include <float.h>
#include <math.h>
#include <string.h>
#define CUI_CHART_MAX_ITEMS 4096
#define CUI_CHART_PI 3.14159265358979323846
static cui_color palette(const cui_chart_options *o, size_t i) {
    cui_color c = o->data[i].color;
    if (c.a)
        return c;
    c = o->series[i % 8];
    return c.a ? c : (cui_color){64, 120, 220, 255};
}
static bool valid_str(cui_str s) {
    return !s.length || s.data;
}
static cui_plot_result validate(const cui_chart_state *s, const cui_chart_options *o) {
    if (!s || !o || !s->pixels || !o->data || !o->count || o->count > CUI_CHART_MAX_ITEMS ||
        o->kind < CUI_CHART_LINE || o->kind > CUI_CHART_SANKEY || !o->width || !o->height ||
        o->curve < CUI_CHART_CURVE_NATURAL || o->curve > CUI_CHART_CURVE_STEP_AFTER ||
        o->extra_series_count > 7 || (o->extra_series_count && !o->extra_series) ||
        (o->extra_series_count && o->kind != CUI_CHART_LINE && o->kind != CUI_CHART_AREA) ||
        o->width > 4096 || o->height > 4096 || o->width > SIZE_MAX / 4 / o->height ||
        ((o->kind == CUI_CHART_PIE || o->kind == CUI_CHART_RADAR || o->kind == CUI_CHART_SANKEY) &&
         o->count > 256) ||
        !isfinite(o->inner_radius) || o->inner_radius < 0 || !isfinite(o->pad_angle) ||
        o->pad_angle < 0 || o->pad_angle > 2 * CUI_CHART_PI || !isfinite(o->bar_ratio) ||
        o->bar_ratio < 0 || o->bar_ratio > 1 || o->tick_count == 1 || o->tick_count > 1000 ||
        o->bar_alignment < CUI_CHART_BAR_BOTTOM || o->bar_alignment > CUI_CHART_BAR_RIGHT ||
        o->x_tick_count > 1000 || o->grid_columns > 1000 || o->reference_count > 16 ||
        (o->reference_count && !o->reference_values) ||
        (o->point_count && o->point_count < o->count) || !isfinite(o->logical_width) ||
        !isfinite(o->logical_height) || o->logical_width < 0 || o->logical_height < 0 ||
        (o->has_domain &&
         (!isfinite(o->domain_min) || !isfinite(o->domain_max) || o->domain_min >= o->domain_max)))
        return CUI_PLOT_INVALID;
    if (s->byte_capacity < (size_t)o->width * o->height * 4)
        return CUI_PLOT_CAPACITY;
    for (size_t i = 0; i < o->count; i++) {
        const cui_chart_datum *d = &o->data[i];
        if (!valid_str(d->label) || !isfinite(d->value))
            return CUI_PLOT_INVALID;
        if (o->kind == CUI_CHART_CANDLESTICK &&
            (!isfinite(d->open) || !isfinite(d->high) || !isfinite(d->low) || !isfinite(d->close) ||
             d->low > d->high || d->open < d->low || d->open > d->high || d->close < d->low ||
             d->close > d->high))
            return CUI_PLOT_INVALID;
        if ((o->kind == CUI_CHART_PIE || o->kind == CUI_CHART_SANKEY) && d->value < 0)
            return CUI_PLOT_INVALID;
    }
    for (size_t series = 0; series < o->extra_series_count; series++) {
        const cui_chart_series *extra = &o->extra_series[series];
        if (!extra->values || extra->count != o->count || !valid_str(extra->name))
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
static void pixel(cui_chart_state *s, const cui_chart_options *o, int x, int y, cui_color c) {
    if (x < 0 || y < 0 || (unsigned)x >= o->width || (unsigned)y >= o->height || !c.a)
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
static void rect(cui_chart_state *s, const cui_chart_options *o, int x0, int y0, int x1, int y1,
                 cui_color c) {
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
            pixel(s, o, x, y, c);
}
static void line(cui_chart_state *s, const cui_chart_options *o, double ax, double ay, double bx,
                 double by, cui_color c) {
    double dx = bx - ax, dy = by - ay, steps = fmax(fabs(dx), fabs(dy));
    if (!isfinite(steps) || steps > 20000)
        return;
    if (steps < 1) {
        pixel(s, o, (int)lround(ax), (int)lround(ay), c);
        return;
    }
    for (int i = 0; i <= (int)ceil(steps); i++) {
        double t = i / ceil(steps);
        pixel(s, o, (int)lround(ax + dx * t), (int)lround(ay + dy * t), c);
    }
}
static void guide(cui_chart_state *s, const cui_chart_options *o, double ax, double ay, double bx,
                  double by, cui_color c, bool solid) {
    if (solid) {
        line(s, o, ax, ay, bx, by, c);
        return;
    }
    double distance = hypot(bx - ax, by - ay);
    if (!isfinite(distance) || distance <= 0)
        return;
    for (double start = 0; start < distance; start += 6) {
        double end = fmin(distance, start + 4);
        line(s, o, ax + (bx - ax) * start / distance, ay + (by - ay) * start / distance,
             ax + (bx - ax) * end / distance, ay + (by - ay) * end / distance, c);
    }
}
static double plot_left(const cui_chart_options *o) {
    return o->show_y_axis && !o->y_axis_inside ? 50 : 24;
}
static double x_at(const cui_chart_options *o, size_t i, double left, double right) {
    size_t n = o->point_count ? o->point_count : o->count;
    return n > 1 ? left + (right - left) * i / (n - 1) : (left + right) / 2;
}
static double map_y(double v, double lo, double hi, double top, double bottom) {
    cui_plot_scale scale = {CUI_PLOT_LINEAR, lo, hi, bottom, top, true};
    double mapped = bottom;
    (void)cui_plot_map(&scale, v, &mapped);
    return mapped;
}
static void domain(const cui_chart_options *o, double *lo, double *hi) {
    if (o->has_domain) {
        *lo = o->domain_min;
        *hi = o->domain_max;
        return;
    }
    *lo = 0;
    *hi = 0;
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
            *lo -= 1;
            *hi += 1;
        }
    }
}
bool cui_chart_value_domain(const cui_chart_options *o, double *minimum, double *maximum) {
    if (!o || !minimum || !maximum || !o->data || !o->count || o->count > CUI_CHART_MAX_ITEMS ||
        o->extra_series_count > 7 || (o->extra_series_count && !o->extra_series))
        return false;
    for (size_t i = 0; i < o->count; i++)
        if (!isfinite(o->data[i].value))
            return false;
    for (size_t series = 0; series < o->extra_series_count; series++) {
        const cui_chart_series *extra = &o->extra_series[series];
        if (!extra->values || extra->count != o->count)
            return false;
        for (size_t i = 0; i < o->count; i++)
            if (!isfinite(extra->values[i]))
                return false;
    }
    domain(o, minimum, maximum);
    return isfinite(*minimum) && isfinite(*maximum) && *minimum < *maximum;
}
static double series_value(const cui_chart_options *o, size_t series, size_t point) {
    return series ? o->extra_series[series - 1].values[point] : o->data[point].value;
}
static void series_point(const cui_chart_options *o, size_t series, size_t point, double left,
                         double right, double lo, double hi, double top, double bottom, double *x,
                         double *y) {
    *x = x_at(o, point, left, right);
    *y = map_y(series_value(o, series, point), lo, hi, top, bottom);
    if (*y < top)
        *y = top;
    if (*y > bottom)
        *y = bottom;
}
static void draw_series(cui_chart_state *s, const cui_chart_options *o, size_t series, double left,
                        double right, double top, double bottom, double lo, double hi,
                        double baseline) {
    cui_color color = series ? o->extra_series[series - 1].color : o->series[0];
    if (!color.a)
        color = o->series[series].a ? o->series[series] : palette(o, 0);
    cui_color fill = color;
    fill.a = (unsigned char)(fill.a / 3);
    double first_x, first_y;
    series_point(o, series, 0, left, right, lo, hi, top, bottom, &first_x, &first_y);
    if (o->kind == CUI_CHART_AREA)
        rect(s, o, (int)floor(first_x), (int)floor(fmin(first_y, baseline)), (int)ceil(first_x + 1),
             (int)ceil(fmax(first_y, baseline)), fill);
    for (size_t i = 1; i < o->count; i++) {
        double x0, y0, x1, y1;
        series_point(o, series, i - 1, left, right, lo, hi, top, bottom, &x0, &y0);
        series_point(o, series, i, left, right, lo, hi, top, bottom, &x1, &y1);
        if (o->curve == CUI_CHART_CURVE_STEP_AFTER) {
            line(s, o, x0, y0, x1, y0, color);
            if (i + 1 < o->count)
                line(s, o, x1, y0, x1, y1, color);
            if (o->kind == CUI_CHART_AREA)
                rect(s, o, (int)ceil(x0), (int)floor(fmin(y0, baseline)), (int)ceil(x1),
                     (int)ceil(fmax(y0, baseline)), fill);
        } else if (o->curve == CUI_CHART_CURVE_LINEAR) {
            line(s, o, x0, y0, x1, y1, color);
            if (o->kind == CUI_CHART_AREA)
                for (int x = (int)ceil(x0); x <= (int)floor(x1); x++) {
                    double y = y0 + (y1 - y0) * (x - x0) / (x1 - x0);
                    rect(s, o, x, (int)floor(fmin(y, baseline)), x + 1,
                         (int)ceil(fmax(y, baseline)), fill);
                }
        } else {
            double prev_y = y0, prev_x = x0, p0y = y0, p3y = y1;
            if (i > 1) {
                double unused;
                series_point(o, series, i - 2, left, right, lo, hi, top, bottom, &unused, &p0y);
            }
            if (i + 1 < o->count) {
                double unused;
                series_point(o, series, i + 1, left, right, lo, hi, top, bottom, &unused, &p3y);
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
                line(s, o, prev_x, prev_y, x, y, color);
                if (o->kind == CUI_CHART_AREA)
                    rect(s, o, (int)floor(x), (int)floor(fmin(y, baseline)), (int)ceil(x + 1),
                         (int)ceil(fmax(y, baseline)), fill);
                prev_x = x;
                prev_y = y;
            }
        }
    }
    for (size_t i = 0; i < o->count; i++) {
        double x, y;
        series_point(o, series, i, left, right, lo, hi, top, bottom, &x, &y);
        rect(s, o, (int)lround(x) - 1, (int)lround(y) - 1, (int)lround(x) + 2, (int)lround(y) + 2,
             color);
    }
}
static void cartesian(cui_chart_state *s, const cui_chart_options *o) {
    double left = plot_left(o), right = o->width > 36 ? o->width - 12 : left, top = 12,
           bottom = o->height > 32 ? o->height - 20 : 12;
    if (right <= left || bottom <= top)
        return;
    double lo, hi;
    domain(o, &lo, &hi);
    bool horizontal = o->kind == CUI_CHART_BAR && (o->bar_alignment == CUI_CHART_BAR_LEFT ||
                                                   o->bar_alignment == CUI_CHART_BAR_RIGHT);
    cui_color grid = o->grid.a ? o->grid : (cui_color){120, 120, 120, 80};
    if (!o->hide_grid) {
        unsigned n = o->tick_count ? o->tick_count : 5;
        for (unsigned i = 0; i + 1 < n; i++) {
            if (horizontal) {
                double x = left + (right - left) * i / (n - 1);
                guide(s, o, x, top, x, bottom, grid, o->grid_solid);
            } else {
                double y = top + (bottom - top) * i / (n - 1);
                guide(s, o, left, y, right, y, grid, o->grid_solid);
            }
        }
        for (unsigned i = 0; i < o->grid_columns; i++) {
            double x = left + (right - left) * i / o->grid_columns;
            guide(s, o, x, top, x, bottom, grid, o->grid_solid);
        }
    }
    if (!o->hide_axes) {
        line(s, o, left, top, left, bottom, grid);
        if (!o->hide_x_axis)
            line(s, o, left, bottom, right, bottom, grid);
    }
    for (size_t i = 0; i < o->reference_count; i++) {
        double value = o->reference_values[i];
        if (value >= lo && value <= hi) {
            double y = map_y(value, lo, hi, top, bottom);
            guide(s, o, left, y, right, y, o->foreground.a ? o->foreground : grid, false);
        }
    }
    double baseline = map_y(0, lo, hi, top, bottom);
    if (baseline < top)
        baseline = top;
    if (baseline > bottom)
        baseline = bottom;
    if (o->kind == CUI_CHART_LINE || o->kind == CUI_CHART_AREA) {
        for (size_t series = 0; series <= o->extra_series_count; series++)
            draw_series(s, o, series, left, right, top, bottom, lo, hi, baseline);
        return;
    }
    for (size_t i = 0; i < o->count; i++) {
        const cui_chart_datum *d = &o->data[i];
        cui_color c = palette(o, i);
        double x = x_at(o, i, left, right), y = map_y(d->value, lo, hi, top, bottom);
        if (y < top)
            y = top;
        if (y > bottom)
            y = bottom;
        if (o->kind == CUI_CHART_BAR) {
            size_t n = o->point_count ? o->point_count : o->count;
            if (horizontal) {
                double band = (bottom - top) / n, center = top + (i + .5) * band;
                double thick = band * (o->bar_ratio ? o->bar_ratio : .6);
                cui_plot_scale scale = {CUI_PLOT_LINEAR,
                                        lo,
                                        hi,
                                        o->bar_alignment == CUI_CHART_BAR_LEFT ? left : right,
                                        o->bar_alignment == CUI_CHART_BAR_LEFT ? right : left,
                                        true};
                double at = left, zero = left;
                (void)cui_plot_map(&scale, d->value, &at);
                (void)cui_plot_map(&scale, 0, &zero);
                rect(s, o, (int)floor(fmin(at, zero)), (int)floor(center - thick / 2),
                     (int)ceil(fmax(at, zero)), (int)ceil(center + thick / 2), c);
            } else {
                double band = (right - left) / n, w = band * (o->bar_ratio ? o->bar_ratio : .6);
                if (o->bar_alignment == CUI_CHART_BAR_TOP) {
                    y = top + (d->value - lo) * (bottom - top) / (hi - lo);
                    baseline = top + (0 - lo) * (bottom - top) / (hi - lo);
                }
                rect(s, o, (int)floor(x - w / 2), (int)floor(fmin(y, baseline)),
                     (int)ceil(x + w / 2), (int)ceil(fmax(y, baseline)), c);
            }
        } else if (o->kind == CUI_CHART_CANDLESTICK) {
            double yo = map_y(d->open, lo, hi, top, bottom),
                   yc = map_y(d->close, lo, hi, top, bottom);
            double yl = map_y(d->low, lo, hi, top, bottom),
                   yh = map_y(d->high, lo, hi, top, bottom);
            c = d->close > d->open ? (o->bullish.a ? o->bullish : c)
                                   : (o->bearish.a ? o->bearish : c);
            line(s, o, x, yh, x, yl, c);
            double band = (right - left) / (o->point_count ? o->point_count : o->count);
            double w = band * (o->bar_ratio ? o->bar_ratio : .6);
            rect(s, o, (int)floor(x - w / 2), (int)floor(fmin(yo, yc)), (int)ceil(x + w / 2),
                 (int)ceil(fmax(yo, yc) + 1), c);
        }
    }
}
static void pie(cui_chart_state *s, const cui_chart_options *o) {
    long double sum = 0;
    for (size_t i = 0; i < o->count; i++)
        sum += o->data[i].value;
    if (sum <= 0)
        return;
    double cx = o->width / 2.0, cy = o->height / 2.0, r = fmin(cx, cy) - 4;
    if (r <= 0)
        return;
    double inner = fmin(o->inner_radius, r), edges[257] = {0};
    for (size_t i = 0; i < o->count; i++)
        edges[i + 1] = edges[i] + 2 * CUI_CHART_PI * (double)(o->data[i].value / sum);
    edges[o->count] = 2 * CUI_CHART_PI;
    int y0 = (int)floor(cy - r), y1 = (int)ceil(cy + r), x0 = (int)floor(cx - r),
        x1 = (int)ceil(cx + r);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            double dx = x + .5 - cx, dy = y + .5 - cy, rr = dx * dx + dy * dy;
            if (rr < inner * inner || rr > r * r)
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
            pixel(s, o, x, y, palette(o, low));
        }
}
static void radar(cui_chart_state *s, const cui_chart_options *o) {
    if (o->count < 3)
        return;
    double cx = o->width / 2.0, cy = o->height / 2.0, radius = fmin(cx, cy) - 12, max = 0;
    for (size_t i = 0; i < o->count; i++)
        if (o->data[i].value > max)
            max = o->data[i].value;
    if (max <= 0 || radius <= 0)
        return;
    cui_color grid = o->grid.a ? o->grid : (cui_color){120, 120, 120, 80};
    unsigned levels = o->tick_count ? o->tick_count : 5;
    if (!o->hide_grid)
        for (unsigned level = 1; level <= levels; level++) {
            double r = radius * level / levels;
            for (size_t i = 0; i < o->count; i++) {
                double a = -CUI_CHART_PI / 2 + 2 * CUI_CHART_PI * i / o->count;
                double b = -CUI_CHART_PI / 2 + 2 * CUI_CHART_PI * ((i + 1) % o->count) / o->count;
                line(s, o, cx + r * cos(a), cy + r * sin(a), cx + r * cos(b), cy + r * sin(b),
                     grid);
            }
        }
    for (size_t i = 0; i < o->count; i++) {
        double a = -CUI_CHART_PI / 2 + 2 * CUI_CHART_PI * i / o->count;
        double b = -CUI_CHART_PI / 2 + 2 * CUI_CHART_PI * ((i + 1) % o->count) / o->count;
        double ra = radius * fmax(0, o->data[i].value) / max;
        double rb = radius * fmax(0, o->data[(i + 1) % o->count].value) / max;
        if (!o->hide_axes)
            line(s, o, cx, cy, cx + radius * cos(a), cy + radius * sin(a), grid);
        line(s, o, cx + ra * cos(a), cy + ra * sin(a), cx + rb * cos(b), cy + rb * sin(b),
             palette(o, i));
        rect(s, o, (int)lround(cx + ra * cos(a)) - 1, (int)lround(cy + ra * sin(a)) - 1,
             (int)lround(cx + ra * cos(a)) + 2, (int)lround(cy + ra * sin(a)) + 2, palette(o, i));
    }
}
cui_plot_result cui_chart_sankey_layout(const cui_chart_options *o, cui_chart_sankey_node *out,
                                        size_t capacity) {
    if (!o || o->kind != CUI_CHART_SANKEY || !o->data || !o->count || o->count > 256 ||
        o->link_count > 1024 || (o->link_count && !o->links) || !out ||
        o->sankey_align < CUI_CHART_SANKEY_JUSTIFY || o->sankey_align > CUI_CHART_SANKEY_CENTER ||
        !o->width || !o->height || o->width > 4096 || o->height > 4096 ||
        !isfinite(o->node_width) || !isfinite(o->node_padding) || o->node_width < 0 ||
        o->node_padding < 0)
        return CUI_PLOT_INVALID;
    if (capacity < o->count)
        return CUI_PLOT_CAPACITY;
    cui_chart_sankey_node nodes[256] = {{0}};
    size_t indegree[256] = {0}, order[256] = {0}, order_count = 0;
    long double incoming[256] = {0}, outgoing[256] = {0};
    for (size_t i = 0; i < o->link_count; i++) {
        const cui_chart_link *link = &o->links[i];
        if (link->source >= o->count || link->target >= o->count || link->source == link->target ||
            !isfinite(link->value) || link->value < 0)
            return CUI_PLOT_INVALID;
        indegree[link->target]++;
        outgoing[link->source] += link->value;
        incoming[link->target] += link->value;
    }
    for (size_t i = 0; i < o->count; i++) {
        if (!valid_str(o->data[i].label) || !isfinite(o->data[i].value) || incoming[i] > DBL_MAX ||
            outgoing[i] > DBL_MAX)
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
        sums[layer] += nodes[i].throughput;
    }
    double width = o->node_width ? o->node_width : 10;
    double padding = o->node_padding ? o->node_padding : 16;
    if (width >= o->width || o->height <= 16)
        return CUI_PLOT_INVALID;
    size_t max_count = 1;
    for (size_t l = 0; l < layers; l++)
        if (counts[l] > max_count)
            max_count = counts[l];
    padding = fmin(padding, (o->height - 16.0) / (max_count > 1 ? max_count - 1 : 1));
    double ky = DBL_MAX;
    for (size_t l = 0; l < layers; l++)
        if (sums[l] > 0) {
            double available = o->height - 16.0 - padding * (counts[l] - 1);
            double candidate = (double)(available / sums[l]);
            if (candidate < ky)
                ky = candidate;
        }
    if (ky == DBL_MAX)
        ky = 0;
    if (!isfinite(ky) || ky < 0)
        return CUI_PLOT_OVERFLOW;
    for (size_t l = 0; l < layers; l++) {
        double total = (double)sums[l] * ky + padding * (counts[l] ? counts[l] - 1 : 0);
        double y = (o->height - total) / 2;
        for (size_t i = 0; i < o->count; i++)
            if (nodes[i].layer == l) {
                double x = 8 + (layers > 1 ? (o->width - 16.0 - width) * l / (layers - 1) : 0);
                nodes[i].x0 = (float)x;
                nodes[i].x1 = (float)(x + width);
                nodes[i].y0 = (float)y;
                y += nodes[i].throughput * ky;
                nodes[i].y1 = (float)y;
                y += padding;
            }
    }
    memcpy(out, nodes, o->count * sizeof(*out));
    return CUI_PLOT_OK;
}
static void sankey(cui_chart_state *s, const cui_chart_options *o,
                   const cui_chart_sankey_node *nodes) {
    double source_used[256] = {0}, target_used[256] = {0};
    for (size_t i = 0; i < o->link_count; i++) {
        const cui_chart_link *link = &o->links[i];
        size_t source = link->source, target = link->target;
        double source_scale = nodes[source].throughput > 0
                                  ? (nodes[source].y1 - nodes[source].y0) / nodes[source].throughput
                                  : 0;
        double target_scale = nodes[target].throughput > 0
                                  ? (nodes[target].y1 - nodes[target].y0) / nodes[target].throughput
                                  : 0;
        double start_y = nodes[source].y0 + source_used[source] * source_scale;
        double end_y = nodes[target].y0 + target_used[target] * target_scale;
        double start_w = link->value * source_scale, end_w = link->value * target_scale;
        source_used[source] += link->value;
        target_used[target] += link->value;
        double x0 = nodes[source].x1, x1 = nodes[target].x0;
        if (x1 <= x0)
            continue;
        cui_color color = link->color.a ? link->color : palette(o, source);
        color.a = (unsigned char)(color.a / 2);
        for (int x = (int)ceil(x0); x < (int)x1; x++) {
            double t = (x - x0) / (x1 - x0),
                   center = (1 - t) * (start_y + start_w / 2) + t * (end_y + end_w / 2);
            double half = ((1 - t) * start_w + t * end_w) / 2;
            rect(s, o, x, (int)floor(center - half), x + 1, (int)ceil(center + half), color);
        }
    }
    for (size_t i = 0; i < o->count; i++)
        rect(s, o, (int)floor(nodes[i].x0), (int)floor(nodes[i].y0), (int)ceil(nodes[i].x1),
             (int)ceil(nodes[i].y1), palette(o, i));
}
static uint64_t mix(uint64_t h, uint64_t v) {
    return (h ^ v) * UINT64_C(1099511628211);
}
static uint64_t bits(double v) {
    uint64_t u;
    memcpy(&u, &v, sizeof u);
    return u;
}
static uint64_t cache_key(const cui_chart_options *o, const cui_chart_state *s) {
    if (!o->data_revision)
        return 0;
    uint64_t h = UINT64_C(1469598103934665603);
    h = mix(h, o->data_revision);
    h = mix(h, (uintptr_t)s->pixels);
    h = mix(h, (uintptr_t)o->data);
    h = mix(h, (uintptr_t)o->links);
    h = mix(h, (uintptr_t)o->extra_series);
    h = mix(h, o->count);
    h = mix(h, o->link_count);
    h = mix(h, o->extra_series_count);
    h = mix(h, o->kind);
    h = mix(h, o->width);
    h = mix(h, o->height);
    h = mix(h, o->point_count);
    h = mix(h, o->tick_count);
    h = mix(h, o->grid_columns);
    h = mix(h, o->curve);
    h = mix(h, o->bar_alignment);
    h = mix(h, o->sankey_align);
    h = mix(h, o->hide_grid);
    h = mix(h, o->hide_axes);
    h = mix(h, o->hide_x_axis);
    h = mix(h, o->show_y_axis);
    h = mix(h, o->y_axis_inside);
    h = mix(h, o->grid_solid);
    h = mix(h, o->has_domain);
    h = mix(h, bits(o->domain_min));
    h = mix(h, bits(o->domain_max));
    h = mix(h, bits(o->inner_radius));
    h = mix(h, bits(o->pad_angle));
    h = mix(h, bits(o->bar_ratio));
    h = mix(h, bits(o->node_width));
    h = mix(h, bits(o->node_padding));
    h = mix(h, o->reference_count);
    for (size_t i = 0; i < o->reference_count; i++)
        h = mix(h, bits(o->reference_values[i]));
    const cui_color *colors = &o->background;
    for (size_t i = 0; i < sizeof(cui_color) * 13; i++)
        h = mix(h, ((const unsigned char *)colors)[i]);
    return h ? h : 1;
}
cui_plot_result cui_chart_render(cui_chart_state *s, const cui_chart_options *o) {
    cui_plot_result status = validate(s, o);
    if (status != CUI_PLOT_OK)
        return status;
    cui_chart_sankey_node nodes[256];
    if (o->kind == CUI_CHART_SANKEY) {
        status = cui_chart_sankey_layout(o, nodes, 256);
        if (status != CUI_PLOT_OK)
            return status;
    }
    uint64_t key = cache_key(o, s);
    if (key && key == s->cache_key)
        return CUI_PLOT_OK;
    for (size_t i = 0; i < (size_t)o->width * o->height; i++) {
        s->pixels[4 * i] = o->background.r;
        s->pixels[4 * i + 1] = o->background.g;
        s->pixels[4 * i + 2] = o->background.b;
        s->pixels[4 * i + 3] = o->background.a;
    }
    if (o->kind == CUI_CHART_PIE)
        pie(s, o);
    else if (o->kind <= CUI_CHART_BAR || o->kind == CUI_CHART_CANDLESTICK)
        cartesian(s, o);
    else if (o->kind == CUI_CHART_RADAR)
        radar(s, o);
    else if (o->kind == CUI_CHART_SANKEY)
        sankey(s, o, nodes);
    s->hovered = SIZE_MAX;
    s->hovered_value = 0;
    s->revision++;
    s->cache_key = key;
    return CUI_PLOT_OK;
}
cui_chart_hit cui_chart_hit_test(const cui_chart_options *o, float x, float y) {
    cui_chart_hit hit = {0};
    if (!o || !o->data || !o->count || o->count > CUI_CHART_MAX_ITEMS || o->noninteractive ||
        !isfinite(x) || !isfinite(y) || x < 0 || y < 0 ||
        x >= (o->logical_width > 0 ? o->logical_width : o->width) ||
        y >= (o->logical_height > 0 ? o->logical_height : o->height))
        return hit;
    if (o->logical_width > 0)
        x = (float)(x * o->width / o->logical_width);
    if (o->logical_height > 0)
        y = (float)(y * o->height / o->logical_height);
    size_t index = 0;
    if (o->kind == CUI_CHART_PIE) {
        double cx = o->width / 2.0, cy = o->height / 2.0, r = fmin(cx, cy) - 4, dx = x - cx,
               dy = y - cy;
        if (dx * dx + dy * dy > r * r || dx * dx + dy * dy < o->inner_radius * o->inner_radius)
            return hit;
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
        double left = plot_left(o), right = o->width > 36 ? o->width - 12 : left;
        double top = 12, bottom = o->height > 32 ? o->height - 20 : 12;
        if (x < left || x > right || y < top || y > bottom || right <= left)
            return hit;
        size_t n = o->point_count ? o->point_count : o->count;
        if (o->kind == CUI_CHART_BAR &&
            (o->bar_alignment == CUI_CHART_BAR_LEFT || o->bar_alignment == CUI_CHART_BAR_RIGHT)) {
            index = (size_t)((y - top) * n / (bottom - top));
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
    hit.value = o->data[index].value;
    return hit;
}
