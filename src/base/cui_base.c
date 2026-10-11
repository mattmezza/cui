/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include <math.h>
cui_interaction cui_base_button(cui_context *ui,cui_id id,cui_str name,bool disabled) {
 return cui_interact(ui,id,CUI_ROLE_BUTTON,name,disabled,false,0);
}
static bool cui_base_binary(cui_context *ui,cui_id id,cui_str name,bool *value,bool disabled,cui_role role) {
 if(!value)return false;
 cui_interaction s=cui_interact(ui,id,role,name,disabled,*value,0);
 if(disabled||!s.activated)return false;
 *value=!*value; cui_update_semantic(ui,id,*value,*value?1:0);return true;
}
bool cui_base_checkbox(cui_context *u,cui_id i,cui_str n,bool *v,bool d){return cui_base_binary(u,i,n,v,d,CUI_ROLE_CHECKBOX);}
bool cui_base_switch(cui_context *u,cui_id i,cui_str n,bool *v,bool d){return cui_base_binary(u,i,n,v,d,CUI_ROLE_SWITCH);}
bool cui_base_toggle(cui_context *u,cui_id i,cui_str n,bool *v,bool d){return cui_base_binary(u,i,n,v,d,CUI_ROLE_BUTTON);}
bool cui_base_check(cui_context *ui,cui_id id,cui_str name,cui_check_state *v,bool d){
 if(!v)return false;
 cui_interaction s=cui_interact(ui,id,CUI_ROLE_CHECKBOX,name,d,*v==CUI_CHECK_CHECKED,(float)*v);
 if(d||!s.activated)return false;
 *v=*v==CUI_CHECK_CHECKED?CUI_CHECK_UNCHECKED:CUI_CHECK_CHECKED;
 cui_update_semantic(ui,id,*v==CUI_CHECK_CHECKED,(float)*v);return true;
}
bool cui_base_radio(cui_context *ui,cui_id id,cui_str name,int *value,int option,bool disabled){
 if(!value)return false;
 cui_interaction s=cui_interact(ui,id,CUI_ROLE_RADIO,name,disabled,*value==option,(float)option);
 if(disabled||!s.activated||*value==option)return false;
 *value=option;cui_update_semantic(ui,id,true,(float)option);return true;
}
static float cui_base_clamp(float x,float a,float b){return x<a?a:x>b?b:x;}
static float cui_base_quantize(float x,float a,float b,float step){
 x=cui_base_clamp(x,a,b);if(step>0&&isfinite(step)&&x!=b)x=a+roundf((x-a)/step)*step;
 return cui_base_clamp(x,a,b);
}
bool cui_base_slider(cui_context *ui,cui_id id,cui_str name,float *value,float min,float max,float step,bool disabled){
 if(!value||!isfinite(min)||!isfinite(max)||max<min)return false;
 float old=*value;
 float next=isfinite(old)?cui_base_clamp(old,min,max):min;
 cui_interaction s=cui_interact(ui,id,CUI_ROLE_SLIDER,name,disabled,false,next);
 if(disabled)return false;
 cui_rect b=cui_bounds(ui,id);size_t count=0;
 const cui_event *events=cui_frame_events(ui,&count);
 bool dragging=s.pressed;
 for(size_t i=0;i<count;i++){
  cui_event e=events[i];
  if(cui_event_target(ui,i)!=id)continue;
  if(e.type==CUI_EVENT_POINTER_DOWN)dragging=true;
  if(dragging&&(e.type==CUI_EVENT_POINTER_DOWN||e.type==CUI_EVENT_POINTER_MOVE||e.type==CUI_EVENT_POINTER_UP)&&b.w>0)
   next=cui_base_quantize(min+(max-min)*cui_base_clamp((e.x-b.x)/b.w,0,1),min,max,step);
  if(e.type==CUI_EVENT_POINTER_UP||e.type==CUI_EVENT_CANCEL)dragging=false;
  if(e.type==CUI_EVENT_KEY_DOWN){
   float increment=step>0&&isfinite(step)?step:(max-min)/100;
   if(e.key==CUI_KEY_HOME)next=min;
   else if(e.key==CUI_KEY_END)next=max;
   else if(e.key==CUI_KEY_LEFT||e.key==CUI_KEY_DOWN)next=cui_base_quantize(next-increment,min,max,step);
   else if(e.key==CUI_KEY_RIGHT||e.key==CUI_KEY_UP)next=cui_base_quantize(next+increment,min,max,step);
  }
 }
 *value=next;cui_update_semantic(ui,id,false,next);return old!=next;
}

cui_input_result cui_base_input(cui_context *ui,cui_id id,cui_str name,cui_edit *edit,bool disabled){
 cui_input_result result={.error=CUI_EDIT_OK};
 if(!edit){result.error=CUI_EDIT_INVALID_ARGUMENT;return result;}
 cui_edit_state before=cui_edit_get_state(edit);
 cui_interact(ui,id,CUI_ROLE_TEXTBOX,name,disabled,false,(float)before.length);
 result.error=cui_edit_apply_events(edit,ui,id,disabled);
 cui_edit_state after=cui_edit_get_state(edit);
 result.changed=before.revision!=after.revision;
 if(!disabled&&!after.multiline){
  size_t n=0;const cui_event *events=cui_frame_events(ui,&n);
  for(size_t i=0;i<n;i++)if(cui_event_target(ui,i)==id&&events[i].type==CUI_EVENT_KEY_DOWN&&events[i].key==CUI_KEY_ENTER&&!events[i].repeat)result.submitted=true;
 }
 cui_update_semantic(ui,id,false,(float)after.length);return result;
}
