/* SPDX-License-Identifier: MIT */
#ifndef CUI_BACKGROUND_VALID_INTERNAL_H
#define CUI_BACKGROUND_VALID_INTERNAL_H
#include <math.h>
static inline bool cui_background_unit_internal(float x) {
    return isfinite(x) && x >= 0 && x <= 1;
}
static inline bool cui_background_color_valid_internal(cui_background_color c) {
    return cui_background_unit_internal(c.r) && cui_background_unit_internal(c.g) &&
           cui_background_unit_internal(c.b) && cui_background_unit_internal(c.a);
}
static inline bool cui_background_value_valid_internal(const cui_background *bg) {
    if (!bg)
        return false;
    if (bg->kind == CUI_BACKGROUND_SOLID)
        return cui_background_color_valid_internal(bg->solid);
    return bg->kind == CUI_BACKGROUND_LINEAR &&
           (bg->model == CUI_BACKGROUND_CSS || bg->model == CUI_BACKGROUND_SOURCE_038) &&
           (bg->space == CUI_BACKGROUND_SRGB || bg->space == CUI_BACKGROUND_OKLAB) &&
           isfinite(bg->angle_degrees) && bg->angle_degrees >= 0 && bg->angle_degrees <= 360 &&
           cui_background_color_valid_internal(bg->stops[0].color) &&
           cui_background_color_valid_internal(bg->stops[1].color) &&
           cui_background_unit_internal(bg->stops[0].position) &&
           cui_background_unit_internal(bg->stops[1].position) &&
           (bg->model != CUI_BACKGROUND_CSS || bg->stops[0].position <= bg->stops[1].position);
}
#endif
