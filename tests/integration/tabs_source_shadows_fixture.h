/* SPDX-License-Identifier: MIT */
#ifndef CUI_TABS_SOURCE_SHADOWS_FIXTURE_H
#define CUI_TABS_SOURCE_SHADOWS_FIXTURE_H
#include "cui/cui_choice.h"
#include <assert.h>
/* Actual component geometry/alpha fixture. TEXT is omitted equally for every
 * consumer; this does not assert installed-font pixels. */
static cui_draw_list tabs_shadow_fixture(unsigned dark, unsigned mode, bool translucent,
                                         cui_draw_command out[64], cui_rect *caster) {
    cui_context *ui = cui_create((cui_config){.width = 128, .height = 128});
    assert(ui);
    cui_theme theme = dark ? cui_theme_dark() : cui_theme_light();
    theme.background = (cui_color){120, 80, 40, translucent ? 128 : 255};
    theme.tab_bar_segmented = (cui_color){0};
    cui_choice_item items[] = {{10, {"A", 1}, mode >= 3}, {20, {"B", 1}, false}};
    cui_id selected = 10;
    cui_tab_animation animation = {0};
    cui_tabs_options options = {.variant = CUI_TAB_SEGMENTED,
                                .size = CUI_TAB_MEDIUM,
                                .density = 1,
                                .shadow_policy = mode == 0 ? CUI_TAB_SHADOW_SOURCE_TAB
                                                           : CUI_TAB_SHADOW_SOURCE_BAR,
                                .animation = mode == 0 ? NULL : &animation,
                                .reduced_motion = true};
    cui_draw_list original = {0};
    unsigned frames = mode == 2 || mode == 4 ? 2 : 1;
    for (unsigned frame = 0; frame < frames; ++frame) {
        assert(cui_begin_frame(ui, 128, 128, .016f));
        cui_begin_box(ui, 777, (cui_box){.width = 128, .height = 128, .padding = 24});
        (void)cui_tabs(ui, 101, &theme, items, 2, &selected, options, NULL, NULL);
        cui_end_box(ui);
        original = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    *caster = cui_bounds(ui, 10 ^ UINT64_C(0xacf7823ab98e17d0));
    size_t n = 0, shadows = 0;
    for (size_t i = 0; i < original.count; ++i) {
        if (original.commands[i].type == CUI_DRAW_TEXT)
            continue;
        assert(n < 64);
        out[n++] = original.commands[i];
        if (original.commands[i].type == CUI_DRAW_DROP_SHADOW) {
            if (!shadows)
                *caster = original.commands[i].shadow.caster;
            ++shadows;
        }
    }
    assert(shadows == (mode == 0 ? 1u : mode == 3 ? 0u : 2u));
    assert(selected == 10);
    cui_destroy(ui);
    return (cui_draw_list){out, n, 128, 128};
}
#endif
