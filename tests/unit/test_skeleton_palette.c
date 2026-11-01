/* SPDX-License-Identifier: MIT */
#include "cui/cui_skeleton_palette.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void color_equal(cui_color a, cui_color b) {
    assert(a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
}
int main(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_skeleton_palette p;
    for (size_t i = 0; i < sizeof themes / sizeof *themes; ++i) {
        assert(cui_skeleton_palette_from_theme(&themes[i], &p));
        color_equal(p.background, themes[i].secondary);
    }
    cui_theme custom = cui_theme_light();
    custom.secondary = (cui_color){21, 34, 55, 0};
    assert(cui_skeleton_palette_from_theme(&custom, &p));
    color_equal(p.background, custom.secondary);
    /* Even preset-looking custom values never trigger theme identity inference. */
    assert(cui_theme_preset_get(0, &custom));
    assert(cui_skeleton_palette_from_theme(&custom, &p));
    color_equal(p.background, custom.secondary);
    unsigned char saved[sizeof p];
    memcpy(saved, &p, sizeof p);
    assert(!cui_skeleton_palette_from_theme(NULL, &p));
    assert(!memcmp(saved, &p, sizeof p));
    assert(!cui_skeleton_palette_from_theme(&custom, NULL));
    assert(!cui_skeleton_palette_preset_get(cui_theme_preset_count(), &p));
    assert(!memcmp(saved, &p, sizeof p));
    assert(!cui_skeleton_palette_preset_get((size_t)-1, &p));
    assert(!memcmp(saved, &p, sizeof p));
    assert(!cui_skeleton_palette_preset_get(0, NULL));
    for (size_t i = 0; i < cui_theme_preset_count(); ++i) {
        assert(cui_skeleton_palette_preset_get(i, &p));
        printf("%zu %u %u %u %u\n", i, p.background.r, p.background.g, p.background.b,
               p.background.a);
    }
    return 0;
}
