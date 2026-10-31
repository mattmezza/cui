/* SPDX-License-Identifier: MIT */
/* Preserve the original point-policy source/receipt byte-for-byte. */
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
int main(void) {
    tiny_overlap_oracles();
    actual_limits();
    scenario cases[20];
    size_t n = box_scenarios(cases);
    puts("name,kind,box_max_alpha_bytes,box_rms_alpha_bytes,point_max_alpha_bytes,point_rms_alpha_"
         "bytes,ref_convergence_bytes,support_extra,support_missing,legacy_rgba_mismatches,uniform_"
         "point_mismatches,box_taps,point_taps,reference_rows,box_cpu_seconds,total_cpu_seconds");
    size_t total_legacy = 0, total_uniform = 0;
    for (size_t k = 0; k < n; ++k)
        for (unsigned inset = 0; inset < 2; ++inset) {
            const scenario *s = &cases[k];
            clock_t begin = clock();
            unsigned char box[64];
            size_t taps = 0;
            assert(shadow_box_candidate(s, inset != 0, SHADOW_BOX_MAX_TAPS, box, &taps) == CUI_OK);
            clock_t box_ticks = clock() - begin;
            unsigned char old[WIDTH * HEIGHT * 4] = {0};
            if (s->legacy_unit) {
                cui_draw_command c = {.type = inset ? CUI_DRAW_INSET_SHADOW : CUI_DRAW_DROP_SHADOW,
                                      .shadow = s->paint};
                cui_draw_list l = {.commands = &c, .count = 1, .width = WIDTH, .height = HEIGHT};
                assert(software_v2_original_reference(&l, (cui_color){0}, WIDTH, HEIGHT, old));
            }
            work count = {0};
            double maximum = 0, squares = 0, pmax = 0, psquares = 0, convergence = 0;
            size_t extra = 0, missing = 0, legacy = 0, uniform = 0;
            for (unsigned y = 0; y < HEIGHT; ++y)
                for (unsigned x = 0; x < WIDTH; ++x) {
                    unsigned a = box[y * WIDTH + x], point = candidate(s, inset != 0, x, y, &count);
                    double b = reference(s, inset != 0, x, y, FINE, &count),
                           coarse = reference(s, inset != 0, x, y, COARSE, &count);
                    assert(isfinite(b) && b >= -1e-7 && b <= 255 + 1e-7);
                    double error = fabs(a - b), pe = fabs(point - b);
                    maximum = fmax(maximum, error);
                    pmax = fmax(pmax, pe);
                    squares += error * error;
                    psquares += pe * pe;
                    convergence = fmax(convergence, fabs(b - coarse));
                    extra += a > 0 && b < .5;
                    missing += a == 0 && b >= .5;
                    if (s->sx == s->sy && s->sx >= 1)
                        uniform += a != point;
                    if (s->legacy_unit) {
                        const unsigned char *p = old + ((size_t)y * WIDTH + x) * 4;
                        unsigned char rgba[4] = {a ? s->paint.layer.color.r : 0,
                                                 a ? s->paint.layer.color.g : 0,
                                                 a ? s->paint.layer.color.b : 0, (unsigned char)a};
                        legacy += memcmp(p, rgba, 4) != 0;
                    }
                }
            printf("%s,%s,%.9g,%.9g,%.9g,%.9g,%.9g,%zu,%zu,%zu,%zu,%zu,%zu,%zu,%.6f,%.6f\n",
                   s->name, inset ? "inset" : "drop", maximum, sqrt(squares / 64), pmax,
                   sqrt(psquares / 64), convergence, extra, missing, legacy, uniform, taps,
                   count.gathers, count.reference_rows, (double)box_ticks / CLOCKS_PER_SEC,
                   (double)(clock() - begin) / CLOCKS_PER_SEC);
            total_legacy += legacy;
            total_uniform += uniform;
        }
    assert(!total_legacy && !total_uniform);
    return 0;
}
