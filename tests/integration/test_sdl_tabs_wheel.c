/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>

typedef struct {
    cui_sdl3 *host;
    cui_context *ui;
    SDL_WindowID window;
    cui_tab_scroll_state scroll;
    cui_id selected;
} tabs_scene;

static void frame(tabs_scene *scene, bool vertical) {
    const cui_choice_item items[] = {
        {10, cui_string("Overview and activity"), false},
        {20, cui_string("Team and collaboration"), false},
        {30, cui_string("Projects and deployments"), false},
    };
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(scene->ui, 320, 240, .016f));
    assert(!cui_tabs(scene->ui, 1, &theme, items, 3, &scene->selected,
                     (cui_tabs_options){.vertical = vertical,
                                        .max_width = 100,
                                        .scroll = &scene->scroll,
                                        .bar_extent = vertical ? 48 : 110},
                     NULL, NULL));
    assert(cui_end_frame(scene->ui).count > 0);
    assert(cui_last_error(scene->ui) == CUI_OK);
    assert(scene->selected == 10);
}

static void wheel(tabs_scene *scene, float dx, float dy, SDL_MouseWheelDirection direction) {
    SDL_Event event = {.type = SDL_EVENT_MOUSE_WHEEL};
    event.wheel.windowID = scene->window;
    event.wheel.x = dx;
    event.wheel.y = dy;
    event.wheel.direction = direction;
    event.wheel.mouse_x = scene->scroll.viewport.x + 2;
    event.wheel.mouse_y = scene->scroll.viewport.y + 2;
    assert(SDL_PushEvent(&event));
}

static void shift(tabs_scene *scene, bool held) {
    SDL_Event event = {.type = held ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP};
    event.key.windowID = scene->window;
    event.key.key = SDLK_LSHIFT;
    event.key.scancode = SDL_SCANCODE_LSHIFT;
    event.key.mod = held ? SDL_KMOD_LSHIFT : SDL_KMOD_NONE;
    event.key.down = held;
    assert(SDL_PushEvent(&event));
}

static void check_wheels(tabs_scene *scene, const cui_event *expected, size_t count) {
    size_t event_count;
    const cui_event *events = cui_frame_events(scene->ui, &event_count);
    size_t index = 0;
    for (size_t i = 0; i < event_count; ++i) {
        if (events[i].type != CUI_EVENT_SCROLL)
            continue;
        assert(index < count);
        assert(events[i].dx == expected[index].dx && events[i].dy == expected[index].dy);
        assert(events[i].modifiers == expected[index].modifiers);
        assert(events[i].x == scene->scroll.viewport.x + 2);
        assert(events[i].y == scene->scroll.viewport.y + 2);
        assert(!events[i].scroll_precise);
        assert(cui_event_consumed(scene->ui, i) == (expected[index].dx != 0));
        ++index;
    }
    assert(index == count);
}

int main(void) {
    assert(SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE));
    tabs_scene scene = {
        .host = cui_sdl3_create_with_options("SDL Tabs wheel", 320, 240, NULL,
                                             (cui_sdl3_window_options){.initially_hidden = true}),
        .ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256}),
        .selected = 10,
    };
    assert(scene.host && scene.ui);
    int window_count;
    SDL_Window **windows = SDL_GetWindows(&window_count);
    assert(windows && window_count == 1);
    scene.window = SDL_GetWindowID(windows[0]);
    SDL_free(windows);
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    frame(&scene, false);
    assert(scene.scroll.maximum >= 64);
    float first = cui_bounds(scene.ui, 10).x;

    /* One poll batch must see the modifier at each event, even when Shift is
     * already up at the moment SDL hands CUI the batch. */
    shift(&scene, true);
    wheel(&scene, 0, -1.25f, SDL_MOUSEWHEEL_NORMAL);
    shift(&scene, false);
    wheel(&scene, 0, -1, SDL_MOUSEWHEEL_NORMAL);
    assert(cui_sdl3_wait_events(scene.host, 0) && cui_sdl3_poll(scene.host, scene.ui));
    frame(&scene, false);
    assert(scene.scroll.changed && scene.scroll.offset == 40);
    assert(cui_bounds(scene.ui, 10).x == first - 40);
    check_wheels(&scene, (cui_event[]){{.dx = -1.25f, .modifiers = CUI_MOD_SHIFT}, {.dy = -1}}, 2);

    /* A trackpad or a platform that already shifts the wheel can supply both
     * axes. Keep them, including the sign SDL delivered for natural scrolling. */
    shift(&scene, true);
    wheel(&scene, .5f, -.75f, SDL_MOUSEWHEEL_FLIPPED);
    assert(cui_sdl3_poll(scene.host, scene.ui));
    frame(&scene, false);
    assert(scene.scroll.offset == 24);
    assert(cui_bounds(scene.ui, 10).x == first - 24);
    check_wheels(&scene, (cui_event[]){{.dx = .5f, .dy = -.75f, .modifiers = CUI_MOD_SHIFT}}, 1);

    /* Focus loss invalidates a retained Shift snapshot, even if we never see
     * its release. Regaining focus and unmodified wheel must not move the strip. */
    SDL_Event focus = {.type = SDL_EVENT_WINDOW_FOCUS_LOST};
    focus.window.windowID = scene.window;
    assert(SDL_PushEvent(&focus));
    focus.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
    assert(SDL_PushEvent(&focus));
    wheel(&scene, 0, -1, SDL_MOUSEWHEEL_NORMAL);
    assert(cui_sdl3_poll(scene.host, scene.ui));
    frame(&scene, false);
    assert(!scene.scroll.changed && scene.scroll.offset == 24);
    check_wheels(&scene, (cui_event[]){{.dy = -1}}, 1);

    scene.scroll = (cui_tab_scroll_state){0};
    frame(&scene, true);
    assert(scene.scroll.maximum >= 32);
    first = cui_bounds(scene.ui, 10).y;
    wheel(&scene, 0, -1, SDL_MOUSEWHEEL_NORMAL);
    assert(cui_sdl3_poll(scene.host, scene.ui));
    frame(&scene, true);
    assert(scene.scroll.changed && scene.scroll.offset == 32);
    assert(cui_bounds(scene.ui, 10).y == first - 32);

    cui_destroy(scene.ui);
    cui_sdl3_destroy(scene.host);
    return 0;
}
