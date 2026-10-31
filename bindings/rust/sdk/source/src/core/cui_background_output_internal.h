/* SPDX-License-Identifier: MIT */
#ifndef CUI_BACKGROUND_OUTPUT_INTERNAL_H
#define CUI_BACKGROUND_OUTPUT_INTERNAL_H
#include <math.h>
static inline double cui_background_output_clamp_internal(double x) {
    return fmax(0, fmin(1, x));
}
static inline double cui_background_encode_internal(double x) {
    return x <= .0031308 ? x * 12.92 : 1.055 * pow(x, 1. / 2.4) - .055;
}
/* One output policy shared by pure sampling and future preparation consumers.
 * Keep RGB/base alpha separate from ancestor opacity: multiplying premultiplied
 * floats and dividing again would introduce avoidable RGB rounding drift. */
static inline bool cui_background_output_srgba8_internal(cui_background_pixel p, float opacity,
                                                         cui_color *out) {
    if (!out || !isfinite(p.r) || !isfinite(p.g) || !isfinite(p.b) || !isfinite(p.a) || p.a < 0 ||
        p.a > 1 || !isfinite(opacity) || opacity < 0 || opacity > 1)
        return false;
    double alpha = (double)p.a * opacity;
    cui_color value = {0};
    if (alpha > 0) {
        value.r =
            (uint8_t)floor(cui_background_output_clamp_internal(cui_background_encode_internal(
                               cui_background_output_clamp_internal((double)p.r / p.a))) *
                               255 +
                           .5);
        value.g =
            (uint8_t)floor(cui_background_output_clamp_internal(cui_background_encode_internal(
                               cui_background_output_clamp_internal((double)p.g / p.a))) *
                               255 +
                           .5);
        value.b =
            (uint8_t)floor(cui_background_output_clamp_internal(cui_background_encode_internal(
                               cui_background_output_clamp_internal((double)p.b / p.a))) *
                               255 +
                           .5);
        value.a = (uint8_t)floor(alpha * 255 + .5);
    }
    *out = value;
    return true;
}
#endif
