/* SPDX-License-Identifier: MIT */
#include "cui/cui_chrome.h"
#include "cui/cui_sdl3.h"
#include <SDL3/SDL.h>
#include <assert.h>

static const char *driver_override;

const char *SDLCALL __real_SDL_GetCurrentVideoDriver(void);
const char *SDLCALL __wrap_SDL_GetCurrentVideoDriver(void) {
    return driver_override ? driver_override : __real_SDL_GetCurrentVideoDriver();
}

static void check_title_bar(cui_context *ui, cui_window_host *window,
                            cui_window_decoration decoration, float content_x, unsigned controls) {
    assert(window->query(window->user).decoration == decoration);
    cui_theme theme = cui_theme_light();
    cui_title_bar_state state = {0};
    cui_chrome_item title = {.id = 101, .text = cui_string("Document")};
    assert(cui_begin_frame(ui, 480, 180, .016f));
    assert(!cui_title_bar(ui, 100, &theme, &state, &title, 1,
                          (cui_title_bar_options){.width = 480, .host = window}));
    assert(cui_end_frame(ui).count > 0 && cui_last_error(ui) == CUI_OK);
    assert(cui_bounds(ui, 101).x == content_x);
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    unsigned buttons = 0;
    for (size_t i = 0; i < count; i++)
        buttons += nodes[i].role == CUI_ROLE_BUTTON;
    assert(buttons == controls);
}

int main(void) {
    assert(SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE));
    cui_sdl3 *host = cui_sdl3_create_with_options(
        "SDL title bar", 480, 180, NULL, (cui_sdl3_window_options){.initially_hidden = true});
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    assert(host && ui);
    cui_window_host window = cui_sdl3_window_interface(host);
    check_title_bar(ui, &window, CUI_WINDOW_SERVER_DECORATED, 12, 0);
    assert(window.query(window.user).actions & (1u << CUI_WINDOW_TITLEBAR_DOUBLE_CLICK));

    /* Keep the actual window and flags. Simulate which SDL driver reports
     * them to exercise the Cocoa policy in the private Linux display. */
    driver_override = "cocoa";
    check_title_bar(ui, &window, CUI_WINDOW_NATIVE_TRAFFIC_LIGHTS, 80, 0);
    assert(!(window.query(window.user).actions & (1u << CUI_WINDOW_TITLEBAR_DOUBLE_CLICK)));
    assert(cui_sdl3_set_decorated(host, false));
    check_title_bar(ui, &window, CUI_WINDOW_CLIENT_DECORATED, 12, 3);
    assert(cui_sdl3_set_decorated(host, true));
    check_title_bar(ui, &window, CUI_WINDOW_NATIVE_TRAFFIC_LIGHTS, 80, 0);

    driver_override = "dummy";
    check_title_bar(ui, &window, CUI_WINDOW_SERVER_DECORATED, 12, 0);
    assert(window.query(window.user).actions == (1u << CUI_WINDOW_CLOSE));

    driver_override = NULL;
    cui_destroy(ui);
    cui_sdl3_destroy(host);
    return 0;
}
