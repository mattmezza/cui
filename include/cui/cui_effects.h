/* SPDX-License-Identifier: MIT */
#ifndef CUI_EFFECTS_H
#define CUI_EFFECTS_H
#include "cui.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Caller-owned writable straight-alpha RGBA8 storage; size includes padding. */
typedef struct {
    unsigned char *pixels;
    unsigned width, height;
    size_t stride, size;
} cui_effect_buffer;
typedef struct {
    float position; /* Strictly increasing, in [0,1]. */
    cui_color color;
} cui_gradient_stop;
/* Angle in degrees: 0 runs left to right, 90 top to bottom. Endpoints span
 * the projected pixel-center bounds. Single-pixel images sample position 0.
 * Interpolates premultiplied sRGB channels, then stores straight alpha.
 * Stops may omit 0/1: outer colors extend to the endpoints. */
bool cui_effect_linear_gradient(cui_effect_buffer output, const cui_gradient_stop *stops,
                                size_t count, float angle_degrees);
/* Preserves straight RGB; scales alpha. Source/destination may be identical,
 * otherwise their memory ranges must not overlap. Finite opacity in [0,1]. */
bool cui_effect_opacity(cui_bitmap source, cui_effect_buffer output, float opacity);
/* Separable box blur, radius in pixels, replicated edge samples. Processes
 * premultiplied RGBA internally to prevent transparent color halos. Scratch
 * requires width*height*4 floats and must overlap neither image. Source and
 * destination may be identical. No allocation. Invalid calls leave output
 * untouched. Radius 0 copies the source. */
size_t cui_effect_blur_scratch_floats(unsigned width, unsigned height);
bool cui_effect_blur(cui_bitmap source, cui_effect_buffer output, unsigned radius, float *scratch,
                     size_t scratch_floats);
#ifdef __cplusplus
}
#endif
#endif
