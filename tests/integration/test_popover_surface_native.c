/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "cui/cui_sokol.h"
#include "cui/cui_surface.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <assert.h>
#include <stdio.h>
int main(void) {
    assert(SDL_Init(SDL_INIT_VIDEO));
    cui_sdl3 *sdl = cui_sdl3_create("Popover mesh gap", 400, 320, NULL);
    assert(sdl);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window *window =
        SDL_CreateWindow("Popover Sokol anchor readback", 400, 320, SDL_WINDOW_OPENGL);
    assert(window);
    SDL_GLContext gl = SDL_GL_CreateContext(window);
    assert(gl);
    GLuint fbo, texture;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 400, 320, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    cui_sokol *sokol =
        cui_sokol_create((cui_sokol_config){.max_quads = 256, .target = {fbo, 400, 320}});
    assert(sokol);
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 320});
    assert(ui);
    cui_theme t = cui_theme_light();
    t.popover = (cui_color){255, 0, 0, 255};
    t.radius = 9;
    cui_surface_state s;
    cui_surface_init(&s, 100, CUI_SURFACE_POPOVER);
    cui_surface_open(ui, &s, 0);
    for (unsigned anchor = 0; anchor < 8; ++anchor) {
        assert(cui_begin_frame(ui, 400, 320, .016f));
        cui_surface_options o = {.viewport = {0, 0, 400, 320},
                                 .anchor = {180, 140, 40, 20},
                                 .width = 120,
                                 .height = 100,
                                 .hide_close = true,
                                 .popover_appearance = CUI_POPOVER_APPEARANCE_SOURCE};
        assert(cui_popover_begin(ui, &t, &s, (cui_str){0}, (cui_str){0}, o,
                                 (cui_popover_position){.mode = CUI_POPOVER_POSITION_ANCHOR,
                                                        .anchor = (cui_popover_anchor)anchor,
                                                        .arrow = true}));
        cui_surface_end(ui);
        cui_draw_list list = cui_end_frame(ui);
        assert(list.count);
        assert(SDL_GL_MakeCurrent(window, gl));
        assert(cui_sokol_render(sokol, &list, (cui_color){255, 255, 255, 255}));
        unsigned char pixels[400 * 320 * 4];
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glReadPixels(0, 0, 400, 320, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        assert(glGetError() == GL_NO_ERROR);
        /* Independent seam center samples span both sides of each base edge.
         * Avoid slope boundary AA, which remains a documented source gap. */
        const unsigned centers[8][2] = {{195, 170}, {200, 170}, {205, 170}, {195, 130},
                                        {200, 130}, {205, 130}, {230, 150}, {170, 150}};
        for (unsigned i = 0; i < 2; ++i) {
            unsigned x = centers[anchor][0], y = centers[anchor][1];
            if (anchor < 6)
                y -= i;
            else
                x -= i;
            const unsigned char *pixel = pixels + ((size_t)(319 - y) * 400 + x) * 4;
            assert(pixel[0] > 245 && pixel[1] < 10 && pixel[2] < 10);
        }
        if (anchor == 0) {
            /* Baseline SDL3 handles ordinary rect commands, but rejects Core
             * mesh commands. Keep this gap explicit until adapter follow-up. */
            cui_draw_command rect = {
                .type = CUI_DRAW_RECT, .bounds = {0, 0, 10, 10}, .color = {255, 0, 0, 255}};
            cui_draw_list simple = {&rect, 1, 400, 320};
            assert(cui_sdl3_render(sdl, &simple, (cui_color){255, 255, 255, 255}));
            const cui_draw_command *arrow = NULL;
            for (size_t i = 0; i < list.count; ++i)
                if (list.commands[i].type == CUI_DRAW_MESH) {
                    arrow = &list.commands[i];
                    break;
                }
            assert(arrow);
            simple.commands = arrow;
            assert(!cui_sdl3_render(sdl, &simple, (cui_color){255, 255, 255, 255}));
        }
    }
    assert(SDL_GL_MakeCurrent(window, gl));
    printf("Popover Sokol GLCORE: %s; SDL mesh unsupported as expected\n",
           glGetString(GL_RENDERER));
    cui_destroy(ui);
    cui_sokol_destroy(sokol);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);
    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(window);
    cui_sdl3_destroy(sdl);
    SDL_Quit();
    return 0;
}
