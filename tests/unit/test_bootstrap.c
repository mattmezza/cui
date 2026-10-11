#include "cui/cui.h"
#include <assert.h>
int main(void){cui_context *a=cui_create((cui_config){.width=200,.height=100});assert(a);assert(cui_id_from(a,"key")==cui_id_from(a,"key"));assert(cui_begin_frame(a,200,100,0));cui_begin_box(a,cui_id_from(a,"box"),(cui_box){.width=100,.height=40,.background={255,0,0,255}});cui_end_box(a);cui_draw_list l=cui_end_frame(a);assert(l.count==1);assert(l.commands[0].bounds.w==100);assert(cui_last_error(a)==CUI_OK);cui_destroy(a);return 0;}
