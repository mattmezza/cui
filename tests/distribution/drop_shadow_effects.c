/* SPDX-License-Identifier: MIT */
/* Standalone Effects implementation: deliberately no Core implementation. */
#define CUI_EFFECTS_IMPLEMENTATION
#include "cui_effects.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static cui_drop_shadow_paint paint(void) {
    return (cui_drop_shadow_paint){.caster = {20, 20, 20, 12},
                                   .layer = {.sigma = 3, .color = {200, 100, 50, 128}}};
}
static void numerical(void) {
    cui_drop_shadow_paint p = paint();
    float a, b;
    /* Independent exact rectangle convolution: erf CDF in both axes. The
     * pinned four-Y-sample approximation is bounded against this reference. */
    for (int y = 12; y < 40; y++)
        for (int x = 12; x < 48; x++) {
            assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, x + .5f, y + .5f, &a));
            double sx = p.layer.sigma * sqrt(2.0);
            double gx = .5 * (erf((40 - x - .5) / sx) - erf((20 - x - .5) / sx));
            double gy = .5 * (erf((32 - y - .5) / sx) - erf((20 - y - .5) / sx));
            assert(fabs(a - gx * gy) < .025);
        }
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 10.5f, 24.5f, &a) && a == 0);
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 18.5f, 24.5f, &a) && a > .2f);
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 41.5f, 24.5f, &b) && fabsf(a - b) < 1e-6f);
    p.layer.sigma = 0;
    p.layer.spread = 1;
    p.radii = (cui_corner_radii){5, 5, 5, 5};
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 19.5f, 25.5f, &a) && a == 1);
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 19.5f, 19.5f, &a) && a == 0);
    p.radii = (cui_corner_radii){0};
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 19.5f, 19.5f, &a) && a == 1);
}
static void atomic(void) {
    cui_drop_shadow_paint p = paint();
    cui_drop_shadow_raster q;
    float a_dummy;
    assert(cui_effect_drop_shadow_query(p, 2, .25f, .75f, &q));
    assert(q.width == 77 && q.height == 61);
    cui_drop_shadow_raster unchanged = q;
    p.layer.sigma = NAN;
    assert(!cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
    assert(q.width == unchanged.width && q.bounds.x == unchanged.bounds.x);
    unsigned char bytes[32];
    memset(bytes, 123, sizeof(bytes));
    p = paint();
    assert(!cui_effect_drop_shadow_raster(p, 1, 0, 0, (cui_effect_buffer){bytes, 1, 1, 4, 4}));
    for (size_t i = 0; i < sizeof(bytes); i++)
        assert(bytes[i] == 123);
    p.layer.spread = -10;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q) && !q.width);
    p = paint();
    p.layer.sigma = 1e-10f;
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 20.5f, 20.5f, &a_dummy));
    p.layer.sigma = nextafterf(0, 1);
    a_dummy = 123;
    assert(!cui_effect_drop_shadow_sample(p, 1, 0, 0, 20.5f, 20.5f, &a_dummy) && a_dummy == 123);
    p = paint();
    p.caster.x = 16384;
    p.caster.y = -16384;
    p.layer.sigma = .0001f;
    assert(cui_effect_drop_shadow_sample(p, 64, 1048576, -1048576, 2097152.5f, -2097152.5f,
                                         &a_dummy) &&
           isfinite(a_dummy));
    p = paint();
    p.caster.w = 16384;
    p.caster.h = 16384;
    assert(!cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
}
static void raster_pixels(void) {
    cui_drop_shadow_paint p = paint();
    cui_drop_shadow_raster q;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
    size_t stride = (size_t)q.width * 4 + 8, bytes = stride * q.height;
    unsigned char *pixels = malloc(bytes);
    assert(pixels);
    memset(pixels, 123, bytes);
    assert(cui_effect_drop_shadow_raster(
        p, 1, 0, 0, (cui_effect_buffer){pixels, q.width, q.height, stride, bytes}));
    for (unsigned y = 0; y < q.height; y++) {
        for (unsigned x = 0; x < q.width; x++) {
            const unsigned char *pixel = pixels + y * stride + (size_t)x * 4;
            double sx = p.layer.sigma * sqrt(2.0), px = q.bounds.x + x + .5,
                   py = q.bounds.y + y + .5;
            double gx = .5 * (erf((40 - px) / sx) - erf((20 - px) / sx));
            double gy = .5 * (erf((32 - py) / sx) - erf((20 - py) / sx));
            assert(pixel[0] == 200 && pixel[1] == 100 && pixel[2] == 50);
            assert(fabs(pixel[3] - 128 * gx * gy) < 4);
        }
        for (size_t x = (size_t)q.width * 4; x < stride; x++)
            assert(pixels[y * stride + x] == 123);
    }
    free(pixels);
}
int main(void) {
    numerical();
    atomic();
    raster_pixels();
    return 0;
}
