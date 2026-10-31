/* SPDX-License-Identifier: MIT */
/* Measurements only: no renderer/policy acceptance or production shadow API. */
#include "cui/cui_effects.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
bool software_v2_original_reference(const cui_draw_list *, cui_color, unsigned, unsigned,
                                    unsigned char *);
enum { WIDTH = 8, HEIGHT = 8, FINE = 2048, COARSE = 512 };
typedef struct {
    const char *name;
    cui_drop_shadow_paint paint;
    float sx, sy, ox, oy;
    bool legacy_unit;
} scenario;
typedef struct {
    double l, t, r, b, rx[4], ry[4];
} geometry;
typedef struct {
    size_t gathers, collapsed, collapsed_active, reference_rows;
} work;
/* Reduce exact f32 q*origin/scale with small integer modular arithmetic.
 * These bounded scenarios need <=63 denominator bits; no generic bigint API. */
static double phase_exact(float q, float origin, float scale) {
    if (origin == 0)
        return 0;
    int eq, eo, es;
    uint64_t mq = (uint64_t)ldexp(frexpf(q, &eq), 24);
    uint64_t mo = (uint64_t)ldexp(frexpf(fabsf(origin), &eo), 24);
    uint64_t ms = (uint64_t)ldexp(frexpf(scale, &es), 24);
    int shift = eq + eo - es - 24;
    uint64_t numerator = mq * mo, denominator = ms, remainder;
    if (shift >= 0) {
        remainder = numerator % denominator;
        for (int i = 0; i < shift; ++i)
            remainder = remainder * 2 % denominator;
    } else {
        assert(-shift < 40 && denominator <= UINT64_MAX >> -shift);
        denominator <<= -shift;
        remainder = numerator % denominator;
    }
    double fraction = (double)remainder / denominator;
    return origin < 0 && remainder ? 1 - fraction : fraction;
}
static float phase(float q, float origin, float scale) {
    double z = (double)q * origin / scale;
    return (float)(z - floor(z));
}
static double phase_error(float q, float origin, float scale) {
    double d = fabs(phase(q, origin, scale) - phase_exact(q, origin, scale));
    return fmin(d, 1 - d);
}
static unsigned candidate(const scenario *s, bool inset, unsigned x, unsigned y, work *count) {
    if (!s->paint.layer.color.a)
        return 0;
    float q = fmaxf(1, fmaxf(s->sx, s->sy));
    float ox = phase(q, s->ox, s->sx), oy = phase(q, s->oy, s->sy);
    double gx = q * ((x + .5 - s->ox) / s->sx) + ox;
    double gy = q * ((y + .5 - s->oy) / s->sy) + oy;
    double ix = floor(gx - .5), iy = floor(gy - .5);
    double wx = gx - (ix + .5), wy = gy - (iy + .5), coverage = 0;
    bool collapsed = (float)(ix + .5) == (float)(ix + 1.5) || (float)(iy + .5) == (float)(iy + 1.5);
    bool active = false;
    cui_drop_shadow_paint p = s->paint;
    p.layer.color.a = 255;
    for (unsigned j = 0; j < 2; ++j)
        for (unsigned i = 0; i < 2; ++i) {
            float alpha = -1;
            bool ok = inset ? cui_effect_inset_shadow_sample(p, q, ox, oy, (float)(ix + i + .5),
                                                             (float)(iy + j + .5), &alpha)
                            : cui_effect_drop_shadow_sample(p, q, ox, oy, (float)(ix + i + .5),
                                                            (float)(iy + j + .5), &alpha);
            assert(ok && isfinite(alpha) && alpha >= 0 && alpha <= 1);
            count->gathers++;
            active |= alpha > 0;
            unsigned source_byte = (unsigned)lroundf(255 * alpha);
            coverage += source_byte * (i ? wx : 1 - wx) * (j ? wy : 1 - wy);
        }
    count->collapsed += collapsed;
    count->collapsed_active += collapsed && active;
    return (unsigned)lroundf((float)coverage * (s->paint.layer.color.a / 255.f));
}
/* Reference geometry deliberately has no source-grid snapping. Materialize the
 * existing logical f32 Bounds::dilate order, then transform axes independently. */
