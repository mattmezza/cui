/* SPDX-License-Identifier: MIT */
#define CUI_SLIDER_PALETTE_IMPLEMENTATION
#ifdef CUI_SLIDER_PALETTE_UMBRELLA
#include "cui_all.h"
#else
#include "cui_slider_palette.h"
#endif
#include <assert.h>
#include <stdint.h>
int main(void) {
    cui_slider_palette palette;
    assert(cui_slider_palette_preset_count() == 38);
    assert(cui_slider_palette_preset_get(0, &palette));
    assert(cui_slider_palette_valid(&palette));
    assert(cui_slider_palette_builtin_get(CUI_SLIDER_PALETTE_DARK, &palette));
    assert(!cui_slider_palette_preset_get(SIZE_MAX, &palette));
    return 0;
}
