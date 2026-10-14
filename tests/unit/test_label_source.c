/* SPDX-License-Identifier: MIT */
#include "cui/cui_label.h"
#include <assert.h>
#include <string.h>
static cui_theme theme;
static char text[256];
static cui_text_color_span spans[64];
static cui_label_prepared prepared;
static cui_label_options options(const char *primary,const char *secondary,const char *match) {
    return (cui_label_options){.primary=cui_string(primary),.secondary=cui_string(secondary),
        .has_secondary=true,.highlight=cui_string(match)};
}
static void prepare(cui_label_options o) {
    assert(cui_label_prepare(&theme,o,text,sizeof(text),spans,64,&prepared));
}
static cui_color color_at(size_t byte) {
    for(size_t i=0;i<prepared.span_count;++i)
        if(byte>=spans[i].byte_start && byte<spans[i].byte_end) return spans[i].color;
    assert(0);return (cui_color){0};
}
int main(void) {
    theme=(cui_theme){.text={1,0,0,255},.muted={2,0,0,255},.info={3,0,0,255},.font_size=16};
    cui_label_options o=options("É🙂","世界 É","é");prepare(o);
    assert(prepared.text_length==strlen("É🙂 世界 É"));
    assert(color_at(0).r==3 && color_at(2).r==1 && color_at(6).r==2 && color_at(14).r==3);
    o=options("İX É","É","é");prepare(o);
    assert(color_at(4).r==3 && color_at(7).r==3); /* expansion before match */
    o=options("İX","","i\xcc\x87");o.match=CUI_LABEL_MATCH_PREFIX;prepare(o);
    assert(color_at(0).r==3 && color_at(2).r==1);
    o=options("ΟΣ","","ος");prepare(o);assert(color_at(0).r==3); /* final sigma */
    o=options("ΟΣΑ","","οσα");prepare(o);assert(color_at(0).r==3);
    o=options("𐐀","𐐀","𐐨");prepare(o);assert(color_at(0).r==3 && color_at(5).r==3);
    o=options("ÉÉ","","é");o.match=CUI_LABEL_MATCH_PREFIX;prepare(o);
    assert(color_at(0).r==3 && color_at(2).r==1);
    o=options("É","","e");prepare(o);assert(color_at(0).r==1); /* no normalization */
    o=options("","","");prepare(o);assert(prepared.text_length==1 && color_at(0).r==2);
    o=options("aaa","","aa");prepare(o);assert(color_at(2).r==3); /* overlapping */
    o=options("é🙂","世界","🙂 世");o.masked=true;prepare(o);
    assert(prepared.text_length==15 && prepared.span_count==0);
    assert(!memcmp(text,"•••••",15));
    cui_label_prepared before=prepared;
    memset(text,42,sizeof(text));memset(spans,42,sizeof(spans));
    assert(!cui_label_prepare(&theme,o,text,14,spans,64,&prepared));
    assert(!memcmp(&before,&prepared,sizeof(before)) && text[0]==42);
    o.masked=false;o.primary=cui_string("\xc0\xaf");
    assert(!cui_label_prepare(&theme,o,text,sizeof(text),spans,64,&prepared));
    assert(text[0]==42 && ((unsigned char *)spans)[0]==42);
    o=options("A","B","a");
    assert(!cui_label_prepare(&theme,o,text,sizeof(text),spans,0,&prepared));assert(text[0]==42);
    cui_context *ui=cui_create((cui_config){.max_elements=64,.frame_bytes=8192});assert(ui);
    assert(cui_begin_frame(ui,400,200,0));
    o=options("é🙂","世界","🙂 世");o.masked=true;o.semantic_id=42;o.control=43;
    o.style=(cui_text_style){.weight=CUI_TEXT_WEIGHT_BOLD,.italic=true,.family=7};o.line_height=28;
    assert(cui_label_source(ui,&theme,o));
    cui_draw_list list=cui_end_frame(ui);assert(list.count==1);
    assert(list.commands[0].text.length==15 && !memcmp(list.commands[0].text.data,"•••••",15));
    assert(!memcmp(list.commands[0].text_source.paragraph.data,"•••••",15));
    assert(list.commands[0].text_style.weight==CUI_TEXT_WEIGHT_BOLD && list.commands[0].text_style.italic);
    assert(list.commands[0].text_style.family==7 && list.commands[0].bounds.h==28);
    size_t n=0;const cui_semantic *sem=cui_semantics(ui,&n);assert(n==1);
    assert(sem[0].name.length==15 && !memcmp(sem[0].name.data,"•••••",15));
    assert(sem[0].details.controls==43 && sem[0].details.focus_target==43 && sem[0].details.secret);
    assert(cui_begin_frame(ui,400,200,0));o=options("Primary","Secondary","ary");
    assert(cui_label_source(ui,&theme,o));list=cui_end_frame(ui);assert(list.count==1);
    assert(list.commands[0].bounds.h==20);
    cui_destroy(ui);
    return 0;
}
