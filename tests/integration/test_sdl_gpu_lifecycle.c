/* SPDX-License-Identifier: MIT */
/* Native GNU/LLVM --wrap fault fixture; not a portable/WASI target. */
#include "cui/cui_sdl_gpu.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned acquisitions, cancels, submissions, renderpasses, blits, assertions, texture_calls;
static unsigned fail_texture, fail_map;
static bool fail_acquire, null_swap, fake_size;
static SDL_AssertState SDLCALL on_assert(const SDL_AssertData *data, void *user) {
    (void)user;
    assertions++;
    fprintf(stderr, "SDL ASSERT: %s at %s:%d\n", data->condition, data->filename, data->linenum);
    return SDL_ASSERTION_ABORT;
}
SDL_GPUTexture *__real_SDL_CreateGPUTexture(SDL_GPUDevice *, const SDL_GPUTextureCreateInfo *);
SDL_GPUTexture *__wrap_SDL_CreateGPUTexture(SDL_GPUDevice *d, const SDL_GPUTextureCreateInfo *i) {
    if (fail_texture && ++texture_calls == fail_texture) {
        SDL_SetError("injected texture allocation");
        return NULL;
    }
    return __real_SDL_CreateGPUTexture(d, i);
}
void *__real_SDL_MapGPUTransferBuffer(SDL_GPUDevice *, SDL_GPUTransferBuffer *, bool);
void *__wrap_SDL_MapGPUTransferBuffer(SDL_GPUDevice *d, SDL_GPUTransferBuffer *b, bool cycle) {
    if (fail_map) {
        fail_map--;
        SDL_SetError("injected map failure");
        return NULL;
    }
    return __real_SDL_MapGPUTransferBuffer(d, b, cycle);
}
bool __real_SDL_WaitAndAcquireGPUSwapchainTexture(SDL_GPUCommandBuffer *, SDL_Window *,
                                                  SDL_GPUTexture **, Uint32 *, Uint32 *);
bool __wrap_SDL_WaitAndAcquireGPUSwapchainTexture(SDL_GPUCommandBuffer *cb, SDL_Window *w,
                                                  SDL_GPUTexture **t, Uint32 *x, Uint32 *y) {
    acquisitions++;
    if (fail_acquire || null_swap) {
        *t = NULL;
        return null_swap;
    }
    return __real_SDL_WaitAndAcquireGPUSwapchainTexture(cb, w, t, x, y);
}
bool __real_SDL_CancelGPUCommandBuffer(SDL_GPUCommandBuffer *);
bool __wrap_SDL_CancelGPUCommandBuffer(SDL_GPUCommandBuffer *cb) {
    cancels++;
    return __real_SDL_CancelGPUCommandBuffer(cb);
}
bool __real_SDL_SubmitGPUCommandBuffer(SDL_GPUCommandBuffer *);
bool __wrap_SDL_SubmitGPUCommandBuffer(SDL_GPUCommandBuffer *cb) {
    submissions++;
    return __real_SDL_SubmitGPUCommandBuffer(cb);
}
SDL_GPURenderPass *__real_SDL_BeginGPURenderPass(SDL_GPUCommandBuffer *,
                                                 const SDL_GPUColorTargetInfo *, Uint32,
                                                 const SDL_GPUDepthStencilTargetInfo *);
