/* SPDX-License-Identifier: MIT */
#include "cui/cui_icon.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static const char svg[] = "<svg viewBox=\"0 0 24 24\"><path d=\"M2 3h8v5H2z\"/></svg>";
static unsigned long hash(cui_bitmap b) {
    unsigned long h = 5381;
    for (unsigned y = 0; y < b.height; y++)
        for (unsigned x = 0; x < b.width * 4; x++)
            h = h * 33 + b.pixels[y * b.stride + x];
    return h;
}
int main(void) {
    cui_context *ui =
        cui_create((cui_config){.max_elements = 128, .max_commands = 256, .frame_bytes = 65536});
    cui_software *r = cui_software_create(160, 100, NULL);
    assert(ui && r);
    cui_theme t = cui_theme_light();
    cui_icon_state state = {0};
    assert(cui_icon_set_data(&state, cui_string(svg)));
    for (int pass = 0; pass < 7; pass++) {
        cui_box style = {0};
        cui_icon_constraints limits = {0};
        cui_icon_options o = {.size = CUI_ICON_CUSTOM,
                              .custom_size = 16,
                              .style = &style,
                              .constraints = &limits,
                              .accessible_name = {"Image", 5}};
        float expectedw = 16, expectedh = 16;
        if (pass == 1)
            style.padding = 4;
        if (pass == 2) {
            style.custom_padding = true;
            style.padding_edges = (cui_border_edges){24, 8, 4, 20};
            expectedw = 32;
            expectedh = 24;
        }
        if (pass == 3) {
            style.border_width = 10;
            expectedw = expectedh = 20;
        }
        if (pass == 4) {
            style.max_width = 8;
            expectedw = 8;
        }
        if (pass == 5) {
            limits.custom_max_width = true;
            limits.max_width = 0;
            expectedw = 0;
        }
        if (pass == 6) {
            limits = (cui_icon_constraints){.custom_min_width = true,
                                            .min_width = 30,
                                            .custom_max_width = true,
                                            .max_width = 8};
            expectedw = 30;
        }
        assert(cui_begin_frame(ui, 160, 100, 0));
        assert(cui_icon(ui, 1, &t, &state, o) == CUI_ICON_OK);
        cui_draw_list d = cui_end_frame(ui);
        cui_rect b = cui_bounds(ui, 1);
        assert(b.w == expectedw && b.h == expectedh);
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        assert(count == 1 && sem[0].bounds.w == expectedw && sem[0].tab_stop_disabled);
        assert(cui_software_render(r, &d, (cui_color){0, 0, 0, 255}));
        if (pass == 1) {
            assert(d.count == 1 && d.commands[0].bounds.x == b.x && d.commands[0].bounds.w == 16);
        }
    }
    /* Each source size combines with small/large padding and a cap. */
    const float sizes[] = {16, 12, 14, 16, 24, 16};
    for (int size = CUI_ICON_INHERITED; size <= CUI_ICON_CUSTOM; size++)
        for (int pad = 0; pad < 2; pad++) {
            cui_box style = {.padding = pad ? 24 : 4, .max_width = 10};
            cui_icon_constraints limits = {.custom_min_height = true, .min_height = 20};
            assert(cui_begin_frame(ui, 160, 100, 0));
            assert(cui_icon(ui, 1, &t, &state,
                            (cui_icon_options){.size = (cui_icon_size)size,
                                               .custom_size = 16,
                                               .style = &style,
                                               .constraints = &limits}) == CUI_ICON_OK);
            cui_end_frame(ui);
            cui_rect b = cui_bounds(ui, 1);
            assert(b.w == fmaxf(fminf(sizes[size], 10), pad ? 48 : 8));
            assert(b.h == fmaxf(fmaxf(sizes[size], 20), pad ? 48 : 8));
        }
    /* One explicit axis suppresses inherited sizing of the other axis, but
     * padding still supplies its source border-box floor. */
    cui_box padded = {.padding = 4};
    assert(cui_begin_frame(ui, 160, 100, 0));
    assert(cui_icon(ui, 1, &t, &state,
                    (cui_icon_options){.custom_width = true, .width = 32, .style = &padded}) ==
           CUI_ICON_OK);
    cui_end_frame(ui);
    assert(cui_bounds(ui, 1).w == 32 && cui_bounds(ui, 1).h == 8);
    /* One state, three different raster versions, consumed together after all submissions. */
    assert(cui_begin_frame(ui, 160, 100, 0));
    cui_begin_box(ui, 10, (cui_box){.row = true, .width = 160, .height = 100});
    for (int i = 0; i < 3; i++) {
        cui_box style = {.custom_opacity = true, .opacity = i == 2 ? .5f : 1};
        assert(cui_icon(ui, 20 + i, &t, &state,
                        (cui_icon_options){
                            .size = CUI_ICON_CUSTOM,
                            .custom_size = 24 + 8 * i,
                            .custom_color = true,
                            .color = {i == 0 ? 255 : 0, i == 1 ? 255 : 0, i == 2 ? 255 : 0, 255},
                            .custom_transform = true,
                            .transform = cui_icon_rotation(i * 1.57079632679f),
                            .style = &style,
                            .accessible_name = {"Image", 5}}) == CUI_ICON_OK);
    }
    cui_end_box(ui);
    cui_draw_list d = cui_end_frame(ui);
    assert(d.count == 3);
    cui_bitmap old[3];
    unsigned long hashes[3];
    for (int i = 0; i < 3; i++) {
        old[i] = d.commands[i].bitmap.bitmap;
        hashes[i] = hash(old[i]);
        assert(old[i].width >= (unsigned)(24 + 8 * i) && old[i].width <= (unsigned)(26 + 8 * i));
    }
    size_t semantic_count;
    const cui_semantic *named = cui_semantics(ui, &semantic_count);
    assert(semantic_count == 3);
    for (int i = 0; i < 3; i++)
        assert(named[i].bounds.w == 24 + 8 * i && named[i].bounds.h == 24 + 8 * i &&
               named[i].tab_stop_disabled);
    assert(cui_icon_set_empty(&state));
    for (int i = 0; i < 3; i++)
        assert(hash(old[i]) == hashes[i]);
    assert(cui_software_render(r, &d, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *p = cui_software_pixels(r, &w, &h);
    size_t red = 0, green = 0, blue = 0;
    for (size_t i = 0; i < (size_t)w * h; i++) {
        red += p[i * 4] > 200;
        green += p[i * 4 + 1] > 200;
        blue += p[i * 4 + 2] > 80 && p[i * 4 + 2] < 150;
    }
    assert(red && green && blue);
    cui_icon_state_collect(&state);
    assert(cui_begin_frame(ui, 160, 100, 0));
    const char *names[] = {"filter", "icons/trash-2.svg", "more-vertical"};
    for (int i = 0; i < 3; i++)
        assert(cui_icon_named(ui, 30 + i, &t, cui_string(names[i]), (cui_icon_options){0}) ==
               CUI_ICON_OK);
    cui_end_frame(ui);
    assert(cui_begin_frame(ui, 160, 100, 0));
    cui_icon_constraints invalid = {.custom_max_width = true, .max_width = NAN};
    assert(cui_icon_named(ui, 40, &t, cui_string("check"),
                          (cui_icon_options){.constraints = &invalid}) == CUI_ICON_INVALID);
    assert(cui_end_frame(ui).count == 0);
    cui_icon_state_destroy(&state);
    cui_software_destroy(r);
    cui_destroy(ui);
    return 0;
}
