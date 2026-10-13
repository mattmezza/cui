/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl_gpu.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <math.h>
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
static void bitmap_pixels(cui_sdl_gpu *gpu) {
    unsigned char data[] = {255, 0, 0, 255, 0, 255, 0, 128};
    cui_draw_command c = {
        .type = CUI_DRAW_IMAGE,
        .bounds = {0, 0, 40, 20},
        .bitmap = {.bitmap = {data, 2, 1, 8, 1}, .source = {0, 0, 2, 1}, .opacity = 1}};
    cui_draw_list list = {&c, 1, 200, 200};
    for (int i = 0; i < 3; i++) {
        if (i == 2) {
            data[0] = 0;
            data[2] = 255;
            c.bitmap.bitmap.revision++;
        }
        assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0, 0, 0, 255}));
        unsigned w, h;
        unsigned char *p = capture(gpu, &w, &h);
        pixel(p, w, h, 5.5f / 200, 5.5f / 200, i == 2 ? 0 : 255, 0, i == 2 ? 255 : 0);
        pixel(p, w, h, 30.5f / 200, 5.5f / 200, 0, 128, 0);
        free(p);
    }
    c.bitmap.source = (cui_rect){1, 0, 1, 1};
    c.bitmap.grayscale = true;
    c.radius = 10;
    assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    unsigned char *p = capture(gpu, &w, &h);
    pixel(p, w, h, .5f / 200, .5f / 200, 0, 0, 0);
    pixel(p, w, h, 20.5f / 200, 10.5f / 200, 91, 91, 91);
    free(p);
}
static void mesh_pixels(cui_sdl_gpu *gpu) {
    const cui_mesh_vertex vertices[] = {{0, 0, 0, 1}, {80, 0, 0, 1}, {0, 80, 0, 1}};
    cui_draw_command c = {.type = CUI_DRAW_MESH,
                          .bounds = {20, 20, 80, 80},
                          .mesh = {.vertices = vertices,
                                   .vertex_count = 3,
                                   .origin_x = 20,
                                   .origin_y = 20,
                                   .scale = 1,
                                   .color = {0.10702057f, 0.10702057f, 0.10702057f, 0.5f}}};
    cui_draw_command overlap[] = {c, c};
    cui_draw_list list = {overlap, 2, 200, 200};
    assert(cui_sdl_gpu_interface(gpu).capabilities & CUI_RENDER_MESH);
    assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0, 0, 0, 255}));
    unsigned w, h;
    unsigned char *p = capture(gpu, &w, &h);
    pixel(p, w, h, .15f, .15f, 96, 96, 96);
    pixel(p, w, h, .05f, .15f, 0, 0, 0);
    free(p);
    cui_draw_command mixed[] = {
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 200, 200}, .color = {0, 0, 255, 255}},
        c,
        {.type = CUI_DRAW_RECT, .bounds = {20, 20, 10, 80}, .color = {255, 0, 0, 255}},
        c};
    list = (cui_draw_list){mixed, 4, 200, 200};
    assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0}));
    p = capture(gpu, &w, &h);
    pixel(p, w, h, .125f, .125f, 191, 64, 64);
    pixel(p, w, h, .2f, .15f, 96, 96, 160);
    free(p);
    cui_draw_command clips[] = {
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {30, 20, 10, 40}}, c, {.type = CUI_DRAW_CLIP_POP}};
    list = (cui_draw_list){clips, 3, 100, 100};
    clips[1].mesh.scale = .5f;
    assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0, 0, 0, 255}));
    p = capture(gpu, &w, &h);
    pixel(p, w, h, .35f, .25f, 64, 64, 64);
    pixel(p, w, h, .29f, .25f, 0, 0, 0);
    pixel(p, w, h, .41f, .25f, 0, 0, 0);
    free(p);
    const cui_mesh_vertex curve[] = {{20, 20, 0, 0}, {44, 20, .5f, 0}, {44, 44, 1, 1}};
    c.mesh =
        (cui_mesh_paint){.vertices = curve, .vertex_count = 3, .scale = 1, .color = {1, 1, 1, 1}};
    list = (cui_draw_list){&c, 1, 200, 200};
    assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0, 0, 0, 255}));
    p = capture(gpu, &w, &h);
    unsigned partial = 0, opaque = 0;
    for (size_t i = 0; i < (size_t)w * h; i++) {
        partial += p[i * 3] > 0 && p[i * 3] < 255;
        opaque += p[i * 3] == 255;
    }
    assert(partial > 0 && opaque > 0);
    for (unsigned y = 0; y < h; y++)
        for (unsigned x = 0; x < w; x++) {
            float lx = (x + .5f) * 200 / w, ly = (y + .5f) * 200 / h;
            if (lx <= 21 || lx >= 43 || ly <= 21 || ly >= lx - 1)
                continue;
            float st = ((lx - 20) + (ly - 20)) / 48, t = (ly - 20) / 24;
            float gradient = hypotf(st / 24 * 200 / w, (st - 1) / 24 * 200 / h);
            float coverage = fminf(1, fmaxf(0, .5f - (st * st - t) / gradient));
            assert(abs((int)p[((size_t)y * w + x) * 3] - (int)lroundf(coverage * 255)) <= 2);
        }
    free(p);
    const cui_mesh_vertex vertical[] = {{20, 20, 0, 0}, {44, 32, .5f, 0}, {20, 44, 1, 1}};
    c.mesh.vertices = vertical;
    assert(cui_sdl_gpu_render(gpu, &list, (cui_color){0, 0, 0, 255}));
    p = capture(gpu, &w, &h);
    partial = 0;
    for (unsigned y = 0; y < h; y++)
        for (unsigned x = 0; x < w; x++) {
            float lx = (x + .5f) * 200 / w, ly = (y + .5f) * 200 / h;
            if (lx <= 21 || ly <= 21 || ly >= 43 || lx - 20 >= 2 * fminf(ly - 20, 44 - ly) - 1)
                continue;
            float st = (ly - 20) / 24, t = st - (lx - 20) / 48;
            float gradient = hypotf(200.f / (48 * w), (2 * st - 1) * 200 / (24 * h));
            float coverage = fminf(1, fmaxf(0, .5f - (st * st - t) / gradient));
            int value = p[((size_t)y * w + x) * 3];
            assert(abs(value - (int)lroundf(coverage * 255)) <= 2);
            partial += value > 0 && value < 255;
        }
    assert(partial > 0);
    free(p);
    c.mesh.vertex_count = 2;
    assert(!cui_sdl_gpu_render(gpu, &list, (cui_color){255, 0, 0, 255}));
    p = capture(gpu, &w, &h);
    pixel(p, w, h, .01f, .01f, 0, 0, 0);
    free(p);
    size_t count = 32768u * 6u + 3u;
    cui_mesh_vertex *large = calloc(count, sizeof(*large));
    assert(large);
    for (size_t i = 0; i < count; i++)
        large[i] = vertices[i % 3];
    c.mesh.vertices = large;
    c.mesh.vertex_count = count;
    assert(!cui_sdl_gpu_render(gpu, &list, (cui_color){255, 0, 0, 255}));
    free(large);
    c.type = (cui_draw_type)999;
    c.mesh.vertex_count = 3;
    c.mesh.vertices = vertices;
    assert(!cui_sdl_gpu_render(gpu, &list, (cui_color){255, 0, 0, 255}));
    p = capture(gpu, &w, &h);
    pixel(p, w, h, .01f, .01f, 0, 0, 0);
    free(p);
    assert(vertices[1].x == 80 && vertices[1].s == 0);
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
    mesh_pixels(gpu);
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
    bitmap_pixels(gpu);
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
