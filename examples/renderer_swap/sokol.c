/* SPDX-License-Identifier: MIT */
/* The host owns SDL windowing. The adapter receives only a current GL context
 * and a framebuffer descriptor; it has no SDL dependency. */
#include "cui/cui_components.h"
#include "cui/cui_sokol.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static cui_key key(SDL_Keycode k) {
    switch (k) {
    case SDLK_TAB:
        return CUI_KEY_TAB;
    case SDLK_RETURN:
        return CUI_KEY_ENTER;
    case SDLK_SPACE:
        return CUI_KEY_SPACE;
    case SDLK_ESCAPE:
        return CUI_KEY_ESCAPE;
    case SDLK_Z:
        return CUI_KEY_Z;
    case SDLK_Y:
        return CUI_KEY_Y;
    case SDLK_C:
        return CUI_KEY_C;
    case SDLK_X:
        return CUI_KEY_X;
    case SDLK_V:
        return CUI_KEY_V;
    default:
        return CUI_KEY_NONE;
    }
}
static cui_font *open_font(const char *path) {
#ifdef CUI_HAS_TEXT
    const char *paths[] = {path,
                           getenv("CUI_FONT_PATH"),
                           "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                           "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                           "/usr/share/fonts/TTF/DejaVuSans.ttf",
                           "/System/Library/Fonts/Supplemental/Arial.ttf",
                           "C:/Windows/Fonts/segoeui.ttf"};
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++)
        if (paths[i]) {
            cui_font *f = cui_font_open(paths[i]);
            if (f)
                return f;
        }
#else
    (void)path;
