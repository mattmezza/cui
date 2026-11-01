/* SPDX-License-Identifier: MIT */
#include "cui/cui_skeleton_presentation.h"
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {
    cui_skeleton_palette *palette;
    cui_skeleton_presentation *presentation;
    unsigned calls;
    bool fail;
} observer;
static float easing(float phase, void *user) {
    observer *o = user;
    o->calls++;
    o->palette->background = (cui_color){201, 202, 203, 204};
    o->presentation->root.background = (cui_color){101, 102, 103, 104};
    o->presentation->root.width = 1;
    return o->fail ? NAN : phase;
}
static void snapshots(cui_context *ui) {
    for (unsigned custom = 0; custom < 2; ++custom) {
        cui_skeleton_palette palette = {{21, 34, 55, 200}};
        cui_skeleton_presentation p = cui_skeleton_presentation_default();
        p.root.width_percent = false; p.root.width = 80;
        p.root.background = (cui_color){13, 57, 91, 180};
        p.root.border = (cui_color){255, 255, 255, 255}; p.root.border_width = 1;
        p.root.custom_opacity = true; p.root.opacity = NAN;
        p.custom_background = custom != 0; p.secondary = true;
        observer o = {&palette, &p, 0, false}; p.ease = easing; p.ease_user = &o;
        cui_loading_motion state = {0}; cui_loading_motion_result result;
        assert(cui_begin_frame(ui, 640, 300, 0));
        assert(cui_skeleton_present(ui, 1, &palette, &state, p, &result));
        cui_draw_list list = cui_end_frame(ui);
        assert(o.calls == 1 && palette.background.r == 201 && p.root.width == 1);
        assert(cui_bounds(ui, 1).w == 80 && result.opacity == 1);
        size_t rectangles = 0, borders = 0;
        for (size_t i = 0; i < list.count; ++i) {
            cui_draw_command c = list.commands[i];
            if (c.type == CUI_DRAW_RECT) {
                assert(c.color.r == (custom ? 13 : 21));
                assert(c.color.g == (custom ? 57 : 34));
                assert(c.color.b == (custom ? 91 : 55));
                assert(c.color.a == (custom ? 180 : 100));
                rectangles++;
            }
            if (c.type == CUI_DRAW_BORDER) { assert(c.color.a == 255); borders++; }
        }
        assert(rectangles == 1 && borders == 1);
        size_t count; cui_semantics(ui, &count); assert(count == 0 && !cui_focused(ui));
    }
}
static void failures(cui_context *ui) {
    for (unsigned which = 0; which < 15; ++which) {
        cui_skeleton_palette palette = {{21, 34, 55, 0}};
        cui_skeleton_presentation p = cui_skeleton_presentation_default();
        cui_loading_motion state = {0};
        cui_loading_motion_result result;
        memset(&result, 0x5a, sizeof result);
        observer o = {&palette, &p, 0, false}; p.ease = easing; p.ease_user = &o;
        cui_context *context = ui; cui_id id = 1;
        const cui_skeleton_palette *input = &palette;
        cui_loading_motion *model = &state; cui_loading_motion_result *output = &result;
        switch (which) {
        case 0: p.duration = NAN; break;
        case 1: p.duration = -1; break;
        case 2: p.duration = 61; break;
        case 3: p.now_seconds = NAN; break;
        case 4: p.now_seconds = -1; break;
        case 5: p.now_seconds = 1e12 + 1; break;
        case 6: state.ready = true; state.started = NAN; break;
        case 7: state.ready = true; state.started = 2; state.last_time = 1; break;
        case 8: context = NULL; break;
        case 9: id = 0; break;
        case 10: input = NULL; break;
        case 11: model = NULL; break;
        case 12: output = NULL; break;
        case 13: o.fail = true; break;
        case 14: state.ready = true; state.started = -1; break;
        }
        unsigned char before_state[sizeof state], before_result[sizeof result];
        memcpy(before_state, &state, sizeof state); memcpy(before_result, &result, sizeof result);
        assert(cui_begin_frame(ui, 640, 300, 0));
        assert(!cui_skeleton_present(context, id, input, model, p, output));
        assert(!memcmp(before_state, &state, sizeof state));
        assert(!memcmp(before_result, &result, sizeof result));
        assert(o.calls == (which == 13 ? 1u : 0u));
        assert(cui_end_frame(ui).count == 0 && cui_last_error(ui) == CUI_OK);
    }
}
static void pulse_border(cui_context *ui) {
    const double times[] = {0, .25, .5, .75, 1};
    cui_loading_motion state = {.ready = true, .started = 0, .last_time = 0};
    for (size_t i = 0; i < sizeof times / sizeof *times; ++i) {
        cui_skeleton_palette palette = {{255, 255, 255, 201}};
        cui_skeleton_presentation p = cui_skeleton_presentation_default();
        p.root.width = 40; p.root.width_percent = false;
        p.root.background = (cui_color){13, 57, 91, 173};
        p.root.border = (cui_color){255, 255, 255, 221}; p.root.border_width = 1;
        p.custom_background = true; p.secondary = true; p.now_seconds = times[i];
        cui_loading_motion_result result;
        assert(cui_begin_frame(ui, 640, 300, 0));
        cui_begin_box(ui, 10, (cui_box){.width = 80, .height = 80,
                                       .custom_opacity = true, .opacity = .5f});
        assert(cui_skeleton_present(ui, 12, &palette, &state, p, &result));
        cui_end_box(ui); cui_draw_list list = cui_end_frame(ui);
        double triangle = times[i]; /* First half of period2 has triangle=t. */
        double eased = triangle < .5 ? 2 * triangle * triangle
                                     : 1 - 2 * (1 - triangle) * (1 - triangle);
        float opacity = (float)(1 - eased * .5);
        assert(result.opacity == opacity);
        size_t backgrounds = 0, borders = 0;
        for (size_t j = 0; j < list.count; ++j) {
            cui_draw_command c = list.commands[j];
            if (c.type == CUI_DRAW_RECT) {
                assert(c.color.a == (uint8_t)(173 * (opacity * .5f)));
                backgrounds++;
            }
            if (c.type == CUI_DRAW_BORDER) {
                assert(c.color.a == (uint8_t)(221 * (opacity * .5f)));
                borders++;
            }
        }
        assert(backgrounds == 1 && borders == 1);
    }
}
static void grow_and_precedence(cui_context *ui) {
    const cui_box roots[] = {
        {.width = -1, .height = 16},
        {.width = .5f, .height = 16, .width_percent = true, .fixed_width = true, .max_width = 10},
        {.width = 120, .height = 16},
        {.width = 0, .height = 16, .fixed_width = true}
    };
    const float expected[] = {280, 160, 120, 0};
    for (size_t i = 0; i < sizeof roots / sizeof *roots; ++i) {
        cui_skeleton_palette palette = {{255, 255, 255, 255}};
        cui_skeleton_presentation p = cui_skeleton_presentation_default();
        p.root = roots[i]; p.reduced_motion = true;
        cui_loading_motion state = {0}; cui_loading_motion_result result;
        assert(cui_begin_frame(ui, 640, 300, 0));
        cui_begin_box(ui, 10, (cui_box){.width = 320, .height = 80, .row = true});
        cui_begin_box(ui, 11, (cui_box){.width = 40, .height = 16}); cui_end_box(ui);
        assert(cui_skeleton_present(ui, 12, &palette, &state, p, &result));
        cui_end_box(ui); (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        assert(fabsf(cui_bounds(ui, 12).w - expected[i]) < .01f);
    }
    /* Layout-only suppression does not imply undoing successful owner sampling. */
    cui_skeleton_palette palette = {{255, 255, 255, 255}};
    cui_skeleton_presentation p = cui_skeleton_presentation_default();
    p.root.layout_only = true; p.root.inert = true; p.now_seconds = 8;
    cui_loading_motion state = {0}; cui_loading_motion_result result;
    assert(cui_begin_frame(ui, 640, 300, 0));
    assert(cui_skeleton_present(ui, 12, &palette, &state, p, &result));
    assert(cui_end_frame(ui).count == 0 && state.ready && state.started == 8);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 300});
    assert(ui);
    cui_skeleton_presentation d = cui_skeleton_presentation_default();
    assert(d.root.width == 1 && d.root.width_percent && d.root.height == 16 && d.duration == 2);
    snapshots(ui); failures(ui); pulse_border(ui); grow_and_precedence(ui);
    cui_destroy(ui);
    return 0;
}
