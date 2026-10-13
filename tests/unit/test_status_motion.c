/* SPDX-License-Identifier: MIT */
#include "cui/cui_content.h"
#include "cui/cui_icon.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static float linear(float phase, void *user) {
    (*(unsigned *)user)++;
    return phase;
}
static float invalid(float phase, void *user) {
    (void)phase;
    (void)user;
    return NAN;
}
typedef struct {
    unsigned calls;
    float radians;
    bool fail;
} paint_state;
static bool paint(cui_context *ui, cui_id id, const cui_theme *theme, float size, cui_color color,
                  float radians, void *user) {
    (void)theme;
    paint_state *s = user;
    s->calls++;
    s->radians = radians;
    if (s->fail)
        return false;
    cui_begin_box(ui, id, (cui_box){.width = size, .height = size, .background = color});
    cui_end_box(ui);
    return true;
}
static bool custom_icon(cui_context *ui, cui_id id, const cui_theme *theme, float size,
                        cui_color color, float radians, void *user) {
    (*(unsigned *)user)++;
    return cui_icon_named(ui, id, theme, cui_string("loader-circle"),
                          (cui_icon_options){.size = CUI_ICON_CUSTOM,
                                             .custom_size = size,
                                             .custom_color = true,
                                             .color = color,
                                             .custom_transform = true,
                                             .transform = cui_icon_rotation(radians)}) ==
           CUI_ICON_OK;
}
static void pure(void) {
    cui_loading_motion s = {0};
    cui_loading_motion_result r;
    assert(cui_loading_motion_sample(&s, 10, .8f, false, false, NULL, NULL, &r));
    assert(r.phase == 0 && r.active && r.next_wake_seconds > 10 && r.next_wake_seconds < 10.02);
    assert(cui_loading_motion_sample(&s, 10.2, .8f, false, false, NULL, NULL, &r));
    assert(fabsf(r.phase - .125f) < .0001f);
    assert(cui_loading_motion_sample(&s, 9, .8f, false, false, NULL, NULL, &r));
    assert(s.last_time == 10.2 && fabsf(r.phase - .125f) < .0001f);
    unsigned calls = 0;
    assert(cui_loading_motion_sample(&s, 10.4, .8f, false, false, linear, &calls, &r));
    assert(calls == 1 && fabsf(r.phase - .5f) < .0001f);
    assert(cui_loading_motion_sample(&s, 11, 2, false, true, NULL, NULL, &r));
    assert(r.opacity == .5f);
    assert(cui_loading_motion_sample(&s, 12, 2, false, true, NULL, NULL, &r));
    assert(r.opacity == 1);
    assert(cui_loading_motion_sample(&s, 12, 2, true, true, invalid, NULL, &r));
    assert(!r.active && r.next_wake_seconds == 0 && r.opacity == 1);
    cui_loading_motion old = s;
    cui_loading_motion_result saved = r;
    assert(!cui_loading_motion_sample(&s, 12, 2, false, true, invalid, NULL, &r));
    assert(!memcmp(&s, &old, sizeof s) && !memcmp(&r, &saved, sizeof r));
    const float periods[] = {0, -1, NAN, INFINITY, 61};
    for (size_t i = 0; i < sizeof periods / sizeof *periods; i++)
        assert(!cui_loading_motion_sample(&s, 12, periods[i], false, false, NULL, NULL, &r));
    assert(!cui_loading_motion_sample(&s, -1, 2, false, false, NULL, NULL, &r));
    assert(!cui_loading_motion_sample(&s, 1e13, 2, false, false, NULL, NULL, &r));
    assert(!cui_loading_motion_sample(&s, NAN, 2, false, false, NULL, NULL, &r));
    assert(!memcmp(&s, &old, sizeof s) && !memcmp(&r, &saved, sizeof r));
    s = (cui_loading_motion){0};
    assert(cui_loading_motion_sample(&s, 20, 2, false, false, NULL, NULL, &r) && r.phase == 0);
    s.last_time = 19;
    assert(!cui_loading_motion_sample(&s, 20, 2, false, false, NULL, NULL, &r));
}
int main(void) {
    pure();
    /* Existing Progress source motion remains one-second quadratic, and reduced
     * indeterminate progress is a centered 35% segment without animation. */
    cui_progress_geometry progress = cui_progress_measure(.9f, true, true, .75, false);
    assert(progress.animating && fabsf(progress.start - .5f) < .0001f &&
           fabsf(progress.length - .375f) < .0001f);
    progress = cui_progress_measure(.9f, true, true, .75, true);
    assert(!progress.animating && progress.start == .325f && progress.length == .35f);
    cui_progress_motion transition = {0};
    cui_progress_motion_result pr;
    assert(cui_progress_animate(&transition, 0, 0, .3f, false, &pr));
    assert(cui_progress_animate(&transition, 1, 0, .3f, false, &pr) && pr.active);
    assert(cui_progress_animate(&transition, 1, .31, .3f, false, &pr));
    assert(!pr.active && pr.value == 1);
    cui_context *ui = cui_create((cui_config){.width = 160, .height = 100});
    assert(ui);
    cui_theme t = cui_theme_light();
    cui_loading_motion spinner = {0}, skeleton = {0};
    cui_loading_motion_result r;
    assert(cui_spinner_options_default().size == 16);
    cui_spinner_options o = cui_spinner_options_default();
    o.size = 32;
    o.duration = 0;
    assert(cui_begin_frame(ui, 160, 100, 0));
    assert(cui_spinner_ex(ui, 1, &t, &spinner, o, &r) && r.phase == 0 && r.active);
    cui_draw_list list = cui_end_frame(ui);
    bool bitmap = false;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type == CUI_DRAW_IMAGE) {
            const cui_bitmap *b = &list.commands[i].bitmap.bitmap;
            size_t covered = 0;
            for (unsigned y = 0; y < b->height; y++)
                for (unsigned x = 0; x < b->width; x++)
                    covered += b->pixels[y * b->stride + x * 4 + 3] > 0;
            assert(covered > 0);
            bitmap = true;
        }
    assert(bitmap && !cui_needs_redraw(ui));
    cui_software *svg_renderer = cui_software_create(160, 100, NULL);
    assert(svg_renderer);
    assert(cui_software_render(svg_renderer, &list, (cui_color){0, 0, 0, 255}));
    unsigned sw, sh;
    const unsigned char *sp = cui_software_pixels(svg_renderer, &sw, &sh);
    size_t ink = 0;
    for (size_t i = 0; i < (size_t)sw * sh; i++)
        ink += sp[i * 4] > 0 || sp[i * 4 + 1] > 0 || sp[i * 4 + 2] > 0;
    assert(ink > 0 && ink < 32 * 32);
    cui_software_destroy(svg_renderer);
    size_t count;
    cui_semantics(ui, &count);
    assert(count == 0);
    paint_state p = {0};
    o.paint = paint;
    o.paint_user = &p;
    o.custom_color = true;
    o.color = (cui_color){19, 87, 123, 255};
    o.now_seconds = .2;
    assert(cui_begin_frame(ui, 160, 100, 0));
    assert(cui_spinner_ex(ui, 1, &t, &spinner, o, &r));
    assert(p.calls == 1 && fabsf(p.radians - .785398f) < .0001f);
    cui_skeleton_options sk = cui_skeleton_options_default();
    sk.secondary = true;
    sk.duration = 0;
    sk.now_seconds = 0;
    assert(cui_skeleton_ex(ui, 2, &t, &skeleton, sk, &r));
    list = cui_end_frame(ui);
    cui_semantics(ui, &count);
    assert(count == 0);
    cui_software *renderer = cui_software_create(160, 100, NULL);
    assert(renderer);
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    assert(w == 160 && h == 100 && pixels[0] == 19 && pixels[1] == 87 && pixels[2] == 123);
    cui_software_destroy(renderer);
    cui_loading_motion before = spinner;
    cui_loading_motion_result saved = r;
    assert(cui_begin_frame(ui, 160, 100, 0));
    const float bad[] = {-1, NAN, INFINITY, 61};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; i++) {
        o.duration = bad[i];
        assert(!cui_spinner_ex(ui, 1, &t, &spinner, o, &r));
        sk.duration = bad[i];
        assert(!cui_skeleton_ex(ui, 2, &t, &skeleton, sk, &r));
    }
    o.duration = 0;
    o.ease = invalid;
    assert(!cui_spinner_ex(ui, 1, &t, &spinner, o, &r));
    assert(!memcmp(&before, &spinner, sizeof before) && !memcmp(&saved, &r, sizeof saved));
    o.ease = NULL;
    p.fail = true;
    assert(!cui_spinner_ex(ui, 1, &t, &spinner, o, &r));
    assert(!memcmp(&before, &spinner, sizeof before));
    list = cui_end_frame(ui);
    assert(list.count == 0);
    p.fail = false;
    cui_set_reduced_motion(ui, true);
    assert(cui_begin_frame(ui, 160, 100, 0));
    assert(cui_spinner_ex(ui, 1, &t, &spinner, o, &r) && !r.active && r.next_wake_seconds == 0);
    sk.duration = 0;
    assert(cui_skeleton_ex(ui, 2, &t, &skeleton, sk, &r) && !r.active && r.opacity == 1);
    cui_end_frame(ui);
    assert(!cui_needs_redraw(ui));
    assert(cui_begin_frame(ui, 160, 100, 0));
    cui_spinner(ui, 3, &t, 24, .5f, true);
    cui_skeleton(ui, 4, &t, 100, 16, 4, .5f, false, true);
    cui_end_frame(ui);
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].role == CUI_ROLE_STATUS && !sem[0].details.busy);
    cui_set_reduced_motion(ui, false);
    unsigned icon_calls = 0;
    o.paint = custom_icon;
    o.paint_user = &icon_calls;
    o.now_seconds = .6;
    assert(cui_begin_frame(ui, 160, 100, 0));
    assert(cui_spinner_ex(ui, 5, &t, &spinner, o, &r) && icon_calls == 1);
    list = cui_end_frame(ui);
    renderer = cui_software_create(160, 100, NULL);
    assert(renderer);
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    pixels = cui_software_pixels(renderer, &w, &h);
    size_t colored = 0;
    for (size_t i = 0; i < (size_t)w * h; i++)
        colored += pixels[i * 4] == 19 && pixels[i * 4 + 1] == 87 && pixels[i * 4 + 2] == 123;
    assert(colored > 0 && !cui_needs_redraw(ui));
    cui_software_destroy(renderer);
    cui_destroy(ui);
    ui = cui_create((cui_config){.width = 100, .height = 100, .max_resources = 1});
    assert(ui);
    spinner = (cui_loading_motion){0};
    cui_loading_motion rejected = {0};
    o = cui_spinner_options_default();
    assert(cui_begin_frame(ui, 100, 100, 0));
    assert(cui_spinner_ex(ui, 1, &t, &spinner, o, &r));
    saved = r;
    before = rejected;
    assert(!cui_spinner_ex(ui, 2, &t, &rejected, o, &r));
    assert(!memcmp(&before, &rejected, sizeof before) && !memcmp(&saved, &r, sizeof saved));
    cui_end_frame(ui);
    cui_destroy(ui);
    return 0;
}
