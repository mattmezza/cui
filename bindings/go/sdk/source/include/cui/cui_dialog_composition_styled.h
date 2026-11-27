/* SPDX-License-Identifier: MIT */
#ifndef CUI_DIALOG_COMPOSITION_STYLED_H
#define CUI_DIALOG_COMPOSITION_STYLED_H
#include "cui_button_styles.h"
#include "cui_dialog_composition.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Standard button options/callbacks run before owner close reduction. */
bool cui_dialog_action_v1(cui_context *, const cui_dialog_frame *, const cui_theme *, cui_id,
                          cui_str, cui_dialog_action_kind, cui_button_options);
/* Same admission and close reduction, with an optional Button focus treatment. */
bool cui_dialog_action_focus_v1(cui_context *, const cui_dialog_frame *, const cui_theme *, cui_id,
                                cui_str, cui_dialog_action_kind, cui_button_options,
                                cui_button_focus_mode);
#ifdef __cplusplus
}
#endif
#endif
