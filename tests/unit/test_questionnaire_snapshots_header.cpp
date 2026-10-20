/* SPDX-License-Identifier: MIT */
#include "cui/cui_questionnaire_snapshots.h"
static_assert(CUI_QUESTIONNAIRE_SNAPSHOT_OK == 0, "success zero");
cui_questionnaire_snapshot_result
questionnaire_snapshots_cpp(const cui_questionnaire_state *state) {
    cui_questionnaire_query_snapshot query{};
    return cui_questionnaire_query_copy(state, &query);
}
