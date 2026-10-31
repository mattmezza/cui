/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
int main(void) {
    assert(!cui_sdl3_set_visible(NULL, true));
    assert(!cui_sdl3_set_visible(NULL, false));
    assert(!cui_sdl3_is_visible(NULL));
    cui_sdl3 *host =
        cui_sdl3_create_with_options("hidden creation proof", 120, 80, NULL,
                                     (cui_sdl3_window_options){.initially_hidden = true});
    assert(host);
    assert(!cui_sdl3_is_visible(host));
    /* SDL's actual window flag corroborates the public query. */
    int count = 0;
    SDL_Window **windows = SDL_GetWindows(&count);
    assert(windows && count == 1);
    SDL_Window *window = windows[0];
    SDL_free(windows);
    assert(window);
    assert(SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN);
    assert(cui_sdl3_set_visible(host, true));
    assert(cui_sdl3_is_visible(host));
    assert(!(SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN));
    assert(cui_sdl3_set_visible(host, false));
    assert(!cui_sdl3_is_visible(host));
    cui_sdl3_destroy(host);
    host =
        cui_sdl3_create_with_options("zero visible", 120, 80, NULL, (cui_sdl3_window_options){0});
    assert(host && cui_sdl3_is_visible(host));
    cui_sdl3_destroy(host);
    host = cui_sdl3_create("legacy visible", 120, 80, NULL);
    assert(host && cui_sdl3_is_visible(host));
    cui_sdl3_destroy(host);
    return 0;
}
