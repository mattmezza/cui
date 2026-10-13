/* SPDX-License-Identifier: MIT */
#include "../../backends/software/cui_shadow.h"
#include "cui/cui_effects.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static cui_inset_shadow_paint paint(void) {
    return (cui_inset_shadow_paint){.caster = {20, 20, 20, 20},
                                    .layer = {.spread = 2, .color = {0, 0, 0, 255}}};
}
static void geometry_pixels(void) {
    cui_inset_shadow_paint p = paint();
    cui_inset_shadow_raster q;
    float a;
    assert(cui_effect_inset_shadow_query(p, 1, 0, 0, &q));
    assert(q.covered_hole.x == 22 && q.covered_hole.w == 16 && q.width == 20);
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 21.5f, 30.5f, &a) && a == 1);
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 22.5f, 30.5f, &a) && a == 0);
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 19.5f, 30.5f, &a) && a == 0);
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 38.2f, 38.2f, &a) && fabsf(a - .7f) < 1e-5f);
    p.radii = (cui_corner_radii){10, 2, 6, 0};
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 21.5f, 21.5f, &a) && a == 0);
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 38.5f, 38.5f, &a) && a == 1);
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 23, 23, &a));
    assert(fabsf(a - (.5f - (sqrtf(98) - 10))) < 1e-5f);
    p = paint();
    p.layer.offset_x = 8;
    p.layer.sigma = 1;
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 21.5f, 30.5f, &a) && a > .99f);
    p = paint();
    p.caster = (cui_rect){4000.1001f, 4000.1001f, 32, 32};
    p.layer.offset_x = p.layer.offset_y = 1.5f;
    p.layer.spread = .6f;
    assert(cui_effect_inset_shadow_query(p, 1, -4000, -4000, &q));
    assert(q.covered_hole.w == 32 && q.covered_hole.h == 32 && q.width == 33 && q.height == 33);
    p = paint();
    p.caster = (cui_rect){20.25f, 20.25f, 1, 1};
    p.layer.spread = .6f;
    assert(cui_effect_inset_shadow_query(p, 1, 0, 0, &q) && q.covered_hole.w == 1);
    assert(cui_effect_inset_shadow_query(p, 2, 0, 0, &q) && q.covered_hole.w == 1);
    p.layer.spread = 10;
    assert(cui_effect_inset_shadow_query(p, 1, 0, 0, &q) && q.width == 0);
    p = paint();
    p.radii = (cui_corner_radii){100, 2, 6, 0};
    p.layer.spread = -2;
    assert(cui_effect_inset_shadow_query(p, 2, .25f, .75f, &q));
    assert(q.covered_hole.w == 49 && q.covered_element.w == 41);
    for (unsigned y = 0; y < q.height; y++)
        for (unsigned x = 0; x < q.width; x++) {
            assert(cui_effect_inset_shadow_sample(p, 2, .25f, .75f, q.covered_element.x + x + .5f,
                                                  q.covered_element.y + y + .5f, &a));
            assert(isfinite(a) && a >= 0 && a <= 1);
        }
    p = paint();
    assert(cui_effect_inset_shadow_query(p, 1, 0, 0, &q));
    unsigned char pixels[20 * 20 * 4];
    memset(pixels, 123, sizeof pixels);
    assert(!cui_effect_inset_shadow_raster(p, 1, 0, 0,
                                           (cui_effect_buffer){pixels, 19, 20, 80, sizeof pixels}));
    for (size_t i = 0; i < sizeof pixels; i++)
        assert(pixels[i] == 123);
    assert(cui_effect_inset_shadow_raster(p, 1, 0, 0,
                                          (cui_effect_buffer){pixels, 20, 20, 80, sizeof pixels}));
    assert(pixels[(10 * 20 + 1) * 4 + 3] == 255 && pixels[(10 * 20 + 2) * 4 + 3] == 0);
    cui_inset_shadow_raster old = q;
    p.layer.sigma = NAN;
    assert(!cui_effect_inset_shadow_query(p, 1, 0, 0, &q) && q.width == old.width &&
           q.bounds.x == old.bounds.x);
    p = paint();
    p.caster = (cui_rect){0, 0, 4096, 4096};
    assert(cui_effect_inset_shadow_query(p, 1, 0, 0, &q) && q.bytes == 67108864);
    old = q;
    p.caster.h = 4097;
    assert(!cui_effect_inset_shadow_query(p, 1, 0, 0, &q) && q.bytes == old.bytes &&
           q.height == old.height);
    p = paint();
    p.layer.sigma = nextafterf(0, 1);
    a = 123;
    assert(!cui_effect_inset_shadow_sample(p, 1, 0, 0, 25, 25, &a) && a == 123);
}
static void gaussian_reference(void) {
    cui_inset_shadow_paint p = paint();
    p.layer.sigma = 3;
    for (int y = 20; y < 40; y++)
        for (int x = 20; x < 40; x++) {
            float a;
            double sx = 3 * sqrt(2.0);
            double gx = .5 * (erf((38 - x - .5) / sx) - erf((22 - x - .5) / sx));
            double gy = .5 * (erf((38 - y - .5) / sx) - erf((22 - y - .5) / sx));
            assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, x + .5f, y + .5f, &a));
            assert(fabs(a - (1 - gx * gy)) < .025);
        }
    p = paint();
    p.layer.sigma = 4 * sqrtf(FLT_MIN);
    float tiny;
    assert(cui_effect_inset_shadow_sample(p, 1, 0, 0, 25.5f, 25.5f, &tiny) && isfinite(tiny));
    p.layer.sigma /= 64;
    assert(cui_effect_inset_shadow_sample(p, 64, 0, 0, 1600.5f, 1600.5f, &tiny) && isfinite(tiny));
    const float sigmas[] = {0, .0001f, 1, 256};
    for (size_t i = 0; i < sizeof sigmas / sizeof sigmas[0]; i++) {
        p.caster = (cui_rect){16384, -16384, 16384, 16384};
        p.radii = (cui_corner_radii){16384, 1, 4096, 0};
        p.layer.offset_x = 4096;
        p.layer.offset_y = -4096;
        p.layer.spread = -4096;
        p.layer.sigma = sigmas[i];
        float a;
        assert(
            cui_effect_inset_shadow_sample(p, 64, 1048576, -1048576, 2097152.5f, -2097152.5f, &a));
        assert(isfinite(a) && a >= 0 && a <= 1);
    }
}
static void cache_culling(void) {
    cui_draw_command c = {.type = CUI_DRAW_INSET_SHADOW, .shadow = paint()};
    cui_draw_list list = {&c, 1, 64, 64}, out;
    cui_shadow_cache cache = {0};
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out) && cache.misses == 1);
    c.shadow.caster.x++;
    c.shadow.layer.color.a = 128;
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out) && cache.hits == 1);
    c.type = CUI_DRAW_DROP_SHADOW;
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out) && cache.misses == 2);
    c.type = CUI_DRAW_INSET_SHADOW;
    c.shadow = paint();
    cui_draw_command clipped[] = {
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {20, 20, 1, 20}}, c, {.type = CUI_DRAW_CLIP_POP}};
    list = (cui_draw_list){clipped, 3, 64, 64};
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out));
    assert(out.commands[1].type == CUI_DRAW_RECT && out.commands[1].bounds.w == 0);
    clipped[0].bounds = (cui_rect){20, 20, 4, 20};
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &out));
    assert(out.commands[1].type == CUI_DRAW_INSET_SHADOW && out.commands[1].bounds.x == 20);
    cui_software *r = cui_software_create(64, 64, NULL);
    assert(r && cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
    const unsigned char *p = cui_software_pixels(r, NULL, NULL);
    assert(p[(30 * 64 + 21) * 4] == 0 && p[(30 * 64 + 22) * 4] == 255);
    cui_draw_list bad = list;
    bad.height = INFINITY;
    assert(!cui_software_render(r, &bad, (cui_color){0}));
    assert(p[(30 * 64 + 21) * 4] == 0);
    cui_draw_command many[65];
    for (size_t i = 0; i < 65; i++) {
        many[i] = (cui_draw_command){.type = CUI_DRAW_INSET_SHADOW, .shadow = paint()};
        many[i].shadow.layer.color.r = (unsigned char)i;
    }
    cui_draw_list crowded = {many, 65, 64, 64};
    assert(!cui_software_render(r, &crowded, (cui_color){0}));
    assert(p[(30 * 64 + 21) * 4] == 0 && p[(30 * 64 + 22) * 4] == 255);
    cui_software_destroy(r);
    cui_shadow_destroy(&cache);
    cache = (cui_shadow_cache){0};
    c = (cui_draw_command){.type = CUI_DRAW_INSET_SHADOW,
                           .shadow = {.caster = {0, 0, 16384, 16384},
                                      .layer = {.spread = 4096, .color = {0, 0, 0, 255}}}};
    list = (cui_draw_list){&c, 1, 16384, 16384};
    float scale = 1.0f / 65536, origin = 52428 * scale;
    assert(cui_shadow_prepare(&cache, &list, scale, scale, origin, origin, &out));
    assert(out.commands[0].bitmap.bitmap.width == 2 && out.commands[0].bitmap.bitmap.height == 2);
    assert(out.commands[0].bitmap.bitmap.pixels[3] == 0);
    assert(out.commands[0].bitmap.bitmap.pixels[out.commands[0].bitmap.bitmap.stride + 7] == 255);
    assert(cui_shadow_prepare(&cache, &list, scale, scale, origin, origin, &out) &&
           cache.hits == 1);
    c.shadow.layer.sigma = NAN;
    cui_draw_list unchanged = out;
    assert(!cui_shadow_prepare(&cache, &list, scale, scale, origin, origin, &out));
    assert(out.commands == unchanged.commands && out.count == unchanged.count);
    cui_shadow_destroy(&cache);
}
static bool copied(const cui_prepaint_info *info, const void *payload, cui_prepaint_writer *w) {
    (void)info;
    return cui_prepaint_inset_shadow(w, payload, false);
}
static void core_order(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    assert(ui && cui_begin_frame(ui, 64, 64, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 20,
                            .height = 20,
                            .fixed_width = true,
                            .fixed_height = true,
                            .clip = true,
                            .background = {255, 255, 255, 255}});
    cui_box_shadow layers[] = {{paint().layer, CUI_BOX_SHADOW_INSET},
                               {{.spread = 1, .color = {255, 0, 0, 255}}, CUI_BOX_SHADOW_DROP}};
    assert(cui_set_box_shadows(ui, layers, 2, (cui_corner_radii){0}));
    layers[0].kind = (cui_box_shadow_kind)99;
    assert(!cui_set_box_shadows(ui, layers, 2, (cui_corner_radii){0}));
    memset(layers, 0, sizeof layers);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    size_t drop = SIZE_MAX, surface = SIZE_MAX, inset = SIZE_MAX, clip = SIZE_MAX;
    for (size_t i = 0; i < list.count; i++) {
        if (list.commands[i].type == CUI_DRAW_DROP_SHADOW)
            drop = i;
        if (list.commands[i].type == CUI_DRAW_INSET_SHADOW)
            inset = i;
        if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].color.a)
            surface = i;
        if (list.commands[i].type == CUI_DRAW_CLIP_PUSH)
            clip = i;
    }
    assert(drop < surface && surface < inset && inset < clip);
    cui_software *r = cui_software_create(64, 64, NULL);
    assert(r && cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
    const unsigned char *p = cui_software_pixels(r, NULL, NULL);
    assert(p[(10 * 64 + 1) * 4] == 0 && p[(10 * 64 + 2) * 4] == 255);
    assert(cui_begin_frame(ui, 64, 64, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 64, .height = 64});
    cui_inset_shadow_paint payload = paint();
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = copied,
                                                       .payload = &payload,
                                                       .payload_bytes = sizeof payload,
                                                       .command_budget = 3}));
    memset(&payload, 0, sizeof payload);
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
    p = cui_software_pixels(r, NULL, NULL);
    assert(p[(30 * 64 + 21) * 4] == 0);
    assert(cui_begin_frame(ui, 64, 64, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 20, .height = 20});
    cui_box_shadow layer = {paint().layer, CUI_BOX_SHADOW_INSET};
    assert(cui_set_box_shadows(ui, &layer, 1, (cui_corner_radii){0}));
    cui_box_shadow bounded[8];
    for (size_t i = 0; i < 8; i++)
        bounded[i] = (cui_box_shadow){.layer = paint().layer,
                                      .kind = i % 2 ? CUI_BOX_SHADOW_INSET : CUI_BOX_SHADOW_DROP};
    assert(cui_set_box_shadows(ui, bounded, 8, (cui_corner_radii){0}));
    assert(!cui_set_box_shadows(ui, bounded, 9, (cui_corner_radii){0}));
    assert(cui_set_drop_shadows(ui, NULL, 0, (cui_corner_radii){0}));
    cui_end_box(ui);
    list = cui_end_frame(ui);
    for (size_t i = 0; i < list.count; i++)
        assert(list.commands[i].type != CUI_DRAW_INSET_SHADOW);
    assert(cui_begin_frame(ui, 64, 64, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 20,
                            .height = 20,
                            .fixed_width = true,
                            .fixed_height = true,
                            .background = {200, 120, 80, 128},
                            .opacity = .5f,
                            .custom_opacity = true});
    cui_box_shadow mixed[] = {
        {.layer = {.spread = 2, .color = {255, 0, 0, 128}}, .kind = CUI_BOX_SHADOW_INSET},
        {.layer = {.spread = 1, .color = {0, 255, 0, 255}}, .kind = CUI_BOX_SHADOW_DROP},
        {.layer = {.spread = 2, .color = {0, 0, 255, 128}}, .kind = CUI_BOX_SHADOW_INSET}};
    assert(cui_set_box_shadows(ui, mixed, 3, (cui_corner_radii){0}));
    assert(!cui_set_box_shadows(ui, mixed, 9, (cui_corner_radii){0}));
    cui_end_box(ui);
    list = cui_end_frame(ui);
    assert(cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
    p = cui_software_pixels(r, NULL, NULL);
    /* Green DROP is below the half-opacity surface; red then blue INSET
     * composite above it. Calculate all four straight-alpha over operations. */
    float rgb[] = {255, 255, 255};
    const float colors[][3] = {{0, 255, 0}, {200, 120, 80}, {255, 0, 0}, {0, 0, 255}};
    const float alphas[] = {127 / 255.0f, 64 / 255.0f, 64 / 255.0f, 64 / 255.0f};
    for (size_t layer = 0; layer < 4; layer++)
        for (size_t k = 0; k < 3; k++)
            rgb[k] = colors[layer][k] * alphas[layer] + rgb[k] * (1 - alphas[layer]);
    for (size_t k = 0; k < 3; k++)
        assert(fabsf(p[(10 * 64 + 1) * 4 + k] - rgb[k]) <= 3);
    cui_software_destroy(r);
    cui_destroy(ui);
}
int main(void) {
    geometry_pixels();
    gaussian_reference();
    cache_culling();
    core_order();
    return 0;
}
