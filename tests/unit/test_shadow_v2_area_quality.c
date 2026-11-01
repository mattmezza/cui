/* SPDX-License-Identifier: MIT */
/* TEST ONLY independent pixel-area Gaussian reference. */
#define main shadow_point_quality_original_main
#include "test_shadow_v2_quality.c"
#undef main
#include "shadow_v2_box_internal.h"

static size_t box_scenarios(scenario *s) {
    size_t n = 0;
    s[n++] = make("unit-crisp", 1, 1, 0, 0, 0);
    s[n - 1].legacy_unit = true;
    s[n++] = make("unit-blur", 1, 1, 0, 0, 1.5f);
    s[n - 1].legacy_unit = true;
    s[n++] = make("fractional-crisp", 1, 1, .375f, -.625f, 0);
    s[n++] = make("fractional-blur", 1, 1, .375f, -.625f, 1.5f);
    s[n++] = make("wide-crisp", 2, .75f, -3, .125f, 0);
    s[n++] = make("wide-blur", 2, .75f, -3, .125f, 1.5f);
    s[n++] = make("tall-crisp", .5f, 3, .25f, -6, 0);
    s[n++] = make("tall-blur", .5f, 3, .25f, -6, 1.5f);
    s[n++] = make("min-axis-crisp", 1.f / 65536, 4, .5f - 4.5f / 65536, -8, 0);
    s[n++] = make("min-axis-blur", 1.f / 65536, 4, .5f - 4.5f / 65536, -8, 1.5f);
    s[n++] = make("max-min-crisp", 64, 1.f / 65536, -144, .5f - 4.f / 65536, 0);
    s[n++] = make("max-min-blur", 64, 1.f / 65536, -144, .5f - 4.f / 65536, 1.5f);
    s[n++] = make("max-both", 64, 64, -144, -160, 1.5f);
    s[n++] = make("large-visible", nextafterf(1.f / 65536, INFINITY), 64, .25f, -960000, 1.5f);
    s[n - 1].paint.caster = (cui_rect){16384, 15000, 4, 4};
    s[n - 1].ox = .5f - s[n - 1].sx * 16386;
    s[n] = s[n - 1];
    n++;
    s[n - 1].name = "large-visible-crisp";
    s[n - 1].paint.layer.sigma = 0;
    s[n++] = make("negative-spread", 1.25f, .625f, .5f, 1, 1.5f);
    s[n - 1].paint.layer.spread = -1.5f;
    s[n++] = make("transparent", 2, .75f, -3, .125f, 1.5f);
    s[n - 1].paint.layer.color.a = 0;
    s[n++] = make("offscreen-phase", nextafterf(1.f / 65536, INFINITY), 64, 1000000, .375f, 1.5f);
    s[n++] = make("collapsed-hole", 1, 1, 0, 0, 1.5f);
    s[n - 1].paint.layer.spread = 2;
    s[n - 1].legacy_unit = true;
    s[n++] = make("subpixel-thin", 1, 1, 0, 0, 0);
    s[n - 1].paint.caster.w = .125f;
    s[n - 1].paint.caster.h = .25f;
    s[n - 1].legacy_unit = true;
    assert(n == 20);
    return n;
}

