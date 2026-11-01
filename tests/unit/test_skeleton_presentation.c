/* SPDX-License-Identifier: MIT */
#include "cui/cui_content.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include "../fixtures/skeleton/frozen.c.inc"
static float invalid(float t, void *user) {
    (void)t;
    (*(unsigned *)user)++;
    return NAN;
}
typedef struct {
    cui_theme *theme;
    cui_skeleton_palette *palette;
    unsigned calls;
} mutation;
static float mutate(float t, void *user) {
    mutation *m = user;
    m->theme->secondary = (cui_color){13, 57, 91, 200};
    m->palette->background = (cui_color){111, 122, 133, 244};
    m->calls++;
    return t;
}
static void callback_timing(cui_context *ui) {
    for (unsigned route = 0; route < 3; ++route) {
        cui_theme theme = cui_theme_light();
        theme.secondary = (cui_color){21, 34, 55, 128};
        cui_skeleton_palette palette = {theme.secondary};
        mutation m = {&theme, &palette, 0};
        cui_skeleton_options o = cui_skeleton_options_default();
        o.width = 20; o.height = 16; o.ease = mutate; o.ease_user = &m;
        cui_loading_motion state = {0}; cui_loading_motion_result result;
        assert(cui_begin_frame(ui, 64, 48, 0));
        if (route == 0)
            assert(frozen_skeleton_ex(ui, 1, &theme, &state, o, &result));
        else if (route == 1)
            assert(cui_skeleton_ex(ui, 1, &theme, &state, o, &result));
        else
            assert(cui_skeleton_with_palette(ui, 1, &palette, &state, o, &result));
        cui_draw_list list = cui_end_frame(ui);
        assert(m.calls == 1 && palette.background.r == 111);
        assert(list.count == 1 && list.commands[0].type == CUI_DRAW_RECT);
        cui_color c = list.commands[0].color;
        if (route < 2)
            assert(c.r == 13 && c.g == 57 && c.b == 91 && c.a == 200);
        else
            assert(c.r == 21 && c.g == 34 && c.b == 55 && c.a == 128);
    }
}
static void same_color(cui_color a, cui_color b) {
    assert(a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a);
}
static void snapshot(cui_context *ui, cui_software *software, int route, cui_theme *theme,
                     cui_skeleton_palette palette, cui_skeleton_options options,
                     cui_loading_motion *state, cui_loading_motion_result *result,
                     unsigned char pixels[64 * 48 * 4], cui_draw_command *rect) {
    assert(cui_begin_frame(ui, 64, 48, 0));
    cui_begin_box(ui, 99, (cui_box){.width = 40, .height = 32, .clip = true,
                                  .custom_opacity = true, .opacity = .5f});
    if (route == 0)
        assert(frozen_skeleton_ex(ui, 1, theme, state, options, result));
    else if (route == 1)
        assert(cui_skeleton_ex(ui, 1, theme, state, options, result));
    else if (route == 2)
        assert(cui_skeleton_with_palette(ui, 1, &palette, state, options, result));
    else if (route == 3)
        frozen_skeleton(ui, 1, theme, options.width, options.height, options.radius,
                        (float)options.now_seconds, options.secondary, options.reduced_motion);
    else
        cui_skeleton(ui, 1, theme, options.width, options.height, options.radius,
                     (float)options.now_seconds, options.secondary, options.reduced_motion);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    size_t found = 0;
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_RECT) {
            *rect = list.commands[i];
            found++;
        }
    /* Alpha0 may be culled; exact output pixels still prove transparency. */
    if (palette.background.a || route != 2)
        assert(found == 1);
    size_t count;
    cui_semantics(ui, &count);
    assert(count == 0 && !cui_focused(ui));
    assert(cui_software_render(software, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *data = cui_software_pixels(software, &w, &h);
    assert(w == 64 && h == 48);
    memcpy(pixels, data, 64 * 48 * 4);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 64, .height = 48});
    cui_software *software = cui_software_create(64, 48, NULL);
    assert(ui && software);
    callback_timing(ui);
    cui_theme theme = cui_theme_light();
    theme.secondary = (cui_color){255, 255, 255, 200};
    cui_skeleton_palette palette;
    assert(cui_skeleton_palette_from_theme(&theme, &palette));
    cui_skeleton_options o = cui_skeleton_options_default();
    o.width = 48; o.height = 24; o.radius = 8;
    unsigned char reference[64 * 48 * 4], actual[64 * 48 * 4];
    const double times[] = {0, .25, .5, .75, 1, 1.25, 1.5, 1.75, 2, 4.001};
    for (unsigned secondary = 0; secondary < 2; ++secondary) {
        cui_loading_motion baseline = {0}, current = {0}, colored = {0};
        o.secondary = secondary != 0;
        for (size_t i = 0; i < sizeof times / sizeof *times; ++i) {
            o.now_seconds = times[i];
            cui_loading_motion_result expected, result;
            cui_draw_command a = {0}, b = {0};
            snapshot(ui, software, 0, &theme, palette, o, &baseline, &expected, reference, &a);
            for (int route = 1; route <= 2; ++route) {
                snapshot(ui, software, route, &theme, palette, o,
                         route == 1 ? &current : &colored, &result, actual, &b);
                assert(!memcmp(reference, actual, sizeof actual));
                assert(expected.phase == result.phase && expected.opacity == result.opacity &&
                       expected.active == result.active &&
                       expected.next_wake_seconds == result.next_wake_seconds);
                assert(a.type == b.type && a.bounds.x == b.bounds.x && a.bounds.y == b.bounds.y &&
                       a.bounds.w == b.bounds.w && a.bounds.h == b.bounds.h && a.radius == b.radius);
                same_color(a.color, b.color);
            }
            /* Independent triangular/quadratic pulse and single alpha truncation. */
            double phase = fmod(times[i], 2) / 2;
            double triangle = phase < .5 ? phase * 2 : (1 - phase) * 2;
            double eased = triangle < .5 ? 2 * triangle * triangle
                                         : 1 - 2 * (1 - triangle) * (1 - triangle);
            assert(fabs(result.opacity - (1 - eased * .5)) < 1e-6);
            assert(b.color.a == (uint8_t)((uint8_t)(200 * result.opacity *
                                                    (secondary ? .5f : 1)) * .5f));
            /* Rounded corner clear, interior painted, parent clip excludes x>=40. */
            assert(actual[0] == 0 && actual[(12 * 64 + 12) * 4] > 0);
            assert(actual[(12 * 64 + 42) * 4] == 0);
        }
    }
    for (int route = 3; route <= 4; ++route) {
        cui_loading_motion s = {0}; cui_loading_motion_result r = {0}; cui_draw_command c = {0};
        snapshot(ui, software, route, &theme, palette, o, &s, &r,
                 route == 3 ? reference : actual, &c);
    }
    assert(!memcmp(reference, actual, sizeof actual));
    palette.background = (cui_color){21, 34, 55, 0};
    cui_loading_motion s = {0}; cui_loading_motion_result r = {0}; cui_draw_command c = {0};
    snapshot(ui, software, 2, &theme, palette, o, &s, &r, actual, &c);
    for (size_t i = 0; i < sizeof actual; i += 4)
        assert(actual[i] == 0 && actual[i + 1] == 0 && actual[i + 2] == 0 && actual[i + 3] == 255);
    o.reduced_motion = true;
    snapshot(ui, software, 2, &theme, palette, o, &s, &r, actual, &c);
    assert(!r.active && r.opacity == 1 && r.next_wake_seconds == 0);
    double retained_start = s.started;
    /* Omission does not silently reset caller state; explicit reset starts at now. */
    assert(cui_begin_frame(ui, 64, 48, 0));
    assert(cui_end_frame(ui).count == 0);
    palette.background = (cui_color){255, 255, 255, 200};
    o.reduced_motion = false; o.now_seconds = 7.25;
    snapshot(ui, software, 2, &theme, palette, o, &s, &r, actual, &c);
    assert(s.started == retained_start && r.opacity < 1);
    s = (cui_loading_motion){0};
    snapshot(ui, software, 2, &theme, palette, o, &s, &r, actual, &c);
    assert(s.started == 7.25 && r.opacity == 1);
    o.reduced_motion = true; o.secondary = false;
    o.width = 24; o.height = 24; o.radius = 100;
    snapshot(ui, software, 2, &theme, palette, o, &s, &r, actual, &c);
    assert(actual[0] == 0 && actual[(12 * 64 + 12) * 4] > 0);
    o.radius = 0;
    snapshot(ui, software, 2, &theme, palette, o, &s, &r, actual, &c);
    assert(actual[0] > 0);
    /* Own validation, sampler failure and callback effects are separate. */
    unsigned calls = 0;
    o.reduced_motion = false; o.ease = invalid; o.ease_user = &calls;
    unsigned char old_state[sizeof s], old_result[sizeof r];
    memcpy(old_state, &s, sizeof s); memcpy(old_result, &r, sizeof r);
    assert(cui_begin_frame(ui, 64, 48, 0));
    assert(!cui_skeleton_with_palette(ui, 1, &palette, &s, o, &r));
    assert(calls == 1 && !memcmp(old_state, &s, sizeof s) && !memcmp(old_result, &r, sizeof r));
    o.ease = NULL; o.width = NAN;
    assert(!cui_skeleton_with_palette(ui, 1, &palette, &s, o, &r));
    o.width = 20;
    assert(!cui_skeleton_with_palette(ui, 0, &palette, &s, o, &r));
    assert(!cui_skeleton_with_palette(ui, 1, NULL, &s, o, &r));
    assert(cui_end_frame(ui).count == 0);
    assert(!memcmp(old_state, &s, sizeof s) && !memcmp(old_result, &r, sizeof r));
    cui_software_destroy(software); cui_destroy(ui);
    return 0;
}
