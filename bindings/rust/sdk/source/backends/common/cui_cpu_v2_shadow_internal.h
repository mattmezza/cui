/* SPDX-License-Identifier: MIT */
#ifndef CUI_CPU_V2_SHADOW_INTERNAL_H
#define CUI_CPU_V2_SHADOW_INTERNAL_H
// clang-format off
#include "cui/cui_draw_v2.h"

#include "../../src/effects/cui_shadow_profile_internal.h"
#include <string.h>
// clang-format on
#define CUI_CPU_V2_SHADOW_TAPS ((size_t)1048576)
#define CUI_CPU_V2_SHADOW_CELLS ((size_t)1048576)
#ifndef CUI_CPU_V2_SHADOW_SAMPLE_HOOK
#define CUI_CPU_V2_SHADOW_SAMPLE_HOOK() ((void)0)
#endif
typedef struct {
    double left, top, right, bottom;
} cui_cpu_v2_shadow_clip;
typedef struct {
    union {
        cui_shadow_drop_profile_internal drop;
        cui_shadow_inset_profile_internal inset;
    } profile;
    cui_shadow_profile_support_internal support;
    cui_cpu_v2_shadow_clip clip;
    float q, phase_x, phase_y;
    double dx, dy;
    int32_t x, y;
    unsigned width, height;
    size_t command_index, bytes, taps;
    unsigned char *pixels;
    bool inset;
} cui_cpu_v2_shadow_descriptor;
typedef struct {
    double left, top, right, bottom;
    int64_t x0, y0, x1, y1;
    size_t taps;
} cui_cpu_v2_shadow_cell;
static inline float cui_cpu_v2_shadow_phase(float q, float origin, float scale) {
    double v = (double)q * origin / scale;
    return (float)(v - floor(v));
}
static inline bool cui_cpu_v2_shadow_profile(cui_draw_type kind, cui_drop_shadow_paint p,
                                             const cui_render_v2_options *o,
                                             cui_cpu_v2_shadow_descriptor *out) {
    cui_cpu_v2_shadow_descriptor d = {0};
    d.inset = kind == CUI_DRAW_INSET_SHADOW;
    d.q = fmaxf(1, fmaxf(o->scale_x, o->scale_y));
    d.phase_x = cui_cpu_v2_shadow_phase(d.q, o->origin_x, o->scale_x);
    d.phase_y = cui_cpu_v2_shadow_phase(d.q, o->origin_y, o->scale_y);
    bool ok;
    if (d.inset)
        ok = cui_shadow_inset_prepare_internal(p, d.q, d.phase_x, d.phase_y, &d.profile.inset) &&
             cui_shadow_inset_support_internal(&d.profile.inset, &d.support);
    else
        ok = kind == CUI_DRAW_DROP_SHADOW &&
             cui_shadow_drop_prepare_internal(p, d.q, d.phase_x, d.phase_y, &d.profile.drop) &&
             cui_shadow_drop_support_internal(&d.profile.drop, &d.support);
    if (!ok)
        return false;
    d.dx = d.q / (double)o->scale_x;
    d.dy = d.q / (double)o->scale_y;
    if (!isfinite(d.dx) || !isfinite(d.dy) || !isfinite(d.dx * d.dy) || d.dx < 1 || d.dy < 1)
        return false;
    *out = d;
    return true;
}
/* All options were structurally validated. No kernel/provider/allocation calls.
 * Clip is hard center membership; footprint/normalization remain complete. */
