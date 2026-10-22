#include "cui/cui_questionnaire_geometry.h"
void questionnaire_geometry_example(cui_context *ui, const cui_theme *theme,
                                    cui_questionnaire_state *state) {
    cui_questionnaire_options options = {.design_size = CUI_DESIGN_BASE};
    cui_questionnaire_geometry_options geometry = {
        .metric_fields = CUI_QUESTIONNAIRE_METRIC_MINIMUM_HEIGHT,
        .metrics = {.minimum_height = 64},
        .card = {.fields = CUI_QUESTIONNAIRE_BOX_PADDING,
                 .padding = {.left = 12, .right = 16, .top = 8, .bottom = 8}},
        .answer = {.fields = CUI_QUESTIONNAIRE_TEXT_SIZE | CUI_QUESTIONNAIRE_TEXT_LINE_HEIGHT,
                   .font_size = 18,
                   .line_height = 28}};
    if (!cui_questionnaire_begin_ex(ui, 100, theme, state, options, NULL, &geometry))
        return;
    size_t item = state->current;
    if (cui_questionnaire_item_begin_ex(ui, 101, state, item, options, &geometry)) {
        if (cui_questionnaire_choices_begin_ex(ui, 102, state, item, options, &geometry)) {
            for (size_t choice = 0; choice < state->items[item].choice_count; ++choice)
                (void)cui_questionnaire_choice_control_ex(ui, 200 + choice, theme, state, item,
                                                          choice, options, NULL, &geometry);
            cui_questionnaire_part_end(ui);
        }
        cui_questionnaire_part_end(ui);
    }
    cui_questionnaire_end(ui);
}
