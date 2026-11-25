/* SPDX-License-Identifier: MIT */
export function hoverCardUsage(mode, preview) {
 const edge = preview === 2;
 const body = preview
  ? 'Alex designs tools for teams and keeps a notebook full of ideas from the people who use them.'
  : 'A composable floating card.';
 const begin = mode ? `    cui_hover_card_policy policy = {.mode = CUI_HOVER_CARD_TAP_TOGGLE,
        .accepted_trigger_activation = accepted, .outside_tap = true,
        .relationship = true};
    cui_hover_card_extended_result result = cui_begin_hover_card_with_release(
        ui, card, theme, state, cui_string("Profile details"), options, &policy,
        ${mode === 2 ? 'NULL, CUI_HOVER_CARD_RELEASE_RAW_PRIMARY' : 'contact, CUI_HOVER_CARD_RELEASE_LEGACY'},
        (cui_hover_handler){0});
    if (result.lifecycle.control_result) {` : `    if (cui_begin_hover_card(ui, card, theme, state, cui_string("Profile details"), options)) {`;
 return `#include "cui/cui_components.h"
${mode ? '#include "cui/cui_hover_card_extended.h"\n' : ''}
/* Keep state per card across frames.${mode === 1 ? ' Also retain contact for strict outside taps.' : ''}
 * Begin inside a viewport-sized box. The card fits its children with height = 0. */
void draw_profile(cui_context *ui, const cui_theme *theme, cui_hover_state *state,
${mode === 1 ? '                  cui_hover_card_contact *contact,\n' : ''}                  cui_rect viewport, float dt, bool disabled) {
    cui_id trigger = cui_id_from(ui, "profile-trigger");
    cui_id card = cui_id_from(ui, "profile-card");
${edge ? `    cui_begin_absolute_box(ui, 0, (cui_box){.align_x = CUI_ALIGN_END},
                           (cui_rect){24, viewport.h - 103, viewport.w - 48,
                                      theme->control_height});\n` : ''}    ${mode ? 'bool accepted = ' : ''}cui_button(ui, trigger, theme, cui_string("Hover or focus here"), disabled);
${edge ? '    cui_end_box(ui);\n' : ''}    cui_hover_options options = {.trigger = trigger, .viewport = viewport,
        .side = CUI_POPUP_BELOW, .width = viewport.w > 360 ? 280 : viewport.w - 48,
        .gap = 8, .dt = dt, .open_delay = .25f, .close_delay = .1f, .disabled = disabled};
${begin}
        cui_label(ui, theme, cui_string("Alex Morgan"));
        cui_label(ui, theme, cui_string("${body}"));
        cui_end_hover_card(ui);
    }
}
`;
}
