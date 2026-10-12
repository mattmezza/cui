/* SPDX-License-Identifier: MIT */
#include "cui/cui_chrome.h"
#include <assert.h>
#include <math.h>
static void near(float a, float b) {
    assert(fabsf(a - b) < .02f);
}
static unsigned calls;
static void control(cui_context *ui, const cui_theme *theme, cui_id id, void *user) {
    assert(user == &calls);
    near(theme->font_size, 12);
    calls++;
    cui_begin_box(ui, id ^ 999, (cui_box){.width = 30, .height = 25});
    cui_interact(ui, id ^ 999, CUI_ROLE_BUTTON, cui_string("Action"), false, false, 0);
    cui_end_box(ui);
}
int main(void) {
    assert(cui_status_bar_center_alignment(false, false) == CUI_ALIGN_START);
    assert(cui_status_bar_center_alignment(false, true) == CUI_ALIGN_START);
    assert(cui_status_bar_center_alignment(true, false) == CUI_ALIGN_END);
    assert(cui_status_bar_center_alignment(true, true) == CUI_ALIGN_CENTER);
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 200});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    cui_chrome_item left = {.id = 10, .text = {"Left", 4}},
                    center = {.id = 20, .text = {"Center", 6}},
                    right = {.id = 30, .text = {"Right", 5}};
    for (unsigned t = 0; t < 4; ++t)
        for (unsigned mask = 0; mask < 4; ++mask)
            for (unsigned resize = 0; resize < 2; ++resize) {
                float width = resize ? 500 : 400, density = themes[t].font_size / 16;
                bool l = (mask & 1) != 0, r = (mask & 2) != 0;
                assert(cui_begin_frame(ui, 600, 200, .016f));
                assert(cui_status_bar(ui, 1, &themes[t], (cui_chrome_region){&left, l},
                                      (cui_chrome_region){&center, 1},
                                      (cui_chrome_region){&right, r},
                                      (cui_status_bar_options){.width = width}));
                cui_end_frame(ui);
                cui_rect a = cui_bounds(ui, 10), b = cui_bounds(ui, 20), c = cui_bounds(ui, 30);
                float start = l ? a.x + a.w + 8 * density : 8 * density;
                float end = r ? c.x - 8 * density : width - 8 * density;
                if (l && r)
                    near(b.x + b.w / 2, (start + end) / 2);
                else if (l)
                    near(b.x + b.w, end);
                else
                    near(b.x, start);
                if (l)
                    near(a.x, 8 * density);
                if (r)
                    near(c.x + c.w, width - 8 * density);
                near(cui_bounds(ui, 1).h, 8 * density + 1 + 12 * density * 1.618034f);
                size_t n;
                const cui_semantic *s = cui_semantics(ui, &n);
                assert(n == 2u + l + r);
                for (size_t i = 0; i < n; ++i)
                    assert(s[i].tab_index == -1);
            }
    center.render = control;
    center.user = &calls;
    assert(cui_begin_frame(ui, 600, 200, .016f));
    assert(cui_status_bar(ui, 1, themes, (cui_chrome_region){0}, (cui_chrome_region){&center, 1},
                          (cui_chrome_region){0}, (cui_status_bar_options){.width = 300}));
    cui_end_frame(ui);
    assert(calls == 1);
    near(cui_bounds(ui, 1).h, 34);
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    assert(n == 2 && s[1].role == CUI_ROLE_BUTTON && s[1].tab_index == 0);
    assert(cui_begin_frame(ui, 600, 200, .016f));
    right.id = left.id;
    assert(!cui_status_bar(ui, 1, themes, (cui_chrome_region){&left, 1}, (cui_chrome_region){0},
                           (cui_chrome_region){&right, 1}, (cui_status_bar_options){0}));
    cui_end_frame(ui);
    cui_destroy(ui);
    return 0;
}
