/* SPDX-License-Identifier: MIT */
#ifdef NDEBUG
#undef NDEBUG
#endif
#define NANOSVG_IMPLEMENTATION
#include "../../third_party/nanosvg/nanosvg.h"
#include <assert.h>
#include <stdio.h>
static void equal(const float *a, const float *b) {
    for (unsigned i = 0; i < 6; ++i)
        assert(isfinite(a[i]) && fabsf(a[i] - b[i]) < .0001f);
}
static void malformed(void) {
    const char *cases[] = {"matrix()",
                           "matrix(1 2 3 4 5)",
                           "matrix(1 2 3 4 5 6 7)",
                           "matrix",
                           "matrix(1 0 0 1 0 0",
                           "translate()",
                           "translate(1 2 3)",
                           "translate",
                           "translate(2",
                           "scale()",
                           "scale(1 2 3)",
                           "scale",
                           "scale(2",
                           "rotate()",
                           "rotate(45 10)",
                           "rotate(45 10 20 30)",
                           "rotate",
                           "rotate(45",
                           "skewX()",
                           "skewX(1 2)",
                           "skewX",
                           "skewX(2",
                           "skewY()",
                           "skewY(1 2)",
                           "skewY",
                           "skewY(2",
                           "translate(1e999 2)",
                           "scale(1e999)",
                           "rotate(1e999)",
                           "skewX(1e999)",
                           "skewY(1e999)",
                           "matrix(1e999 0 0 1 0 0)"};
    float identity[6], out[6];
    nsvg__xformIdentity(identity);
    nsvg__parseTransform(out, "scale(2) matrix(1 2) translate(3 4)");
    equal(out, (float[]){2, 0, 0, 2, 6, 8});
    nsvg__parseTransform(out, "rotate(90 1) scale(2 3)");
    equal(out, (float[]){2, 0, 0, 3, 0, 0});
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        for (unsigned j = 0; j < 6; ++j)
            out[j] = 12345;
        nsvg__parseTransform(out, cases[i]);
        equal(out, identity);
        char combined[256];
        snprintf(combined, sizeof(combined), "translate(3 4) %s", cases[i]);
        float translation[6];
        nsvg__xformSetTranslation(translation, 3, 4);
        nsvg__parseTransform(out, combined);
        equal(out, translation);
    }
    typedef int (*helper)(float *, const char *);
    helper helpers[] = {nsvg__parseMatrix, nsvg__parseTranslate, nsvg__parseScale,
                        nsvg__parseRotate, nsvg__parseSkewX,     nsvg__parseSkewY};
    for (size_t i = 0; i < sizeof(helpers) / sizeof(helpers[0]); ++i) {
        for (unsigned j = 0; j < 6; ++j)
            out[j] = NAN;
        helpers[i](out, "()");
        equal(out, identity);
    }
}
static void valid(void) {
    float out[6], expected[6];
    nsvg__parseTransform(out, "matrix(1 2 3 4 5 6)");
    equal(out, (float[]){1, 2, 3, 4, 5, 6});
    nsvg__parseTransform(out, "translate(3)");
    equal(out, (float[]){1, 0, 0, 1, 3, 0});
    nsvg__parseTransform(out, "scale(2)");
    equal(out, (float[]){2, 0, 0, 2, 0, 0});
    nsvg__parseTransform(out, "rotate(90)");
    equal(out, (float[]){0, 1, -1, 0, 0, 0});
    nsvg__parseTransform(out, "rotate(90 10 20)");
    float x, y;
    nsvg__xformPoint(&x, &y, 10, 20, out);
    assert(fabsf(x - 10) < .0001f && fabsf(y - 20) < .0001f);
    nsvg__parseTransform(out, "skewX(45)");
    equal(out, (float[]){1, 0, 1, 1, 0, 0});
    nsvg__parseTransform(out, "skewY(45)");
    equal(out, (float[]){1, 1, 0, 1, 0, 0});
    nsvg__parseTransform(out, "translate(3,4) scale(2,3) rotate(90)");
    equal(out, (float[]){0, 3, -2, 0, 3, 4});
    nsvg__parseTransform(out, "translate(1e1,-2e1)");
    equal(out, (float[]){1, 0, 0, 1, 10, -20});
    nsvg__xformIdentity(expected);
    nsvg__parseTransform(out, "");
    equal(out, expected);
    char document[] = "<svg width=\"100\" height=\"100\"><path transform=\"translate(3 4) "
                      "scale(2)\" d=\"M0 0L10 0L10 10Z\"/></svg>";
    NSVGimage *image = nsvgParse(document, "px", 96);
    assert(image && image->shapes);
    for (unsigned i = 0; i < 4; ++i)
        assert(fabsf(image->shapes->bounds[i] - (float[]){3, 4, 23, 24}[i]) < .0001f);
    nsvgDelete(image);
}
int main(void) {
    malformed();
    valid();
    return 0;
}
