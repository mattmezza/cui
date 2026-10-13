/* SPDX-License-Identifier: MIT */
#include "cui/cui_effects.h"
#include <math.h>
#include <stdlib.h>
static float values[16];
static cui_drop_shadow_raster raster;
static unsigned char *pixels;
static size_t capacity;
float *cui_canvas_shadow_values(void) {
    return values;
}
unsigned char *cui_canvas_shadow_pixels(void) {
    return pixels;
}
unsigned cui_canvas_shadow_width(void) {
    return raster.width;
}
unsigned cui_canvas_shadow_height(void) {
    return raster.height;
}
float cui_canvas_shadow_bound(unsigned i) {
    const float b[] = {raster.bounds.x, raster.bounds.y, raster.bounds.w, raster.bounds.h};
    return i < 4 ? b[i] : 0;
}
float cui_canvas_shadow_layer_bound(unsigned i) {
    const float b[] = {raster.covered_layer.x, raster.covered_layer.y, raster.covered_layer.w,
                       raster.covered_layer.h};
    return i < 4 ? b[i] : 0;
}
static bool bridge_paint(cui_drop_shadow_paint *out) {
    for (unsigned i = 12; i < 16; i++)
        if (!isfinite(values[i]) || values[i] < 0 || values[i] > 255 ||
            floorf(values[i]) != values[i])
            return false;
    *out =
        (cui_drop_shadow_paint){.caster = {values[0], values[1], values[2], values[3]},
                                .radii = {values[4], values[5], values[6], values[7]},
                                .layer = {values[8],
                                          values[9],
                                          values[10],
                                          values[11],
                                          {(unsigned char)values[12], (unsigned char)values[13],
                                           (unsigned char)values[14], (unsigned char)values[15]}}};
    return true;
}
int cui_canvas_shadow_query(float scale, float ox, float oy) {
    cui_drop_shadow_paint p;
    cui_drop_shadow_raster next;
    if (!bridge_paint(&p) || !cui_effect_drop_shadow_query(p, scale, ox, oy, &next))
        return 0;
    raster = next;
    return 1;
}
int cui_canvas_shadow_prepare(float scale, float ox, float oy) {
    cui_drop_shadow_paint p;
    cui_drop_shadow_raster next;
    if (!bridge_paint(&p) || !cui_effect_drop_shadow_query(p, scale, ox, oy, &next) ||
        next.bytes > 32u * 1024u * 1024u)
        return 0;
    if (next.bytes > capacity) {
        void *new_pixels = realloc(pixels, next.bytes);
        if (!new_pixels)
            return 0;
        pixels = new_pixels;
        capacity = next.bytes;
    }
    if (!cui_effect_drop_shadow_raster(p, scale, ox, oy,
                                       (cui_effect_buffer){pixels, next.width, next.height,
                                                           (size_t)next.width * 4, next.bytes}))
        return 0;
    raster = next;
    return 1;
}
