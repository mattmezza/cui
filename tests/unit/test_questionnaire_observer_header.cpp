/* SPDX-License-Identifier: MIT */
#include "cui/cui_questionnaire_observer.h"
static_assert(CUI_QUESTIONNAIRE_OBSERVER_OK == 0, "success zero");
cui_questionnaire_observer_result questionnaire_observer_cpp(cui_questionnaire_state *state) {
    static cui_questionnaire_observer observer{};
    return cui_questionnaire_observer_attach(state, &observer, 900, nullptr, 0,
                                             cui_questionnaire_observation_handler{});
}
