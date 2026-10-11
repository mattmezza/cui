/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <math.h>
static cui_box control(const cui_theme *t,bool disabled,bool selected){
 return (cui_box){.height=t->control_height,.padding=t->padding,.gap=t->gap,.radius=t->radius,.row=true,
 .background=selected&&!disabled?t->primary:t->surface,.border=disabled?t->muted:t->border,.border_width=1};
}
static void focus(cui_context *ui,cui_id id,const cui_theme *t,cui_box *b){
 if(cui_focused(ui)==id){b->border=t->focus;b->border_width=2;}
}
static void mark(cui_context *ui,const cui_theme *t,const char *text,bool selected,bool disabled){
 cui_text(ui,cui_string(text),t->font_size,disabled?t->muted:selected?t->on_primary:t->text);
}
bool cui_button(cui_context *ui,cui_id id,const cui_theme *t,cui_str name,bool disabled){
 cui_interaction state=cui_base_button(ui,id,name,disabled);
 cui_box b=control(t,disabled,true);focus(ui,id,t,&b);
 if(!disabled&&(state.hovered||state.pressed)){
  float factor=state.pressed?.75f:.9f;
  b.background.r=(uint8_t)(b.background.r*factor);b.background.g=(uint8_t)(b.background.g*factor);b.background.b=(uint8_t)(b.background.b*factor);
 }
 cui_begin_box(ui,id,b);
 cui_text(ui,name,t->font_size,disabled?t->muted:t->on_primary);cui_end_box(ui);return state.activated;
}
static bool binary_control(cui_context *ui,cui_id id,const cui_theme *t,cui_str name,bool *value,bool disabled,int kind){
 if(!value)return false;
 bool changed=kind==0?cui_base_checkbox(ui,id,name,value,disabled):kind==1?cui_base_switch(ui,id,name,value,disabled):cui_base_toggle(ui,id,name,value,disabled);
 cui_box b=control(t,disabled,kind==2&&*value);focus(ui,id,t,&b);cui_begin_box(ui,id,b);
 const char *indicator=kind==0?(*value?"[x]":"[ ]"):kind==1?(*value?"ON":"OFF"):(*value?"+":"-");
 mark(ui,t,indicator,kind==2&&*value,disabled);cui_text(ui,name,t->font_size,disabled?t->muted:kind==2&&*value?t->on_primary:t->text);cui_end_box(ui);return changed;
}
bool cui_checkbox(cui_context *u,cui_id i,const cui_theme *t,cui_str n,bool *v,bool d){return binary_control(u,i,t,n,v,d,0);}
bool cui_switch(cui_context *u,cui_id i,const cui_theme *t,cui_str n,bool *v,bool d){return binary_control(u,i,t,n,v,d,1);}
bool cui_toggle(cui_context *u,cui_id i,const cui_theme *t,cui_str n,bool *v,bool d){return binary_control(u,i,t,n,v,d,2);}
bool cui_radio(cui_context *ui,cui_id id,const cui_theme *t,cui_str name,int *value,int option,bool disabled){
 if(!value)return false;
 bool changed=cui_base_radio(ui,id,name,value,option,disabled);
 cui_box b=control(t,disabled,false);focus(ui,id,t,&b);cui_begin_box(ui,id,b);mark(ui,t,*value==option?"(*)":"( )",false,disabled);
 cui_text(ui,name,t->font_size,disabled?t->muted:t->text);cui_end_box(ui);return changed;
}
void cui_label(cui_context *ui,const cui_theme *t,cui_str label){cui_text(ui,label,t->font_size,t->text);}
void cui_badge(cui_context *ui,cui_id id,const cui_theme *t,cui_str label){
 cui_begin_box(ui,id,(cui_box){.padding=4,.radius=t->radius,.background=t->primary});cui_text(ui,label,t->font_size*.8f,t->on_primary);cui_end_box(ui);
}
void cui_progress(cui_context *ui,cui_id id,const cui_theme *t,float fraction,float width){
 if(!isfinite(fraction))fraction=0;
 fraction=fraction<0?0:fraction>1?1:fraction;
 if(width<=0||!isfinite(width))width=160;
 cui_begin_box(ui,id,(cui_box){.width=width,.height=8,.radius=t->radius,.background=t->border,.row=true});
 if(fraction>0){cui_id fill=id^UINT64_C(0x7129d4cd19342f11);cui_begin_box(ui,fill,(cui_box){.width=width*fraction,.height=8,.radius=t->radius,.background=t->primary});cui_end_box(ui);}
 cui_end_box(ui);
}
bool cui_slider(cui_context *ui,cui_id id,const cui_theme *t,cui_str name,float *value,float min,float max,float step,bool disabled){
 cui_box b=control(t,disabled,false);b.width=260;b.row=false;b.height=0;focus(ui,id,t,&b);cui_begin_box(ui,id,b);
 bool changed=cui_base_slider(ui,id,name,value,min,max,step,disabled);
 cui_text(ui,name,t->font_size,disabled?t->muted:t->text);
 float ratio=value&&isfinite(*value)&&max>min?(*value-min)/(max-min):0;
 cui_progress(ui,id^UINT64_C(0x39c1a2170384edaa),t,ratio,240);cui_end_box(ui);return changed;
}
void cui_separator(cui_context *ui,cui_id id,const cui_theme *t,float width){
 cui_begin_box(ui,id,(cui_box){.width=width!=0?width:160,.height=1,.background=t->border});cui_end_box(ui);
}
