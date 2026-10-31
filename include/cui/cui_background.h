/* SPDX-License-Identifier: MIT */
#ifndef CUI_BACKGROUND_H
#define CUI_BACKGROUND_H
#include "cui.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Unreleased, allocation-free owned values only: no element setter, draw command,
 * renderer support or background rendering is implied by this header. */
typedef struct {
    float r, g, b, a; /* Straight LINEAR sRGB, finite [0,1], including RGB at a=0. */
} cui_background_color;
typedef enum { CUI_BACKGROUND_SOLID, CUI_BACKGROUND_LINEAR } cui_background_kind;
typedef enum { CUI_BACKGROUND_CSS, CUI_BACKGROUND_SOURCE_038 } cui_background_model;
typedef enum { CUI_BACKGROUND_SRGB, CUI_BACKGROUND_OKLAB } cui_background_space;
typedef struct {
    cui_background_color color;
    float position; /* [0,1]; CSS ordered, SOURCE_038 may reverse. */
} cui_background_stop;
typedef struct {
    cui_background_kind kind;
    cui_background_color solid;
    cui_background_model model;
    cui_background_space space;
    float angle_degrees; /* [0,360]: 0 up, 90 right; 360 equals 0. */
    cui_background_stop stops[2];
} cui_background;
/* Float sampler output is premultiplied LINEAR sRGB. Oklab out-of-gamut RGB
 * remains unclamped (may be negative or exceed alpha); alpha stays [0,1]. */
typedef struct {
    float r, g, b, a;
} cui_background_pixel;
/* Encoded sRGBA8 is explicitly decoded, without premultiplication/quantization. */
cui_background_color cui_background_color_from_srgba8(cui_color color);
/* Normalized source HSLA uses the SOURCE_038 shader conversion: resulting RGB
 * is treated as linear, with NO additional sRGB decode. Inputs finite [0,1]. */
bool cui_background_color_from_source_038_hsla(float h, float s, float l, float a,
                                               cui_background_color *out);
/* Checked constructors copy all active values; inactive fields are canonical zero.
 * Validation ignores inactive fields: SOLID ignores model/space/angle/stops,
 * LINEAR ignores solid. All bool functions preserve output on invalid input;
 * Constructors accept input values drawn from their output object.
 * No context/error state, allocation or borrowing. */
bool cui_background_make_solid(cui_background_color color, cui_background *out);
bool cui_background_make_linear(cui_background_model model, cui_background_space space,
                                float angle_degrees, cui_background_stop first,
                                cui_background_stop second, cui_background *out);
bool cui_background_valid(const cui_background *background);
/* Frame finite x/y in [-1e6,1e6], positive w/h <=1e6, finite point x/y in
 * [-1e6,1e6]. Coordinates are logical; no pixel-center offset, clipping, radius,
 * opacity or target ownership. Outer stop colors extend beyond their interval.
 * CSS uses full diagonal projection and alpha-premultiplied interpolation in the
 * selected space; SOURCE_038 uses short-axis/dominant-axis projection and straight
 * selected-space interpolation with independent alpha. Equal stops switch to the
 * second at/after their position, including reversed SOURCE_038 intervals. */
bool cui_background_sample(const cui_background *background, cui_rect frame, float x, float y,
                           cui_background_pixel *out);
/* Same sample, converted to straight encoded sRGBA8: unpremultiply linear RGB,
 * clip each channel to [0,1], encode sRGB, round half upward to nearest byte;
 * alpha rounds independently. Exact zero alpha returns transparent black. */
bool cui_background_sample_srgba8(const cui_background *background, cui_rect frame, float x,
                                  float y, cui_color *out);
#ifdef __cplusplus
}
#endif
#endif
