/* SPDX-License-Identifier: MIT */
#include "cui/cui_plot.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
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
int main(void) {
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
    /* Deterministic numeric properties over varying scale direction and viewport. */
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
    /* All finite IEEE double domain magnitudes, including overflow-width spans. */
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
