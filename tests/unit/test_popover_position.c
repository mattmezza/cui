/* SPDX-License-Identifier: MIT */
#include "cui/cui_surface.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>
static void rect(cui_rect a, cui_rect b) {
    assert(a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h);
}
int main(void) {
    cui_popover_position p = {.mode = CUI_POPOVER_POSITION_ANCHOR};
    cui_rect trigger = {100, 100, 40, 20}, viewport = {0, 0, 500, 500}, out;
    /* Independent goldens from frozen 87d10ae5 crates/base/src/popup.rs
     * anchor_position + Positioner Bounds::from_anchor_and_size, NOT the old
     * resolved_corner helper: trigger=(100,100,40,20), popup=(80,60), gap=4. */
    const cui_rect expected[8] = {{100, 124, 80, 60}, {80, 124, 80, 60}, {60, 124, 80, 60},
                                  {100, 36, 80, 60},  {80, 36, 80, 60},  {60, 36, 80, 60},
                                  {144, 80, 80, 60},  {16, 80, 80, 60}};
    for (unsigned a = 0; a < 8; ++a) {
        p.anchor = (cui_popover_anchor)a;
        assert(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 0, p, &out));
        rect(out, expected[a]);
        p.arrow = true;
        assert(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 0, p, &out));
        cui_rect arrow = expected[a];
        if (a < 3)
            arrow.y += 6;
        else if (a < 6)
            arrow.y -= 6;
        else if (a == 6)
            arrow.x += 6;
        else
            arrow.x -= 6;
        rect(out, arrow);
        cui_popover_arrow_geometry g;
        assert(cui_popover_arrow_resolve(out, trigger, p.anchor, 6, 4, &g));
        if (a < 6) {
            float target = a % 3 == 0 ? 100 : a % 3 == 1 ? 120 : 140;
            float center = fmaxf(out.x + 10, fminf(out.x + out.w - 10, target));
            assert(g.points[1].x == center);
            assert(g.points[1].y == (a < 3 ? out.y - 6 : out.y + out.h + 6));
            assert(g.join.y == (a < 3 ? out.y - 1 : out.y + out.h - 1));
            assert(g.join.w == 10 && g.join.h == 2);
            assert(g.side == (a < 3 ? CUI_POPUP_BELOW : CUI_POPUP_ABOVE));
        } else {
            assert(g.points[1].y == 110);
            assert(g.points[1].x == (a == 6 ? out.x - 6 : out.x + out.w + 6));
            assert(g.join.w == 2 && g.join.h == 10);
            assert(g.side == (a == 6 ? CUI_POPUP_RIGHT : CUI_POPUP_LEFT));
        }
        p.arrow = false;
        const float gaps[] = {-8, 0, 14};
        for (unsigned i = 0; i < 3; ++i) {
            p.custom_offset = true;
            p.offset = gaps[i];
            assert(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 0, p, &out));
            cui_rect custom = expected[a];
            if (a < 3)
                custom.y += gaps[i] - 4;
            else if (a < 6)
                custom.y -= gaps[i] - 4;
            else if (a == 6)
                custom.x += gaps[i] - 4;
            else
                custom.x -= gaps[i] - 4;
            rect(out, custom);
        }
        p.custom_offset = false;
    }
    p.anchor = CUI_POPOVER_TOP_LEFT;
    p.reference_rem = 32;
    p.arrow = true;
    assert(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 0, p, &out));
    assert(out.y == 140);
    p.custom_offset = true;
    p.offset = -12;
    assert(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 0, p, &out));
    assert(out.y == 120);
    p.offset = 0;
    p.arrow = false;
    assert(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 0, p, &out));
    assert(out.y == 120);
    /* Logical geometry retains fractions; source paint rounds displacement in
     * Positioner::paint. Host pixel snapping is deliberately outside this API. */
    p.offset = .5f;
    assert(cui_popover_position_resolve((cui_rect){100.25f, 100.25f, 40, 20}, (cui_size){80, 60},
                                        viewport, 0, p, &out));
    assert(out.x == 100.25f && out.y == 120.75f);
    p.offset = 0;
    /* No flip or size reduction, including nonzero viewport origin. */
    viewport = (cui_rect){10, 20, 100, 80};
    assert(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 4, p, &out));
    rect(out, (cui_rect){26, 36, 80, 60});
    assert(cui_popover_position_resolve(trigger, (cui_size){200, 150}, viewport, 4, p, &out));
    rect(out, (cui_rect){14, 24, 200, 150});
    assert(cui_popover_position_resolve((cui_rect){-100, -100, 0, 0}, (cui_size){20, 20}, viewport,
                                        4, p, &out));
    rect(out, (cui_rect){14, 24, 20, 20});
    /* Each viewport corner clamps independently, without side selection. */
    const cui_rect corners[4] = {
        {-100, -100, 0, 0}, {1000, -100, 0, 0}, {-100, 1000, 0, 0}, {1000, 1000, 0, 0}};
    for (unsigned i = 0; i < 4; ++i) {
        assert(cui_popover_position_resolve(corners[i], (cui_size){20, 20}, viewport, 4, p, &out));
        rect(out, (cui_rect){i % 2 ? 86 : 14, i / 2 ? 76 : 24, 20, 20});
    }
    /* Inactive explicit offset never participates in validation. */
    p.custom_offset = false;
    p.offset = NAN;
    assert(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 4, p, &out));
    cui_rect sentinel = {1, 2, 3, 4};
