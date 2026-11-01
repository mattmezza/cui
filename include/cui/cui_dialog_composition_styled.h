/* SPDX-License-Identifier: MIT */
#ifndef CUI_DIALOG_COMPOSITION_STYLED_H
#define CUI_DIALOG_COMPOSITION_STYLED_H
#include "cui_components.h"
#include "cui_dialog_composition.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Standard button options/callbacks run before owner close reduction. */
bool cui_dialog_action_v1(cui_context *, const cui_dialog_frame *, const cui_theme *, cui_id,
                          cui_str, cui_dialog_action_kind, cui_button_options);
#ifdef __cplusplus
}
#endif
#endif
