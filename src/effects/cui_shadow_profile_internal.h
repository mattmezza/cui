/* SPDX-License-Identifier: MIT */
/* Private copied source-profile values. No raster allocation or public API. */
#ifndef CUI_SHADOW_PROFILE_INTERNAL_H
#define CUI_SHADOW_PROFILE_INTERNAL_H
#include "../core/cui_drop_shadow_bounds_internal.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
static inline float shadow_clamp(float v, float lo, float hi) {
    return fminf(hi, fmaxf(lo, v));
}
static inline bool shadow_geometry(cui_drop_shadow_paint p, float scale, float ox, float oy,
                                   cui_rect *bounds, cui_corner_radii *radii) {
    cui_rect unused;
    if (!cui_drop_shadow_bounds_checked(p, &unused) || !isfinite(scale) || scale <= 0 ||
        scale > 64 || scale < 1.0f / 65536 ||
        (p.layer.sigma > 0 && p.layer.sigma * scale < 4 * sqrtf(FLT_MIN)) || !isfinite(ox) ||
        !isfinite(oy) || fabsf(ox) > 1048576 || fabsf(oy) > 1048576)
        return false;
    float limit = fminf(p.caster.w, p.caster.h) / 2;
    *radii = (cui_corner_radii){
        fminf(limit, p.radii.top_left) * scale, fminf(limit, p.radii.top_right) * scale,
        fminf(limit, p.radii.bottom_left) * scale, fminf(limit, p.radii.bottom_right) * scale};
    cui_rect logical = cui_drop_shadow_logical_bounds(p);
    float x = logical.x * scale + ox, y = logical.y * scale + oy;
    float far_x = (logical.x + logical.w) * scale + ox;
    float far_y = (logical.y + logical.h) * scale + oy;
    float near_x = floorf(x), near_y = floorf(y);
    *bounds = (cui_rect){near_x, near_y, fmaxf(ceilf(far_x), near_x) - near_x,
                         fmaxf(ceilf(far_y), near_y) - near_y};
    return isfinite(bounds->w) && isfinite(bounds->h);
}
static inline float shadow_erf(float v) {
    float a = fabsf(v), t = 1 + (.278393f + (.230389f + (.000972f + .078108f * a) * a) * a) * a;
    float s = v < 0 ? -1.0f : (v > 0 ? 1.0f : 0.0f);
    t *= t;
    return s - s / (t * t);
}
static inline float shadow_sample(cui_drop_shadow_paint p, float scale, cui_rect b,
                                  cui_corner_radii r, float x, float y) {
    if (b.w <= 0 || b.h <= 0)
        return 0;
    float sigma = p.layer.sigma * scale, margin = 3 * sigma;
    if (x < b.x - margin || y < b.y - margin || x >= b.x + b.w + margin || y >= b.y + b.h + margin)
        return 0;
    float hx = b.w / 2, hy = b.h / 2, px = x - b.x - hx, py = y - b.y - hy;
    float corner =
        px < 0 ? (py < 0 ? r.top_left : r.bottom_left) : (py < 0 ? r.top_right : r.bottom_right);
    if (sigma == 0) {
        float qx = fabsf(px) - hx + corner, qy = fabsf(py) - hy + corner;
        float d = sqrtf(fmaxf(qx, 0) * fmaxf(qx, 0) + fmaxf(qy, 0) * fmaxf(qy, 0)) +
                  fminf(fmaxf(qx, qy), 0) - corner;
        return shadow_clamp(.5f - d, 0, 1);
    }
    float low = py - hy, high = py + hy, start = shadow_clamp(-margin, low, high);
    float end = shadow_clamp(margin, low, high), step = (end - start) / 4,
          y_sample = start + step * .5f, alpha = 0;
    for (unsigned i = 0; i < 4; i++, y_sample += step) {
        float delta = fminf(hy - corner - fabsf(py - y_sample), 0);
        float curved = hx - corner + sqrtf(fmaxf(0, corner * corner - delta * delta));
        float factor = .7071067811865475244f / sigma;
        float integral =
            .5f * (shadow_erf((px + curved) * factor) - shadow_erf((px - curved) * factor));
        float gaussian =
            expf(-(y_sample * y_sample) / (2 * sigma * sigma)) / (2.5066282746310005f * sigma);
        alpha += integral * gaussian * step;
    }
    return shadow_clamp(alpha, 0, 1);
}
static inline bool inset_geometry(cui_inset_shadow_paint p, float scale, float ox, float oy,
                                  cui_rect *hole, cui_rect *element, cui_corner_radii *hr,
                                  cui_corner_radii *er) {
    cui_drop_shadow_paint h = p, e = p;
    h.layer.spread = -p.layer.spread;
    e.layer.offset_x = e.layer.offset_y = e.layer.spread = 0;
    if (!shadow_geometry(h, scale, ox, oy, hole, hr) ||
        !shadow_geometry(e, scale, ox, oy, element, er))
        return false;
    float limit = fminf(p.caster.w, p.caster.h) / 2;
    *hr = (cui_corner_radii){fmaxf(fminf(limit, p.radii.top_left) - p.layer.spread, 0) * scale,
                             fmaxf(fminf(limit, p.radii.top_right) - p.layer.spread, 0) * scale,
                             fmaxf(fminf(limit, p.radii.bottom_left) - p.layer.spread, 0) * scale,
                             fmaxf(fminf(limit, p.radii.bottom_right) - p.layer.spread, 0) * scale};
    return true;
}
static inline float inset_sdf(cui_rect b, cui_corner_radii r, float x, float y) {
    float px = x - (b.x + b.w / 2), py = y - (b.y + b.h / 2);
    float radius =
        px < 0 ? (py < 0 ? r.top_left : r.bottom_left) : (py < 0 ? r.top_right : r.bottom_right);
    float qx = fabsf(px) - b.w / 2 + radius, qy = fabsf(py) - b.h / 2 + radius;
    if (radius == 0)
        return fmaxf(qx, qy);
    return sqrtf(fmaxf(qx, 0) * fmaxf(qx, 0) + fmaxf(qy, 0) * fmaxf(qy, 0)) +
           fminf(fmaxf(qx, qy), 0) - radius;
}
static inline float inset_sample(cui_inset_shadow_paint p, float scale, cui_rect h, cui_rect e,
                                 cui_corner_radii hr, cui_corner_radii er, float x, float y) {
    if (h.w <= 0 || h.h <= 0 || e.w <= 0 || e.h <= 0 || x < e.x || y < e.y || x >= e.x + e.w ||
        y >= e.y + e.h)
        return 0;
    float sigma = p.layer.sigma * scale, coverage;
    if (sigma == 0)
        coverage = shadow_clamp(.5f - inset_sdf(h, hr, x, y), 0, 1);
    else {
        float hx = h.w / 2, hy = h.h / 2, px = x - (h.x + hx), py = y - (h.y + hy);
        float corner = px < 0 ? (py < 0 ? hr.top_left : hr.bottom_left)
                              : (py < 0 ? hr.top_right : hr.bottom_right);
        float start = shadow_clamp(-3 * sigma, py - hy, py + hy),
              end = shadow_clamp(3 * sigma, py - hy, py + hy);
        float step = (end - start) / 4, ys = start + step * .5f;
        coverage = 0;
        for (unsigned i = 0; i < 4; i++, ys += step) {
            float delta = fminf(hy - corner - fabsf(py - ys), 0);
            float curved = hx - corner + sqrtf(fmaxf(0, corner * corner - delta * delta));
            float factor = .7071067811865475244f / sigma;
            float low = .5f + .5f * shadow_erf((px - curved) * factor);
            float high = .5f + .5f * shadow_erf((px + curved) * factor);
            float gaussian = expf(-(ys * ys) / (2 * sigma * sigma)) / (2.5066282746310005f * sigma);
            coverage += (high - low) * gaussian * step;
        }
    }
    return shadow_clamp((1 - coverage) * shadow_clamp(.5f - inset_sdf(e, er, x, y), 0, 1), 0, 1);
}
typedef struct {
    cui_drop_shadow_paint paint;
    float scale;
    cui_rect covered;
    cui_corner_radii radii;
} cui_shadow_drop_profile_internal;
typedef struct {
    cui_inset_shadow_paint paint;
    float scale;
    cui_rect hole, element;
    cui_corner_radii hole_radii, element_radii;
} cui_shadow_inset_profile_internal;
typedef struct {
    int64_t left, top, right, bottom;
} cui_shadow_profile_support_internal;
static inline bool cui_shadow_drop_prepare_internal(cui_drop_shadow_paint p, float scale, float ox,
                                                    float oy,
                                                    cui_shadow_drop_profile_internal *out) {
    cui_shadow_drop_profile_internal next = {.paint = p, .scale = scale};
    if (!out || !shadow_geometry(p, scale, ox, oy, &next.covered, &next.radii))
        return false;
    *out = next;
    return true;
}
static inline bool cui_shadow_inset_prepare_internal(cui_inset_shadow_paint p, float scale,
                                                     float ox, float oy,
                                                     cui_shadow_inset_profile_internal *out) {
    cui_shadow_inset_profile_internal next = {.paint = p, .scale = scale};
    if (!out || !inset_geometry(p, scale, ox, oy, &next.hole, &next.element, &next.hole_radii,
                                &next.element_radii))
        return false;
    *out = next;
    return true;
}
/* Evaluators require an immutable successful prepare and finite x/y. Coverage
 * ignores paint color; byte quantization remains the caller's responsibility. */
