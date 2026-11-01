/* SPDX-License-Identifier: MIT */
/* Exact frozen pre-resource header; no new Icon symbols or records used. */
#include "../reference/bridge_icon_public_dbfb7945.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    cui_bridge_options o = {.struct_size = sizeof o,
                            .abi_version = 1,
                            .width = 320,
                            .height = 240,
                            .max_nodes = 32,
                            .max_events = 32};
    cui_bridge_status status;
    cui_bridge_app *a = cui_bridge_create(&o, &status);
    assert(a && status == CUI_BRIDGE_OK);
    cui_bridge_node root, badge, icon;
    assert(cui_bridge_node_create(a, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "root", &root) ==
           CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(a, root, CUI_BRIDGE_BADGE, "badge", &badge) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(a, root, CUI_BRIDGE_ICON, "icon", &icon) == CUI_BRIDGE_OK);
    assert(cui_bridge_set_text(a, badge, "Ready") == CUI_BRIDGE_OK &&
           cui_bridge_set_text(a, icon, "check") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(a, .016f) == CUI_BRIDGE_OK &&
           cui_bridge_step(a, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect b, i;
    assert(cui_bridge_node_bounds(a, badge, &b) == CUI_BRIDGE_OK &&
           cui_bridge_node_bounds(a, icon, &i) == CUI_BRIDGE_OK);
    assert(b.width > 0 && b.height > 0 && i.width > 0 && i.height > 0);
    printf("OLD ABI %u %u:%u %u:%u %.9g %.9g %.9g %.9g\n", cui_bridge_abi_version(), badge.slot,
           badge.generation, icon.slot, icon.generation, b.width, b.height, i.width, i.height);
    assert(cui_bridge_request_close(a) == CUI_BRIDGE_OK);
    cui_bridge_destroy(a);
    return 0;
}
