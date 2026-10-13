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
    cui_software_destroy(renderer);
    cui_destroy(ui);
    cui_font_close(font);
}
#endif
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
int main(void) {
    test_rounded_bar_raster();
#ifdef CUI_HAS_TEXT
    native_labels();
#endif
    cui_context *ui = cui_create((cui_config){.width = 100, .height = 100, .measure = measure});
    assert(ui);
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