static inline float cui_shadow_drop_eval_internal(const cui_shadow_drop_profile_internal *p,
                                                  float x, float y) {
    return shadow_sample(p->paint, p->scale, p->covered, p->radii, x, y);
}
static inline float cui_shadow_inset_eval_internal(const cui_shadow_inset_profile_internal *p,
                                                   float x, float y) {
    return inset_sample(p->paint, p->scale, p->hole, p->element, p->hole_radii, p->element_radii, x,
                        y);
}
static inline bool cui_shadow_profile_integer_internal(float v, int64_t *out) {
    double d = v;
    if (!out || !isfinite(v) || d < -0x1p63 || d >= 0x1p63 || floor(d) != d)
        return false;
    *out = (int64_t)d;
    return true;
}
static inline bool cui_shadow_profile_cover_internal(cui_rect b, float margin,
                                                     cui_shadow_profile_support_internal *out) {
    if (!out)
        return false;
    cui_shadow_profile_support_internal next = {0};
    if (b.w > 0 && b.h > 0) {
        if (!cui_shadow_profile_integer_internal(floorf(b.x - margin), &next.left) ||
            !cui_shadow_profile_integer_internal(floorf(b.y - margin), &next.top) ||
            !cui_shadow_profile_integer_internal(ceilf(b.x + b.w + margin), &next.right) ||
            !cui_shadow_profile_integer_internal(ceilf(b.y + b.h + margin), &next.bottom))
            return false;
    }
    *out = next;
    return true;
}
/* Support requires the same immutable successful prepare as eval. It describes
 * potential cells, not bitmap allocation admission, and never erases alpha0. */
static inline bool cui_shadow_drop_support_internal(const cui_shadow_drop_profile_internal *p,
                                                    cui_shadow_profile_support_internal *out) {
    return p && cui_shadow_profile_cover_internal(p->covered, 3 * (p->paint.layer.sigma * p->scale),
                                                  out);
}
static inline bool cui_shadow_inset_support_internal(const cui_shadow_inset_profile_internal *p,
                                                     cui_shadow_profile_support_internal *out) {
    if (!p || !out)
        return false;
    cui_rect b = p->element;
    if (p->hole.w <= 0 || p->hole.h <= 0)
        b = (cui_rect){0};
    return cui_shadow_profile_cover_internal(b, 0, out);
}
#endif
