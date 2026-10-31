/* SPDX-License-Identifier: MIT */
export function hoverCardUsage(mode) {
 if(!mode)return null;
 return `#include "cui/cui_components.h"
#include "cui/cui_hover_card_extended.h"

/* Retain state per card; LEGACY outside taps also need a contact workspace.
 * Reset retained state/contact on context recreation.
 * Submit after leaf input consumers. Descriptors are borrowed only this call.
 * Raw policy uses content-only prior bounds; host-consumed releases are ignored.
 * Core snapshots support controls/expanded; collapsed native AT is still limited. */
void draw_profile(cui_context *ui, const cui_theme *theme,
                  cui_hover_state *state, cui_hover_card_contact *contact,
                  cui_rect viewport, bool disabled) {
    cui_id trigger = cui_id_from(ui, "profile-trigger");
    bool accepted = cui_button(ui, trigger, theme, cui_string("Profile"), disabled);
    cui_hover_card_policy policy = {.mode = CUI_HOVER_CARD_TAP_TOGGLE,
        .accepted_trigger_activation = accepted, .outside_tap = true,
        .relationship = true};
    cui_hover_card_extended_result result = cui_begin_hover_card_with_release(
        ui, cui_id_from(ui, "profile-card"), theme, state, cui_string("Profile details"),
        (cui_hover_options){.trigger = trigger, .viewport = viewport,
            .side = CUI_POPUP_BELOW, .width = 280, .gap = 8, .disabled = disabled},
        &policy, contact, ${mode===2?'CUI_HOVER_CARD_RELEASE_RAW_PRIMARY':'CUI_HOVER_CARD_RELEASE_LEGACY'},
        (cui_hover_handler){0});
    if (result.lifecycle.control_result) {
        cui_label(ui, theme, cui_string("Alex Morgan"));
        cui_label(ui, theme, cui_string("A composable floating card."));
        cui_end_hover_card(ui);
    }
}
`;
}
