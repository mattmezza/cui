/* SPDX-License-Identifier: MIT */
/* Reuse the real DROP compatibility harness and its pre-Present capture wrappers. */
#define GL_GLEXT_PROTOTYPES 1
#define main drop_compatibility_main
#include "test_drop_shadow_adapters.c"
#undef main
static cui_draw_command inset_command(void) {
    return (cui_draw_command){
        .type = CUI_DRAW_INSET_SHADOW,
        .shadow = {.caster = {20, 20, 20, 20}, .layer = {.spread = 2, .color = {0, 0, 0, 255}}}};
}
static void inset_pixels(const unsigned char *p, size_t pitch, unsigned channels, unsigned height,
                         bool upside, unsigned scale) {
    unsigned row = 30 * scale;
    if (upside)
        row = height - 1 - row;
    const unsigned char *edge = p + (size_t)row * pitch + 21 * scale * channels;
    const unsigned char *inside = p + (size_t)row * pitch + 23 * scale * channels;
    const unsigned char *outside = p + (size_t)row * pitch + 19 * scale * channels;
    for (unsigned i = 0; i < 3; i++)
        assert(edge[i] == 0 && inside[i] == 255 && outside[i] == 255);
}
static cui_draw_command gaussian_command(void) {
    cui_draw_command c = inset_command();
    c.shadow.radii = (cui_corner_radii){10, 2, 6, 0};
    c.shadow.layer = (cui_drop_shadow){
        .offset_x = -2, .offset_y = 3, .sigma = 3, .spread = -1, .color = {60, 100, 180, 128}};
    return c;
}
static void gaussian_pixels(const unsigned char *p, size_t pitch, unsigned channels,
                            unsigned height, bool upside, unsigned scale) {
    cui_draw_command c = gaussian_command();
    const unsigned points[][2] = {{20, 30}, {21, 21}, {23, 25}, {35, 22},
                                  {38, 38}, {30, 30}, {39, 28}};
    for (size_t i = 0; i < sizeof points / sizeof points[0]; i++) {
        unsigned x = points[i][0] * scale, y = points[i][1] * scale,
                 row = upside ? height - 1 - y : y;
        float coverage;
        assert(cui_effect_inset_shadow_sample(c.shadow, (float)scale, 0, 0, x + .5f, y + .5f,
                                              &coverage));
        float alpha = coverage * 128 / 255;
        const unsigned char *actual = p + (size_t)row * pitch + x * channels;
        const float rgb[] = {60, 100, 180};
        for (unsigned k = 0; k < 3; k++) {
            if (fabsf(actual[k] - (rgb[k] * alpha + 255 * (1 - alpha))) > 2)
                fprintf(stderr,
                        "Gaussian scale%u xy%u,%u channel%u actual%u expected%g coverage%g\n",
                        scale, x, y, k, actual[k], rgb[k] * alpha + 255 * (1 - alpha), coverage);
            assert(fabsf(actual[k] - (rgb[k] * alpha + 255 * (1 - alpha))) <= 2);
        }
    }
}
static void inset_renderer(void) {
    cui_sdl3 *r = cui_sdl3_create("inset SDL", 128, 128, NULL);
    assert(r);
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    SDL_Renderer *renderer = NULL;
    for (int i = 0; i < count; i++)
        if (!strcmp(SDL_GetWindowTitle(windows[i]), "inset SDL"))
            renderer = SDL_GetRenderer(windows[i]);
    SDL_free(windows);
    assert(renderer);
    capture_renderer = renderer;
    printf("INSET SDL Renderer: %s\n", SDL_GetRendererName(renderer));
    cui_draw_command command = inset_command();
    cui_draw_list one = {&command, 1, 64, 64};
    for (unsigned scale = 1; scale <= 2; scale++) {
        assert(SDL_SetRenderScale(renderer, (float)scale, (float)scale));
        assert(cui_sdl3_render(r, &one, (cui_color){255, 255, 255, 255}));
        SDL_Surface *surface = last_completed(renderer);
        assert(surface);
        inset_pixels(surface->pixels, (size_t)surface->pitch, 4, (unsigned)surface->h, false,
                     scale);
        SDL_DestroySurface(surface);
        command = gaussian_command();
        assert(cui_sdl3_render(r, &one, (cui_color){255, 255, 255, 255}));
        surface = last_completed(renderer);
        assert(surface);
        gaussian_pixels(surface->pixels, (size_t)surface->pitch, 4, (unsigned)surface->h, false,
                        scale);
        SDL_DestroySurface(surface);
        float restored_sx, restored_sy;
        assert(SDL_GetRenderScale(renderer, &restored_sx, &restored_sy));
        assert(restored_sx == scale && restored_sy == scale);
        assert(!SDL_RenderViewportSet(renderer) && !SDL_RenderClipEnabled(renderer));
        assert(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255));
        SDL_FRect ordinary = {45, 45, 2, 2};
        assert(SDL_RenderFillRect(renderer, &ordinary));
        SDL_RenderPresent(renderer);
        surface = last_completed(renderer);
        assert(surface);
        const unsigned char *ordinary_pixel =
            (unsigned char *)surface->pixels + 45 * scale * surface->pitch + 45 * scale * 4;
        assert(ordinary_pixel[0] == 255 && ordinary_pixel[1] == 0 && ordinary_pixel[2] == 0);
        SDL_DestroySurface(surface);
        assert(cui_sdl3_render(r, &one, (cui_color){255, 255, 255, 255}));
        command = inset_command();
    }
    unsigned old_presents = presents, old_clears = clears, old_draws = draws;
    command.shadow.layer.sigma = NAN;
    assert(!cui_sdl3_render(r, &one, (cui_color){0}));
    assert(presents == old_presents && clears == old_clears && draws == old_draws);
    gaussian_pixels(completed_capture->pixels, (size_t)completed_capture->pitch, 4,
                    (unsigned)completed_capture->h, false, 2);
    command = inset_command();
    command.shadow.caster = (cui_rect){0, 0, 16384, 16384};
    command.shadow.layer.spread = 4096;
    one.width = one.height = 16384;
    assert(SDL_SetRenderScale(renderer, 1.0f / 65536, 1.0f / 65536));
    SDL_Rect viewport = {52428, 52428, 8388608, 8388608};
    assert(SDL_SetRenderViewport(renderer, &viewport));
    assert(cui_sdl3_render(r, &one, (cui_color){255, 255, 255, 255}));
    SDL_Rect actual_viewport;
    float actual_sx, actual_sy;
    assert(SDL_GetRenderViewport(renderer, &actual_viewport));
    assert(SDL_GetRenderScale(renderer, &actual_sx, &actual_sy));
    fprintf(stderr, "tiny viewport %d,%d scale %g,%g pixel0=%u pixel1=%u\n", actual_viewport.x,
            actual_viewport.y, actual_sx, actual_sy,
            ((unsigned char *)completed_capture->pixels)[0],
            ((unsigned char *)completed_capture->pixels)[completed_capture->pitch + 4]);
    SDL_Surface *tiny = last_completed(renderer);
    assert(tiny);
    const unsigned char *tiny_pixel = (unsigned char *)tiny->pixels + tiny->pitch + 4;
    assert(tiny_pixel[0] == 0 && tiny_pixel[1] == 0 && tiny_pixel[2] == 0);
    const unsigned char *tiny_hole = (unsigned char *)tiny->pixels;
    assert(tiny_hole[0] == 255 && tiny_hole[1] == 255 && tiny_hole[2] == 255);
    old_presents = presents;
    old_clears = clears;
    old_draws = draws;
    command.shadow.layer.sigma = NAN;
    assert(!cui_sdl3_render(r, &one, (cui_color){0}));
    assert(presents == old_presents && clears == old_clears && draws == old_draws);
    assert(memcmp(tiny->pixels, completed_capture->pixels, (size_t)tiny->pitch * tiny->h) == 0);
    assert(SDL_RenderViewportSet(renderer) && !SDL_RenderClipEnabled(renderer));
    assert(actual_viewport.x == viewport.x && actual_viewport.y == viewport.y);
    assert(actual_sx == 1.0f / 65536 && actual_sy == actual_sx);
    SDL_DestroySurface(tiny);
    SDL_DestroySurface(completed_capture);
    completed_capture = NULL;
    capture_renderer = NULL;
    cui_sdl3_destroy(r);
}
static unsigned char *read_ppm(const char *path, unsigned *w, unsigned *h) {
    FILE *f = fopen(path, "rb");
    assert(f);
    char magic[3];
    unsigned max;
    assert(fscanf(f, "%2s %u %u %u", magic, w, h, &max) == 4 && fgetc(f) == '\n');
    unsigned char *p = malloc((size_t)*w * *h * 3);
    assert(p);
    assert(fread(p, 3, (size_t)*w * *h, f) == (size_t)*w * *h);
    fclose(f);
    return p;
}
static void inset_gpu(void) {
    SDL_Window *w = SDL_CreateWindow("inset GPU", 128, 128, 0);
    assert(w);
    SDL_GPUDevice *d = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    assert(d && SDL_ClaimWindowForGPUDevice(d, w));
    printf("INSET SDL GPU: %s\n", SDL_GetGPUDeviceDriver(d));
    cui_sdl_gpu *r = cui_sdl_gpu_create(d, w, NULL);
    assert(r);
    cui_draw_command command = inset_command();
    for (unsigned scale = 1; scale <= 2; scale++) {
        int pw, ph;
        assert(SDL_GetWindowSizeInPixels(w, &pw, &ph));
        cui_draw_list one = {&command, 1, (float)pw / scale, (float)ph / scale};
        assert(cui_sdl_gpu_render(r, &one, (cui_color){255, 255, 255, 255}));
        assert(cui_sdl_gpu_save_ppm(r, "inset-gpu.ppm"));
        unsigned width, height;
        unsigned char *p = read_ppm("inset-gpu.ppm", &width, &height);
        inset_pixels(p, (size_t)width * 3, 3, height, false, scale);
        command.shadow.layer.sigma = NAN;
        assert(!cui_sdl_gpu_render(r, &one, (cui_color){0}));
        assert(cui_sdl_gpu_save_ppm(r, "inset-gpu-rejected.ppm"));
        unsigned rw, rh;
        unsigned char *retained = read_ppm("inset-gpu-rejected.ppm", &rw, &rh);
        assert(rw == width && rh == height && !memcmp(p, retained, (size_t)rw * rh * 3));
        free(p);
        free(retained);
        command = gaussian_command();
        assert(cui_sdl_gpu_render(r, &one, (cui_color){255, 255, 255, 255}));
        assert(cui_sdl_gpu_save_ppm(r, "inset-gaussian-gpu.ppm"));
        p = read_ppm("inset-gaussian-gpu.ppm", &width, &height);
        gaussian_pixels(p, (size_t)width * 3, 3, height, false, scale);
        free(p);
        command = inset_command();
    }
    cui_sdl_gpu_destroy(r);
    SDL_ReleaseWindowFromGPUDevice(d, w);
    SDL_DestroyGPUDevice(d);
    SDL_DestroyWindow(w);
}
static void inset_sokol(void) {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window *w =
        SDL_CreateWindow("inset Sokol", 128, 128, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(w);
    SDL_GLContext context = SDL_GL_CreateContext(w);
    assert(context);
    printf("INSET Sokol GL: %s / %s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
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
    cui_draw_command command = inset_command();
    unsigned char p[128 * 128 * 4], retained[sizeof p];
    for (unsigned scale = 1; scale <= 2; scale++) {
        cui_draw_list one = {&command, 1, 128.0f / scale, 128.0f / scale};
        assert(cui_sokol_render(r, &one, (cui_color){255, 255, 255, 255}));
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, p);
        assert(glGetError() == GL_NO_ERROR);
        inset_pixels(p, 128 * 4, 4, 128, true, scale);
        command.shadow.layer.sigma = NAN;
        assert(!cui_sokol_render(r, &one, (cui_color){0}));
        glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, retained);
        assert(glGetError() == GL_NO_ERROR && !memcmp(p, retained, sizeof p));
        command = gaussian_command();
        assert(cui_sokol_render(r, &one, (cui_color){255, 255, 255, 255}));
        glReadPixels(0, 0, 128, 128, GL_RGBA, GL_UNSIGNED_BYTE, p);
        assert(glGetError() == GL_NO_ERROR);
        gaussian_pixels(p, 128 * 4, 4, 128, true, scale);
        command = inset_command();
    }
    cui_sokol_destroy(r);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(w);
}
int main(void) {
    assert(drop_compatibility_main() == 0);
    assert(SDL_Init(SDL_INIT_VIDEO));
    inset_renderer();
    inset_gpu();
    inset_sokol();
    assert(!sdl_assertions);
    SDL_Quit();
    puts("PASS actual INSET SDL Renderer/GPU/Sokol scales1/2 and rejected frames");
    return 0;
}
