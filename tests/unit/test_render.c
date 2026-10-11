/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
static const unsigned char *pixel(cui_software *r,int x,int y){unsigned w;cui_software_pixels(r,&w,NULL);return cui_software_pixels(r,NULL,NULL)+((size_t)y*w+(unsigned)x)*4;}
int main(void){
    cui_software *r=cui_software_create(16,16,NULL);assert(r);
    cui_draw_command commands[]={
        {.type=CUI_DRAW_RECT,.bounds={0,0,16,16},.color={255,0,0,128}},
        {.type=CUI_DRAW_CLIP_PUSH,.bounds={4,4,8,8}},
        {.type=CUI_DRAW_CLIP_PUSH,.bounds={6,2,8,6}},
        {.type=CUI_DRAW_RECT,.bounds={0,0,16,16},.color={0,255,0,255}},
        {.type=CUI_DRAW_CLIP_POP},{.type=CUI_DRAW_CLIP_POP}
    };
    cui_draw_list list={commands,6,16,16};assert(cui_software_render(r,&list,(cui_color){0,0,255,255}));
    assert(pixel(r,0,0)[0]==128 && pixel(r,0,0)[2]==127 && pixel(r,0,0)[3]==255);
    assert(pixel(r,6,4)[1]==255);assert(pixel(r,5,4)[1]==0);assert(pixel(r,6,8)[1]==0);
    commands[0]=(cui_draw_command){.type=CUI_DRAW_RECT,.bounds={2,2,12,12},.radius=6,.color={255,255,255,255}};
    list.count=1;assert(cui_software_render(r,&list,(cui_color){0,0,0,0}));
    assert(pixel(r,2,2)[3]==0);assert(pixel(r,8,8)[3]==255);
    commands[0]=(cui_draw_command){.type=CUI_DRAW_BORDER,.bounds={2,2,12,12},.radius=3,.stroke=2,.color={255,255,255,255}};
    assert(cui_software_render(r,&list,(cui_color){0,0,0,0}));assert(pixel(r,8,2)[3]==255);assert(pixel(r,8,8)[3]==0);
    commands[0]=(cui_draw_command){.type=CUI_DRAW_RECT,.bounds={0,0,1,1},.color={100,40,20,128}};
    assert(cui_software_render(r,&list,(cui_color){0,0,0,0}));assert(pixel(r,0,0)[0]==100 && pixel(r,0,0)[3]==128);
    commands[0].type=CUI_DRAW_CLIP_POP;assert(!cui_software_render(r,&list,(cui_color){0}));
    assert(!cui_software_resize(r,0,16));assert(cui_software_resize(r,8,8));
    unsigned w,h;cui_software_pixels(r,&w,&h);assert(w==8 && h==8);
    cui_software_destroy(r);puts("software renderer geometry, nested clipping and straight alpha passed");return 0;
}
