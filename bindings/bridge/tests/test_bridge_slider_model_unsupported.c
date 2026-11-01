/* SPDX-License-Identifier: MIT */
/* Explicit unsupported-target build: old ABI still usable, new API never enables. */
#define CUI_BRIDGE_BUILD 1
#define CUI_BRIDGE_SLIDER_NO_TLS 1
#include "../../../src/bridge/cui_bridge.c"
#include <assert.h>
int main(void) {
    _Static_assert(BRIDGE_SLIDER_SUPPORTED == 0, "unsupported policy fixture");
    assert(cui_bridge_slider_model_versions() == 0);
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 320,
                                  .height = 200};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK && !app->slider);
    cui_bridge_node root;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           CUI_BRIDGE_OK);
    cui_bridge_slider_queue_options_v1 q = {sizeof q, 1, 16, 0};
    assert(cui_bridge_slider_queue_enable_v1(app, sizeof q, &q) == CUI_BRIDGE_BACKEND_UNAVAILABLE);
    assert(!app->slider);
    cui_bridge_slider_request_v1 request;
    memset(&request, 0x5a, sizeof request);
    unsigned char before[sizeof request];
    memcpy(before, &request, sizeof request);
    assert(cui_bridge_poll_slider_request_v1(app, sizeof request, &request) ==
           CUI_BRIDGE_BACKEND_UNAVAILABLE);
    assert(!memcmp(before, &request, sizeof request));
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(app);
    return 0;
}
