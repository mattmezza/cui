/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
static void frame(cui_context *ui) {
    assert(cui_begin_frame(ui, 320, 160, 0));
    cui_begin_box(ui, 100, (cui_box){.width = 320, .height = 160});
    for (int row = 0; row < 4; ++row) {
        cui_begin_box(ui, (cui_id)(row + 1), (cui_box){.width = 320, .height = 40});
        (void)cui_interact(ui, (cui_id)(row + 1), CUI_ROLE_BUTTON, cui_string("row"), false, false,
                           0);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    (void)cui_end_frame(ui);
}
int main(void) {
    cui_sdl3 *host = cui_sdl3_create("hover probe", 320, 160, NULL);
    cui_context *ui = cui_create((cui_config){0});
    assert(host && ui);
    frame(ui);
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    assert(windows && count == 1);
    SDL_WarpMouseInWindow(windows[0], 20, 140);
    SDL_free(windows);
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    SDL_Event enter = {.type = SDL_EVENT_WINDOW_MOUSE_ENTER};
    assert(SDL_PushEvent(&enter));
    SDL_Event button = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    button.button.button = SDL_BUTTON_LEFT;
    button.button.x = button.button.y = 20;
    assert(SDL_PushEvent(&button));
    button.type = SDL_EVENT_MOUSE_BUTTON_UP;
    assert(SDL_PushEvent(&button));
    /* A motion burst exceeds Core's bounded event queue. Latest position must
     * reach the very next frame, without dropping intervening click events. */
    for (int i = 0; i < 3000; ++i) {
        SDL_Event e = {.type = SDL_EVENT_MOUSE_MOTION};
        e.motion.x = 20;
        e.motion.y = i == 2999 ? 140 : 20;
        assert(SDL_PushEvent(&e));
    }
    assert(cui_sdl3_poll(host, ui));
    frame(ui);
    assert(cui_hovered(ui, 4) && !cui_hovered(ui, 1));
    size_t count_events;
    const cui_event *events = cui_frame_events(ui, &count_events);
    bool down = false, up = false;
    for (size_t i = 0; i < count_events; ++i) {
        if (events[i].type == CUI_EVENT_POINTER_DOWN)
            down = true;
        if (events[i].type == CUI_EVENT_POINTER_UP) {
            assert(down);
            up = true;
        }
    }
    assert(down && up && count_events < 10);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    cui_sdl3_destroy(host);
    return 0;
}
