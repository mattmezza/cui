/* SPDX-License-Identifier: MIT */
#include "cui/cui_plot_view.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){(float)text.length * size / 2, size};
}
static bool closef(float a, float b) {
    return fabsf(a - b) < .001f;
}
static const cui_color red = {255, 0, 0, 255}, green = {0, 255, 0, 255}, blue = {0, 0, 255, 255};
static void axes(cui_context *ui) {
    cui_plot_axis_text x[] = {
        {.text = {"ten", 3}, .tick = 20, .align = CUI_ALIGN_CENTER},
        {.text = {"end", 3}, .tick = 60, .font_size = 20, .align = CUI_ALIGN_END}};
    cui_plot_axis_text y[] = {{.text = {"left", 4}, .tick = 40, .align = CUI_ALIGN_END}};
    cui_plot_axis_options o = cui_plot_axis_defaults(80, 80);
    assert(o.x_line && !o.y_line && !o.has_x && !o.has_y);
    assert(cui_plot_axis_gutter(10) == 18 && cui_plot_axis_gutter(NAN) == 0);
    o.has_x = o.has_y = true;
    o.x = 60;
    o.y = 10;
    o.x_labels = x;
    o.x_count = 2;
    o.y_labels = y;
    o.y_count = 1;
    cui_plot_axis_label_geometry labels[3], saved[3];
    size_t required = 99;
    memset(labels, 0x5a, sizeof labels);
    memcpy(saved, labels, sizeof labels);
    assert(cui_plot_axis_layout(ui, &o, labels, 2, &required) == CUI_PLOT_CAPACITY &&
           required == 3);
    assert(memcmp(saved, labels, sizeof labels) == 0);
    assert(cui_plot_axis_layout(ui, &o, labels, 3, &required) == CUI_PLOT_OK);
    assert(closef(labels[0].bounds.x, 12.5f) && labels[0].bounds.y == 66 && labels[0].horizontal);
    assert(labels[1].bounds.x == 30 && labels[1].bounds.y == 66);
    assert(labels[2].bounds.x == -8 && labels[2].bounds.y == 35 && !labels[2].horizontal);
    o.x_side = o.y_side = CUI_PLOT_AXIS_START;
    assert(cui_plot_axis_layout(ui, &o, labels, 3, &required) == CUI_PLOT_OK);
    assert(labels[0].bounds.y == 46 && labels[1].bounds.y == 36 && labels[2].bounds.x == -12);
    memcpy(saved, labels, sizeof labels);
    required = 99;
    x[1].font_size = NAN;
    assert(cui_plot_axis_layout(ui, &o, labels, 3, &required) == CUI_PLOT_INVALID &&
           required == 99);
    assert(memcmp(saved, labels, sizeof labels) == 0);
    x[1].font_size = 20;
    o.has_x = false;
    assert(cui_plot_axis_layout(ui, &o, labels, 3, &required) == CUI_PLOT_OK && required == 1 &&
           !labels[0].horizontal);
    cui_plot_numeric_tick ticks[16];
    cui_plot_scale scale = {CUI_PLOT_LINEAR, 0, 10, 70, 10, false};
    assert(cui_plot_axis_ticks(&scale, 6, NULL, 0, &required) == CUI_PLOT_CAPACITY &&
           required == 6);
    assert(cui_plot_axis_ticks(&scale, 6, ticks, 16, &required) == CUI_PLOT_OK);
    assert(ticks[0].value == 0 && ticks[0].label.tick == 70 && ticks[5].label.tick == 10);
    assert(ticks[0].label.text.data == ticks[0].text && strcmp(ticks[5].text, "10") == 0);
    cui_theme theme = cui_theme_light();
    theme.border = red;
    theme.muted = green;
    o.has_x = true;
    o.x_side = o.y_side = CUI_PLOT_AXIS_END;
    o.x_line = true;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_axis(ui, 15, &theme, &o) == CUI_PLOT_OK);
    cui_draw_list draw = cui_end_frame(ui);
    size_t texts = 0, lines = 0;
    for (size_t i = 0; i < draw.count; ++i) {
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            ++texts;
            assert(draw.commands[i].color.g == 255);
        }
        if (draw.commands[i].type == CUI_DRAW_RECT && draw.commands[i].color.r == 255) {
            ++lines;
            assert(draw.commands[i].bounds.y == 59.5f && draw.commands[i].bounds.h == 1);
        }
    }
    assert(texts == 3 && lines == 1);
    assert(cui_last_error(ui) == CUI_OK);
}
static void source_axis_grid(cui_context *ui) {
    cui_plot_axis_text x = {.text = {"AB", 2}, .tick = 40, .font_size = 20, .align = CUI_ALIGN_END};
    cui_plot_axis_text y = {
        .text = {"AB", 2}, .tick = 40, .font_size = 30, .align = CUI_ALIGN_CENTER};
    cui_plot_axis_options o = cui_plot_axis_source_defaults(100, 100);
    assert(o.source_offsets && o.explicit_colors && o.x_line && !o.y_line);
    o.has_x = o.has_y = true;
    o.x = 60;
    o.y = 10;
    o.y_line = true;
    o.x_side = o.y_side = CUI_PLOT_AXIS_START;
    o.x_labels = &x;
    o.x_count = 1;
    o.y_labels = &y;
    o.y_count = 1;
    cui_plot_axis_label_geometry geometry[2], old[2];
    size_t required;
    assert(cui_plot_axis_layout(ui, &o, geometry, 2, &required) == CUI_PLOT_OK && required == 2);
    assert(geometry[0].bounds.x == 20 && geometry[0].bounds.y == 46);
    assert(geometry[1].bounds.x == -7 && geometry[1].bounds.y == 35);
    memcpy(old, geometry, sizeof old);
    assert(cui_plot_axis_layout(ui, &o, geometry, 1, &required) == CUI_PLOT_CAPACITY);
    assert(memcmp(old, geometry, sizeof old) == 0);
    o.x_side = o.y_side = CUI_PLOT_AXIS_END;
    assert(cui_plot_axis_layout(ui, &o, geometry, 2, &required) == CUI_PLOT_OK);
    assert(geometry[0].bounds.y == 66 && geometry[1].bounds.x == -3 && geometry[1].bounds.y == 35);
    cui_theme theme = cui_theme_light();
    theme.border = red;
    theme.muted = green;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_axis(ui, 190, &theme, &o) == CUI_PLOT_OK);
    cui_draw_list draw = cui_end_frame(ui);
    size_t texts = 0;
    for (size_t i = 0; i < draw.count; ++i)
        if (draw.commands[i].type == CUI_DRAW_TEXT) {
            assert(draw.commands[i].color.a == 0);
            ++texts;
        }
    assert(texts == 2);
    /* Pixel proof for lines is font-independent; text colors were inspected above. */
    o.x_count = o.y_count = 0;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_axis(ui, 190, &theme, &o) == CUI_PLOT_OK);
    draw = cui_end_frame(ui);
    cui_software *renderer = cui_software_create(100, 100, NULL);
    assert(renderer);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(59 * w + 50) * 4 + 3] == 0 && pixels[(50 * w + 9) * 4 + 3] == 0);
    o.explicit_colors = false;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_axis(ui, 190, &theme, &o) == CUI_PLOT_OK);
    draw = cui_end_frame(ui);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(59 * w + 50) * 4] == 255);
    o.has_x = o.has_y = false;
    assert(cui_plot_axis_layout(ui, &o, NULL, 0, &required) == CUI_PLOT_OK && required == 0);
    double positions[] = {20};
    cui_plot_grid_options grid = {.width = 100, .height = 100, .x = positions, .x_count = 1};
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_grid_ex(ui, 191, &theme, &grid, (cui_color){0}, true) == CUI_PLOT_OK);
    draw = cui_end_frame(ui);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(50 * w + 19) * 4 + 3] == 0);
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_grid_ex(ui, 191, &theme, &grid, blue, true) == CUI_PLOT_OK);
    draw = cui_end_frame(ui);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(50 * w + 19) * 4 + 2] == 255);
    cui_software_destroy(renderer);
}
static void grid_pixels(cui_context *ui) {
    const double x[] = {10}, y[] = {20}, dash[] = {4, 2};
    cui_plot_grid_options grid = {.width = 40,
                                  .height = 40,
                                  .x = x,
                                  .y = y,
                                  .x_count = 1,
                                  .y_count = 1,
                                  .dashes = dash,
                                  .dash_count = 2};
    cui_theme theme = cui_theme_light();
    theme.border = red;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_grid(ui, 55, &theme, &grid, (cui_color){0}) == CUI_PLOT_OK);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_software *renderer = cui_software_create(100, 100, NULL);
    assert(renderer);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(2 * w + 9) * 4] == 255 && pixels[(5 * w + 9) * 4 + 3] == 0);
    assert(pixels[(19 * w + 2) * 4] == 255 && pixels[(19 * w + 5) * 4 + 3] == 0);
    cui_software_destroy(renderer);
}
typedef struct {
    size_t calls, index;
    cui_plot_rect frame;
    cui_plot_bar_alignment alignment;
} bar_calls;
static cui_color bar_fill(const cui_plot_bar_sample *sample, size_t index, cui_plot_rect frame,
                          cui_plot_bar_alignment alignment, void *user) {
    assert(sample->has_cross && sample->has_value);
    bar_calls *calls = user;
    ++calls->calls;
    calls->index = index;
    calls->frame = frame;
    calls->alignment = alignment;
    return red;
}
static void marker_pixels(cui_context *ui) {
    cui_theme theme = cui_theme_light();
    theme.primary = green;
    cui_plot_sample dots[] = {{20, NAN, 20, true}, {NAN, NAN, NAN, false}};
    cui_plot_dot_options style = cui_plot_dot_defaults();
    assert(style.size == 4 && style.use_theme_fill && !style.custom_stroke);
    style.size = 12;
    style.use_theme_fill = false;
    style.fill = red;
    style.stroke = blue;
    style.custom_stroke = true;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_dots(ui, 101, &theme, (cui_size){40, 40}, dots, 2, style) == CUI_PLOT_OK);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    size_t semantic_count;
    (void)cui_semantics(ui, &semantic_count);
    assert(semantic_count == 0);
    cui_software *renderer = cui_software_create(100, 100, NULL);
    assert(renderer);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(20 * w + 20) * 4] == 255 && pixels[(20 * w + 14) * 4 + 2] == 255 &&
           pixels[(14 * w + 14) * 4 + 3] == 0);
    style.use_theme_fill = true;
    style.custom_stroke = false;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_dots(ui, 101, &theme, (cui_size){40, 40}, dots, 2, style) == CUI_PLOT_OK);
    draw = cui_end_frame(ui);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(20 * w + 20) * 4 + 1] == 255 && pixels[(20 * w + 14) * 4 + 1] == 255);
    const double pi = acos(-1.);
    cui_plot_radial_sample radial[] = {
        {0, 0, 10, true}, {pi / 2, 0, 10, true}, {NAN, NAN, NAN, false}};
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_radial_dots(ui, 102, &theme, (cui_size){40, 40}, radial, 3, style) ==
           CUI_PLOT_OK);
    draw = cui_end_frame(ui);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(10 * w + 20) * 4 + 1] == 255 && pixels[(20 * w + 30) * 4 + 1] == 255);
    bar_calls calls = {0};
    cui_plot_bar_sample bars[] = {
        {8, 32, 8, true, true}, {NAN, NAN, NAN, false, true}, {30, 20, 20, true, true}};
    cui_plot_bar_options bar = {.alignment = CUI_PLOT_BAR_BOTTOM,
                                .band_width = 24,
                                .fill_for = bar_fill,
                                .user = &calls,
                                .corners = {100, 0, 0, 0}};
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_bars(ui, 103, &theme, (cui_size){64, 64}, bars, 3, &bar) == CUI_PLOT_OK);
    draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(calls.calls == 2 && calls.index == 2 && calls.alignment == CUI_PLOT_BAR_BOTTOM &&
           calls.frame.x_min == 30 && calls.frame.y_min == 20 && calls.frame.y_max == 20);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(8 * w + 8) * 4 + 3] == 0 && pixels[(8 * w + 31) * 4] == 255 &&
           pixels[(31 * w + 8) * 4] == 255);
    cui_plot_bar_options themed = cui_plot_bar_defaults();
    themed.alignment = CUI_PLOT_BAR_LEFT;
    themed.band_width = 12;
    cui_plot_bar_sample horizontal_bar = {8, 8, 32, true, true};
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_bars(ui, 104, &theme, (cui_size){64, 64}, &horizontal_bar, 1, &themed) ==
           CUI_PLOT_OK);
    draw = cui_end_frame(ui);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    pixels = cui_software_pixels(renderer, &w, &h);
    assert(pixels[(12 * w + 18) * 4 + 1] == 255);
    /* A late invalid sample must suppress even the earlier fill callback. */
    bars[2].value = NAN;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_bars(ui, 103, &theme, (cui_size){64, 64}, bars, 3, &bar) == CUI_PLOT_INVALID &&
           calls.calls == 2);
    draw = cui_end_frame(ui);
    assert(draw.count == 0 && cui_last_error(ui) == CUI_OK);
    dots[0].y1 = NAN;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_dots(ui, 101, &theme, (cui_size){40, 40}, dots, 2, style) == CUI_PLOT_INVALID);
    draw = cui_end_frame(ui);
    assert(draw.count == 0 && cui_last_error(ui) == CUI_OK);
    style.size = -1;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_radial_dots(ui, 102, &theme, (cui_size){40, 40}, radial, 3, style) ==
           CUI_PLOT_INVALID);
    draw = cui_end_frame(ui);
    assert(draw.count == 0 && cui_last_error(ui) == CUI_OK);
    cui_software_destroy(renderer);
}
static void raster(void) {
    unsigned char pixels[64 * 64 * 4], saved[sizeof pixels];
    cui_plot_path_command commands[64];
    size_t count;
    cui_plot_sample line[] = {{4, 0, 8, true}, {56, 0, 8, true}};
    assert(cui_plot_path(line, 2, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SKIP, false, commands, 64,
                         &count) == CUI_PLOT_OK);
    cui_plot_shape shape = {commands, count, {0}, red, 3};
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(pixels[(8 * 64 + 30) * 4] == 255 && pixels[(20 * 64 + 30) * 4 + 3] == 0);
    cui_plot_sample area[] = {{8, 50, 10, true}, {50, 50, 20, true}};
    assert(cui_plot_path(area, 2, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SKIP, true, commands, 64,
                         &count) == CUI_PLOT_OK);
    shape = (cui_plot_shape){commands, count, green, {0}, 0};
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(pixels[(30 * 64 + 30) * 4 + 1] == 255 && pixels[(5 * 64 + 30) * 4 + 3] == 0);
    /* Frozen Area strokes only the upper line, never the closing baseline. */
    cui_plot_area_sample source_samples[] = {{8, 10, true, true}, {50, 20, true, true}};
    cui_plot_path_command source_fill[16], source_stroke[16];
    cui_plot_area_buffers source_buffers = {
        .fill = source_fill, .stroke = source_stroke, .fill_capacity = 16, .stroke_capacity = 16};
    assert(cui_plot_area_paths(source_samples, 2, CUI_PLOT_CURVE_LINEAR, true, 50,
                               &source_buffers) == CUI_PLOT_OK);
    cui_plot_shape source_shapes[] = {{source_fill, source_buffers.fill_count, green, {0}, 0},
                                      {source_stroke, source_buffers.stroke_count, {0}, blue, 3}};
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, source_shapes, 2, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(pixels[(49 * 64 + 30) * 4 + 1] == 255 && pixels[(49 * 64 + 30) * 4 + 2] == 0);
    assert(pixels[(15 * 64 + 30) * 4 + 2] == 255);
    /* Projected bar uses the same polygon renderer. */
    cui_plot_scale xs = {CUI_PLOT_LINEAR, 0, 10, 0, 64, false},
                   ys = {CUI_PLOT_LINEAR, 0, 10, 64, 0, false};
    cui_plot_rect rect;
    assert(cui_plot_project_bar(&xs, &ys, 5, 12, 6, 0, (cui_plot_rect){0, 0, 64, 64}, &rect) ==
           CUI_PLOT_OK);
    commands[0] =
        (cui_plot_path_command){.kind = CUI_PLOT_PATH_MOVE, .point = {rect.x_min, rect.y_min}};
    commands[1] =
        (cui_plot_path_command){.kind = CUI_PLOT_PATH_LINE, .point = {rect.x_max, rect.y_min}};
    commands[2] =
        (cui_plot_path_command){.kind = CUI_PLOT_PATH_LINE, .point = {rect.x_max, rect.y_max}};
    commands[3] =
        (cui_plot_path_command){.kind = CUI_PLOT_PATH_LINE, .point = {rect.x_min, rect.y_max}};
    commands[4] = (cui_plot_path_command){.kind = CUI_PLOT_PATH_CLOSE};
    shape = (cui_plot_shape){commands, 5, blue, {0}, 0};
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(pixels[(40 * 64 + 32) * 4 + 2] == 255 && pixels[(40 * 64 + 15) * 4 + 3] == 0);
    const double pi = acos(-1.);
    cui_plot_arc arc = {0, 1, 0, pi / 2, 0};
    assert(cui_plot_arc_flatten(&arc, 0, 20, 32, commands, 64, &count) == CUI_PLOT_OK);
    for (size_t i = 0; i < count; ++i)
        if (commands[i].kind != CUI_PLOT_PATH_CLOSE) {
            commands[i].point.x += 32;
            commands[i].point.y += 32;
        }
    shape = (cui_plot_shape){commands, count, red, {0}, 0};
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(pixels[(22 * 64 + 42) * 4] == 255 && pixels[(42 * 64 + 22) * 4 + 3] == 0);
    memcpy(saved, pixels, sizeof pixels);
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels - 1, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_CAPACITY);
    assert(memcmp(saved, pixels, sizeof pixels) == 0);
    cui_plot_shape invalid[] = {shape, shape};
    invalid[1].commands = NULL;
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, invalid, 2, (cui_color){0}) ==
           CUI_PLOT_INVALID);
    assert(memcmp(saved, pixels, sizeof pixels) == 0);
    commands[0].point.x = NAN;
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_INVALID);
    assert(memcmp(saved, pixels, sizeof pixels) == 0);
    unsigned char padded[2 * 12];
    memset(padded, 0x5a, sizeof padded);
    assert(cui_plot_raster(padded, 2, 2, 12, sizeof padded, NULL, 0, red) == CUI_PLOT_OK);
    assert(padded[0] == 255 && padded[8] == 0x5a && padded[20] == 0x5a);
    /* Two closed subpaths form an even-odd hole; translucent fill preserves
     * straight-alpha color and composites over an opaque background. */
    cui_plot_path_command hole[] = {{.kind = CUI_PLOT_PATH_MOVE, .point = {4, 4}},
                                    {.kind = CUI_PLOT_PATH_LINE, .point = {60, 4}},
                                    {.kind = CUI_PLOT_PATH_LINE, .point = {60, 60}},
                                    {.kind = CUI_PLOT_PATH_LINE, .point = {4, 60}},
                                    {.kind = CUI_PLOT_PATH_CLOSE},
                                    {.kind = CUI_PLOT_PATH_MOVE, .point = {20, 20}},
                                    {.kind = CUI_PLOT_PATH_LINE, .point = {44, 20}},
                                    {.kind = CUI_PLOT_PATH_LINE, .point = {44, 44}},
                                    {.kind = CUI_PLOT_PATH_LINE, .point = {20, 44}},
                                    {.kind = CUI_PLOT_PATH_CLOSE}};
    shape = (cui_plot_shape){hole, 10, {255, 0, 0, 128}, {0}, 0};
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, &shape, 1, blue) == CUI_PLOT_OK);
    assert(pixels[(10 * 64 + 10) * 4] == 128 && pixels[(10 * 64 + 10) * 4 + 2] == 127 &&
           pixels[(30 * 64 + 30) * 4] == 0 && pixels[(30 * 64 + 30) * 4 + 2] == 255);
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(pixels[(10 * 64 + 10) * 4] == 255 && pixels[(10 * 64 + 10) * 4 + 3] == 128);
    memcpy(saved, pixels, sizeof pixels);
    hole[1].kind = CUI_PLOT_PATH_ARC;
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, &shape, 1, blue) ==
           CUI_PLOT_INVALID);
    assert(memcmp(saved, pixels, sizeof pixels) == 0);
    /* Work limit is checked before clearing caller pixels. */
    cui_plot_path_command dense[4096];
    for (size_t i = 0; i < 4096; ++i)
        dense[i] = (cui_plot_path_command){.kind = i ? CUI_PLOT_PATH_LINE : CUI_PLOT_PATH_MOVE,
                                           .point = {8, 8}};
    cui_plot_shape costly[5];
    for (size_t i = 0; i < 5; ++i)
        costly[i] = (cui_plot_shape){dense, 4096, red, {0}, 0};
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, costly, 5, blue) ==
           CUI_PLOT_CAPACITY);
    assert(memcmp(saved, pixels, sizeof pixels) == 0);
    /* Cubic commands are visibly rasterized, not ignored. */
    cui_plot_sample curve[] = {{4, 0, 40, true}, {30, 0, 8, true}, {56, 0, 40, true}};
    assert(cui_plot_path(curve, 3, CUI_PLOT_CURVE_NATURAL, CUI_PLOT_GAPS_SKIP, false, commands, 64,
                         &count) == CUI_PLOT_OK);
    shape = (cui_plot_shape){commands, count, {0}, blue, 3};
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(pixels[(8 * 64 + 30) * 4 + 2] == 255);
}
typedef struct {
    unsigned count;
    cui_plot_point point;
} activation;
static void activated(cui_context *ui, cui_id id, cui_plot_point point, void *user) {
    (void)ui;
    assert(id == 44);
    activation *a = user;
    ++a->count;
    a->point = point;
}
static void view_frame(cui_context *ui, cui_plot_view_options *o, bool check_pixels) {
    assert(cui_begin_frame(ui, 100, 100, .016f));
    cui_theme theme = cui_theme_light();
    assert(cui_plot_view(ui, 44, &theme, o) == CUI_PLOT_OK);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    if (check_pixels) {
        cui_software *renderer = cui_software_create(100, 100, NULL);
        assert(renderer);
        assert(cui_software_render(renderer, &draw, (cui_color){0}));
        unsigned w, h;
        const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
        assert(w == 100 && h == 100 && pixels[(10 * 100 + 10) * 4 + 2] == 255);
        cui_software_destroy(renderer);
    }
}
#ifdef CUI_HAS_TEXT
static void native_labels(void) {
    const char *paths[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans.ttf",
                           "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
                           "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf"};
    cui_font *font = NULL;
    for (size_t i = 0; i < sizeof paths / sizeof paths[0] && !font; ++i)
        font = cui_font_open(paths[i]);
    if (!font)
        return; /* Headless geometry checks above do not require a host font. */
    cui_context *ui = cui_create((cui_config){.width = 100,
                                              .height = 100,
                                              .measure = cui_font_measure,
                                              .measure_styled = cui_font_measure_styled,
                                              .measure_user = font,
                                              .font_metrics = cui_font_get_metrics});
    cui_software *renderer = cui_software_create(100, 100, font);
    assert(ui && renderer);
    cui_plot_axis_text label = {.text = {"100", 3}, .tick = 30, .align = CUI_ALIGN_CENTER};
    cui_plot_axis_options axis = cui_plot_axis_defaults(100, 100);
    axis.has_x = true;
    axis.x = 60;
    axis.x_line = false;
    axis.x_labels = &label;
    axis.x_count = 1;
    cui_theme theme = cui_theme_light();
    theme.muted = red;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_axis(ui, 80, &theme, &axis) == CUI_PLOT_OK);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    size_t ink = 0;
    for (unsigned y = 60; y < 90; ++y)
        for (unsigned x = 10; x < 50; ++x)
            if (pixels[(y * w + x) * 4] > 100 && pixels[(y * w + x) * 4 + 3])
                ++ink;
    assert(ink > 0);
    cui_plot_label general = {
        .text = {"Plot", 4}, .origin = {50, 20}, .color = green, .align = CUI_ALIGN_CENTER};
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_labels(ui, 81, (cui_size){100, 100}, &general, 1) == CUI_PLOT_OK);
    draw = cui_end_frame(ui);
    assert(cui_software_render(renderer, &draw, (cui_color){0}));
    pixels = cui_software_pixels(renderer, &w, &h);
    ink = 0;
    for (unsigned y = 15; y < 40; ++y)
        for (unsigned x = 25; x < 75; ++x)
            if (pixels[(y * w + x) * 4 + 1] > 100 && pixels[(y * w + x) * 4 + 3])
                ++ink;
    assert(ink > 0);
    char truncated[32];
    size_t required;
    assert(cui_plot_truncate_text(ui, (cui_str){"Plot labels", 11}, 10, 20, truncated,
                                  sizeof truncated, &required) == CUI_PLOT_OK);
    assert(required > 3 && required < 12);
    cui_software_destroy(renderer);
    cui_destroy(ui);
    cui_font_close(font);
}
#endif
static void general_labels(cui_context *ui) {
    cui_plot_label labels[] = {
        {.text = {"abc", 3}, .origin = {40, 12}, .align = CUI_ALIGN_START, .color = {1, 2, 3, 0}},
        {.text = {"abc", 3},
         .origin = {40, 24},
         .align = CUI_ALIGN_CENTER,
         .color = {255, 0, 0, 255}},
        {.text = {"abc", 3}, .origin = {40, 36}, .align = CUI_ALIGN_END, .font_size = 20}};
    cui_rect bounds[3], saved[3];
    size_t required;
    assert(cui_plot_labels_layout(ui, labels, 3, bounds, 3, &required) == CUI_PLOT_OK &&
           required == 3);
    assert(closef(bounds[0].x, 40) && closef(bounds[0].w, 15));
    assert(closef(bounds[1].x, 32.5f) && closef(bounds[1].y, 24));
    assert(closef(bounds[2].x, 10) && closef(bounds[2].h, 20));
    memcpy(saved, bounds, sizeof bounds);
    assert(cui_plot_labels_layout(ui, labels, 3, bounds, 2, &required) == CUI_PLOT_CAPACITY);
    assert(memcmp(saved, bounds, sizeof bounds) == 0);
    labels[2].font_size = NAN;
    assert(cui_plot_labels_layout(ui, labels, 3, bounds, 3, &required) == CUI_PLOT_INVALID);
    assert(memcmp(saved, bounds, sizeof bounds) == 0);
    labels[2].font_size = 20;
    assert(cui_begin_frame(ui, 100, 100, .016f));
    assert(cui_plot_labels(ui, 900, (cui_size){100, 100}, labels, 3) == CUI_PLOT_OK);
    cui_draw_list frame = cui_end_frame(ui);
    size_t texts = 0;
    for (size_t i = 0; i < frame.count; ++i)
        if (frame.commands[i].type == CUI_DRAW_TEXT)
            ++texts;
    assert(texts == 3);
    char output[64], old[64];
    memset(output, 'x', sizeof output);
    memcpy(old, output, sizeof output);
    assert(cui_plot_truncate_text(ui, (cui_str){"abcdef", 6}, 10, 0, output, 64, &required) ==
           CUI_PLOT_OK);
    assert(strcmp(output, "abcdef") == 0 && required == 7);
    assert(cui_plot_truncate_text(ui, (cui_str){"abcdef", 6}, 10, 25, output, 64, &required) ==
           CUI_PLOT_OK);
    assert(strcmp(output, "ab\xe2\x80\xa6") == 0);
    assert(cui_plot_truncate_text(ui, (cui_str){"abcdef", 6}, 10, 1, output, 64, &required) ==
           CUI_PLOT_OK);
    assert(strcmp(output, "\xe2\x80\xa6") == 0 && required == 4);
    const char utf8[] = "\xc3\xa9\xe4\xb8\xad"
                        "abcdef";
    assert(cui_plot_truncate_text(ui, (cui_str){utf8, sizeof utf8 - 1}, 10, 25, output, 64,
                                  &required) == CUI_PLOT_OK);
    assert(strcmp(output, "\xc3\xa9\xe2\x80\xa6") == 0);
    strcpy(output, "abcdef");
    assert(cui_plot_truncate_text(ui, (cui_str){output, 6}, 10, 25, output, 64, &required) ==
           CUI_PLOT_OK);
    assert(strcmp(output, "ab\xe2\x80\xa6") == 0);
    memcpy(old, output, sizeof output);
    assert(cui_plot_truncate_text(ui, (cui_str){"abcdef", 6}, 10, 25, output, 2, &required) ==
           CUI_PLOT_CAPACITY);
    assert(required == 6 && memcmp(old, output, sizeof output) == 0);
    required = 99;
    assert(cui_plot_truncate_text(ui, (cui_str){"\xc0\x80", 2}, 10, 25, output, 64, &required) ==
           CUI_PLOT_INVALID);
    assert(required == 99 && memcmp(old, output, sizeof output) == 0);
    assert(cui_plot_truncate_text(ui, (cui_str){"", 0}, 0, 1, output, 64, &required) ==
           CUI_PLOT_OK);
    assert(output[0] == 0 && required == 1);
    const char *invalid[] = {"\xed\xa0\x80", "\xf4\x90\x80\x80", "\xe2\x80", "\x80"};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        required = 99;
        assert(cui_plot_truncate_text(ui, (cui_str){invalid[i], strlen(invalid[i])}, 10, 25, output,
                                      64, &required) == CUI_PLOT_INVALID);
        assert(output[0] == 0 && required == 99);
    }
    char long_text[4096];
    memset(long_text, 'x', sizeof long_text);
    assert(cui_plot_truncate_text(ui, (cui_str){long_text, sizeof long_text}, 10, 25, output, 64,
                                  &required) == CUI_PLOT_OK);
    assert(strcmp(output, "xx\xe2\x80\xa6") == 0);
}
static void test_sankey_raster(void) {
    cui_plot_sankey_ribbon link = {2, 14, 8, 8, 4, 8};
    cui_plot_path_command path[5];
    size_t required;
    assert(cui_plot_sankey_link_path(&link, 0, (cui_plot_point){0, 0}, path, 5, &required) ==
           CUI_PLOT_OK);
    cui_plot_shape shape = {.commands = path, .count = required, .fill = {20, 200, 50, 255}};
    uint8_t pixels[16 * 16 * 4];
    assert(cui_plot_raster(pixels, 16, 16, 64, sizeof pixels, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(pixels[(8 * 16 + 8) * 4 + 1] == 200);
    assert(pixels[(5 * 16 + 3) * 4 + 3] == 0);
    assert(pixels[(5 * 16 + 13) * 4 + 3] == 255);
    assert(pixels[(3 * 16 + 13) * 4 + 3] == 0);
}
static void test_rounded_bar_raster(void) {
    cui_plot_path_command path[10];
    double radii[] = {4, 0, 0, 0};
    size_t required;
    assert(cui_plot_bar_path(CUI_PLOT_BAR_BOTTOM, 2, 8, 12, 2, radii, path, 10, &required) ==
           CUI_PLOT_OK);
    uint8_t pixels[16 * 16 * 4];
    cui_plot_shape shape = {.commands = path, .count = required, .fill = {200, 50, 10, 255}};
    assert(cui_plot_raster(pixels, 16, 16, 64, sizeof pixels, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(pixels[(2 * 16 + 2) * 4 + 3] == 0);
    assert(pixels[(2 * 16 + 9) * 4 + 3] == 255);
    assert(pixels[(11 * 16 + 2) * 4 + 3] == 255);
    assert(pixels[(6 * 16 + 6) * 4] == 200);
}
typedef struct {
    const cui_plot_path_command *commands;
    size_t count, builds;
    cui_plot_result result;
} cache_builder;
static cui_plot_result build_cache(void *user, cui_plot_shape *shape) {
    cache_builder *builder = user;
    ++builder->builds;
    *shape = (cui_plot_shape){.commands = builder->commands, .count = builder->count};
    return builder->result;
}
static void path_caches(void) {
    cui_plot_path_command commands[] = {
        {.kind = CUI_PLOT_PATH_MOVE, .point = {2, 2}},
        {.kind = CUI_PLOT_PATH_CUBIC, .point = {12, 2}, .control1 = {4, 8}, .control2 = {10, 8}},
        {.kind = CUI_PLOT_PATH_LINE, .point = {2, 2}},
        {.kind = CUI_PLOT_PATH_CLOSE}};
    cui_plot_path_segment storage[32], saved_storage[32];
    memset(storage, 0x5a, sizeof storage);
    cui_plot_path_cache cache;
    assert(cui_plot_path_cache_init(&cache, storage, 32) == CUI_PLOT_OK);
    assert(!cui_plot_path_cache_is_warm(&cache));
    cache_builder builder = {commands, 4, 0, CUI_PLOT_OK};
    cui_plot_cached_path path;
    assert(cui_plot_path_cache_get(&cache, 1, (cui_plot_point){0}, build_cache, &builder, &path) ==
           CUI_PLOT_OK);
    assert(builder.builds == 1 && path.count == 18 && cui_plot_path_cache_is_warm(&cache));
    for (size_t i = 0; i < path.count; ++i)
        assert(path.segments[i].closed);
    memcpy(saved_storage, storage, sizeof storage);
    cui_plot_cached_shape shape = {path, green, red, 1};
    unsigned char direct[32 * 32 * 4], cached[sizeof direct];
    cui_plot_shape original = {commands, 4, green, red, 1};
    assert(cui_plot_raster(direct, 32, 32, 128, sizeof direct, &original, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(cui_plot_raster_cached(cached, 32, 32, 128, sizeof cached, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(memcmp(direct, cached, sizeof direct) == 0);
    /* Origin/color changes do not rebuild; translated cached raster equals translated commands. */
    assert(cui_plot_path_cache_get(&cache, 1, (cui_plot_point){5, 3}, NULL, NULL, &path) ==
           CUI_PLOT_OK);
    assert(builder.builds == 1 && memcmp(storage, saved_storage, sizeof storage) == 0);
    for (size_t i = 0; i < 3; ++i) {
        commands[i].point.x += 5;
        commands[i].point.y += 3;
    }
    commands[1].control1.x += 5;
    commands[1].control1.y += 3;
    commands[1].control2.x += 5;
    commands[1].control2.y += 3;
    shape = (cui_plot_cached_shape){path, blue, green, 2};
    original = (cui_plot_shape){commands, 4, blue, green, 2};
    assert(cui_plot_raster(direct, 32, 32, 128, sizeof direct, &original, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(cui_plot_raster_cached(cached, 32, 32, 128, sizeof cached, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(memcmp(direct, cached, sizeof direct) == 0);
    bool painted = false;
    for (size_t i = 3; i < sizeof cached; i += 4)
        painted |= cached[i] != 0;
    assert(painted);
    cui_plot_path_cache before = cache;
    cui_plot_cached_path saved_path = path;
    assert(cui_plot_path_cache_get(&cache, 2, (cui_plot_point){0}, NULL, NULL, &path) ==
           CUI_PLOT_INVALID);
    builder.result = CUI_PLOT_INVALID;
    assert(cui_plot_path_cache_get(&cache, 2, (cui_plot_point){0}, build_cache, &builder, &path) ==
           CUI_PLOT_INVALID);
    assert(memcmp(&before, &cache, sizeof cache) == 0 &&
           memcmp(&saved_path, &path, sizeof path) == 0);
    assert(memcmp(storage, saved_storage, sizeof storage) == 0);
    builder.result = CUI_PLOT_OK;
    assert(cui_plot_path_cache_get(&cache, 2, (cui_plot_point){0}, build_cache, &builder, &path) ==
           CUI_PLOT_OK);
    assert(builder.builds == 3 && cache.key == 2);
    before = cache;
    saved_path = path;
    assert(cui_plot_path_cache_get(&cache, 2, (cui_plot_point){16384, 0}, NULL, NULL, &path) ==
           CUI_PLOT_INVALID);
    assert(memcmp(&before, &cache, sizeof cache) == 0 &&
           memcmp(&saved_path, &path, sizeof path) == 0);
    builder.result = CUI_PLOT_EMPTY;
    assert(cui_plot_path_cache_get(&cache, 3, (cui_plot_point){0}, build_cache, &builder, &path) ==
           CUI_PLOT_EMPTY);
    assert(path.count == 0 && cui_plot_path_cache_is_warm(&cache));
    assert(cui_plot_path_cache_get(&cache, 3, (cui_plot_point){1, 1}, NULL, NULL, &path) ==
           CUI_PLOT_EMPTY);
    assert(builder.builds == 4);
    cui_plot_path_cache small;
    assert(cui_plot_path_cache_init(&small, storage, 1) == CUI_PLOT_OK);
    builder.result = CUI_PLOT_OK;
    memcpy(saved_storage, storage, sizeof storage);
    assert(cui_plot_path_cache_get(&small, 1, (cui_plot_point){0}, build_cache, &builder, &path) ==
           CUI_PLOT_CAPACITY);
    assert(!small.warm && small.count == 0);
    assert(memcmp(storage, saved_storage, sizeof storage) == 0);
    shape.path.origin.x = NAN;
    memset(cached, 0x5a, sizeof cached);
    memcpy(direct, cached, sizeof cached);
    assert(cui_plot_raster_cached(cached, 32, 32, 128, sizeof cached, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_INVALID);
    assert(memcmp(direct, cached, sizeof cached) == 0);
    assert(cui_plot_path_cache_get(&cache, 3, (cui_plot_point){NAN, 0}, NULL, NULL, &path) ==
           CUI_PLOT_INVALID);
}
typedef struct {
    int order[8];
    size_t used, resolves, tooltips, activations;
    cui_plot_point resolved;
    bool fail;
    cui_plot_appear_value appearance;
} element_probe;
static double element_linear(double progress, void *user) {
    (void)user;
    return progress;
}
static cui_plot_result element_resolve(cui_plot_point point, cui_plot_rect bounds,
                                       cui_plot_tooltip_state *out, void *user) {
    element_probe *probe = user;
    ++probe->resolves;
    probe->order[probe->used++] = 1;
    probe->resolved = point;
    assert(bounds.x_max - bounds.x_min == 64);
    if (probe->fail)
        return CUI_PLOT_INVALID;
    *out = (cui_plot_tooltip_state){.index = point.x < 32 ? 1 : 2, .cross_line = point};
    return CUI_PLOT_OK;
}
static void element_marks(cui_context *ui, cui_id id, const cui_theme *theme,
                          const cui_plot_hover_memory *hover, cui_plot_rect bounds, void *user) {
    (void)theme;
    (void)bounds;
    element_probe *probe = user;
    probe->order[probe->used++] = 2;
    if (hover) {
        cui_begin_absolute_box(
            ui, id + 1, (cui_box){.background = green},
            (cui_rect){(float)hover->state.cross_line.x, (float)hover->state.cross_line.y, 3, 3});
        cui_end_box(ui);
    }
}
static void element_children(cui_context *ui, cui_id id, const cui_theme *theme,
                             const cui_plot_hover_memory *hover, cui_plot_rect bounds, void *user) {
    (void)theme;
    (void)hover;
    (void)bounds;
    element_probe *probe = user;
    probe->order[probe->used++] = 3;
    cui_begin_absolute_box(ui, id + 2, (cui_box){.background = red}, (cui_rect){22, 22, 10, 6});
    cui_end_box(ui);
}
static void element_tooltip(cui_context *ui, cui_id id, const cui_theme *theme,
                            const cui_plot_hover_memory *hover, cui_plot_rect bounds, void *user) {
    (void)theme;
    (void)bounds;
    element_probe *probe = user;
    probe->order[probe->used++] = 4;
    ++probe->tooltips;
    assert(hover && hover->has_state);
    cui_begin_absolute_box(
        ui, id + 3, (cui_box){.background = blue},
        (cui_rect){(float)hover->cursor.x + 2, (float)hover->cursor.y + 2, 10, 6});
    (void)cui_interact(ui, id + 3, CUI_ROLE_BUTTON, cui_string("tooltip"), false, false, 0);
    cui_end_box(ui);
}
static void element_activate(cui_context *ui, cui_id id, cui_plot_point point, void *user) {
    (void)ui;
    assert(id == 444);
    assert(point.x == 24 && point.y == 24);
    ++((element_probe *)user)->activations;
}
static cui_draw_list element_frame(cui_context *ui, cui_plot_element_state *state,
                                   cui_plot_element_options *o, cui_plot_view_options *view,
                                   element_probe *probe, float dt) {
    probe->used = 0;
    assert(cui_begin_frame(ui, 100, 100, dt));
    cui_theme theme = cui_theme_light();
    cui_begin_box(
        ui, 9990,
        (cui_box){.width = 100, .height = 100, .fixed_width = true, .fixed_height = true});
    cui_begin_absolute_box(ui, 9991, (cui_box){0},
                           (cui_rect){(float)o->bounds.x_min, (float)o->bounds.y_min, 64, 64});
    assert(cui_plot_element(ui, 444, &theme, view, state, o) == CUI_PLOT_OK);
    cui_end_box(ui);
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return draw;
}
static void plot_elements(void) {
    cui_context *ui = cui_create((cui_config){.width = 100, .height = 100, .measure = measure});
    assert(ui);
    unsigned char bitmap[64 * 64 * 4] = {0};
    element_probe probe = {0};
    cui_plot_view_options view = {.bitmap = {bitmap, 64, 64, 256, 1},
                                  .interactive = true,
                                  .on_activate = element_activate,
                                  .user = &probe};
    cui_plot_element_state state = {0};
    cui_plot_element_options o = {.bounds = {10, 15, 74, 79},
                                  .pointer = {30, 35},
                                  .enter = {1, 0, element_linear, NULL},
                                  .exit = {1, 0, element_linear, NULL},
                                  .resolve = element_resolve,
                                  .marks = element_marks,
                                  .children = element_children,
                                  .tooltip = element_tooltip,
                                  .user = &probe};
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(!state.hover.has_state && probe.resolves == 0 && probe.used == 2);
    o.hovered = true;
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.hover.entering && state.request_frame && probe.used == 4);
    for (size_t i = 0; i < 4; ++i)
        assert(probe.order[i] == (int)i + 1);
    cui_draw_list draw = element_frame(ui, &state, &o, &view, &probe, 1);
    assert(state.hover.progress == 1 && !state.request_frame);
    cui_rect tooltip = cui_bounds(ui, 447);
    assert(tooltip.x == 32 && tooltip.y == 37 && tooltip.w == 10 && tooltip.h == 6);
    size_t semantics_count;
    const cui_semantic *semantics = cui_semantics(ui, &semantics_count);
    for (size_t i = 0; i < semantics_count; ++i)
        assert(semantics[i].id != 447);
    cui_software *renderer = cui_software_create(100, 100, NULL);
    assert(renderer && cui_software_render(renderer, &draw, (cui_color){0}));
    unsigned width, height;
    size_t stride = 100 * 4;
    const unsigned char *pixels = cui_software_pixels(renderer, &width, &height);
    assert(pixels[38 * stride + 33 * 4 + 2] == 255); /* tooltip overlays ordinary child */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 34, .y = 39}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 34, .y = 39}));
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(probe.activations == 1); /* inert tooltip does not capture the plot's click */
    /* Stationary pointer, current bounds move: datum is recomputed this frame. */
    o.bounds = (cui_plot_rect){20, 20, 84, 84};
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(probe.resolved.x == 10 && probe.resolved.y == 15);
    size_t resolves = probe.resolves;
    o.hovered = false; /* host says popup/modal occludes plot */
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(probe.resolves == resolves && state.hover.has_state && !state.hover.hovered);
    (void)element_frame(ui, &state, &o, &view, &probe, .25f);
    assert(closef((float)state.hover.progress, .75f) && state.hover.cursor.x == 10);
    (void)element_frame(ui, &state, &o, &view, &probe, .75f);
    assert(!state.hover.has_state && probe.used == 2);
    o.hovered = true;
    o.appearing = true;
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(probe.resolves == resolves && !state.hover.has_state);
    o.appearing = false;
    cui_set_reduced_motion(ui, true);
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.hover.progress == 1 && state.hover.has_state && !state.request_frame);
    view.disabled = true;
    resolves = probe.resolves;
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(!state.hover.has_state && probe.resolves == resolves && probe.used == 2);
    /* Touch requires an explicitly accepted long press, not desktop hover. */
    view.disabled = false;
    o.touch = true;
    o.hovered = false;
    cui_plot_cursor_update update;
    assert(cui_plot_cursor_long_press(&state.cursor, o.bounds, o.pointer,
                                      CUI_PLOT_LONG_PRESS_STARTED, true, false,
                                      &update) == CUI_PLOT_OK);
    assert(update.prevent_default);
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.hover.has_state);
    o.pointer.x = 90;
    assert(cui_plot_cursor_long_press(&state.cursor, o.bounds, o.pointer, CUI_PLOT_LONG_PRESS_MOVED,
                                      false, false, &update) == CUI_PLOT_OK);
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(!state.hover.has_state); /* live outside current bounds */
    assert(cui_plot_cursor_long_press(&state.cursor, o.bounds, o.pointer, CUI_PLOT_LONG_PRESS_ENDED,
                                      false, false, &update) == CUI_PLOT_OK);
    o.pointer.x = 30;
    o.hovered = true;
    resolves = probe.resolves;
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(probe.resolves == resolves && !state.hover.has_state);
    o.touch = false;
    /* Resolver preflight error submits no layout and preserves caller state. */
    view.disabled = false;
    probe.fail = true;
    cui_plot_element_state before = state;
    assert(cui_begin_frame(ui, 100, 100, 0));
    cui_theme theme = cui_theme_light();
    assert(cui_plot_element(ui, 444, &theme, &view, &state, &o) == CUI_PLOT_INVALID);
    assert(memcmp(&state, &before, sizeof state) == 0);
    assert(cui_end_frame(ui).count == 0);
    cui_software_destroy(renderer);
    cui_destroy(ui);
}
static void appear_notify(cui_id id, const cui_plot_appear_value *value, void *user) {
    assert(id);
    element_probe *probe = user;
    probe->order[probe->used++] = 0;
    probe->appearance = *value;
}
static void appear_marks(cui_context *ui, cui_id id, const cui_theme *theme,
                         const cui_plot_hover_memory *hover, cui_plot_rect bounds, void *user) {
    (void)theme;
    (void)hover;
    (void)bounds;
    element_probe *probe = user;
    probe->order[probe->used++] = 2;
    cui_begin_absolute_box(ui, id + 1, (cui_box){.background = green},
                           (cui_rect){0, 0, (float)(32 * probe->appearance.progress), 3});
    cui_end_box(ui);
}
static void empty_host_frame(cui_context *ui, float dt) {
    assert(cui_begin_frame(ui, 100, 100, dt));
    (void)cui_end_frame(ui);
}
static void styled_appearance(void) {
    cui_context *ui = cui_create((cui_config){.width = 100, .height = 100, .measure = measure});
    assert(ui);
    unsigned char bitmap[64 * 64 * 4] = {0};
    element_probe probe = {0};
    cui_plot_view_options view = {.bitmap = {bitmap, 64, 64, 256, 1}, .interactive = true};
    cui_plot_element_state state = {0};
    cui_plot_element_options o = {.bounds = {10, 15, 74, 79},
                                  .pointer = {30, 35},
                                  .hovered = true,
                                  .resolve = element_resolve,
                                  .marks = appear_marks,
                                  .children = element_children,
                                  .tooltip = element_tooltip,
                                  .user = &probe,
                                  .has_appearance = true,
                                  .appear = {.duration = 1},
                                  .on_appear = appear_notify,
                                  .frame_serial = 1};
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 0 && state.request_frame && probe.resolves == 0);
    assert(probe.used == 3 && probe.order[0] == 0 && probe.order[1] == 2 && probe.order[2] == 3);
    o.frame_serial = 2;
    cui_draw_list draw = element_frame(ui, &state, &o, &view, &probe, .5f);
    assert(state.appearance.time == .5 && state.appearance.progress == .875 && probe.resolves == 0);
    assert(cui_bounds(ui, 445).w == 28); /* eased marks are actual layout */
    cui_software *renderer = cui_software_create(100, 100, NULL);
    assert(renderer && cui_software_render(renderer, &draw, (cui_color){0}));
    const unsigned char *pixels = cui_software_pixels(renderer, NULL, NULL);
    assert(pixels[(16 * 100 + 20) * 4 + 1] == 255);
    o.frame_serial = 3;
    (void)element_frame(ui, &state, &o, &view, &probe, .5f);
    assert(state.appearance.time == 1 && !state.request_frame && state.hover.has_state);
    assert(probe.used == 5 && probe.order[0] == 0 && probe.order[1] == 1);
    empty_host_frame(ui, 0); /* omitted serial4: unscoped completed plot replays */
    o.frame_serial = 5;
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 0 && !state.hover.has_state);
    o.generation = 1;
    o.frame_serial = 6;
    (void)element_frame(ui, &state, &o, &view, &probe, .5f);
    assert(state.appearance.time == 0);
    cui_set_reduced_motion(ui, true);
    o.frame_serial = 7;
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 1 && !state.request_frame);
    cui_set_reduced_motion(ui, false);
    /* Reusing caller storage under another owner resets the appearance channel. */
    o.frame_serial = 8;
    probe.used = 0;
    assert(cui_begin_frame(ui, 100, 100, 0));
    cui_theme theme = cui_theme_light();
    cui_begin_box(
        ui, 9900,
        (cui_box){.width = 100, .height = 100, .fixed_width = true, .fixed_height = true});
    cui_begin_absolute_box(ui, 9901, (cui_box){0}, (cui_rect){10, 15, 64, 64});
    assert(cui_plot_element(ui, 544, &theme, &view, &state, &o) == CUI_PLOT_OK);
    cui_end_box(ui);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(state.owner == 544 && state.appearance.time == 0);
    /* Passive/disabled plots still run appearance and retain ordinary children. */
    view.interactive = false;
    view.disabled = true;
    o.frame_serial = 9;
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 0 && probe.used == 3);
    o.frame_serial = 10;
    (void)element_frame(ui, &state, &o, &view, &probe, .5f);
    assert(state.appearance.time == .5 && state.request_frame && probe.used == 3);
    cui_plot_appear_scope storage[2];
    cui_plot_appear_registry scope;
    assert(cui_plot_appear_registry_init(&scope, storage, 2) == CUI_PLOT_OK);
    o.appear_scope = &scope;
    o.generation = 42;
    o.frame_serial = 11;
    assert(cui_plot_appear_registry_begin(&scope, 88, 11) == CUI_PLOT_OK);
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 0 && scope.count == 0);
    assert(cui_plot_appear_registry_end(&scope) == CUI_PLOT_OK);
    o.frame_serial = 12;
    assert(cui_plot_appear_registry_begin(&scope, 88, 12) == CUI_PLOT_OK);
    (void)element_frame(ui, &state, &o, &view, &probe, 1);
    assert(state.appearance.time == 1 && scope.count == 1 && storage[0].completed_generation == 42);
    assert(cui_plot_appear_registry_end(&scope) == CUI_PLOT_OK);
    assert(cui_plot_appear_registry_begin(&scope, 88, 13) == CUI_PLOT_OK);
    empty_host_frame(ui, 0);
    assert(cui_plot_appear_registry_end(&scope) == CUI_PLOT_OK);
    o.frame_serial = 14;
    assert(cui_plot_appear_registry_begin(&scope, 88, 14) == CUI_PLOT_OK);
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 1 && !state.request_frame); /* completed scoped remount */
    assert(cui_plot_appear_registry_end(&scope) == CUI_PLOT_OK);
    o.generation = 43;
    o.frame_serial = 15;
    assert(cui_plot_appear_registry_begin(&scope, 88, 15) == CUI_PLOT_OK);
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 0 && storage[0].completed_generation == 42);
    assert(cui_plot_appear_registry_end(&scope) == CUI_PLOT_OK);
    assert(cui_plot_appear_registry_begin(&scope, 88, 16) == CUI_PLOT_OK);
    empty_host_frame(ui, .5f);
    assert(cui_plot_appear_registry_end(&scope) == CUI_PLOT_OK);
    o.frame_serial = 17;
    assert(cui_plot_appear_registry_begin(&scope, 88, 17) == CUI_PLOT_OK);
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 0); /* unfinished omission restarts */
    assert(cui_plot_appear_registry_end(&scope) == CUI_PLOT_OK);
    o.frame_serial = 18;
    assert(cui_plot_appear_registry_begin(&scope, 99, 18) == CUI_PLOT_OK);
    (void)element_frame(ui, &state, &o, &view, &probe, .5f);
    assert(state.appearance.time == 0 && scope.count == 0); /* scope rekey resets */
    assert(cui_plot_appear_registry_end(&scope) == CUI_PLOT_OK);
    o.appear_scope = NULL;
    o.frame_serial = 19;
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 0); /* removing scope restarts local channel */
    o.has_appearance = false;
    o.frame_serial = 20;
    (void)element_frame(ui, &state, &o, &view, &probe, 0);
    assert(state.appearance.time == 1 && !state.request_frame); /* no generation means complete */
    cui_plot_appear_registry full;
    assert(cui_plot_appear_registry_init(&full, NULL, 0) == CUI_PLOT_OK);
    assert(cui_plot_appear_registry_begin(&full, 1, 21) == CUI_PLOT_OK);
    o.has_appearance = true;
    o.appear_scope = &full;
    o.appear.duration = 0;
    o.frame_serial = 21;
    cui_plot_element_state before = state;
    assert(cui_begin_frame(ui, 100, 100, 0));
    assert(cui_plot_element(ui, 444, &theme, &view, &state, &o) == CUI_PLOT_CAPACITY);
    assert(memcmp(&state, &before, sizeof state) == 0 && full.count == 0);
    assert(cui_end_frame(ui).count == 0);
    assert(cui_plot_appear_registry_end(&full) == CUI_PLOT_OK);
    cui_software_destroy(renderer);
    cui_destroy(ui);
}
static void software_backgrounds(void) {
    cui_plot_rect frame = {0, 0, 8, 8};
    cui_plot_background bg = cui_plot_linear_background(0, (cui_plot_color_stop){red, 0},
                                                        (cui_plot_color_stop){blue, 1});
    cui_color color;
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 0}, &color) == CUI_PLOT_OK);
    assert(color.b == 255 && color.r == 0);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 8}, &color) == CUI_PLOT_OK);
    assert(color.r == 255 && color.b == 0);
    bg.angle = 90;
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 4}, &color) == CUI_PLOT_OK);
    assert(color.r == 128 && color.b == 128 && color.a == 255);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){0, 4}, &color) == CUI_PLOT_OK &&
           color.r == 255);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){8, 4}, &color) == CUI_PLOT_OK &&
           color.b == 255);
    bg.angle = 450;
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){8, 4}, &color) == CUI_PLOT_OK &&
           color.b == 255);
    bg.angle = 45;
    assert(cui_plot_background_sample(&bg, (cui_plot_rect){0, 0, 16, 8}, (cui_plot_point){16, 0},
                                      &color) == CUI_PLOT_OK &&
           color.b == 255);
    bg.angle = 90;
    bg.from.position = 1;
    bg.to.position = 0;
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){0, 4}, &color) == CUI_PLOT_OK &&
           color.b == 255);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){8, 4}, &color) == CUI_PLOT_OK &&
           color.r == 255);
    bg.from.position = .5;
    bg.to.position = .5;
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){3, 4}, &color) == CUI_PLOT_OK &&
           color.r == 255);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 4}, &color) == CUI_PLOT_OK &&
           color.b == 255);
    bg.from.position = .25;
    bg.to.position = .75;
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){1, 4}, &color) == CUI_PLOT_OK &&
           color.r == 255);
    bg = cui_plot_linear_background(0, (cui_plot_color_stop){{255, 0, 0, 128}, 1},
                                    (cui_plot_color_stop){{0, 0, 255, 0}, 0});
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 0}, &color) == CUI_PLOT_OK &&
           color.a == 128);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 8}, &color) == CUI_PLOT_OK &&
           color.a == 0);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 4}, &color) == CUI_PLOT_OK &&
           color.a == 64);
    cui_color saved = color;
    bg.from.position = NAN;
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){1, 4}, &color) ==
           CUI_PLOT_INVALID);
    assert(memcmp(&color, &saved, sizeof color) == 0);
    bg = cui_plot_solid_background(green);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){1, 4}, &color) == CUI_PLOT_OK &&
           color.g == 255);
    assert(cui_plot_background_sample(&bg, (cui_plot_rect){0}, (cui_plot_point){0}, &color) ==
           CUI_PLOT_INVALID);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){NAN, 0}, &color) ==
           CUI_PLOT_INVALID);
    cui_plot_path_command commands[] = {{.kind = CUI_PLOT_PATH_MOVE, .point = {0, 0}},
                                        {.kind = CUI_PLOT_PATH_LINE, .point = {8, 0}},
                                        {.kind = CUI_PLOT_PATH_LINE, .point = {8, 8}},
                                        {.kind = CUI_PLOT_PATH_LINE, .point = {0, 8}},
                                        {.kind = CUI_PLOT_PATH_CLOSE}};
    cui_plot_path_segment storage[8] = {0}, retained[8];
    cui_plot_path_cache cache;
    cui_plot_cached_path path;
    cache_builder builder = {commands, 5, 0, CUI_PLOT_OK};
    assert(cui_plot_path_cache_init(&cache, storage, 8) == CUI_PLOT_OK);
    assert(cui_plot_path_cache_get(&cache, 1, (cui_plot_point){0}, build_cache, &builder, &path) ==
           CUI_PLOT_OK);
    unsigned char pixels[12 * 10 * 4], direct[sizeof pixels];
    cui_plot_background_shape shape = {.path = path,
                                       .background_frame = frame,
                                       .fill = cui_plot_solid_background(red),
                                       .stroke = cui_plot_solid_background(blue),
                                       .stroke_width = 1};
    cui_plot_cached_shape solid = {path, red, blue, 1};
    assert(cui_plot_raster_cached(direct, 12, 10, 48, sizeof direct, &solid, 1, (cui_color){0}) ==
           CUI_PLOT_OK);
    assert(cui_plot_raster_backgrounds(pixels, 12, 10, 48, sizeof pixels, &shape, 1,
                                       (cui_color){0}) == CUI_PLOT_OK);
    assert(memcmp(direct, pixels, sizeof pixels) == 0);
    shape.stroke_width = 0;
    shape.fill = cui_plot_linear_background(90, (cui_plot_color_stop){red, 0},
                                            (cui_plot_color_stop){blue, 1});
    assert(cui_plot_raster_backgrounds(pixels, 12, 10, 48, sizeof pixels, &shape, 1,
                                       (cui_color){0}) == CUI_PLOT_OK);
    assert(pixels[(3 * 12) * 4] == 239 && pixels[(3 * 12) * 4 + 2] == 16);
    assert(pixels[(3 * 12 + 7) * 4] == 16 && pixels[(3 * 12 + 7) * 4 + 2] == 239);
    memcpy(retained, storage, sizeof storage);
    assert(cui_plot_path_cache_get(&cache, 1, (cui_plot_point){2, 1}, NULL, NULL, &shape.path) ==
           CUI_PLOT_OK);
    assert(cui_plot_raster_backgrounds(pixels, 12, 10, 48, sizeof pixels, &shape, 1,
                                       (cui_color){0}) == CUI_PLOT_OK);
    assert(builder.builds == 1 && memcmp(storage, retained, sizeof storage) == 0);
    assert(pixels[(4 * 12 + 2) * 4] == 239 && pixels[(4 * 12 + 2) * 4 + 2] == 16);
    shape.fill = cui_plot_linear_background(0, (cui_plot_color_stop){{255, 0, 0, 128}, 1},
                                            (cui_plot_color_stop){{0, 0, 255, 0}, 0});
    assert(cui_plot_raster_backgrounds(pixels, 12, 10, 48, sizeof pixels, &shape, 1,
                                       (cui_color){0}) == CUI_PLOT_OK);
    assert(pixels[(1 * 12 + 2) * 4 + 3] == 120 && pixels[(8 * 12 + 2) * 4 + 3] == 8);
    shape.fill = cui_plot_linear_background(90, (cui_plot_color_stop){red, 0},
                                            (cui_plot_color_stop){blue, 1});
    /* Stroke background uses the caller frame verbatim as fill does. */
    commands[0].point.y = commands[1].point.y = 4;
    builder.count = 2;
    assert(cui_plot_path_cache_get(&cache, 2, (cui_plot_point){0}, build_cache, &builder,
                                   &shape.path) == CUI_PLOT_OK);
    shape.stroke_width = 2;
    shape.stroke = shape.fill;
    shape.fill = cui_plot_solid_background((cui_color){0});
    assert(cui_plot_raster_backgrounds(pixels, 12, 10, 48, sizeof pixels, &shape, 1,
                                       (cui_color){0}) == CUI_PLOT_OK);
    assert(pixels[(4 * 12) * 4] == 239 && pixels[(4 * 12 + 7) * 4 + 2] == 239);
    shape.stroke.from.position = 2;
    memcpy(direct, pixels, sizeof pixels);
    assert(cui_plot_raster_backgrounds(pixels, 12, 10, 48, sizeof pixels, &shape, 1,
                                       (cui_color){0}) == CUI_PLOT_INVALID);
    assert(memcmp(direct, pixels, sizeof pixels) == 0);
    shape.stroke.from.position = 0;
    assert(cui_plot_raster_backgrounds(pixels, 12, 10, 48, 0, &shape, 1, (cui_color){0}) ==
           CUI_PLOT_CAPACITY);
    assert(memcmp(direct, pixels, sizeof pixels) == 0);
}
static void source_backgrounds(void) {
    cui_plot_rect frame = {0, 0, 8, 8};
    cui_color c, saved;
    cui_plot_background bg = cui_plot_linear_background_source(
        90, (cui_plot_color_stop){red, 0}, (cui_plot_color_stop){blue, 1}, CUI_PLOT_COLOR_OKLAB);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 4}, &c) == CUI_PLOT_OK);
    /* Published Oklab red/blue midpoint, encoded back to sRGB. */
    assert(c.r == 140 && c.g == 83 && c.b == 162 && c.a == 255);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){0, 4}, &c) == CUI_PLOT_OK);
    assert(memcmp(&c, &red, sizeof c) == 0);
    bg.from.color = (cui_color){0, 0, 0, 0};
    bg.to.color = (cui_color){255, 255, 255, 255};
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 4}, &c) == CUI_PLOT_OK);
    assert(c.r == 99 && c.g == 99 && c.b == 99 && c.a == 128);
    bg.color_space = CUI_PLOT_COLOR_SRGB;
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 4}, &c) == CUI_PLOT_OK);
    assert(c.r == 128 && c.a == 128);
    bg = cui_plot_linear_background_source(45, (cui_plot_color_stop){red, 0},
                                           (cui_plot_color_stop){blue, 1}, CUI_PLOT_COLOR_SRGB);
    assert(cui_plot_background_sample(&bg, (cui_plot_rect){0, 0, 16, 8}, (cui_plot_point){12, 4},
                                      &c) == CUI_PLOT_OK);
    assert(c.r == 70 && c.b == 185); /* short-axis rescale, not diagonal extent */
    bg = cui_plot_checkerboard((cui_color){20, 40, 60, 128}, 2);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){.5, .5}, &c) == CUI_PLOT_OK &&
           c.a == 0);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){2.5, .5}, &c) == CUI_PLOT_OK &&
           c.a == 128 && c.r == 20);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){2.5, 2.5}, &c) == CUI_PLOT_OK &&
           c.a == 0);
    assert(cui_plot_background_sample(&bg, (cui_plot_rect){10, 20, 26, 36},
                                      (cui_plot_point){12.5, 20.5}, &c) == CUI_PLOT_OK &&
           c.a == 128);
    /* Shader signed remainder: negative odd checks are transparent. */
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){-1, .5}, &c) == CUI_PLOT_OK &&
           c.a == 0);
    bg = cui_plot_pattern_slash((cui_color){20, 40, 60, 128}, 2, 2);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){0, 0}, &c) == CUI_PLOT_OK &&
           c.a == 128);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){1, 0}, &c) == CUI_PLOT_OK &&
           c.a >= 63 && c.a <= 65);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){2, 0}, &c) == CUI_PLOT_OK &&
           c.a == 0);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){4, 0}, &c) == CUI_PLOT_OK &&
           c.a == 128);
    assert(cui_plot_background_sample(&bg, (cui_plot_rect){10, 20, 26, 36},
                                      (cui_plot_point){11, 20}, &saved) == CUI_PLOT_OK);
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){1, 0}, &c) == CUI_PLOT_OK);
    assert(memcmp(&c, &saved, sizeof c) == 0);
    bg.pattern_width = 2.003; /* same source 1/255 quantized width */
    assert(cui_plot_background_sample(&bg, frame, (cui_plot_point){1, 0}, &saved) == CUI_PLOT_OK);
    assert(memcmp(&c, &saved, sizeof c) == 0);
    const cui_plot_background invalid[] = {
        cui_plot_pattern_slash(red, 0, 0),   cui_plot_pattern_slash(red, -1, 2),
        cui_plot_pattern_slash(red, 2, 257), cui_plot_pattern_slash(red, NAN, 2),
        cui_plot_pattern_slash(red, 258, 2), cui_plot_checkerboard(red, 0),
        cui_plot_checkerboard(red, 16385),   cui_plot_checkerboard(red, NAN)};
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        assert(cui_plot_background_sample(&invalid[i], frame, (cui_plot_point){1, 0}, &saved) ==
               CUI_PLOT_INVALID);
        assert(memcmp(&c, &saved, sizeof c) == 0);
    }
    cui_plot_path_command commands[] = {{.kind = CUI_PLOT_PATH_MOVE, .point = {0, 0}},
                                        {.kind = CUI_PLOT_PATH_LINE, .point = {8, 0}},
                                        {.kind = CUI_PLOT_PATH_LINE, .point = {8, 8}},
                                        {.kind = CUI_PLOT_PATH_LINE, .point = {0, 8}},
                                        {.kind = CUI_PLOT_PATH_CLOSE}};
    cui_plot_path_segment storage[8];
    cui_plot_path_cache cache;
    cui_plot_cached_path path;
    cache_builder builder = {commands, 5, 0, CUI_PLOT_OK};
    assert(cui_plot_path_cache_init(&cache, storage, 8) == CUI_PLOT_OK);
    assert(cui_plot_path_cache_get(&cache, 1, (cui_plot_point){0}, build_cache, &builder, &path) ==
           CUI_PLOT_OK);
    unsigned char pixels[8 * 8 * 4], before[sizeof pixels];
    cui_plot_background_shape shape = {.path = path,
                                       .background_frame = frame,
                                       .fill =
                                           cui_plot_checkerboard((cui_color){255, 0, 0, 128}, 2),
                                       .stroke = cui_plot_solid_background((cui_color){0})};
    assert(cui_plot_raster_backgrounds(pixels, 8, 8, 32, sizeof pixels, &shape, 1,
                                       (cui_color){0}) == CUI_PLOT_OK);
    assert(pixels[3] == 0 && pixels[2 * 4] == 255 && pixels[2 * 4 + 3] == 128);
    shape.fill = cui_plot_pattern_slash((cui_color){255, 0, 0, 0}, 2, 2);
    assert(cui_plot_raster_backgrounds(pixels, 8, 8, 32, sizeof pixels, &shape, 1, blue) ==
           CUI_PLOT_OK);
    for (size_t i = 0; i < sizeof pixels; i += 4)
        assert(pixels[i] == 0 && pixels[i + 2] == 255 && pixels[i + 3] == 255);
    shape.fill = cui_plot_linear_background_source(
        90, (cui_plot_color_stop){red, 0}, (cui_plot_color_stop){blue, 1}, CUI_PLOT_COLOR_OKLAB);
    assert(cui_plot_raster_backgrounds(pixels, 8, 8, 32, sizeof pixels, &shape, 1,
                                       (cui_color){0}) == CUI_PLOT_OK);
    assert(pixels[4 * 4 + 1] > 70); /* Oklab fill has a visible green component */
    shape.fill = cui_plot_solid_background((cui_color){0});
    shape.stroke_width = 2;
    shape.stroke = cui_plot_checkerboard(red, 2);
    assert(cui_plot_raster_backgrounds(pixels, 8, 8, 32, sizeof pixels, &shape, 1,
                                       (cui_color){0}) == CUI_PLOT_OK);
    assert(pixels[3] == 0 && pixels[2 * 4] == 255 && pixels[2 * 4 + 3] == 255);
    memcpy(before, pixels, sizeof pixels);
    shape.stroke = invalid[0];
    assert(cui_plot_raster_backgrounds(pixels, 8, 8, 32, sizeof pixels, &shape, 1, blue) ==
           CUI_PLOT_INVALID);
    assert(memcmp(before, pixels, sizeof pixels) == 0 && builder.builds == 1);
}
static void hsla_backgrounds(void) {
    cui_plot_rect frame = {0, 0, 8, 8};
    cui_plot_hsla_background bg = {.solid = {0, 1, .5f, .5f}};
    cui_plot_linear_rgba c, saved;
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){4, 4}, .5, &c) ==
           CUI_PLOT_OK);
    assert(fabs(c.r - .25) < 1e-6 && c.g == 0 && c.b == 0 && c.a == .25f);
    bg.solid = (cui_plot_hsla){1.f / 3, 1, .5f, 1};
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){4, 4}, 1, &c) ==
           CUI_PLOT_OK);
    assert(fabs(c.g - 1) < 1e-6 && fabs(c.r) < 1e-6 && fabs(c.b) < 1e-6);
    bg = (cui_plot_hsla_background){.kind = CUI_PLOT_BACKGROUND_LINEAR,
                                    .angle = 90,
                                    .from = {{0, 0, 0, 0}, 0},
                                    .to = {{0, 0, 1, 1}, 1}};
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){4, 4}, 1, &c) ==
           CUI_PLOT_OK);
    /* Source: linear Hsla endpoints -> encoded sRGB mix -> linear premultiplied. */
    assert(fabs(c.r - .1070205702) < 1e-6 && c.r == c.g && c.g == c.b && c.a == .5f);
    bg.color_space = CUI_PLOT_COLOR_OKLAB;
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){4, 4}, 1, &c) ==
           CUI_PLOT_OK);
    assert(fabs(c.r - .0625) < 1e-6 && fabs(c.g - .0625) < 1e-6 && fabs(c.b - .0625) < 1e-6 &&
           c.a == .5f);
    bg.from.color = (cui_plot_hsla){0, 1, .5f, 0};
    bg.to.color = (cui_plot_hsla){2.f / 3, 1, .5f, 1};
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){4, 4}, 1, &c) ==
           CUI_PLOT_OK);
    assert(fabs(c.r - .13186714) < 1e-5 && fabs(c.g - .04328584) < 1e-5 &&
           fabs(c.b - .18141216) < 1e-5 && c.a == .5f);
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){0, 4}, 1, &c) ==
           CUI_PLOT_OK);
    assert(c.r == 0 && c.g == 0 && c.b == 0 && c.a == 0);
    bg = (cui_plot_hsla_background){
        .kind = CUI_PLOT_BACKGROUND_CHECKERBOARD, .solid = {0, 0, .5f, .123f}, .pattern_size = 2};
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){2.5, .5}, 1, &c) ==
           CUI_PLOT_OK);
    assert(fabs(c.r - .0615) < 1e-7 && fabs(c.a - .123) < 1e-7);
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){.5, .5}, 1, &c) ==
               CUI_PLOT_OK &&
           c.a == 0 && c.r == 0);
    bg.kind = CUI_PLOT_BACKGROUND_SLASH;
    bg.pattern_width = 2;
    bg.pattern_interval = 2;
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){1, 0}, 1, &c) ==
           CUI_PLOT_OK);
    assert(c.a > .060 && c.a < .063 && fabs(c.r - c.a / 2) < 1e-7);
    saved = c;
    bg.solid.h = NAN;
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){1, 0}, 1, &c) ==
           CUI_PLOT_INVALID);
    assert(memcmp(&c, &saved, sizeof c) == 0);
    bg.solid.h = 0;
    assert(cui_plot_hsla_background_sample(&bg, frame, (cui_plot_point){1, 0}, -1, &c) ==
           CUI_PLOT_INVALID);
    assert(memcmp(&c, &saved, sizeof c) == 0);
    cui_plot_path_segment segments[] = {{{{0, 0}, {8, 0}}, true},
                                        {{{8, 0}, {8, 8}}, true},
                                        {{{8, 8}, {0, 8}}, true},
                                        {{{0, 8}, {0, 0}}, true}};
    cui_plot_hsla_shape shape = {.path = {segments, 4, {0, 0}},
                                 .background_frame = frame,
                                 .fill = {.solid = {0, 1, .5f, .5f}},
                                 .stroke = {.solid = {2.f / 3, 1, .5f, .5f}},
                                 .stroke_width = 2};
    cui_plot_linear_rgba pixels[8 * 8], before[8 * 8];
    assert(cui_plot_raster_hsla(pixels, 8, 8, 8, 64, &shape, 1, (cui_plot_linear_rgba){0}) ==
           CUI_PLOT_OK);
    assert(fabs(pixels[0].r - .25) < 1e-6 && fabs(pixels[0].b - .5) < 1e-6 && pixels[0].a == .75f);
    assert(pixels[4 * 8 + 4].r == .5f && pixels[4 * 8 + 4].b == 0 && pixels[4 * 8 + 4].a == .5f);
    shape.stroke_width = 0;
    assert(cui_plot_raster_hsla(pixels, 8, 8, 8, 64, &shape, 1,
                                (cui_plot_linear_rgba){0, 0, 1, 1}) == CUI_PLOT_OK);
    assert(pixels[0].r == .5f && pixels[0].b == .5f && pixels[0].a == 1);
    shape.fill = (cui_plot_hsla_background){.kind = CUI_PLOT_BACKGROUND_LINEAR,
                                            .angle = 90,
                                            .color_space = CUI_PLOT_COLOR_OKLAB,
                                            .from = {{0, 0, 0, 1}, 0},
                                            .to = {{0, 0, 1, 1}, 1}};
    assert(cui_plot_raster_hsla(pixels, 8, 8, 8, 64, &shape, 1, (cui_plot_linear_rgba){0}) ==
           CUI_PLOT_OK);
    assert(fabs(pixels[4].r - pow(.5625, 3)) < 1e-6);
    memcpy(before, pixels, sizeof pixels);
    shape.fill.from.position = NAN;
    assert(cui_plot_raster_hsla(pixels, 8, 8, 8, 64, &shape, 1, (cui_plot_linear_rgba){0}) ==
           CUI_PLOT_INVALID);
    assert(memcmp(before, pixels, sizeof pixels) == 0);
    cui_plot_linear_rgba padded[6];
    for (size_t i = 0; i < 6; i++)
        padded[i] = (cui_plot_linear_rgba){.25f, .25f, .25f, .5f};
    assert(cui_plot_raster_hsla(padded, 2, 2, 3, 6, NULL, 0, (cui_plot_linear_rgba){0}) ==
           CUI_PLOT_OK);
    assert(padded[0].a == 0 && padded[1].a == 0 && padded[3].a == 0 && padded[4].a == 0);
    assert(padded[2].a == .5f && padded[5].r == .25f);
    assert(cui_plot_raster_hsla(padded, 1, 2, SIZE_MAX / 2, SIZE_MAX, NULL, 0,
                                (cui_plot_linear_rgba){0}) == CUI_PLOT_INVALID);
    shape.fill.from.position = 0;
    assert(cui_plot_raster_hsla(pixels, 8, 8, 8, 63, &shape, 1, (cui_plot_linear_rgba){0}) ==
           CUI_PLOT_CAPACITY);
    assert(memcmp(before, pixels, sizeof pixels) == 0);
    assert(cui_plot_raster_hsla(pixels, 8, 8, 8, 64, &shape, 1,
                                (cui_plot_linear_rgba){1, 0, 0, 0}) == CUI_PLOT_INVALID);
    assert(memcmp(before, pixels, sizeof pixels) == 0);
}
int main(void) {
    hsla_backgrounds();
    source_backgrounds();
    software_backgrounds();
    styled_appearance();
    plot_elements();
    path_caches();
    test_sankey_raster();
    test_rounded_bar_raster();
#ifdef CUI_HAS_TEXT
    native_labels();
#endif
    cui_context *ui = cui_create((cui_config){.width = 100, .height = 100, .measure = measure});
    assert(ui);
    general_labels(ui);
    source_axis_grid(ui);
    axes(ui);
    grid_pixels(ui);
    marker_pixels(ui);
    raster();
    unsigned char pixels[64 * 64 * 4];
    assert(cui_plot_raster(pixels, 64, 64, 256, sizeof pixels, NULL, 0, blue) == CUI_PLOT_OK);
    activation a = {0};
    cui_plot_view_options o = {.bitmap = {pixels, 64, 64, 256, 1},
                               .name = {"Plot", 4},
                               .interactive = true,
                               .on_activate = activated,
                               .user = &a};
    cui_plot_view_options invalid_view = o;
    invalid_view.name = (cui_str){NULL, 1};
    assert(cui_begin_frame(ui, 100, 100, .016f));
    cui_theme theme = cui_theme_light();
    assert(cui_plot_view(ui, 44, &theme, &invalid_view) == CUI_PLOT_INVALID);
    cui_draw_list empty = cui_end_frame(ui);
    assert(empty.count == 0 && cui_last_error(ui) == CUI_OK);
    view_frame(ui, &o, true);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 10, .y = 12}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 10, .y = 12}));
    view_frame(ui, &o, false);
    assert(a.count == 1 && a.point.x == 10 && a.point.y == 12);
    view_frame(ui, &o, false);
    assert(a.count == 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 10, .y = 12}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 90, .y = 90}));
    view_frame(ui, &o, false);
    assert(a.count == 1);
    o.disabled = true;
    view_frame(ui, &o, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 10, .y = 12}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 10, .y = 12}));
    view_frame(ui, &o, false);
    assert(a.count == 1);
    o.disabled = false;
    o.interactive = false;
    view_frame(ui, &o, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 10, .y = 12}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 10, .y = 12}));
    view_frame(ui, &o, false);
    assert(a.count == 1);
    o.interactive = true;
    view_frame(ui, &o, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = 10,
                                           .y = 12,
                                           .pointer_button = CUI_POINTER_SECONDARY}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = 10,
                                           .y = 12,
                                           .pointer_button = CUI_POINTER_SECONDARY}));
    view_frame(ui, &o, false);
    assert(a.count == 1);
    cui_destroy(ui);
    return 0;
}
