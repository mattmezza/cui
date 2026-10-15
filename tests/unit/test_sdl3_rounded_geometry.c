/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "../../backends/software/cui_shape.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* The reference retains the former per-row SDL_RenderFillRect path. Linker
 * wrapping counts adapter submissions without counting SDL's internal calls. */
static SDL_FRect expected[4096];
static int expected_count, consumed, single_calls, batch_calls;
static bool reference;
static int fail_call;
bool __real_SDL_RenderFillRect(SDL_Renderer *, const SDL_FRect *);
bool __real_SDL_RenderFillRects(SDL_Renderer *, const SDL_FRect *, int);
bool __wrap_SDL_RenderFillRect(SDL_Renderer *renderer, const SDL_FRect *box) {
    single_calls++;
    return __real_SDL_RenderFillRect(renderer, box);
}
bool __wrap_SDL_RenderFillRects(SDL_Renderer *renderer, const SDL_FRect *boxes, int count) {
    batch_calls++;
    if (fail_call == batch_calls)
        return SDL_SetError("Injected rounded-strip submission failure");
    assert(count > 0 && count <= 128);
    for (int i = 0; i < count; i++) {
        assert(consumed < expected_count);
        const SDL_FRect *old = &expected[consumed++];
        assert(boxes[i].x == old->x && boxes[i].y == old->y && boxes[i].w == old->w &&
               boxes[i].h == old->h);
        if (reference && !__real_SDL_RenderFillRect(renderer, old))
            return false;
    }
    return reference || __real_SDL_RenderFillRects(renderer, boxes, count);
}
static void old_span(const cui_draw_command *c, float left, float right, float y, float h) {
    float x0 = ceilf(c->bounds.x + left - .5f);
    float x1 = ceilf(c->bounds.x + right - .5f);
    if (x1 <= x0)
        return;
    assert(expected_count < (int)(sizeof expected / sizeof expected[0]));
    expected[expected_count++] = (SDL_FRect){x0, c->bounds.y + y, x1 - x0, h};
}
static void old_rect(const cui_draw_command *c) {
    float radii[4], edges[4];
    cui_shape_values(c, radii, edges);
    for (int y = 0; y < (int)ceilf(c->bounds.h); y++) {
        float h = fminf(1, c->bounds.h - y), left, right, il, ir;
        if (!cui_shape_span(c, radii, edges, false, y + h / 2, &left, &right))
            continue;
        if (c->type == CUI_DRAW_BORDER &&
            cui_shape_span(c, radii, edges, true, y + h / 2, &il, &ir)) {
            old_span(c, left, il, (float)y, h);
            old_span(c, ir, right, (float)y, h);
        } else
            old_span(c, left, right, (float)y, h);
    }
}
static void compare(cui_sdl3 *backend, cui_draw_command shape, bool clipped) {
    cui_draw_command commands[] = {
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {19.25f, 17.75f, 170.5f, 245.25f}},
        shape, {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list list = {clipped ? commands : &commands[1], clipped ? 3 : 1, 256, 384};
    expected_count = 0;
    old_rect(&shape);
    assert(expected_count > 0);
    for (int pass = 0; pass < 2; pass++) {
        reference = pass == 0;
        consumed = single_calls = batch_calls = 0;
        assert(cui_sdl3_render_capture(backend, &list, (cui_color){31, 67, 103, 255},
                                       pass ? "rounded-batch.bmp" : "rounded-reference.bmp"));
        assert(consumed == expected_count && single_calls == 0);
        assert(batch_calls == (expected_count + 127) / 128);
        assert(batch_calls < expected_count);
    }
    SDL_Surface *a = SDL_LoadBMP("rounded-reference.bmp");
    SDL_Surface *b = SDL_LoadBMP("rounded-batch.bmp");
    assert(a && b && a->w == b->w && a->h == b->h && a->format == b->format);
    int bytes = SDL_BYTESPERPIXEL(a->format) * a->w;
    for (int y = 0; y < a->h; y++)
        assert(memcmp((char *)a->pixels + y * a->pitch,
                      (char *)b->pixels + y * b->pitch, (size_t)bytes) == 0);
    SDL_DestroySurface(a);
    SDL_DestroySurface(b);
}
int main(void) {
    cui_sdl3 *backend = cui_sdl3_create("Rounded strip parity", 256, 384, NULL);
    assert(backend);
    cui_draw_command shape = {.type = CUI_DRAW_RECT,
        .bounds = {8.25f, 6.75f, 221.5f, 340.25f}, .color = {210, 121, 37, 128},
        .custom_corners = true, .corner_radii = {37, 0, 63.5f, 12.25f},
        .custom_border_edges = true, .border_edges = {2.25f, 9, 17.5f, 4}};
    for (int type = 0; type < 2; type++) {
        shape.type = type ? CUI_DRAW_BORDER : CUI_DRAW_RECT;
        for (int alpha = 0; alpha < 3; alpha++) {
            shape.color.a = alpha == 0 ? 0 : alpha == 1 ? 128 : 255;
            compare(backend, shape, false);
            compare(backend, shape, true);
        }
    }
    /* Both full-buffer and final partial-buffer failures reach the caller. */
    cui_draw_list failure_list = {&shape, 1, 256, 384};
    expected_count = 0;
    old_rect(&shape);
    for (int pass = 0; pass < 2; pass++) {
        reference = false;
        consumed = single_calls = batch_calls = 0;
        fail_call = pass ? (expected_count + 127) / 128 : 1;
        assert(!cui_sdl3_render(backend, &failure_list, (cui_color){0, 0, 0, 255}));
        assert(batch_calls == fail_call);
    }
    fail_call = 0;
    shape.bounds = (cui_rect){-7.5f, -2.25f, 87.25f, 45.5f};
    shape.corner_radii = (cui_corner_radii){100, 75, 60, 90};
    compare(backend, shape, true);
    cui_sdl3_destroy(backend);
    remove("rounded-reference.bmp");
    remove("rounded-batch.bmp");
    return 0;
}
