/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
static int right_edge(cui_sdl3 *host, const cui_draw_list *list) {
    assert(cui_sdl3_render_capture(host, list, (cui_color){0,0,0,255}, "tracking-sdl.bmp"));
    SDL_Surface *surface = SDL_LoadBMP("tracking-sdl.bmp"); assert(surface);
    int right = -1;
    for (int y = 0; y < surface->h; ++y) for (int x = 0; x < surface->w; ++x) {
        Uint8 r,g,b,a; assert(SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a));
        if (r > 30 && x > right) right = x;
    }
    SDL_DestroySurface(surface); assert(right > 0); return right;
}
int main(int argc, char **argv) {
    assert(argc == 2); cui_font *font = cui_font_open(argv[1]); assert(font);
    cui_sdl3 *host = cui_sdl3_create("tracking pixels",400,240,font); assert(host);
    int count; SDL_Window **windows = SDL_GetWindows(&count); assert(windows && count == 1);
    SDL_Renderer *renderer = SDL_GetRenderer(windows[0]); SDL_free(windows); assert(renderer);
    cui_draw_command command = {.type=CUI_DRAW_TEXT,.bounds={5,5,150,70},.font_size=40,
        .text={"MM",2},.color={255,255,255,255}};
    for (int pass = 0; pass < 3; ++pass) {
        float scale = pass == 0 ? 1 : pass == 1 ? 1.5f : 2;
        assert(SDL_SetRenderLogicalPresentation(renderer,0,0,SDL_LOGICAL_PRESENTATION_DISABLED));
        assert(SDL_SetRenderScale(renderer,scale,scale));
        cui_draw_list list = {&command,1,400/scale,240/scale};
        command.text_style.letter_spacing=0; int plain=right_edge(host,&list);
        command.text_style.letter_spacing=12; assert(abs(right_edge(host,&list)-plain-(int)(12*scale)) <= 1);
        command.text_style.letter_spacing=-4; assert(abs(right_edge(host,&list)-plain+(int)(4*scale)) <= 1);
        /* Gradient uses the same positioned coverage and tracking. */
        cui_text_gradient_stop stops[]={{0,{255,0,0,255}},{1,{255,100,0,255}}};
        command.text_paint=(cui_text_paint){.kind=CUI_TEXT_PAINT_LINEAR,.x1=100,.stops=stops,.stop_count=2};
        assert(abs(right_edge(host,&list)-plain+(int)(4*scale)) <= 1);
        command.text_paint=(cui_text_paint){0};
    }
    remove("tracking-sdl.bmp"); cui_sdl3_destroy(host); cui_font_close(font); return 0;
}
