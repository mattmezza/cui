/* SPDX-License-Identifier: MIT */
#include "cui/cui_questionnaire_keyboard.h"
static_assert(CUI_QUESTIONNAIRE_SHORTCUT_LEGACY == 0, "legacy zero");
void questionnaire_keyboard_header(cui_context *ui, cui_theme *theme,
                                   cui_questionnaire_state *state) {
    cui_questionnaire_keyboard_options keyboard{};
    cui_questionnaire_options options{};
    if (cui_questionnaire_begin_with_keyboard(ui, 10, theme, state, options, &keyboard))
        cui_questionnaire_end(ui);
}
