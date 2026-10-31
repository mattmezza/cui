/* SPDX-License-Identifier: MIT */
#include "cui/cui_slider_style.h"
#include <math.h>
#include <string.h>
static bool cui_slider_style_nonnegative(float x) {
    return isfinite(x) && x >= 0;
}
static bool cui_slider_style_color_valid(cui_slider_palette_color c) {
    return isfinite(c.r) && c.r >= 0 && c.r <= 1 && isfinite(c.g) && c.g >= 0 && c.g <= 1 &&
           isfinite(c.b) && c.b >= 0 && c.b <= 1 && isfinite(c.a) && c.a >= 0 && c.a <= 1;
}
static cui_background_color cui_slider_style_color(cui_slider_palette_color c) {
    return (cui_background_color){c.r, c.g, c.b, c.a};
}
bool cui_slider_style_resolve_v1(const cui_slider_style_options_v1 *o,
                                 cui_slider_style_plan_v1 *out) {
    if (!o || !out || o->version != 1 || o->struct_size != sizeof(*o) || o->reserved[0] ||
        o->reserved[1] || o->layout > CUI_SLIDER_STYLE_PARENT || (o->flags & ~UINT32_C(3)) ||
        (o->color_overrides & ~UINT32_C(3)) || (o->track_corner_overrides & ~UINT32_C(15)) ||
        !isfinite(o->rem) || o->rem <= 0 || !isfinite(o->rem * 1.5f) ||
        (o->layout == CUI_SLIDER_STYLE_FIXED && (!isfinite(o->length) || o->length <= 0)) ||
        !cui_slider_style_nonnegative(o->full_radius) || !cui_slider_palette_valid(&o->palette) ||
        ((o->color_overrides & CUI_SLIDER_STYLE_BAR_OVERRIDE) &&
         !cui_slider_style_color_valid(o->bar_override)) ||
        ((o->color_overrides & CUI_SLIDER_STYLE_THUMB_OVERRIDE) &&
         !cui_slider_style_color_valid(o->thumb_override)))
        return false;
    const float corners[4] = {o->track_corners.top_left, o->track_corners.top_right,
                              o->track_corners.bottom_left, o->track_corners.bottom_right};
    float resolved[4];
    for (unsigned i = 0; i < 4; ++i) {
        bool active = (o->track_corner_overrides & (1u << i)) != 0;
        if (active && !cui_slider_style_nonnegative(corners[i]))
            return false;
        resolved[i] = active ? corners[i] : o->full_radius;
    }
    cui_slider_style_plan_v1 p;
    memset(&p, 0, sizeof p);
    p.version = 1;
    p.struct_size = sizeof p;
    p.layout = o->layout;
    p.flags = o->flags;
    p.length = o->layout == CUI_SLIDER_STYLE_FIXED ? o->length : 0;
    p.cross_extent = 1.5f * o->rem;
    p.track_extent = .375f * o->rem;
    p.thumb_extent = o->rem;
    p.surface_extent = fmaxf(0, o->rem - 2);
    p.padding = 1;
    p.axis_offset = -8;
    p.cross_offset = -5;
    p.ring_max_width = 3;
    p.full_radius = o->full_radius;
    p.track_corners = (cui_corner_radii){resolved[0], resolved[1], resolved[2], resolved[3]};
    p.fill =
        (o->color_overrides & CUI_SLIDER_STYLE_BAR_OVERRIDE) ? o->bar_override : o->palette.bar;
    p.track = p.outer = p.fill;
    p.track.a *= (o->flags & CUI_SLIDER_STYLE_ACTIVE) ? .4f : .2f;
    p.outer.a *= .5f;
    p.ring = o->palette.ring;
    const cui_slider_palette_brush *b = &o->palette.thumb;
    bool valid;
    if (o->color_overrides & CUI_SLIDER_STYLE_THUMB_OVERRIDE)
        valid = cui_background_make_solid(cui_slider_style_color(o->thumb_override), &p.thumb);
    else if (b->kind == CUI_SLIDER_BRUSH_SOLID)
        valid = cui_background_make_solid(cui_slider_style_color(b->solid), &p.thumb);
    else
        valid = cui_background_make_linear(
            CUI_BACKGROUND_SOURCE_038,
            b->space == CUI_SLIDER_BRUSH_SRGB ? CUI_BACKGROUND_SRGB : CUI_BACKGROUND_OKLAB,
            b->angle_degrees,
            (cui_background_stop){cui_slider_style_color(b->stops[0].color), b->stops[0].position},
            (cui_background_stop){cui_slider_style_color(b->stops[1].color), b->stops[1].position},
            &p.thumb);
    if (!valid)
        return false;
    *out = p;
    return true;
}
