/* SPDX-License-Identifier: MIT */
/* Prepared source checkpoint: not executed until an explicit compiler lease. */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>

static unsigned calls;
static void content(cui_context *ui, void *user) {
    assert(user == &calls);
    ++calls;
    cui_begin_box(ui, 101, (cui_box){.width = 80, .height = 40});
    cui_end_box(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 200});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_icon_state state;
    cui_icon_state_init(&state);
    assert(cui_icon_set_data(&state,
                             cui_string("<svg width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
                                        "<path d=\"M1 1H14V5H5V14H1Z\"/></svg>")));
    cui_badge_options badge = cui_badge_defaults();
    badge.variant = CUI_BADGE_ICON;
    cui_icon_constraints constraints = {.max_width = 18, .custom_max_width = true};
    cui_box style = {.background = {17, 23, 31, 255}, .radius = 2};
    cui_badge_icon_content icon = {.state = &state,
                                   .options = {.size = CUI_ICON_CUSTOM,
                                               .custom_size = 23,
                                               .custom_color = true,
                                               .color = {211, 19, 73, 128},
                                               .custom_transform = true,
                                               .transform = {0, 1, -1, 0, 0, 0},
                                               .style = &style,
                                               .constraints = &constraints,
                                               .accessible_name = {"Must remain decorative", 22}}};
    const cui_id marker = cui_badge_indicator_id(100);
    const cui_id artwork = marker ^ UINT64_C(0x2c38f42991a75e63);
    assert(cui_begin_frame(ui, 400, 200, .016f));
    cui_badge_result result = cui_badge_with_icon(ui, 100, &theme, badge, &icon, content, &calls);
    assert(result.submitted && result.icon_error == CUI_ICON_OK && calls == 1);
    cui_end_frame(ui);
    assert(cui_bounds(ui, 100).w == 80 && cui_bounds(ui, 100).h == 40);
    assert(cui_bounds(ui, marker).w == 16 && cui_bounds(ui, marker).h == 16);
    assert(cui_bounds(ui, artwork).w == 18 && cui_bounds(ui, artwork).h == 23);
    size_t count = 0;
    const cui_semantic *semantics = cui_semantics(ui, &count);
    assert(count == 1 && semantics[0].role == CUI_ROLE_STATUS);
    assert(!icon.options.decorative && icon.options.inherited_text_size == 0);
    assert(icon.options.color.r == 211 && style.radius == 2 && constraints.max_width == 18);
    /* Structural rejection is atomic; artwork validation is explicitly later. */
    assert(cui_begin_frame(ui, 400, 200, .016f));
    badge.variant = CUI_BADGE_DOT;
    result = cui_badge_with_icon(ui, 100, &theme, badge, &icon, content, &calls);
    assert(!result.submitted && result.icon_error == CUI_ICON_INVALID && calls == 1);
    badge.variant = CUI_BADGE_ICON;
    assert(!cui_badge_with_icon(ui, 100, &theme, badge, NULL, content, &calls).submitted);
    icon.options.custom_size = NAN;
    result = cui_badge_with_icon(ui, 100, &theme, badge, &icon, content, &calls);
    assert(result.submitted && result.icon_error == CUI_ICON_INVALID && calls == 2);
    cui_end_frame(ui);
    assert(cui_bounds(ui, marker).w == 16);
    cui_destroy(ui);
    cui_icon_state_destroy(&state);
    return 0;
}
