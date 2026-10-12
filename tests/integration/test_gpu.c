/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl_gpu.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static void pixel(const unsigned char *pixels, unsigned w, unsigned h, float x, float y, int red,
                  int green, int blue) {
    unsigned px = (unsigned)(x * w), py = (unsigned)(y * h);
    const unsigned char *p = pixels + ((size_t)py * w + px) * 3;
    assert(abs((int)p[0] - red) <= 1 && abs((int)p[1] - green) <= 1 && abs((int)p[2] - blue) <= 1);
}
static unsigned char *capture(cui_sdl_gpu *gpu, unsigned *w, unsigned *h) {
    assert(cui_sdl_gpu_save_ppm(gpu, "gpu-shapes.ppm"));
    FILE *f = fopen("gpu-shapes.ppm", "rb");
    assert(f);
    char magic[3];
    unsigned max;
    assert(fscanf(f, "%2s %u %u %u", magic, w, h, &max) == 4 && max == 255);
    assert(fgetc(f) == '\n');
    unsigned char *data = malloc((size_t)*w * *h * 3);
    assert(data && fread(data, 3, (size_t)*w * *h, f) == (size_t)*w * *h);
    assert(!fclose(f));
    return data;
}
static void shapes(cui_sdl_gpu *gpu) {
    cui_draw_command c = {.type = CUI_DRAW_RECT,
                          .bounds = {8, 8, 48, 48},
                          .color = {255, 0, 0, 128},
                          .custom_corners = true,
                          .corner_radii = {12, 0, 0, 6},
                          .custom_border_edges = true,
                          .border_edges = {0, 4, 2, 6}};
    cui_draw_list list = {&c, 1, 200, 200};
    assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    unsigned char *data = capture(gpu, &w, &h);
    pixel(data, w, h, 8.5f / 200, 8.5f / 200, 0, 0, 0);
    pixel(data, w, h, 55.5f / 200, 8.5f / 200, 128, 0, 0);
    pixel(data, w, h, 8.5f / 200, 55.5f / 200, 128, 0, 0);
    pixel(data, w, h, 55.5f / 200, 55.5f / 200, 0, 0, 0);
    free(data);
    c.type = CUI_DRAW_BORDER;
    assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0, 0, 0, 255}));
    data = capture(gpu, &w, &h);
    pixel(data, w, h, 8.5f / 200, 32.5f / 200, 0, 0, 0);
    pixel(data, w, h, 54.5f / 200, 32.5f / 200, 128, 0, 0);
    pixel(data, w, h, 32.5f / 200, 8.5f / 200, 128, 0, 0);
    pixel(data, w, h, 32.5f / 200, 54.5f / 200, 128, 0, 0);
    pixel(data, w, h, 32.5f / 200, 32.5f / 200, 0, 0, 0);
    free(data);
    cui_draw_command clipped[] = {
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {32, 0, 32, 64}}, c, {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list clip_list = {clipped, 3, 200, 200};
    assert(cui_sdl_gpu_render(gpu, &clip_list, (cui_color){0, 0, 0, 255}));
    data = capture(gpu, &w, &h);
    pixel(data, w, h, 20.5f / 200, 8.5f / 200, 0, 0, 0);
    pixel(data, w, h, 40.5f / 200, 8.5f / 200, 128, 0, 0);
    free(data);
}
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *window = SDL_CreateWindow("CUI GPU behavioral verification", 200, 200, 0);
    assert(window);
    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    assert(device);
    assert(SDL_ClaimWindowForGPUDevice(device, window));
    cui_sdl_gpu *gpu = cui_sdl_gpu_create(device, window, NULL);
    assert(gpu);
    cui_draw_command commands[] = {
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 200, 200}, .color = {200, 20, 20, 255}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {40, 40, 120, 120}},
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 200, 200}, .color = {20, 180, 40, 255}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {80, 80, 40, 40}},
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 200, 200}, .color = {20, 40, 200, 255}},
        {.type = CUI_DRAW_CLIP_POP},
        {.type = CUI_DRAW_RECT, .bounds = {40, 40, 20, 20}, .color = {255, 255, 255, 128}},
        {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list list = {commands, sizeof(commands) / sizeof(commands[0]), 200, 200};
    for (int i = 0; i < 3; i++)
        assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0}));
    assert(cui_sdl_gpu_save_ppm(gpu, "gpu-test.ppm"));
    FILE *f = fopen("gpu-test.ppm", "rb");
    assert(f);
    char magic[3];
    unsigned w, h, max;
    assert(fscanf(f, "%2s %u %u %u", magic, &w, &h, &max) == 4);
    assert(magic[0] == 'P' && magic[1] == '6' && max == 255);
    assert(fgetc(f) == '\n');
    unsigned char *pixels = malloc((size_t)w * h * 3);
    assert(pixels);
    assert(fread(pixels, 3, (size_t)w * h, f) == (size_t)w * h);
    assert(!fclose(f));
    pixel(pixels, w, h, .1f, .1f, 200, 20, 20);
    pixel(pixels, w, h, .7f, .7f, 20, 180, 40);
    pixel(pixels, w, h, .5f, .5f, 20, 40, 200);
    pixel(pixels, w, h, .25f, .25f, 138, 218, 148);
    free(pixels);
    shapes(gpu);
    cui_draw_command malformed = {.type = CUI_DRAW_CLIP_POP};
    list = (cui_draw_list){&malformed, 1, 200, 200};
    assert(!cui_sdl_gpu_render(gpu, &list, (cui_color){0}));
    printf("SDL_GPU %s passed nested clip/alpha/readback validation\n", cui_sdl_gpu_driver(gpu));
    cui_sdl_gpu_destroy(gpu);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