static geometry projected(const scenario *s, bool hole, bool element) {
    cui_drop_shadow_paint p = s->paint;
    float spread = element ? 0 : hole ? -p.layer.spread : p.layer.spread;
    float dx = element ? 0 : p.layer.offset_x, dy = element ? 0 : p.layer.offset_y;
    float l = (p.caster.x + dx) - spread, t = (p.caster.y + dy) - spread;
    float w = p.caster.w + (spread + spread), h = p.caster.h + (spread + spread);
    geometry g = {.l = (double)l * s->sx + s->ox, .t = (double)t * s->sy + s->oy};
    g.r = g.l + (double)w * s->sx;
    g.b = g.t + (double)h * s->sy;
    float limit = fminf(p.caster.w, p.caster.h) / 2;
    float radii[4] = {p.radii.top_left, p.radii.top_right, p.radii.bottom_left,
                      p.radii.bottom_right};
    for (unsigned i = 0; i < 4; ++i) {
        double radius = fminf(limit, radii[i]);
        if (hole)
            radius = fmaxf((float)radius - p.layer.spread, 0);
        g.rx[i] = radius * s->sx;
        g.ry[i] = radius * s->sy;
    }
    return g;
}
static bool reference_span(geometry g, double y, double *left, double *right) {
    if (g.l >= g.r || g.t >= g.b || y < g.t || y >= g.b)
        return false;
    double offset[4] = {0};
    for (unsigned i = 0; i < 4; ++i) {
        double distance = i & 2 ? g.b - y : y - g.t;
        if (g.rx[i] > 0 && g.ry[i] > 0 && distance < g.ry[i]) {
            double d = g.ry[i] - distance;
            offset[i] = g.rx[i] - (g.rx[i] / g.ry[i]) * sqrt(fmax(0, g.ry[i] * g.ry[i] - d * d));
        }
    }
    *left = g.l + fmax(offset[0], offset[2]);
    *right = g.r - fmax(offset[1], offset[3]);
    return *left < *right;
}
/* sigma0 policy: integrate actual pixel-square area, rather than an SDF ramp.
 * sigma>0: independent-axis Gaussian center convolution, tails truncated at8sigma.
 * Fixed midpoint integration has an independently reported convergence estimate. */
