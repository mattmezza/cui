/* SPDX-License-Identifier: MIT */
#include "../../../src/bridge/cui_bridge_internal.h"
#include <assert.h>
int main(void) {
    cui_bridge_options o = {.struct_size = sizeof(o), .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .width = 120, .height = 80};
    cui_bridge_window_options w = {sizeof(w), 1, 1};
    cui_bridge_status status;
    assert(cui_bridge_window_options_size() == sizeof(w));
    assert(!cui_bridge_create_with_window_options(&o, &w, &status));
    assert(status == CUI_BRIDGE_BACKEND_UNAVAILABLE);
    w.initially_hidden = 2;
    assert(!cui_bridge_create_with_window_options(&o, &w, &status));
    assert(status == CUI_BRIDGE_INVALID_ARGUMENT);
    w.initially_hidden = 0; w.version = 2;
    assert(!cui_bridge_create_with_window_options(&o, &w, &status));
    assert(status == CUI_BRIDGE_INVALID_ARGUMENT);
    w.version = 1; w.struct_size--;
    assert(!cui_bridge_create_with_window_options(&o, &w, &status));
    assert(status == CUI_BRIDGE_INVALID_ARGUMENT);
    w.struct_size = sizeof(w);
    cui_bridge_app *app = cui_bridge_create_with_window_options(&o, &w, &status);
    assert(app && status == CUI_BRIDGE_OK);
    uint32_t visible = 99;
    assert(cui_bridge_get_window_visible(NULL, &visible) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_get_window_visible(app, NULL) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_window_visible(app, 2) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_get_window_visible(app, &visible) == CUI_BRIDGE_BACKEND_UNAVAILABLE);
    assert(visible == 99);
    assert(cui_bridge_set_window_visible(app, 1) == CUI_BRIDGE_BACKEND_UNAVAILABLE);
    app->stepping = true;
    assert(cui_bridge_set_window_visible(app, 1) == CUI_BRIDGE_INVALID_STATE);
    assert(cui_bridge_get_window_visible(app, &visible) == CUI_BRIDGE_INVALID_STATE);
    assert(visible == 99);
    app->stepping = false; app->closed = true;
    assert(cui_bridge_get_window_visible(app, &visible) == CUI_BRIDGE_CLOSED);
    assert(cui_bridge_set_window_visible(app, 1) == CUI_BRIDGE_CLOSED);
    assert(visible == 99);
    cui_bridge_destroy(app);
    app = cui_bridge_create_with_window_options(&o, NULL, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
    return 0;
}
