/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
/* Pinned public source tuples, independent of the CUI factory. */
static const float source[][2][4] = {{{0}},
                                     {{1, 0, 0, 13}},
                                     {{1, 2, 0, 13}},
                                     {{1, 3, 0, 26}, {1, 2, -1, 26}},
                                     {{4, 6, -1, 26}, {2, 4, -2, 26}},
                                     {{10, 15, -3, 26}, {4, 6, -4, 26}},
                                     {{20, 25, -5, 26}, {8, 10, -6, 26}},
                                     {{25, 50, -12, 64}}};
static void factories(void) {
    for (int preset = CUI_SHADOW_NONE; preset <= CUI_SHADOW_2XL; preset++) {
        cui_drop_shadow out[3] = {{.offset_x = 777}, {.offset_x = 777}, {.offset_x = 777}};
        size_t count = 999;
        assert(cui_shadow_preset_layers((cui_shadow_preset)preset, out, 3, &count));
        size_t expected = preset == 0                                          ? 0
                          : preset >= CUI_SHADOW_SM && preset <= CUI_SHADOW_XL ? 2
                                                                               : 1;
        assert(count == expected && out[count].offset_x == 777);
        for (size_t i = 0; i < count; i++) {
            assert(out[i].offset_x == 0 && out[i].offset_y == source[preset][i][0]);
            assert(out[i].sigma == source[preset][i][1] && out[i].spread == source[preset][i][2]);
            assert(out[i].color.r == 0 && out[i].color.g == 0 && out[i].color.b == 0 &&
                   out[i].color.a == source[preset][i][3]);
        }
    }
    cui_drop_shadow out[2] = {{.offset_x = 777}, {.offset_x = 888}};
    size_t count = 999;
    assert(!cui_shadow_preset_layers(CUI_SHADOW_MD, out, 1, &count));
    assert(!cui_shadow_preset_layers((cui_shadow_preset)-1, out, 2, &count));
    assert(!cui_shadow_preset_layers((cui_shadow_preset)8, out, 2, &count));
    assert(!cui_shadow_preset_layers(CUI_SHADOW_XS, NULL, 2, &count));
    assert(!cui_shadow_preset_layers(CUI_SHADOW_XS, out, SIZE_MAX, &count));
    assert(!cui_shadow_preset_layers(CUI_SHADOW_NONE, out, 2, NULL));
    assert(count == 999 && out[0].offset_x == 777 && out[1].offset_x == 888);
    union {
        size_t count;
        cui_drop_shadow layers[2];
    } alias;
    memset(&alias, 0x5a, sizeof alias);
    unsigned char before[sizeof alias];
    memcpy(before, &alias, sizeof alias);
    assert(!cui_shadow_preset_layers(CUI_SHADOW_SM, alias.layers, 2, &alias.count));
    assert(memcmp(before, &alias, sizeof alias) == 0);
    assert(cui_shadow_preset_layers(CUI_SHADOW_NONE, NULL, 0, &count) && count == 0);
    cui_box_shadow layer = {.kind = CUI_BOX_SHADOW_INSET, .layer = {.sigma = 77}};
    assert(!cui_box_shadow_init(NAN, 0, (cui_color){0}, &layer));
    assert(!cui_box_shadow_init(0, INFINITY, (cui_color){0}, &layer));
    assert(!cui_box_shadow_init(-4097, 0, (cui_color){0}, &layer));
    assert(!cui_box_shadow_init(0, 4097, (cui_color){0}, &layer));
    assert(!cui_box_shadow_init(0, 0, (cui_color){0}, NULL));
    assert(layer.kind == CUI_BOX_SHADOW_INSET && layer.layer.sigma == 77);
    assert(cui_box_shadow_init(-4096, 4096, (cui_color){1, 2, 3, 4}, &layer));
    assert(layer.kind == CUI_BOX_SHADOW_DROP && layer.layer.offset_x == -4096 &&
           layer.layer.offset_y == 4096 && layer.layer.sigma == 0 && layer.layer.spread == 0 &&
           layer.layer.color.a == 4);
}
static size_t shadows(cui_draw_list list, cui_draw_type type) {
    size_t n = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == type)
            n++;
    return n;
}
static void replacement(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 128});
    assert(ui);
    for (int mode = 0; mode < 4; mode++) {
        assert(cui_begin_frame(ui, 96, 96, .016f));
        cui_begin_box(
            ui, 1, (cui_box){.width = 20, .height = 20, .fixed_width = true, .fixed_height = true});
        if (mode) {
            cui_box_shadow mixed[] = {
                {{.spread = 2, .color = {0, 0, 0, 255}}, CUI_BOX_SHADOW_INSET},
                {{.sigma = 2, .color = {0, 0, 0, 255}}, CUI_BOX_SHADOW_DROP}};
            assert(cui_set_box_shadows(ui, mixed, 2, (cui_corner_radii){0}));
            assert(cui_set_shadow_preset(ui, mode == 1 ? CUI_SHADOW_NONE : CUI_SHADOW_MD,
                                         (cui_corner_radii){0}));
            if (mode == 3)
                assert(!cui_set_shadow_preset(ui, (cui_shadow_preset)99, (cui_corner_radii){0}));
        }
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(shadows(list, CUI_DRAW_INSET_SHADOW) == 0);
        assert(shadows(list, CUI_DRAW_DROP_SHADOW) == (mode < 2 ? 0 : 2));
    }
    cui_destroy(ui);
}
static void composition(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 128});
    assert(ui);
    cui_software *r = cui_software_create(96, 96, NULL);
    assert(r);
    unsigned char enabled[96 * 96 * 4];
    for (int disabled = 0; disabled < 3; disabled++) {
        if (disabled == 2) {
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 25, .y = 30}));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 25, .y = 30}));
        }
        assert(cui_begin_frame(ui, 96, 96, .016f));
        cui_begin_box(ui, 1, (cui_box){.padding = 20, .opacity = .5f, .custom_opacity = true});
        cui_begin_box(ui, 2,
                      (cui_box){.width = 20,
                                .height = 20,
                                .fixed_width = true,
                                .fixed_height = true,
                                .background = {200, 120, 80, 128}});
        assert(cui_set_shadow_preset(ui, CUI_SHADOW_2XS, (cui_corner_radii){0}));
        cui_interaction action =
            cui_interact(ui, 2, CUI_ROLE_BUTTON, cui_string("owner"), disabled, false, 0);
        if (disabled)
            assert(!action.activated);
        cui_end_box(ui);
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_software_render(r, &list, (cui_color){255, 255, 255, 255}));
        const unsigned char *pixels = cui_software_pixels(r, NULL, NULL);
        const unsigned char *edge = pixels + (40 * 96 + 25) * 4;
        assert(edge[0] == 249 && edge[1] == 249 && edge[2] == 249);
        const unsigned char *inside = pixels + (30 * 96 + 25) * 4;
        const float channels[] = {200, 120, 80};
        for (size_t k = 0; k < 3; k++)
            assert(fabsf(inside[k] - (channels[k] * (64 / 255.0f) + 249 * (1 - 64 / 255.0f))) <= 1);
        if (!disabled)
            memcpy(enabled, pixels, sizeof enabled);
        else
            assert(memcmp(enabled, pixels, sizeof enabled) == 0);
        size_t n;
        const cui_semantic *sem = cui_semantics(ui, &n);
        assert(n == 1 && sem[0].disabled == (bool)disabled);
    }
    cui_software_destroy(r);
    cui_destroy(ui);
}
int main(void) {
    factories();
    replacement();
    composition();
    return 0;
}