#endif
    return NULL;
}
static bool save_ppm(int w, int h, const char *path) {
    if (w <= 0 || h <= 0 || w > 16384 || h > 16384)
        return false;
    unsigned char *pixels = malloc((size_t)w * h * 4);
    if (!pixels)
        return false;
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    if (glGetError() != GL_NO_ERROR) {
        free(pixels);
        return false;
    }
    FILE *f = fopen(path, "wb");
    if (!f) {
        free(pixels);
        return false;
    }
    bool ok = fprintf(f, "P6\n%d %d\n255\n", w, h) > 0;
    for (int y = h - 1; y >= 0 && ok; y--)
        for (int x = 0; x < w && ok; x++)
            ok = fwrite(pixels + ((size_t)y * w + x) * 4, 1, 3, f) == 3;
    if (fclose(f))
        ok = false;
    free(pixels);
    return ok;
}
int main(int argc, char **argv) {
    unsigned smoke = 0;
    const char *capture = NULL, *font_path = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--smoke") && i + 1 < argc)
            smoke = (unsigned)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--screenshot") && i + 1 < argc)
            capture = argv[++i];
        else if (!strcmp(argv[i], "--font") && i + 1 < argc)
            font_path = argv[++i];
        else {
            fprintf(stderr, "Usage: %s [--smoke N] [--screenshot FILE.ppm] [--font FILE]\n",
                    argv[0]);
            return 2;
        }
    }
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
    SDL_Window *window =
        SDL_CreateWindow("CUI / independent Sokol renderer", 960, 600,
                         SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_GLContext gl = window ? SDL_GL_CreateContext(window) : NULL;
    if (!gl) {
        fprintf(stderr, "GL host: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    printf("video=%s; Sokol GLCORE; GL vendor=%s; GL renderer=%s\n", SDL_GetCurrentVideoDriver(),
           glGetString(GL_VENDOR), glGetString(GL_RENDERER));
    cui_font *font = open_font(font_path);
    cui_config config = {.width = 960, .height = 600};
#ifdef CUI_HAS_TEXT
    if (!font) {
        fprintf(stderr, "Text disabled: pass --font or set CUI_FONT_PATH.\n");
    } else {
        config.measure = cui_font_measure;
        config.measure_user = font;
    }
#endif
    cui_context *ui = cui_create(config);
    int pw, ph;
    SDL_GetWindowSizeInPixels(window, &pw, &ph);
    cui_sokol *renderer = cui_sokol_create(
        (cui_sokol_config){.font = font, .target = {0, (unsigned)pw, (unsigned)ph}});
    if (!ui || !renderer) {
        fprintf(stderr, "CUI/Sokol initialization failed\n");
        cui_sokol_destroy(renderer);
        cui_destroy(ui);
#ifdef CUI_HAS_TEXT
        cui_font_close(font);
#endif
        SDL_GL_DestroyContext(gl);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    bool dark = true, quit = false, ok = true;
    unsigned frames = 0;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            cui_event event = {0};
            bool send = true;
            switch (e.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                quit = true;
                send = false;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                event.type = CUI_EVENT_POINTER_MOVE;
                event.x = e.motion.x;
                event.y = e.motion.y;
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (e.button.button != SDL_BUTTON_LEFT && e.button.button != SDL_BUTTON_RIGHT &&
                    e.button.button != SDL_BUTTON_MIDDLE) {
                    send = false;
                    break;
                }
                event.type = e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? CUI_EVENT_POINTER_DOWN
                                                                   : CUI_EVENT_POINTER_UP;
                event.pointer_button = e.button.button == SDL_BUTTON_RIGHT ? CUI_POINTER_SECONDARY
                                       : e.button.button == SDL_BUTTON_MIDDLE ? CUI_POINTER_MIDDLE
                                                                              : CUI_POINTER_PRIMARY;
                event.x = e.button.x;
                event.y = e.button.y;
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                event.type = e.type == SDL_EVENT_KEY_DOWN ? CUI_EVENT_KEY_DOWN : CUI_EVENT_KEY_UP;
                event.key = key(e.key.key);
                event.repeat = e.key.repeat;
                event.modifiers = (e.key.mod & SDL_KMOD_SHIFT) ? CUI_MOD_SHIFT : 0;
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                event.type = CUI_EVENT_CANCEL;
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            case SDL_EVENT_WINDOW_EXPOSED:
                cui_invalidate(ui);
                send = false;
                break;
            default:
                send = false;
                break;
            }
            if (send)
                cui_queue_event(ui, event);
        }
        if (quit)
            break;
        if (frames && !smoke && !cui_needs_redraw(ui)) {
            SDL_Delay(8);
            continue;
        }
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_GetWindowSizeInPixels(window, &pw, &ph);
        if (w <= 0 || h <= 0 || pw <= 0 || ph <= 0) {
            SDL_Delay(8);
            continue;
        }
        if (!cui_sokol_set_target(renderer, (cui_sokol_target){0, (unsigned)pw, (unsigned)ph})) {
            ok = false;
            break;
        }
        cui_theme theme = dark ? cui_theme_dark() : cui_theme_light();
        cui_begin_frame(ui, (float)w, (float)h, 0.016f);
        cui_begin_box(ui, cui_id_from(ui, "root"),
                      (cui_box){.width = (float)w,
                                .height = (float)h,
                                .padding = 32,
                                .gap = 24,
                                .background = theme.background,
                                .clip = true});
        if (font) {
            cui_text(ui, cui_string("An independent renderer. The same C UI."), 26, theme.text);
            cui_label(ui, &theme,
                      cui_string("Host-owned OpenGL context / Sokol GLCORE / CUI "
                                 "draw-list contract"));
            bool before = dark;
            cui_switch(ui, cui_id_from(ui, "theme"), &theme, cui_string("Dark theme"), &dark,
                       false);
            if (before != dark)
                cui_invalidate(ui);
        }
        cui_begin_box(ui, cui_id_from(ui, "specimen"),
                      (cui_box){.width = -1,
                                .height = -1,
                                .padding = 24,
                                .gap = 18,
                                .background = theme.surface,
                                .radius = 16,
                                .border = theme.border,
                                .border_width = 1});
        if (font) {
            cui_text(ui, cui_string("Solid, rounded, clipped, shaped."), 22, theme.text);
            cui_label(ui, &theme,
                      cui_string("This frame is drawn by Sokol, without SDL_Renderer "
                                 "or SDL_GPU."));
            cui_label(ui, &theme,
                      cui_string("Text coverage is shaped with HarfBuzz and cached "
                                 "as immutable GPU textures."));
            cui_label(ui, &theme,
                      cui_string("Tab focuses the theme switch. Space changes the "
                                 "application-owned value."));
        }
        cui_begin_box(ui, cui_id_from(ui, "sample_row"),
                      (cui_box){.width = -1, .height = 96, .row = true, .gap = 20});
        for (unsigned i = 0; i < 3; i++) {
            char name[32];
            snprintf(name, sizeof(name), "sample_%u", i);
            cui_begin_box(
                ui, cui_id_from(ui, name),
                (cui_box){.width = 180,
                          .height = 80,
                          .padding = 16,
                          .radius = 8 + 4 * i,
                          .background = i == 0   ? theme.primary
                                        : i == 1 ? theme.border
                                                 : (cui_color){theme.primary.r, theme.primary.g,
                                                               theme.primary.b, 120}});
            if (font)
                cui_text(ui,
                         cui_string(i == 0   ? "Solid"
                                    : i == 1 ? "Rounded"
                                             : "Alpha"),
                         18, i == 0 ? theme.on_primary : theme.text);
            cui_end_box(ui);
        }
        cui_end_box(ui);
        cui_end_box(ui);
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        if (!cui_sokol_render(renderer, &list, theme.background)) {
            fprintf(stderr, "Render: %s\n", cui_sokol_error(renderer));
            ok = false;
            break;
        }
        frames++;
        if (smoke && frames >= smoke) {
            if (capture)
                ok = save_ppm(pw, ph, capture);
            SDL_GL_SwapWindow(window);
            break;
        }
        SDL_GL_SwapWindow(window);
    }
    cui_sokol_stats stats = cui_sokol_get_stats(renderer);
    printf("frames=%u; quads=%zu; glyph cache entries=%zu hits=%llu misses=%llu\n", frames,
           stats.quads, stats.entries, (unsigned long long)stats.hits,
           (unsigned long long)stats.misses);
    cui_sokol_destroy(renderer);
    cui_destroy(ui);
#ifdef CUI_HAS_TEXT
    cui_font_close(font);
#endif
    SDL_GL_DestroyContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return ok ? 0 : 1;
}
