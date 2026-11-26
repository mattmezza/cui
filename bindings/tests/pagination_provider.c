/* SPDX-License-Identifier: MIT */
/* The configuration functions must remain unreachable on rejected providers. */
#include <cui/cui_bridge.h>
#include <stdlib.h>

uint32_t cui_bridge_pagination_versions(void) {
#ifdef CUI_TEST_PAGINATION_BAD_VERSION
    return 2u;
#else
    return CUI_BRIDGE_PAGINATION_V1;
#endif
}

uint32_t cui_bridge_pagination_config_size_v1(void) {
#if defined(CUI_TEST_PAGINATION_PARTIAL) || defined(CUI_TEST_PAGINATION_BAD_VERSION)
    return sizeof(cui_bridge_pagination_config_v1);
#else
    return sizeof(cui_bridge_pagination_config_v1) + 4u;
#endif
}

cui_bridge_status
cui_bridge_pagination_configure_v1(cui_bridge_app *app, cui_bridge_node node,
                                   const cui_bridge_pagination_config_v1 *config) {
    (void)app;
    (void)node;
    (void)config;
    abort();
}

#ifndef CUI_TEST_PAGINATION_PARTIAL
cui_bridge_status cui_bridge_pagination_get_config_v1(cui_bridge_app *app, cui_bridge_node node,
                                                      cui_bridge_pagination_config_v1 *config) {
    (void)app;
    (void)node;
    (void)config;
    abort();
}
#endif
