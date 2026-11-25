/* SPDX-License-Identifier: MIT */
#include "cui/cui_sdl3.h"
#include "cui/cui_tabs_events.h"
#include <SDL3/SDL.h>
#include <assert.h>

enum { WIDTH = 480, HEIGHT = 320, POPUP = 1000, TRIGGER = 1001 };
enum { FIRST = 10, DISABLED = 20, LAST = 30 };

typedef struct {
    cui_sdl3 *host;
    cui_context *ui;
    SDL_WindowID window;
    cui_tab_scroll_state scroll;
    cui_tab_overflow overflow;
    cui_menu_entry entries[3];
    cui_id selected;
    bool vertical;
    float extent;
    size_t panel;
    unsigned changes;
} scene;

static void selected_panel(cui_context *ui, size_t index, void *user) {
    (void)ui;
    ((scene *)user)->panel = index;
}

static void selected_changed(void *user, const cui_tabs_selection_change *change) {
    scene *test = user;
    assert(change->value == test->selected);
    assert(change->previous != change->value);
    ++test->changes;
}

static void frame(scene *test, cui_id selected) {
    const cui_choice_item items[] = {
        {FIRST, cui_string("Overview and activity"), false},
        {DISABLED, cui_string("Unavailable tab"), true},
        {LAST, cui_string("Projects and deployments"), false},
    };
    cui_theme theme = cui_theme_light();
    bool expected_change = test->selected != selected;
    unsigned before = test->changes;
    assert(cui_sdl3_poll(test->host, test->ui));
    assert(cui_begin_frame(test->ui, WIDTH, HEIGHT, .016f));
    test->panel = SIZE_MAX;
    cui_tabs_event_result result =
        cui_tabs_events(test->ui, 1, &theme, items, 3, &test->selected,
                        (cui_tabs_options){.vertical = test->vertical,
                                           .max_width = 96,
                                           .scroll = &test->scroll,
                                           .bar_extent = test->extent,
                                           .overflow = &test->overflow,
                                           .reveal_selected = true},
                        selected_panel, test, (cui_tabs_handler){selected_changed, test});
    assert(cui_end_frame(test->ui).count > 0);
    assert(cui_last_error(test->ui) == CUI_OK && test->selected == selected);
    assert(result.changed == expected_change && result.change_count == expected_change);
    assert(test->changes == before + expected_change);
    assert(test->panel == (selected == FIRST ? 0u : 2u));
}

static void key(scene *test, SDL_Keycode code) {
    SDL_Event event = {.type = SDL_EVENT_KEY_DOWN};
    event.key.windowID = test->window;
    event.key.key = code;
    event.key.down = true;
    assert(SDL_PushEvent(&event));
    event.type = SDL_EVENT_KEY_UP;
    event.key.down = false;
    assert(SDL_PushEvent(&event));
}

static void tap(scene *test, cui_id target, bool cancel) {
    cui_rect rect = cui_bounds(test->ui, target);
    assert(rect.w > 0 && rect.h > 0);
    SDL_Event event = {.type = SDL_EVENT_FINGER_DOWN};
    event.tfinger.windowID = test->window;
    event.tfinger.touchID = 11;
    event.tfinger.fingerID = 41;
    event.tfinger.x = (rect.x + rect.w / 2) / WIDTH;
    event.tfinger.y = (rect.y + rect.h / 2) / HEIGHT;
    assert(SDL_PushEvent(&event));
    if (cancel) {
        event.type = SDL_EVENT_FINGER_CANCELED;
        assert(SDL_PushEvent(&event));
    }
    event.type = SDL_EVENT_FINGER_UP;
    assert(SDL_PushEvent(&event));
}

