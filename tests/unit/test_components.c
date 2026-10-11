/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
static bool checked;
static void scene(cui_context *ui,const cui_theme *t){
 assert(cui_begin_frame(ui,640,480,.016f));
 cui_begin_box(ui,1,(cui_box){.width=640,.padding=16,.gap=8,.background=t->background});
 cui_label(ui,t,cui_string("Settings"));cui_button(ui,2,t,cui_string("Save"),false);
 cui_checkbox(ui,3,t,cui_string("Notifications"),&checked,false);
 cui_switch(ui,4,t,cui_string("Sync"),&checked,true);
 cui_badge(ui,5,t,cui_string("Preview"));cui_progress(ui,6,t,.5f,200);cui_separator(ui,7,t,300);
 cui_end_box(ui);cui_draw_list list=cui_end_frame(ui);assert(list.count>10);assert(cui_last_error(ui)==CUI_OK);
 bool fill=false;for(size_t i=0;i<list.count;i++)if(list.commands[i].type==CUI_DRAW_RECT&&list.commands[i].bounds.w==100)fill=true;
 assert(fill);
}
int main(void){
 cui_context *ui=cui_create((cui_config){.width=640,.height=480});assert(ui);
 cui_theme themes[]={cui_theme_light(),cui_theme_dark(),cui_theme_high_contrast(),cui_theme_terminal()};
 for(size_t i=0;i<4;i++)scene(ui,&themes[i]);
 cui_rect b=cui_bounds(ui,3);assert(cui_queue_event(ui,(cui_event){.type=CUI_EVENT_POINTER_DOWN,.x=b.x+5,.y=b.y+5}));assert(cui_queue_event(ui,(cui_event){.type=CUI_EVENT_POINTER_UP,.x=b.x+5,.y=b.y+5}));scene(ui,&themes[0]);assert(checked);
 size_t count;const cui_semantic *s=cui_semantics(ui,&count);assert(count==3);assert(s[1].role==CUI_ROLE_CHECKBOX&&s[1].checked);assert(s[2].disabled&&s[2].role==CUI_ROLE_SWITCH);
 assert(themes[0].radius!=themes[3].radius&&themes[0].primary.b!=themes[3].primary.b);
 char buffer[64]="Ada";cui_edit *edit=cui_edit_create((cui_edit_config){.buffer=buffer,.capacity=sizeof(buffer),.history_bytes=1024,.max_history_steps=16});assert(edit);
 assert(cui_begin_frame(ui,640,480,.016f));cui_input_result ir=cui_input(ui,44,&themes[0],cui_string("Name"),edit,false);assert(!ir.changed&&!ir.submitted&&ir.error==CUI_EDIT_OK);cui_end_frame(ui);
 cui_focus(ui,44);assert(cui_queue_event(ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_A,.modifiers=CUI_MOD_CTRL}));
 assert(cui_queue_event(ui,(cui_event){.type=CUI_EVENT_TEXT,.text="Eva"}));
 assert(cui_begin_frame(ui,640,480,.016f));ir=cui_input(ui,44,&themes[0],cui_string("Name"),edit,false);assert(ir.changed&&ir.error==CUI_EDIT_OK);cui_end_frame(ui);assert(cui_edit_get_text(edit).length==3&&buffer[0]=='E');
 s=cui_semantics(ui,&count);assert(count==1&&s[0].role==CUI_ROLE_TEXTBOX&&s[0].value==3);
 assert(cui_queue_event(ui,(cui_event){.type=CUI_EVENT_KEY_DOWN,.key=CUI_KEY_ENTER}));
 assert(cui_begin_frame(ui,640,480,.016f));ir=cui_input(ui,44,&themes[0],cui_string("Name"),edit,false);assert(ir.submitted&&!ir.changed);cui_end_frame(ui);
 assert(cui_queue_event(ui,(cui_event){.type=CUI_EVENT_TEXT,.text="!"}));
 assert(cui_begin_frame(ui,640,480,.016f));ir=cui_input(ui,44,&themes[0],cui_string("Name"),edit,true);assert(!ir.changed);cui_end_frame(ui);assert(cui_edit_get_text(edit).length==3);
 cui_edit_destroy(edit);cui_destroy(ui);return 0;
}
