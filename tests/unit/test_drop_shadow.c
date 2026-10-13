/* SPDX-License-Identifier: MIT */
#include "../../backends/software/cui_shadow.h"
#include "cui/cui_effects.h"
#include "cui/cui_software.h"
#include "cui/cui_theme.h"
#include <assert.h>
#include <stdio.h>
static cui_drop_shadow_paint paint(void) {
    return (cui_drop_shadow_paint){.caster = {20, 20, 20, 12},
                                   .layer = {.sigma = 3, .color = {200, 100, 50, 128}}};
}
static void invalid_mask(void) {
    cui_shadow_cache cache = {0};
    cui_draw_command commands[] = {{.type = CUI_DRAW_CLIP_PUSH, .bounds = {NAN, 0, 4, 4}},
                                   {.type = CUI_DRAW_DROP_SHADOW, .shadow = paint()},
                                   {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list list = {commands, 3, 64, 64}, out = {.width = 777};
    assert(!cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out) && out.width == 777);
    commands[0].bounds = (cui_rect){0, 0, -1, 4};
    assert(!cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out) && out.width == 777);
    commands[0].bounds = (cui_rect){0, 0, 4, INFINITY};
    assert(!cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out) && out.width == 777);
    cui_shadow_destroy(&cache);
}
static void numerical(void) {
    cui_drop_shadow_paint p = paint();
    float a, b;
    /* Independent exact rectangle convolution: erf CDF in both axes. The
     * pinned four-Y-sample approximation is bounded against this reference. */
    for (int y = 12; y < 40; y++)
        for (int x = 12; x < 48; x++) {
            assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, x + .5f, y + .5f, &a));
            double sx = p.layer.sigma * sqrt(2.0);
            double gx = .5 * (erf((40 - x - .5) / sx) - erf((20 - x - .5) / sx));
            double gy = .5 * (erf((32 - y - .5) / sx) - erf((20 - y - .5) / sx));
            assert(fabs(a - gx * gy) < .025);
        }
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 10.5f, 24.5f, &a) && a == 0);
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 18.5f, 24.5f, &a) && a > .2f);
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 41.5f, 24.5f, &b) && fabsf(a - b) < 1e-6f);
    p.layer.sigma = 0;
    p.layer.spread = 1;
    p.radii = (cui_corner_radii){5, 5, 5, 5};
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 19.5f, 25.5f, &a) && a == 1);
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 19.5f, 19.5f, &a) && a == 0);
    p.radii = (cui_corner_radii){0};
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 19.5f, 19.5f, &a) && a == 1);
}
static void atomic(void) {
    cui_drop_shadow_paint p = paint();
    cui_drop_shadow_raster q;
    float a_dummy;
    assert(cui_effect_drop_shadow_query(p, 2, .25f, .75f, &q));
    assert(q.width == 77 && q.height == 61);
    cui_drop_shadow_raster unchanged = q;
    p.layer.sigma = NAN;
    assert(!cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
    assert(q.width == unchanged.width && q.bounds.x == unchanged.bounds.x);
    unsigned char bytes[32];
    memset(bytes, 123, sizeof(bytes));
    p = paint();
    assert(!cui_effect_drop_shadow_raster(p, 1, 0, 0, (cui_effect_buffer){bytes, 1, 1, 4, 4}));
    for (size_t i = 0; i < sizeof(bytes); i++)
        assert(bytes[i] == 123);
    p.layer.spread = -10;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q) && !q.width);
    p = paint();
    p.layer.sigma = 1e-10f;
    assert(cui_effect_drop_shadow_sample(p, 1, 0, 0, 20.5f, 20.5f, &a_dummy));
    p.layer.sigma = nextafterf(0, 1);
    a_dummy = 123;
    assert(!cui_effect_drop_shadow_sample(p, 1, 0, 0, 20.5f, 20.5f, &a_dummy) && a_dummy == 123);
    p = paint();
    p.caster.x = 16384;
    p.caster.y = -16384;
    p.layer.sigma = .0001f;
    assert(cui_effect_drop_shadow_sample(p, 64, 1048576, -1048576, 2097152.5f, -2097152.5f,
                                         &a_dummy) &&
           isfinite(a_dummy));
    p = paint();
    p.caster.w = 16384;
    p.caster.h = 16384;
    assert(!cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
}
static void cache_and_pixels(void) {
    cui_shadow_cache cache = {0};
    cui_draw_command c = {.type = CUI_DRAW_DROP_SHADOW, .shadow = paint()};
    cui_draw_list list = {&c, 1, 64, 64}, out;
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out));
    assert(cache.misses == 1);
    c.shadow.caster.x += 1;
    c.shadow.layer.color.a = 64;
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out));
    assert(cache.hits == 1 && cache.misses == 1);
    c.shadow.caster.x += .25f;
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out));
    assert(cache.misses == 2);
    assert(!cui_shadow_prepare(&cache, &list, 1, 2, 0, 0, &out));
    c = (cui_draw_command){.type = CUI_DRAW_RECT, .bounds = {0, 0, 1, 1}};
    assert(cui_shadow_prepare(&cache, &list, 1, 2, 0, 0, &out));
    cui_draw_command tail[] = {{.type = CUI_DRAW_CLIP_PUSH, .bounds = {10, 20, 7, 12}},
                               {.type = CUI_DRAW_DROP_SHADOW, .shadow = paint()},
                               {.type = CUI_DRAW_CLIP_POP}};
    list = (cui_draw_list){tail, 3, 64, 64};
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out));
    assert(out.commands[1].type == CUI_DRAW_RECT);
    tail[0].bounds.w = 12;
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out));
    assert(out.commands[1].type == CUI_DRAW_DROP_SHADOW);
    cui_software *r = cui_software_create(64, 64, NULL);
    assert(r);
    assert(cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
    const unsigned char *pixels = cui_software_pixels(r, NULL, NULL);
    assert(pixels[(25 * 64 + 19) * 4] < 255 && pixels[(25 * 64 + 22) * 4] == 255);
    unsigned char before = pixels[(25 * 64 + 19) * 4];
    tail[1].shadow.layer.sigma = NAN;
    assert(!cui_software_render(r, &list, (cui_color){0}));
    assert(pixels[(25 * 64 + 19) * 4] == before);
    cui_software_destroy(r);
    cui_shadow_destroy(&cache);
}
static void fractional_and_cancellation(void) {
    cui_shadow_cache cache = {0};
    cui_draw_command commands[] = {{.type = CUI_DRAW_CLIP_PUSH, .bounds = {18.25f, 22.25f, 3, 5}},
                                   {.type = CUI_DRAW_DROP_SHADOW, .shadow = paint()},
                                   {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list list = {commands, 3, 64, 64}, out;
    assert(cui_shadow_prepare(&cache, &list, 2, 2, 0, 0, &out));
    assert(out.commands[1].bounds.x == 18 && out.commands[1].bounds.y == 22 &&
           out.commands[1].bounds.w == 3.5f && out.commands[1].bounds.h == 5.5f);
    assert(out.commands[1].bitmap.source.w == 7 && out.commands[1].bitmap.source.h == 11);
    cui_shadow_destroy(&cache);
    // f32 large-origin addition loses a fractional width. Culling must use
    // the query's covered layer, not recompute the nominal caster in double.
    cui_drop_shadow_paint p = paint();
    p.caster = (cui_rect){16384, 0, 1.0001f, 12};
    p.layer.offset_x = 0;
    cui_drop_shadow_raster q;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
    assert(q.covered_layer.x == 16384 && q.covered_layer.w == 1);
    commands[0].bounds = (cui_rect){16384, 0, 1, 12};
    commands[1].shadow = p;
    list.width = 16384 + 8;
    assert(cui_shadow_prepare(&cache, &list, 1, 1, -16384, 0, &out));
    const cui_draw_command *c = &out.commands[1];
    assert(c->type == CUI_DRAW_DROP_SHADOW);
    // Coverage at the prepared crop must equal original high-coordinate sample.
    float alpha;
    assert(cui_effect_drop_shadow_sample(p, 1, -16384, 0, .5f, 5.5f, &alpha));
    unsigned sx = (unsigned)c->bitmap.source.x, sy = (unsigned)(c->bitmap.source.y + 5);
    const unsigned char *pixel =
        c->bitmap.bitmap.pixels + (size_t)sy * c->bitmap.bitmap.stride + sx * 4;
    assert(abs((int)pixel[3] - (int)lroundf(alpha * 255)) <= 1);
    cui_shadow_destroy(&cache);
}
static void capacity_and_own_clip(void) {
    cui_shadow_cache cache = {0};
    cui_draw_command c[65];
    for (unsigned i = 0; i < 65; i++) {
        c[i] = (cui_draw_command){.type = CUI_DRAW_DROP_SHADOW, .shadow = paint()};
        c[i].shadow.layer.color.r = (unsigned char)i;
    }
    cui_draw_list source = {c, 65, 64, 64}, out = {.count = 123};
    assert(!cui_shadow_prepare(&cache, &source, 1, 1, 0, 0, &out) && out.count == 123);
    c[0].shadow.caster.w = 4000;
    c[0].shadow.caster.h = 3000;
    source.count = 1;
    assert(!cui_shadow_prepare(&cache, &source, 1, 1, 0, 0, &out) && out.count == 123);
    cui_shadow_destroy(&cache);
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    assert(ui);
    assert(cui_begin_frame(ui, 64, 64, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 20,
                            .height = 12,
                            .fixed_width = true,
                            .fixed_height = true,
                            .clip = true,
                            .background = {255, 255, 255, 255}});
    cui_drop_shadow layers[9] = {0};
    layers[0] = (cui_drop_shadow){.spread = 1, .color = {0, 0, 0, 255}};
    assert(cui_set_drop_shadows(ui, layers, 1, (cui_corner_radii){0}));
    assert(!cui_set_drop_shadows(ui, layers, 9, (cui_corner_radii){0}));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    size_t shadow = SIZE_MAX, clip = SIZE_MAX;
    for (size_t i = 0; i < list.count; i++) {
        if (list.commands[i].type == CUI_DRAW_DROP_SHADOW)
            shadow = i;
        if (list.commands[i].type == CUI_DRAW_CLIP_PUSH)
            clip = i;
    }
    assert(shadow < clip); // current ancestor mask, before the caster's child-overflow clip
    cui_software *r = cui_software_create(64, 64, NULL);
    assert(r && cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
    const unsigned char *pixels = cui_software_pixels(r, NULL, NULL);
    assert(pixels[(6 * 64 + 20) * 4] == 0 && pixels[(6 * 64 + 21) * 4] == 255);
    cui_software_destroy(r);
    cui_destroy(ui);
}
static void core_and_theme(void) {
    cui_theme theme = cui_theme_light();
    cui_drop_shadow layers[3];
    assert(cui_theme_popover_shadows(&theme, 1, layers));
    assert(layers[0].spread == 1 && layers[0].color.a == 26 && layers[1].sigma == 3 &&
           layers[1].offset_y == 4 && layers[2].sigma == 2 && layers[2].spread == -2);
    cui_context *ui = cui_create((cui_config){.max_commands = 64, .max_elements = 32});
    assert(ui);
    assert(cui_begin_frame(ui, 64, 64, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 20,
                            .height = 12,
                            .fixed_width = true,
                            .fixed_height = true,
                            .background = {255, 255, 255, 128},
                            .custom_opacity = true,
                            .opacity = .5f});
    assert(cui_set_drop_shadows(ui, layers, 3, (cui_corner_radii){4, 4, 4, 4}));
    layers[0].sigma = NAN;
    assert(!cui_set_drop_shadows(ui, layers, 3, (cui_corner_radii){0}));
    memset(layers, 0, sizeof(layers));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    size_t count = 0, first = SIZE_MAX, surface = SIZE_MAX;
    for (size_t i = 0; i < list.count; i++) {
        if (list.commands[i].type == CUI_DRAW_DROP_SHADOW) {
            if (first == SIZE_MAX)
                first = i;
            assert(list.commands[i].shadow.layer.color.a == 13);
            count++;
        }
        if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].color.a == 64)
            surface = i;
    }
    assert(count == 3 && first < surface);
    cui_destroy(ui);
}
int main(void) {
    invalid_mask();
    numerical();
    atomic();
    cache_and_pixels();
    fractional_and_cancellation();
    capacity_and_own_clip();
    core_and_theme();
    puts("drop shadow numerical/core/cache/pixels passed");
    return 0;
}
