/* SPDX-License-Identifier: MIT */
#define CUI_SKELETON_PALETTE_IMPLEMENTATION
#ifdef CUI_SKELETON_PALETTE_UMBRELLA
#include "cui_all.h"
#else
#include "cui_skeleton_palette.h"
#endif
#include <assert.h>
#include <stdint.h>
int main(void) {
    cui_skeleton_palette palette;
    cui_theme theme = cui_theme_light();
    assert(cui_skeleton_palette_from_theme(&theme, &palette));
    assert(palette.background.r == theme.secondary.r);
    assert(cui_skeleton_palette_preset_get(0, &palette));
    assert(palette.background.r == 245 && palette.background.g == 245 &&
           palette.background.b == 245 && palette.background.a == 255);
    assert(!cui_skeleton_palette_preset_get(SIZE_MAX, &palette));
    return 0;
}
