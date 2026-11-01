/* SPDX-License-Identifier: MIT */
#define CUI_BRIDGE_SLIDER_NO_TLS 1
#define CUI_BRIDGE_BUILD 1
#include "../../src/bridge/cui_bridge.c"
#include <assert.h>
int main(void) {
    assert(cui_bridge_time_field_versions() == 0);
    assert(cui_bridge_slider_model_versions() == 0);
    cui_bridge_options o = {.struct_size = sizeof o, .abi_version = 1, .width = 640, .height = 480};
    cui_bridge_status status;
    cui_bridge_app *a = cui_bridge_create(&o, &status);
    assert(a && status == CUI_BRIDGE_OK);
    assert(cui_bridge_time_field_enable_v1(a, 1) == CUI_BRIDGE_BACKEND_UNAVAILABLE);
    assert(!a->time_fields && !a->slider);
    cui_bridge_time_field_change_v1 out = {.struct_size = sizeof out, .version = 1};
    unsigned char before[sizeof out];
    memcpy(before, &out, sizeof out);
    assert(cui_bridge_time_field_poll_change_v1(a, &out) == CUI_BRIDGE_BACKEND_UNAVAILABLE);
    assert(memcmp(before, &out, sizeof out) == 0);
    cui_bridge_node root;
    assert(cui_bridge_node_create(a, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
    cui_bridge_destroy(a);
    return 0;
}
