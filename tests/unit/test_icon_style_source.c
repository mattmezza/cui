/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_content.h"
#include "cui/cui_icon.h"
#include "cui/cui_label.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static const char svg[] = "<svg viewBox=\"0 0 24 24\"><path d=\"M2 3h8v5H2z\"/></svg>";
static unsigned long hash(cui_bitmap b) {
    unsigned long h = 5381;
    for (unsigned y = 0; y < b.height; y++)
        for (unsigned x = 0; x < b.width * 4; x++)
            h = h * 33 + b.pixels[y * b.stride + x];
    return h;
}
static bool same_tint(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static void text_defaults(cui_context *ui, const cui_theme *theme, cui_icon_state *state) {
    const cui_color teal = {14, 152, 137, 255}, violet = {119, 24, 189, 255};
    const cui_color transparent = {14, 18, 22, 0};
    cui_box row = {.row = true, .width = 440, .gap = 2};
    cui_loading_motion motion = {0};
    cui_loading_motion_result sample;
    assert(!cui_set_text_defaults(ui, (cui_text_defaults){.font_size = 12}));
    assert(!cui_get_text_defaults(NULL).custom_color);
    assert(cui_begin_frame(ui, 450, 160, 0));
    assert(!cui_set_text_defaults(ui, (cui_text_defaults){.font_size = 12}));
    cui_begin_box(ui, 100, row);
    assert(cui_set_text_defaults(
        ui, (cui_text_defaults){.font_size = 26, .custom_color = true, .color = teal}));
    assert(!cui_set_text_defaults(ui, (cui_text_defaults){.font_size = NAN}));
    assert(cui_get_text_defaults(ui).font_size == 26);
    assert(cui_icon(ui, 101, theme, state, (cui_icon_options){0}) == CUI_ICON_OK);
    cui_label(ui, theme, cui_string("Scope"));
    assert(cui_label_source(
        ui, theme,
        (cui_label_options){.primary = {"Source", 6}, .no_wrap = true, .semantic_id = 111}));
    cui_text(ui, cui_string("Explicit"), 12, violet);
    cui_begin_box(ui, 102, (cui_box){.row = true});
    assert(cui_set_text_defaults(ui, (cui_text_defaults){.font_size = 32}));
    assert(cui_get_text_defaults(ui).font_size == 32);
    assert(cui_set_text_defaults(ui, (cui_text_defaults){.custom_color = true, .color = violet}));
    assert(cui_get_text_defaults(ui).font_size == 26);
    assert(cui_icon_named(ui, 103, theme, cui_string("check"),
                          (cui_icon_options){.inherited_text_size = 18,
                                             .custom_inherited_color = true,
                                             .inherited_color = teal,
                                             .custom_color = true,
                                             .color = transparent}) == CUI_ICON_OK);
    assert(cui_icon_named(ui, 104, theme, cui_string("check"),
                          (cui_icon_options){.size = CUI_ICON_SMALL, .rem_size = 16}) ==
           CUI_ICON_OK);
    cui_end_box(ui);
    assert(cui_icon_named(ui, 105, theme, cui_string("check"),
                          (cui_icon_options){.custom_inherited_color = true,
                                             .inherited_color = violet}) == CUI_ICON_OK);
    assert(cui_spinner_ex(ui, 106, theme, &motion, cui_spinner_options_default(), &sample));
    cui_begin_absolute_box(ui, 107, (cui_box){0}, (cui_rect){8, 90, 40, 40});
    assert(cui_icon_named(ui, 108, theme, cui_string("check"), (cui_icon_options){0}) ==
           CUI_ICON_OK);
    cui_end_box(ui);
    cui_begin_portal(ui, 109, (cui_box){0}, (cui_rect){60, 90, 40, 40}, 1);
    assert(cui_get_text_defaults(ui).font_size == 26);
    assert(cui_icon_named(ui, 110, theme, cui_string("check"), (cui_icon_options){0}) ==
           CUI_ICON_OK);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_begin_box(ui, 112, row);
    assert(cui_get_text_defaults(ui).font_size == 0 && !cui_get_text_defaults(ui).custom_color);
    assert(cui_icon_named(ui, 113, theme, cui_string("check"), (cui_icon_options){0}) ==
           CUI_ICON_OK);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    const cui_id ids[] = {101, 103, 104, 105, 106, 108, 110, 113};
    const float sizes[] = {26, 18, 14, 26, 16, 26, 26, theme->font_size};
    const cui_color colors[] = {teal, transparent, violet, violet, teal, teal, teal, theme->text};
    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++) {
        cui_rect bounds = cui_bounds(ui, ids[i]);
        assert(bounds.w == sizes[i] && bounds.h == sizes[i]);
        bool found = false;
        for (size_t j = 0; j < draw.count; j++) {
            const cui_draw_command *command = &draw.commands[j];
            if (command->type == CUI_DRAW_IMAGE && command->bounds.x == bounds.x &&
                command->bounds.y == bounds.y && command->bounds.w == bounds.w &&
                same_tint(command->bitmap.tint, colors[i]))
                found = true;
        }
        if (!found)
            fprintf(stderr, "Missing icon %llu at %.1f,%.1f (%.1f x %.1f), tint %u/%u/%u/%u\n",
                    (unsigned long long)ids[i], bounds.x, bounds.y, bounds.w, bounds.h, colors[i].r,
                    colors[i].g, colors[i].b, colors[i].a);
        assert(found);
    }
    size_t scoped = 0, explicit = 0;
    for (size_t i = 0; i < draw.count; i++) {
        const cui_draw_command *command = &draw.commands[i];
        if (command->type == CUI_DRAW_TEXT) {
            scoped += command->font_size == 26 && same_tint(command->color, teal);
            explicit += command->font_size == 12 && same_tint(command->color, violet);
        }
    }
    assert(scoped == 2 && explicit == 1);
    assert(!cui_get_text_defaults(ui).custom_color);
    assert(cui_begin_frame(ui, 160, 100, 0));
    cui_begin_box(ui, 100, (cui_box){0});
    assert(cui_get_text_defaults(ui).font_size == 0 && !cui_get_text_defaults(ui).custom_color);
    cui_end_box(ui);
    cui_end_frame(ui);
}
int main(void) {
    cui_context *ui =
        cui_create((cui_config){.max_elements = 128, .max_commands = 256, .frame_bytes = 65536});
    cui_software *r = cui_software_create(160, 100, NULL);
    assert(ui && r);
    cui_theme t = cui_theme_light();
    cui_icon_state state = {0};
    assert(cui_icon_set_data(&state, cui_string(svg)));
    text_defaults(ui, &t, &state);
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
