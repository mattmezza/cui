/* SPDX-License-Identifier: MIT */
/* Uses only the old public ABI; run unchanged against independent old/new libraries. */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <stdio.h>
static void record(cui_bridge_app *app, cui_bridge_node slider, const char *phase) {
    double value;
    assert(cui_bridge_get_value(app, slider, &value) == CUI_BRIDGE_OK);
    cui_bridge_rect r;
    assert(cui_bridge_node_bounds(app, slider, &r) == CUI_BRIDGE_OK);
    printf("%s value=%a bounds=%a,%a,%a,%a\n", phase, value, (double)r.x, (double)r.y,
           (double)r.width, (double)r.height);
    cui_bridge_event_ex e;
    char text[32];
    uint32_t required;
    cui_bridge_status status;
    while ((status = cui_bridge_poll_event_ex(app, &e, text, sizeof text, &required)) ==
           CUI_BRIDGE_OK)
        printf("event=%u node=%u:%u frame=%llu checked=%u value=%a text=%u:%s\n", e.type,
               e.node.slot, e.node.generation, (unsigned long long)e.frame_sequence, e.checked,
               e.value, required, text);
    assert(status == CUI_BRIDGE_EMPTY);
}
int main(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = CUI_BRIDGE_ABI_VERSION,
                            .width = 640,
                            .height = 480};
    cui_bridge_status status;
    cui_bridge_app *app = cui_bridge_create(&o, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root, slider;
    assert(cui_bridge_node_create(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_SLIDER, "volume", &slider) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(app, slider, "Volume") == CUI_BRIDGE_OK);
    assert(cui_bridge_set_range(app, slider, 0, 100, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_value(app, slider, 40) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    record(app, slider, "initial");
    cui_bridge_rect r;
    assert(cui_bridge_node_bounds(app, slider, &r) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, r.x + r.width * .7f,
                                    r.y + r.height / 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, r.x + r.width * .7f,
                                    r.y + r.height / 2) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    record(app, slider, "pointer-autoaccept");
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_RIGHT, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_RIGHT, 0, 1) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    record(app, slider, "two-keys");
    assert(cui_bridge_set_disabled(app, slider, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_key(app, CUI_BRIDGE_KEY_DOWN, CUI_BRIDGE_KEY_LEFT, 0, 0) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    record(app, slider, "disabled");
    cui_bridge_destroy(app);
    return 0;
}
