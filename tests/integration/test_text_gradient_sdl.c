/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
int main(int argc, char **argv) {
    assert(argc == 2);
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    cui_sdl3 *host = cui_sdl3_create("gradient pixels", 320, 160, font);
    assert(host);
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    assert(windows && count == 1);
    SDL_Renderer *renderer = SDL_GetRenderer(windows[0]);
    SDL_free(windows);
    assert(renderer);
    cui_text_gradient_stop stops[] = {{0, {255, 0, 0, 255}}, {1, {0, 0, 255, 255}}};
    cui_draw_command command = {
        .type = CUI_DRAW_TEXT,
        .bounds = {0, 0, 60, 80},
        .text = {"M", 1},
        .font_size = 64,
        .text_paint = {.kind = CUI_TEXT_PAINT_LINEAR, .x1 = 50, .stops = stops, .stop_count = 2}};
    for (int pass = 0; pass < 3; ++pass) {
        float scale = pass == 0 ? 1 : pass == 1 ? 1.5f : 2;
        assert(SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED));
        assert(SDL_SetRenderScale(renderer, scale, scale));
        cui_draw_list list = {&command, 1, 320 / scale, 160 / scale};
        assert(cui_sdl3_render_capture(host, &list, (cui_color){0, 0, 0, 255}, "gradient-sdl.bmp"));
        SDL_Surface *surface = SDL_LoadBMP("gradient-sdl.bmp");
        assert(surface);
        size_t red = 0, blue = 0;
        for (int y = 0; y < surface->h; ++y)
            for (int x = 0; x < surface->w; ++x) {
                Uint8 r, g, b, a;
                assert(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a));
                red += r > b + 20;
                blue += b > r + 20;
            }
        assert(red > 20 && blue > 20);
        SDL_DestroySurface(surface);
        cui_draw_command clipped[] = {{.type = CUI_DRAW_CLIP_PUSH, .bounds = {0, 0, 20, 100}},
                                      command,
                                      {.type = CUI_DRAW_CLIP_POP}};
        list.commands = clipped;
        list.count = 3;
        assert(cui_sdl3_render_capture(host, &list, (cui_color){0, 0, 0, 255}, "gradient-sdl.bmp"));
        surface = SDL_LoadBMP("gradient-sdl.bmp");
        assert(surface);
        for (int y = 0; y < surface->h; ++y)
            for (int x = (int)(20 * scale); x < surface->w; ++x) {
                Uint8 r, g, b, a;
                assert(SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a) && !r && !g && !b);
            }
        SDL_DestroySurface(surface);
    }
    assert(cui_sdl3_interface(host).capabilities & CUI_RENDER_TEXT_LINEAR_GRADIENT);
    remove("gradient-sdl.bmp");
    cui_sdl3_destroy(host);
    cui_font_close(font);
    return 0;
}
