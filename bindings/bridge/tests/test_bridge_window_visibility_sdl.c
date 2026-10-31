/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static void actual(bool hidden) {
    int count = 0;
    SDL_Window **windows = SDL_GetWindows(&count);
    assert(windows && count == 1);
    assert(((SDL_GetWindowFlags(windows[0]) & SDL_WINDOW_HIDDEN) != 0) == hidden);
    SDL_free(windows);
}
static cui_bridge_status after_frame(void *user) {
    cui_bridge_app *app = user;
    uint32_t visible = 99;
    assert(cui_bridge_get_window_visible(app, &visible) == CUI_BRIDGE_OK && visible == 0);
    actual(true);
    assert(cui_bridge_set_window_visible(app, 1) == CUI_BRIDGE_OK);
    actual(false);
    assert(cui_bridge_set_window_visible(app, 0) == CUI_BRIDGE_OK);
    actual(true);
    return CUI_BRIDGE_CLOSED;
}
int main(void) {
    assert(getenv("CUI_TEST_VIRTUAL_DISPLAY") && strcmp(getenv("CUI_TEST_VIRTUAL_DISPLAY"), "1") == 0);
    cui_bridge_options o = {.struct_size=sizeof(o), .abi_version=CUI_BRIDGE_ABI_VERSION,
        .backend=CUI_BRIDGE_BACKEND_SDL, .width=120, .height=80,
        .font_path_utf8=getenv("CUI_TEST_FONT")};
    cui_bridge_window_options w = {sizeof(w), 1, 1};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create_with_window_options(&o, &w, &status);
    assert(app && status == CUI_BRIDGE_OK);
    actual(true);
    cui_bridge_node root = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    actual(true);
    assert(cui_bridge_run(app, after_frame, app) == CUI_BRIDGE_CLOSED);
    uint32_t visible = 99;
    assert(cui_bridge_get_window_visible(app, &visible) == CUI_BRIDGE_CLOSED && visible == 99);
    cui_bridge_destroy(app);
    w.initially_hidden = 0;
    app = cui_bridge_create_with_window_options(&o, &w, &status);
    assert(app && status == CUI_BRIDGE_OK); actual(false); cui_bridge_destroy(app);
    app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK); actual(false); cui_bridge_destroy(app);
    return 0;
}
