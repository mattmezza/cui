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
    assert(cui_plot_point_at(&scale, 2, &pixel) == CUI_PLOT_OK && pixel == 80);
    assert(cui_plot_point_at(&scale, 3, &pixel) == CUI_PLOT_OK && pixel == 100);
    assert(cui_plot_point_nearest(&scale, 50, &index) == CUI_PLOT_OK && index == 1);
    assert(cui_plot_point_nearest(&scale, 30, &index) == CUI_PLOT_OK && index == 0);
    assert(cui_plot_point_nearest(&scale, 110, &index) == CUI_PLOT_OK && index == 3);
    assert(cui_plot_point_init(&scale, keys, 4, 100, 40) == CUI_PLOT_OK);
    assert(cui_plot_point_at(&scale, 2, &pixel) == CUI_PLOT_OK && pixel == 60);
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
int main(void) {
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
