/* SPDX-License-Identifier: MIT */
#ifndef CUI_DROP_SHADOW_BOUNDS_INTERNAL_H
#define CUI_DROP_SHADOW_BOUNDS_INTERNAL_H
/* Private value-only geometry shared by Core and standalone Effects.
 * The including translation unit supplies the public CUI value types. */
#include <math.h>
/* Preserve source Bounds::dilate materialization and f32 operation order.
 * Covering uses this stored origin plus stored size, not an algebraic rewrite. */
static inline cui_rect cui_drop_shadow_logical_bounds(cui_drop_shadow_paint p) {
    return (cui_rect){(p.caster.x + p.layer.offset_x) - p.layer.spread,
                      (p.caster.y + p.layer.offset_y) - p.layer.spread,
                      p.caster.w + (p.layer.spread + p.layer.spread),
                      p.caster.h + (p.layer.spread + p.layer.spread)};
}
static inline bool cui_drop_shadow_bounds_checked(cui_drop_shadow_paint p, cui_rect *out) {
    const float values[] = {p.caster.x,        p.caster.y,          p.caster.w,
                            p.caster.h,        p.layer.offset_x,    p.layer.offset_y,
                            p.layer.sigma,     p.layer.spread,      p.radii.top_left,
                            p.radii.top_right, p.radii.bottom_left, p.radii.bottom_right};
    if (!out)
        return false;
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
        if (!isfinite(values[i]))
            return false;
    if (fabsf(p.caster.x) > 16384 || fabsf(p.caster.y) > 16384 || p.caster.w < 0 ||
        p.caster.h < 0 || p.caster.w > 16384 || p.caster.h > 16384 ||
        fabsf(p.layer.offset_x) > 4096 || fabsf(p.layer.offset_y) > 4096 ||
        fabsf(p.layer.spread) > 4096 || p.layer.sigma < 0 || p.layer.sigma > 256 ||
        p.radii.top_left < 0 || p.radii.top_right < 0 || p.radii.bottom_left < 0 ||
        p.radii.bottom_right < 0 || p.radii.top_left > 16384 || p.radii.top_right > 16384 ||
        p.radii.bottom_left > 16384 || p.radii.bottom_right > 16384)
        return false;
    cui_rect logical = cui_drop_shadow_logical_bounds(p);
    float margin = 3 * p.layer.sigma + 1;
    /* Metadata is conservative logical support, not the checked physical
     * covering. Even a negative logical size can cover a physical pixel. */
    cui_rect next = {logical.x - margin, logical.y - margin, fmaxf(logical.w, 0) + 2 * margin,
                     fmaxf(logical.h, 0) + 2 * margin};
    *out = next;
    return true;
}
static inline bool cui_inset_shadow_bounds_checked(cui_inset_shadow_paint p, cui_rect *out) {
    cui_rect ignored;
    if (!out || !cui_drop_shadow_bounds_checked(p, &ignored))
        return false;
    p.layer.offset_x = p.layer.offset_y = p.layer.spread = p.layer.sigma = 0;
    return cui_drop_shadow_bounds_checked(p, out);
}
#endif