SDL_GPURenderPass *__wrap_SDL_BeginGPURenderPass(SDL_GPUCommandBuffer *cb,
                                                 const SDL_GPUColorTargetInfo *colors, Uint32 n,
                                                 const SDL_GPUDepthStencilTargetInfo *depth) {
    renderpasses++;
    return __real_SDL_BeginGPURenderPass(cb, colors, n, depth);
}
void __real_SDL_BlitGPUTexture(SDL_GPUCommandBuffer *, const SDL_GPUBlitInfo *);
void __wrap_SDL_BlitGPUTexture(SDL_GPUCommandBuffer *cb, const SDL_GPUBlitInfo *i) {
    blits++;
    __real_SDL_BlitGPUTexture(cb, i);
}
bool __real_SDL_GetWindowSizeInPixels(SDL_Window *, int *, int *);
bool __wrap_SDL_GetWindowSizeInPixels(SDL_Window *w, int *x, int *y) {
    bool ok = __real_SDL_GetWindowSizeInPixels(w, x, y);
    if (ok && fake_size)
        *x += 8;
    return ok;
}
static void reset(void) {
    acquisitions = cancels = submissions = renderpasses = blits = assertions = texture_calls = 0;
    fail_texture = fail_map = 0;
    fail_acquire = null_swap = fake_size = false;
}
static unsigned char *capture(cui_sdl_gpu *r, size_t *size) {
    assert(cui_sdl_gpu_save_ppm(r, "gpu-lifecycle.ppm"));
    FILE *f = fopen("gpu-lifecycle.ppm", "rb");
    assert(f);
    assert(fseek(f, 0, SEEK_END) == 0);
    long length = ftell(f);
    assert(length > 0 && fseek(f, 0, SEEK_SET) == 0);
    unsigned char *bytes = malloc((size_t)length);
    assert(bytes && fread(bytes, 1, (size_t)length, f) == (size_t)length);
    fclose(f);
    *size = (size_t)length;
    return bytes;
}
static void unchanged(cui_sdl_gpu *r, const unsigned char *before, size_t n) {
    size_t size;
    unsigned char *after = capture(r, &size);
    assert(size == n && !memcmp(before, after, n));
    free(after);
}
static void counts(unsigned acquire, unsigned cancel, unsigned submit, unsigned render,
                   unsigned blit) {
    assert(acquisitions == acquire && cancels == cancel && submissions == submit &&
           renderpasses == render && blits == blit && !assertions);
}
int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    SDL_SetAssertionHandler(on_assert, NULL);
    assert(SDL_Init(SDL_INIT_VIDEO));
    SDL_Window *w = SDL_CreateWindow("CUI GPU lifecycle faults", 64, 64, 0);
    assert(w);
    SDL_GPUDevice *d = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    assert(d && SDL_ClaimWindowForGPUDevice(d, w));
    if (argc == 2 && !strcmp(argv[1], "--driver-only")) {
        SDL_ReleaseWindowFromGPUDevice(d, w);
        SDL_DestroyGPUDevice(d);
        SDL_DestroyWindow(w);
        SDL_Quit();
        puts("SDL/Vulkan initialization/destruction only; no CUI adapter created");
        return 0;
    }
    const char *font_path = getenv("CUI_TEST_FONT");
    cui_font *font =
        cui_font_open(font_path ? font_path : "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf");
    assert(font);
    cui_sdl_gpu *r = cui_sdl_gpu_create(d, w, font);
    assert(r);
    int pw, ph;
    assert(SDL_GetWindowSizeInPixels(w, &pw, &ph));
    cui_draw_command rect = {
        .type = CUI_DRAW_RECT, .bounds = {8, 8, 32, 24}, .color = {200, 40, 80, 255}};
    cui_draw_list list = {&rect, 1, (float)pw, (float)ph};
    reset();
    fail_texture = 1;
    assert(!cui_sdl_gpu_render(r, &list, (cui_color){0}));
    counts(0, 0, 0, 0, 0);
    reset();
    fail_map = 1;
    assert(!cui_sdl_gpu_render(r, &list, (cui_color){0}));
    counts(0, 0, 0, 0, 0);
    reset();
    assert(cui_sdl_gpu_render(r, &list, (cui_color){255, 255, 255, 255}));
    counts(1, 0, 1, 1, 1);
    size_t n;
    unsigned char *before = capture(r, &n);
    cui_draw_command invalid = {.type = CUI_DRAW_CLIP_POP};
    list.commands = &invalid;
    reset();
    assert(!cui_sdl_gpu_render(r, &list, (cui_color){0}));
    counts(0, 0, 0, 0, 0);
    unchanged(r, before, n);
    list.commands = &rect;
    reset();
    fake_size = true;
    fail_texture = 1;
    assert(!cui_sdl_gpu_render(r, &list, (cui_color){0}));
    counts(0, 0, 0, 0, 0);
    fake_size = false;
    fail_texture = 0;
    unchanged(r, before, n);
    const cui_mesh_vertex vertices[] = {{0, 0, 0, 1}, {10, 0, 0, 1}, {0, 10, 0, 1}};
    cui_draw_command mesh = {
        .type = CUI_DRAW_MESH,
        .bounds = {0, 0, 10, 10},
        .mesh = {.vertices = vertices, .vertex_count = 3, .scale = 1, .color = {1, 0, 0, 1}}};
    list.commands = &mesh;
    reset();
    fail_map = 1;
    assert(!cui_sdl_gpu_render(r, &list, (cui_color){0}));
    counts(0, 0, 0, 0, 0);
    unchanged(r, before, n);
    list.commands = &rect;
    reset();
    fake_size = true;
    fail_texture = 2;
    assert(!cui_sdl_gpu_render(r, &list, (cui_color){0}));
    counts(1, 0, 1, 0, 1);
    fake_size = false;
    fail_texture = 0;
    unchanged(r, before, n);
    cui_draw_command text = {.type = CUI_DRAW_TEXT,
                             .bounds = {8, 8, 80, 24},
                             .text = {"Retry", 5},
                             .font_size = 20,
                             .color = {0, 0, 0, 255}};
    list.commands = &text;
    reset();
    fail_acquire = true;
    assert(!cui_sdl_gpu_render(r, &list, (cui_color){0}));
    counts(1, 1, 0, 0, 0);
    fail_acquire = false;
    unchanged(r, before, n);
    reset();
    null_swap = true;
    assert(cui_sdl_gpu_render(r, &list, (cui_color){0}));
    counts(1, 1, 0, 0, 0);
    null_swap = false;
    unchanged(r, before, n);
    reset();
    assert(cui_sdl_gpu_render(r, &list, (cui_color){255, 255, 255, 255}));
    counts(1, 0, 1, 1, 1);
    size_t size;
    unsigned char *after = capture(r, &size);
    assert(size == n && memcmp(before, after, n));
    // The glyph must survive failed acquisition and null-swap cancelled upload.
    size_t dark = 0;
    for (size_t i = size - (size_t)pw * ph * 3; i < size; i += 3)
        dark += after[i] < 128 && after[i + 1] < 128 && after[i + 2] < 128;
    assert(dark > 20);
    free(after);
    free(before);
    printf("PASS GPU lifecycle faults/previous-frame preservation/atlas retry; driver=%s; "
           "target=%dx%d; SDL assertions=0\n",
           cui_sdl_gpu_driver(r), pw, ph);
    cui_sdl_gpu_destroy(r);
    cui_font_close(font);
    SDL_ReleaseWindowFromGPUDevice(d, w);
    SDL_DestroyGPUDevice(d);
    SDL_DestroyWindow(w);
    SDL_Quit();
    return 0;
}
