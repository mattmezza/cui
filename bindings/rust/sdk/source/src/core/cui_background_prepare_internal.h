/* SPDX-License-Identifier: MIT */
#ifndef CUI_BACKGROUND_PREPARE_INTERNAL_H
#define CUI_BACKGROUND_PREPARE_INTERNAL_H
#include "cui/cui_draw_v2.h"
#include "cui_background_output_internal.h"
#include <math.h>
#include <stdint.h>
/* Private allocation-free policy. Not a renderer or capability advertisement.
 * Owned draw-by-value; no caches, caller pointer retention, target/upload handles.
 * Query/out/options and the WHOLE supplied pixel capacity must be disjoint.
 * Pointer validity/allocated object extent remain the caller's ordinary C contract.
 * Failure leaves query and every buffer byte unchanged. Empty work canonicalizes
 * ALL query fields to zero; NULL pixels accepted only with zero capacity there.
 * Query reports packed minimum stride/bytes; raster reports ACTUAL supplied
 * stride and validated CPU span. Row padding is never written. The caller supplies actual
 * stride/capacity; packed upload bytes and actual CPU span are checked separately. */
typedef struct {
    int32_t physical_x, physical_y;
    unsigned width, height;
    size_t stride, bytes;
    cui_rect logical_covered;
} cui_background_prepared_query;
static inline bool cui_background_prepare_disjoint_internal(const void *a, size_t na, const void *b,
                                                            size_t nb) {
    if (!na || !nb)
        return true;
    if (!a || !b)
        return false;
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    if (na > UINTPTR_MAX - x || nb > UINTPTR_MAX - y)
        return false;
    return x + na <= y || y + nb <= x;
}
static inline bool cui_background_prepare_clip_valid_internal(cui_rect r) {
    return isfinite(r.x) && isfinite(r.y) && isfinite(r.w) && isfinite(r.h) && r.w >= 0 && r.h >= 0;
}
static inline cui_error cui_background_prepare_query(cui_background_draw_v2 draw,
                                                     const cui_render_v2_options *options,
                                                     cui_rect clip,
                                                     cui_background_prepared_query *out) {
    if (!options || !out ||
        !cui_background_prepare_disjoint_internal(options, sizeof(*options), out, sizeof(*out)))
        return CUI_ERROR_INVALID_ACTION;
    cui_render_v2_options o = *options;
    if (o.size != sizeof(o) || o.background_output != CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1 ||
        !isfinite(o.scale_x) || !isfinite(o.scale_y) || o.scale_x < 1.f / 65536 || o.scale_x > 64 ||
        o.scale_y < 1.f / 65536 || o.scale_y > 64 || !isfinite(o.origin_x) ||
        !isfinite(o.origin_y) || fabsf(o.origin_x) > 1e6f || fabsf(o.origin_y) > 1e6f ||
        !o.max_pixels || !o.max_pixels_per_background || !o.cpu_bytes || !o.texture_bytes ||
        !o.transfer_bytes || !o.metadata_bytes || !cui_background_valid(&draw.brush) ||
        !cui_background_prepare_clip_valid_internal(draw.bounds) || fabsf(draw.bounds.x) > 1e6f ||
        fabsf(draw.bounds.y) > 1e6f || draw.bounds.w > 1e6f || draw.bounds.h > 1e6f ||
        !isfinite(draw.radii.top_left) || !isfinite(draw.radii.top_right) ||
        !isfinite(draw.radii.bottom_left) || !isfinite(draw.radii.bottom_right) ||
        !isfinite(draw.opacity) || draw.opacity < 0 || draw.opacity > 1 ||
        !cui_background_prepare_clip_valid_internal(clip))
        return CUI_ERROR_INVALID_ACTION;
    if (o.viewport_width > 16384 || o.viewport_height > 16384 ||
        o.max_pixels > (size_t)8 * 1024 * 1024 ||
        o.max_pixels_per_background > (size_t)4 * 1024 * 1024 ||
        o.cpu_bytes > (size_t)32 * 1024 * 1024 || o.texture_bytes > (size_t)32 * 1024 * 1024 ||
        o.transfer_bytes > (size_t)32 * 1024 * 1024 || o.metadata_bytes > (size_t)1024 * 1024)
        return CUI_ERROR_CAPACITY;
    cui_background_prepared_query q = {0};
    if (!o.viewport_width || !o.viewport_height || draw.bounds.w == 0 || draw.bounds.h == 0 ||
        clip.w == 0 || clip.h == 0) {
        *out = q;
        return CUI_OK;
    }
    double left = fmax(0, fmax((double)draw.bounds.x * o.scale_x + o.origin_x,
                               (double)clip.x * o.scale_x + o.origin_x));
    double top = fmax(0, fmax((double)draw.bounds.y * o.scale_y + o.origin_y,
                              (double)clip.y * o.scale_y + o.origin_y));
    double right = fmin(o.viewport_width,
                        fmin(((double)draw.bounds.x + draw.bounds.w) * o.scale_x + o.origin_x,
                             ((double)clip.x + clip.w) * o.scale_x + o.origin_x));
    double bottom = fmin(o.viewport_height,
                         fmin(((double)draw.bounds.y + draw.bounds.h) * o.scale_y + o.origin_y,
                              ((double)clip.y + clip.h) * o.scale_y + o.origin_y));
    if (!isfinite(left) || !isfinite(top) || !isfinite(right) || !isfinite(bottom))
        return CUI_ERROR_INVALID_ACTION;
    if (right <= left || bottom <= top) {
        *out = q;
        return CUI_OK;
    }
    q.physical_x = (int32_t)floor(left);
    q.physical_y = (int32_t)floor(top);
    q.width = (unsigned)ceil(right) - (unsigned)q.physical_x;
    q.height = (unsigned)ceil(bottom) - (unsigned)q.physical_y;
    /* Conservative covered-grid extrema, including cells later masked out by
     * corner/box shape. Never crop meaningful fill to hide sampler-domain error. */
    double x0 = (q.physical_x + .5 - o.origin_x) / o.scale_x;
    double y0 = (q.physical_y + .5 - o.origin_y) / o.scale_y;
    double x1 = ((double)q.physical_x + q.width - .5 - o.origin_x) / o.scale_x;
    double y1 = ((double)q.physical_y + q.height - .5 - o.origin_y) / o.scale_y;
    if (!isfinite(x0) || !isfinite(y0) || !isfinite(x1) || !isfinite(y1) || x0 < -1e6 ||
        y0 < -1e6 || x1 > 1e6 || y1 > 1e6)
        return CUI_ERROR_INVALID_ACTION;
    if (q.height > SIZE_MAX / q.width)
        return CUI_ERROR_CAPACITY;
    size_t pixels = (size_t)q.width * q.height;
    if (pixels > o.max_pixels || pixels > o.max_pixels_per_background || pixels > SIZE_MAX / 4)
        return CUI_ERROR_CAPACITY;
    q.stride = (size_t)q.width * 4;
    q.bytes = pixels * 4;
    if (q.bytes > o.cpu_bytes || q.bytes > o.texture_bytes || q.bytes > o.transfer_bytes ||
        sizeof(q) > o.metadata_bytes)
        return CUI_ERROR_CAPACITY;
    q.logical_covered =
        (cui_rect){(float)(((double)q.physical_x - o.origin_x) / o.scale_x),
                   (float)(((double)q.physical_y - o.origin_y) / o.scale_y),
                   (float)(q.width / (double)o.scale_x), (float)(q.height / (double)o.scale_y)};
    *out = q;
    return CUI_OK;
}
static inline bool cui_background_prepare_inside_internal(cui_background_draw_v2 draw, double x,
                                                          double y) {
    double left = draw.bounds.x, top = draw.bounds.y;
    double right = left + draw.bounds.w, bottom = top + draw.bounds.h;
    if (x < left || x >= right || y < top || y >= bottom)
        return false;
    double limit = fmin(draw.bounds.w, draw.bounds.h) / 2;
    double r[4] = {fmin(fmax(draw.radii.top_left, 0), limit),
                   fmin(fmax(draw.radii.top_right, 0), limit),
                   fmin(fmax(draw.radii.bottom_left, 0), limit),
                   fmin(fmax(draw.radii.bottom_right, 0), limit)};
    double cx[4] = {left + r[0], right - r[1], left + r[2], right - r[3]};
    double cy[4] = {top + r[0], top + r[1], bottom - r[2], bottom - r[3]};
    for (unsigned i = 0; i < 4; i++) {
        bool horizontal = (i % 2 == 0) ? x < cx[i] : x > cx[i];
        bool vertical = (i < 2) ? y < cy[i] : y > cy[i];
        if (r[i] > 0 && horizontal && vertical) {
            double dx = x - cx[i], dy = y - cy[i];
            if (dx * dx + dy * dy > r[i] * r[i])
                return false;
        }
    }
    return true;
}
static inline cui_error cui_background_prepare_raster(cui_background_draw_v2 draw,
                                                      const cui_render_v2_options *options,
                                                      cui_rect clip, unsigned char *rgba,
                                                      size_t stride, size_t capacity,
                                                      cui_background_prepared_query *out) {
    if (!options || !out ||
        !cui_background_prepare_disjoint_internal(options, sizeof(*options), out, sizeof(*out)) ||
        (capacity && !rgba) ||
        !cui_background_prepare_disjoint_internal(rgba, capacity, options, sizeof(*options)) ||
        !cui_background_prepare_disjoint_internal(rgba, capacity, out, sizeof(*out)))
        return CUI_ERROR_INVALID_ACTION;
    cui_render_v2_options o = *options;
    cui_background_prepared_query q;
    cui_error error = cui_background_prepare_query(draw, &o, clip, &q);
    if (error != CUI_OK)
        return error;
    if (!q.bytes) {
        *out = q;
        return CUI_OK;
    }
    if (!rgba)
        return CUI_ERROR_INVALID_ACTION;
    if (stride < q.stride || (q.height - 1) > (SIZE_MAX - q.stride) / stride)
        return CUI_ERROR_CAPACITY;
    size_t span = (q.height - 1) * stride + q.stride;
    if (span > capacity || span > o.cpu_bytes)
        return CUI_ERROR_CAPACITY;
    /* Query proved the complete numeric/sample domain and output capacity. No
     * failure path follows the first write; all remaining inputs are owned values. */
    for (unsigned row = 0; row < q.height; row++)
        for (unsigned column = 0; column < q.width; column++) {
            double x = ((double)q.physical_x + column + .5 - o.origin_x) / o.scale_x;
            double y = ((double)q.physical_y + row + .5 - o.origin_y) / o.scale_y;
            cui_color color = {0};
            if (cui_background_prepare_inside_internal(draw, x, y)) {
                cui_background_pixel p = {0};
                (void)cui_background_sample(&draw.brush, draw.bounds, (float)x, (float)y, &p);
                (void)cui_background_output_srgba8_internal(p, draw.opacity, &color);
            }
            size_t at = (size_t)row * stride + (size_t)column * 4;
            rgba[at] = color.r;
            rgba[at + 1] = color.g;
            rgba[at + 2] = color.b;
            rgba[at + 3] = color.a;
        }
    q.stride = stride;
    q.bytes = span;
    *out = q;
    return CUI_OK;
}
#endif
