#include "cui/cui_questionnaire_presentation.h"
static_assert(CUI_QUESTIONNAIRE_TEXT_LEGACY == 0, "zero compatibility");
void presentation_cpp(cui_context *ui, const cui_theme *t, const cui_questionnaire_state *s) {
    cui_questionnaire_text_options p = {};
    (void)cui_questionnaire_title_ex(ui, 1, t, s, 0, cui_questionnaire_options{}, p);
}
