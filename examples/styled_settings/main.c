/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <stdio.h>
int main(void){
 cui_context *ui=cui_create((cui_config){.width=640,.height=600});if(!ui)return 1;
 char username[64]="Ada";cui_edit *edit=cui_edit_create((cui_edit_config){.buffer=username,.capacity=sizeof(username)});if(!edit){cui_destroy(ui);return 1;}
 cui_theme t=cui_theme_light();bool notifications=true,sync=false;float volume=60;int mode=1;
 cui_begin_frame(ui,640,600,.016f);cui_begin_box(ui,1,(cui_box){.width=640,.padding=24,.gap=12,.background=t.background});
 cui_label(ui,&t,cui_string("Workspace settings"));
 cui_input(ui,9,&t,cui_string("Display name"),edit,false);
 cui_checkbox(ui,2,&t,cui_string("Email notifications"),&notifications,false);
 cui_switch(ui,3,&t,cui_string("Sync across devices"),&sync,false);
 cui_radio(ui,4,&t,cui_string("Balanced"),&mode,1,false);cui_radio(ui,5,&t,cui_string("Performance"),&mode,2,false);
 cui_slider(ui,6,&t,cui_string("Notification volume"),&volume,0,100,5,false);
 cui_badge(ui,7,&t,cui_string("Changes save locally"));cui_button(ui,8,&t,cui_string("Save settings"),false);
 cui_end_box(ui);cui_draw_list list=cui_end_frame(ui);printf("settings: %zu draw commands\n",list.count);
 cui_error e=cui_last_error(ui);cui_edit_destroy(edit);cui_destroy(ui);return e==CUI_OK?0:1;
}
