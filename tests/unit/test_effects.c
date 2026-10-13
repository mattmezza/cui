/* SPDX-License-Identifier: MIT */
#include "cui/cui_effects.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void gradients(void) {
    unsigned char p[32];
    memset(p, 77, sizeof(p));
    cui_effect_buffer d = {p, 3, 2, 16, sizeof(p)};
    cui_gradient_stop s[] = {
        {0, {255, 0, 0, 255}}, {0.5f, {0, 255, 0, 255}}, {1, {0, 0, 255, 255}}};
    assert(cui_effect_linear_gradient(d, s, 3, 0));
    assert(p[0] == 255 && p[5] == 255 && p[10] == 255 && p[12] == 77);
    assert(cui_effect_linear_gradient(d, s, 3, 180));
    assert(p[2] == 255 && p[8] == 255);
    assert(cui_effect_linear_gradient(d, s, 3, 90));
    assert(p[0] == 255 && p[18] == 255);
    assert(cui_effect_linear_gradient(d, s, 3, -90));
    assert(p[2] == 255 && p[16] == 255);
    assert(cui_effect_linear_gradient(d, s, 3, 45));
    assert(p[0] == 255 && p[26] == 255);
    unsigned char one[4];
    assert(cui_effect_linear_gradient((cui_effect_buffer){one, 1, 1, 4, 4}, s, 3, 45));
    assert(one[0] == 255 && one[3] == 255);
    s[1].position = 0;
    unsigned char saved[32];
    memcpy(saved, p, sizeof(p));
    assert(!cui_effect_linear_gradient(d, s, 3, 0));
    assert(!cui_effect_linear_gradient(d, s, 3, NAN));
    assert(!memcmp(saved, p, sizeof(p)));
    cui_gradient_stop alpha[] = {{0, {255, 0, 0, 255}}, {1, {0, 0, 255, 0}}};
    assert(cui_effect_linear_gradient(d, alpha, 2, 0));
    assert(p[4] == 255 && p[6] == 0 && p[7] == 128 && p[11] == 0);
    alpha[0].position = .25f;
    alpha[1].position = .75f;
    assert(cui_effect_linear_gradient(d, alpha, 2, 0));
    assert(p[0] == 255 && p[11] == 0);
    assert(!cui_effect_linear_gradient((cui_effect_buffer){p, 3, 2, 16, 27}, alpha, 2, 0));
}
static void blur(void) {
    unsigned char p[36] = {0}, q[48];
    float scratch[36];
    for (unsigned i = 0; i < 9; ++i)
        p[i * 4 + 3] = 255;
    p[16] = 255;
    cui_bitmap s = {p, 3, 3, 12, 0};
    cui_effect_buffer d = {q, 3, 3, 16, sizeof(q)};
    memset(q, 99, sizeof(q));
    assert(cui_effect_blur(s, d, 1, scratch, 36));
    for (unsigned y = 0; y < 3; ++y)
        for (unsigned x = 0; x < 3; ++x) {
            assert(q[y * 16 + x * 4] == 28);
            assert(q[y * 16 + x * 4 + 3] == 255);
        }
    assert(q[12] == 99 && q[28] == 99);
    unsigned char saved[48];
    memcpy(saved, q, sizeof(q));
    assert(!cui_effect_blur(s, d, 1, scratch, 35));
    assert(!cui_effect_blur(s, (cui_effect_buffer){q, 3, 3, 11, 48}, 1, scratch, 36));
    assert(!cui_effect_blur(s, d, 1, (float *)q, 36));
    assert(!memcmp(q, saved, sizeof(q)));
    for (unsigned i = 0; i < 9; ++i) {
        p[i * 4] = 20;
        p[i * 4 + 1] = 80;
        p[i * 4 + 2] = 140;
        p[i * 4 + 3] = 120;
    }
    assert(cui_effect_blur(s, d, UINT_MAX, scratch, 36));
    assert(q[0] == 20 && q[1] == 80 && q[2] == 140 && q[3] == 120);
    unsigned char halo[] = {255, 0, 0, 255, 0, 0, 255, 0, 0, 0, 255, 0};
    float hs[12];
    unsigned char ho[12];
    cui_bitmap h = {halo, 3, 1, 12, 0};
    cui_effect_buffer hd = {ho, 3, 1, 12, 12};
    assert(cui_effect_blur(h, hd, 1, hs, 12));
    assert(ho[0] == 255 && ho[2] == 0 && ho[3] == 170);
    assert(ho[4] == 255 && ho[6] == 0 && ho[7] == 85);
    assert(ho[8] == 0 && ho[11] == 0);
    assert(cui_effect_blur(h, (cui_effect_buffer){halo, 3, 1, 12, 12}, 0, hs, 12));
    assert(halo[6] == 255);
    assert(cui_effect_blur(h, (cui_effect_buffer){halo, 3, 1, 12, 12}, 1, hs, 12));
    assert(halo[4] == 255 && halo[7] == 85);
    unsigned char stable[48];
    memcpy(stable, q, sizeof(q));
    assert(!cui_effect_blur((cui_bitmap){p, 3, 3, SIZE_MAX, 0}, d, 1, scratch, 36));
    assert(!cui_effect_blur(s, (cui_effect_buffer){q, 3, 3, 16, 43}, 1, scratch, 36));
    assert(!memcmp(stable, q, sizeof(q)));
    assert(cui_effect_blur_scratch_floats(0, 1) == 0);
    assert(cui_effect_blur_scratch_floats(UINT_MAX, UINT_MAX) == 0);
}
static void opacity(void) {
    unsigned char p[] = {10, 20, 30, 255, 1, 2, 3, 100};
    cui_bitmap s = {p, 2, 1, 8, 0};
    cui_effect_buffer d = {p, 2, 1, 8, 8};
    assert(cui_effect_opacity(s, d, .5f));
    assert(p[0] == 10 && p[3] == 128 && p[7] == 50);
    unsigned char saved[8];
    memcpy(saved, p, 8);
    assert(!cui_effect_opacity(s, d, NAN));
    assert(!cui_effect_opacity(s, d, -1));
    assert(!cui_effect_opacity(s, (cui_effect_buffer){p + 1, 2, 1, 8, 8}, 1));
    assert(!memcmp(p, saved, 8));
    assert(cui_effect_opacity(s, d, 0));
    assert(p[0] == 10 && p[3] == 0);
}
int main(void) {
    gradients();
    blur();
    opacity();
    puts("effects tests passed");
    return 0;
}
