/* SPDX-License-Identifier: MIT */
#include "cui/cui_surface.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static unsigned decisions;
static bool decide(void *user, cui_surface_result result) {
    (void)user;
    (void)result;
    ++decisions;
    return true;
}
static cui_draw_list scene(cui_context *ui, cui_surface_state *state, cui_theme *theme,
                           cui_surface_options options) {
    assert(cui_begin_frame(ui, 320, 240, .016f));
    if (cui_surface_begin(ui, theme, state, cui_string("Title"), (cui_str){0}, options)) {
        cui_text(ui, cui_string("Body"), 14, theme->text);
        cui_surface_end(ui);
    }
    return cui_end_frame(ui);
}
int main(void) {
    for (unsigned dark = 0; dark < 2; ++dark) {
        cui_theme t = dark ? cui_theme_dark() : cui_theme_light();
        t.popover = (cui_color){15, 35, 55, 255};
        t.popover_foreground = (cui_color){75, 95, 115, 255};
        t.text = (cui_color){135, 155, 175, 255};
        cui_popover_style style;
        assert(cui_popover_style_resolve(&t, CUI_POPOVER_APPEARANCE_SOURCE,
                                         (cui_popover_refinement){0}, &style));
        assert(style.padding == 12 && style.border_width == 0 && style.shadow_count == 3);
        assert(style.background.r == 15 && style.foreground.r == 75);
        assert(style.shadows[0].layer.spread == 1 && style.shadows[0].layer.sigma == 0);
        assert(style.shadows[0].layer.color.r == 135 && style.shadows[0].layer.color.a == 26);
        assert(style.shadows[1].layer.offset_y == 4 && style.shadows[1].layer.sigma == 3 &&
               style.shadows[1].layer.spread == -1);
        assert(style.shadows[2].layer.offset_y == 2 && style.shadows[2].layer.sigma == 2 &&
               style.shadows[2].layer.spread == -2);
        cui_popover_style none;
        assert(cui_popover_style_resolve(&t, CUI_POPOVER_APPEARANCE_NONE,
                                         (cui_popover_refinement){0}, &none));
        assert(none.padding == 0 && none.radius == 0 && none.border_width == 0 &&
               none.shadow_count == 0 && none.background.a == 0 && none.foreground.r == 135);
        cui_context *ui = cui_create((cui_config){.width = 320, .height = 240});
        assert(ui);
        cui_surface_state state;
        cui_surface_init(&state, 101, CUI_SURFACE_POPOVER);
        cui_surface_open(ui, &state, 0);
        cui_surface_options options = {.viewport = {0, 0, 320, 240},
                                       .anchor = {80, 40, 20, 20},
                                       .width = 160,
                                       .height = 100,
                                       .hide_close = true,
                                       .popover_appearance = CUI_POPOVER_APPEARANCE_SOURCE};
        cui_draw_list list = scene(ui, &state, &t, options);
        size_t shadows = 0;
        cui_rect caster = {0};
        for (size_t i = 0; i < list.count; ++i) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type == CUI_DRAW_DROP_SHADOW) {
                assert(shadows < 3);
                if (!shadows)
                    caster = c->shadow.caster;
                assert(c->shadow.caster.x == caster.x && c->shadow.caster.y == caster.y &&
                       c->shadow.caster.w == 160 && c->shadow.caster.h == 100);
                assert(c->shadow.layer.sigma == style.shadows[shadows].layer.sigma);
                assert(c->shadow.layer.spread == style.shadows[shadows].layer.spread);
                assert(c->shadow.radii.top_left == t.radius);
                ++shadows;
            }
        }
        assert(shadows == 3 && cui_last_error(ui) == CUI_OK);
        assert(cui_begin_frame(ui, 320, 240, .016f));
        cui_begin_box(
            ui, 999,
            (cui_box){.width = 320, .height = 240, .custom_opacity = true, .opacity = .5f});
        assert(cui_surface_begin(ui, &t, &state, (cui_str){0}, (cui_str){0}, options));
        cui_surface_end(ui);
        cui_end_box(ui);
        list = cui_end_frame(ui);
        shadows = 0;
        for (size_t i = 0; i < list.count; ++i)
            if (list.commands[i].type == CUI_DRAW_DROP_SHADOW) {
                assert(list.commands[i].shadow.layer.color.a == 13);
                ++shadows;
            }
        assert(shadows == 3);
        options.popover_appearance = CUI_POPOVER_APPEARANCE_LEGACY;
        list = scene(ui, &state, &t, options);
        for (size_t i = 0; i < list.count; ++i)
            assert(list.commands[i].type != CUI_DRAW_DROP_SHADOW);
        options.popover_appearance = CUI_POPOVER_APPEARANCE_NONE;
        options.popover_refinement = (cui_popover_refinement){
            .flags = CUI_POPOVER_REFINE_BACKGROUND | CUI_POPOVER_REFINE_SHADOWS,
            .background = {99, 88, 77, 255},
            .shadow_count = 1,
            .shadows = {
                {{.offset_y = 7, .sigma = 2, .color = {0, 0, 0, 44}}, CUI_BOX_SHADOW_DROP}}};
        list = scene(ui, &state, &t, options);
        shadows = 0;
        for (size_t i = 0; i < list.count; ++i)
            if (list.commands[i].type == CUI_DRAW_DROP_SHADOW) {
                assert(list.commands[i].shadow.layer.offset_y == 7);
                ++shadows;
            }
        assert(shadows == 1);
        cui_surface_request_close(ui, &state, CUI_SURFACE_CLOSED);
        cui_surface_state before;
        memcpy(&before, &state, sizeof(state));
        options.decide = decide;
        options.popover_appearance = (cui_popover_appearance)99;
        assert(cui_begin_frame(ui, 320, 240, .016f));
        assert(!cui_surface_begin(ui, &t, &state, (cui_str){0}, (cui_str){0}, options));
        assert(!cui_base_surface_update(ui, &state, options));
        assert(!memcmp(&before, &state, sizeof(state)) && decisions == 0);
        (void)cui_end_frame(ui);
        cui_popover_style before_style;
        memcpy(&before_style, &style, sizeof(style));
        assert(!cui_popover_style_resolve(&t, (cui_popover_appearance)99,
                                          (cui_popover_refinement){0}, &style));
        assert(!memcmp(&before_style, &style, sizeof(style)));
        cui_popover_refinement bad = {.flags = CUI_POPOVER_REFINE_PADDING, .padding = NAN};
        assert(!cui_popover_style_resolve(&t, CUI_POPOVER_APPEARANCE_SOURCE, bad, &style));
        bad = (cui_popover_refinement){.flags = CUI_POPOVER_REFINE_SHADOWS, .shadow_count = 9};
        assert(!cui_popover_refinement_valid(CUI_POPOVER_APPEARANCE_SOURCE, bad));
        bad.shadow_count = 1;
        bad.shadows[0].layer.sigma = -1;
        assert(!cui_popover_refinement_valid(CUI_POPOVER_APPEARANCE_SOURCE, bad));
        cui_destroy(ui);
    }
    return 0;
}
