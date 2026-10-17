#ifdef CUI_TEST_BRIDGE_PORTABLE
#include "cui/cui_bridge.h"
#include <assert.h>
#include <stddef.h>
int main(void) {
    cui_bridge_status status;
    cui_bridge_options options = {.struct_size = cui_bridge_options_size(),
                                  .abi_version = cui_bridge_abi_version(),
                                  .width = 320,
                                  .height = 160};
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_ROW, "root", &root) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_run(app, NULL, NULL) == CUI_BRIDGE_BACKEND_UNAVAILABLE);
    assert(cui_bridge_step(app, 0) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
    return 0;
}
#elif defined(CUI_TEST_BRIDGE_RUN)
/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
typedef struct {
    cui_bridge_app *app;
    const char *mode;
    unsigned calls;
} probe;
static cui_bridge_status after_frame(void *user) {
    probe *p = user;
    p->calls++;
    if (!strcmp(p->mode, "error"))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (!strcmp(p->mode, "reentrant")) {
        assert(cui_bridge_run(p->app, NULL, NULL) == CUI_BRIDGE_INVALID_STATE);
        assert(cui_bridge_step(p->app, 0) == CUI_BRIDGE_INVALID_STATE);
    }
    if (!strcmp(p->mode, "destroy")) {
        cui_bridge_destroy(p->app);
        cui_bridge_destroy(p->app); /* Both defer while callback owns the app. */
    } else
        assert(cui_bridge_request_close(p->app) == CUI_BRIDGE_OK);
    return CUI_BRIDGE_OK;
}
static Uint32 SDLCALL close_timer(void *user, SDL_TimerID timer, Uint32 interval) {
    (void)user;
    (void)timer;
    (void)interval;
    SDL_Event event = {.type = SDL_EVENT_QUIT};
    assert(SDL_PushEvent(&event));
    return 0;
}
int main(int argc, char **argv) {
    assert(argc == 3 || argc == 4);
    cui_bridge_status status;
    cui_bridge_options options = {.struct_size = cui_bridge_options_size(),
                                  .abi_version = cui_bridge_abi_version(),
                                  .backend = CUI_BRIDGE_BACKEND_SDL,
                                  .width = 320,
                                  .height = 160,
                                  .font_path_utf8 = argv[2]};
    probe p = {.mode = argv[1]};
    p.app = cui_bridge_create(&options, &status);
    assert(p.app && status == CUI_BRIDGE_OK);
    cui_bridge_node root;
    assert(cui_bridge_node_create(p.app, (cui_bridge_node){0}, CUI_BRIDGE_ROW, "root", &root) ==
           CUI_BRIDGE_OK);
    cui_bridge_node button;
    assert(cui_bridge_node_create(p.app, root, CUI_BRIDGE_BUTTON, "save", &button) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(p.app, button, "Save changes") == CUI_BRIDGE_OK);
    if (!strcmp(p.mode, "unicode")) {
        assert(argc == 4);
        const char *fallbacks[] = {argv[3]};
        assert(cui_bridge_set_font_fallbacks(p.app, fallbacks, 1) == CUI_BRIDGE_OK);
        assert(cui_bridge_set_text(p.app, button, "Save ✓") == CUI_BRIDGE_OK);
    }
    bool no_callback = !strcmp(p.mode, "none");
    if (no_callback)
        assert(SDL_AddTimer(50, close_timer, NULL));
    status = cui_bridge_run(p.app, no_callback ? NULL : after_frame, &p);
    assert(status == (!strcmp(p.mode, "error") ? CUI_BRIDGE_INVALID_ARGUMENT : CUI_BRIDGE_CLOSED));
    assert(no_callback ? p.calls == 0 : p.calls == 1);
    if (strcmp(p.mode, "destroy")) {
        assert(cui_bridge_step(p.app, 0) == CUI_BRIDGE_CLOSED);
        cui_bridge_destroy(p.app);
    }
    return 0;
}
#else
/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stddef.h>
typedef struct {
    cui_sdl3 *host;
    cui_context *ui;
    unsigned frames;
} probe;
static bool init(void *user, int argc, char **argv, cui_sdl3 **host, cui_context **ui) {
    (void)argc;
    (void)argv;
    probe *p = user;
    *host = p->host = cui_sdl3_create("resize probe", 320, 160, NULL);
    *ui = p->ui = cui_create((cui_config){0});
    return *host && *ui;
}
static int frame(void *user, cui_sdl3 *host, cui_context *ui) {
    probe *p = user;
    assert(SDL_IsMainThread());
    assert(cui_sdl3_poll(host, ui));
    float w, h;
    cui_sdl3_size(host, &w, &h);
    if (p->frames) {
        assert(w == 480 && h == 240);
        assert(cui_needs_redraw(ui));
    }
    assert(cui_begin_frame(ui, w, h, 0));
    cui_begin_box(ui, 1, (cui_box){.width = -1, .height = -1, .background = {30, 100, 200, 255}});
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_bounds(ui, 1).w == w && cui_bounds(ui, 1).h == h);
    assert(cui_sdl3_render(host, &draw, (cui_color){0}));
    if (++p->frames == 2)
        return 1;
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    assert(windows && count == 1);
    assert(SDL_SetWindowSize(windows[0], 480, 240));
    assert(SDL_SyncWindow(windows[0]));
    SDL_Event event = {.type = SDL_EVENT_WINDOW_EXPOSED};
    event.window.windowID = SDL_GetWindowID(windows[0]);
    event.window.data1 = 1; /* Same expose payload as SDL's modal resize tick. */
    assert(SDL_PushEvent(&event));
    SDL_free(windows);
    return 0;
}
static void quit(void *user, bool success) {
    probe *p = user;
    assert(success && p->frames == 2 && SDL_IsMainThread());
    cui_destroy(p->ui);
    cui_sdl3_destroy(p->host);
}
int main(int argc, char **argv) {
    probe p = {0};
    return cui_sdl3_run(argc, argv, (cui_sdl3_app){&p, init, frame, quit});
}
#endif
