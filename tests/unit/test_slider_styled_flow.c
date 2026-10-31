/* SPDX-License-Identifier: MIT */
/* Test-only ordinary-flow candidate. No production presenter/hook/API. */
#include "cui/cui.h"
#include "cui/cui_draw_v2.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
/* Test-only presentation inputs; no production spring or Styled API. */
static float ring_progress = 1, parent_opacity = 1;
static cui_color ring_source_color = {90, 43, 177, 7};
static bool ring_square;
typedef struct {
    float stroke, radius;
    cui_color color;
} ring_payload;
static bool paint_ring(const cui_prepaint_info *info, const void *copied,
                       cui_prepaint_writer *writer) {
    const ring_payload *p = copied;
    return cui_prepaint_emit(writer,
                             &(cui_draw_command){.type = CUI_DRAW_BORDER,
                                                 .bounds = info->bounds,
                                                 .color = p->color,
                                                 .stroke = p->stroke,
                                                 .radius = p->radius},
                             false);
}
static void decoration(cui_context *ui, cui_id id, float rem) {
    float width = 3 * ring_progress;
    if (width > 0) {
        cui_begin_absolute_box(ui, id + RING, (cui_box){0},
                               (cui_rect){-width, -width, rem + 2 * width, rem + 2 * width});
        ring_payload payload = {.stroke = width,
                                .radius = ring_square ? 0 : rem * .5f + width,
                                .color = ring_source_color};
        payload.color.a = (unsigned char)lroundf(.5f * ring_progress * 255); /* REPLACE alpha. */
        assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = paint_ring,
                                                           .payload = &payload,
                                                           .payload_bytes = sizeof payload,
                                                           .command_budget = 3}));
        memset(&payload, 0xdd, sizeof payload); /* Core must own its copied bytes. */
        cui_end_box(ui);
    }
    float inside = fmaxf(0, rem - 2);
    cui_begin_absolute_box(
        ui, id + SURFACE,
        (cui_box){.background = {80, 0, 0, 255}, .radius = ring_square ? 0 : 1000},
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
    cui_begin_absolute_box(
        ui, id, (cui_box){.background = {70, 0, 0, 255}, .radius = ring_square ? 0 : 1000},
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
                            .clip = clip,
                            .custom_opacity = true,
                            .opacity = parent_opacity});
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
static bool annulus(double x, double y, cui_rect thumb, double width, bool square) {
    if (width <= 0)
        return false;
    if (square)
        return x >= thumb.x - width && x < thumb.x + thumb.w + width && y >= thumb.y - width &&
               y < thumb.y + thumb.h + width &&
               !(x >= thumb.x && x < thumb.x + thumb.w && y >= thumb.y && y < thumb.y + thumb.h);
    double dx = x - (thumb.x + thumb.w * .5), dy = y - (thumb.y + thumb.h * .5);
    double inner = thumb.w * .5, outer = inner + width, r2 = dx * dx + dy * dy;
    /* Half-open horizontal coverage: exact left edge is inside, right edge
     * outside. rem24/progress.5 has a pixel center exactly on that boundary. */
    bool in_outer = r2 < outer * outer || (r2 == outer * outer && dx < 0);
    bool in_inner = r2 < inner * inner || (r2 == inner * inner && dx < 0);
    return in_outer && !in_inner;
}
/* Independent point-in-annulus + double source-over oracle. Do not call the
 * backend's scanline/shape/span helpers or use its rendered image as expected. */
