/* SPDX-License-Identifier: MIT */
#ifndef CUI_COMPONENTS_H
#define CUI_COMPONENTS_H
#include "cui_base.h"
#include "cui_choice.h"
#include "cui_content.h"
#include "cui_disclosure.h"
#include "cui_numeric.h"
#include "cui_picker.h"
#include "cui_scroll.h"
#include "cui_table.h"
#include "cui_theme.h"
#include "cui_tree.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    CUI_BUTTON_PRIMARY,
    CUI_BUTTON_SECONDARY,
    CUI_BUTTON_OUTLINE,
    CUI_BUTTON_GHOST,
    CUI_BUTTON_DANGER,
    CUI_BUTTON_LINK
} cui_button_variant;
typedef enum { CUI_CONTROL_MEDIUM, CUI_CONTROL_SMALL, CUI_CONTROL_LARGE } cui_control_size;
typedef void (*cui_button_content_fn)(cui_context *, void *user);
typedef struct {
    cui_button_variant variant;
    cui_control_size size;
    bool disabled, loading, selected;
    float width; /* zero fits contents; negative grows within parent */
    cui_str leading, trailing;
    cui_button_content_fn content; /* Optional arbitrary child content instead of label. */
    void *content_user;
} cui_button_options;
bool cui_button_ex(cui_context *, cui_id, const cui_theme *, cui_str name, cui_button_options);
bool cui_button(cui_context *, cui_id, const cui_theme *, cui_str, bool disabled);
bool cui_checkbox(cui_context *, cui_id, const cui_theme *, cui_str, bool *, bool disabled);
bool cui_switch(cui_context *, cui_id, const cui_theme *, cui_str, bool *, bool disabled);
bool cui_toggle(cui_context *, cui_id, const cui_theme *, cui_str, bool *, bool disabled);
bool cui_radio(cui_context *, cui_id, const cui_theme *, cui_str, int *, int option, bool disabled);
bool cui_slider(cui_context *, cui_id, const cui_theme *, cui_str, float *, float minimum,
                float maximum, float step, bool disabled);
typedef struct {
    cui_input_behavior_state behavior;
    float scroll_x;
} cui_input_state;
typedef struct {
    bool disabled, readonly, required, invalid;
    float width; /* Zero defaults to 280; negative grows. */
    cui_str placeholder;
    const cui_platform *clipboard;
} cui_input_options;
cui_id cui_input_viewport_id(cui_id);
/* Paint only: compose with a reducer without applying the event stream twice. */
void cui_input_paint(cui_context *, cui_id, const cui_theme *, cui_str, cui_edit *,
                     cui_input_state *, cui_input_options);
cui_input_result cui_input_ex(cui_context *, cui_id, const cui_theme *, cui_str name, cui_edit *,
                              cui_input_state *, cui_input_options);
cui_input_result cui_input(cui_context *, cui_id, const cui_theme *, cui_str name, cui_edit *,
                           bool disabled);
void cui_label(cui_context *, const cui_theme *, cui_str);
void cui_badge(cui_context *, cui_id, const cui_theme *, cui_str);
void cui_progress(cui_context *, cui_id, const cui_theme *, float fraction, float width);
void cui_separator(cui_context *, cui_id, const cui_theme *, float width);
#ifdef __cplusplus
}
#endif
#endif