static void check_reveal(scene *test, cui_id selected) {
    cui_rect tab = cui_bounds(test->ui, selected), viewport = test->scroll.viewport;
    assert(tab.w > 0 && tab.h > 0 && viewport.w > 0 && viewport.h > 0);
    if (test->vertical)
        assert(tab.y >= viewport.y && tab.y + tab.h <= viewport.y + viewport.h + .01f);
    else
        assert(tab.x >= viewport.x && tab.x + tab.w <= viewport.x + viewport.w + .01f);
    assert(!test->overflow.menu.overlay.open && cui_focused(test->ui) == TRIGGER);
}

static void exercise(cui_sdl3 *host, SDL_WindowID window, bool vertical) {
    scene test = {
        .host = host,
        .window = window,
        .ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256}),
        .selected = FIRST,
        .vertical = vertical,
        .extent = vertical ? 52 : 110,
    };
    assert(test.ui);
    cui_tab_overflow_init(&test.overflow, POPUP, TRIGGER, test.entries, 3);
    test.overflow.options.popup.viewport = (cui_rect){0, 0, WIDTH, HEIGHT};
    test.overflow.options.popup.width = 240;
    frame(&test, FIRST);
    assert(test.overflow.visible && test.scroll.maximum > 0);
    cui_rect hit = cui_bounds(test.ui, TRIGGER);
    assert(hit.w == hit.h && hit.w >= 32);

    key(&test, SDLK_TAB);
    frame(&test, FIRST);
    if (cui_focused(test.ui) != TRIGGER) {
        assert(cui_focused(test.ui) == FIRST);
        key(&test, SDLK_TAB);
        frame(&test, FIRST);
    }
    assert(cui_focused(test.ui) == TRIGGER);
    key(&test, SDLK_RETURN);
    frame(&test, FIRST);
    assert(test.overflow.menu.overlay.open && cui_focused(test.ui) == test.entries[0].id);
    key(&test, SDLK_DOWN);
    frame(&test, FIRST);
    assert(cui_focused(test.ui) == test.entries[2].id);
    key(&test, SDLK_RETURN);
    frame(&test, LAST);
    assert(test.changes == 1 && test.scroll.offset > 0);
    check_reveal(&test, LAST);

    key(&test, SDLK_RETURN);
    frame(&test, LAST);
    assert(test.overflow.menu.overlay.open);
    key(&test, SDLK_ESCAPE);
    frame(&test, LAST);
    check_reveal(&test, LAST);

    tap(&test, TRIGGER, false);
    frame(&test, LAST);
    assert(test.overflow.menu.overlay.open);
    tap(&test, test.entries[1].id, false);
    frame(&test, LAST);
    assert(test.overflow.menu.overlay.open);
    tap(&test, test.entries[0].id, true);
    frame(&test, LAST);
    assert(test.overflow.menu.overlay.open);
    tap(&test, test.entries[0].id, false);
    frame(&test, FIRST);
    assert(test.changes == 2 && test.scroll.offset == 0);
    check_reveal(&test, FIRST);

    tap(&test, TRIGGER, false);
    frame(&test, FIRST);
    assert(test.overflow.menu.overlay.open);
    test.extent = vertical ? 220 : 390;
    frame(&test, FIRST);
    assert(!test.overflow.visible && !test.overflow.menu.overlay.open);
    size_t count;
    const cui_semantic *nodes = cui_semantics(test.ui, &count);
    for (size_t i = 0; i < count; ++i)
        assert(nodes[i].id != TRIGGER && nodes[i].id != POPUP);
    cui_destroy(test.ui);
}

int main(void) {
    assert(SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE));
    cui_sdl3 *host =
        cui_sdl3_create_with_options("SDL tab overflow", WIDTH, HEIGHT, NULL,
                                     (cui_sdl3_window_options){.initially_hidden = true});
    assert(host);
    int count;
    SDL_Window **windows = SDL_GetWindows(&count);
    assert(windows && count == 1);
    SDL_WindowID window = SDL_GetWindowID(windows[0]);
    SDL_free(windows);
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
    exercise(host, window, false);
    exercise(host, window, true);
    cui_sdl3_destroy(host);
    return 0;
}
