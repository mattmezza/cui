/* SPDX-License-Identifier: MIT */
#include "../../../src/bridge/cui_bridge_internal.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static int SDLCALL stop_after_expiry(void *unused) {
    (void)unused;
    SDL_Delay(7500);
    SDL_Event event = {.type = SDL_EVENT_QUIT};
    /* SDL may consume the quit in its callback filter. */
    (void)SDL_PushEvent(&event);
    return 0;
}

int main(void) {
    const char *isolated = getenv("CUI_TEST_VIRTUAL_DISPLAY");
    assert(isolated && !strcmp(isolated, "1"));
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_SDL,
                                  .width = 320,
                                  .height = 240,
                                  .font_path_utf8 = getenv("CUI_TEST_FONT")};
    cui_bridge_window_options window = {sizeof window, 1, 1};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create_with_window_options(&options, &window, &status);
    assert(app && status == CUI_BRIDGE_OK);

    cui_bridge_node root, notice;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_NOTIFICATION, "notice", &notice) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, notice, "Saved") == CUI_BRIDGE_OK);
    assert(cui_bridge_notification_set_auto_hide_v1(app, notice, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_open(app, notice, 1) == CUI_BRIDGE_OK);

    /* This thread only terminates the test. No user input or after-frame
     * callback forces a redraw while the card is timing out. */
    SDL_Thread *stop = SDL_CreateThread(stop_after_expiry, "notification watchdog", NULL);
    assert(stop);
    assert(cui_bridge_run(app, NULL, NULL) == CUI_BRIDGE_CLOSED);
    SDL_WaitThread(stop, NULL);
    assert(app->notifications->count == 0 && app->notification_wake_seconds == 0);
    assert(app->event_count == 1);
    const cui_bridge_event *event = &app->events[app->event_head];
    assert(event->type == CUI_BRIDGE_EVENT_CLOSE && event->node.slot == notice.slot &&
           event->node.generation == notice.generation);
    cui_bridge_destroy(app);
    return 0;
}
