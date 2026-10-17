/* SPDX-License-Identifier: MIT */
#include "cui/cui_content.h"
#include "cui/cui_icon.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
static bool icon(cui_context *ui, cui_id id, const cui_theme *theme, float size, cui_color color,
                 float radians, void *user) {
    const cui_box style = {.radius = 3, .background = {9, 12, 15, 255}};
    return cui_icon_named(ui, id, theme, cui_string(user),
                          (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                             .custom_size = size,
                                             .custom_color = true,
                                             .color = color,
                                             .custom_transform = true,
                                             .transform = cui_icon_rotation(radians),
                                             .style = &style}) == CUI_ICON_OK;
}
static uint64_t sum(const unsigned char *data) {
    uint64_t result = 0;
    for (size_t i = 0; i < 96 * 96; ++i)
        result += data[i * 4] + data[i * 4 + 1] + data[i * 4 + 2];
    return result;
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .frame_bytes = 65536});
    cui_software *renderer = cui_software_create(96, 96, NULL);
    assert(ui && renderer);
    cui_theme theme = cui_theme_light();
    theme.text = (cui_color){220, 60, 90, 255};
    const float sizes[] = {12, 14, 16, 24, 64};
    for (size_t n = 0; n < sizeof sizes / sizeof *sizes; ++n) {
        cui_loading_motion state = {0};
        cui_loading_motion_result motion;
        cui_spinner_options options = cui_spinner_options_default();
        options.size = sizes[n];
        options.reduced_motion = true;
        assert(cui_begin_frame(ui, 96, 96, 0));
        assert(cui_spinner_ex(ui, 11, &theme, &state, options, &motion));
        cui_draw_list list = cui_end_frame(ui);
        assert(list.count == 1 && list.commands[0].type == CUI_DRAW_IMAGE);
        assert(list.commands[0].bounds.w == sizes[n] && list.commands[0].bounds.h == sizes[n]);
        assert(list.commands[0].bitmap.tint.r == 220);
        assert(!motion.active && motion.phase == 0 && motion.next_wake_seconds == 0);
        assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
        unsigned w, h;
        assert(sum(cui_software_pixels(renderer, &w, &h)) > 0);
        size_t semantics;
        cui_semantics(ui, &semantics);
        assert(semantics == 0 && !cui_needs_redraw(ui));
    }
    /* Full rotation changes real SVG coverage, then reduced motion restores
     * precisely the first static frame. The custom icon retains Icon styling. */
    const char *names[] = {"loader", "loader-circle", "arrow-right"};
    for (size_t n = 0; n < 3; ++n) {
        cui_loading_motion state = {0};
        cui_loading_motion_result motion;
        unsigned char initial[96 * 96 * 4];
        uint64_t opaque = 0;
        for (int pass = 0; pass < 5; ++pass) {
            cui_spinner_options options = cui_spinner_options_default();
            options.size = 48;
            options.now_seconds = pass ? .2 : 0;
            options.reduced_motion = pass >= 2;
            options.custom_color = true;
            options.color = (cui_color){20, 190, 80, pass == 4 ? 0 : 255};
            if (n) {
                options.paint = icon;
                options.paint_user = (void *)names[n];
            }
            assert(cui_begin_frame(ui, 96, 96, 0));
            cui_begin_box(ui, 10,
                          (cui_box){.width = 60,
                                    .height = 60,
                                    .custom_opacity = true,
                                    .opacity = pass == 3 ? .5f : 1});
            assert(cui_spinner_ex(ui, 11, &theme, &state, options, &motion));
            cui_end_box(ui);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
            unsigned w, h;
            const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
            if (!pass) {
                memcpy(initial, pixels, sizeof initial);
                opaque = sum(pixels);
            }
            if (pass == 1)
                assert(memcmp(initial, pixels, sizeof initial));
            if (pass == 2)
                assert(!memcmp(initial, pixels, sizeof initial));
            if (pass == 3)
                assert(sum(pixels) < opaque * .55 && sum(pixels) > opaque * .4);
            if (pass == 4 && !n)
                assert(sum(pixels) == 0);
            if (pass >= 2)
                assert(!motion.active && motion.next_wake_seconds == 0);
            assert(cui_last_error(ui) == CUI_OK && !cui_needs_redraw(ui));
        }
    }
    /* Parent clipping is applied to rotated artwork before presentation. */
    cui_loading_motion state = {0};
    cui_loading_motion_result motion;
    cui_spinner_options options = cui_spinner_options_default();
    options.size = 64;
    assert(cui_begin_frame(ui, 96, 96, 0));
    cui_begin_box(ui, 20, (cui_box){.width = 32, .height = 32, .clip = true});
    assert(cui_spinner_ex(ui, 21, &theme, &state, options, &motion));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    assert(sum(pixels) > 0);
    for (unsigned y = 0; y < h; ++y)
        for (unsigned x = 0; x < w; ++x)
            if (x >= 32 || y >= 32)
                assert(!pixels[(y * w + x) * 4] && !pixels[(y * w + x) * 4 + 1] &&
                       !pixels[(y * w + x) * 4 + 2]);
    cui_software_destroy(renderer);
    cui_destroy(ui);
    return 0;
}