typedef struct {
    shadow_box_support support;
    const unsigned *bytes;
    size_t calls, fail;
} table;
static bool table_read(void *user, int64_t x, int64_t y, unsigned *byte) {
    table *t = user;
    assert(x >= t->support.left && x < t->support.right && y >= t->support.top &&
           y < t->support.bottom);
    t->calls++;
    if (t->calls == t->fail)
        return false;
    *byte = t->bytes[(size_t)(y - t->support.top) * (size_t)(t->support.right - t->support.left) +
                     (size_t)(x - t->support.left)];
    return true;
}
static void tiny_overlap_oracles(void) {
    const unsigned cells[4] = {10, 30, 50, 90};
    table t = {{0, 0, 2, 2}, cells, 0, 0};
    double out[64];
    size_t taps = 99;
    for (unsigned i = 0; i < 64; ++i)
        out[i] = -777;
    assert(shadow_box_integrate(0, 0, 1, 1, 2, 2, t.support, 4, table_read, &t, out, &taps) ==
           CUI_OK);
    assert(taps == 4 && t.calls == 4 && out[0] == 10 && out[1] == 30 && out[2] == 50 &&
           out[3] == 90 && out[4] == -777);
    t.calls = 0;
    assert(shadow_box_integrate(0, 0, 2, 2, 1, 1, t.support, 4, table_read, &t, out, &taps) ==
               CUI_OK &&
           out[0] == 45);
    t.calls = 0;
    assert(shadow_box_integrate(-.5, -.5, 2, 2, 1, 1, t.support, 4, table_read, &t, out, &taps) ==
               CUI_OK &&
           out[0] == 18.125);
    t.support = (shadow_box_support){-1, -1, 1, 1};
    t.calls = 0;
    assert(shadow_box_integrate(-.25, -.25, 1.5, 2.5, 1, 1, t.support, 4, table_read, &t, out,
                                &taps) == CUI_OK &&
           out[0] == 29.5);
    t.calls = 0;
    assert(shadow_box_integrate(10, 10, 2, 2, 1, 1, t.support, 0, table_read, &t, out, &taps) ==
               CUI_OK &&
           out[0] == 0 && !taps && !t.calls);
    t.support = (shadow_box_support){0, 0, 2, 2};
    t.calls = 0;
    for (unsigned i = 0; i < 64; ++i)
        out[i] = -777;
    double before[64];
    memcpy(before, out, sizeof out);
    assert(shadow_box_integrate(0, 0, 2, 2, 1, 1, t.support, 3, table_read, &t, out, &taps) ==
               CUI_ERROR_CAPACITY &&
           !taps && !t.calls && memcmp(out, before, sizeof out) == 0);
    t.fail = 3;
    t.calls = 0;
    assert(shadow_box_integrate(0, 0, 2, 2, 1, 1, t.support, 4, table_read, &t, out, &taps) ==
               CUI_ERROR_INVALID_ACTION &&
           taps == 3 && t.calls == 3 && memcmp(out, before, sizeof out) == 0);
    t.fail = 0;
    t.calls = 0;
    assert(shadow_box_integrate(0, 0, 2, 2, 1, 1, t.support, 4, table_read, &t, out, &taps) ==
               CUI_OK &&
           out[0] == 45);
    t.support = (shadow_box_support){-1000000, -1000000, 1000000, 1000000};
    t.calls = 0;
    memcpy(before, out, sizeof out);
    assert(shadow_box_integrate(-2000000, -2000000, 4000000, 4000000, 1, 1, t.support,
                                SHADOW_BOX_MAX_TAPS, table_read, &t, out,
                                &taps) == CUI_ERROR_CAPACITY &&
           !taps && !t.calls && memcmp(out, before, sizeof out) == 0);
    unsigned uniform[16];
    for (unsigned i = 0; i < 16; ++i)
        uniform[i] = 255;
    t = (table){{0, 0, 4, 4}, uniform, 0, 0};
    assert(shadow_box_integrate(0, 0, 2, 3, 1, 1, t.support, 6, table_read, &t, out, &taps) ==
               CUI_OK &&
           taps == 6 && out[0] == 255);
    const unsigned one[] = {200};
    t = (table){{0, 0, 1, 1}, one, 0, 0};
    assert(shadow_box_integrate(-1, -.5, 4, 2, 1, 1, t.support, 1, table_read, &t, out, &taps) ==
               CUI_OK &&
           taps == 1 && out[0] == 25);
}
static void actual_limits(void) {
    scenario s = make("large-query", 64, 1.f / 65536, 0, .5f - 1.f / 65536, 0);
    s.paint =
        (cui_drop_shadow_paint){.caster = {0, 0, 16384, 2}, .layer = {.color = {40, 80, 120, 173}}};
    cui_drop_shadow_raster old_query;
    memset(&old_query, 0xa7, sizeof old_query);
    cui_drop_shadow_raster unchanged;
    memcpy(&unchanged, &old_query, sizeof unchanged);
    assert(!cui_effect_drop_shadow_query(s.paint, 64, 0, 0, &old_query) &&
           memcmp(&old_query, &unchanged, sizeof old_query) == 0);
    unsigned char out[64], before[64];
    memset(out, 0xa7, sizeof out);
    memcpy(before, out, sizeof out);
    size_t taps = 0;
    assert(shadow_box_candidate(&s, false, 1023, out, &taps) == CUI_ERROR_CAPACITY && !taps &&
           memcmp(out, before, sizeof out) == 0);
    assert(shadow_box_candidate(&s, false, 1024, out, &taps) == CUI_OK && taps == 1024);
    for (unsigned i = 0; i < 64; ++i)
        assert(out[i] == 0);
    s.sx = 1.f / 65536;
    s.sy = 64;
    s.ox = .375f;
    s.oy = 0;
    memset(out, 0xa7, sizeof out);
    memcpy(before, out, sizeof out);
    assert(shadow_box_candidate(&s, false, SHADOW_BOX_MAX_TAPS, out, &taps) == CUI_ERROR_CAPACITY &&
           !taps && memcmp(out, before, sizeof out) == 0);
    s.paint.layer.color.a = 0;
    assert(shadow_box_candidate(&s, false, 0, out, &taps) == CUI_OK && !taps);
    for (unsigned i = 0; i < 64; ++i)
        assert(out[i] == 0);
    s.paint.caster.x = 1000000;
    memset(out, 0xa7, sizeof out);
    memcpy(before, out, sizeof out);
    assert(shadow_box_candidate(&s, false, 0, out, &taps) == CUI_ERROR_INVALID_ACTION &&
           memcmp(out, before, sizeof out) == 0);
}

