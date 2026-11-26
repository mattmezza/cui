/* SPDX-License-Identifier: MIT */
_Static_assert(sizeof(cui_bridge_rating_config_v1) == 48 &&
                   offsetof(cui_bridge_rating_config_v1, outline_icon) == 32,
               "Rating V1 wire layout");

static struct {
    uint32_t (*config_size_v1)(void);
    cui_bridge_status (*configure_v1)(cui_bridge_app *, cui_bridge_node,
                                      const cui_bridge_rating_config_v1 *);
    cui_bridge_status (*get_config_v1)(cui_bridge_app *, cui_bridge_node,
                                       cui_bridge_rating_config_v1 *);
} rating_api;
static int rating_supported;

/* Initialized under the bridge loader lock. Older providers keep Rating. */
static void go_rating_initialize(void) {
    if (bridge_lib) {
#define GO_RATING_LOAD(field)                                                                      \
    go_load_optional_symbol(&rating_api.field, sizeof rating_api.field, "cui_bridge_rating_" #field)
        GO_RATING_LOAD(config_size_v1);
        GO_RATING_LOAD(configure_v1);
        GO_RATING_LOAD(get_config_v1);
#undef GO_RATING_LOAD
    }
#if defined(CUI_GO_STATIC)
    else {
        rating_api.config_size_v1 = cui_bridge_rating_config_size_v1;
        rating_api.configure_v1 = cui_bridge_rating_configure_v1;
        rating_api.get_config_v1 = cui_bridge_rating_get_config_v1;
    }
#endif
    rating_supported = rating_api.config_size_v1 && rating_api.configure_v1 &&
                       rating_api.get_config_v1 &&
                       rating_api.config_size_v1() == sizeof(cui_bridge_rating_config_v1);
}

static int go_rating_supported(void) {
    return rating_supported;
}

static cui_bridge_status go_rating_configure(cui_bridge_app *app, cui_bridge_node node,
                                             const cui_bridge_rating_config_v1 *config) {
    return rating_supported ? rating_api.configure_v1(app, node, config)
                            : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status go_rating_get_config(cui_bridge_app *app, cui_bridge_node node,
                                              cui_bridge_rating_config_v1 *config) {
    return rating_supported ? rating_api.get_config_v1(app, node, config)
                            : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
