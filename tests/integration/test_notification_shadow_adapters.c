/* SPDX-License-Identifier: MIT */
#define GL_GLEXT_PROTOTYPES 1
#define main drop_compatibility_main
#include "test_drop_shadow_adapters.c"
#undef main
#include "../unit/notification_shadow_fixture.h"
static void notification_pixels(const unsigned char *pixels, size_t pitch, unsigned channels,
                                unsigned height, bool upside, unsigned scale, cui_theme theme) {
    const unsigned points[][2] = {{32, 42}, {14, 28}, {50, 28}, {32, 47}};
    for (size_t i = 0; i < 4; i++) {
        unsigned x = points[i][0] * scale, y = points[i][1] * scale;
        float expected = 255;
        const cui_drop_shadow source[] = {
            {.offset_y = 10, .sigma = 7.5f, .spread = -3, .color = {0, 0, 0, 26}},
            {.offset_y = 4, .sigma = 3, .spread = -4, .color = {0, 0, 0, 26}}};
        for (size_t layer = 0; layer < 2; layer++) {
            float a;
            assert(cui_effect_drop_shadow_sample(
                (cui_drop_shadow_paint){
                    .caster = {16, 16, 32, 24},
                    .radii = {theme.radius_lg, theme.radius_lg, theme.radius_lg, theme.radius_lg},
                    .layer = source[layer]},
                scale, 0, 0, x + .5f, y + .5f, &a));
            expected *= 1 - a * 26 / 255;
        }
        const unsigned char *p = pixels + (upside ? height - 1 - y : y) * pitch + x * channels;
        for (size_t k = 0; k < 3; k++)
            assert(fabsf(p[k] - expected) <= 2);
    }
    unsigned row = 28 * scale;
    if (upside)
        row = height - 1 - row;
    const unsigned char *center = pixels + row * pitch + 32 * scale * channels;
    assert(center[0] == theme.popover.r && center[1] == theme.popover.g &&
           center[2] == theme.popover.b);
}
static unsigned char *notification_ppm(const char *path, unsigned *w, unsigned *h) {
    FILE *file = fopen(path, "rb");
    assert(file);
    char magic[3];
    unsigned max;
    assert(fscanf(file, "%2s %u %u %u", magic, w, h, &max) == 4 && fgetc(file) == '\n');
    unsigned char *p = malloc((size_t)*w * *h * 3);
    assert(p && fread(p, 3, (size_t)*w * *h, file) == (size_t)*w * *h);
    fclose(file);
    return p;
}
static void notification_renderer(void) {
    cui_sdl3 *r = cui_sdl3_create("notification SDL", 128, 128, NULL);
    assert(r);
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    SDL_Renderer *renderer = NULL;
    for (int i = 0; i < count; i++)
        if (!strcmp(SDL_GetWindowTitle(windows[i]), "notification SDL"))
            renderer = SDL_GetRenderer(windows[i]);
    SDL_free(windows);
    assert(renderer);
    capture_renderer = renderer;
    printf("Notification SDL Renderer: %s\n", SDL_GetRendererName(renderer));
    for (int dark = 0; dark < 2; dark++)
        for (unsigned scale = 1; scale <= 2; scale++) {
            notification_fixture f;
            notification_fixture_init(&f, dark, CUI_NOTIFICATION_SHADOW_SOURCE);
            cui_draw_list list =
                notification_fixture_frame(&f, 128.0f / scale, 128.0f / scale, 1, 1);
            assert(SDL_SetRenderScale(renderer, scale, scale));
            assert(cui_sdl3_render(r, &list, (cui_color){255, 255, 255, 255}));
            notification_pixels(completed_capture->pixels, completed_capture->pitch, 4,
                                completed_capture->h, false, scale, f.theme);
            unsigned old_clear = clears, old_present = presents, old_draw = draws;
            cui_draw_command copied[1024];
            memcpy(copied, list.commands, list.count * sizeof(*copied));
            for (size_t i = 0; i < list.count; i++)
                if (copied[i].type == CUI_DRAW_DROP_SHADOW) {
                    copied[i].shadow.layer.sigma = NAN;
                    break;
                }
            list.commands = copied;
            assert(!cui_sdl3_render(r, &list, (cui_color){0}));
            assert(clears == old_clear && presents == old_present && draws == old_draw);
            cui_destroy(f.ui);
        }
    SDL_DestroySurface(completed_capture);
    completed_capture = NULL;
    capture_renderer = NULL;
    cui_sdl3_destroy(r);
}
static void notification_gpu(void) {
    SDL_Window *window = SDL_CreateWindow("notification GPU", 128, 128, 0);
    assert(window);
    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    assert(device && SDL_ClaimWindowForGPUDevice(device, window));
    printf("Notification SDL GPU: %s\n", SDL_GetGPUDeviceDriver(device));
    cui_sdl_gpu *r = cui_sdl_gpu_create(device, window, NULL);
    assert(r);
    for (int dark = 0; dark < 2; dark++)
        for (unsigned scale = 1; scale <= 2; scale++) {
            int pw, ph;
            assert(SDL_GetWindowSizeInPixels(window, &pw, &ph));
            notification_fixture f;
            notification_fixture_init(&f, dark, CUI_NOTIFICATION_SHADOW_SOURCE);
            cui_draw_list list =
                notification_fixture_frame(&f, (float)pw / scale, (float)ph / scale, 1, 1);
            assert(cui_sdl_gpu_render(r, &list, (cui_color){255, 255, 255, 255}));
            assert(cui_sdl_gpu_save_ppm(r, "notification-gpu.ppm"));
            unsigned w, h;
            unsigned char *p = notification_ppm("notification-gpu.ppm", &w, &h);
            notification_pixels(p, (size_t)w * 3, 3, h, false, scale, f.theme);
            cui_draw_command copied[1024];
            memcpy(copied, list.commands, list.count * sizeof(*copied));
            for (size_t i = 0; i < list.count; i++)
                if (copied[i].type == CUI_DRAW_DROP_SHADOW) {
                    copied[i].shadow.layer.sigma = NAN;
                    break;
                }
            list.commands = copied;
            assert(!cui_sdl_gpu_render(r, &list, (cui_color){0}));
            assert(cui_sdl_gpu_save_ppm(r, "notification-gpu-rejected.ppm"));
            unsigned rw, rh;
            unsigned char *retained = notification_ppm("notification-gpu-rejected.ppm", &rw, &rh);
            assert(rw == w && rh == h && !memcmp(p, retained, (size_t)w * h * 3));
            free(retained);
            free(p);
            cui_destroy(f.ui);
        }
    cui_sdl_gpu_destroy(r);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
}
static void notification_sokol(void) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window *window =
        SDL_CreateWindow("notification Sokol", 128, 128, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    assert(context);
    printf("Notification Sokol: %s / %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    GLuint fbo, texture;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    cui_sokol *r =
        cui_sokol_create((cui_sokol_config){.max_quads = 1024, .target = {fbo, 128, 128}});
    assert(r);
    unsigned char pixels[128 * 128 * 4];
    for (int dark = 0; dark < 2; dark++)
        for (unsigned scale = 1; scale <= 2; scale++) {
            notification_fixture f;
            notification_fixture_init(&f, dark, CUI_NOTIFICATION_SHADOW_SOURCE);
            cui_draw_list list =
                notification_fixture_frame(&f, 128.0f / scale, 128.0f / scale, 1, 1);
            assert(cui_sokol_render(r, &list, (cui_color){255, 255, 255, 255}));
            glBindFramebuffer(GL_FRAMEBUFFER, fbo);
            glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            assert(glGetError() == GL_NO_ERROR);
            notification_pixels(pixels, 128 * 4, 4, 128, true, scale, f.theme);
            cui_draw_command copied[1024];
            memcpy(copied, list.commands, list.count * sizeof(*copied));
            for (size_t i = 0; i < list.count; i++)
                if (copied[i].type == CUI_DRAW_DROP_SHADOW) {
                    copied[i].shadow.layer.sigma = NAN;
                    break;
                }
            list.commands = copied;
            assert(!cui_sokol_render(r, &list, (cui_color){0}));
            unsigned char retained[sizeof pixels];
            glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, retained);
            assert(glGetError() == GL_NO_ERROR && !memcmp(pixels, retained, sizeof pixels));
            cui_destroy(f.ui);
        }
    cui_sokol_destroy(r);
    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &fbo);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
}
int main(void) {
    SDL_SetAssertionHandler(stop_assert, NULL);
    assert(SDL_Init(SDL_INIT_VIDEO));
    notification_renderer();
    notification_gpu();
    notification_sokol();
    assert(!sdl_assertions);
    SDL_Quit();
    puts("PASS actual Notification SOURCE component SDL Renderer/GPU/Sokol light/dark DPR1/2");
    return 0;
}
