/* SPDX-License-Identifier: MIT */
#if defined(CUI_MODEL_DISTRIBUTION)
#define CUI_SLIDER_MODEL_IMPLEMENTATION
#if defined(CUI_MODEL_UMBRELLA)
#include "cui_all.h"
#else
#include "cui_slider_model.h"
#endif
#else
#include "cui/cui_slider_model.h"
#endif
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static cui_slider_model_domain_v1 linear(double min, double max, double step) {
    return (cui_slider_model_domain_v1){min, max, step, CUI_SLIDER_MODEL_LINEAR, 0};
}
static void validity(void) {
    cui_slider_model_domain_v1 d = linear(0, 1, .125);
    cui_slider_model_value_v1 single = {CUI_SLIDER_MODEL_SINGLE, 0, NAN, .5};
    assert(cui_slider_model_value_valid_v1(d, single));
    cui_slider_model_value_v1 range = {CUI_SLIDER_MODEL_RANGE, 0, .25, .75};
    assert(cui_slider_model_value_valid_v1(d, range));
    range.start = .8;
    assert(!cui_slider_model_value_valid_v1(d, range));
    range.start = NAN;
    assert(!cui_slider_model_value_valid_v1(d, range));
    single.reserved = 1;
    assert(!cui_slider_model_value_valid_v1(d, single));
    single.reserved = 0;
    single.kind = 2;
    assert(!cui_slider_model_value_valid_v1(d, single));
    const cui_slider_model_domain_v1 bad[] = {
        {NAN, 1, 0, 0, 0}, {0, INFINITY, 0, 0, 0}, {1, 0, 0, 0, 0}, {0, 1, -1, 0, 0},
        {0, 1, NAN, 0, 0}, {0, 1, 0, 2, 0},        {0, 1, 0, 0, 1}, {-DBL_MAX, DBL_MAX, 0, 0, 0},
        {0, 1, 0, 1, 0},   {1, 1, 0, 1, 0}};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; ++i)
        assert(!cui_slider_model_domain_valid_v1(bad[i]));
    assert(cui_slider_model_domain_valid_v1(linear(-0., 0, 0)));
    assert(cui_slider_model_domain_valid_v1(linear(DBL_TRUE_MIN, DBL_MAX, DBL_TRUE_MIN)));
}
static void atomicity(void) {
    cui_slider_model_domain_v1 d = linear(0, 1, .125);
    cui_slider_model_value_v1 v = {0, 0, NAN, .25}, out, saved;
    memset(&out, 0xa5, sizeof out);
    memcpy(&saved, &out, sizeof out);
    assert(!cui_slider_model_value_constrain_v1(d, v, 0, .5, &out));
    assert(!memcmp(&out, &saved, sizeof out));
    assert(!cui_slider_model_value_constrain_v1(d, v, 2, .5, &out));
    assert(!cui_slider_model_value_constrain_v1(d, v, 1, NAN, &out));
    assert(!cui_slider_model_value_constrain_v1(d, v, 1, .5, NULL));
    assert(!memcmp(&out, &saved, sizeof out));
    double scalar = 123;
    assert(!cui_slider_model_to_fraction_v1(d, -1, &scalar));
    assert(!cui_slider_model_from_fraction_v1(d, 2, &scalar));
    assert(!cui_slider_model_from_fraction_v1(d, NAN, &scalar));
    assert(!cui_slider_model_from_fraction_v1(d, .5, NULL));
    assert(!cui_slider_model_to_fraction_v1(d, .5, NULL));
    assert(scalar == 123);
    cui_slider_model_domain_v1 bad = d;
    bad.reserved = 1;
    assert(!cui_slider_model_value_constrain_v1(bad, v, 1, .5, &out));
    assert(!memcmp(&out, &saved, sizeof out));
    assert(!cui_slider_model_to_fraction_v1(bad, .5, &scalar));
    assert(!cui_slider_model_from_fraction_v1(bad, .5, &scalar));
    assert(scalar == 123);
    cui_slider_model_state_v1 state, prior;
    memset(&state, 0xa5, sizeof state);
    memcpy(&prior, &state, sizeof prior);
    assert(!cui_slider_model_state_init_v1(sizeof state - 1, &state));
    assert(!cui_slider_model_state_init_v1(sizeof state + 1, &state));
    assert(!cui_slider_model_state_init_v1(sizeof state, NULL));
    assert(!memcmp(&state, &prior, sizeof state));
    assert(cui_slider_model_state_init_v1(sizeof state, &state));
    memset(&prior, 0, sizeof prior);
    prior.version = 1;
    prior.struct_size = sizeof prior;
    prior.active_thumb = CUI_SLIDER_MODEL_END;
    assert(!memcmp(&state, &prior, sizeof state));
    assert(cui_slider_model_value_valid_v1(linear(0, 0, 0), state.preview));
    cui_slider_model_state_v1 copy = state;
    memset(&state, 0xff, sizeof state);
    assert(copy.version == 1 && !copy.owner && !copy.flags && !copy.request_sequence);
}
static void snapping(void) {
    cui_slider_model_domain_v1 d = linear(-3, 7, 2);
    cui_slider_model_value_v1 v = {0, 0, NAN, 0};
    assert(cui_slider_model_value_constrain_v1(d, v, 1, -2, &v));
    assert(v.end == -1 && v.start == 0); /* min-origin upper midpoint */
    assert(cui_slider_model_value_constrain_v1(d, v, 1, 7, &v) && v.end == 7);
    d = linear(0, 1, .25);
    v = (cui_slider_model_value_v1){1, 0, .3, .7};
    assert(cui_slider_model_value_constrain_v1(d, v, 0, .9, &v) && v.start == .7);
    assert(cui_slider_model_value_constrain_v1(d, v, 1, .1, &v) && v.end == .7);
    d.step = DBL_TRUE_MIN; /* quotient overflows but finite proposal is valid */
    v = (cui_slider_model_value_v1){0, 0, NAN, .5};
    assert(cui_slider_model_value_constrain_v1(d, v, 1, .375, &v) && v.end == .375);
    d = linear(DBL_TRUE_MIN, 2, 1);
    v.end = d.minimum;
    assert(cui_slider_model_value_constrain_v1(d, v, 1, .5, &v));
    assert(v.end == DBL_TRUE_MIN); /* below midpoint by subnormal origin */
    d.minimum = -DBL_TRUE_MIN;
    assert(cui_slider_model_value_constrain_v1(d, v, 1, .5, &v) && v.end == 1);
    d = linear(-1, 1, .1);
    assert(cui_slider_model_value_constrain_v1(d, v, 1, 0, &v));
    assert(v.end > 0 && v.end < 1e-16); /* lattice phase survives cancellation */
    d = linear(0, DBL_TRUE_MIN * 9, DBL_TRUE_MIN * 3);
    v.end = DBL_TRUE_MIN;
    assert(cui_slider_model_value_constrain_v1(d, v, 1, DBL_TRUE_MIN * 2, &v));
    assert(v.end == DBL_TRUE_MIN * 3);
}
static void neighbors(void) {
    cui_slider_model_domain_v1 d = linear(-3, 7, .37);
    cui_slider_model_value_v1 current = {1, 0, -.123, 4.567};
    for (unsigned thumb = 0; thumb < 2; ++thumb)
        for (unsigned i = 0; i <= 128; ++i) {
            double requested = -5 + i / 8.;
            cui_slider_model_value_v1 out;
            assert(cui_slider_model_value_constrain_v1(d, current, thumb, requested, &out));
            assert(cui_slider_model_value_valid_v1(d, out));
            assert(thumb ? out.start == current.start : out.end == current.end);
        }
}
static void mappings(void) {
    const cui_slider_model_domain_v1 domains[] = {{0, 1, 0, 0, 0},
                                                  {-DBL_MAX / 2, DBL_MAX / 2, 0, 0, 0},
                                                  {DBL_TRUE_MIN, DBL_MAX, 0, 1, 0},
                                                  {1e308, 1.5e308, 0, 1, 0},
                                                  {1e308, 0, 0, 1, 0},
                                                  {DBL_TRUE_MIN, DBL_TRUE_MIN * 7, 0, 1, 0}};
    for (size_t k = 0; k < sizeof domains / sizeof *domains; ++k) {
        cui_slider_model_domain_v1 d = domains[k];
        if (k == 4)
            d.maximum = nextafter(d.minimum, INFINITY);
        double previous = d.minimum;
        for (unsigned i = 0; i <= 1024; ++i) {
            double fraction = i / 1024., value, mapped;
            assert(cui_slider_model_from_fraction_v1(d, fraction, &value));
            assert(isfinite(value) && value >= previous && value <= d.maximum);
            assert(cui_slider_model_to_fraction_v1(d, value, &mapped));
            assert(isfinite(mapped) && mapped >= 0 && mapped <= 1);
            previous = value;
        }
        double value;
        assert(cui_slider_model_from_fraction_v1(d, 0, &value) && value == d.minimum);
        assert(cui_slider_model_from_fraction_v1(d, 1, &value) && value == d.maximum);
    }
    double result;
    cui_slider_model_domain_v1 d = linear(-17, -17, 0);
    assert(cui_slider_model_to_fraction_v1(d, -17, &result) && result == 0);
    assert(cui_slider_model_from_fraction_v1(d, .5, &result) && result == -17);
}
static int probe(void) {
    unsigned op, scale, kind, thumb;
    double min, max, step, start, end, x;
    while (scanf("%u %la %la %la %u %u %la %la %u %la", &op, &min, &max, &step, &scale, &kind,
                 &start, &end, &thumb, &x) == 10) {
        cui_slider_model_domain_v1 d = {min, max, step, scale, 0};
        cui_slider_model_value_v1 v = {kind, 0, start, end}, out;
        double result = 0;
        bool ok;
        if (op == 1) {
            ok = cui_slider_model_value_constrain_v1(d, v, thumb, x, &out);
            printf("%u %a %a\n", ok, ok ? out.start : 0, ok ? out.end : 0);
        } else {
            ok = op == 2 ? cui_slider_model_to_fraction_v1(d, x, &result)
                         : cui_slider_model_from_fraction_v1(d, x, &result);
            printf("%u %a\n", ok, result);
        }
    }
    return 0;
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--probe"))
        return probe();
    validity();
    atomicity();
    snapping();
    neighbors();
    mappings();
    printf("state_v1=%zu value_v1=%zu domain_v1=%zu\n", sizeof(cui_slider_model_state_v1),
           sizeof(cui_slider_model_value_v1), sizeof(cui_slider_model_domain_v1));
    return 0;
}