static double area_phi(double x) {
    return .5 * erfc(-x / sqrt(2.));
}
static double area_primitive(double x, double sigma) {
    double t = x / sigma;
    if (t > 9)
        return x;
    if (t < -9)
        return 0;
    return x * area_phi(t) + sigma * exp(-.5 * t * t) / sqrt(2 * 3.14159265358979323846);
}
/* Integral over target [a,b] of convolution of source interval [l,r].
 * Primitive branches are Gaussian-tail truncation, not profile reconstruction. */
static double area_interval(double l, double r, double a, double b, double sigma) {
    if (l >= r || a >= b)
        return 0;
    if (!sigma)
        return fmax(0, fmin(r, b) - fmax(l, a));
    if (l >= b + 9 * sigma || r <= a - 9 * sigma)
        return 0;
    double v = area_primitive(r - a, sigma) - area_primitive(r - b, sigma) -
               area_primitive(l - a, sigma) + area_primitive(l - b, sigma);
    assert(v >= -1e-8 && v <= b - a + 1e-8);
    return fmax(0, fmin(b - a, v));
}
/* Explicit segment boundaries resolve corners and narrow Gaussian transitions.
 * Midpoint refinement is reported independently, never a proven error bound. */
static size_t area_splits(double lo, double hi, geometry g, double a, double b, double sigma,
                          double out[32]) {
    size_t n = 0;
    double points[] = {lo,
                       hi,
                       g.t,
                       g.b,
                       g.t + g.ry[0],
                       g.t + g.ry[1],
                       g.b - g.ry[2],
                       g.b - g.ry[3],
                       a,
                       b,
                       a - 8 * sigma,
                       a - sigma,
                       a + sigma,
                       a + 8 * sigma,
                       b - 8 * sigma,
                       b - sigma,
                       b + sigma,
                       b + 8 * sigma};
    for (size_t i = 0; i < sizeof points / sizeof *points; ++i) {
        double v = points[i];
        if (v < lo || v > hi)
            continue;
        size_t j = 0;
        while (j < n && out[j] < v)
            ++j;
        if (j < n && out[j] == v)
            continue;
        memmove(out + j + 1, out + j, (n - j) * sizeof *out);
        out[j] = v;
        ++n;
    }
    return n;
}
static double area_drop(geometry g, unsigned x, unsigned y, double sx, double sy, unsigned rows,
                        size_t *work_count) {
    double lo = fmax(g.t, y - 8 * sy), hi = fmin(g.b, y + 1. + 8 * sy), split[32];
    if (lo >= hi)
        return 0;
    size_t n = area_splits(lo, hi, g, y, y + 1., sy, split);
    double total = 0;
    for (size_t j = 1; j < n; ++j) {
        double step = (split[j] - split[j - 1]) / rows;
        for (unsigned i = 0; i < rows; ++i) {
            double v = split[j - 1] + (i + .5) * step, l, r;
            ++*work_count;
            if (reference_span(g, v, &l, &r)) {
                double wy = area_phi((y + 1. - v) / sy) - area_phi((y - v) / sy);
                total += area_interval(l, r, x, x + 1., sx) * wy * step;
            }
        }
    }
    return total;
}
static double area_hole_row(geometry hole, double target_y, double l, double r, double sx,
                            double sy, unsigned rows, size_t *work_count) {
    double lo = fmax(-8, (hole.t - target_y) / sy);
    double hi = fmin(8, (hole.b - target_y) / sy);
    if (lo >= hi || hole.l >= hole.r)
        return 0;
    double total = 0, step = (hi - lo) / rows;
    for (unsigned i = 0; i < rows; ++i) {
        double z = lo + (i + .5) * step, a, b;
        ++*work_count;
        if (reference_span(hole, target_y + z * sy, &a, &b))
            total += area_interval(a, b, l, r, sx) * exp(-.5 * z * z) /
                     sqrt(2 * 3.14159265358979323846) * step;
    }
    return total;
}
static double area_reference(const scenario *s, bool inset, unsigned x, unsigned y, unsigned rows,
                             size_t *work_count) {
    if (!s->paint.layer.color.a)
        return 0;
    if (!s->paint.layer.sigma) {
        work count = {0};
        double v = reference(s, inset, x, y, rows * 4, &count);
        *work_count += count.reference_rows;
        return v;
    }
    double sx = s->paint.layer.sigma * (double)s->sx;
    double sy = s->paint.layer.sigma * (double)s->sy;
    geometry g = projected(s, inset, false);
    double total;
    if (!inset)
        total = area_drop(g, x, y, sx, sy, rows * 2, work_count);
    else {
        geometry element = projected(s, false, true);
        double lo = fmax(y, element.t), hi = fmin(y + 1., element.b), split[32];
        if (lo >= hi)
            return 0;
        size_t n = area_splits(lo, hi, element, g.t, g.b, sy, split);
        total = 0;
        for (size_t j = 1; j < n; ++j) {
            double step = (split[j] - split[j - 1]) / rows;
            for (unsigned i = 0; i < rows; ++i) {
                double v = split[j - 1] + (i + .5) * step, l, r;
                ++*work_count;
                if (!reference_span(element, v, &l, &r))
                    continue;
                l = fmax(l, x);
                r = fmin(r, x + 1.);
                if (l < r)
                    total +=
                        ((r - l) - area_hole_row(g, v, l, r, sx, sy, rows * 4, work_count)) * step;
            }
        }
    }
    assert(total >= -1e-7 && total <= 1 + 1e-7);
    return fmax(0, fmin(1, total)) * s->paint.layer.color.a;
}
static void area_analytical_oracles(void) {
    double sigma = .75;
    /* Gaussian/box convolution is symmetric in source/target intervals. */
    double a = area_interval(-.3, .8, -.7, .2, sigma);
    assert(fabs(a - area_interval(-.7, .2, -.3, .8, sigma)) < 1e-14);
    assert(area_interval(-100, 100, 0, 1, sigma) == 1);
    assert(area_interval(100, 101, 0, 1, sigma) == 0);
    /* Independent midpoint integration of CDF difference, no antiderivative. */
    double b = 0;
    for (unsigned i = 0; i < 65536; ++i) {
        double u = -.7 + (i + .5) * .9 / 65536;
        b += (area_phi((.8 - u) / sigma) - area_phi((-.3 - u) / sigma)) * .9 / 65536;
    }
    assert(fabs(a - b) < 1e-10);
}
int main(void) {
    area_analytical_oracles();
    tiny_overlap_oracles();
    actual_limits();
    scenario cases[20];
    size_t n = box_scenarios(cases), legacy = 0;
    puts("name,kind,area_max_alpha_bytes,area_rms_alpha_bytes,center_max_alpha_bytes,"
         "area_refinement_bytes,area_support_extra,area_support_missing,reference_work,"
         "legacy_rgba_mismatches,cpu_seconds");
    for (size_t k = 0; k < n; ++k)
        for (unsigned inset = 0; inset < 2; ++inset) {
            const scenario *s = &cases[k];
            clock_t start = clock();
            unsigned char box[64], old[256] = {0};
            size_t taps, count = 0, mismatch = 0, extra = 0, missing = 0;
            assert(shadow_box_candidate(s, inset != 0, SHADOW_BOX_MAX_TAPS, box, &taps) == CUI_OK);
            if (s->legacy_unit) {
                cui_draw_command c = {.type = inset ? CUI_DRAW_INSET_SHADOW : CUI_DRAW_DROP_SHADOW,
                                      .shadow = s->paint};
                cui_draw_list l = {.commands = &c, .count = 1, .width = WIDTH, .height = HEIGHT};
                assert(software_v2_original_reference(&l, (cui_color){0}, WIDTH, HEIGHT, old));
            }
            double maximum = 0, squares = 0, center = 0, convergence = 0;
            for (unsigned y = 0; y < HEIGHT; ++y)
                for (unsigned x = 0; x < WIDTH; ++x) {
                    double a = area_reference(s, inset != 0, x, y, 64, &count);
                    double b = area_reference(s, inset != 0, x, y, 32, &count);
                    work w = {0};
                    double c = reference(s, inset != 0, x, y, FINE, &w);
                    unsigned v = box[y * WIDTH + x];
                    double error = fabs(v - a);
                    maximum = fmax(maximum, error);
                    squares += error * error;
                    convergence = fmax(convergence, fabs(a - b));
                    center = fmax(center, fabs(v - c));
                    extra += v > 0 && a < .5;
                    missing += !v && a >= .5;
                    if (s->legacy_unit) {
                        unsigned char rgba[4] = {v ? s->paint.layer.color.r : 0,
                                                 v ? s->paint.layer.color.g : 0,
                                                 v ? s->paint.layer.color.b : 0, (unsigned char)v};
                        mismatch += memcmp(old + (y * WIDTH + x) * 4, rgba, 4) != 0;
                    }
                }
            printf("%s,%s,%.9g,%.9g,%.9g,%.9g,%zu,%zu,%zu,%zu,%.6f\n", s->name,
                   inset ? "inset" : "drop", maximum, sqrt(squares / 64), center, convergence,
                   extra, missing, count, mismatch, (double)(clock() - start) / CLOCKS_PER_SEC);
            legacy += mismatch;
        }
    assert(!legacy);
    return 0;
}
