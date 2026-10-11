/* SPDX-License-Identifier: MIT */
#ifndef CUI_COMPONENTS_H
#define CUI_COMPONENTS_H
#include "cui_base.h"
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
bool cui_button(cui_context *,cui_id,const cui_theme *,cui_str,bool disabled);
bool cui_checkbox(cui_context *,cui_id,const cui_theme *,cui_str,bool *,bool disabled);
bool cui_switch(cui_context *,cui_id,const cui_theme *,cui_str,bool *,bool disabled);
bool cui_toggle(cui_context *,cui_id,const cui_theme *,cui_str,bool *,bool disabled);
bool cui_radio(cui_context *,cui_id,const cui_theme *,cui_str,int *,int option,bool disabled);
bool cui_slider(cui_context *,cui_id,const cui_theme *,cui_str,float *,float minimum,float maximum,float step,bool disabled);
void cui_label(cui_context *,const cui_theme *,cui_str);
void cui_badge(cui_context *,cui_id,const cui_theme *,cui_str);
void cui_progress(cui_context *,cui_id,const cui_theme *,float fraction,float width);
void cui_separator(cui_context *,cui_id,const cui_theme *,float width);
#ifdef __cplusplus
}
#endif
#endif
