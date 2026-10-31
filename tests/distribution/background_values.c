/* SPDX-License-Identifier: MIT */
/* This pure standalone module needs neither Core implementation nor components. */
#define CUI_BACKGROUND_IMPLEMENTATION
#include "cui_background.h"
#include <assert.h>
int main(void) {
    cui_background bg;
    cui_color out;
    cui_background_color a = cui_background_color_from_srgba8((cui_color){255, 0, 0, 0});
    cui_background_color b = cui_background_color_from_srgba8((cui_color){0, 0, 255, 255});
    assert(cui_background_make_linear(CUI_BACKGROUND_CSS, CUI_BACKGROUND_SRGB, 90,
                                      (cui_background_stop){a, 0}, (cui_background_stop){b, 1},
                                      &bg));
    assert(cui_background_sample_srgba8(&bg, (cui_rect){0, 0, 100, 20}, 50, 10, &out));
    assert(out.r == 0 && out.g == 0 && out.b == 255 && out.a == 128);
    return 0;
}
