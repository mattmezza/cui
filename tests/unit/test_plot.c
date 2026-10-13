/* SPDX-License-Identifier: MIT */
#include "cui/cui_plot.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
static bool close(double a, double b) {
    return fabs(a - b) <= 1e-11 * fmax(1, fmax(fabs(a), fabs(b)));
}
static bool relative(double a, double b) {
    return fabs(a - b) <= 1e-11 * fmax(fabs(a), fabs(b));
}
static uint64_t seed = 91;
static double random_unit(void) {
    seed = seed * UINT64_C(6364136223846793005) + 1;
    return (double)(seed >> 11) / 9007199254740992.0;
}
static void band_scales(void) {
    const uint64_t keys[] = {1, 2, 3, 1, 2, 3};
    uint64_t storage[8] = {99, 99, 99, 99, 99, 99, 99, 99};
    cui_plot_band_scale scale = {0};
    size_t required = 999, index = 999;
    cui_plot_band_options options = {.range_start = 0, .range_end = 90};
    assert(cui_plot_band_init(&scale, NULL, 0, keys, 6, options, &required) == CUI_PLOT_CAPACITY);
    assert(required == 3 && scale.count == 0);
    assert(cui_plot_band_init(&scale, storage, 2, keys, 6, options, &required) ==
           CUI_PLOT_CAPACITY);
    assert(storage[0] == 99 && scale.count == 0 && required == 3);
    assert(cui_plot_band_init(&scale, storage, 8, keys, 6, options, &required) == CUI_PLOT_OK);
    assert(required == 3 && scale.count == 3 && storage[0] == 1 && storage[1] == 2 &&
           storage[2] == 3);
    cui_plot_band_geometry geometry;
    assert(cui_plot_band_layout(&scale, &geometry) == CUI_PLOT_OK);
    assert(geometry.start == 0 && geometry.step == 30 && geometry.band_width == 30);
    double pixel = 1234;
    assert(cui_plot_band_map(&scale, 2, &pixel) == CUI_PLOT_OK && pixel == 30);
    assert(cui_plot_band_map(&scale, 9, &pixel) == CUI_PLOT_EMPTY && pixel == 30);
    assert(cui_plot_band_at(&scale, 3, &pixel) == CUI_PLOT_EMPTY && pixel == 30);
    assert(cui_plot_band_nearest(&scale, 15, &index) == CUI_PLOT_OK && index == 1);
    assert(cui_plot_band_nearest(&scale, -100, &index) == CUI_PLOT_OK && index == 0);
    assert(cui_plot_band_nearest(&scale, 100, &index) == CUI_PLOT_OK && index == 2);
    scale.options.padding_inner = .4;
    scale.options.padding_outer = .2;
    assert(cui_plot_band_layout(&scale, &geometry) == CUI_PLOT_OK);
    assert(close(geometry.start, 6) && close(geometry.step, 31.2) &&
           close(geometry.band_width, 18));
    assert(cui_plot_band_map(&scale, 3, &pixel) == CUI_PLOT_OK && close(pixel, 68.4));
    scale.options.has_max_band_width = true;
    scale.options.max_band_width = 10;
    assert(cui_plot_band_layout(&scale, &geometry) == CUI_PLOT_OK && geometry.band_width == 10);
    assert(cui_plot_band_map(&scale, 3, &pixel) == CUI_PLOT_OK && close(pixel, 68.4));
    scale.options.range_start = 90;
    scale.options.range_end = 0;
    assert(cui_plot_band_map(&scale, 1, &pixel) == CUI_PLOT_OK && close(pixel, 6));
    scale.options = (cui_plot_band_options){.range_start = 0,
                                            .range_end = 100,
                                            .padding_inner = .4,
                                            .padding_outer = .2,
                                            .band_count = 4};
    assert(cui_plot_band_layout(&scale, &geometry) == CUI_PLOT_OK && geometry.band_count == 4);
    assert(close(geometry.step, 25.5) && close(geometry.band_width, 15));
    assert(cui_plot_band_map(&scale, 2, &pixel) == CUI_PLOT_OK && close(pixel, 30.5));
    assert(cui_plot_band_nearest(&scale, 81.5, &index) == CUI_PLOT_OK && index == 3);
    scale.count = 1;
    assert(cui_plot_band_map(&scale, 1, &pixel) == CUI_PLOT_OK && close(pixel, 5));
    scale.options.band_count = 0;
    assert(cui_plot_band_map(&scale, 1, &pixel) == CUI_PLOT_OK && close(pixel, 20));
    scale.options.has_max_band_width = true;
    scale.options.max_band_width = 30;
    assert(cui_plot_band_map(&scale, 1, &pixel) == CUI_PLOT_OK && pixel == 35);
    assert(cui_plot_band_layout(&scale, &geometry) == CUI_PLOT_OK && geometry.step == 100);
    scale.options.range_start = scale.options.range_end = 12;
    assert(cui_plot_band_map(&scale, 1, &pixel) == CUI_PLOT_OK && pixel == 12);
    scale.count = 3;
    assert(cui_plot_band_nearest(&scale, 12, &index) == CUI_PLOT_OK && index == 0);
    assert(cui_plot_band_nearest(&scale, 13, &index) == CUI_PLOT_OK && index == 2);
    assert(cui_plot_band_init(&scale, NULL, 0, NULL, 0, options, &required) == CUI_PLOT_OK);
    assert(cui_plot_band_layout(&scale, &geometry) == CUI_PLOT_OK && geometry.band_width == 0);
    assert(cui_plot_band_nearest(&scale, 50, &index) == CUI_PLOT_OK && index == 0);
    scale.options.band_count = 4;
    assert(cui_plot_band_nearest(&scale, 60, &index) == CUI_PLOT_OK && index == 3);
    uint64_t aliased[] = {3, 1, 3, 2, 1};
    assert(cui_plot_band_init(&scale, aliased + 1, 4, aliased, 5, options, &required) ==
           CUI_PLOT_OK);
    assert(scale.count == 3 && aliased[1] == 3 && aliased[2] == 1 && aliased[3] == 2);
    cui_plot_band_scale saved = scale;
    uint64_t saved_keys[5];
    memcpy(saved_keys, aliased, sizeof aliased);
    cui_plot_band_options bad[] = {{.range_end = NAN},
                                   {.range_start = INFINITY},
                                   {.padding_inner = -1},
                                   {.padding_inner = 2},
                                   {.padding_outer = -1},
                                   {.padding_outer = 2},
                                   {.max_band_width = -1, .has_max_band_width = true},
                                   {.max_band_width = INFINITY, .has_max_band_width = true},
                                   {.band_count = CUI_PLOT_CATEGORY_LIMIT + 1}};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; ++i) {
        required = 987;
        assert(cui_plot_band_init(&scale, aliased, 5, keys, 6, bad[i], &required) != CUI_PLOT_OK);
        assert(required == 987 && !memcmp(&scale, &saved, sizeof scale) &&
               !memcmp(aliased, saved_keys, sizeof aliased));
    }
    pixel = 321;
    index = 432;
    geometry = (cui_plot_band_geometry){.start = 543};
    assert(cui_plot_band_map(NULL, 1, &pixel) == CUI_PLOT_INVALID && pixel == 321);
    assert(cui_plot_band_layout(&scale, NULL) == CUI_PLOT_INVALID);
    assert(cui_plot_band_nearest(&scale, NAN, &index) == CUI_PLOT_INVALID && index == 432);
    scale.options = (cui_plot_band_options){.range_start = -DBL_MAX, .range_end = DBL_MAX};
    scale.count = 1;
    assert(cui_plot_band_layout(&scale, &geometry) == CUI_PLOT_OVERFLOW && geometry.start == 543);
}
static void point_scales(void) {
    const uint64_t keys[] = {5, 7, 5, 9};
    cui_plot_point_scale scale;
    assert(cui_plot_point_init(&scale, keys, 4, 40, 100) == CUI_PLOT_OK);
    double pixel = 987;
    size_t index = 876;
    assert(cui_plot_point_map(&scale, 5, &pixel) == CUI_PLOT_OK && pixel == 40);
    /* Long double is binary64 on some targets; 2/3 interpolation may differ by one ULP. */
    assert(cui_plot_point_at(&scale, 2, &pixel) == CUI_PLOT_OK && close(pixel, 80));
    assert(cui_plot_point_at(&scale, 3, &pixel) == CUI_PLOT_OK && pixel == 100);
    assert(cui_plot_point_nearest(&scale, 50, &index) == CUI_PLOT_OK && index == 1);
    assert(cui_plot_point_nearest(&scale, 30, &index) == CUI_PLOT_OK && index == 0);
    assert(cui_plot_point_nearest(&scale, 110, &index) == CUI_PLOT_OK && index == 3);
    assert(cui_plot_point_init(&scale, keys, 4, 100, 40) == CUI_PLOT_OK);
    assert(cui_plot_point_at(&scale, 2, &pixel) == CUI_PLOT_OK && close(pixel, 60));
    assert(cui_plot_point_nearest(&scale, 90, &index) == CUI_PLOT_OK && index == 1);
    assert(cui_plot_point_nearest(&scale, 120, &index) == CUI_PLOT_OK && index == 0);
    assert(cui_plot_point_nearest(&scale, 10, &index) == CUI_PLOT_OK && index == 3);
    assert(cui_plot_point_init(&scale, keys, 1, 100, 40) == CUI_PLOT_OK);
    assert(cui_plot_point_at(&scale, 0, &pixel) == CUI_PLOT_OK && pixel == 70);
    assert(cui_plot_point_nearest(&scale, DBL_MAX, &index) == CUI_PLOT_OK && index == 0);
    assert(cui_plot_point_init(&scale, NULL, 0, 40, 100) == CUI_PLOT_OK);
    assert(cui_plot_point_nearest(&scale, 50, &index) == CUI_PLOT_OK && index == 0);
    pixel = 123;
    assert(cui_plot_point_map(&scale, 5, &pixel) == CUI_PLOT_EMPTY && pixel == 123);
    assert(cui_plot_point_at(&scale, 0, &pixel) == CUI_PLOT_EMPTY && pixel == 123);
    assert(cui_plot_point_init(&scale, keys, 4, 20, 20) == CUI_PLOT_OK);
    assert(cui_plot_point_at(&scale, 3, &pixel) == CUI_PLOT_OK && pixel == 20);
    assert(cui_plot_point_nearest(&scale, 30, &index) == CUI_PLOT_OK && index == 0);
    cui_plot_point_scale saved = scale;
    assert(cui_plot_point_init(&scale, NULL, 1, 0, 1) == CUI_PLOT_INVALID);
    assert(cui_plot_point_init(&scale, keys, CUI_PLOT_CATEGORY_LIMIT + 1, 0, 1) ==
           CUI_PLOT_CAPACITY);
    assert(cui_plot_point_init(&scale, keys, 4, NAN, 1) == CUI_PLOT_INVALID);
    assert(!memcmp(&scale, &saved, sizeof scale));
    assert(cui_plot_point_map(NULL, 1, &pixel) == CUI_PLOT_INVALID);
    assert(cui_plot_point_at(&scale, 0, NULL) == CUI_PLOT_INVALID);
    index = 987;
    assert(cui_plot_point_nearest(&scale, INFINITY, &index) == CUI_PLOT_INVALID && index == 987);
    assert(cui_plot_point_init(&scale, keys, 3, -DBL_MAX, DBL_MAX) == CUI_PLOT_OK);
    assert(cui_plot_point_at(&scale, 1, &pixel) == CUI_PLOT_OK && pixel == 0);
    assert(cui_plot_point_at(&scale, 2, &pixel) == CUI_PLOT_OK && pixel == DBL_MAX);
    for (unsigned i = 0; i < 500; ++i) {
        double start = random_unit() * 1000 - 500, end = random_unit() * 1000 - 500;
        if (fabs(start - end) < 1e-9)
            continue;
        assert(cui_plot_point_init(&scale, keys, 4, start, end) == CUI_PLOT_OK);
        for (size_t j = 0; j < 4; ++j) {
            assert(cui_plot_point_at(&scale, j, &pixel) == CUI_PLOT_OK);
            assert(close(pixel, start + (end - start) * (double)j / 3));
            assert(cui_plot_point_nearest(&scale, pixel, &index) == CUI_PLOT_OK && index == j);
        }
    }
}
static void ordinal_scales(void) {
    const uint64_t keys[] = {1, 2, 1, 3, 4, 5};
    const unsigned range[] = {10, 20, 30};
    unsigned unknown = 0, output = 987;
    cui_plot_ordinal_scale scale;
    assert(cui_plot_ordinal_init(&scale, keys, 6, range, 3, sizeof *range, NULL) == CUI_PLOT_OK);
    const unsigned expected[] = {10, 20, 10, 10, 20, 30};
    for (size_t i = 0; i < 6; ++i) {
        assert(cui_plot_ordinal_map(&scale, keys[i], &output, sizeof output) == CUI_PLOT_OK);
        assert(output == expected[i]);
    }
    output = 987;
    assert(cui_plot_ordinal_map(&scale, 99, &output, sizeof output) == CUI_PLOT_EMPTY &&
           output == 987);
    assert(cui_plot_ordinal_map(&scale, 1, &output, 1) == CUI_PLOT_CAPACITY && output == 987);
    assert(cui_plot_ordinal_map(&scale, 1, NULL, 0) == CUI_PLOT_CAPACITY);
    assert(cui_plot_ordinal_map(&scale, 1, NULL, sizeof output) == CUI_PLOT_INVALID);
    assert(cui_plot_ordinal_init(&scale, keys, 6, range, 3, sizeof *range, &unknown) ==
           CUI_PLOT_OK);
    assert(cui_plot_ordinal_map(&scale, 99, &output, sizeof output) == CUI_PLOT_OK && output == 0);
    assert(cui_plot_ordinal_init(&scale, keys, 6, NULL, 0, sizeof *range, &unknown) == CUI_PLOT_OK);
    output = 987;
    assert(cui_plot_ordinal_map(&scale, 1, &output, sizeof output) == CUI_PLOT_EMPTY &&
           output == 987);
    assert(cui_plot_ordinal_map(&scale, 99, &output, sizeof output) == CUI_PLOT_OK && output == 0);
    assert(cui_plot_ordinal_init(&scale, NULL, 0, range, 3, sizeof *range, &unknown) ==
           CUI_PLOT_OK);
    assert(cui_plot_ordinal_map(&scale, 1, &output, sizeof output) == CUI_PLOT_OK && output == 0);
    typedef struct {
        unsigned char r, g, b, a;
    } rgba;
    const rgba colors[] = {{1, 2, 3, 255}, {4, 5, 6, 128}};
    rgba color = {0};
    assert(cui_plot_ordinal_init(&scale, keys, 6, colors, 2, sizeof *colors, NULL) == CUI_PLOT_OK);
    assert(cui_plot_ordinal_map(&scale, 3, &color, sizeof color) == CUI_PLOT_OK);
    assert(!memcmp(&color, &colors[1], sizeof color));
    cui_plot_ordinal_scale saved = scale;
    assert(cui_plot_ordinal_init(&scale, keys, 6, colors, 2, 0, NULL) == CUI_PLOT_INVALID);
    assert(cui_plot_ordinal_init(&scale, keys, 6, NULL, 2, sizeof color, NULL) == CUI_PLOT_INVALID);
    assert(cui_plot_ordinal_init(&scale, NULL, 6, colors, 2, sizeof color, NULL) ==
           CUI_PLOT_INVALID);
    assert(cui_plot_ordinal_init(&scale, keys, 6, colors, SIZE_MAX, sizeof color, NULL) ==
           CUI_PLOT_CAPACITY);
    assert(cui_plot_ordinal_init(&scale, keys, 6, colors, 2, SIZE_MAX, NULL) == CUI_PLOT_CAPACITY);
    assert(!memcmp(&scale, &saved, sizeof scale));
    assert(cui_plot_ordinal_map(NULL, 1, &color, sizeof color) == CUI_PLOT_INVALID);
}
static void shape_geometry(void) {
    cui_plot_sample samples[] = {
        {0, 0, 2, true}, {1, 1, 4, true}, {NAN, NAN, NAN, false}, {3, 2, 8, true}};
    cui_plot_path_command path[32], saved[32];
    memset(path, 0x5a, sizeof path);
    memcpy(saved, path, sizeof path);
    size_t needed = 99;
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SKIP, false, NULL, 0,
                         &needed) == CUI_PLOT_CAPACITY &&
           needed == 3);
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SKIP, false, path, 2,
                         &needed) == CUI_PLOT_CAPACITY);
    assert(memcmp(path, saved, sizeof path) == 0);
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SPLIT, false, path, 32,
                         &needed) == CUI_PLOT_OK &&
           needed == 3);
    assert(path[0].kind == CUI_PLOT_PATH_MOVE && path[1].kind == CUI_PLOT_PATH_LINE &&
           path[2].kind == CUI_PLOT_PATH_MOVE && path[2].point.x == 3);
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SPLIT, true, path, 32,
                         &needed) == CUI_PLOT_OK &&
           needed == 8);
    assert(path[2].point.x == 1 && path[2].point.y == 1 && path[3].point.x == 0);
    assert(path[4].kind == CUI_PLOT_PATH_CLOSE && path[5].kind == CUI_PLOT_PATH_MOVE &&
           path[6].point.y == 2 && path[7].kind == CUI_PLOT_PATH_CLOSE);
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_NATURAL, CUI_PLOT_GAPS_SKIP, false, path, 32,
                         &needed) == CUI_PLOT_OK &&
           needed == 3);
    assert(path[1].kind == CUI_PLOT_PATH_CUBIC && close(path[1].control1.x, 1. / 6) &&
           close(path[1].control1.y, 2 + 2. / 6) && close(path[1].control2.x, .5) &&
           close(path[1].control2.y, 3));
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_STEP_AFTER, CUI_PLOT_GAPS_SKIP, false, path, 32,
                         &needed) == CUI_PLOT_OK &&
           needed == 4);
    assert(path[1].point.x == 1 && path[1].point.y == 2 && path[2].point.y == 4 &&
           path[3].point.x == 3 && path[3].point.y == 4);
    samples[0].y0 = NAN; /* Line ignores baseline. */
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SKIP, false, path, 32,
                         &needed) == CUI_PLOT_OK);
    memcpy(saved, path, sizeof path);
    needed = 99;
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SKIP, true, path, 32,
                         &needed) == CUI_PLOT_INVALID &&
           needed == 99);
    assert(memcmp(saved, path, sizeof path) == 0);
    samples[0].y0 = 0;
    samples[3].y1 = INFINITY;
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SKIP, false, path, 32,
                         &needed) == CUI_PLOT_INVALID);
    samples[3].y1 = 8;
    assert(cui_plot_path(samples, 4, (cui_plot_curve)99, CUI_PLOT_GAPS_SKIP, false, path, 32,
                         &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_path(samples, 4, CUI_PLOT_CURVE_LINEAR, (cui_plot_gap_policy)99, false, path,
                         32, &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_path(NULL, 1, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SKIP, false, path, 32,
                         &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_path(NULL, 0, CUI_PLOT_CURVE_LINEAR, CUI_PLOT_GAPS_SPLIT, true, NULL, 0,
                         &needed) == CUI_PLOT_OK &&
           needed == 0);
    cui_plot_sample extremes[] = {
        {-DBL_MAX, 0, 0, true}, {DBL_MAX, 0, 0, true}, {DBL_MAX, 0, 0, true}};
    needed = 99;
    assert(cui_plot_path(extremes, 3, CUI_PLOT_CURVE_NATURAL, CUI_PLOT_GAPS_SKIP, false, path, 32,
                         &needed) == CUI_PLOT_OVERFLOW &&
           needed == 99);
    assert(memcmp(saved, path, sizeof path) == 0);
    extremes[0].x = DBL_MAX;
    extremes[1].x = -DBL_MAX;
    assert(cui_plot_path(extremes, 3, CUI_PLOT_CURVE_NATURAL, CUI_PLOT_GAPS_SKIP, false, path, 32,
                         &needed) == CUI_PLOT_OK);
    assert(isfinite(path[1].control1.x) && path[1].control2.x == -DBL_MAX);
    cui_plot_value values[] = {{2, true},    {-3, true}, {4, true},
                               {NAN, false}, {-1, true}, {5, true}};
    cui_plot_stack_point stack[6], original[6];
    memset(stack, 0x5a, sizeof stack);
    memcpy(original, stack, sizeof stack);
    assert(cui_plot_stack(values, 3, 2, stack, 5, &needed) == CUI_PLOT_CAPACITY && needed == 6);
    assert(memcmp(stack, original, sizeof stack) == 0);
    assert(cui_plot_stack(values, 3, 2, stack, 6, &needed) == CUI_PLOT_OK);
    assert(stack[0].y0 == 0 && stack[0].y1 == 2 && stack[2].y0 == 2 && stack[2].y1 == 6 &&
           stack[4].y0 == 6 && stack[4].y1 == 5);
    assert(stack[1].y1 == -3 && stack[3].y0 == -3 && stack[3].y1 == -3 && stack[5].y1 == 2);
    memcpy(original, stack, sizeof stack);
    needed = 99;
    values[0].value = DBL_MAX;
    values[2].value = DBL_MAX;
    assert(cui_plot_stack(values, 3, 2, stack, 6, &needed) == CUI_PLOT_OVERFLOW && needed == 99);
    assert(memcmp(stack, original, sizeof stack) == 0);
    values[0].value = NAN;
    assert(cui_plot_stack(values, 3, 2, stack, 6, &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_stack(NULL, 4096, 4096, stack, 6, &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_stack(NULL, 0, 0, NULL, 0, &needed) == CUI_PLOT_OK && needed == 0);
}
static void pie_geometry(void) {
    const double pi = acos(-1.);
    cui_plot_value values[] = {{0, true}, {1, true}, {NAN, false}, {-1, true}, {3, true}};
    cui_plot_arc arcs[4], saved[4];
    memset(arcs, 0x5a, sizeof arcs);
    memcpy(saved, arcs, sizeof arcs);
    size_t required = 99;
    assert(cui_plot_pie(values, 5, 0, 2 * pi, .2, NULL, 0, &required) == CUI_PLOT_CAPACITY &&
           required == 2);
    assert(cui_plot_pie(values, 5, 0, 2 * pi, .2, arcs, 1, &required) == CUI_PLOT_CAPACITY);
    assert(memcmp(arcs, saved, sizeof arcs) == 0);
    assert(cui_plot_pie(values, 5, 0, 2 * pi, .2, arcs, 4, &required) == CUI_PLOT_OK);
    assert(arcs[0].index == 1 && arcs[1].index == 4 && arcs[0].value == 1 &&
           arcs[0].start_angle == 0 && close(arcs[0].end_angle, pi / 2) &&
           arcs[1].start_angle == arcs[0].end_angle && arcs[1].end_angle == 2 * pi &&
           arcs[1].pad_angle == .2);
    assert(cui_plot_pie(values, 5, 2 * pi, 0, 0, arcs, 4, &required) == CUI_PLOT_OK &&
           close(arcs[0].end_angle, 1.5 * pi) && arcs[1].end_angle == 0);
    values[1].value = DBL_MAX;
    values[4].value = DBL_MAX;
    assert(cui_plot_pie(values, 5, -DBL_MAX, DBL_MAX, 0, arcs, 4, &required) == CUI_PLOT_OK);
    assert(arcs[0].end_angle == 0 && arcs[1].end_angle == DBL_MAX);
    memcpy(saved, arcs, sizeof arcs);
    required = 99;
    values[4].value = INFINITY;
    assert(cui_plot_pie(values, 5, 0, pi, 0, arcs, 4, &required) == CUI_PLOT_INVALID &&
           required == 99);
    assert(memcmp(arcs, saved, sizeof arcs) == 0);
    assert(cui_plot_pie(NULL, 0, 0, pi, -1, NULL, 0, &required) == CUI_PLOT_INVALID);
    assert(cui_plot_pie(NULL, 0, 0, pi, 0, NULL, 0, &required) == CUI_PLOT_OK && required == 0);
    cui_plot_arc half = {0, 1, 0, pi, .8};
    cui_plot_point centroid = {99, 99};
    bool inside = true;
    assert(cui_plot_arc_centroid(&half, 10, 40, &centroid) == CUI_PLOT_OK &&
           close(centroid.x, 25) && close(centroid.y, 0));
    assert(cui_plot_arc_contains(&half, 10, 40, (cui_plot_point){30, 0}, &inside) == CUI_PLOT_OK &&
           inside);
    assert(cui_plot_arc_contains(&half, 10, 40, (cui_plot_point){0, -30}, &inside) == CUI_PLOT_OK &&
           inside);
    assert(cui_plot_arc_contains(&half, 10, 40, (cui_plot_point){0, 30}, &inside) == CUI_PLOT_OK &&
           !inside);
    assert(cui_plot_arc_contains(&half, 10, 40, (cui_plot_point){-30, 0}, &inside) == CUI_PLOT_OK &&
           !inside);
    assert(cui_plot_arc_contains(&half, 10, 40, (cui_plot_point){5, 0}, &inside) == CUI_PLOT_OK &&
           !inside);
    assert(cui_plot_arc_contains(&half, 10, 40, (cui_plot_point){40, 0}, &inside) == CUI_PLOT_OK &&
           inside);
    assert(cui_plot_arc_contains(&half, 10, 40, (cui_plot_point){41, 0}, &inside) == CUI_PLOT_OK &&
           !inside);
    inside = true;
    assert(cui_plot_arc_contains(&half, 40, 10, (cui_plot_point){30, 0}, &inside) ==
               CUI_PLOT_INVALID &&
           inside);
    centroid = (cui_plot_point){99, 99};
    assert(cui_plot_arc_centroid(&half, -1, 40, &centroid) == CUI_PLOT_INVALID && centroid.x == 99);
    half.start_angle = pi;
    half.end_angle = 0;
    assert(cui_plot_arc_contains(&half, 10, 40, (cui_plot_point){30, 0}, &inside) == CUI_PLOT_OK &&
           !inside);
}
static void arc_and_radial_paths(void) {
    const double pi = acos(-1.);
    cui_plot_arc arc = {0, 1, 0, pi / 2, 0};
    cui_plot_path_command path[2053], saved[8];
    memset(path, 0x5a, sizeof path);
    memcpy(saved, path, sizeof saved);
    size_t needed = 99;
    assert(cui_plot_arc_path(&arc, 10, 20, NULL, 0, &needed) == CUI_PLOT_CAPACITY && needed == 5);
    assert(cui_plot_arc_path(&arc, 10, 20, path, 4, &needed) == CUI_PLOT_CAPACITY);
    assert(memcmp(path, saved, sizeof saved) == 0);
    assert(cui_plot_arc_path(&arc, 10, 20, path, 8, &needed) == CUI_PLOT_OK);
    assert(path[0].kind == CUI_PLOT_PATH_MOVE && close(path[0].point.x, 0) &&
           close(path[0].point.y, -20));
    assert(path[1].kind == CUI_PLOT_PATH_ARC && path[1].clockwise && !path[1].large_arc &&
           path[1].radius == 20 && close(path[1].point.x, 20) && close(path[1].point.y, 0));
    assert(path[2].kind == CUI_PLOT_PATH_LINE && close(path[2].point.x, 10));
    assert(path[3].kind == CUI_PLOT_PATH_ARC && !path[3].clockwise && path[3].radius == 10 &&
           path[4].kind == CUI_PLOT_PATH_CLOSE);
    arc.end_angle = 1.5 * pi;
    assert(cui_plot_arc_path(&arc, 10, 20, path, 8, &needed) == CUI_PLOT_OK && path[1].large_arc &&
           path[3].large_arc);
    arc.end_angle = 2 * pi;
    arc.pad_angle = .5;
    assert(cui_plot_arc_path(&arc, 10, 20, path, 8, &needed) == CUI_PLOT_OK && needed == 7);
    assert(path[1].kind == CUI_PLOT_PATH_ARC && path[2].kind == CUI_PLOT_PATH_ARC &&
           path[4].kind == CUI_PLOT_PATH_ARC && path[5].kind == CUI_PLOT_PATH_ARC &&
           !path[4].clockwise && close(path[0].point.y, -20) && close(path[2].point.y, -20));
    assert(!path[1].large_arc && !path[2].large_arc && !path[4].large_arc && !path[5].large_arc);
    assert(cui_plot_arc_flatten(&arc, 10, 20, 8, path, 2053, &needed) == CUI_PLOT_OK &&
           needed == 19);
    assert(path[0].kind == CUI_PLOT_PATH_MOVE && path[18].kind == CUI_PLOT_PATH_CLOSE);
    for (size_t i = 0; i < 9; ++i)
        assert(close(hypot(path[i].point.x, path[i].point.y), 20));
    for (size_t i = 9; i < 18; ++i)
        assert(close(hypot(path[i].point.x, path[i].point.y), 10));
    assert(close(path[2].point.x, 20) && close(path[2].point.y, 0));
    const double midpoint_radius =
        hypot((path[0].point.x + path[1].point.x) / 2, (path[0].point.y + path[1].point.y) / 2);
    assert(close(20 - midpoint_radius, 20 * (1 - cos(pi / 8))));

    assert(cui_plot_arc_flatten(&arc, 10, 20, 1024, NULL, 0, &needed) == CUI_PLOT_CAPACITY &&
           needed == 2051);
    arc.end_angle = pi / 2;
    arc.pad_angle = .2;
    assert(cui_plot_arc_path(&arc, 5, 20, path, 8, &needed) == CUI_PLOT_OK);
    assert(close(path[0].point.x, 20 * sin(.1)) && close(path[0].point.y, -20 * cos(.1)));
    assert(close(path[2].point.x, 5 * cos(.4)) && close(path[2].point.y, -5 * sin(.4)));
    arc.pad_angle = 1;
    assert(cui_plot_arc_path(&arc, 1, 20, path, 8, &needed) == CUI_PLOT_OK);
    assert(close(path[2].point.x, cos(pi * .2)) && close(path[2].point.y, -sin(pi * .2)));
    arc.pad_angle = 2;
    assert(cui_plot_arc_path(&arc, 0, 20, NULL, 0, &needed) == CUI_PLOT_OK && needed == 0);
    assert(cui_plot_arc_flatten(&arc, 0, 20, 8, NULL, 0, &needed) == CUI_PLOT_OK && needed == 0);
    arc.pad_angle = 0;
    assert(cui_plot_arc_path(&arc, 0, 20, path, 8, &needed) == CUI_PLOT_OK && needed == 4 &&
           path[2].point.x == 0 && path[2].point.y == 0);
    assert(cui_plot_arc_flatten(&arc, 0, 20, 8, path, 2053, &needed) == CUI_PLOT_OK && needed == 5);
    assert(cui_plot_arc_path(&arc, 0, 0, NULL, 0, &needed) == CUI_PLOT_OK && needed == 0);
    arc.end_angle = -1;
    assert(cui_plot_arc_path(&arc, 0, 20, NULL, 0, &needed) == CUI_PLOT_OK && needed == 0);
    arc.end_angle = 3 * pi;
    needed = 99;
    memcpy(saved, path, sizeof saved);
    assert(cui_plot_arc_path(&arc, 0, 20, path, 8, &needed) == CUI_PLOT_INVALID && needed == 99);
    assert(memcmp(saved, path, sizeof saved) == 0);
    assert(cui_plot_arc_flatten(&arc, 0, 20, 8, path, 8, &needed) == CUI_PLOT_INVALID);
    arc.end_angle = pi / 2;
    assert(cui_plot_arc_path(&arc, 20, 10, path, 8, &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_arc_path(NULL, 0, 20, path, 8, &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_arc_flatten(&arc, 0, 20, 3, path, 8, &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_arc_flatten(&arc, 0, 20, 1025, path, 8, &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_arc_flatten(&arc, 0, 20, 8, path, 1, &needed) == CUI_PLOT_CAPACITY &&
           memcmp(saved, path, sizeof saved) == 0);
    assert(cui_plot_arc_path(&arc, DBL_TRUE_MIN, DBL_MAX, path, 8, &needed) == CUI_PLOT_OK);
    assert(isfinite(path[3].point.x) && path[3].point.y == -DBL_TRUE_MIN);
    cui_plot_radial_sample samples[] = {{0, 5, 10, true},
                                        {pi / 2, 5, 10, true},
                                        {NAN, NAN, NAN, false},
                                        {pi, 5, 10, true},
                                        {1.5 * pi, 5, 10, true}};
    assert(cui_plot_radial_path(samples, 5, CUI_PLOT_GAPS_SKIP, false, false, false, path, 8,
                                &needed) == CUI_PLOT_OK &&
           needed == 4);
    assert(close(path[0].point.y, -10) && close(path[1].point.x, 10) &&
           close(path[2].point.y, 10) && close(path[3].point.x, -10));
    assert(cui_plot_radial_path(samples, 5, CUI_PLOT_GAPS_SKIP, true, false, false, path, 8,
                                &needed) == CUI_PLOT_OK &&
           needed == 5 && path[4].kind == CUI_PLOT_PATH_CLOSE);
    assert(cui_plot_radial_path(samples, 5, CUI_PLOT_GAPS_SKIP, false, true, false, path, 8,
                                &needed) == CUI_PLOT_OK &&
           needed == 5);
    assert(cui_plot_radial_path(samples, 5, CUI_PLOT_GAPS_SPLIT, true, false, false, path, 8,
                                &needed) == CUI_PLOT_OK &&
           needed == 4 && path[2].kind == CUI_PLOT_PATH_MOVE);
    assert(cui_plot_radial_path(samples, 5, CUI_PLOT_GAPS_SPLIT, false, true, false, NULL, 0,
                                &needed) == CUI_PLOT_OK &&
           needed == 0);
    assert(cui_plot_radial_path(samples, 5, CUI_PLOT_GAPS_SPLIT, false, false, true, path, 16,
                                &needed) == CUI_PLOT_OK &&
           needed == 10);
    assert(path[4].kind == CUI_PLOT_PATH_CLOSE && path[9].kind == CUI_PLOT_PATH_CLOSE &&
           close(path[2].point.x, 5));
    assert(cui_plot_radial_path(samples, 1, CUI_PLOT_GAPS_SKIP, true, false, false, path, 8,
                                &needed) == CUI_PLOT_OK &&
           needed == 1);
    memcpy(saved, path, sizeof saved);
    needed = 99;
    samples[4].radius1 = -1;
    assert(cui_plot_radial_path(samples, 5, CUI_PLOT_GAPS_SKIP, false, false, false, path, 8,
                                &needed) == CUI_PLOT_INVALID &&
           needed == 99);
    assert(memcmp(saved, path, sizeof saved) == 0);
    samples[4].radius1 = 10;
    samples[0].radius0 = NAN;
    assert(cui_plot_radial_path(samples, 5, CUI_PLOT_GAPS_SKIP, false, false, false, NULL, 0,
                                &needed) == CUI_PLOT_CAPACITY &&
           needed == 4);
    assert(cui_plot_radial_path(samples, 5, CUI_PLOT_GAPS_SKIP, false, false, true, path, 16,
                                &needed) == CUI_PLOT_INVALID);
    assert(cui_plot_radial_path(NULL, 0, CUI_PLOT_GAPS_SKIP, false, false, false, NULL, 0,
                                &needed) == CUI_PLOT_OK &&
           needed == 0);
}
static void bar_and_grid_geometry(void) {
    cui_plot_bar_geometry bar, original;
    memset(&bar, 0x5a, sizeof bar);
    original = bar;
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_BOTTOM, 10, 8, 50, 20, &bar) == CUI_PLOT_OK);
    assert(bar.frame.x_min == 10 && bar.frame.x_max == 18 && bar.frame.y_min == 20 &&
           bar.frame.y_max == 50 && bar.label_origin.x == 14 && bar.label_origin.y == 8 &&
           bar.gradient_angle == 0);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_BOTTOM, 10, 8, 20, 50, &bar) == CUI_PLOT_OK &&
           bar.label_origin.y == 52);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_TOP, 10, 8, 20, 50, &bar) == CUI_PLOT_OK &&
           bar.label_origin.y == 52 && bar.gradient_angle == 180);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_TOP, 10, 8, 50, 20, &bar) == CUI_PLOT_OK &&
           bar.label_origin.y == 8);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_LEFT, 10, 8, 20, 50, &bar) == CUI_PLOT_OK &&
           bar.frame.x_min == 20 && bar.frame.x_max == 50 && bar.frame.y_min == 10 &&
           bar.frame.y_max == 18 && bar.label_origin.x == 52 && bar.label_origin.y == 9 &&
           bar.gradient_angle == 90);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_LEFT, 10, 8, 50, 20, &bar) == CUI_PLOT_OK &&
           bar.label_origin.x == 18);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_RIGHT, 10, 8, 50, 20, &bar) == CUI_PLOT_OK &&
           bar.label_origin.x == 18 && bar.gradient_angle == 270);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_RIGHT, 10, 8, 20, 50, &bar) == CUI_PLOT_OK &&
           bar.label_origin.x == 52);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_BOTTOM, 10, 0, 20, 20, &bar) == CUI_PLOT_OK &&
           bar.label_origin.y == 8);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_TOP, 10, 0, 20, 20, &bar) == CUI_PLOT_OK &&
           bar.label_origin.y == 22);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_LEFT, 10, 0, 20, 20, &bar) == CUI_PLOT_OK &&
           bar.label_origin.x == 22);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_RIGHT, 10, 0, 20, 20, &bar) == CUI_PLOT_OK &&
           bar.label_origin.x == 18);
    original = bar;
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_LEFT, DBL_MAX, DBL_MAX, 0, 1, &bar) ==
               CUI_PLOT_OVERFLOW &&
           memcmp(&bar, &original, sizeof bar) == 0);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_LEFT, 0, -1, 0, 1, &bar) == CUI_PLOT_INVALID);
    assert(cui_plot_bar_layout((cui_plot_bar_alignment)99, 0, 1, 0, 1, &bar) == CUI_PLOT_INVALID);
    assert(cui_plot_bar_layout(CUI_PLOT_BAR_LEFT, 0, 1, NAN, 1, &bar) == CUI_PLOT_INVALID);
    double x[] = {10}, y[] = {5}, dashes[] = {4, 2, 1};
    cui_plot_grid_options o = {
        .width = 20, .height = 11, .x = x, .y = y, .x_count = 1, .y_count = 1};
    cui_plot_rect quads[16], saved[16];
    size_t required = 99;
    memset(quads, 0x5a, sizeof quads);
    memcpy(saved, quads, sizeof quads);
    assert(cui_plot_grid_layout(&o, quads, 1, &required) == CUI_PLOT_CAPACITY && required == 2 &&
           memcmp(quads, saved, sizeof quads) == 0);
    assert(cui_plot_grid_layout(&o, quads, 16, &required) == CUI_PLOT_OK && required == 2);
    assert(quads[0].x_min == 9.5 && quads[0].x_max == 10.5 && quads[0].y_min == 0 &&
           quads[0].y_max == 11);
    assert(quads[1].x_min == 0 && quads[1].x_max == 20 && quads[1].y_min == 4.5 &&
           quads[1].y_max == 5.5);
    o.x_count = 0;
    o.dashes = dashes;
    o.dash_count = 2;
    o.width = 11;
    assert(cui_plot_grid_layout(&o, quads, 16, &required) == CUI_PLOT_OK && required == 2);
    assert(quads[0].x_min == 0 && quads[0].x_max == 4 && quads[1].x_min == 6 &&
           quads[1].x_max == 10);
    o.dash_count = 3;
    o.width = 20;
    assert(cui_plot_grid_layout(&o, quads, 16, &required) == CUI_PLOT_OK && required == 4);
    assert(quads[1].x_min == 6 && quads[1].x_max == 7 && quads[2].x_min == 11 &&
           quads[2].x_max == 13 && quads[3].x_min == 14 && quads[3].x_max == 18);
    dashes[0] = -1;
    dashes[1] = 0;
    o.dash_count = 2;
    assert(cui_plot_grid_layout(&o, quads, 16, &required) == CUI_PLOT_OK && required == 1 &&
           quads[0].x_max == 20);
    dashes[1] = 2;
    assert(cui_plot_grid_layout(&o, quads, 16, &required) == CUI_PLOT_OK && required == 0);
    dashes[0] = 1;
    dashes[1] = DBL_TRUE_MIN;
    memcpy(saved, quads, sizeof quads);
    required = 99;
    assert(cui_plot_grid_layout(&o, quads, 16, &required) == CUI_PLOT_DEGENERATE &&
           required == 99 && memcmp(quads, saved, sizeof quads) == 0);
    dashes[0] = .001;
    dashes[1] = .001;
    o.width = 20;
    assert(cui_plot_grid_layout(&o, quads, 16, &required) == CUI_PLOT_CAPACITY &&
           required == 4097 && memcmp(quads, saved, sizeof quads) == 0);
    dashes[0] = NAN;
    required = 99;
    assert(cui_plot_grid_layout(&o, quads, 16, &required) == CUI_PLOT_INVALID && required == 99);
    o = (cui_plot_grid_options){.width = 20, .height = 11};
    assert(cui_plot_grid_layout(&o, NULL, 0, &required) == CUI_PLOT_OK && required == 0);
    o.y_count = 1;
    assert(cui_plot_grid_layout(&o, quads, 16, &required) == CUI_PLOT_INVALID);
}
static void source_area_paths(void) {
    cui_plot_area_sample samples[] = {
        {0, 1, true, true}, {1, 3, true, true}, {2, NAN, true, false}, {3, 2, true, true}};
    cui_plot_path_command fill[32], stroke[32], saved_fill[32], saved_stroke[32];
    memset(fill, 0x5a, sizeof fill);
    memset(stroke, 0x5a, sizeof stroke);
    memcpy(saved_fill, fill, sizeof fill);
    memcpy(saved_stroke, stroke, sizeof stroke);
    cui_plot_area_buffers buffers = {.fill = fill,
                                     .stroke = stroke,
                                     .fill_capacity = 2,
                                     .stroke_capacity = 32,
                                     .fill_count = 99,
                                     .stroke_count = 99};
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_LINEAR, true, 5, &buffers) ==
               CUI_PLOT_CAPACITY &&
           buffers.fill_count == 6 && buffers.stroke_count == 3);
    assert(memcmp(saved_fill, fill, sizeof fill) == 0 &&
           memcmp(saved_stroke, stroke, sizeof stroke) == 0);
    buffers.fill_capacity = 32;
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_LINEAR, true, 5, &buffers) ==
           CUI_PLOT_OK);
    assert(fill[0].kind == CUI_PLOT_PATH_MOVE && fill[1].point.x == 1 && fill[2].point.x == 3 &&
           fill[3].point.x == 3 && fill[3].point.y == 5 && fill[4].point.x == 0 &&
           fill[5].kind == CUI_PLOT_PATH_CLOSE);
    assert(stroke[2].kind == CUI_PLOT_PATH_LINE && stroke[2].point.x == 3);
    assert(memcmp(fill, stroke, 3 * sizeof *fill) == 0);
    /* The source closes against original endpoint x even if its y is missing. */
    samples[0].has_y = false;
    samples[3].has_y = false;
    samples[2].has_y = true;
    samples[2].y = 2;
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_LINEAR, true, 5, &buffers) ==
               CUI_PLOT_OK &&
           buffers.fill_count == 5 && buffers.stroke_count == 2);
    assert(fill[0].point.x == 1 && fill[1].point.x == 2 && fill[2].point.x == 3 &&
           fill[3].point.x == 0);
    samples[0].has_x = false;
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_LINEAR, true, 5, &buffers) ==
               CUI_PLOT_OK &&
           buffers.fill_count == 2 && buffers.stroke_count == 2);
    assert(fill[1].kind == CUI_PLOT_PATH_LINE); /* Open fill contour. */
    samples[0].has_x = true;
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_LINEAR, false, NAN, &buffers) ==
               CUI_PLOT_OK &&
           buffers.fill_count == 2);
    samples[2].has_y = false;
    assert(
        cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_NATURAL, true, 5, &buffers) == CUI_PLOT_OK &&
        buffers.fill_count == 1 && buffers.stroke_count == 1 && fill[0].kind == CUI_PLOT_PATH_MOVE);
    samples[0].has_y = true;
    samples[2].has_y = true;
    samples[3].has_y = true;
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_NATURAL, true, 5, &buffers) ==
               CUI_PLOT_OK &&
           stroke[1].kind == CUI_PLOT_PATH_CUBIC);
    assert(close(stroke[1].control1.x, 1. / 6) && close(stroke[1].control1.y, 1 + 2. / 6));
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_STEP_AFTER, true, 5, &buffers) ==
               CUI_PLOT_OK &&
           buffers.stroke_count == 6 && buffers.fill_count == 9);
    assert(stroke[5].point.x == 3 && stroke[5].point.y == 2 && fill[6].point.y == 5);
    memcpy(saved_fill, fill, sizeof fill);
    memcpy(saved_stroke, stroke, sizeof stroke);
    buffers.fill_count = buffers.stroke_count = 99;
    samples[3].y = NAN;
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_LINEAR, true, 5, &buffers) ==
               CUI_PLOT_INVALID &&
           buffers.fill_count == 99 && buffers.stroke_count == 99);
    assert(memcmp(saved_fill, fill, sizeof fill) == 0 &&
           memcmp(saved_stroke, stroke, sizeof stroke) == 0);
    samples[3].y = 2;
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_LINEAR, true, NAN, &buffers) ==
           CUI_PLOT_INVALID);
    assert(cui_plot_area_paths(NULL, 1, CUI_PLOT_CURVE_LINEAR, false, 0, &buffers) ==
           CUI_PLOT_INVALID);
    assert(cui_plot_area_paths(samples, 4, (cui_plot_curve)99, false, 0, &buffers) ==
           CUI_PLOT_INVALID);
    cui_plot_area_buffers query = {0};
    assert(cui_plot_area_paths(samples, 4, CUI_PLOT_CURVE_LINEAR, true, 5, &query) ==
               CUI_PLOT_CAPACITY &&
           query.fill_count == 7 && query.stroke_count == 4);
    assert(cui_plot_area_paths(NULL, 0, CUI_PLOT_CURVE_LINEAR, false, 0, &query) == CUI_PLOT_OK &&
           query.fill_count == 0 && query.stroke_count == 0);
    cui_plot_area_sample extreme[] = {
        {-DBL_MAX, 0, true, true}, {DBL_MAX, 0, true, true}, {DBL_MAX, 0, true, true}};
    assert(cui_plot_area_paths(extreme, 3, CUI_PLOT_CURVE_NATURAL, true, 5, &buffers) ==
               CUI_PLOT_OVERFLOW &&
           buffers.fill_count == 99 && buffers.stroke_count == 99);
    assert(memcmp(saved_fill, fill, sizeof fill) == 0 &&
           memcmp(saved_stroke, stroke, sizeof stroke) == 0);
}
static void dot_geometry(void) {
    cui_plot_dot_geometry dot;
    assert(cui_plot_dot_layout((cui_plot_point){10, 20}, 4, &dot) == CUI_PLOT_OK);
    assert(dot.frame.x_min == 8 && dot.frame.x_max == 12 && dot.frame.y_min == 18 &&
           dot.frame.y_max == 22 && dot.radius == 2 && dot.border_width == 1);
    assert(cui_plot_dot_layout((cui_plot_point){-10, -20}, 8, &dot) == CUI_PLOT_OK &&
           dot.frame.x_min == -14 && dot.frame.y_max == -16);
    assert(cui_plot_dot_layout((cui_plot_point){1, 2}, 0, &dot) == CUI_PLOT_OK && dot.radius == 0 &&
           dot.frame.x_min == dot.frame.x_max);
    cui_plot_dot_geometry before = dot;
    assert(cui_plot_dot_layout((cui_plot_point){DBL_MAX, 0}, DBL_MAX, &dot) == CUI_PLOT_OVERFLOW);
    assert(memcmp(&before, &dot, sizeof dot) == 0);
    assert(cui_plot_dot_layout((cui_plot_point){0, 0}, -1, &dot) == CUI_PLOT_INVALID);
    assert(cui_plot_dot_layout((cui_plot_point){NAN, 0}, 4, &dot) == CUI_PLOT_INVALID);
    assert(cui_plot_dot_layout((cui_plot_point){0, 0}, INFINITY, &dot) == CUI_PLOT_INVALID);
    assert(cui_plot_dot_layout((cui_plot_point){0, 0}, 4, NULL) == CUI_PLOT_INVALID);
    assert(memcmp(&before, &dot, sizeof dot) == 0);
}
static void test_sankey_large_chain(void) {
    cui_plot_sankey_node nodes[256];
    cui_plot_sankey_link_layout links[255];
    cui_plot_sankey_link input[255];
    for (size_t i = 0; i < 255; ++i)
        input[i] = (cui_plot_sankey_link){i, i + 1, 1};
    cui_plot_sankey_graph graph = {nodes, links, 0, 0, 256, 255};
    cui_plot_sankey_options o = cui_plot_sankey_defaults();
    o.extent = (cui_plot_rect){0, 0, 512, 128};
    o.node_width = 2;
    assert(cui_plot_sankey_layout(&graph, 256, input, 255, &o, NULL) == CUI_PLOT_OK);
    assert(cui_plot_sankey_layer_count(&graph) == 256);
    assert(nodes[0].height == 255 && nodes[255].depth == 255 && nodes[255].x1 == 512);
    for (size_t i = 0; i < 255; ++i) {
        assert(links[i].source == i && links[i].target == i + 1);
        assert(close(links[i].source_width, 128) && close(links[i].target_width, 128));
    }
}
static void test_sankey_graph(void) {
    cui_plot_sankey_node nodes[8], old_nodes[8];
    cui_plot_sankey_link_layout links[8], old_links[8];
    memset(nodes, 0x5a, sizeof nodes);
    memset(links, 0x5a, sizeof links);
    cui_plot_sankey_graph graph = {nodes, links, 0, 0, 8, 8};
    cui_plot_sankey_options o = cui_plot_sankey_defaults();
    assert(o.node_width == 24 && o.node_padding == 8 && o.iterations == 6 &&
           o.align == CUI_PLOT_SANKEY_JUSTIFY && o.value_scale == CUI_PLOT_SANKEY_LINEAR);
    o.extent = (cui_plot_rect){10, 20, 110, 120};
    o.node_width = 10;
    cui_plot_sankey_link input[] = {{0, 1, 5}, {1, 2, 5}};
    cui_plot_sankey_error error;
    assert(cui_plot_sankey_topology(&graph, 3, input, 2, &o, &error) == CUI_PLOT_OK);
    assert(graph.node_count == 3 && graph.link_count == 2 &&
           error.kind == CUI_PLOT_SANKEY_NO_ERROR);
    assert(nodes[0].depth == 0 && nodes[1].depth == 1 && nodes[2].depth == 2);
    assert(nodes[0].height == 2 && nodes[2].height == 0 && nodes[1].value == 5);
    assert(nodes[0].x0 == 10 && nodes[1].x0 == 55 && nodes[2].x0 == 100);
    assert(nodes[0].source_count == 1 && nodes[1].target_count == 1);
    assert(nodes[1].y0 == 0 && links[0].width == 0 && cui_plot_sankey_layer_count(&graph) == 3);
    input[0].value = 999; /* topology owns copied values */
    assert(cui_plot_sankey_layout_from(&graph, &o) == CUI_PLOT_OK);
    assert(nodes[0].y0 == 20 && nodes[0].y1 == 120 && links[0].source_width == 100);
    assert(links[0].y0 == 70 && links[0].target_width == 100 && links[0].width == 100);
    size_t order[8], required;
    assert(cui_plot_sankey_node_links(&graph, 1, false, order, 8, &required) == CUI_PLOT_OK &&
           required == 1 && order[0] == 0);
    o.extent = (cui_plot_rect){0, 0, 200, 200};
    o.align = CUI_PLOT_SANKEY_RIGHT;
    assert(cui_plot_sankey_layout_from(&graph, &o) == CUI_PLOT_OK);
    assert(nodes[2].x0 == 190 && nodes[0].value == 5 && nodes[0].y1 == 200);
    memcpy(old_nodes, nodes, sizeof nodes);
    memcpy(old_links, links, sizeof links);
    cui_plot_sankey_graph before = graph;
    cui_plot_sankey_link cycle[] = {{0, 1, 1}, {1, 0, 1}};
    assert(cui_plot_sankey_layout(&graph, 2, cycle, 2, &o, &error) == CUI_PLOT_INVALID);
    assert(error.kind == CUI_PLOT_SANKEY_CIRCULAR_LINK);
    assert(memcmp(nodes, old_nodes, sizeof nodes) == 0 &&
           memcmp(links, old_links, sizeof links) == 0 &&
           memcmp(&graph, &before, sizeof graph) == 0);
    cui_plot_sankey_link missing[] = {{0, 9, 1}};
    assert(cui_plot_sankey_topology(&graph, 3, missing, 1, &o, &error) == CUI_PLOT_INVALID &&
           error.kind == CUI_PLOT_SANKEY_MISSING_NODE && error.node == 9);
    assert(memcmp(nodes, old_nodes, sizeof nodes) == 0);
    assert(cui_plot_sankey_layout(&graph, 9, NULL, 0, &o, &error) == CUI_PLOT_CAPACITY);
    assert(memcmp(nodes, old_nodes, sizeof nodes) == 0);
    o.iterations = 0;
    o.node_padding = 0;
    o.value_scale = CUI_PLOT_SANKEY_SQRT;
    cui_plot_sankey_link split[] = {{0, 1, 9}, {0, 2, 16}, {1, 3, 4}, {2, 3, 25}};
    assert(cui_plot_sankey_layout(&graph, 4, split, 4, &o, &error) == CUI_PLOT_OK);
    assert(links[0].value == 3 && links[1].value == 4 && nodes[0].value == 7);
    assert(nodes[1].value == 3 && nodes[2].value == 5 && nodes[3].value == 7);
    assert(close(links[0].source_width + links[1].source_width, nodes[0].y1 - nodes[0].y0));
    assert(close(links[0].target_width, nodes[1].y1 - nodes[1].y0));
    assert(close(links[2].source_width, nodes[1].y1 - nodes[1].y0));
    assert(close(links[2].target_width + links[3].target_width, nodes[3].y1 - nodes[3].y0));
    double scaled = links[0].value;
    o.value_scale = CUI_PLOT_SANKEY_LINEAR;
    assert(cui_plot_sankey_layout_from(&graph, &o) == CUI_PLOT_OK && links[0].value == scaled);
    assert(cui_plot_sankey_node_links(&graph, 0, true, NULL, 0, &required) == CUI_PLOT_CAPACITY &&
           required == 2);
    assert(cui_plot_sankey_node_links(&graph, 0, true, order, 8, &required) == CUI_PLOT_OK);
    assert(order[0] == 0 && order[1] == 1);
    o.node_padding = 100;
    o.iterations = 6;
    assert(cui_plot_sankey_layout_from(&graph, &o) == CUI_PLOT_OK);
    assert(nodes[1].y1 <= nodes[2].y0 || nodes[2].y1 <= nodes[1].y0);
    for (size_t i = 0; i < 4; ++i)
        assert(nodes[i].y0 >= 0 && nodes[i].y1 <= 200);

    double nominal = links[0].width;
    nodes[0].y0 = 10;
    nodes[0].y1 = 110;
    assert(cui_plot_sankey_link_breadths(&graph) == CUI_PLOT_OK);
    assert(close(links[0].source_width + links[1].source_width, 100) && links[0].width == nominal);
    assert(close(links[0].y0, 10 + links[0].source_width / 2));
    memcpy(old_links, links, sizeof links);
    nodes[0].y1 = 0;
    assert(cui_plot_sankey_link_breadths(&graph) == CUI_PLOT_INVALID);
    assert(memcmp(old_links, links, sizeof links) == 0);
    nodes[0].y1 = 110;
    o.node_width = 0;
    assert(cui_plot_sankey_layout_from(&graph, &o) == CUI_PLOT_OK && nodes[0].x0 == nodes[0].x1);
    cui_plot_sankey_link zeros[] = {{0, 1, 0}};
    assert(cui_plot_sankey_layout(&graph, 3, zeros, 1, &o, &error) == CUI_PLOT_OK);
    assert(nodes[0].layer == 0 && nodes[1].layer == 1 && nodes[2].layer == 1);
    assert(links[0].source_width == 0 && links[0].target_width == 0);

    cui_plot_sankey_link branch[] = {{0, 1, 1}, {1, 2, 1}, {0, 3, 1}};
    const size_t expected[4][4] = {{0, 1, 2, 2}, {0, 1, 2, 1}, {0, 1, 2, 2}, {0, 1, 2, 1}};
    for (size_t align = 0; align < 4; ++align) {
        o.align = (cui_plot_sankey_align)align;
        assert(cui_plot_sankey_topology(&graph, 4, branch, 3, &o, &error) == CUI_PLOT_OK);
        for (size_t node = 0; node < 4; ++node)
            assert(nodes[node].layer == expected[align][node]);
    }
    o.align = CUI_PLOT_SANKEY_JUSTIFY;
    o.iterations = 0;
    o.node_padding = 0;
    o.node_width = 10;
    cui_plot_sankey_link crossing[] = {{0, 2, 1}, {0, 3, 9}, {1, 2, 9}, {1, 3, 1}};
    assert(cui_plot_sankey_layout(&graph, 4, crossing, 4, &o, &error) == CUI_PLOT_OK);
    double initial = nodes[0].y0;
    o.iterations = 6;
    assert(cui_plot_sankey_layout_from(&graph, &o) == CUI_PLOT_OK);
    assert(fabs(nodes[0].y0 - initial) > 1e-4 || fabs(nodes[1].y0 - 100) > 1e-4);
    memcpy(old_nodes, nodes, sizeof nodes);
    memcpy(old_links, links, sizeof links);
    cui_plot_sankey_link huge[] = {{0, 1, DBL_MAX}, {0, 2, DBL_MAX}};
    assert(cui_plot_sankey_layout(&graph, 3, huge, 2, &o, &error) == CUI_PLOT_OVERFLOW);
    assert(memcmp(nodes, old_nodes, sizeof nodes) == 0 &&
           memcmp(links, old_links, sizeof links) == 0);
    cui_plot_sankey_graph empty = {0};
    assert(cui_plot_sankey_layout(&empty, 0, NULL, 0, &o, &error) == CUI_PLOT_OK);
    assert(cui_plot_sankey_layer_count(&empty) == 0);
    o.extent.y_max = NAN;
    memcpy(old_nodes, nodes, sizeof nodes);
    assert(cui_plot_sankey_layout_from(&graph, &o) == CUI_PLOT_INVALID);
    assert(memcmp(nodes, old_nodes, sizeof nodes) == 0);
}
static void test_sankey_ribbon(void) {
    cui_plot_sankey_ribbon link = {10, 50, 20, 30, 4, 12};
    cui_plot_path_command path[5], old[5];
    memset(path, 0x5a, sizeof path);
    memcpy(old, path, sizeof path);
    size_t required;
    assert(cui_plot_sankey_link_path(&link, 8, (cui_plot_point){2, 3}, NULL, 0, &required) ==
               CUI_PLOT_CAPACITY &&
           required == 5);
    assert(cui_plot_sankey_link_path(&link, 8, (cui_plot_point){2, 3}, path, 4, &required) ==
           CUI_PLOT_CAPACITY);
    assert(memcmp(path, old, sizeof path) == 0);
    assert(cui_plot_sankey_link_path(&link, 8, (cui_plot_point){2, 3}, path, 5, &required) ==
           CUI_PLOT_OK);
    assert(path[0].point.x == 12 && path[0].point.y == 19);
    assert(path[1].point.x == 52 && path[1].point.y == 27);
    assert(path[1].control1.x == 32 && path[1].control1.y == 19);
    assert(path[1].control2.x == 32 && path[1].control2.y == 27);
    assert(path[2].point.x == 52 && path[2].point.y == 39);
    assert(path[3].point.x == 12 && path[3].point.y == 27);
    assert(path[3].control1.x == 32 && path[3].control1.y == 39);
    assert(path[3].control2.x == 32 && path[3].control2.y == 27);
    assert(path[4].kind == CUI_PLOT_PATH_CLOSE);
    link.source_x = 50;
    link.target_x = 10;
    assert(cui_plot_sankey_link_path(&link, 0, (cui_plot_point){0, 0}, path, 5, &required) ==
           CUI_PLOT_OK);
    assert(path[0].point.x == 50 && path[1].point.x == 10 && path[1].control1.x == 30);
    memcpy(old, path, sizeof path);
    required = 99;
    link.source_width = -1;
    assert(cui_plot_sankey_link_path(&link, 0, (cui_plot_point){0, 0}, path, 5, &required) ==
           CUI_PLOT_INVALID);
    assert(required == 99 && memcmp(path, old, sizeof path) == 0);
    link.source_width = 0;
    link.target_width = 0;
    assert(cui_plot_sankey_link_path(&link, 0, (cui_plot_point){0, 0}, path, 5, &required) ==
           CUI_PLOT_OK);
    assert(path[0].point.y == path[3].point.y && path[1].point.y == path[2].point.y);
    memcpy(old, path, sizeof path);
    required = 99;
    link.source_x = DBL_MAX;
    assert(cui_plot_sankey_link_path(&link, 0, (cui_plot_point){DBL_MAX, 0}, path, 5, &required) ==
           CUI_PLOT_OVERFLOW);
    assert(required == 99 && memcmp(path, old, sizeof path) == 0);
    assert(cui_plot_sankey_link_path(NULL, 0, (cui_plot_point){0, 0}, path, 5, &required) ==
           CUI_PLOT_INVALID);
}
static void test_tooltip_cursor(void) {
    cui_plot_rect bounds = {10, 20, 110, 120};
    cui_plot_cursor_memory memory = {0};
    cui_plot_cursor_update update;
    assert(cui_plot_cursor_mouse(&memory, bounds, (cui_plot_point){40, 50}, true, &update) ==
           CUI_PLOT_OK);
    assert(memory.visible && memory.relative.x == 30 && update.changed && update.request_frame);
    assert(cui_plot_cursor_mouse(&memory, bounds, (cui_plot_point){41, 50}, true, &update) ==
           CUI_PLOT_OK);
    assert(update.changed && !update.request_frame);
    assert(cui_plot_cursor_mouse(&memory, bounds, (cui_plot_point){41, 50}, true, &update) ==
           CUI_PLOT_OK);
    assert(!update.changed && !update.request_frame);
    cui_plot_point relative = {99, 99};
    bool resolved;
    bounds.x_min = 20;
    assert(cui_plot_cursor_resolve(&memory, bounds, (cui_plot_point){41, 50}, false, &relative,
                                   &resolved) == CUI_PLOT_OK);
    assert(resolved && relative.x == 21 && relative.y == 30);
    assert(cui_plot_cursor_resolve(&memory, bounds, (cui_plot_point){41, 50}, true, &relative,
                                   &resolved) == CUI_PLOT_OK);
    assert(!resolved && relative.x == 21);
    assert(cui_plot_cursor_mouse(&memory, bounds, (cui_plot_point){41, 50}, false, &update) ==
           CUI_PLOT_OK);
    assert(!memory.visible && update.request_frame);
    assert(cui_plot_cursor_long_press(&memory, bounds, (cui_plot_point){41, 50},
                                      CUI_PLOT_LONG_PRESS_MOVED, true, false,
                                      &update) == CUI_PLOT_OK);
    assert(!memory.visible && !update.changed);
    assert(cui_plot_cursor_long_press(&memory, bounds, (cui_plot_point){41, 50},
                                      CUI_PLOT_LONG_PRESS_STARTED, true, true,
                                      &update) == CUI_PLOT_OK);
    assert(!memory.visible && !update.prevent_default);
    assert(cui_plot_cursor_long_press(&memory, bounds, (cui_plot_point){41, 50},
                                      CUI_PLOT_LONG_PRESS_STARTED, true, false,
                                      &update) == CUI_PLOT_OK);
    assert(memory.visible && update.prevent_default && update.request_frame);
    assert(cui_plot_cursor_long_press(&memory, bounds, (cui_plot_point){200, 50},
                                      CUI_PLOT_LONG_PRESS_MOVED, false, true,
                                      &update) == CUI_PLOT_OK);
    assert(memory.visible && memory.relative.x == 180 && update.changed);
    assert(cui_plot_cursor_resolve(&memory, bounds, (cui_plot_point){200, 50}, false, &relative,
                                   &resolved) == CUI_PLOT_OK &&
           !resolved);
    assert(cui_plot_cursor_long_press(&memory, bounds, (cui_plot_point){200, 50},
                                      CUI_PLOT_LONG_PRESS_ENDED, false, false,
                                      &update) == CUI_PLOT_OK);
    assert(!memory.visible && update.changed);
    assert(cui_plot_cursor_long_press(&memory, bounds, (cui_plot_point){41, 50},
                                      CUI_PLOT_LONG_PRESS_STARTED, true, false,
                                      &update) == CUI_PLOT_OK);
    assert(cui_plot_cursor_long_press(&memory, bounds, (cui_plot_point){41, 50},
                                      CUI_PLOT_LONG_PRESS_CANCELLED, true, false,
                                      &update) == CUI_PLOT_OK &&
           !memory.visible);
    cui_plot_cursor_memory old = memory;
    cui_plot_cursor_update saved = update;
    assert(cui_plot_cursor_mouse(&memory, bounds, (cui_plot_point){NAN, 50}, true, &update) ==
           CUI_PLOT_INVALID);
    assert(memcmp(&old, &memory, sizeof old) == 0 && memcmp(&saved, &update, sizeof saved) == 0);
    assert(cui_plot_cursor_mouse(&memory, bounds, (cui_plot_point){110, 50}, true, &update) ==
               CUI_PLOT_OK &&
           !memory.visible);
    assert(cui_plot_cursor_mouse(&memory, bounds, (cui_plot_point){20, 20}, true, &update) ==
               CUI_PLOT_OK &&
           memory.visible);
    old = memory;
    saved = update;
    cui_plot_rect extreme = {-DBL_MAX, 0, DBL_MAX, 100};
    assert(cui_plot_cursor_mouse(&memory, extreme, (cui_plot_point){DBL_MAX / 2, 50}, true,
                                 &update) == CUI_PLOT_OVERFLOW);
    assert(memcmp(&old, &memory, sizeof old) == 0 && memcmp(&saved, &update, sizeof saved) == 0);
    relative = (cui_plot_point){99, 99};
    resolved = true;
    assert(cui_plot_cursor_resolve(&memory, bounds, (cui_plot_point){NAN, 0}, false, &relative,
                                   &resolved) == CUI_PLOT_INVALID);
    assert(relative.x == 99 && resolved);
    assert(cui_plot_cursor_long_press(&memory, bounds, (cui_plot_point){40, 50},
                                      (cui_plot_long_press_phase)99, true, false,
                                      &update) == CUI_PLOT_INVALID);
}
static void test_source_arc_padding(void) {
    const double pi = acos(-1.0);
    cui_plot_arc arc = {.start_angle = 0, .end_angle = 2 * pi, .pad_angle = .3};
    cui_plot_path_command path[32] = {{0}}, flat[32], old[32];
    size_t count, n;
    assert(cui_plot_arc_path_ex(&arc, 10, 20, CUI_PLOT_ARC_SOURCE_PADDING, path, 32, &count) ==
               CUI_PLOT_OK &&
           count == 5);
    assert(path[1].kind == CUI_PLOT_PATH_ARC && path[1].large_arc && path[1].clockwise);
    assert(path[3].kind == CUI_PLOT_PATH_ARC && path[3].large_arc && !path[3].clockwise);
    assert(close(path[0].point.x, 20 * sin(.00005)));
    assert(close(path[2].point.x, -10 * sin(.0001)));
    assert(cui_plot_arc_flatten_ex(&arc, 10, 20, 8, CUI_PLOT_ARC_SOURCE_PADDING, flat, 32, &n) ==
               CUI_PLOT_OK &&
           n == 19);
    assert(close(flat[0].point.x, path[0].point.x));
    assert(close(flat[8].point.x, path[1].point.x));
    assert(cui_plot_arc_path(&arc, 10, 20, path, 32, &count) == CUI_PLOT_OK && count == 7);
    assert(close(path[0].point.x, 0));
    arc.end_angle = pi;
    assert(cui_plot_arc_path_ex(&arc, 0, 20, CUI_PLOT_ARC_SOURCE_PADDING, path, 32, &count) ==
               CUI_PLOT_OK &&
           count == 4);
    assert(close(path[0].point.x, 20 * sin(.00005)) && !path[1].large_arc);
    arc.end_angle = pi / 2;
    assert(cui_plot_arc_path_ex(&arc, 0, 20, CUI_PLOT_ARC_SOURCE_PADDING, path, 32, &count) ==
           CUI_PLOT_OK);
    assert(close(path[0].point.x, 20 * sin(.15)));
    arc.pad_angle = 0;
    assert(cui_plot_arc_path_ex(&arc, 1e-13, 20, CUI_PLOT_ARC_SOURCE_PADDING, path, 32, &count) ==
               CUI_PLOT_OK &&
           count == 4);
    assert(path[2].point.x == 0 && path[2].point.y == 0);
    memcpy(old, path, sizeof path);
    count = 99;
    assert(cui_plot_arc_path_ex(&arc, 0, 20, (cui_plot_arc_policy)99, path, 32, &count) ==
               CUI_PLOT_INVALID &&
           count == 99);
    assert(memcmp(path, old, sizeof path) == 0);
    assert(cui_plot_arc_path_ex(&arc, 0, 20, CUI_PLOT_ARC_SOURCE_PADDING, path, 1, &count) ==
               CUI_PLOT_CAPACITY &&
           count == 4);
    assert(memcmp(path, old, sizeof path) == 0);
    arc.end_angle = 1e-13;
    assert(cui_plot_arc_path_ex(&arc, 0, 20, CUI_PLOT_ARC_SOURCE_PADDING, path, 32, &count) ==
               CUI_PLOT_OK &&
           count == 0);
    arc.end_angle = pi / 2;
    assert(cui_plot_arc_flatten_ex(&arc, 0, 1e-13, 8, CUI_PLOT_ARC_SOURCE_PADDING, path, 32,
                                   &count) == CUI_PLOT_OK &&
           count == 0);
}
static void test_rounded_bar_path(void) {
    double radii[] = {4, 0, 2, 99};
    cui_plot_path_command path[10], old[10];
    memset(path, 0x5a, sizeof path);
    memcpy(old, path, sizeof path);
    size_t required = 99;
    assert(cui_plot_bar_path(CUI_PLOT_BAR_BOTTOM, 2, 8, 20, 4, radii, NULL, 0, &required) ==
               CUI_PLOT_CAPACITY &&
           required == 10);
    assert(cui_plot_bar_path(CUI_PLOT_BAR_BOTTOM, 2, 8, 20, 4, radii, path, 9, &required) ==
           CUI_PLOT_CAPACITY);
    assert(memcmp(path, old, sizeof path) == 0);
    assert(cui_plot_bar_path(CUI_PLOT_BAR_BOTTOM, 2, 8, 20, 4, radii, path, 10, &required) ==
           CUI_PLOT_OK);
    assert(path[0].point.x == 6 && path[0].point.y == 4);
    assert(path[2].point.x == 10 && path[2].point.y == 4);
    assert(path[4].point.x == 8 && path[4].point.y == 20);
    assert(path[6].point.x == 2 && path[6].point.y == 16);
    assert(path[8].point.x == 6 && path[8].point.y == 4);
    assert(path[9].kind == CUI_PLOT_PATH_CLOSE);
    memcpy(old, path, sizeof path);
    radii[3] = -1;
    required = 99;
    assert(cui_plot_bar_path(CUI_PLOT_BAR_BOTTOM, 2, 8, 20, 4, radii, path, 10, &required) ==
               CUI_PLOT_INVALID &&
           required == 99);
    assert(memcmp(path, old, sizeof path) == 0);
    radii[3] = 4;
    assert(cui_plot_bar_path(CUI_PLOT_BAR_LEFT, 2, 8, 20, 4, radii, path, 10, &required) ==
           CUI_PLOT_OK);
    assert(path[0].point.x == 8 && path[0].point.y == 2);
    assert(cui_plot_bar_path(CUI_PLOT_BAR_RIGHT, 2, 0, 20, 4, radii, path, 10, &required) ==
               CUI_PLOT_EMPTY &&
           required == 0);
    assert(cui_plot_bar_path(CUI_PLOT_BAR_TOP, 2, 8, 4, 4, radii, path, 10, &required) ==
           CUI_PLOT_EMPTY);
    assert(cui_plot_bar_path(CUI_PLOT_BAR_TOP, NAN, 8, 4, 4, radii, path, 10, &required) ==
           CUI_PLOT_INVALID);
}
static void test_linear_extent(void) {
    const double domain[] = {3, 1, 2, 1};
    cui_plot_linear_extent s;
    double pixel = -99;
    assert(cui_plot_linear_extent_init(&s, domain, 4, 100, 0) == CUI_PLOT_OK);
    assert(s.domain_min == 1 && s.domain_max == 3);
    assert(cui_plot_linear_extent_tick(&s, 2, &pixel) == CUI_PLOT_OK && close(pixel, 50));
    assert(cui_plot_linear_extent_tick(&s, 4, &pixel) == CUI_PLOT_OK && close(pixel, -50));
    assert(cui_plot_linear_extent_init(&s, domain, 4, 7, 7) == CUI_PLOT_OK);
    assert(cui_plot_linear_extent_tick(&s, DBL_MAX, &pixel) == CUI_PLOT_OK && pixel == 7);
    cui_plot_scale strict = {CUI_PLOT_LINEAR, 1, 3, 7, 7, false};
    assert(cui_plot_validate(&strict) == CUI_PLOT_DEGENERATE);
    assert(cui_plot_linear_extent_init(&s, NULL, 0, 0, 100) == CUI_PLOT_OK);
    assert(cui_plot_linear_extent_tick(&s, 1, &pixel) == CUI_PLOT_EMPTY && pixel == 7);
    assert(cui_plot_linear_extent_init(&s, domain, 1, 0, 0) == CUI_PLOT_OK);
    assert(cui_plot_linear_extent_tick(&s, 3, &pixel) == CUI_PLOT_EMPTY && pixel == 7);
    cui_plot_linear_extent old = s;
    double bad[] = {1, NAN};
    assert(cui_plot_linear_extent_init(&s, bad, 2, 0, 1) == CUI_PLOT_INVALID);
    assert(memcmp(&s, &old, sizeof s) == 0);
    assert(cui_plot_linear_extent_init(&s, NULL, 1, 0, 1) == CUI_PLOT_INVALID);
    assert(cui_plot_linear_extent_init(&s, domain, 4097, 0, 1) == CUI_PLOT_INVALID);
    assert(cui_plot_linear_extent_tick(&s, NAN, &pixel) == CUI_PLOT_INVALID && pixel == 7);
    double extreme[] = {-DBL_MAX, DBL_MAX};
    assert(cui_plot_linear_extent_init(&s, extreme, 2, -DBL_MAX, DBL_MAX) == CUI_PLOT_OK);
    assert(cui_plot_linear_extent_tick(&s, 0, &pixel) == CUI_PLOT_OK && pixel == 0);
    double tiny[] = {0, 1};
    assert(cui_plot_linear_extent_init(&s, tiny, 2, 0, DBL_MAX) == CUI_PLOT_OK);
    pixel = 7;
    assert(cui_plot_linear_extent_tick(&s, 2, &pixel) == CUI_PLOT_OVERFLOW && pixel == 7);
    assert(cui_plot_linear_extent_tick(NULL, 1, &pixel) == CUI_PLOT_INVALID);
    assert(cui_plot_linear_extent_tick(&s, 1, NULL) == CUI_PLOT_INVALID);
}
static void test_lifecycle(void) {
    cui_plot_hover_memory memory = {0};
    cui_plot_point dots[] = {{2, 3}}, cursor = {4, 5};
    cui_plot_tooltip_state state = {7, {8, 9}, dots, 1};
    assert(cui_plot_hover_track(&memory, &state, &cursor, 0) == CUI_PLOT_OK);
    assert(memory.entering && memory.hovered && memory.has_state);
    assert(cui_plot_hover_track(&memory, NULL, NULL, .4) == CUI_PLOT_OK);
    assert(!memory.entering && !memory.hovered && memory.has_state);
    assert(memory.state.index == 7 && memory.state.dots == dots && memory.cursor.x == 4);
    cui_plot_hover_memory before = memory;
    assert(cui_plot_hover_track(&memory, &state, NULL, .5) == CUI_PLOT_INVALID);
    assert(memcmp(&memory, &before, sizeof memory) == 0);
    dots[0].x = NAN;
    assert(cui_plot_hover_track(&memory, &state, &cursor, .5) == CUI_PLOT_INVALID);
    assert(memcmp(&memory, &before, sizeof memory) == 0);
    dots[0].x = 2;
    state.index = 8;
    assert(cui_plot_hover_track(&memory, &state, &cursor, .4) == CUI_PLOT_OK);
    assert(memory.hovered && !memory.entering && memory.state.index == 8);
    assert(cui_plot_hover_track(&memory, NULL, NULL, 0) == CUI_PLOT_OK);
    assert(!memory.has_state && memory.state.dots == NULL);
    cui_plot_appear_scope scope = {0};
    assert(!cui_plot_appear_complete(&scope, 12, 1));
    assert(cui_plot_appear_record(&scope, 12, 1, .5) == CUI_PLOT_OK);
    assert(!cui_plot_appear_complete(&scope, 12, 1));
    assert(cui_plot_appear_record(&scope, 12, 1, 1) == CUI_PLOT_OK);
    assert(cui_plot_appear_complete(&scope, 12, 1));
    assert(!cui_plot_appear_complete(&scope, 12, 2));
    assert(cui_plot_appear_complete(&scope, 12, 1));
    cui_plot_appear_scope old = scope;
    assert(cui_plot_appear_record(&scope, 99, 2, NAN) == CUI_PLOT_INVALID);
    assert(memcmp(&old, &scope, sizeof scope) == 0);
    assert(!cui_plot_appear_complete(&scope, 99, 1));
    assert(!scope.has_completed);
    double local = -1;
    assert(cui_plot_appear_stagger(.5, 1, 3, .5, &local) == CUI_PLOT_OK);
    assert(local == .5);
    assert(cui_plot_appear_stagger(.5, 99, 3, .5, &local) == CUI_PLOT_OK && local == 0);
    assert(cui_plot_appear_stagger(1, 99, 3, .95, &local) == CUI_PLOT_OK && local == 1);
    assert(cui_plot_appear_stagger(.25, 0, 0, 3, &local) == CUI_PLOT_OK && local == .25);
    assert(cui_plot_appear_stagger(.25, 0, 3, -2, &local) == CUI_PLOT_OK && local == .25);
    assert(cui_plot_appear_stagger(.5, 0, 3, NAN, &local) == CUI_PLOT_INVALID && local == .25);
}
int main(void) {
    test_sankey_large_chain();
    test_sankey_graph();
    test_sankey_ribbon();
    test_tooltip_cursor();
    test_source_arc_padding();
    test_rounded_bar_path();
    test_linear_extent();
    test_lifecycle();
    shape_geometry();
    pie_geometry();
    arc_and_radial_paths();
    bar_and_grid_geometry();
    source_area_paths();
    dot_geometry();
    band_scales();
    point_scales();
    ordinal_scales();
    cui_plot_scale linear = {CUI_PLOT_LINEAR, 0, 100, 0, 500, false};
    double pixel, value;
    assert(cui_plot_map(&linear, 25, &pixel) == CUI_PLOT_OK && pixel == 125);
    assert(cui_plot_inverse(&linear, 125, &value) == CUI_PLOT_OK && value == 25);
    linear.range_start = 500;
    linear.range_end = 0;
    assert(cui_plot_map(&linear, 25, &pixel) == CUI_PLOT_OK && pixel == 375);
    assert(cui_plot_pan(&linear, 50) == CUI_PLOT_OK && close(linear.domain_min, 10));
    assert(cui_plot_map(&linear, 25, &pixel) == CUI_PLOT_OK && close(pixel, 425));
    linear.clamp = true;
    assert(cui_plot_map(&linear, -100, &pixel) == CUI_PLOT_OK && pixel == 500);
    assert(cui_plot_inverse(&linear, -100, &value) == CUI_PLOT_OK && value == 110);
    cui_plot_scale log = {CUI_PLOT_LOG10, 1, 1000, 0, 300, false};
    assert(cui_plot_map(&log, 10, &pixel) == CUI_PLOT_OK && close(pixel, 100));
    assert(cui_plot_inverse(&log, 200, &value) == CUI_PLOT_OK && close(value, 100));
    assert(cui_plot_pan(&log, 100) == CUI_PLOT_OK && relative(log.domain_min, .1) &&
           relative(log.domain_max, 100));
    double ticks[1024];
    size_t count;
    cui_plot_scale nice = {CUI_PLOT_LINEAR, 3, 97, 0, 1, false};
    assert(cui_plot_ticks(&nice, 6, NULL, 0, &count) == CUI_PLOT_CAPACITY && count == 4);
    ticks[0] = 1234;
    assert(cui_plot_ticks(&nice, 6, ticks, 1, &count) == CUI_PLOT_CAPACITY && ticks[0] == 1234);
    assert(cui_plot_ticks(&nice, 6, ticks, 1024, &count) == CUI_PLOT_OK && count == 4);
    assert(ticks[0] == 20 && ticks[3] == 80);
    assert(cui_plot_nice(&nice, 6) == CUI_PLOT_OK && nice.domain_min == 0 &&
           nice.domain_max == 100);
    log = (cui_plot_scale){CUI_PLOT_LOG10, 1, 1000, 0, 300, false};
    assert(cui_plot_ticks(&log, 4, ticks, 1024, &count) == CUI_PLOT_OK && count == 4);
    assert(ticks[0] == 1 && ticks[1] == 10 && ticks[2] == 100 && ticks[3] == 1000);
    /* Endpoint transforms may land on either side of an integer exponent;
       exact decades remain inclusive, but genuinely excluded powers do not. */
    const double decade_domains[][2] = {{10, 1000}, {.1, 100}, {.01, 10}, {1e-300, 1e300}};
    const size_t decade_counts[] = {3, 4, 4, 7};
    for (size_t i = 0; i < sizeof(decade_counts) / sizeof(decade_counts[0]); i++) {
        cui_plot_scale boundary = {CUI_PLOT_LOG10, decade_domains[i][0], decade_domains[i][1], 0, 1,
                                   false};
        assert(cui_plot_ticks(&boundary, decade_counts[i], ticks, 1024, &count) == CUI_PLOT_OK &&
               count == decade_counts[i]);
        assert(ticks[0] == boundary.domain_min && ticks[count - 1] == boundary.domain_max);
    }
    cui_plot_scale excluded = {CUI_PLOT_LOG10, nextafter(10, INFINITY), nextafter(1000, 0), 0, 1,
                               false};
    assert(cui_plot_ticks(&excluded, 3, ticks, 1024, &count) == CUI_PLOT_OK && count == 1 &&
           ticks[0] == 100);
    excluded = (cui_plot_scale){CUI_PLOT_LOG10, 10.000001, 999.999999, 0, 1, false};
    assert(cui_plot_ticks(&excluded, 3, ticks, 1024, &count) == CUI_PLOT_OK && count == 1 &&
           ticks[0] == 100);
    cui_plot_scale decimal = {CUI_PLOT_LINEAR, .1, .3, 0, 1, false};
    assert(cui_plot_ticks(&decimal, 3, ticks, 1024, &count) == CUI_PLOT_OK && count > 0);
    for (size_t i = 0; i < count; i++)
        assert(ticks[i] >= decimal.domain_min && ticks[i] <= decimal.domain_max);
    /* Deterministic numeric properties over varying scale direction and viewport.
     */
    for (unsigned i = 0; i < 1500; ++i) {
        double a = random_unit() * 2000 - 1000;
        double span = 1 + random_unit() * 10000;
        cui_plot_scale s = {i % 2 ? CUI_PLOT_LOG10 : CUI_PLOT_LINEAR,
                            i % 2 ? pow(10, -8 + random_unit() * 8) : a,
                            i % 2 ? pow(10, 1 + random_unit() * 8) : a + span,
                            i % 3 ? 40 : 900,
                            i % 3 ? 900 : 40,
                            false};
        double t = .05 + random_unit() * .9;
        double anchor = s.range_start * (1 - t) + s.range_end * t;
        assert(cui_plot_inverse(&s, anchor, &value) == CUI_PLOT_OK);
        assert(cui_plot_map(&s, value, &pixel) == CUI_PLOT_OK && close(pixel, anchor));
        assert(cui_plot_zoom(&s, .5 + random_unit() * 3, anchor) == CUI_PLOT_OK);
        assert(cui_plot_map(&s, value, &pixel) == CUI_PLOT_OK && close(pixel, anchor));
        assert(cui_plot_ticks(&s, 10, ticks, 1024, &count) == CUI_PLOT_OK);
        for (size_t j = 0; j < count; ++j) {
            assert(isfinite(ticks[j]) && ticks[j] >= s.domain_min && ticks[j] <= s.domain_max);
            if (j)
                assert(ticks[j] > ticks[j - 1]);
        }
    }
    /* All finite IEEE double domain magnitudes, including overflow-width spans.
     */
    cui_plot_scale huge = {CUI_PLOT_LINEAR, -DBL_MAX, DBL_MAX, -DBL_MAX, DBL_MAX, false};
    assert(cui_plot_map(&huge, 0, &pixel) == CUI_PLOT_OK && pixel == 0);
    assert(cui_plot_inverse(&huge, 0, &value) == CUI_PLOT_OK && value == 0);
    assert(cui_plot_ticks(&huge, 8, ticks, 1024, &count) == CUI_PLOT_OK && count > 0);
    assert(cui_plot_nice(&huge, 8) == CUI_PLOT_OK && isfinite(huge.domain_min) &&
           isfinite(huge.domain_max));
    huge = (cui_plot_scale){CUI_PLOT_LINEAR, 0, 10, -DBL_MAX, DBL_MAX, false};
    assert(cui_plot_pan(&huge, DBL_MAX) == CUI_PLOT_OK && close(huge.domain_min, -5) &&
           close(huge.domain_max, 5));
    cui_plot_scale tiny = {CUI_PLOT_LINEAR, DBL_TRUE_MIN, 2 * DBL_TRUE_MIN, 0, 100, false};
    assert(cui_plot_map(&tiny, DBL_TRUE_MIN, &pixel) == CUI_PLOT_OK && pixel == 0);
    assert(cui_plot_inverse(&tiny, 100, &value) == CUI_PLOT_OK && value == 2 * DBL_TRUE_MIN);
    assert(cui_plot_ticks(&tiny, 10, ticks, 1024, &count) == CUI_PLOT_OK && count > 0);
    assert(cui_plot_zoom(&tiny, DBL_TRUE_MIN, 0) == CUI_PLOT_OK &&
           tiny.domain_min == DBL_TRUE_MIN && tiny.domain_max == 1);
    log = (cui_plot_scale){CUI_PLOT_LOG10, DBL_TRUE_MIN, DBL_MAX, 0, 100, false};
    assert(cui_plot_map(&log, 1e-200, &pixel) == CUI_PLOT_OK);
    assert(cui_plot_inverse(&log, pixel, &value) == CUI_PLOT_OK && relative(value, 1e-200));
    assert(cui_plot_ticks(&log, 10, ticks, 1024, &count) == CUI_PLOT_OK && count > 0);
    /* Failure is explicit and preserves outputs/mutable scales. */
    cui_plot_scale invalid = {CUI_PLOT_LINEAR, 2, 1, 0, 100, false};
    pixel = 55;
    assert(cui_plot_map(&invalid, 1, &pixel) == CUI_PLOT_INVALID && pixel == 55);
    invalid.domain_max = 2;
    assert(cui_plot_validate(&invalid) == CUI_PLOT_DEGENERATE);
    invalid.domain_max = NAN;
    assert(cui_plot_validate(&invalid) == CUI_PLOT_INVALID);
    invalid = (cui_plot_scale){CUI_PLOT_LOG10, 0, 1, 0, 100, false};
    assert(cui_plot_validate(&invalid) == CUI_PLOT_INVALID);
    invalid = (cui_plot_scale){CUI_PLOT_LINEAR, 1, 2, 0, 100, false};
    cui_plot_scale original = invalid;
    assert(cui_plot_zoom(&invalid, 0, 30) == CUI_PLOT_INVALID);
    assert(cui_plot_zoom(&invalid, DBL_TRUE_MIN, 30) == CUI_PLOT_OVERFLOW);
    assert(invalid.domain_min == original.domain_min && invalid.domain_max == original.domain_max);
    assert(cui_plot_zoom(&invalid, DBL_MAX, 30) == CUI_PLOT_DEGENERATE);
    assert(cui_plot_ticks(&invalid, 1001, ticks, 1024, &count) == CUI_PLOT_INVALID);
    assert(cui_plot_map(&log, -1, &pixel) == CUI_PLOT_INVALID);
    cui_plot_rect viewport = {0, 0, 100, 100};
    cui_plot_line line;
    assert(cui_plot_clip_line((cui_plot_line){{-50, -50}, {150, 150}}, viewport, &line) ==
           CUI_PLOT_OK);
    assert(close(line.start.x, 0) && close(line.start.y, 0) && close(line.end.x, 100) &&
           close(line.end.y, 100));
    assert(cui_plot_clip_line((cui_plot_line){{-50, 10}, {-10, 90}}, viewport, &line) ==
           CUI_PLOT_EMPTY);
    assert(cui_plot_clip_line((cui_plot_line){{-DBL_MAX, 50}, {DBL_MAX, 50}}, viewport, &line) ==
           CUI_PLOT_OK);
    assert(line.start.x == 0 && line.end.x == 100 && line.start.y == 50 && line.end.y == 50);
    cui_plot_scale xs = {CUI_PLOT_LINEAR, 0, 10, 0, 100, false};
    cui_plot_scale ys = {CUI_PLOT_LINEAR, 0, 100, 100, 0, false};
    assert(cui_plot_project_line(&xs, &ys, (cui_plot_point){-5, 50}, (cui_plot_point){15, 50},
                                 viewport, &line) == CUI_PLOT_OK);
    assert(line.start.x == 0 && line.end.x == 100 && line.start.y == 50);
    cui_plot_rect bar;
    assert(cui_plot_project_bar(&xs, &ys, 10, 20, 25, 0, viewport, &bar) == CUI_PLOT_OK);
    assert(bar.x_min == 90 && bar.x_max == 100 && bar.y_min == 75 && bar.y_max == 100);
    assert(cui_plot_project_bar(&xs, &ys, 5, 10, 0, 0, viewport, &bar) == CUI_PLOT_EMPTY);
    assert(cui_plot_project_bar(&xs, &ys, 5, -1, 25, 0, viewport, &bar) == CUI_PLOT_INVALID);
    return 0;
}
