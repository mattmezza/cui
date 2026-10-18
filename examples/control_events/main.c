/* SPDX-License-Identifier: MIT */
#include "app.h"
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    ce_app app;
    cui_sdl3 *host;
    cui_context *ui;
    cui_font *font;
    bool smoke, captured;
    unsigned stage;
    Uint32 window;
    const char *capture;
} ce_host;
static void click(ce_host *h, cui_id id) {
    cui_rect b = cui_bounds(h->ui, id);
    assert(b.w > 0 && b.h > 0);
    SDL_Event event = {.type = SDL_EVENT_MOUSE_MOTION};
    event.motion.windowID = h->window;
    event.motion.x = b.x + 5;
    event.motion.y = b.y + 5;
    assert(SDL_PushEvent(&event));
    event = (SDL_Event){.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.windowID = h->window;
    event.button.button = SDL_BUTTON_LEFT;
    event.button.x = b.x + 5;
    event.button.y = b.y + 5;
    event.button.down = true;
    assert(SDL_PushEvent(&event));
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.down = false;
    assert(SDL_PushEvent(&event));
}
static void space(ce_host *h) {
    /* Source Checkbox intentionally avoids pointer focus. Explicitly place
     * keyboard focus for this deterministic keyboard path, then use SDL keys. */
    cui_focus(h->ui, CE_SETTING);
    SDL_Event event = {.type = SDL_EVENT_KEY_DOWN};
    event.key.windowID = h->window;
    event.key.key = SDLK_SPACE;
    event.key.scancode = SDL_SCANCODE_SPACE;
    event.key.down = true;
    assert(SDL_PushEvent(&event));
    event.type = SDL_EVENT_KEY_UP;
    event.key.down = false;
    assert(SDL_PushEvent(&event));
}
static bool initialize(void *user, int argc, char **argv, cui_sdl3 **host, cui_context **ui) {
    ce_host *h = user;
    const char *font = NULL;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--font") && i + 1 < argc)
            font = argv[++i];
        else if (!strcmp(argv[i], "--capture") && i + 1 < argc)
            h->capture = argv[++i];
        else if (!strcmp(argv[i], "--smoke"))
            h->smoke = true;
        else {
            fprintf(stderr, "Usage: cui_control_events --font PATH [--smoke --capture BMP]\n");
            return false;
        }
    }
    if (!font || (h->capture && !h->smoke))
        return false;
    ce_init(&h->app);
    h->font = cui_font_open(font);
    if (!h->font)
        return false;
    h->host = cui_sdl3_create("CUI control events", 760, 460, h->font);
    if (!h->host)
        return false;
    h->ui = cui_create((cui_config){.max_elements = 128,
                                    .max_commands = 256,
                                    .frame_bytes = 65536,
                                    .measure = cui_font_measure,
                                    .measure_styled = cui_font_measure_styled,
                                    .text_geometry = cui_font_text_geometry,
                                    .text_geometry_line = cui_font_text_geometry_line,
                                    .font_metrics = cui_font_get_metrics,
                                    .measure_user = h->font,
                                    .text_capabilities = cui_font_text_capabilities()});
    if (!h->ui)
        return false;
    int count = 0;
    SDL_Window **windows = SDL_GetWindows(&count);
    if (!windows || count != 1) {
        SDL_free(windows);
        return false;
    }
    h->window = SDL_GetWindowID(windows[0]);
    SDL_free(windows);
    *host = h->host;
    *ui = h->ui;
    return true;
}
static void metadata(const cui_component_event_metadata *event, cui_id id,
                     cui_component_event_source source) {
    assert(event->target == id && event->source == source);
    assert((event->flags & (CUI_COMPONENT_EVENT_HAS_FRAME | CUI_COMPONENT_EVENT_HAS_INDEX |
                            CUI_COMPONENT_EVENT_HAS_ACTIVATION)) ==
           (CUI_COMPONENT_EVENT_HAS_FRAME | CUI_COMPONENT_EVENT_HAS_INDEX |
            CUI_COMPONENT_EVENT_HAS_ACTIVATION));
}
static bool visible_pixels(const char *path) {
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface)
        return false;
    size_t ink = 0;
    for (int y = 0; y < surface->h; ++y)
        for (int x = 0; x < surface->w; ++x) {
            Uint8 r, g, b, a;
            if (!SDL_ReadSurfacePixel(surface, x, y, &r, &g, &b, &a)) {
                SDL_DestroySurface(surface);
                return false;
            }
            ink += r < 160 && g < 160 && b < 160;
        }
    SDL_DestroySurface(surface);
    return ink > 200;
}
static int frame(void *user, cui_sdl3 *host, cui_context *ui) {
    ce_host *h = user;
    assert(SDL_IsMainThread());
    float width, height;
    cui_sdl3_size(host, &width, &height);
    unsigned requests = h->app.requests, actions = h->app.actions;
    bool allow = h->app.allow_toggle;
    if (!cui_begin_frame(ui, width, height, cui_sdl3_frame_delta(host)))
        return -1;
    cui_theme theme = cui_theme_light();
    ce_frame_result result = ce_scene(ui, &h->app, &theme);
    /* Copy/inspect per-call results; all side effects happen in callbacks. */
    ce_frame_result inspected = result;
    assert(inspected.apply.activation_count == result.apply.activation_count);
    assert(actions + result.apply.activation_count == h->app.actions);
    assert(requests + result.setting.request_count == h->app.requests);
    cui_draw_list list = cui_end_frame(ui);
    bool capture = h->smoke && h->stage == 5 && h->app.actions == 1 && h->capture;
    bool rendered = capture ? cui_sdl3_render_capture(host, &list, theme.background, h->capture)
                            : cui_sdl3_render(host, &list, theme.background);
    if (!rendered || cui_last_error(ui) != CUI_OK)
        return -1;
    if (capture)
        h->captured = visible_pixels(h->capture);
    /* Resource teardown/omission is requested by a callback and applied only
     * after the completed frame's renderer consumed its borrowed draw data. */
    if (ce_finish_frame(&h->app) || requests != h->app.requests || actions != h->app.actions ||
        allow != h->app.allow_toggle)
        cui_invalidate(ui);
    if (!h->smoke)
        return 0;
    switch (h->stage) {
    case 0:
        click(h, CE_SETTING);
        h->stage = 1;
        break;
    case 1:
        if (h->app.requests != 1)
            break;
        assert(h->app.setting == CUI_CHECK_UNCHECKED && h->app.rejected == 1);
        metadata(&h->app.last_request.event, CE_SETTING, CUI_COMPONENT_EVENT_SOURCE_POINTER);
        click(h, CE_ALLOW);
        h->stage = 2;
        break;
    case 2:
        if (!h->app.allow_toggle)
            break;
        click(h, CE_SETTING);
        h->stage = 3;
        break;
    case 3:
        if (h->app.requests != 2)
            break;
        assert(h->app.setting == CUI_CHECK_CHECKED && h->app.rejected == 1);
        assert(h->app.last_request.current == CUI_CHECK_UNCHECKED);
        space(h);
        h->stage = 4;
        break;
    case 4:
        if (h->app.requests != 3)
            break;
        assert(h->app.setting == CUI_CHECK_UNCHECKED);
        metadata(&h->app.last_request.event, CE_SETTING, CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
        click(h, CE_APPLY);
        h->stage = 5;
        break;
    case 5:
        if (h->app.actions != 1)
            break;
        assert(result.apply.activation_count == 1 && result.apply.request_count == 0);
        metadata(&h->app.last_activation, CE_APPLY, CUI_COMPONENT_EVENT_SOURCE_POINTER);
        h->stage = 6;
        cui_invalidate(ui);
        break;
    case 6:
        assert(h->app.actions == 1 && result.apply.activation_count == 0 &&
               result.setting.request_count == 0);
        click(h, CE_CLOSE);
        h->stage = 7;
        break;
    case 7:
        if (h->app.visible)
            break;
        /* Close request was applied after this frame. Wait for actual omitted
         * submission before examining the next semantic tree. */
        h->stage = 8;
        cui_invalidate(ui);
        break;
    case 8: {
        size_t count;
        const cui_semantic *nodes = cui_semantics(ui, &count);
        for (size_t i = 0; i < count; ++i)
            assert(nodes[i].id != CE_SETTING && nodes[i].id != CE_ALLOW &&
                   nodes[i].id != CE_APPLY && nodes[i].id != CE_CLOSE);
        assert(h->app.actions == 1 && h->app.requests == 3 && h->app.closes == 1);
        assert(!h->capture || h->captured);
        puts("PASS SDL pointer/key owner accept/reject, metadata, passive results, pixels and "
             "deferred omission");
        return 1;
    }
    default:
        return -1;
    }
    return 0;
}
static void shutdown(void *user, bool success) {
    ce_host *h = user;
    if (!success)
        fprintf(stderr, "Control events failed: %s\n", cui_sdl3_error());
    cui_destroy(h->ui);
    cui_sdl3_destroy(h->host);
    cui_font_close(h->font);
}
int main(int argc, char **argv) {
    ce_host host = {0};
    return cui_sdl3_run(argc, argv, (cui_sdl3_app){&host, initialize, frame, shutdown});
}
