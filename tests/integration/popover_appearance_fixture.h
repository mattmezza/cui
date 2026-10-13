/* SPDX-License-Identifier: MIT */
#ifndef CUI_POPOVER_APPEARANCE_FIXTURE_H
#define CUI_POPOVER_APPEARANCE_FIXTURE_H
#include "cui/cui_surface.h"
#include <assert.h>
#include <string.h>
/* Copy actual styled command payloads; omit TEXT identically in each consumer
 * so this geometry/alpha test needs no platform-specific font installation. */
static cui_draw_list popover_fixture(unsigned dark, unsigned policy, bool translucent,
                                     cui_draw_command out[64]) {
    cui_context *ui = cui_create((cui_config){.width = 128, .height = 128});
    assert(ui);
    cui_surface_state state;
    cui_surface_init(&state, 100, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &state, 0);
    cui_theme theme = dark ? cui_theme_dark() : cui_theme_light();
    theme.popover = (cui_color){120, 80, 40, translucent ? 128 : 255};
    theme.text = dark ? (cui_color){230, 210, 190, 255} : (cui_color){30, 50, 70, 255};
    cui_surface_options options = {.viewport = {0, 0, 128, 128},
                                   .anchor = {36, 10, 16, 10},
                                   .width = 56,
                                   .height = 56,
                                   .hide_close = true,
                                   .popover_appearance = (cui_popover_appearance)policy};
    assert(cui_begin_frame(ui, 128, 128, .016f));
    assert(cui_surface_begin(ui, &theme, &state, (cui_str){0}, (cui_str){0}, options));
    cui_surface_end(ui);
    cui_draw_list original = cui_end_frame(ui);
    size_t n = 0, shadows = 0;
    for (size_t i = 0; i < original.count; ++i) {
        if (original.commands[i].type == CUI_DRAW_TEXT)
            continue;
        assert(n < 64);
        out[n++] = original.commands[i];
        if (original.commands[i].type == CUI_DRAW_DROP_SHADOW)
            ++shadows;
    }
    assert(shadows == (policy == CUI_POPOVER_APPEARANCE_SOURCE ? 3u : 0u));
    cui_destroy(ui); /* copied shadow/geometry payloads survive frame/context */
    return (cui_draw_list){out, n, 128, 128};
}
#endif
