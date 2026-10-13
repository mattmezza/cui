/* SPDX-License-Identifier: MIT */
/* Native adapter evidence uses queued SDL events, not a physical device. */
#include "../web/button_group_probe.c"
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static void mouse_click(float x, float y) {
    SDL_Event event = {.type = SDL_EVENT_MOUSE_BUTTON_DOWN};
    event.button.button = SDL_BUTTON_LEFT;
    event.button.which = 1;
    event.button.x = x;
    event.button.y = y;
    assert(SDL_PushEvent(&event));
    event.type = SDL_EVENT_MOUSE_BUTTON_UP;
    assert(SDL_PushEvent(&event));
}
int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : getenv("CUI_TEST_FONT");
    if (!path) {
        fputs("Set CUI_TEST_FONT or pass FONT [BMP]\n", stderr);
        return 77;
    }
    cui_font *font = cui_font_open(path);
    assert(font);
    cui_sdl3 *backend = cui_sdl3_create("ButtonGroup adapter proof", 360, 300, font);
    assert(backend);
    assert(!probe_init(0));
    assert(cui_sdl3_poll(backend, ui));
    assert(!probe_frame());
    assert(!probe_frame());
    mouse_click(270, 36);
    mouse_click(70, 36);
    assert(cui_sdl3_poll(backend, ui));
    assert(!probe_frame());
    assert(requests == 2 && order[0] == 2 && order[1] == 0 && selected[0] && !children);
    assert(cui_sdl3_render(backend, &draw, (cui_color){245, 245, 245, 255}));
    assert(!probe_init(1));
    assert(!probe_frame());
    assert(!probe_frame());
    SDL_Event finger = {.type = SDL_EVENT_FINGER_DOWN};
    finger.tfinger.fingerID = 41;
    finger.tfinger.touchID = 11;
    finger.tfinger.x = 70.0f / 360;
    finger.tfinger.y = 36.0f / 300;
    assert(SDL_PushEvent(&finger));
    finger.type = SDL_EVENT_FINGER_UP;
    assert(SDL_PushEvent(&finger));
    assert(cui_sdl3_poll(backend, ui));
    assert(!probe_frame());
    assert(requests == 1 && selected[0] && selected[1] && !children);
    if (argc > 2)
        assert(cui_sdl3_render_capture(backend, &draw, (cui_color){245, 245, 245, 255}, argv[2]));
    else
        assert(cui_sdl3_render(backend, &draw, (cui_color){245, 245, 245, 255}));
    assert(!probe_init(3));
    assert(!probe_frame());
    assert(!probe_frame());
    mouse_click(70, 36);
    assert(cui_sdl3_poll(backend, ui));
    assert(!probe_frame());
    assert(!requests);
    /* System cursors may be unavailable on a dummy driver; Xvfb supports them. */
    bool cursor_supported = cui_sdl3_set_cursor(backend, CUI_CURSOR_CROSSHAIR);
    printf("ButtonGroup SDL queued mouse/touch and native draw PASS; cursor supported=%d\n",
           cursor_supported);
    cui_sdl3_destroy(backend);
    cui_destroy(ui);
    cui_software_destroy(renderer);
    cui_font_close(font);
}