static void software_ring(void) {
    enum { WIDTH = 200, HEIGHT = 100, BYTES = WIDTH * HEIGHT * 4 };
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 128});
    assert(ui);
    cui_software *r = cui_software_create(WIDTH, HEIGHT, NULL);
    assert(r);
    unsigned char *baseline = malloc(BYTES);
    assert(baseline);
    unsigned cases = 0;
    for (unsigned rem = 0; rem < 2; ++rem)
        for (unsigned square = 0; square < 2; ++square)
            for (unsigned clipping = 0; clipping < 2; ++clipping)
                for (unsigned opacity = 0; opacity < 2; ++opacity)
                    for (unsigned alpha = 0; alpha < 3; ++alpha)
                        for (unsigned progress = 0; progress < 5; ++progress) {
                            const unsigned char source_alpha[] = {0, 37, 255};
                            ring_progress = progress * .25f;
                            ring_square = square != 0;
                            parent_opacity = opacity ? .5f : 1;
                            ring_source_color.a = source_alpha[alpha];
                            plan p = {false,
                                      true,
                                      false,
                                      false,
                                      128,
                                      rem ? 24 : 16,
                                      clipping ? 0 : 16,
                                      0,
                                      clipping ? 1 : .5};
                            assert(cui_begin_frame(ui, WIDTH, HEIGHT, 0));
                            parent(ui, p, clipping != 0);
                            cui_draw_list list = cui_end_frame(ui);
                            assert(cui_last_error(ui) == CUI_OK);
                            cui_draw_command *without = malloc(list.count * sizeof *without);
                            assert(without);
                            size_t n = 0;
                            unsigned rings = 0;
                            cui_rect thumb = expected_thumb(p, p.end);
                            double stroke = 3 * ring_progress;
                            unsigned char raw = (unsigned char)lround(.5 * ring_progress * 255);
                            unsigned char effective = (unsigned char)(raw * parent_opacity);
                            for (size_t k = 0; k < list.count; ++k) {
                                const cui_draw_command *c = &list.commands[k];
                                if (c->type == CUI_DRAW_BORDER &&
                                    c->color.r == ring_source_color.r &&
                                    c->color.g == ring_source_color.g &&
                                    c->color.b == ring_source_color.b) {
                                    ++rings;
                                    rectangle(c->bounds, (cui_rect){thumb.x - (float)stroke,
                                                                    thumb.y - (float)stroke,
                                                                    p.rem + 2 * (float)stroke,
                                                                    p.rem + 2 * (float)stroke});
                                    assert(c->stroke == (float)stroke && c->color.a == effective);
                                    assert(c->radius == (square ? 0 : p.rem * .5f + (float)stroke));
                                } else
                                    without[n++] = *c;
                            }
                            assert(rings == (progress ? 1u : 0u));
                            cui_draw_list background = {without, n, WIDTH, HEIGHT};
                            assert(cui_software_render(r, &background, (cui_color){0}));
                            memcpy(baseline, cui_software_pixels(r, NULL, NULL), BYTES);
                            assert(cui_software_render(r, &list, (cui_color){0}));
                            const unsigned char *actual = cui_software_pixels(r, NULL, NULL);
                            unsigned changed = 0;
                            for (unsigned y = 0; y < HEIGHT; ++y)
                                for (unsigned x = 0; x < WIDTH; ++x) {
                                    size_t index = ((size_t)y * WIDTH + x) * 4;
                                    bool in_clip = !clipping || (x < p.length && y < 1.5f * p.rem);
                                    bool ink = in_clip &&
                                               annulus(x + .5, y + .5, thumb, stroke, square != 0);
                                    if (!ink) {
                                        if (memcmp(actual + index, baseline + index, 4))
                                            fprintf(stderr,
                                                    "outside ring rem%g square%u clip%u opacity%g "
                                                    "sourcealpha%u progress%g pixel%u,%u "
                                                    "thumb%g,%g,%g: actual%u,%u,%u,%u "
                                                    "baseline%u,%u,%u,%u\n",
                                                    p.rem, square, clipping, parent_opacity,
                                                    source_alpha[alpha], ring_progress, x, y,
                                                    thumb.x, thumb.y, thumb.w, actual[index],
                                                    actual[index + 1], actual[index + 2],
                                                    actual[index + 3], baseline[index],
                                                    baseline[index + 1], baseline[index + 2],
                                                    baseline[index + 3]);
                                        assert(!memcmp(actual + index, baseline + index, 4));
                                        continue;
                                    }
                                    double sa = effective / 255., ba = baseline[index + 3] / 255.,
                                           a = sa + ba * (1 - sa);
                                    const unsigned char source[] = {ring_source_color.r,
                                                                    ring_source_color.g,
                                                                    ring_source_color.b};
                                    for (unsigned c = 0; c < 4; ++c) {
                                        double value =
                                            c == 3 ? a * 255
                                            : a    ? ((source[c] * sa +
                                                       baseline[index + c] * ba * (1 - sa)) /
                                                      a)
                                                   : 0;
                                        int expected = (int)lround(value);
                                        assert(abs((int)actual[index + c] - expected) <= 1);
                                    }
                                    changed += memcmp(actual + index, baseline + index, 4) != 0;
                                }
                            assert(progress ? changed > 0 : changed == 0);
                            free(without);
                            ++cases;
                        }
    printf("PASS %u real software ring images: fractional stroke/source alpha "
           "replacement/clip/opacity/rounded and square\n",
           cases);
    ring_progress = parent_opacity = 1;
    ring_square = false;
    free(baseline);
    cui_software_destroy(r);
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
    software_ring();
    registered_scroll();
    return 0;
}
