#include "cui/cui_questionnaire_progress.h"
static_assert(CUI_QUESTIONNAIRE_PROGRESS_LABEL == 1, "presence mask");
void progress_cpp(cui_context *ui, const cui_theme *theme, const cui_questionnaire_state *state) {
    cui_questionnaire_progress_options options = {};
    (void)cui_questionnaire_progress_ex(ui, 1, theme, state, cui_questionnaire_options{}, options);
}
