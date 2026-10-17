/* SPDX-License-Identifier: MIT */
#include "cui/cui_responsive.h"
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
int main(void) {
    const float thresholds[] = {640, 768, 1024, 1280, 1536};
    for (int i = 0; i < 5; ++i) {
        cui_breakpoint token = (cui_breakpoint)(i + 1);
        CHECK(!cui_responsive_at_least(cui_responsive_query(thresholds[i] - 1, NULL), token));
        CHECK(cui_responsive_at_least(cui_responsive_query(thresholds[i], NULL), token));
    }
    cui_responsive r = cui_responsive_query(532, NULL);
    CHECK(cui_responsive_fits(r, 260, 260, 12));
    CHECK(!cui_responsive_fits(cui_responsive_query(531, NULL), 260, 260, 12));
    CHECK(!cui_responsive_fits(r, NAN, 1, 0));
    CHECK(!cui_responsive_fits(r, -1, 1, 0));
    CHECK(!cui_responsive_fits(r, 1, -1, 0));
    CHECK(!cui_responsive_fits(cui_responsive_query(NAN, NULL), 0, 0, 0));
    CHECK(!cui_responsive_fits(cui_responsive_query(FLT_MAX, NULL), FLT_MAX, FLT_MAX, 0));
    CHECK(cui_responsive_fits(cui_responsive_query(0, NULL), 0, 0, 0));
    CHECK(!cui_responsive_fits(r, 1, 1, -1));
    CHECK(!cui_responsive_fits(r, INFINITY, 1, 0));
    CHECK(cui_responsive_value(r, CUI_BREAKPOINT_MD, 12, 24) == 12);
    cui_breakpoints custom = {200, 400, 600, 800, 1000};
    r = cui_responsive_query(600, &custom);
    CHECK(r.valid && r.breakpoint == CUI_BREAKPOINT_LG);
    CHECK(cui_responsive_value(r, CUI_BREAKPOINT_MD, 12, 24) == 24);
    custom.md = 100;
    CHECK(!cui_responsive_query(600, &custom).valid);
    custom = (cui_breakpoints){200, 400, 600, 800, NAN};
    CHECK(!cui_responsive_query(600, &custom).valid);
    custom.xxl = INFINITY;
    CHECK(!cui_responsive_query(600, &custom).valid);
    custom = (cui_breakpoints){-1, 400, 600, 800, 1000};
    CHECK(!cui_responsive_query(600, &custom).valid);
    custom = (cui_breakpoints){0, 0, 0, 0, 0};
    CHECK(cui_responsive_query(0, &custom).breakpoint == CUI_BREAKPOINT_2XL);
    CHECK(!cui_responsive_query(-1, NULL).valid);
    CHECK(!cui_responsive_query(NAN, NULL).valid);
    CHECK(!cui_responsive_query(INFINITY, NULL).valid);
    CHECK(!cui_responsive_at_least(cui_responsive_query(NAN, NULL), CUI_BREAKPOINT_BASE));
    CHECK(!cui_responsive_at_least(r, (cui_breakpoint)99));
    return 0;
}
