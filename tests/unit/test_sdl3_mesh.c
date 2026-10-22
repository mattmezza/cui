/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "sdl_mesh_oracle.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static SDL_Renderer *renderer;
static int live_textures, presents, upload_calls, fail_upload, fail_config, fail_allocate;
static int scale_calls, fail_scale, draw_calls, fail_draw, fail_surface, fail_blend;
static unsigned cases;
static bool real_present;
bool __real_SDL_RenderPresent(SDL_Renderer *);
SDL_Renderer *__real_SDL_CreateRenderer(SDL_Window *, const char *);
SDL_Renderer *__wrap_SDL_CreateRenderer(SDL_Window *window, const char *name) {
    renderer = __real_SDL_CreateRenderer(window, name);
    return renderer;
}
bool __wrap_SDL_RenderPresent(SDL_Renderer *r) {
    assert(r == renderer);
    presents++;
    return real_present ? __real_SDL_RenderPresent(r) : true;
    /* Otherwise retain the target for defined pre-Present readback. */
}
SDL_Texture *__real_SDL_CreateTextureFromSurface(SDL_Renderer *, SDL_Surface *);
SDL_Texture *__wrap_SDL_CreateTextureFromSurface(SDL_Renderer *r, SDL_Surface *surface) {
    upload_calls++;
    if (fail_upload == upload_calls)
        return NULL;
    SDL_Texture *texture = __real_SDL_CreateTextureFromSurface(r, surface);
    live_textures += texture != NULL;
    return texture;
}
SDL_Surface *__real_SDL_CreateSurfaceFrom(int, int, SDL_PixelFormat, void *, int);
SDL_Surface *__wrap_SDL_CreateSurfaceFrom(int w, int h, SDL_PixelFormat format, void *pixels,
                                          int pitch) {
    if (fail_surface && --fail_surface == 0)
        return NULL;
    return __real_SDL_CreateSurfaceFrom(w, h, format, pixels, pitch);
}
bool __real_SDL_SetTextureBlendMode(SDL_Texture *, SDL_BlendMode);
bool __wrap_SDL_SetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode mode) {
    if (fail_blend && --fail_blend == 0)
        return false;
    return __real_SDL_SetTextureBlendMode(texture, mode);
}
void __real_SDL_DestroyTexture(SDL_Texture *);
void __wrap_SDL_DestroyTexture(SDL_Texture *texture) {
    if (texture)
        live_textures--;
    __real_SDL_DestroyTexture(texture);
}
bool __real_SDL_SetTextureScaleMode(SDL_Texture *, SDL_ScaleMode);
bool __wrap_SDL_SetTextureScaleMode(SDL_Texture *t, SDL_ScaleMode mode) {
    if (fail_config && --fail_config == 0)
        return false;
    return __real_SDL_SetTextureScaleMode(t, mode);
}
void *__real_calloc(size_t, size_t);
void *__wrap_calloc(size_t n, size_t size) {
    if (fail_allocate && --fail_allocate == 0)
        return NULL;
    return __real_calloc(n, size);
}
bool __real_SDL_SetRenderScale(SDL_Renderer *, float, float);
bool __wrap_SDL_SetRenderScale(SDL_Renderer *r, float x, float y) {
    if (++scale_calls == fail_scale)
        return false;
    return __real_SDL_SetRenderScale(r, x, y);
}
bool __real_SDL_RenderTexture(SDL_Renderer *, SDL_Texture *, const SDL_FRect *, const SDL_FRect *);
bool __wrap_SDL_RenderTexture(SDL_Renderer *r, SDL_Texture *t, const SDL_FRect *src,
                              const SDL_FRect *dst) {
    if (++draw_calls == fail_draw)
        return false;
    return __real_SDL_RenderTexture(r, t, src, dst);
}
typedef struct {
    int lw, lh;
    SDL_RendererLogicalPresentation mode;
    float sx, sy;
    SDL_Rect viewport;
    bool viewport_set;
} state;
static state snapshot(void) {
    state s;
    assert(SDL_GetRenderLogicalPresentation(renderer, &s.lw, &s.lh, &s.mode));
    assert(SDL_GetRenderScale(renderer, &s.sx, &s.sy));
    assert(SDL_GetRenderViewport(renderer, &s.viewport));
    s.viewport_set = SDL_RenderViewportSet(renderer);
    return s;
}
static void restore(state s) {
    assert(SDL_SetRenderLogicalPresentation(renderer, s.lw, s.lh, s.mode));
    assert(SDL_SetRenderScale(renderer, s.sx, s.sy));
    assert(SDL_SetRenderViewport(renderer, s.viewport_set ? &s.viewport : NULL));
}
static void physical(void) {
    assert(SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED));
    assert(SDL_SetRenderScale(renderer, 1, 1));
    assert(SDL_SetRenderViewport(renderer, NULL));
    assert(SDL_SetRenderClipRect(renderer, NULL));
}
static SDL_Surface *readback(void) {
    state s = snapshot();
    physical();
    SDL_Surface *raw = SDL_RenderReadPixels(renderer, NULL);
    assert(raw);
    SDL_Surface *rgba = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
    assert(rgba);
    SDL_DestroySurface(raw);
    restore(s);
    return rgba;
}
static void equal(SDL_Surface *a, SDL_Surface *b, int tolerance) {
    assert(a->w == b->w && a->h == b->h);
    for (int y = 0; y < a->h; y++)
        for (int x = 0; x < a->w * 4; x++) {
            int av = ((unsigned char *)a->pixels)[y * a->pitch + x];
            int bv = ((unsigned char *)b->pixels)[y * b->pitch + x];
            if (abs(av - bv) > tolerance) {
                fprintf(stderr, "case%u at %d,%d channel%d: %d vs %d\n", cases, x / 4, y, x % 4, av,
                        bv);
                abort();
            }
        }
}
static void reference_mesh(cui_mesh_paint p, const SDL_Rect *logical) {
    state s = snapshot();
    SDL_FRect presentation;
    assert(SDL_GetRenderLogicalPresentationRect(renderer, &presentation));
    int width, height;
    assert(SDL_GetRenderOutputSize(renderer, &width, &height));
    float sx = s.sx, sy = s.sy, ox = 0, oy = 0;
    int pw = width, ph = height;
    if (s.mode != SDL_LOGICAL_PRESENTATION_DISABLED) {
        sx *= presentation.w / s.lw;
        sy *= presentation.h / s.lh;
        ox = presentation.x;
        oy = presentation.y;
        pw = (int)presentation.w;
        ph = (int)presentation.h;
    }
    reference_sx = sx;
    reference_sy = sy;
    reference_ox = floorf(s.viewport.x * sx + ox);
    reference_oy = floorf(s.viewport.y * sy + oy);
    SDL_Rect viewport = {(int)reference_ox, (int)reference_oy,
                         s.viewport_set ? (int)ceilf(s.viewport.w * sx) : pw,
                         s.viewport_set ? (int)ceilf(s.viewport.h * sy) : ph};
    SDL_Rect clip, target = {0, 0, width, height};
    if (!SDL_GetRectIntersection(&target, &viewport, &clip))
        clip = (SDL_Rect){0};
    if (logical) {
        SDL_Rect lc = {(int)reference_ox + (int)floorf(logical->x * sx),
                       (int)reference_oy + (int)floorf(logical->y * sy),
                       (int)ceilf(logical->w * sx), (int)ceilf(logical->h * sy)};
        if (!SDL_GetRectIntersection(&clip, &lc, &clip))
            clip = (SDL_Rect){0};
    }
    physical();
    unsigned char *pixels = calloc((size_t)width * (unsigned)height, 4);
    assert(pixels);
    mesh_oracle oracle = {(unsigned)width,
                          (unsigned)height,
                          pixels,
                          {(float)clip.x, (float)clip.y, (float)clip.w, (float)clip.h}};
    for (size_t i = 0; i < p.vertex_count; i += 3) {
        memset(pixels, 0, (size_t)width * (unsigned)height * 4);
        cui_mesh_paint triangle = p;
        triangle.vertices = p.vertices + i;
        triangle.vertex_count = 3;
        mesh(&oracle, &triangle);
        SDL_Surface *surface =
            SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels, width * 4);
        assert(surface);
        SDL_Texture *texture = __real_SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        assert(texture);
        assert(SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND));
        assert(__real_SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST));
        SDL_FRect bounds = {0, 0, (float)width, (float)height};
        assert(__real_SDL_RenderTexture(renderer, texture, NULL, &bounds));
        __real_SDL_DestroyTexture(texture);
    }
    free(pixels);
    restore(s);
    assert(SDL_SetRenderClipRect(renderer, logical));
}
static void reference_list(cui_draw_list list, cui_color clear) {
    assert(SDL_SetRenderClipRect(renderer, NULL));
    assert(SDL_SetRenderDrawColor(renderer, clear.r, clear.g, clear.b, clear.a));
    assert(SDL_RenderClear(renderer));
    SDL_Rect stack[64], clip = {0, 0, (int)ceilf(list.width), (int)ceilf(list.height)};
    size_t depth = 0;
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *c = &list.commands[i];
        switch (c->type) {
        case CUI_DRAW_MESH:
            reference_mesh(c->mesh, depth ? &clip : NULL);
            break;
        case CUI_DRAW_RECT: {
            SDL_FRect r = {c->bounds.x, c->bounds.y, c->bounds.w, c->bounds.h};
            assert(
                SDL_SetRenderDrawColor(renderer, c->color.r, c->color.g, c->color.b, c->color.a));
            assert(SDL_RenderFillRect(renderer, &r));
            break;
        }
        case CUI_DRAW_CLIP_PUSH: {
            stack[depth++] = clip;
            SDL_Rect next = {(int)floorf(c->bounds.x), (int)floorf(c->bounds.y),
                             (int)ceilf(c->bounds.w), (int)ceilf(c->bounds.h)};
            if (!SDL_GetRectIntersection(&clip, &next, &clip))
                clip = (SDL_Rect){0};
            assert(SDL_SetRenderClipRect(renderer, &clip));
            break;
        }
        case CUI_DRAW_CLIP_POP:
            clip = stack[--depth];
            assert(SDL_SetRenderClipRect(renderer, depth ? &clip : NULL));
            break;
        default:
            abort();
        }
    }
}
static void compare(cui_sdl3 *backend, cui_draw_list list, cui_color clear) {
    state before = snapshot();
    reference_list(list, clear);
    SDL_Surface *reference = readback();
    assert(cui_sdl3_render(backend, &list, clear));
    state after = snapshot();
    assert(before.lw == after.lw && before.lh == after.lh && before.mode == after.mode);
    assert(before.sx == after.sx && before.sy == after.sy &&
           before.viewport_set == after.viewport_set);
    assert(!memcmp(&before.viewport, &after.viewport, sizeof before.viewport));
    SDL_Surface *actual = readback();
    equal(reference, actual,
          0); /* Full-target independent oracle texture vs clipped preparation. */
    SDL_DestroySurface(reference);
    SDL_DestroySurface(actual);
    assert(live_textures == 0);
    cases++;
}
static void failed(cui_sdl3 *backend, cui_draw_list list) {
    physical();
    assert(SDL_SetRenderDrawColor(renderer, 17, 39, 71, 113));
    assert(SDL_RenderClear(renderer));
    SDL_Surface *before = readback();
    int prior = presents;
    assert(!cui_sdl3_render(backend, &list, (cui_color){255, 0, 0, 255}));
    assert(presents == prior && live_textures == 0);
    SDL_Surface *after = readback();
    equal(before, after, 0);
    SDL_DestroySurface(before);
    SDL_DestroySurface(after);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    cui_sdl3 *backend = cui_sdl3_create("Analytic mesh fixture", 96, 80, font);
    if (!backend)
        fprintf(stderr, "SDL mesh backend creation: %s\n", cui_sdl3_error());
    assert(backend);
    assert(cui_sdl3_interface(backend).capabilities & CUI_RENDER_MESH);
    cui_mesh_vertex vertices[] = {{1, 1, -1, 1}, {28, 2, 1, 1}, {3, 25, 0, -1},
                                  {2, 2, 0, 0},  {29, 2, 0, 0}, {3, 25, 0, 0}};
    cui_draw_command commands[] = {
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 19, 17}, .color = {71, 39, 17, 129}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {2.5f, 1.25f, 26.5f, 27.25f}},
        {.type = CUI_DRAW_MESH, .mesh = {vertices, 6, .25f, .5f, 1, {.15f, .08f, .3f, .5f}}},
        {.type = CUI_DRAW_RECT, .bounds = {8, 4, 4, 9}, .color = {91, 211, 39, 173}},
        {.type = CUI_DRAW_CLIP_POP},
        {.type = CUI_DRAW_MESH, .mesh = {vertices, 3, 7.5f, 4.25f, .75f, {.5f, 0, .1f, .75f}}}};
    cui_draw_list list = {commands, sizeof commands / sizeof *commands, 48, 40};
    SDL_RendererLogicalPresentation modes[] = {
        SDL_LOGICAL_PRESENTATION_DISABLED, SDL_LOGICAL_PRESENTATION_STRETCH,
        SDL_LOGICAL_PRESENTATION_LETTERBOX, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE,
        SDL_LOGICAL_PRESENTATION_OVERSCAN};
    for (size_t m = 0; m < sizeof modes / sizeof *modes; m++)
        for (int scale = 0; scale < 3; scale++)
            for (int viewport = 0; viewport < 2; viewport++) {
                assert(SDL_SetRenderLogicalPresentation(renderer, 53, 37, modes[m]));
                assert(SDL_SetRenderScale(renderer,
                                          scale == 0   ? 1
                                          : scale == 1 ? 2
                                                       : 1.25f,
                                          scale == 0   ? 1
                                          : scale == 1 ? 2
                                                       : .75f));
                SDL_Rect vp = {3, 2, 43, 33};
                assert(SDL_SetRenderViewport(renderer, viewport ? &vp : NULL));
                for (int alpha = 0; alpha < 3; alpha++)
                    compare(backend, list,
                            (cui_color){19, 37, 83,
                                        alpha == 0   ? 0
                                        : alpha == 1 ? 113
                                                     : 255});
            }
    physical();
    cui_draw_command nested[8] = {
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {4.25f, 3.5f, 19.5f, 21.25f}}};
    memcpy(nested + 1, commands, sizeof commands);
    nested[7] = (cui_draw_command){.type = CUI_DRAW_CLIP_POP};
    assert(SDL_SetRenderScale(renderer, 1.25f, .75f));
    compare(backend, (cui_draw_list){nested, 8, 48, 40}, (cui_color){19, 37, 83, 113});
    assert(!SDL_RenderClipEnabled(renderer));
    physical();
    /* Independent solid/shared-edge/gradient-zero/SDR vectors at physical DPI1. */
    cui_mesh_vertex square[6] = {{0, 0, 0, 0}, {8, 0, 0, 0}, {0, 8, 0, 0},
                                 {8, 0, 0, 0}, {8, 8, 0, 0}, {0, 8, 0, 0}};
    cui_draw_command vector = {.type = CUI_DRAW_MESH,
                               .mesh = {square, 6, 0, 0, 1, {-.1f, .0031308f * .5f, 2, .5f}}};
    for (int winding = 0; winding < 2; winding++) {
        compare(backend, (cui_draw_list){&vector, 1, 48, 40}, (cui_color){19, 37, 83, 113});
        for (int triangle = 0; triangle < 2; triangle++) {
            cui_mesh_vertex swap = square[triangle * 3 + 1];
            square[triangle * 3 + 1] = square[triangle * 3 + 2];
            square[triangle * 3 + 2] = swap;
        }
    }
    vector.mesh.vertex_count = 3;
    for (size_t i = 0; i < 3; i++)
        square[i].s = square[i].x - .5f;
    for (int t = -1; t <= 1; t++) {
        for (size_t i = 0; i < 3; i++)
            square[i].t = (float)t;
        compare(backend, (cui_draw_list){&vector, 1, 48, 40}, (cui_color){0, 0, 0, 0});
    }
    square[2] = square[1];
    compare(backend, (cui_draw_list){&vector, 1, 48, 40}, (cui_color){7, 13, 31, 113});
    square[0] = (cui_mesh_vertex){-1e30f, -1e30f, 0, 0};
    square[1] = (cui_mesh_vertex){1e30f, -1e30f, 0, 0};
    square[2] = (cui_mesh_vertex){0, 1e30f, 0, 0};
    compare(backend, (cui_draw_list){&vector, 1, 48, 40}, (cui_color){7, 13, 31, 113});
    vector.mesh.color.a = 0;
    compare(backend, (cui_draw_list){&vector, 1, 48, 40}, (cui_color){7, 13, 31, 113});
    for (int nth = 1; nth <= 3; nth++) {
        upload_calls = 0;
        fail_upload = nth;
        failed(backend, list);
        fail_upload = 0;
        fail_config = nth;
        failed(backend, list);
        fail_config = 0;
        fail_surface = nth;
        failed(backend, list);
        fail_surface = 0;
        fail_blend = nth;
        failed(backend, list);
        fail_blend = 0;
        fail_allocate = nth;
        failed(backend, list);
        fail_allocate = 0;
    }
    cui_draw_command bad[7];
    memcpy(bad, commands, sizeof commands);
    bad[6] = commands[5];
    bad[6].mesh.scale = 0;
    failed(backend, (cui_draw_list){bad, 7, 48, 40});
    bad[6] = (cui_draw_command){.type = CUI_DRAW_CLIP_POP};
    failed(backend, (cui_draw_list){bad, 7, 48, 40});
    bad[6] = (cui_draw_command){.type = (cui_draw_type)-1};
    failed(backend, (cui_draw_list){bad, 7, 48, 40});
    cui_mesh_vertex large[3 * 257];
    for (size_t i = 0; i < sizeof large / sizeof *large; i++)
        large[i] = vertices[i % 3];
    bad[0] =
        (cui_draw_command){.type = CUI_DRAW_MESH,
                           .mesh = {large, sizeof large / sizeof *large, 0, 0, 1, {1, 1, 1, 1}}};
    failed(backend, (cui_draw_list){bad, 1, 48, 40});
    /* Ten worst-case 25-triangle arrow payloads fit; 257 triangles do not. */
    bad[0].mesh.vertex_count = 250 * 3;
    compare(backend, (cui_draw_list){bad, 1, 48, 40}, (cui_color){13, 29, 47, 255});
    /* Large clipped target exceeds 8M texels before scratch/upload allocation. */
    SDL_Texture *target =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 3000, 3000);
    assert(target && SDL_SetRenderTarget(renderer, target));
    cui_mesh_vertex huge[] = {{0, 0, 0, 0}, {3000, 0, 0, 0}, {0, 3000, 0, 0}};
    bad[0].mesh = (cui_mesh_paint){huge, 3, 0, 0, 1, {1, 1, 1, 1}};
    failed(backend, (cui_draw_list){bad, 1, 3000, 3000});
    /* A small real alpha target exercises transparent target compositing too. */
    assert(SDL_SetRenderTarget(renderer, NULL));
    __real_SDL_DestroyTexture(target);
    target = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, 96, 80);
    assert(target && SDL_SetRenderTarget(renderer, target));
    physical();
    compare(backend, list, (cui_color){19, 37, 83, 113});
    assert(SDL_SetRenderScale(renderer, 1.25f, .75f));
    compare(backend, list, (cui_color){19, 37, 83, 0});
    assert(SDL_SetRenderTarget(renderer, NULL));
    __real_SDL_DestroyTexture(target);
    /* Submission/setter failure is after clear: assert cleanup/state/recovery, not rollback. */
    for (int failure = 0; failure < 3; failure++) {
        physical();
        state before = snapshot();
        if (failure)
            fail_scale = scale_calls + failure;
        else
            fail_draw = draw_calls + 1;
        assert(!cui_sdl3_render(backend, &list, (cui_color){0, 0, 0, 0}));
        fail_scale = fail_draw = 0;
        state after = snapshot();
        assert(before.mode == after.mode && before.sx == after.sx && before.sy == after.sy);
        assert(before.viewport_set == after.viewport_set);
        assert(live_textures == 0);
        compare(backend, list, (cui_color){0, 0, 0, 0});
    }
    assert(SDL_SetWindowSize(SDL_GetRenderWindow(renderer), 112, 88));
    SDL_PumpEvents();
    physical();
    compare(backend, list, (cui_color){13, 29, 47, 255});
    vertices[1].x = 13; /* Same borrowed pointer, changed geometry; no stale cross-frame cache. */
    compare(backend, list, (cui_color){13, 29, 47, 255});
    /* Following image/text draws must retain identical mapping after a mesh.
     * Mesh is transparent here; real nonzero overlapping mixed rect/mesh order
     * is covered above. Cache texture ownership remains with the renderer. */
    static const unsigned char image[] = {255, 0, 0,   128, 0,   255, 0,   255,
                                          0,   0, 255, 71,  255, 255, 255, 191};
    cui_draw_command mixed[] = {
        {.type = CUI_DRAW_MESH, .mesh = {vertices, 3, 0, 0, 1, {0, 0, 0, 0}}},
        {.type = CUI_DRAW_IMAGE,
         .bounds = {5, 3, 13, 11},
         .bitmap = {.bitmap = {image, 2, 2, 8, 1}, .source = {0, 0, 2, 2}, .opacity = 1}},
        {.type = CUI_DRAW_TEXT,
         .bounds = {3, 18, 40, 20},
         .font_size = 11,
         .text = CUI_STR("Mesh"),
         .color = {171, 39, 211, 193}}};
    assert(SDL_SetRenderLogicalPresentation(renderer, 53, 37, SDL_LOGICAL_PRESENTATION_LETTERBOX));
    assert(SDL_SetRenderScale(renderer, 1.25f, .75f));
    SDL_Rect vp = {3, 2, 43, 33};
    assert(SDL_SetRenderViewport(renderer, &vp));
    cui_color clear = {29, 47, 61, 113};
    assert(cui_sdl3_render(backend, &(cui_draw_list){mixed + 1, 2, 48, 40}, clear));
    SDL_Surface *baseline = readback();
    assert(cui_sdl3_render(backend, &(cui_draw_list){mixed, 3, 48, 40}, clear));
    SDL_Surface *with_mesh = readback();
    equal(baseline, with_mesh, 0);
    SDL_DestroySurface(baseline);
    SDL_DestroySurface(with_mesh);
    /* Invalid image preflight and unknown command after a valid mesh cannot clear. */
    physical();
    mixed[1].bitmap.bitmap.pixels = NULL;
    int retained = live_textures;
    assert(SDL_SetRenderDrawColor(renderer, 17, 39, 71, 113) && SDL_RenderClear(renderer));
    SDL_Surface *sentinel = readback();
    assert(!cui_sdl3_render(backend, &(cui_draw_list){mixed, 3, 48, 40}, clear));
    with_mesh = readback();
    equal(sentinel, with_mesh, 0);
    SDL_DestroySurface(sentinel);
    SDL_DestroySurface(with_mesh);
    assert(live_textures == retained);
    cases++;
    real_present = true;
    assert(cui_sdl3_render(backend, &list, (cui_color){13, 29, 47, 255}));
    cui_sdl3_destroy(backend);
    assert(live_textures == 0);
    cui_font_close(font);
    printf("SDL mesh: %u real readback comparisons plus preclear failures/recovery passed\n",
           cases);
    return 0;
}
