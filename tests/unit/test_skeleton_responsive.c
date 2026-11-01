/* SPDX-License-Identifier: MIT */
/* Real public own-root API: first frame, resizing, pixels and registered layout. */
#include "cui/cui_skeleton_presentation.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef cui_skeleton_presentation prototype_style;
static prototype_style source_default(void) {
    return cui_skeleton_presentation_default();
}
static void prototype(cui_context *ui, cui_id id, cui_color color, cui_loading_motion *state,
                      prototype_style p, cui_loading_motion_result *out) {
    cui_skeleton_palette palette = {color};
    assert(cui_skeleton_present(ui, id, &palette, state, p, out));
}
static void geometry(cui_rect r, float x, float y, float w, float h) {
    assert(fabsf(r.x - x) < .01f && fabsf(r.y - y) < .01f);
    assert(fabsf(r.w - w) < .01f && fabsf(r.h - h) < .01f);
}
static void ordinary(cui_context *ui, cui_software *renderer) {
    const float widths[] = {320, 91, 420};
    cui_loading_motion state = {0};
    for (size_t frame = 0; frame < sizeof widths / sizeof *widths; ++frame) {
        prototype_style p = source_default();
        cui_loading_motion_result r;
        p.reduced_motion = true;
        p.root.radius = 8;
        p.root.border = (cui_color){255, 255, 255, 255};
        p.root.border_width = 1;
        p.root.custom_opacity = true;
        p.root.opacity = frame == 0 ? 0 : frame == 1 ? .2f : NAN;
        assert(cui_begin_frame(ui, 640, 300, 0));
        cui_begin_box(ui, 100,
                      (cui_box){.width = widths[frame],
                                .height = 80,
                                .padding = 4,
                                .clip = true,
                                .custom_opacity = true,
                                .opacity = .5f});
        prototype(ui, 101, (cui_color){255, 255, 255, 200}, &state, p, &r);
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        geometry(cui_bounds(ui, 101), 4, 4, widths[frame] - 8, 16);
        assert(r.opacity == 1 && !r.active && r.next_wake_seconds == 0);
        size_t rects = 0, borders = 0;
        for (size_t i = 0; i < list.count; ++i) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type == CUI_DRAW_RECT) {
                geometry(c->bounds, 4, 4, widths[frame] - 8, 16);
                assert(c->color.a == 100);
                rects++;
            }
            if (c->type == CUI_DRAW_BORDER) {
                assert(c->color.a == 127);
                borders++;
            }
        }
        assert(rects == 1 && borders == 1);
        assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
        unsigned w, h;
        const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
        assert(w == 640 && h == 300);
        assert(pixels[(12 * 640 + 20) * 4] > 0);
        assert(pixels[(12 * 640 + (unsigned)widths[frame] + 1) * 4] == 0);
        assert(pixels[(4 * 640 + 4) * 4] == 0); /* Rounded outer corner. */
    }
    /* Explicit half width is the own painted root, not a separate wrapper. */
    prototype_style p = source_default();
    p.root.width = .5f;
    p.reduced_motion = true;
    cui_loading_motion_result r;
    assert(cui_begin_frame(ui, 640, 300, 0));
    cui_begin_box(ui, 100, (cui_box){.width = 320, .height = 80, .padding = 4});
    prototype(ui, 101, (cui_color){255, 255, 255, 255}, &state, p, &r);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    geometry(cui_bounds(ui, 101), 4, 4, 156, 16);
    /* Authored transparent override replaces secondary base; root border survives. */
    p.custom_background = true;
    p.secondary = true;
    p.root.background = (cui_color){21, 34, 55, 0};
    p.root.border = (cui_color){255, 255, 255, 255};
    p.root.border_width = 1;
    assert(cui_begin_frame(ui, 640, 300, 0));
    cui_begin_box(ui, 100, (cui_box){.width = 320, .height = 80});
    prototype(ui, 101, (cui_color){255, 255, 255, 200}, &state, p, &r);
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_software_render(renderer, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(8 * 640 + 20) * 4] == 0);
    assert(pixels[0] > 0);
}
static cui_scroll_body_layout_v1 body(cui_context *ui, float width, bool percent_height,
                                      cui_loading_motion *state) {
    cui_begin_box(ui, 200, (cui_box){.width = width, .height = 80});
    cui_begin_box(ui, 201, (cui_box){.width = width, .height = 80, .clip = true});
    cui_begin_box(ui, 202, (cui_box){.width = width, .height = -1});
    cui_begin_box(ui, 203, (cui_box){.width = width, .height = -1});
    cui_begin_absolute_box(ui, 204, (cui_box){.clip = true}, (cui_rect){0, 0, -1, -1});
    cui_begin_box(ui, 205, (cui_box){.width = width, .height = 200});
    prototype_style p = source_default();
    p.reduced_motion = true;
    if (percent_height) {
        p.root.height = .5f;
        p.root.height_percent = true;
    }
    cui_loading_motion_result result;
    prototype(ui, 206, (cui_color){255, 255, 255, 255}, state, p, &result);
    for (unsigned i = 0; i < 6; ++i)
        cui_end_box(ui);
    return (cui_scroll_body_layout_v1){.struct_size = sizeof(cui_scroll_body_layout_v1),
                                       .version = 1,
                                       .mode = CUI_SCROLL_BODY_EXPLICIT,
                                       .panel = 200,
                                       .outer_viewport = 201,
                                       .column = 202,
                                       .body_frame = 203,
                                       .viewport = 204,
                                       .content = 205,
                                       .requested_height = 80,
                                       .max_height = 80};
}
static void registered(cui_context *ui) {
    const float widths[] = {320, 91, 420};
    cui_loading_motion state = {0};
    float outer_y = 0, body_y = 0;
    for (size_t i = 0; i < sizeof widths / sizeof *widths; ++i) {
        assert(cui_begin_frame(ui, 640, 300, 0));
        cui_scroll_body_layout_v1 record = body(ui, widths[i], false, &state);
        assert(cui_layout_scroll_body_v1(ui, &record, &outer_y, &body_y));
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK && list.count > 0);
        geometry(cui_bounds(ui, 206), 0, 0, widths[i], 16);
        assert(outer_y == 0 && body_y == 0);
    }
    assert(cui_begin_frame(ui, 640, 300, 0));
    cui_scroll_body_layout_v1 record = body(ui, 320, true, &state);
    assert(cui_layout_scroll_body_v1(ui, &record, &outer_y, &body_y));
    assert(cui_end_frame(ui).count == 0);
    assert(cui_last_error(ui) == CUI_ERROR_LAYOUT);
    cui_clear_error(ui); /* No claim that clearing makes failed geometry valid. */
    assert(cui_begin_frame(ui, 640, 300, 0));
    record = body(ui, 91, false, &state);
    assert(cui_layout_scroll_body_v1(ui, &record, &outer_y, &body_y));
    assert(cui_end_frame(ui).count > 0 && cui_last_error(ui) == CUI_OK);
    geometry(cui_bounds(ui, 206), 0, 0, 91, 16);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 640, .height = 300});
    cui_software *renderer = cui_software_create(640, 300, NULL);
    assert(ui && renderer);
    ordinary(ui, renderer);
    registered(ui);
    cui_software_destroy(renderer);
    cui_destroy(ui);
    puts("Skeleton public own-root ordinary/registered responsive PASS");
    return 0;
}
