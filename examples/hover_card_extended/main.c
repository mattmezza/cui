/* SPDX-License-Identifier: MIT */
#include "cui/cui_hover_card_extended.h"
#include "cui/cui_components.h"
#include <assert.h>
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 320, .height = 240});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_hover_state state = {0};
    cui_hover_card_contact contact = {0};
    /* In a host, retain state/contact and repeat this submission every frame. */
    for (unsigned frame = 0; frame < 2; ++frame) {
        if (frame) {
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                .accessibility_target = 10, .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
        }
        assert(cui_begin_frame(ui, 320, 240, .016f));
        bool accepted = cui_button(ui, 10, &theme, cui_string("Details"), false);
        cui_hover_card_policy policy = {.mode = CUI_HOVER_CARD_TAP_TOGGLE,
            .accepted_trigger_activation = accepted, .outside_tap = true, .relationship = true};
        cui_hover_card_extended_result result = cui_begin_hover_card_extended(
            ui, 11, &theme, &state, cui_string("Profile details"),
            (cui_hover_options){.trigger = 10, .viewport = {0, 0, 320, 240},
                .side = CUI_POPUP_BELOW, .width = 200, .height = 80},
            &policy, &contact, (cui_hover_handler){0});
        assert(result.lifecycle.valid && result.relationship_applied);
        if (result.lifecycle.control_result) {
            cui_text(ui, cui_string("Application-owned content"), theme.font_size, theme.text);
            cui_end_hover_card(ui);
        }
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    assert(state.open);
    cui_destroy(ui);
    return 0;
}
