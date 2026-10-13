/* SPDX-License-Identifier: MIT */
/* Source: gpui-pre 0.3.8 Window::cover_bounds and Bounds::dilate,
 * locked by shadow_covering_source.json. */
#include "../../backends/software/cui_shadow.h"
#include "cui_effects.h"
#include "cui_software.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static bool copied_paint(const cui_prepaint_info *info, const void *payload,
                         cui_prepaint_writer *writer) {
    (void)info;
    cui_draw_command command = {.type = CUI_DRAW_DROP_SHADOW,
                                .shadow = *(const cui_drop_shadow_paint *)payload};
    return cui_prepaint_emit(writer, &command, false);
}
int main(void) {
    cui_drop_shadow_paint p = {
        .caster = {4000.1001f, 20.25f, 32, 4},
        .layer = {.offset_x = 1.5f, .spread = -.6f, .color = {0, 0, 0, 255}}};
    cui_drop_shadow_raster q;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
    assert(q.covered_layer.x == 4002 && q.covered_layer.w == 32);
    p.caster = (cui_rect){0, 20.25f, 1.0001f, 4};
    p.layer.offset_x = 4096;
    p.layer.spread = .0001f;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q) && q.covered_layer.w == 2);
    p.caster.x = 16384;
    p.layer.spread = .001f;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q) && q.covered_layer.w == 2);
    p.layer.spread = -.6f;
    p.caster = (cui_rect){20.25f, 20.25f, 1, 1};
    p.layer.offset_x = 0;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
    assert(q.width == 1 && q.height == 1 && q.covered_layer.x == 20);
    unsigned char pixel[4] = {0};
    assert(cui_effect_drop_shadow_raster(p, 1, 0, 0,
                                         (cui_effect_buffer){pixel, 1, 1, 4, sizeof pixel}));
    assert(pixel[3] == 255);
    assert(cui_effect_drop_shadow_query(p, 2, 0, 0, &q));
    assert(q.width == 1 && q.height == 1 && q.covered_layer.x == 41);
    cui_draw_command command = {.type = CUI_DRAW_DROP_SHADOW, .shadow = p};
    cui_draw_list list = {&command, 1, 64, 64};
    cui_software *renderer = cui_software_create(64, 64, NULL);
    assert(renderer && cui_software_render(renderer, &list, (cui_color){255, 255, 255, 255}));
    const unsigned char *pixels = cui_software_pixels(renderer, NULL, NULL);
    assert(pixels[(20 * 64 + 20) * 4] == 0);
    assert(pixels[(20 * 64 + 21) * 4] == 255);
    cui_software_destroy(renderer);
    cui_shadow_cache cache = {0};
    cui_draw_list prepared;
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &prepared));
    assert(prepared.count == 1 && cache.misses == 1);
    command.shadow.caster.x += 1;
    assert(cui_shadow_prepare(&cache, &list, 1, 1, 0, 0, &prepared));
    assert(prepared.count == 1 && cache.hits == 1);
    cui_draw_command clipped[] = {{.type = CUI_DRAW_CLIP_PUSH, .bounds = {22, 20, 1, 1}},
                                  command,
                                  {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list masked = {clipped, 3, 64, 64};
    assert(cui_shadow_prepare(&cache, &masked, 1, 1, 0, 0, &prepared));
    assert(prepared.count == 3 && prepared.commands[1].type == CUI_DRAW_RECT &&
           prepared.commands[1].bounds.w == 0); /* source unblurred covering is disjoint */
    cui_shadow_destroy(&cache);
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    assert(ui && cui_begin_frame(ui, 64, 64, .016f));
    cui_begin_box(ui, 2,
                  (cui_box){.width = 64, .height = 64, .fixed_width = true, .fixed_height = true});
    cui_begin_absolute_box(ui, 1, (cui_box){.clip = true}, p.caster);
    assert(cui_set_drop_shadows(ui, &p.layer, 1, (cui_corner_radii){0}));
    cui_end_box(ui);
    cui_end_box(ui);
    list = cui_end_frame(ui);
    bool seen = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_DROP_SHADOW) {
            assert(list.commands[i].bounds.w > 0 && list.commands[i].bounds.h > 0);
            seen = true;
        }
    assert(seen);
    renderer = cui_software_create(64, 64, NULL);
    assert(renderer && cui_software_render(renderer, &list, (cui_color){255, 255, 255, 255}));
    pixels = cui_software_pixels(renderer, NULL, NULL);
    assert(pixels[(20 * 64 + 20) * 4] == 0);
    cui_software_destroy(renderer);
    assert(cui_begin_frame(ui, 64, 64, .016f));
    cui_begin_box(ui, 3,
                  (cui_box){.width = 64, .height = 64, .fixed_width = true, .fixed_height = true});
    cui_drop_shadow_paint copied = p;
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = copied_paint,
                                                       .payload = &copied,
                                                       .payload_bytes = sizeof copied,
                                                       .command_budget = 3}));
    memset(&copied, 0, sizeof copied);
    cui_end_box(ui);
    list = cui_end_frame(ui);
    renderer = cui_software_create(64, 64, NULL);
    assert(renderer && cui_software_render(renderer, &list, (cui_color){255, 255, 255, 255}));
    pixels = cui_software_pixels(renderer, NULL, NULL);
    assert(pixels[(20 * 64 + 20) * 4] == 0);
    cui_software_destroy(renderer);
    cui_destroy(ui);
    p.caster = (cui_rect){20.25f, 20.25f, 0, 0};
    p.layer.spread = 0;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q) && q.width == 1 && q.height == 1);
    p.caster = (cui_rect){20, 20, 0, 0};
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q) && q.width == 0 && q.height == 0);
    p.caster = (cui_rect){20.25f, 20.25f, 1, 1};
    p.layer.spread = -10;
    assert(cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
    assert(q.width == 0 && q.height == 0);
    cui_drop_shadow_raster before = q;
    p.layer.spread = NAN;
    assert(!cui_effect_drop_shadow_query(p, 1, 0, 0, &q));
    assert(q.width == before.width && q.height == before.height);
    return 0;
}
