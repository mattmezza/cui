/* SPDX-License-Identifier: MIT */
/* Test-only ordinary-flow candidate. No production presenter/hook/API. */
#include "cui/cui.h"
#include "cui/cui_draw_v2.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
enum { ROOT = 100, TRACK = 103, FILL = 104, START = 101, END = 102, SURFACE = 200, RING = 300 };
typedef struct {
    bool vertical, grow, range, reverse;
    float length, rem, padding;
    double start, end;
} plan;
static void box(cui_context *ui, cui_id id, cui_box b) {
    cui_begin_box(ui, id, b);
    cui_end_box(ui);
}
static void decoration(cui_context *ui, cui_id id, float rem) {
    cui_begin_absolute_box(ui, id + RING,
                           (cui_box){.border = {90, 0, 0, 255}, .border_width = 3, .radius = 1000},
                           (cui_rect){-3, -3, rem + 6, rem + 6});
    cui_end_box(ui);
    float inside = fmaxf(0, rem - 2);
    cui_begin_absolute_box(ui, id + SURFACE,
                           (cui_box){.background = {80, 0, 0, 255}, .radius = 1000},
                           (cui_rect){1, 1, inside, inside});
    cui_end_box(ui);
}
static void thumb(cui_context *ui, plan p, cui_id id, double fraction) {
    float track = p.rem * .375f;
    /* This row/column follows the fill, so its cross origin is track_extent. */
    cui_begin_box(ui, 0,
                  (cui_box){.row = !p.vertical,
                            .width = p.vertical ? 0 : -1,
                            .height = p.vertical ? -1 : 0,
                            .fixed_width = p.vertical,
                            .fixed_height = !p.vertical});
    box(ui, 0,
        (cui_box){.width = p.vertical ? 0 : (float)fraction,
                  .height = p.vertical ? (float)(1 - fraction) : 0,
                  .width_percent = !p.vertical,
                  .height_percent = p.vertical,
                  .fixed_width = p.vertical,
                  .fixed_height = !p.vertical});
    cui_begin_box(ui, 0,
                  (cui_box){.width = 0, .height = 0, .fixed_width = true, .fixed_height = true});
    cui_rect relative = p.vertical ? (cui_rect){-track - 5, 8 - p.rem, p.rem, p.rem}
                                   : (cui_rect){-8, -track - 5, p.rem, p.rem};
    cui_begin_absolute_box(ui, id, (cui_box){.background = {70, 0, 0, 255}, .radius = 1000},
                           relative);
    decoration(ui, id, p.rem);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
}
static void compose(cui_context *ui, plan p) {
    float cross = 1.5f * p.rem, track = .375f * p.rem;
    cui_begin_box(ui, ROOT,
                  (cui_box){.row = p.vertical,
                            .align_x = CUI_ALIGN_CENTER,
                            .align_y = CUI_ALIGN_CENTER,
                            .width = p.vertical ? cross
                                     : p.grow   ? -1
                                                : p.length,
                            .height = p.vertical ? (p.grow ? -1 : p.length) : cross,
                            .fixed_width = p.vertical || !p.grow,
                            .fixed_height = !p.vertical || !p.grow});
    (void)cui_interact(ui, ROOT, CUI_ROLE_GROUP, cui_string("Slider"), false, false, 0);
    cui_set_semantic_details(ui, ROOT, (cui_semantic_details){.focusable = true});
    if (p.range)
        (void)cui_interact(ui, START, CUI_ROLE_SLIDER, cui_string("Start"), false, false,
                           (float)p.start);
    (void)cui_interact(ui, END, CUI_ROLE_SLIDER, cui_string("End"), false, false, (float)p.end);
    cui_begin_box(ui, TRACK,
                  (cui_box){.row = p.vertical,
                            .width = p.vertical ? track : -1,
                            .height = p.vertical ? -1 : track,
                            .fixed_width = p.vertical,
                            .fixed_height = !p.vertical,
                            .background = {40, 0, 0, 255}});
    cui_begin_box(ui, 0,
                  (cui_box){.row = !p.vertical,
                            .width = p.vertical ? track : -1,
                            .height = p.vertical ? -1 : track,
                            .fixed_width = p.vertical,
                            .fixed_height = !p.vertical});
    double a = p.range     ? p.start
               : p.reverse ? p.end
                           : 0,
           b = p.range     ? p.end
               : p.reverse ? 1
                           : p.end;
    double before = p.vertical ? 1 - b : a;
    box(ui, 0,
        (cui_box){.width = p.vertical ? track : (float)before,
                  .height = p.vertical ? (float)before : track,
                  .width_percent = !p.vertical,
                  .height_percent = p.vertical,
                  .fixed_width = p.vertical,
                  .fixed_height = !p.vertical});
    box(ui, FILL,
        (cui_box){.width = p.vertical ? track : (float)(b - a),
                  .height = p.vertical ? (float)(b - a) : track,
                  .width_percent = !p.vertical,
                  .height_percent = p.vertical,
                  .fixed_width = p.vertical,
                  .fixed_height = !p.vertical,
                  .background = {60, 0, 0, 255}});
    cui_end_box(ui);
    if (p.range)
        thumb(ui, p, START, p.start);
    thumb(ui, p, END, p.end);
    cui_end_box(ui);
    cui_end_box(ui);
}
static void parent(cui_context *ui, plan p, bool clip) {
    float cross = 1.5f * p.rem;
    cui_begin_box(ui, 900,
                  (cui_box){.row = !p.vertical,
                            .width = (p.vertical ? cross : p.length) + 2 * p.padding,
                            .height = (p.vertical ? p.length : cross) + 2 * p.padding,
                            .fixed_width = true,
                            .fixed_height = true,
                            .padding = p.padding,
                            .clip = clip});
    compose(ui, p);
    cui_end_box(ui);
}
static void rectangle(cui_rect r, cui_rect expected) {
    if (fabsf(r.x - expected.x) > .02f || fabsf(r.y - expected.y) > .02f ||
        fabsf(r.w - expected.w) > .02f || fabsf(r.h - expected.h) > .02f)
        fprintf(stderr, "actual {%g,%g,%g,%g}, expected {%g,%g,%g,%g}\n", r.x, r.y, r.w, r.h,
                expected.x, expected.y, expected.w, expected.h);
    assert(fabsf(r.x - expected.x) < .02f && fabsf(r.y - expected.y) < .02f &&
           fabsf(r.w - expected.w) < .02f && fabsf(r.h - expected.h) < .02f);
}
static cui_rect expected_thumb(plan p, double fraction) {
    float track = .375f * p.rem, origin = p.padding + (1.5f * p.rem - track) * .5f;
    return p.vertical
               ? (cui_rect){origin - 5, p.padding + (float)(1 - fraction) * p.length + 8 - p.rem,
                            p.rem, p.rem}
               : (cui_rect){p.padding + (float)fraction * p.length - 8, origin - 5, p.rem, p.rem};
}
static void check_paint(cui_draw_list list, unsigned red, cui_rect expected, unsigned count) {
    unsigned found = 0;
    for (size_t i = 0; i < list.count; ++i)
        if (list.commands[i].type == CUI_DRAW_RECT && list.commands[i].color.r == red) {
            rectangle(list.commands[i].bounds, expected);
            ++found;
        }
    assert(found == count);
}
static void painted_at(cui_draw_list list, cui_draw_type type, unsigned red, cui_rect expected) {
    unsigned found = 0;
    for (size_t i = 0; i < list.count; ++i) {
        const cui_draw_command *c = &list.commands[i];
        if (c->type == type && c->color.r == red && fabsf(c->bounds.x - expected.x) < .02f &&
            fabsf(c->bounds.y - expected.y) < .02f) {
            rectangle(c->bounds, expected);
            ++found;
        }
    }
    assert(found == 1);
}
static void check(cui_context *ui, plan p, cui_draw_list list) {
    float cross = 1.5f * p.rem, track = .375f * p.rem, origin = p.padding + (cross - track) * .5f;
    rectangle(cui_bounds(ui, TRACK), p.vertical ? (cui_rect){origin, p.padding, track, p.length}
                                                : (cui_rect){p.padding, origin, p.length, track});
    double a = p.range     ? p.start
               : p.reverse ? p.end
                           : 0,
           b = p.range     ? p.end
               : p.reverse ? 1
                           : p.end;
    cui_rect fill = p.vertical ? (cui_rect){origin, p.padding + (float)(1 - b) * p.length, track,
                                            (float)(b - a) * p.length}
                               : (cui_rect){p.padding + (float)a * p.length, origin,
                                            (float)(b - a) * p.length, track};
    rectangle(cui_bounds(ui, FILL), fill);
    check_paint(list, 60, fill, 1);
    for (unsigned t = p.range ? 0 : 1; t < 2; ++t) {
        cui_id id = t ? END : START;
        cui_rect outer = expected_thumb(p, t ? p.end : p.start);
        rectangle(cui_bounds(ui, id), outer);
        cui_rect surface = {outer.x + 1, outer.y + 1, p.rem - 2, p.rem - 2};
        cui_rect ring = {outer.x - 3, outer.y - 3, p.rem + 6, p.rem + 6};
        rectangle(cui_bounds(ui, id + SURFACE), surface);
        rectangle(cui_bounds(ui, id + RING), ring);
        painted_at(list, CUI_DRAW_RECT, 70, outer);
        painted_at(list, CUI_DRAW_RECT, 80, surface);
        painted_at(list, CUI_DRAW_BORDER, 90, ring);
    }
}
static cui_frame_v2_options v2_options(void) {
    return (cui_frame_v2_options){.size = sizeof(cui_frame_v2_options),
                                  .support = {.size = sizeof(cui_draw_v2_support),
                                              .abi_version = 2,
                                              .background_output = 1,
                                              .features = 1,
                                              .max_commands = 128,
                                              .max_backgrounds = 1,
                                              .max_pixels = 1024,
                                              .max_pixels_per_background = 1024,
                                              .cpu_bytes = 8192,
                                              .texture_bytes = 8192,
                                              .transfer_bytes = 8192,
                                              .metadata_bytes = 8192},
                                  .required_features = 1,
                                  .command_bytes = 128 * sizeof(cui_draw_command_v2),
                                  .background_bytes = 8192};
}
static void ordinary(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 128});
    assert(ui);
    unsigned frames = 0;
    for (unsigned vertical = 0; vertical < 2; ++vertical)
        for (unsigned grow = 0; grow < 2; ++grow)
            for (unsigned r = 0; r < 3; ++r)
                for (unsigned resize = 0; resize < 3; ++resize)
                    for (unsigned mode = 0; mode < 3; ++mode) {
                        const float rems[] = {12, 16, 24}, lengths[] = {80, 300, 120};
                        plan p = {vertical != 0, grow != 0, mode == 0, mode == 2, lengths[resize],
                                  rems[r],       10,        .25,       .75};
                        assert(cui_begin_frame(ui, 600, 600, 0));
                        parent(ui, p, false);
                        cui_draw_list list = cui_end_frame(ui);
                        assert(cui_last_error(ui) == CUI_OK);
                        check(ui, p, list);
                        ++frames;
                        /* Route to actual just-resolved thumb, then resize before composition. */
                        cui_rect old = cui_bounds(ui, END);
                        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                               .x = old.x + old.w * .5f,
                                                               .y = old.y + old.h * .5f}));
                        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                                               .x = old.x + old.w * .5f,
                                                               .y = old.y + old.h * .5f}));
                        p.length += 37;
                        assert(cui_begin_frame(ui, 600, 600, 0));
                        assert(cui_event_target(ui, 0) == END && cui_event_target(ui, 1) == END);
                        parent(ui, p, false);
                        list = cui_end_frame(ui);
                        assert(cui_last_error(ui) == CUI_OK);
                        check(ui, p, list);
                        ++frames;
                    }
    /* Endpoints, clipping and a genuine matching V2 frame, no brush consumer claim. */
    plan p = {false, true, true, false, 200, 16, 0, 0, 1};
    assert(cui_begin_frame(ui, 600, 600, 0));
    parent(ui, p, true);
    cui_draw_list list = cui_end_frame(ui);
    check(ui, p, list);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 204, .y = 12}));
    assert(cui_begin_frame(ui, 600, 600, 0));
    assert(cui_event_target(ui, 0) != END);
    parent(ui, p, true);
    (void)cui_end_frame(ui);
    cui_frame_v2_options v = v2_options();
    cui_draw_list_v2 output;
    for (unsigned axis = 0; axis < 2; ++axis) {
        p.vertical = axis != 0;
        assert(cui_begin_frame_v2(ui, 600, 600, 0, &v));
        parent(ui, p, false);
        assert(cui_end_frame_v2(ui, &output) && output.count);
        rectangle(cui_bounds(ui, END), expected_thumb(p, p.end));
    }
    printf("PASS %u ordinary first/current-layout frames plus endpoints/clip/V2\n", frames);
    cui_destroy(ui);
}
static void registered_scroll(void) {
    for (unsigned axis = 0; axis < 2; ++axis) {
        cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 128});
        assert(ui);
        assert(cui_begin_frame(ui, 600, 600, 0));
        cui_begin_box(ui, 500,
                      (cui_box){.width = 400,
                                .height = 100,
                                .fixed_width = true,
                                .fixed_height = true,
                                .clip = true});
        cui_begin_box(
            ui, 501,
            (cui_box){.width = 400, .height = 240, .fixed_width = true, .fixed_height = true});
        plan p = {axis != 0, axis == 0, true, false, 120, 16, 0, .25, .75};
        compose(ui, p);
        cui_end_box(ui);
        cui_end_box(ui);
        float offset = 0;
        cui_scroll_clamp_v1 clamp = {sizeof clamp, 1, CUI_SCROLL_CLAMP_Y, 0, 500, 501};
        assert(cui_layout_scroll_clamp_v1(ui, &clamp, &offset));
        cui_draw_list list = cui_end_frame(ui);
        if (!axis) {
            assert(cui_last_error(ui) == CUI_OK && list.count);
            rectangle(cui_bounds(ui, END), (cui_rect){292, 4, 16, 16});
        } else {
            assert(cui_last_error(ui) == CUI_ERROR_LAYOUT && !list.count);
            assert(!cui_accessibility_numeric_snapshot_ready(ui));
            cui_clear_error(ui);
            assert(!cui_accessibility_numeric_snapshot_ready(ui));
            assert(cui_begin_frame(ui, 600, 600, 0) && !cui_frame_input_snapshot_ready(ui));
            (void)cui_end_frame(ui);
            assert(cui_accessibility_numeric_snapshot_ready(ui));
        }
        cui_destroy(ui);
    }
    puts("PASS registered scroll horizontal / explicit vertical rejection and fresh recovery");
}
int main(void) {
    ordinary();
    registered_scroll();
    return 0;
}
