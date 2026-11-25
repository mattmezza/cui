/* SPDX-License-Identifier: MIT */
#define _GNU_SOURCE
#include <cui/cui_bridge.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static bool mutation(const char *mode) {
    const char *active = getenv("CUI_ICON_BADGE_PROVIDER");
    return active && strcmp(active, mode) == 0;
}

static void *bridge_symbol(const char *name) {
    const char *path = getenv("CUI_ICON_BADGE_LIBRARY");
    void *bridge = path ? dlopen(path, RTLD_NOW | RTLD_NOLOAD) : NULL;
    if (!bridge)
        return NULL;
    void *symbol = dlsym(bridge, name);
    dlclose(bridge);
    return symbol;
}

#define FORWARD(name, next)                                                                        \
    void *symbol = bridge_symbol(name);                                                            \
    if (!symbol || sizeof(next) != sizeof(symbol))                                                 \
        return CUI_BRIDGE_INVALID_STATE;                                                           \
    memcpy(&next, &symbol, sizeof next)

cui_bridge_status cui_bridge_icon_get_node_config_v1(cui_bridge_app *app, cui_bridge_node node,
                                                     cui_bridge_icon_handle_v1 *artwork,
                                                     cui_bridge_icon_options_v1 *out) {
    cui_bridge_status (*next)(cui_bridge_app *, cui_bridge_node, cui_bridge_icon_handle_v1 *,
                              cui_bridge_icon_options_v1 *);
    FORWARD("cui_bridge_icon_get_node_config_v1", next);
    cui_bridge_status result = next(app, node, artwork, out);
    if (result == CUI_BRIDGE_OK && mutation("invalid-copy"))
        out->reserved_tail[0] = 1;
    return result;
}

cui_bridge_status cui_bridge_badge_get_config_v1(cui_bridge_app *app, cui_bridge_node node,
                                                 cui_bridge_badge_config_v1 *out,
                                                 cui_bridge_icon_options_v1 *artwork) {
    cui_bridge_status (*next)(cui_bridge_app *, cui_bridge_node, cui_bridge_badge_config_v1 *,
                              cui_bridge_icon_options_v1 *);
    FORWARD("cui_bridge_badge_get_config_v1", next);
    cui_bridge_status result = next(app, node, out, artwork);
    if (result == CUI_BRIDGE_OK && mutation("invalid-copy"))
        out->reserved_tail = 1;
    return result;
}

cui_bridge_status cui_bridge_icon_get_node_result_v1(cui_bridge_app *app, cui_bridge_node node,
                                                     cui_bridge_icon_result_v1 *out) {
    cui_bridge_status (*next)(cui_bridge_app *, cui_bridge_node, cui_bridge_icon_result_v1 *);
    FORWARD("cui_bridge_icon_get_node_result_v1", next);
    cui_bridge_status result = next(app, node, out);
    if (result == CUI_BRIDGE_OK && mutation("invalid-result"))
        out->frame_sequence = 0;
    return result;
}

cui_bridge_status cui_bridge_badge_get_result_v1(cui_bridge_app *app, cui_bridge_node node,
                                                 cui_bridge_badge_result_v1 *out) {
    cui_bridge_status (*next)(cui_bridge_app *, cui_bridge_node, cui_bridge_badge_result_v1 *);
    FORWARD("cui_bridge_badge_get_result_v1", next);
    cui_bridge_status result = next(app, node, out);
    if (result == CUI_BRIDGE_OK && mutation("invalid-result"))
        out->flags |= 2;
    return result;
}

#undef FORWARD