static double reference_coverage(geometry g, double x, double y, double sx, double sy,
                                 unsigned rows, work *count) {
    if (g.l >= g.r || g.t >= g.b)
        return 0;
    double lo, hi;
    if (!sx || !sy) {
        lo = fmax(g.t, y - .5);
        hi = fmin(g.b, y + .5);
    } else {
        lo = fmax(-8, (g.t - y) / sy);
        hi = fmin(8, (g.b - y) / sy);
    }
    if (lo >= hi)
        return 0;
    double step = (hi - lo) / rows, total = 0;
    for (unsigned i = 0; i < rows; ++i) {
        double z = lo + (i + .5) * step, left, right;
        double row_y = !sy ? z : y + z * sy;
        count->reference_rows++;
        if (!reference_span(g, row_y, &left, &right))
            continue;
        if (!sx)
            total += fmax(0, fmin(right, x + .5) - fmax(left, x - .5));
        else {
            double cdf =
                .5 * (erf((right - x) / (sqrt(2.) * sx)) - erf((left - x) / (sqrt(2.) * sx)));
            total += cdf * exp(-.5 * z * z) / sqrt(2 * 3.14159265358979323846);
        }
    }
    return total * step;
}
static double reference(const scenario *s, bool inset, unsigned x, unsigned y, unsigned rows,
                        work *count) {
    if (!s->paint.layer.color.a)
        return 0;
    double sigma_x = s->paint.layer.sigma * (double)s->sx;
    double sigma_y = s->paint.layer.sigma * (double)s->sy;
    double a = reference_coverage(projected(s, inset, false), x + .5, y + .5, sigma_x, sigma_y,
                                  rows, count);
    if (inset) {
        double edge =
            reference_coverage(projected(s, false, true), x + .5, y + .5, 0, 0, rows, count);
        a = (1 - a) * edge;
    }
    return a * s->paint.layer.color.a;
}
static scenario make(const char *name, float sx, float sy, float ox, float oy, float sigma) {
    return (scenario){.name = name,
                      .sx = sx,
                      .sy = sy,
                      .ox = ox,
                      .oy = oy,
                      .paint = {.caster = {2.25f, 2.5f, 4.5f, 3.25f},
                                .radii = {.5f, 1.5f, 1, .25f},
                                .layer = {.offset_x = .375f,
                                          .offset_y = -.25f,
                                          .sigma = sigma,
                                          .spread = .25f,
                                          .color = {40, 80, 120, 173}}}};
}
int main(void) {
    scenario s[20];
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
    assert(n == sizeof(s) / sizeof(s[0]));
    puts("name,kind,max_alpha_bytes,rms_alpha_bytes,ref_convergence_bytes,support_extra,support_"
         "missing,legacy_rgba_mismatches,phase_error,collapsed_cells,collapsed_active,gathers,"
         "reference_rows,cpu_seconds");
    size_t total_unit_mismatches = 0, total_active_collapse = 0;
    for (size_t k = 0; k < n; ++k)
        for (unsigned inset = 0; inset < 2; ++inset) {
            clock_t begin = clock();
            work count = {0};
            double maximum = 0, squares = 0, convergence = 0;
            size_t extra = 0, missing = 0, legacy_mismatches = 0;
            unsigned char old[WIDTH * HEIGHT * 4] = {0};
            if (s[k].legacy_unit) {
                cui_draw_command c = {.type = inset ? CUI_DRAW_INSET_SHADOW : CUI_DRAW_DROP_SHADOW,
                                      .shadow = s[k].paint};
                cui_draw_list l = {.commands = &c, .count = 1, .width = WIDTH, .height = HEIGHT};
                assert(software_v2_original_reference(&l, (cui_color){0}, WIDTH, HEIGHT, old));
            }
            for (unsigned y = 0; y < HEIGHT; ++y)
                for (unsigned x = 0; x < WIDTH; ++x) {
                    unsigned a = candidate(&s[k], inset != 0, x, y, &count);
                    assert(a <= 255);
                    double b = reference(&s[k], inset != 0, x, y, FINE, &count);
                    double coarse = reference(&s[k], inset != 0, x, y, COARSE, &count);
                    assert(isfinite(b) && b >= -1e-7 && b <= 255 + 1e-7);
                    double error = fabs(a - b);
                    maximum = fmax(maximum, error);
                    squares += error * error;
                    convergence = fmax(convergence, fabs(b - coarse));
                    extra += a > 0 && b < .5;
                    missing += a == 0 && b >= .5;
                    if (s[k].legacy_unit) {
                        const unsigned char *p = old + ((size_t)y * WIDTH + x) * 4;
                        unsigned char rgba[4] = {
                            a ? s[k].paint.layer.color.r : 0, a ? s[k].paint.layer.color.g : 0,
                            a ? s[k].paint.layer.color.b : 0, (unsigned char)a};
                        legacy_mismatches += memcmp(p, rgba, 4) != 0;
                    }
                }
            float q = fmaxf(1, fmaxf(s[k].sx, s[k].sy));
            double pe = fmax(phase_error(q, s[k].ox, s[k].sx), phase_error(q, s[k].oy, s[k].sy));
            printf("%s,%s,%.9g,%.9g,%.9g,%zu,%zu,%zu,%.9g,%zu,%zu,%zu,%zu,%.6f\n", s[k].name,
                   inset ? "inset" : "drop", maximum, sqrt(squares / (WIDTH * HEIGHT)), convergence,
                   extra, missing, legacy_mismatches, pe, count.collapsed, count.collapsed_active,
                   count.gathers, count.reference_rows, (double)(clock() - begin) / CLOCKS_PER_SEC);
            total_unit_mismatches += legacy_mismatches;
            total_active_collapse += count.collapsed_active;
        }
    /* Boundaries are real API rejections, not supported-domain float collapse. */
    cui_drop_shadow_paint invalid = s[0].paint;
    invalid.caster.x = 1000000;
    float sentinel = -123;
    assert(!cui_effect_drop_shadow_sample(invalid, 64, 0, 0, 64000000, 0, &sentinel) &&
           sentinel == -123);
    assert(!cui_effect_inset_shadow_sample(invalid, 64, 0, 0, 64000000, 0, &sentinel) &&
           sentinel == -123);
    assert(total_unit_mismatches == 0 && total_active_collapse == 0);
    return 0;
}
