/* SPDX-License-Identifier: MIT */
/* Pure INSET distribution must link without the Core implementation. */
#define CUI_EFFECTS_IMPLEMENTATION
#include "cui_effects.h"
#include <assert.h>
#include <math.h>
#include <string.h>
int main(void) {
    cui_inset_shadow_paint p = {.caster = {20, 20, 20, 20},
                                .layer = {.spread = 2, .color = {60, 100, 180, 128}}};
    cui_inset_shadow_raster q;
    assert(cui_effect_inset_shadow_query(p, 1, 0, 0, &q));
    assert(q.width == 20 && q.height == 20 && q.covered_hole.x == 22 && q.covered_hole.w == 16 &&
           q.covered_element.x == 20);
    float alpha = -1;
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 21.5f, 30.5f, &alpha) && alpha == 1);
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 23.5f, 30.5f, &alpha) && alpha == 0);
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 19.5f, 30.5f, &alpha) && alpha == 0);
    unsigned char bytes[20 * 88];
    memset(bytes, 123, sizeof bytes);
    cui_effect_buffer output = {bytes, 20, 20, 88, sizeof bytes};
    assert(cui_effect_inset_shadow_raster(p, 1, 0, 0, output));
    assert(bytes[10 * 88 + 1 * 4 + 3] == 128 && bytes[10 * 88 + 3 * 4 + 3] == 0);
    p.layer.sigma = 3;
    assert(cui_effect_inset_shadow_raster(p, 1, 0, 0, output));
    for (unsigned y = 0; y < 20; ++y) {
        for (unsigned x = 0; x < 20; ++x) {
            double px = 20.5 + x, py = 20.5 + y, sigma = 3 * sqrt(2.0);
            double gx = .5 * (erf((38 - px) / sigma) - erf((22 - px) / sigma));
            double gy = .5 * (erf((38 - py) / sigma) - erf((22 - py) / sigma));
            assert(fabs(bytes[y * 88 + x * 4 + 3] - 128 * (1 - gx * gy)) < 4);
        }
        for (unsigned x = 80; x < 88; ++x)
            assert(bytes[y * 88 + x] == 123);
    }
    cui_inset_shadow_raster unchanged = q;
    unsigned char before[sizeof bytes];
    memcpy(before, bytes, sizeof bytes);
    p.layer.sigma = NAN;
    alpha = 42;
    assert(!cui_effect_inset_shadow_sample(p, 1, 0, 0, 30, 30, &alpha) && alpha == 42);
    assert(!cui_effect_inset_shadow_query(p, 1, 0, 0, &q));
    assert(q.width == unchanged.width && q.covered_hole.x == unchanged.covered_hole.x);
    assert(!cui_effect_inset_shadow_raster(p, 1, 0, 0, output));
    assert(!memcmp(before, bytes, sizeof bytes));
    return 0;
}
