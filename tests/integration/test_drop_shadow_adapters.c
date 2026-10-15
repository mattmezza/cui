/* SPDX-License-Identifier: MIT */
#include "cui/cui_effects.h"
#include "cui/cui_sdl3.h"
#include "cui/cui_sdl_gpu.h"
#include "cui/cui_sokol.h"
#include "cui/cui_theme.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned sdl_assertions;
static SDL_AssertState SDLCALL stop_assert(const SDL_AssertData *data, void *user) {
    (void)user;
    sdl_assertions++;
    fprintf(stderr, "SDL assertion: %s\n", data->condition);
    return SDL_ASSERTION_ABORT;
}
/* GNU/LLVM native fixture wrappers: capture before the real presentation.
 * Post-Present accelerated backbuffer pixels have no preservation contract. */
static SDL_Renderer *capture_renderer;
static SDL_Surface *completed_capture;
static unsigned presents, clears, draws;
bool __real_SDL_RenderPresent(SDL_Renderer *renderer);
bool __real_SDL_RenderClear(SDL_Renderer *renderer);
bool __real_SDL_RenderTexture(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_FRect *source,
                              const SDL_FRect *destination);
bool __real_SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_FRect *rect);
bool __wrap_SDL_RenderClear(SDL_Renderer *renderer) {
    if (renderer == capture_renderer)
        clears++;
    return __real_SDL_RenderClear(renderer);
}
bool __wrap_SDL_RenderTexture(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_FRect *source,
                              const SDL_FRect *destination) {
    if (renderer == capture_renderer)
        draws++;
    return __real_SDL_RenderTexture(renderer, texture, source, destination);
}
bool __wrap_SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_FRect *rect) {
    if (renderer == capture_renderer)
        draws++;
    return __real_SDL_RenderFillRect(renderer, rect);
}
bool __real_SDL_RenderFillRects(SDL_Renderer *renderer, const SDL_FRect *rects, int count);
bool __wrap_SDL_RenderFillRects(SDL_Renderer *renderer, const SDL_FRect *rects, int count) {
    if (renderer == capture_renderer)
        draws++;
    return __real_SDL_RenderFillRects(renderer, rects, count);
}
bool __wrap_SDL_RenderPresent(SDL_Renderer *renderer) {
    if (renderer != capture_renderer)
        return __real_SDL_RenderPresent(renderer);
    SDL_Surface *surface = SDL_RenderReadPixels(renderer, NULL);
    assert(surface);
    SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(surface);
    assert(rgba);
    SDL_DestroySurface(completed_capture);
    completed_capture = rgba;
    presents++;
    bool ok = __real_SDL_RenderPresent(renderer);
    assert(ok);
    /* Diagnostic only: intentionally do not assert post-Present pixel values. */
    surface = SDL_RenderReadPixels(renderer, NULL);
    SDL_Surface *after = surface ? SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32) : NULL;
    float sx, sy;
    assert(SDL_GetRenderScale(renderer, &sx, &sy));
    unsigned x = (unsigned)(18 * sx), y = (unsigned)(28 * sy);
    if (x < (unsigned)rgba->w && y < (unsigned)rgba->h) {
        const unsigned char *before =
            (unsigned char *)rgba->pixels + (size_t)y * rgba->pitch + x * 4;
        if (after && x < (unsigned)after->w && y < (unsigned)after->h) {
            const unsigned char *post =
                (unsigned char *)after->pixels + (size_t)y * after->pitch + x * 4;
            printf("SDL scale %.0f pre/post-Present RGBA: %u,%u,%u,%u / %u,%u,%u,%u\n", sx,
                   before[0], before[1], before[2], before[3], post[0], post[1], post[2], post[3]);
        } else
            puts("SDL post-Present readback unavailable (diagnostic only)");
    }
    SDL_DestroySurface(after);
    SDL_DestroySurface(surface);
    return ok;
}
static SDL_Surface *last_completed(SDL_Renderer *renderer) {
    assert(renderer == capture_renderer && completed_capture);
    return SDL_DuplicateSurface(completed_capture);
}
static cui_draw_command commands[4];
static cui_draw_list list = {commands, 4, 64, 64};
static void setup(void) {
    cui_drop_shadow layers[3];
    cui_theme t = cui_theme_light();
    assert(cui_theme_popover_shadows(&t, 1, layers));
    for (unsigned i = 0; i < 3; i++)
        commands[i] = (cui_draw_command){
            .type = CUI_DRAW_DROP_SHADOW,
            .shadow = {.caster = {20, 20, 24, 16}, .radii = {4, 4, 4, 4}, .layer = layers[i]}};
    commands[3] = (cui_draw_command){.type = CUI_DRAW_RECT,
                                     .bounds = {20, 20, 24, 16},
                                     .color = {200, 220, 240, 128},
                                     .radius = 4};
}
static void check(const unsigned char *p, unsigned w, unsigned h, unsigned channels, bool upside) {
    const unsigned points[][2] = {{18, 28}, {21, 28}, {30, 30}, {30, 38}, {30, 41}, {10, 10}};
    float scale = w / 64.0f;
    assert(w == h);
    for (unsigned i = 0; i < sizeof(points) / sizeof(points[0]); i++) {
        unsigned x = points[i][0] * (w / 64), y = points[i][1] * (h / 64);
        float expected[3] = {255, 255, 255};
        for (unsigned j = 0; j < 3; j++) {
            float alpha;
            assert(cui_effect_drop_shadow_sample(commands[j].shadow, scale, 0, 0, x + .5f, y + .5f,
                                                 &alpha));
            alpha = lroundf(alpha * 255) / 255.0f * commands[j].shadow.layer.color.a / 255.0f;
            const cui_color c = commands[j].shadow.layer.color;
            const unsigned rgb[] = {c.r, c.g, c.b};
            for (unsigned k = 0; k < 3; k++)
                expected[k] = rgb[k] * alpha + expected[k] * (1 - alpha);
        }
        if (points[i][0] >= 21 && points[i][0] < 44 && points[i][1] >= 24 && points[i][1] < 36)
            for (unsigned k = 0; k < 3; k++) {
                const unsigned rgb[] = {200, 220, 240};
                expected[k] = rgb[k] * (128 / 255.0f) + expected[k] * (127 / 255.0f);
            }
        const unsigned char *actual = p + ((size_t)(upside ? h - 1 - y : y) * w + x) * channels;
        for (unsigned k = 0; k < 3; k++)
            if (abs((int)actual[k] - (int)lroundf(expected[k])) > 2) {
                fprintf(stderr, "pixel %u,%u channel%u got%u expected%.2f\n", x, y, k, actual[k],
                        expected[k]);
                assert(0);
            }
    }
}
static void fractional_pixel(const unsigned char *p, size_t pitch, unsigned channels,
                             unsigned height, bool upside, unsigned scale) {
    unsigned x = (unsigned)floorf(18.25f * scale), y = (unsigned)floorf(23.25f * scale);
    float expected[3] = {255, 255, 255};
    for (unsigned i = 0; i < 3; i++) {
        float a;
        assert(cui_effect_drop_shadow_sample(commands[i].shadow, (float)scale, 0, 0, x + .5f,
                                             y + .5f, &a));
        a = lroundf(a * 255) / 255.0f * commands[i].shadow.layer.color.a / 255.0f;
        const cui_color color = commands[i].shadow.layer.color;
        const unsigned rgb[] = {color.r, color.g, color.b};
        for (unsigned k = 0; k < 3; k++)
            expected[k] = rgb[k] * a + expected[k] * (1 - a);
    }
    const unsigned char *actual = p + (size_t)(upside ? height - 1 - y : y) * pitch + x * channels;
    for (unsigned k = 0; k < 3; k++)
        assert(abs((int)actual[k] - (int)lroundf(expected[k])) <= 2);
    const unsigned char *outside = p + (size_t)(upside ? height - 1 - y : y) * pitch +
                                   (unsigned)ceilf(21.25f * scale) * channels;
    assert(outside[0] == 255 && outside[1] == 255 && outside[2] == 255);
}
static cui_draw_list masked(cui_draw_command out[6], float width, float height) {
    out[0] = (cui_draw_command){.type = CUI_DRAW_CLIP_PUSH, .bounds = {18.25f, 22.25f, 3, 5}};
    memcpy(out + 1, commands, sizeof(commands));
    out[5] = (cui_draw_command){.type = CUI_DRAW_CLIP_POP};
    return (cui_draw_list){out, 6, width, height};
}
static cui_draw_command covered_collapse(void) {
    return (cui_draw_command){.type = CUI_DRAW_DROP_SHADOW,
                              .shadow = {.caster = {20.25f, 20.25f, 1, 1},
                                         .layer = {.spread = -.6f, .color = {0, 0, 0, 255}}}};
}
static void collapse_pixel(const unsigned char *p, size_t pitch, unsigned channels, unsigned height,
                           bool upside, unsigned scale) {
    unsigned x = scale == 1 ? 20 : 41, y = x;
    unsigned row = upside ? height - 1 - y : y;
    const unsigned char *a = p + row * pitch + x * channels;
    const unsigned char *b = a + channels;
    assert(a[0] == 0 && a[1] == 0 && a[2] == 0);
    assert(b[0] == 255 && b[1] == 255 && b[2] == 255);
}
static void sdl_renderer(void) {
    cui_sdl3 *r = cui_sdl3_create("shadow SDL readback", 128, 128, NULL);
    assert(r);
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    SDL_Window *window = NULL;
    for (int i = 0; i < count; i++)
        if (!strcmp(SDL_GetWindowTitle(windows[i]), "shadow SDL readback"))
            window = windows[i];
    SDL_free(windows);
    assert(window);
    SDL_Renderer *renderer = SDL_GetRenderer(window);
    assert(renderer);
    capture_renderer = renderer;
    printf("SDL Renderer driver: %s\n", cui_sdl3_driver(r));
    for (unsigned scale = 1; scale <= 2; scale++) {
        printf("SDL scale%u\n", scale);
        assert(SDL_SetRenderScale(renderer, (float)scale, (float)scale));
        assert(cui_sdl3_render(r, &list, (cui_color){255, 255, 255, 255}));
        SDL_Surface *surface = last_completed(renderer);
        assert(surface);
        SDL_Surface *rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        assert(rgba);
        // Scale1 uses full128 target but logical frame64; check central64 crop.
        if (scale == 1) {
            unsigned char crop[64 * 64 * 4];
            for (unsigned y = 0; y < 64; y++)
                memcpy(crop + y * 64 * 4, (unsigned char *)rgba->pixels + (size_t)y * rgba->pitch,
                       64 * 4);
            check(crop, 64, 64, 4, false);
        } else {
            unsigned char crop[128 * 128 * 4];
            for (unsigned y = 0; y < 128; y++)
                memcpy(crop + y * 128 * 4, (unsigned char *)rgba->pixels + (size_t)y * rgba->pitch,
                       128 * 4);
            check(crop, 128, 128, 4, false);
        }
        SDL_DestroySurface(rgba);
        SDL_DestroySurface(surface);
        cui_draw_command collapse = covered_collapse();
        cui_draw_list one = {&collapse, 1, list.width, list.height};
        assert(cui_sdl3_render(r, &one, (cui_color){255, 255, 255, 255}));
        surface = last_completed(renderer);
        assert(surface);
        rgba = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        assert(rgba);
        collapse_pixel(rgba->pixels, (size_t)rgba->pitch, 4, (unsigned)rgba->h, false, scale);
        SDL_DestroySurface(rgba);
        SDL_DestroySurface(surface);
    }
    cui_draw_command mask[6];
    cui_draw_list clipped = masked(mask, 64, 64);
    assert(cui_sdl3_render(r, &clipped, (cui_color){255, 255, 255, 255}));
    SDL_Surface *capture = last_completed(renderer);
    assert(capture);
    SDL_Surface *rgba = SDL_ConvertSurface(capture, SDL_PIXELFORMAT_RGBA32);
    assert(rgba);
    fractional_pixel(rgba->pixels, (size_t)rgba->pitch, 4, (unsigned)rgba->h, false, 2);
    size_t old_bytes = (size_t)rgba->w * rgba->h * 4;
    unsigned char *old = malloc(old_bytes);
    assert(old);
    for (int y = 0; y < rgba->h; y++)
        memcpy(old + (size_t)y * rgba->w * 4,
               (unsigned char *)rgba->pixels + (size_t)y * rgba->pitch, (size_t)rgba->w * 4);
    SDL_DestroySurface(rgba);
    SDL_DestroySurface(capture);
    assert(SDL_SetRenderScale(renderer, 1, 2));
    unsigned old_presents = presents, old_clears = clears, old_draws = draws;
    assert(!cui_sdl3_render(r, &list, (cui_color){0}));
    assert(presents == old_presents && clears == old_clears && draws == old_draws);
    capture = last_completed(renderer);
    assert(capture);
    rgba = SDL_ConvertSurface(capture, SDL_PIXELFORMAT_RGBA32);
    assert(rgba && (size_t)rgba->w * rgba->h * 4 == old_bytes);
    for (int y = 0; y < rgba->h; y++)
        assert(!memcmp(old + (size_t)y * rgba->w * 4,
                       (unsigned char *)rgba->pixels + (size_t)y * rgba->pitch,
                       (size_t)rgba->w * 4));
    free(old);
    SDL_DestroySurface(rgba);
    SDL_DestroySurface(capture);
    /* Legacy frames may stream more images than fit in the retained cache. */
    assert(SDL_SetRenderScale(renderer, 1, 1));
    unsigned char image_pixels[65][4];
    cui_draw_command images[65];
    for (unsigned i = 0; i < 65; i++) {
        image_pixels[i][0] = (unsigned char)i;
        image_pixels[i][1] = 100;
        image_pixels[i][2] = 200;
        image_pixels[i][3] = 255;
        images[i] = (cui_draw_command){.type = CUI_DRAW_IMAGE,
                                       .bounds = {0, 0, 1, 1},
                                       .bitmap = {.bitmap = {image_pixels[i], 1, 1, 4, 1},
                                                  .source = {0, 0, 1, 1},
                                                  .opacity = 1}};
    }
    cui_draw_list ordinary = {images, 65, 64, 64};
    assert(cui_sdl3_render(r, &ordinary, (cui_color){255, 255, 255, 255}));
    capture = last_completed(renderer);
    assert(capture);
    rgba = SDL_ConvertSurface(capture, SDL_PIXELFORMAT_RGBA32);
    assert(rgba);
    const unsigned char *last = rgba->pixels;
    assert(last[0] == 64 && last[1] == 100 && last[2] == 200 && last[3] == 255);
    SDL_DestroySurface(rgba);
    SDL_DestroySurface(capture);
    SDL_DestroySurface(completed_capture);
    completed_capture = NULL;
    capture_renderer = NULL;
    cui_sdl3_destroy(r);
    puts("actual SDL Renderer shadow readback scales1/2 passed");
}
static void gpu(void) {
    SDL_Window *w = SDL_CreateWindow("shadow GPU", 128, 128, 0);
    assert(w);
    SDL_GPUDevice *d = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    assert(d && SDL_ClaimWindowForGPUDevice(d, w));
    printf("SDL GPU driver: %s\n", SDL_GetGPUDeviceDriver(d));
    cui_sdl_gpu *r = cui_sdl_gpu_create(d, w, NULL);
    assert(r);
    for (unsigned scale = 1; scale <= 2; scale++) {
        int pw, ph;
        assert(SDL_GetWindowSizeInPixels(w, &pw, &ph));
        list.width = (float)pw / scale;
        list.height = (float)ph / scale;
        assert(cui_sdl_gpu_render(r, &list, (cui_color){255, 255, 255, 255}));
        assert(cui_sdl_gpu_save_ppm(r, "shadow-gpu.ppm"));
        FILE *f = fopen("shadow-gpu.ppm", "rb");
        assert(f);
        char magic[3];
        unsigned width, height, max;
        assert(fscanf(f, "%2s %u %u %u", magic, &width, &height, &max) == 4 && fgetc(f) == '\n');
        unsigned char *p = malloc((size_t)width * height * 3);
        assert(p && fread(p, 3, (size_t)width * height, f) == (size_t)width * height);
        fclose(f);
        if (scale == 1) {
            unsigned char crop[64 * 64 * 3];
            for (unsigned y = 0; y < 64; y++)
                memcpy(crop + y * 64 * 3, p + y * width * 3, 64 * 3);
            check(crop, 64, 64, 3, false);
        } else {
            unsigned char crop[128 * 128 * 3];
            for (unsigned y = 0; y < 128; y++)
                memcpy(crop + y * 128 * 3, p + y * width * 3, 128 * 3);
            check(crop, 128, 128, 3, false);
        }
        free(p);
        cui_draw_command collapse = covered_collapse();
        cui_draw_list one = {&collapse, 1, list.width, list.height};
        assert(cui_sdl_gpu_render(r, &one, (cui_color){255, 255, 255, 255}));
        assert(cui_sdl_gpu_save_ppm(r, "shadow-collapse-gpu.ppm"));
        f = fopen("shadow-collapse-gpu.ppm", "rb");
        assert(f);
        assert(fscanf(f, "%2s %u %u %u", magic, &width, &height, &max) == 4 && fgetc(f) == '\n');
        p = malloc((size_t)width * height * 3);
        assert(p && fread(p, 3, (size_t)width * height, f) == (size_t)width * height);
        fclose(f);
        collapse_pixel(p, (size_t)width * 3, 3, height, false, scale);
        free(p);
    }
    cui_draw_command mask[6];
    cui_draw_list clipped = masked(mask, list.width, list.height);
    assert(cui_sdl_gpu_render(r, &clipped, (cui_color){255, 255, 255, 255}));
    assert(cui_sdl_gpu_save_ppm(r, "shadow-mask-gpu.ppm"));
    FILE *f = fopen("shadow-mask-gpu.ppm", "rb");
    assert(f);
    char magic[3];
    unsigned mw, mh, max;
    assert(fscanf(f, "%2s %u %u %u", magic, &mw, &mh, &max) == 4 && fgetc(f) == '\n');
    unsigned char *data = malloc((size_t)mw * mh * 3);
    assert(data && fread(data, 3, (size_t)mw * mh, f) == (size_t)mw * mh);
    fclose(f);
    fractional_pixel(data, (size_t)mw * 3, 3, mh, false, 2);
    list.width = 64;
    list.height = 32;
    assert(!cui_sdl_gpu_render(r, &list, (cui_color){0}));
    assert(cui_sdl_gpu_save_ppm(r, "shadow-rejected-gpu.ppm"));
    f = fopen("shadow-rejected-gpu.ppm", "rb");
    assert(f);
    unsigned rw, rh;
    assert(fscanf(f, "%2s %u %u %u", magic, &rw, &rh, &max) == 4 && fgetc(f) == '\n' && rw == mw &&
           rh == mh);
    unsigned char *retry = malloc((size_t)rw * rh * 3);
    assert(retry && fread(retry, 3, (size_t)rw * rh, f) == (size_t)rw * rh);
    fclose(f);
    assert(!memcmp(data, retry, (size_t)rw * rh * 3));
    free(data);
    free(retry);
    list.width = list.height = 64;
    cui_sdl_gpu_destroy(r);
    SDL_ReleaseWindowFromGPUDevice(d, w);
    SDL_DestroyGPUDevice(d);
    SDL_DestroyWindow(w);
    puts("actual SDL GPU shadow readback scales1/2 passed");
}
static void sokol(void) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window *w =
        SDL_CreateWindow("shadow Sokol", 128, 128, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(w);
    SDL_GLContext gl = SDL_GL_CreateContext(w);
    assert(gl);
    printf("Sokol GL vendor: %s; renderer: %s; version: %s\n", glGetString(GL_VENDOR),
           glGetString(GL_RENDERER), glGetString(GL_VERSION));
    GLuint fbo, texture;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    cui_sokol *r =
        cui_sokol_create((cui_sokol_config){.max_quads = 256, .target = {fbo, 128, 128}});
    assert(r);
    unsigned char pixels[128 * 128 * 4];
    for (unsigned scale = 1; scale <= 2; scale++) {
        list.width = list.height = 128.0f / scale;
        assert(cui_sokol_render(r, &list, (cui_color){255, 255, 255, 255}));
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        assert(glGetError() == GL_NO_ERROR);
        if (scale == 1) {
            unsigned char crop[64 * 64 * 4];
            for (unsigned y = 0; y < 64; y++)
                memcpy(crop + (63 - y) * 64 * 4, pixels + (127 - y) * 128 * 4, 64 * 4);
            check(crop, 64, 64, 4, true);
        } else
            check(pixels, 128, 128, 4, true);
        cui_draw_command collapse = covered_collapse();
        cui_draw_list one = {&collapse, 1, list.width, list.height};
        assert(cui_sokol_render(r, &one, (cui_color){255, 255, 255, 255}));
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        assert(glGetError() == GL_NO_ERROR);
        collapse_pixel(pixels, 128 * 4, 4, 128, true, scale);
    }
    cui_draw_command mask[6];
    cui_draw_list clipped = masked(mask, 64, 64);
    assert(cui_sokol_render(r, &clipped, (cui_color){255, 255, 255, 255}));
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    assert(glGetError() == GL_NO_ERROR);
    fractional_pixel(pixels, 128 * 4, 4, 128, true, 2);
    list.width = 64;
    list.height = 32;
    assert(!cui_sokol_render(r, &list, (cui_color){0}));
    unsigned char retained[sizeof(pixels)];
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, retained);
    assert(glGetError() == GL_NO_ERROR && !memcmp(pixels, retained, sizeof(pixels)));
    list.width = list.height = 64;
    cui_sokol_destroy(r);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(w);
    puts("actual Sokol GLCORE shadow readback scales1/2 passed");
}
int main(void) {
    setbuf(stdout, NULL);
    SDL_SetAssertionHandler(stop_assert, NULL);
    assert(SDL_Init(SDL_INIT_VIDEO));
    setup();
    sdl_renderer();
    gpu();
    sokol();
    assert(!sdl_assertions);
    SDL_Quit();
    return 0;
}
