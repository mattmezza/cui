/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "cui/cui_sdl_gpu.h"
#include "cui/cui_software.h"
#include "cui/cui_sokol.h"
#include "tabs_source_shadows_fixture.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static SDL_AssertState SDLCALL abort_assert(const SDL_AssertData *data, void *user) {
    (void)user;
    fprintf(stderr, "SDL assertion: %s\n", data->condition);
    return SDL_ASSERTION_ABORT;
}
static unsigned char *ppm(const char *path, unsigned *width, unsigned *height) {
    FILE *f = fopen(path, "rb");
    assert(f);
    char magic[3];
    unsigned max;
    assert(fscanf(f, "%2s%u%u%u", magic, width, height, &max) == 4);
    assert(!strcmp(magic, "P6") && max == 255 && fgetc(f) == '\n');
    unsigned char *p = malloc((size_t)*width * *height * 3);
    assert(p);
    assert(fread(p, 3, (size_t)*width * *height, f) == (size_t)*width * *height);
    fclose(f);
    return p;
}
static void compare(const unsigned char *actual, size_t pitch, unsigned channels, bool upside,
                    const unsigned char *expected, unsigned scale, cui_rect caster) {
    unsigned width = 128 * scale;
    /* Interior and outside-caster shadow samples avoid platform corner AA. */
    const unsigned points[][2] = {
        {(unsigned)(caster.x + caster.w * .5f), (unsigned)(caster.y + caster.h * .5f)},
        {(unsigned)(caster.x + caster.w * .5f), (unsigned)(caster.y + caster.h - 2)},
        {(unsigned)(caster.x + caster.w * .5f), (unsigned)(caster.y + caster.h + 1)},
        {(unsigned)fmaxf(0, caster.x - 2), (unsigned)(caster.y + caster.h * .5f)},
        {10, 10}};
    for (size_t i = 0; i < sizeof(points) / sizeof(points[0]); ++i) {
        unsigned x = points[i][0] * scale, y = points[i][1] * scale;
        const unsigned char *a = actual + (upside ? width - 1 - y : y) * pitch + x * channels;
        const unsigned char *e = expected + ((size_t)y * width + x) * 4;
        for (unsigned k = 0; k < 3; ++k) {
            if (abs((int)a[k] - e[k]) > 3)
                fprintf(stderr, "sample%zu channel%u %u vs %u scale%u\n", i, k, a[k], e[k], scale);
            assert(abs((int)a[k] - e[k]) <= 3);
        }
    }
}
int main(void) {
    SDL_SetAssertionHandler(abort_assert, NULL);
    assert(SDL_Init(SDL_INIT_VIDEO));
    cui_sdl3 *sdl = cui_sdl3_create("Tabs source readback", 256, 256, NULL);
    assert(sdl);
    int window_count;
    SDL_Window **windows = SDL_GetWindows(&window_count);
    SDL_Renderer *sdl_renderer = NULL;
    for (int i = 0; i < window_count; ++i)
        if (!strcmp(SDL_GetWindowTitle(windows[i]), "Tabs source readback"))
            sdl_renderer = SDL_GetRenderer(windows[i]);
    SDL_free(windows);
    assert(sdl_renderer);
    SDL_Window *gpu_window = SDL_CreateWindow("Tabs GPU", 256, 256, SDL_WINDOW_UTILITY);
    assert(gpu_window);
    assert(SDL_SetWindowMinimumSize(gpu_window, 256, 256));
    assert(SDL_SetWindowMaximumSize(gpu_window, 256, 256));
    assert(SDL_SetWindowSize(gpu_window, 256, 256));
    assert(SDL_SyncWindow(gpu_window));
    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    assert(device && SDL_ClaimWindowForGPUDevice(device, gpu_window));
    cui_sdl_gpu *gpu = cui_sdl_gpu_create(device, gpu_window, NULL);
    assert(gpu);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window *gl_window =
        SDL_CreateWindow("Tabs Sokol", 256, 256, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(gl_window);
    SDL_GLContext gl = SDL_GL_CreateContext(gl_window);
    assert(gl);
    GLuint fbo, texture;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    cui_sokol *sokol =
        cui_sokol_create((cui_sokol_config){.max_quads = 256, .target = {fbo, 256, 256}});
    assert(sokol);
    unsigned frames = 0;
    for (unsigned dark = 0; dark < 2; ++dark)
        for (unsigned transparent = 0; transparent < 2; ++transparent)
            for (unsigned scale = 1; scale <= 2; ++scale)
                for (unsigned mode = 0; mode < 5; ++mode) {
                    cui_draw_command copied[64];
                    cui_rect caster;
                    cui_draw_list list =
                        tabs_shadow_fixture(dark, mode, transparent != 0, copied, &caster);
                    /* Physical target256; logical dimensions choose exact DPR. */
                    list.width = list.height = 256.0f / scale;
                    cui_software *soft = cui_software_create(256, 256, NULL);
                    assert(soft);
                    cui_draw_command physical[64];
                    memcpy(physical, copied, list.count * sizeof(*copied));
                    for (size_t i = 0; i < list.count; ++i) {
                        cui_draw_command *c = &physical[i];
                        c->bounds.x *= scale;
                        c->bounds.y *= scale;
                        c->bounds.w *= scale;
                        c->bounds.h *= scale;
                        c->radius *= scale;
                        c->stroke *= scale;
                        if (c->type == CUI_DRAW_DROP_SHADOW) {
                            c->shadow.caster.x *= scale;
                            c->shadow.caster.y *= scale;
                            c->shadow.caster.w *= scale;
                            c->shadow.caster.h *= scale;
                            c->shadow.radii.top_left *= scale;
                            c->shadow.radii.top_right *= scale;
                            c->shadow.radii.bottom_left *= scale;
                            c->shadow.radii.bottom_right *= scale;
                            c->shadow.layer.offset_x *= scale;
                            c->shadow.layer.offset_y *= scale;
                            c->shadow.layer.sigma *= scale;
                            c->shadow.layer.spread *= scale;
                        }
                    }
                    cui_draw_list physical_list = {physical, list.count, 256, 256};
                    assert(
                        cui_software_render(soft, &physical_list, (cui_color){255, 255, 255, 255}));
                    const unsigned char *expected = cui_software_pixels(soft, NULL, NULL);
                    /* Software uses target/frame scale; framebuffer stays256 at both DPRs. */
                    assert(SDL_SetRenderScale(sdl_renderer, (float)scale, (float)scale));
                    assert(cui_sdl3_render_capture(sdl, &list, (cui_color){255, 255, 255, 255},
                                                   "tabs-sdl.bmp"));
                    SDL_Surface *bmp = SDL_LoadBMP("tabs-sdl.bmp");
                    assert(bmp);
                    SDL_Surface *rgba = SDL_ConvertSurface(bmp, SDL_PIXELFORMAT_RGBA32);
                    assert(rgba);
                    /* Sample pitch and upside require physical256, handled below via normalized
                     * crop.
                     */
                    unsigned char crop[256 * 256 * 4];
                    unsigned logical_pixels = 128 * scale;
                    for (unsigned y = 0; y < logical_pixels; ++y)
                        memcpy(crop + (size_t)y * logical_pixels * 4,
                               (unsigned char *)rgba->pixels + (size_t)y * rgba->pitch,
                               (size_t)logical_pixels * 4);
                    unsigned char reference[256 * 256 * 4];
                    for (unsigned y = 0; y < logical_pixels; ++y)
                        memcpy(reference + (size_t)y * logical_pixels * 4,
                               expected + (size_t)y * 256 * 4, (size_t)logical_pixels * 4);
                    compare(crop, logical_pixels * 4, 4, false, reference, scale, caster);
                    SDL_DestroySurface(rgba);
                    SDL_DestroySurface(bmp);
                    if (!cui_sdl_gpu_render(gpu, &list, (cui_color){255, 255, 255, 255})) {
                        fprintf(stderr, "GPU error: %s count%zu\n", SDL_GetError(), list.count);
                        for (size_t i = 0; i < list.count; ++i)
                            fprintf(stderr, "cmd%zu type%d %.1f %.1f %.1f %.1f\n", i,
                                    (int)list.commands[i].type, list.commands[i].bounds.x,
                                    list.commands[i].bounds.y, list.commands[i].bounds.w,
                                    list.commands[i].bounds.h);
                        abort();
                    }
                    assert(cui_sdl_gpu_save_ppm(gpu, "tabs-gpu.ppm"));
                    unsigned w, h;
                    unsigned char *p = ppm("tabs-gpu.ppm", &w, &h);
                    assert(w == 256 && h == 256);
                    compare(p, w * 3, 3, false, reference, scale, caster);
                    free(p);
                    assert(SDL_GL_MakeCurrent(gl_window, gl));
                    assert(cui_sokol_render(sokol, &list, (cui_color){255, 255, 255, 255}));
                    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
                    glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, crop);
                    assert(glGetError() == GL_NO_ERROR);
                    /* Crop from top in GL's reversed row order. */
                    unsigned char top[256 * 256 * 4];
                    for (unsigned y = 0; y < logical_pixels; ++y)
                        memcpy(top + (size_t)y * logical_pixels * 4,
                               crop + (size_t)(255 - y) * 256 * 4, (size_t)logical_pixels * 4);
                    compare(top, logical_pixels * 4, 4, false, reference, scale, caster);
                    cui_software_destroy(soft);
                    ++frames;
                }
    cui_sokol_destroy(sokol);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(gl_window);
    cui_sdl_gpu_destroy(gpu);
    SDL_ReleaseWindowFromGPUDevice(device, gpu_window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(gpu_window);
    cui_sdl3_destroy(sdl);
    SDL_Quit();
    printf("PASS actual copied Tabs Software/SDL/GPU/Sokol DPR1/2 %u frames\n", frames);
    return 0;
}
