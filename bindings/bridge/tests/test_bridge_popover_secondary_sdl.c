/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static void press_right(SDL_WindowID window, cui_bridge_rect trigger) {
    SDL_Event event = {0};
    event.button.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
    event.button.windowID = window;
    event.button.button = SDL_BUTTON_RIGHT;
    event.button.down = true;
    event.button.clicks = 1;
    event.button.x = trigger.x + trigger.width / 2;
    event.button.y = trigger.y + trigger.height / 2;
    assert(SDL_PushEvent(&event));
    event.button.type = SDL_EVENT_MOUSE_BUTTON_UP;
    event.button.down = false;
    assert(SDL_PushEvent(&event));
}

int main(void) {
    const char *isolated = getenv("CUI_TEST_VIRTUAL_DISPLAY");
    assert(isolated && strcmp(isolated, "1") == 0);
    const cui_bridge_options options = {.struct_size = sizeof options,
                                        .abi_version = CUI_BRIDGE_ABI_VERSION,
                                        .backend = CUI_BRIDGE_BACKEND_SDL,
                                        .width = 480,
                                        .height = 320,
                                        .font_path_utf8 = getenv("CUI_TEST_FONT")};
    const cui_bridge_window_options window_options = {sizeof window_options, 1, 1};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create_with_window_options(&options, &window_options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    int window_count = 0;
    SDL_Window **windows = SDL_GetWindows(&window_count);
    assert(windows && window_count == 1);
    assert(SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_HIDDEN);
    SDL_WindowID window = SDL_GetWindowID(windows[0]);
    SDL_free(windows);

    cui_bridge_node root, trigger, menu;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_POPOVER, "account-menu", &menu) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_BUTTON, "account", &trigger) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, trigger, "Account") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_anchor(app, menu, trigger) == CUI_BRIDGE_OK);
    const cui_bridge_popover_config_v1 config = {
        .flags = CUI_BRIDGE_POPOVER_SECONDARY_TRIGGER_V1, .width = 180, .height = 120};
    assert(cui_bridge_popover_configure_v1(app, menu, sizeof config, &config) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect anchor, panel;
    assert(cui_bridge_node_bounds(app, trigger, &anchor) == CUI_BRIDGE_OK);
    press_right(window, anchor);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_bounds(app, menu, &panel) == CUI_BRIDGE_OK &&
           panel.width == config.width);
    cui_bridge_event_ex event;
    uint32_t required;
    char payload[8];
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_EMPTY);

    press_right(window, anchor);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    uint32_t open;
    assert(cui_bridge_get_open(app, menu, &open) == CUI_BRIDGE_OK && !open);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_OK);
    assert(event.type == CUI_BRIDGE_EVENT_CLOSE && event.node.slot == menu.slot);
    assert(cui_bridge_poll_event_ex(app, &event, payload, sizeof payload, &required) ==
           CUI_BRIDGE_EMPTY);
    cui_bridge_destroy(app);
    return 0;
}