static inline cui_error cui_cpu_v2_shadow_cell_query(const cui_cpu_v2_shadow_descriptor *d,
                                                     const cui_render_v2_options *o, int x, int y,
                                                     cui_cpu_v2_shadow_cell *out) {
    cui_cpu_v2_shadow_cell c = {0};
    double lx = (x + .5 - o->origin_x) / o->scale_x;
    double ly = (y + .5 - o->origin_y) / o->scale_y;
    if (lx < d->clip.left || lx >= d->clip.right || ly < d->clip.top || ly >= d->clip.bottom) {
        *out = c;
        return CUI_OK;
    }
    c.left = d->q * ((double)x - o->origin_x) / o->scale_x + d->phase_x;
    c.top = d->q * ((double)y - o->origin_y) / o->scale_y + d->phase_y;
    c.right = c.left + d->dx;
    c.bottom = c.top + d->dy;
    if (!isfinite(c.left) || !isfinite(c.top) || !isfinite(c.right) || !isfinite(c.bottom))
        return CUI_ERROR_INVALID_ACTION;
    double l = fmax(c.left, (double)d->support.left), t = fmax(c.top, (double)d->support.top);
    double r = fmin(c.right, (double)d->support.right),
           b = fmin(c.bottom, (double)d->support.bottom);
    if (l >= r || t >= b) {
        *out = c;
        return CUI_OK;
    }
    l = floor(l);
    t = floor(t);
    r = ceil(r);
    b = ceil(b);
    if (l < -0x1p63 || t < -0x1p63 || r >= 0x1p63 || b >= 0x1p63)
        return CUI_ERROR_INVALID_ACTION;
    c.x0 = (int64_t)l;
    c.y0 = (int64_t)t;
    c.x1 = (int64_t)r;
    c.y1 = (int64_t)b;
    double centers[] = {l + .5, r - .5, t + .5, b - .5};
    for (size_t i = 0; i < 4; ++i)
        if ((double)(float)centers[i] != centers[i] ||
            (float)centers[i] == (float)(centers[i] + (i & 1 ? -1 : 1)))
            return CUI_ERROR_INVALID_ACTION;
    uint64_t nx = (uint64_t)(c.x1 - c.x0), ny = (uint64_t)(c.y1 - c.y0);
    if (nx > CUI_CPU_V2_SHADOW_TAPS || ny > CUI_CPU_V2_SHADOW_TAPS / nx)
        return CUI_ERROR_CAPACITY;
    c.taps = (size_t)(nx * ny);
    *out = c;
    return CUI_OK;
}
static inline cui_error cui_cpu_v2_shadow_query(cui_draw_type kind, cui_drop_shadow_paint p,
                                                const cui_render_v2_options *o,
                                                cui_cpu_v2_shadow_clip clip, size_t cell_limit,
                                                size_t tap_limit,
                                                cui_cpu_v2_shadow_descriptor *out) {
    cui_cpu_v2_shadow_descriptor d;
    if (cell_limit > CUI_CPU_V2_SHADOW_CELLS || tap_limit > CUI_CPU_V2_SHADOW_TAPS)
        return CUI_ERROR_CAPACITY;
    if (!cui_cpu_v2_shadow_profile(kind, p, o, &d))
        return CUI_ERROR_INVALID_ACTION;
    d.clip = clip;
    if (p.layer.color.a && d.support.left < d.support.right && d.support.top < d.support.bottom &&
        clip.left < clip.right && clip.top < clip.bottom) {
        double left =
            fmax(0, fmax((d.support.left - (double)d.phase_x) * o->scale_x / d.q + o->origin_x,
                         ceil(clip.left * o->scale_x + o->origin_x - .5)));
        double top =
            fmax(0, fmax((d.support.top - (double)d.phase_y) * o->scale_y / d.q + o->origin_y,
                         ceil(clip.top * o->scale_y + o->origin_y - .5)));
        double right =
            fmin(o->viewport_width,
                 fmin((d.support.right - (double)d.phase_x) * o->scale_x / d.q + o->origin_x,
                      ceil(clip.right * o->scale_x + o->origin_x - .5)));
        double bottom =
            fmin(o->viewport_height,
                 fmin((d.support.bottom - (double)d.phase_y) * o->scale_y / d.q + o->origin_y,
                      ceil(clip.bottom * o->scale_y + o->origin_y - .5)));
        if (!isfinite(left) || !isfinite(top) || !isfinite(right) || !isfinite(bottom))
            return CUI_ERROR_INVALID_ACTION;
        if (left < right && top < bottom) {
            d.x = (int32_t)floor(left);
            d.y = (int32_t)floor(top);
            d.width = (unsigned)ceil(right) - (unsigned)d.x;
            d.height = (unsigned)ceil(bottom) - (unsigned)d.y;
            if (d.height > cell_limit / d.width)
                return CUI_ERROR_CAPACITY;
            d.bytes = (size_t)d.width * d.height;
            for (unsigned y = 0; y < d.height; ++y)
                for (unsigned x = 0; x < d.width; ++x) {
                    cui_cpu_v2_shadow_cell c;
                    cui_error e =
                        cui_cpu_v2_shadow_cell_query(&d, o, d.x + (int)x, d.y + (int)y, &c);
                    if (e != CUI_OK)
                        return e;
                    if (c.taps > tap_limit - d.taps)
                        return CUI_ERROR_CAPACITY;
                    d.taps += c.taps;
                }
        }
    }
    *out = d;
    return CUI_OK;
}
/* Writable A8 plane is owned by counted workspace, disjoint from immutable
 * descriptor/options. Errors can affect private plane only, never the target. */
static inline cui_error cui_cpu_v2_shadow_raster(const cui_cpu_v2_shadow_descriptor *d,
                                                 const cui_render_v2_options *o,
                                                 unsigned char *pixels, size_t capacity) {
    if (capacity != d->bytes || (!pixels && capacity))
        return CUI_ERROR_CAPACITY;
    size_t taps = 0;
    for (unsigned y = 0; y < d->height; ++y)
        for (unsigned x = 0; x < d->width; ++x) {
            cui_cpu_v2_shadow_cell c;
            cui_error e = cui_cpu_v2_shadow_cell_query(d, o, d->x + (int)x, d->y + (int)y, &c);
            if (e != CUI_OK)
                return e;
            double total = 0;
            for (int64_t v = c.y0; v < c.y1; ++v)
                for (int64_t u = c.x0; u < c.x1; ++u) {
                    CUI_CPU_V2_SHADOW_SAMPLE_HOOK();
                    float a = d->inset
                                  ? cui_shadow_inset_eval_internal(&d->profile.inset,
                                                                   (float)(u + .5), (float)(v + .5))
                                  : cui_shadow_drop_eval_internal(&d->profile.drop, (float)(u + .5),
                                                                  (float)(v + .5));
                    if (!isfinite(a) || a < 0 || a > 1)
                        return CUI_ERROR_INVALID_ACTION;
                    double wx = fmax(0, fmin(c.right, u + 1.) - fmax(c.left, (double)u));
                    double wy = fmax(0, fmin(c.bottom, v + 1.) - fmax(c.top, (double)v));
                    total += lroundf(a * 255) * wx * wy;
                    ++taps;
                }
            float alpha = d->inset ? d->profile.inset.paint.layer.color.a
                                   : d->profile.drop.paint.layer.color.a;
            long byte = lroundf((float)(total / (d->dx * d->dy)) * (alpha / 255.f));
            if (byte < 0 || byte > 255)
                return CUI_ERROR_INVALID_ACTION;
            pixels[(size_t)y * d->width + x] = (unsigned char)byte;
        }
    return taps == d->taps ? CUI_OK : CUI_ERROR_INVALID_ACTION;
}
#endif
