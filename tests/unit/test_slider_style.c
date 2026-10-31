/* SPDX-License-Identifier: MIT */
#ifdef CUI_SLIDER_STYLE_DISTRIBUTION
#define CUI_SLIDER_STYLE_IMPLEMENTATION
#ifdef CUI_SLIDER_STYLE_UMBRELLA
#define CUI_IMPLEMENTATION
#include "cui_all.h"
#else
#include "cui_slider_style.h"
#endif
#else
#include "cui/cui_slider_style.h"
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_slider_style_options_v1 options(void) {
    cui_slider_style_options_v1 o = {
        .version = 1, .struct_size = sizeof o, .rem = 16, .length = 240, .full_radius = 999};
    assert(cui_slider_palette_builtin_get(CUI_SLIDER_PALETTE_LIGHT, &o.palette));
    return o;
}
static void color(cui_slider_palette_color c, cui_background_color b) {
    assert(c.r == b.r && c.g == b.g && c.b == b.b && c.a == b.a);
}
static void verify(cui_slider_style_options_v1 o) {
    cui_slider_style_plan_v1 p;
    assert(cui_slider_style_resolve_v1(&o, &p));
    assert(p.version == 1 && p.struct_size == sizeof p);
    assert(p.cross_extent == o.rem * 1.5f && p.track_extent == o.rem * .375f);
    assert(p.thumb_extent == o.rem && p.surface_extent == fmaxf(0, o.rem - 2));
    assert(p.padding == 1 && p.axis_offset == -8 && p.cross_offset == -5 && p.ring_max_width == 3);
    assert(p.fill.r == o.palette.bar.r && p.fill.a == o.palette.bar.a);
    assert(p.track.a == o.palette.bar.a * .2f && p.outer.a == o.palette.bar.a * .5f);
    assert(memcmp(&p.ring, &o.palette.ring, sizeof p.ring) == 0);
    assert(cui_background_valid(&p.thumb));
    if (o.palette.thumb.kind == CUI_SLIDER_BRUSH_SOLID) {
        assert(p.thumb.kind == CUI_BACKGROUND_SOLID);
        color(o.palette.thumb.solid, p.thumb.solid);
    } else {
        assert(p.thumb.kind == CUI_BACKGROUND_LINEAR && p.thumb.model == CUI_BACKGROUND_SOURCE_038);
        assert(p.thumb.angle_degrees == o.palette.thumb.angle_degrees);
        assert((unsigned)p.thumb.space == (unsigned)o.palette.thumb.space);
        for (unsigned i = 0; i < 2; ++i) {
            assert(p.thumb.stops[i].position == o.palette.thumb.stops[i].position);
            color(o.palette.thumb.stops[i].color, p.thumb.stops[i].color);
        }
    }
    o.flags = CUI_SLIDER_STYLE_ACTIVE | CUI_SLIDER_STYLE_DISABLED;
    cui_slider_style_plan_v1 active;
    assert(cui_slider_style_resolve_v1(&o, &active));
    assert(active.flags == o.flags && active.track.a == o.palette.bar.a * .4f);
    assert(memcmp(&active.thumb, &p.thumb, sizeof p.thumb) == 0); /* disabled never flattens */
}
static void invalid(cui_slider_style_options_v1 o) {
    cui_slider_style_plan_v1 p, before;
    memset(&p, 0xa5, sizeof p);
    before = p;
    assert(!cui_slider_style_resolve_v1(&o, &p));
    assert(memcmp(&p, &before, sizeof p) == 0);
}
int main(void) {
    cui_slider_style_options_v1 o = options();
    for (unsigned i = 0; i < 4; ++i) {
        assert(cui_slider_palette_builtin_get((cui_slider_palette_builtin)i, &o.palette));
        verify(o);
    }
    assert(cui_slider_palette_preset_count() == 38);
    for (size_t i = 0; i < 38; ++i) {
        assert(cui_slider_palette_preset_get(i, &o.palette));
        verify(o);
    }
    o = options();
    o.rem = 24;
    verify(o);
    o.rem = 1;
    verify(o); /* no negative inner surface */
    o = options();
    o.palette.bar = (cui_slider_palette_color){.17f, .29f, .41f, .63f};
    o.palette.thumb = (cui_slider_palette_brush){
        .kind = CUI_SLIDER_BRUSH_LINEAR,
        .space = CUI_SLIDER_BRUSH_OKLAB,
        .projection = CUI_SLIDER_BRUSH_SOURCE_038,
        .angle_degrees = 317,
        .stops = {{.9f, {.11f, .22f, .33f, 0}}, {.1f, {.44f, .55f, .66f, .7f}}}};
    verify(o); /* custom reversed stops, hidden RGB, no inferred preset */
    o = options();
    o.full_radius = 0;
    o.track_corner_overrides = CUI_SLIDER_STYLE_TOP_LEFT | CUI_SLIDER_STYLE_BOTTOM_RIGHT;
    o.track_corners = (cui_corner_radii){7, NAN, NAN, 9};
    cui_slider_style_plan_v1 p;
    assert(cui_slider_style_resolve_v1(&o, &p));
    assert(p.track_corners.top_left == 7 && p.track_corners.top_right == 0 &&
           p.track_corners.bottom_left == 0 && p.track_corners.bottom_right == 9 &&
           p.full_radius == 0);
    o.layout = CUI_SLIDER_STYLE_PARENT;
    o.length = NAN;
    o.bar_override.r = NAN;
    o.thumb_override.a = NAN;
    assert(cui_slider_style_resolve_v1(&o, &p) && p.length == 0); /* inactive fields ignored */
    o.bar_override = (cui_slider_palette_color){.2f, .3f, .4f, 0};
    o.thumb_override = (cui_slider_palette_color){.7f, .6f, .5f, 0};
    o.color_overrides = 3;
    assert(cui_slider_style_resolve_v1(&o, &p));
    assert(p.fill.a == 0 && p.fill.r == .2f);
    assert(p.thumb.kind == CUI_BACKGROUND_SOLID && p.thumb.solid.a == 0 && p.thumb.solid.r == .7f);
    o = options();
    o.version = 2;
    invalid(o);
    o = options();
    --o.struct_size;
    invalid(o);
    o = options();
    o.reserved[1] = 1;
    invalid(o);
    o = options();
    o.layout = 2;
    invalid(o);
    o = options();
    o.flags = 4;
    invalid(o);
    o = options();
    o.color_overrides = 4;
    invalid(o);
    o = options();
    o.track_corner_overrides = 16;
    invalid(o);
    o = options();
    o.rem = INFINITY;
    invalid(o);
    o = options();
    o.rem = 0;
    invalid(o);
    o = options();
    o.length = -1;
    invalid(o);
    o = options();
    o.full_radius = NAN;
    invalid(o);
    o = options();
    o.track_corner_overrides = 1;
    o.track_corners.top_left = NAN;
    invalid(o);
    o = options();
    o.color_overrides = 1;
    o.bar_override.r = NAN;
    invalid(o);
    o = options();
    o.color_overrides = 2;
    o.thumb_override.a = 2;
    invalid(o);
    o = options();
    o.palette.thumb.kind = (cui_slider_brush_kind)99;
    invalid(o);
    assert(!cui_slider_style_resolve_v1(NULL, &p));
    assert(!cui_slider_style_resolve_v1(&o, NULL));
    puts("PASS 4 builtins +38 presets, exact owned brush conversion, source metrics/alpha/corners, "
         "custom/inactive/atomic rejection");
    return 0;
}
