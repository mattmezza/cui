#include "cui/cui.h"
#include <stdio.h>
int main(void) { cui_context *ui=cui_create((cui_config){.width=640,.height=480});if(!ui)return 1;cui_begin_frame(ui,640,480,0);cui_begin_box(ui,cui_id_from(ui,"root"),(cui_box){.width=-1,.height=-1,.padding=24,.background={24,30,32,255}});cui_text(ui,cui_string("Hello CUI"),24,(cui_color){240,240,240,255});cui_end_box(ui);cui_draw_list list=cui_end_frame(ui);printf("Clay generated %zu commands\n",list.count);int result=list.count<2||cui_last_error(ui)!=CUI_OK;cui_destroy(ui);return result;}
