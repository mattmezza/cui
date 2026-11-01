/* SPDX-License-Identifier: MIT */
#include "cui/cui_effects.h"

#include "../../src/effects/cui_shadow_profile_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define DECL(kind, type, raster)                                                                   \
    bool original_cui_effect_##kind##_shadow_query(type, float, float, float, raster *);           \
    bool original_cui_effect_##kind##_shadow_sample(type, float, float, float, float, float,       \
                                                    float *);                                      \
    bool original_cui_effect_##kind##_shadow_raster(type, float, float, float, cui_effect_buffer)
DECL(drop, cui_drop_shadow_paint, cui_drop_shadow_raster);
DECL(inset, cui_inset_shadow_paint, cui_inset_shadow_raster);
static void rect_equal(cui_rect a, cui_rect b) {
    assert(a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h);
}
static size_t samples, rasters;
static void compare(cui_drop_shadow_paint p, float scale, float ox, float oy) {
    cui_drop_shadow_raster a, b, a_before;
    cui_inset_shadow_raster c, d, c_before;
    memset(&a, 0xa7, sizeof a);
    memset(&b, 0xa7, sizeof b);
    memset(&c, 0xa7, sizeof c);
    memset(&d, 0xa7, sizeof d);
    memcpy(&a_before, &a, sizeof a);
    memcpy(&c_before, &c, sizeof c);
    bool aq = cui_effect_drop_shadow_query(p, scale, ox, oy, &a);
    bool bq = original_cui_effect_drop_shadow_query(p, scale, ox, oy, &b);
    bool cq = cui_effect_inset_shadow_query(p, scale, ox, oy, &c);
    bool dq = original_cui_effect_inset_shadow_query(p, scale, ox, oy, &d);
    assert(aq == bq && cq == dq);
    if (!aq)
        assert(memcmp(&a, &a_before, sizeof a) == 0 && memcmp(&b, &a_before, sizeof b) == 0);
    if (!cq)
        assert(memcmp(&c, &c_before, sizeof c) == 0 && memcmp(&d, &c_before, sizeof d) == 0);
    if (aq) {
        rect_equal(a.covered_layer, b.covered_layer);
        rect_equal(a.bounds, b.bounds);
        assert(a.width == b.width && a.height == b.height && a.bytes == b.bytes);
    }
    if (cq) {
        rect_equal(c.covered_hole, d.covered_hole);
        rect_equal(c.covered_element, d.covered_element);
        rect_equal(c.bounds, d.bounds);
        assert(c.width == d.width && c.height == d.height && c.bytes == d.bytes);
    }
    cui_shadow_drop_profile_internal drop = {0};
    cui_shadow_inset_profile_internal inset = {0};
    bool dp = cui_shadow_drop_prepare_internal(p, scale, ox, oy, &drop);
    bool ip = cui_shadow_inset_prepare_internal(p, scale, ox, oy, &inset);
    assert(dp == ip);
    cui_shadow_profile_support_internal ds = {0}, is = {0};
    if (dp) {
        assert(cui_shadow_drop_support_internal(&drop, &ds));
        assert(cui_shadow_inset_support_internal(&inset, &is));
    }
    for (unsigned i = 0; i < 81; ++i) {
        float x = dp ? drop.covered.x - 2 + (i % 9) * (drop.covered.w + 4) / 8 : .5f;
        float y = dp ? drop.covered.y - 2 + (i / 9) * (drop.covered.h + 4) / 8 : .5f;
        float u = -123, v = -123;
        bool ok = cui_effect_drop_shadow_sample(p, scale, ox, oy, x, y, &u);
        assert(ok == original_cui_effect_drop_shadow_sample(p, scale, ox, oy, x, y, &v));
        assert(u == v);
        if (dp) {
            assert(ok && u == cui_shadow_drop_eval_internal(&drop, x, y));
            if (x < ds.left || x >= ds.right || y < ds.top || y >= ds.bottom)
                assert(u == 0);
        }
        u = v = -123;
        ok = cui_effect_inset_shadow_sample(p, scale, ox, oy, x, y, &u);
        assert(ok == original_cui_effect_inset_shadow_sample(p, scale, ox, oy, x, y, &v));
        assert(u == v);
        if (ip) {
            assert(ok && u == cui_shadow_inset_eval_internal(&inset, x, y));
            if (x < is.left || x >= is.right || y < is.top || y >= is.bottom)
                assert(u == 0);
        }
        samples += 2;
    }
    unsigned char now[65536], old[65536];
    for (unsigned kind = 0; kind < 2; ++kind) {
        if (!(kind ? cq : aq))
            continue;
        unsigned w = kind ? c.width : a.width, h = kind ? c.height : a.height;
        size_t stride = (size_t)w * 4 + 8, span = h ? (h - 1) * stride + (size_t)w * 4 : 0;
        if (span > sizeof now)
            continue;
        memset(now, 0xa5, sizeof now);
        memset(old, 0xa5, sizeof old);
        cui_effect_buffer n = {now, w, h, stride, span}, o = {old, w, h, stride, span};
        bool nr = kind ? cui_effect_inset_shadow_raster(p, scale, ox, oy, n)
                       : cui_effect_drop_shadow_raster(p, scale, ox, oy, n);
        bool or = kind ? original_cui_effect_inset_shadow_raster(p, scale, ox, oy, o)
                       : original_cui_effect_drop_shadow_raster(p, scale, ox, oy, o);
        assert(nr == or && nr && memcmp(now, old, sizeof now) == 0);
        ++rasters;
        if (w && h) {
            n.size = span - 1;
            memset(now, 0x75, sizeof now);
            assert(!(kind ? cui_effect_inset_shadow_raster(p, scale, ox, oy, n)
                          : cui_effect_drop_shadow_raster(p, scale, ox, oy, n)));
            for (size_t i = 0; i < sizeof now; ++i)
                assert(now[i] == 0x75);
        }
    }
}
static cui_drop_shadow_paint paint(void) {
    return (cui_drop_shadow_paint){.caster = {2.25f, -3.75f, 7.5f, 5.25f},
                                   .radii = {.25f, 2.5f, 1.25f, .75f},
                                   .layer = {.offset_x = .375f,
                                             .offset_y = -.25f,
                                             .sigma = 1.5f,
                                             .spread = .25f,
                                             .color = {40, 80, 120, 173}}};
}
static void negatives(void) {
    cui_drop_shadow_paint p = paint();
    float *fields[] = {&p.caster.x,          &p.caster.y,           &p.caster.w,
                       &p.caster.h,          &p.radii.top_left,     &p.radii.top_right,
                       &p.radii.bottom_left, &p.radii.bottom_right, &p.layer.sigma,
                       &p.layer.spread,      &p.layer.offset_x,     &p.layer.offset_y};
    const float poison[] = {NAN, INFINITY, -INFINITY};
    for (size_t i = 0; i < sizeof fields / sizeof *fields; ++i) {
        float saved = *fields[i];
        for (size_t j = 0; j < sizeof poison / sizeof *poison; ++j) {
            *fields[i] = poison[j];
            compare(p, 1, 0, 0);
            cui_shadow_drop_profile_internal out, before;
            memset(&out, 0xab, sizeof out);
            memcpy(&before, &out, sizeof out);
            assert(!cui_shadow_drop_prepare_internal(p, 1, 0, 0, &out));
            assert(memcmp(&out, &before, sizeof out) == 0);
            cui_shadow_inset_profile_internal io, ib;
            memset(&io, 0x73, sizeof io);
            memcpy(&ib, &io, sizeof io);
            assert(!cui_shadow_inset_prepare_internal(p, 1, 0, 0, &io));
            assert(memcmp(&io, &ib, sizeof io) == 0);
        }
        *fields[i] = saved;
    }
    const float scales[] = {
        0,           -1, NAN, INFINITY, nextafterf(1.f / 65536, 0), nextafterf(64, INFINITY),
        1.f / 65536, 64};
    for (size_t i = 0; i < sizeof scales / sizeof *scales; ++i)
        compare(p, scales[i], 0, 0);
    const float origins[] = {NAN, INFINITY, nextafterf(1048576, INFINITY), -1048576, 1048576};
    for (size_t i = 0; i < sizeof origins / sizeof *origins; ++i) {
        compare(p, 1, origins[i], 0);
        compare(p, 1, 0, origins[i]);
    }
    for (size_t i = 0; i < sizeof fields / sizeof *fields; ++i) {
        float saved = *fields[i];
        float limit = i < 8 ? 16384 : i == 8 ? 256 : 4096;
        *fields[i] = nextafterf(limit, INFINITY);
        compare(p, 1, 0, 0);
        *fields[i] = limit;
        compare(p, 1, 0, 0);
        *fields[i] = nextafterf(i == 0 || i == 1 || i >= 9 ? -limit : 0, -INFINITY);
        compare(p, 1, 0, 0);
        *fields[i] = i == 0 || i == 1 || i >= 9 ? -limit : 0;
        compare(p, 1, 0, 0);
        *fields[i] = saved;
    }
    p.layer.sigma = nextafterf(0, 1);
    compare(p, 1, 0, 0);
    float out = 123;
    assert(!cui_effect_drop_shadow_sample(paint(), 1, 0, 0, NAN, 0, &out) && out == 123);
    assert(!cui_effect_inset_shadow_sample(paint(), 1, 0, 0, 0, INFINITY, &out) && out == 123);
    assert(!cui_shadow_drop_prepare_internal(paint(), 1, 0, 0, NULL));
    assert(!cui_shadow_inset_prepare_internal(paint(), 1, 0, 0, NULL));
    int64_t integer = 123;
    const float bad[] = {NAN, INFINITY, -INFINITY, 0x1p63f, nextafterf(-0x1p63f, -INFINITY), .5f};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; ++i) {
        assert(!cui_shadow_profile_integer_internal(bad[i], &integer) && integer == 123);
    }
    assert(cui_shadow_profile_integer_internal(-0x1p63f, &integer) && integer == INT64_MIN);
}
static void support_cases(void) {
    cui_drop_shadow_paint p = {.caster = {-2, 3, 4, 5}, .layer = {.color = {0}}};
    cui_shadow_drop_profile_internal d;
    cui_shadow_inset_profile_internal i;
    cui_shadow_profile_support_internal s;
    assert(cui_shadow_drop_prepare_internal(p, 1, 0, 0, &d));
    assert(cui_shadow_drop_support_internal(&d, &s));
    assert(s.left == -2 && s.top == 3 && s.right == 2 && s.bottom == 8);
    p.layer.sigma = .5f;
    assert(cui_shadow_drop_prepare_internal(p, 1, 0, 0, &d));
    assert(cui_shadow_drop_support_internal(&d, &s));
    assert(s.left == -4 && s.top == 1 && s.right == 4 && s.bottom == 10);
    p.layer.spread = 8;
    assert(cui_shadow_inset_prepare_internal(p, 1, 0, 0, &i));
    assert(cui_shadow_inset_support_internal(&i, &s));
    assert(!s.left && !s.top && !s.right && !s.bottom);
    assert(cui_shadow_inset_eval_internal(&i, 0, 5) == 0);
    p = (cui_drop_shadow_paint){.caster = {0, 0, 16384, 2}};
    cui_drop_shadow_raster q;
    assert(!cui_effect_drop_shadow_query(p, 64, 0, 0, &q));
    assert(cui_shadow_drop_prepare_internal(p, 64, 0, 0, &d));
    assert(cui_shadow_drop_support_internal(&d, &s) && s.right == 1048576 && s.bottom == 128);
    /* Active extrema across q64 sign/offset/spread/radius/sigma extremes. */
    for (int x = -1; x <= 1; x += 2)
        for (int y = -1; y <= 1; y += 2)
            for (int spread = -1; spread <= 1; spread += 2) {
                p = (cui_drop_shadow_paint){.caster = {x * 16384.f, y * 16384.f, 16384, 16384},
                                            .radii = {16384, 16384, 16384, 16384},
                                            .layer = {.offset_x = x * 4096.f,
                                                      .offset_y = y * 4096.f,
                                                      .spread = spread * 4096.f,
                                                      .sigma = 256}};
                assert(cui_shadow_drop_prepare_internal(p, 64, nextafterf(1, 0), 0, &d));
                assert(cui_shadow_drop_support_internal(&d, &s));
                assert(fabs((double)s.left) < 0x1p22 && fabs((double)s.right) < 0x1p22 &&
                       fabs((double)s.top) < 0x1p22 && fabs((double)s.bottom) < 0x1p22);
                assert((float)(s.left + .5) != (float)(s.left + 1.5));
                assert((float)(s.right - .5) != (float)(s.right - 1.5));
            }
}
int main(void) {
    for (unsigned k = 0; k < 6; ++k) {
        cui_drop_shadow_paint p = paint();
        if (k == 1)
            p.layer.sigma = 0;
        if (k == 2)
            p.layer.spread = -2;
        if (k == 3)
            p.layer.spread = 8;
        if (k == 4) {
            p.caster.w = .1f;
            p.caster.h = .1f;
            p.layer.spread = -.1f;
        }
        if (k == 5)
            p.layer.color.a = 0;
        compare(p, 1, 0, 0);
        compare(p, 1.25f, .375f, -.625f);
        compare(p, 2, -.25f, .75f);
    }
    negatives();
    support_cases();
    printf("original Effects equivalence: %zu finite samples, %zu padded rasters\n", samples,
           rasters);
    return 0;
}
