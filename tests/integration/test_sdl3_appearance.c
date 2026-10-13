/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
static cui_system_appearance actual(void) {
    switch (SDL_GetSystemTheme()) {
    case SDL_SYSTEM_THEME_LIGHT:
        return CUI_SYSTEM_APPEARANCE_LIGHT;
    case SDL_SYSTEM_THEME_DARK:
        return CUI_SYSTEM_APPEARANCE_DARK;
    default:
        return CUI_SYSTEM_APPEARANCE_UNKNOWN;
    }
}
static void frame(cui_context *ui) {
    assert(cui_begin_frame(ui, 120, 80, 0));
    (void)cui_end_frame(ui);
}
int main(void) {
    assert(SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy"));
    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"));
    cui_sdl3 *host = cui_sdl3_create("appearance", 120, 80, NULL);
    assert(host);
    cui_context *ui = cui_create((cui_config){.width = 120, .height = 80});
    assert(ui);
    cui_platform_appearance observed,
        sentinel = {.appearance = CUI_SYSTEM_APPEARANCE_DARK, .capabilities = 99, .revision = 42};
    observed = sentinel;
    assert(!cui_sdl3_appearance(NULL, &observed) && observed.appearance == sentinel.appearance &&
           observed.capabilities == sentinel.capabilities &&
           observed.revision == sentinel.revision);
    assert(!cui_sdl3_appearance(host, NULL));
    assert(cui_sdl3_appearance(host, &observed) && observed.appearance == actual() &&
           observed.capabilities == CUI_APPEARANCE_SYSTEM_THEME && observed.revision == 1);
    assert(cui_sdl3_poll(host, ui));
    frame(ui);
    uint64_t revision = observed.revision;
    assert(cui_sdl3_poll(host, ui));
    assert(cui_sdl3_appearance(host, &observed) && observed.revision == revision);
    assert(!cui_needs_redraw(ui));
    SDL_Event event = {.type = SDL_EVENT_SYSTEM_THEME_CHANGED};
    assert(SDL_PushEvent(&event));
    assert(SDL_PushEvent(&event));
    assert(cui_sdl3_poll(host, ui));
    assert(cui_sdl3_appearance(host, &observed) && observed.appearance == actual() &&
           observed.revision == revision + 2 && cui_needs_redraw(ui));
    frame(ui);
    assert(cui_sdl3_poll(host, ui));
    assert(cui_sdl3_appearance(host, &observed) && observed.revision == revision + 2 &&
           !cui_needs_redraw(ui));
    cui_destroy(ui);
    cui_sdl3_destroy(host);
    /* A fresh instance owns a fresh initial snapshot, no retained UI pointers. */
    host = cui_sdl3_create("appearance-again", 120, 80, NULL);
    assert(host && cui_sdl3_appearance(host, &observed) && observed.revision == 1 &&
           observed.appearance == actual());
    cui_sdl3_destroy(host);
    return 0;
}