#define BAD(expr)                                                                                  \
    do {                                                                                           \
        out = sentinel;                                                                            \
        assert(!(expr));                                                                           \
        rect(out, sentinel);                                                                       \
    } while (0)
    p.mode = CUI_POPOVER_POSITION_LEGACY;
    BAD(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 4, p, &out));
    p.mode = CUI_POPOVER_POSITION_ANCHOR;
    p.anchor = (cui_popover_anchor)8;
    BAD(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 4, p, &out));
    p.anchor = CUI_POPOVER_TOP_LEFT;
    p.reference_rem = NAN;
    BAD(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 4, p, &out));
    p.reference_rem = -1;
    BAD(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 4, p, &out));
    p.reference_rem = 0;
    p.custom_offset = true;
    BAD(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 4, p, &out));
    p.offset = FLT_MAX;
    BAD(cui_popover_position_resolve((cui_rect){0, FLT_MAX, 0, 0}, (cui_size){1, 1}, viewport, 0, p,
                                     &out));
    p.custom_offset = false;
    BAD(cui_popover_position_resolve(trigger, (cui_size){0, 60}, viewport, 4, p, &out));
    BAD(cui_popover_position_resolve((cui_rect){0, 0, -1, 1}, (cui_size){80, 60}, viewport, 4, p,
                                     &out));
    BAD(cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, NAN, p, &out));
    BAD(cui_popover_position_resolve(trigger, (cui_size){80, 60}, (cui_rect){0, 0, 0, 80}, 4, p,
                                     &out));
    assert(!cui_popover_position_resolve(trigger, (cui_size){80, 60}, viewport, 4, p, NULL));
    /* Corner inset collapses tiny bases to midpoint without inverted triangles. */
    cui_popover_arrow_geometry g;
    assert(cui_popover_arrow_resolve((cui_rect){10, 20, 4, 2}, trigger, CUI_POPOVER_TOP_LEFT, 6, 50,
                                     &g));
    assert(g.points[0].x == 10 && g.points[1].x == 12 && g.points[2].x == 14);
    assert(g.join.x == 11 && g.join.w == 2);
    assert(cui_popover_arrow_resolve((cui_rect){10, 20, 4, 2}, trigger, CUI_POPOVER_TOP_LEFT, 0, 50,
                                     &g));
    assert(g.join.w == 0 && g.points[0].x == g.points[2].x);
    unsigned char saved[sizeof g];
    memcpy(saved, &g, sizeof g);
    assert(!cui_popover_arrow_resolve((cui_rect){10, 20, 4, 2}, trigger, CUI_POPOVER_TOP_LEFT, -1,
                                      0, &g));
    assert(memcmp(&g, saved, sizeof g) == 0);
    assert(!cui_popover_arrow_resolve((cui_rect){10, 20, 4, 2}, trigger, CUI_POPOVER_TOP_LEFT,
                                      INFINITY, 0, &g));
    assert(memcmp(&g, saved, sizeof g) == 0);
    /* Public helper addition leaves all existing surface placement untouched. */
    cui_surface_options legacy = {
        .viewport = {0, 0, 200, 100}, .anchor = {30, 90, 20, 10}, .width = 50, .height = 30};
    rect(cui_surface_geometry(CUI_SURFACE_POPOVER, legacy), (cui_rect){30, 54, 50, 30});
    return 0;
}
