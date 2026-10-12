/* SPDX-License-Identifier: MIT */
#include "cui/cui_sokol.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#define SOKOL_GLCORE
#include "sokol_gfx.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
static unsigned char pixels[64 * 64 * 4];
static const unsigned char *pixel(int x, int y) {
    return pixels + ((63 - y) * 64 + x) * 4;
}
static void readback(void) {
    glReadPixels(0, 0, 64, 64, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    assert(glGetError() == GL_NO_ERROR);
}
static cui_font *find_font(void) {
#ifdef CUI_HAS_TEXT
    const char *paths[] = {getenv("CUI_TEST_FONT"), "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                           "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans.ttf"};
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
        if (paths[i]) {
            cui_font *f = cui_font_open(paths[i]);
            if (f)
                return f;
        }
#endif
    return NULL;
}
int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SKIP SDL video: %s\n", SDL_GetError());
        return 77;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_Window *window =
        SDL_CreateWindow("CUI Sokol readback test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    SDL_GLContext gl = window ? SDL_GL_CreateContext(window) : NULL;
    if (!gl) {
        fprintf(stderr, "SKIP GL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 77;
    }
    /* A dedicated RGBA8 FBO makes readback independent of window-manager size. */
    GLuint fbo, texture;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    cui_font *font = find_font();
    cui_sokol *r = cui_sokol_create(
        (cui_sokol_config){.font = font, .max_quads = 1024, .target = {fbo, 64, 64}});
    assert(r);
    cui_draw_command commands[] = {
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 64, 64}, .color = {255, 0, 0, 128}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {10, 10, 30, 30}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {20, 0, 30, 30}},
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 64, 64}, .color = {0, 255, 0, 255}},
        {.type = CUI_DRAW_CLIP_POP},
        {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list list = {commands, 6, 64, 64};
    assert(cui_sokol_render(r, &list, (cui_color){0, 0, 255, 255}));
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    readback();
    assert(abs((int)pixel(0, 0)[0] - 128) <= 1 && abs((int)pixel(0, 0)[2] - 127) <= 1 &&
           pixel(0, 0)[3] == 255);
    assert(pixel(20, 10)[1] == 255);
    assert(pixel(19, 10)[1] == 0);
    assert(pixel(20, 30)[1] == 0);
    /* Logical coordinates scale into physical framebuffer scissors. */
    cui_draw_command scaled[] = {
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 32, 32}, .color = {255, 255, 255, 255}},
        {.type = CUI_DRAW_CLIP_PUSH, .bounds = {4, 4, 4, 4}},
        {.type = CUI_DRAW_RECT, .bounds = {0, 0, 32, 32}, .color = {0, 255, 0, 255}},
        {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list scaled_list = {scaled, 4, 32, 32};
    assert(cui_sokol_render(r, &scaled_list, (cui_color){0}));
    readback();
    assert(pixel(8, 8)[0] == 0 && pixel(8, 8)[1] == 255 && pixel(7, 8)[0] == 255);
    assert(pixel(15, 15)[0] == 0 && pixel(16, 15)[0] == 255);
    /* Borrowed adapters leave runtime ownership and commit with the host. */
    cui_sokol *borrowed = cui_sokol_create(
        (cui_sokol_config){.max_quads = 1, .target = {fbo, 64, 64}, .borrow_runtime = true});
    assert(borrowed);
    cui_draw_list one = {commands, 1, 64, 64};
    assert(cui_sokol_render(borrowed, &one, (cui_color){0}));
    assert(!cui_sokol_render(borrowed, &one, (cui_color){0}));
    sg_commit();
    assert(!cui_sokol_render(borrowed, &scaled_list, (cui_color){0}));
    cui_sokol_destroy(borrowed);
    assert(sg_isvalid());
    commands[0] = (cui_draw_command){.type = CUI_DRAW_RECT,
                                     .bounds = {8, 8, 48, 48},
                                     .radius = 24,
                                     .color = {255, 255, 255, 255}};
    list.count = 1;
    assert(cui_sokol_render(r, &list, (cui_color){0, 0, 0, 0}));
    readback();
    assert(pixel(8, 8)[3] == 0 && pixel(32, 32)[3] == 255);
    commands[0] = (cui_draw_command){.type = CUI_DRAW_BORDER,
                                     .bounds = {8, 8, 48, 48},
                                     .radius = 8,
                                     .stroke = 3,
                                     .color = {255, 255, 255, 255}};
    assert(cui_sokol_render(r, &list, (cui_color){0}));
    readback();
    assert(pixel(32, 9)[3] == 255 && pixel(32, 32)[3] == 0);
    commands[0] = (cui_draw_command){.type = CUI_DRAW_RECT,
                                     .bounds = {8, 8, 48, 48},
                                     .color = {255, 0, 0, 128},
                                     .custom_corners = true,
                                     .corner_radii = {12, 0, 0, 6},
                                     .custom_border_edges = true,
                                     .border_edges = {0, 4, 2, 6}};
    assert(cui_sokol_render(r, &list, (cui_color){0}));
    readback();
    assert(pixel(8, 8)[3] == 0 && abs((int)pixel(55, 8)[3] - 128) <= 1 &&
           abs((int)pixel(8, 55)[3] - 128) <= 1 && pixel(55, 55)[3] == 0);
    commands[0].type = CUI_DRAW_BORDER;
    assert(cui_sokol_render(r, &list, (cui_color){0}));
    readback();
    assert(pixel(8, 32)[3] == 0 && abs((int)pixel(54, 32)[3] - 128) <= 1 &&
           abs((int)pixel(32, 8)[3] - 128) <= 1 && abs((int)pixel(32, 54)[3] - 128) <= 1 &&
           pixel(32, 32)[3] == 0);
    cui_draw_command shape_clip[] = {{.type = CUI_DRAW_CLIP_PUSH, .bounds = {32, 0, 32, 64}},
                                     commands[0],
                                     {.type = CUI_DRAW_CLIP_POP}};
    cui_draw_list shape_clip_list = {shape_clip, 3, 64, 64};
    assert(cui_sokol_render(r, &shape_clip_list, (cui_color){0}));
    readback();
    assert(pixel(20, 8)[3] == 0 && abs((int)pixel(40, 8)[3] - 128) <= 1);
    commands[0].border_edges = (cui_border_edges){0};
    assert(cui_sokol_render(r, &list, (cui_color){0}));
    readback();
    for (size_t i = 3; i < sizeof(pixels); i += 4)
        assert(pixels[i] == 0);
    commands[0].type = CUI_DRAW_CLIP_POP;
    assert(!cui_sokol_render(r, &list, (cui_color){0}));
    commands[0] = (cui_draw_command){.type = CUI_DRAW_RECT, .bounds = {NAN, 0, 10, 10}};
    assert(!cui_sokol_render(r, &list, (cui_color){0}));
#ifdef CUI_HAS_TEXT
    if (font) {
        commands[0] = (cui_draw_command){.type = CUI_DRAW_TEXT,
                                         .bounds = {0, 0, 64, 30},
                                         .font_size = 18,
                                         .color = {255, 255, 255, 255},
                                         .text = {"office", 6}};
        assert(cui_sokol_render(r, &list, (cui_color){0}));
        readback();
        unsigned coverage = 0;
        for (size_t i = 0; i < sizeof(pixels); i += 4)
            coverage += pixels[i + 3] > 0;
        assert(coverage > 0);
        cui_sokol_stats before = cui_sokol_get_stats(r);
        commands[0].color = (cui_color){20, 40, 200, 255};
        assert(cui_sokol_render(r, &list, (cui_color){0}));
        cui_sokol_stats after = cui_sokol_get_stats(r);
        assert(after.misses == before.misses && after.hits > before.hits);
        assert(after.entries <= 256 && after.coverage_bytes <= 4u * 1024u * 1024u);
        for (unsigned size = 8; size < 80; size++) {
            commands[0].font_size = (float)size;
            assert(cui_sokol_render(r, &list, (cui_color){0}));
        }
        cui_sokol_stats stressed = cui_sokol_get_stats(r);
        assert(stressed.misses > 256 && stressed.entries <= 256 &&
               stressed.coverage_bytes <= 4u * 1024u * 1024u);
        puts("Sokol shaped text pixel readback, cache tint reuse and eviction passed");
    } else
        puts("text readback skipped: no test font found");
#endif
    commands[0] = (cui_draw_command){
        .type = CUI_DRAW_RECT, .bounds = {0, 0, 1, 1}, .color = {20, 40, 60, 128}};
    assert(cui_sokol_render(r, &list, (cui_color){0}));
    readback();
    assert(abs((int)pixel(0, 0)[3] - 128) <= 1);
    printf("Sokol GLCORE; vendor=%s; renderer=%s; nested clip, blend, rounded "
           "geometry/readback and runtime ownership passed\n",
           glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    cui_sokol_destroy(r);
    assert(!sg_isvalid());
#ifdef CUI_HAS_TEXT
    cui_font_close(font);
#endif
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
