/* SPDX-License-Identifier: MIT */
#include "cui/cui_content.h"
#include "cui/cui_icon.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <float.h>
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
typedef struct {
    float turns, input;
    unsigned calls;
} easing_probe;
static float fixed_turns(float input, void *user) {
    easing_probe *p = user;
    p->input = input;
    ++p->calls;
    return p->turns;
}
typedef struct {
    float size, radians;
    unsigned calls;
} sizing_probe;
static bool sizing_paint(cui_context *ui, cui_id id, const cui_theme *theme, float size,
                         cui_color color, float radians, void *user) {
    sizing_probe *p = user;
    p->size = size;
    p->radians = radians;
    ++p->calls;
    return icon(ui, id, theme, size, color, radians, (void *)"arrow-right");
}
static void source_motion(cui_context *ui, cui_software *renderer, const cui_theme *theme) {
    unsigned char zero[96 * 96 * 4], quarter[96 * 96 * 4], opposite[96 * 96 * 4];
    const float turns[] = {0, .25f, 1.25f, -.75f, -.25f, FLT_MAX, -FLT_MAX};
    for (size_t i = 0; i < sizeof turns / sizeof *turns; ++i) {
        cui_loading_motion state = {0};
        cui_loading_motion_result result;
        easing_probe easing = {.turns = turns[i]};
        sizing_probe paint = {0};
        cui_spinner_options o = cui_spinner_options_default();
        o.size = 48;
        o.ease = fixed_turns;
        o.ease_user = &easing;
        o.paint = sizing_paint;
        o.paint_user = &paint;
        o.reduced_motion = true;
        assert(cui_begin_frame(ui, 96, 96, 0));
        assert(cui_spinner_ex(ui, 21, theme, &state, o, &result));
        assert(easing.calls == 1 && easing.input == 0 && result.phase == turns[i]);
        assert(!result.active && result.next_wake_seconds == 0 && paint.calls == 1);
        assert(isfinite(paint.radians) && fabsf(paint.radians) < 6.284f);
        cui_draw_list list = cui_end_frame(ui);
        assert(!cui_needs_redraw(ui));
        assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
        unsigned w, h;
        const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
        if (!i)
            memcpy(zero, pixels, sizeof zero);
        if (i == 1) {
            memcpy(quarter, pixels, sizeof quarter);
            assert(memcmp(zero, quarter, sizeof zero));
        }
        if (i == 2 || i == 3)
            assert(!memcmp(quarter, pixels, sizeof quarter));
        if (i == 4) {
            memcpy(opposite, pixels, sizeof opposite);
            assert(memcmp(quarter, opposite, sizeof quarter));
        }
        if (i >= 5)
            assert(!memcmp(zero, pixels, sizeof zero));
        /* Rejected nonfinite reduced ease(0) is still called, paints nothing,
         * and cannot advance the caller's clock or overwrite its result. */
        const float invalid[] = {NAN, INFINITY, -INFINITY};
        for (size_t j = 0; j < sizeof invalid / sizeof *invalid; ++j) {
            cui_loading_motion old = state;
            cui_loading_motion_result saved = result;
            unsigned paints = paint.calls;
            easing.turns = invalid[j];
            o.now_seconds = 4;
            assert(cui_begin_frame(ui, 96, 96, 0));
            assert(!cui_spinner_ex(ui, 21, theme, &state, o, &result));
            assert(!memcmp(&state, &old, sizeof old) && !memcmp(&result, &saved, sizeof saved));
            assert(paint.calls == paints && easing.input == 0);
            list = cui_end_frame(ui);
            assert(list.count == 0);
        }
    }
    /* A custom easing sees natural phase during motion, then phase zero on
     * reduction; phase/result may overshoot while paint is modulo one turn. */
    cui_loading_motion state = {0};
    cui_loading_motion_result result;
    easing_probe easing = {.turns = 1.25f};
    sizing_probe paint = {0};
    cui_spinner_options o = cui_spinner_options_default();
    o.ease = fixed_turns;
    o.ease_user = &easing;
    o.paint = sizing_paint;
    o.paint_user = &paint;
    assert(cui_begin_frame(ui, 96, 96, 0));
    assert(cui_spinner_ex(ui, 21, theme, &state, o, &result));
    cui_end_frame(ui);
    o.now_seconds = .4;
    assert(cui_begin_frame(ui, 96, 96, 0));
    assert(cui_spinner_ex(ui, 21, theme, &state, o, &result));
    assert(fabsf(easing.input - .5f) < .0001f && result.active && result.phase == 1.25f);
    cui_end_frame(ui);
    cui_set_reduced_motion(ui, true);
    assert(cui_begin_frame(ui, 96, 96, 0));
    assert(cui_spinner_ex(ui, 21, theme, &state, o, &result));
    assert(easing.input == 0 && !result.active && result.phase == 1.25f);
    cui_end_frame(ui);
    cui_set_reduced_motion(ui, false);
}
static void source_sizing(cui_context *ui, const cui_theme *theme) {
    const float ratios[] = {0, .75f, .875f, 1, 1.5f};
    for (unsigned custom_paint = 0; custom_paint < 2; ++custom_paint)
        for (unsigned scale = 0; scale < 3; ++scale)
            for (unsigned kind = 0; kind <= CUI_SPINNER_LARGE; ++kind) {
                cui_spinner_sizing sizing = {.size = (cui_spinner_size)kind,
                                             .pixels = 64,
                                             .rem_size = scale == 0   ? 0
                                                         : scale == 1 ? 20
                                                                      : 32};
                float expected =
                    kind ? (sizing.rem_size ? sizing.rem_size : 16) * ratios[kind] : 64;
                cui_loading_motion state = {0};
                cui_loading_motion_result result;
                sizing_probe paint = {0};
                cui_theme sized_theme = *theme;
                sized_theme.font_size = scale == 0 ? 8 : scale == 1 ? 40 : 12;
                cui_spinner_options o = cui_spinner_options_default();
                o.size = NAN; /* Descriptor overrides legacy size, not just a multiplier. */
                o.sizing = &sizing;
                o.reduced_motion = true;
                if (custom_paint) {
                    o.paint = sizing_paint;
                    o.paint_user = &paint;
                }
                assert(cui_begin_frame(ui, 96, 96, 0));
                assert(cui_spinner_ex(ui, 31, &sized_theme, &state, o, &result));
                cui_draw_list list = cui_end_frame(ui);
                cui_rect bounds = cui_bounds(ui, 31);
                assert(bounds.w == expected && bounds.h == expected);
                if (custom_paint)
                    assert(paint.calls == 1 && paint.size == expected);
                else
                    assert(list.count == 1 && list.commands[0].type == CUI_DRAW_IMAGE &&
                           fabsf(list.commands[0].bounds.w - expected) <=
                               1); /* Raster extent rounds. */
            }
    cui_spinner_sizing invalid[] = {{.size = (cui_spinner_size)-1},
                                    {.size = (cui_spinner_size)99},
                                    {.size = CUI_SPINNER_CUSTOM, .pixels = 0},
                                    {.size = CUI_SPINNER_CUSTOM, .pixels = NAN},
                                    {.size = CUI_SPINNER_CUSTOM, .pixels = 4097},
                                    {.size = CUI_SPINNER_LARGE, .rem_size = 4096},
                                    {.size = CUI_SPINNER_MEDIUM, .rem_size = -1},
                                    {.size = CUI_SPINNER_MEDIUM, .rem_size = INFINITY}};
    cui_loading_motion state = {0};
    cui_loading_motion_result result = {.phase = 19};
    cui_loading_motion old = state;
    cui_loading_motion_result saved = result;
    assert(cui_begin_frame(ui, 96, 96, 0));
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
        cui_spinner_options o = cui_spinner_options_default();
        o.sizing = &invalid[i];
        assert(!cui_spinner_ex(ui, 31, theme, &state, o, &result));
        assert(!memcmp(&old, &state, sizeof old) && !memcmp(&saved, &result, sizeof saved));
    }
    assert(cui_end_frame(ui).count == 0);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .frame_bytes = 65536});
    cui_software *renderer = cui_software_create(96, 96, NULL);
    assert(ui && renderer);
    cui_theme theme = cui_theme_light();
    theme.text = (cui_color){220, 60, 90, 255};
    source_motion(ui, renderer, &theme);
    source_sizing(ui, &theme);
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
