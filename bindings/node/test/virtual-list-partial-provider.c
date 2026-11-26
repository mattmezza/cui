/* SPDX-License-Identifier: MIT */
#include <cui/cui_bridge.h>
#ifdef CUI_TEST_VIRTUAL_ANCHOR_ONLY
cui_bridge_status cui_bridge_virtual_list_preserve_anchor_v1(cui_bridge_app *app,
                                                             cui_bridge_node list,
                                                             uint32_t enabled) {
    (void)app; (void)list; (void)enabled;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
#else
cui_bridge_status cui_bridge_virtual_list_set_rows_v1(cui_bridge_app *app, cui_bridge_node list,
                                                      uint32_t size,
                                                      const cui_bridge_virtual_list_row_v1 *rows,
                                                      uint32_t count) {
    (void)app; (void)list; (void)size; (void)rows; (void)count;
    return CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
#endif
