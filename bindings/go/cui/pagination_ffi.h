/* SPDX-License-Identifier: MIT */
_Static_assert(sizeof(cui_bridge_pagination_config_v1) == 916 &&
                   offsetof(cui_bridge_pagination_config_v1, group) == 20 &&
                   offsetof(cui_bridge_pagination_config_v1, later_pages) == 788,
               "Pagination V1 wire layout");

static struct {
    uint32_t (*versions)(void);
    uint32_t (*config_size_v1)(void);
    cui_bridge_status (*configure_v1)(cui_bridge_app *, cui_bridge_node,
                                      const cui_bridge_pagination_config_v1 *);
    cui_bridge_status (*get_config_v1)(cui_bridge_app *, cui_bridge_node,
                                       cui_bridge_pagination_config_v1 *);
    cui_bridge_status (*set_custom_size)(cui_bridge_app *, cui_bridge_node, float);
    cui_bridge_status (*get_custom_size)(cui_bridge_app *, cui_bridge_node, float *);
} pagination_api;
static int pagination_supported;
static int pagination_custom_supported;

/* Initialized once by the provider loader under loaderMu. */
static void go_pagination_initialize(void) {
    if (bridge_lib) {
#define GO_PAGINATION_LOAD(field)                                                                  \
    go_load_optional_symbol(&pagination_api.field, sizeof pagination_api.field,                    \
                            "cui_bridge_pagination_" #field)
        GO_PAGINATION_LOAD(set_custom_size);
        GO_PAGINATION_LOAD(get_custom_size);
        GO_PAGINATION_LOAD(versions);
        GO_PAGINATION_LOAD(config_size_v1);
        GO_PAGINATION_LOAD(configure_v1);
        GO_PAGINATION_LOAD(get_config_v1);
#undef GO_PAGINATION_LOAD
    }
#if defined(CUI_GO_STATIC)
    else {
        pagination_api.set_custom_size = cui_bridge_pagination_set_custom_size;
        pagination_api.get_custom_size = cui_bridge_pagination_get_custom_size;
        pagination_api.versions = cui_bridge_pagination_versions;
        pagination_api.config_size_v1 = cui_bridge_pagination_config_size_v1;
        pagination_api.configure_v1 = cui_bridge_pagination_configure_v1;
        pagination_api.get_config_v1 = cui_bridge_pagination_get_config_v1;
    }
#endif
    pagination_custom_supported = pagination_api.set_custom_size && pagination_api.get_custom_size;
    pagination_supported =
        pagination_api.versions && pagination_api.config_size_v1 && pagination_api.configure_v1 &&
        pagination_api.get_config_v1 && (pagination_api.versions() & CUI_BRIDGE_PAGINATION_V1) &&
        pagination_api.config_size_v1() == sizeof(cui_bridge_pagination_config_v1);
}

static int go_pagination_supported(void) {
    return pagination_supported;
}

static cui_bridge_status go_pagination_configure(cui_bridge_app *app, cui_bridge_node node,
                                                 const cui_bridge_pagination_config_v1 *config) {
    return pagination_supported ? pagination_api.configure_v1(app, node, config)
                                : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status go_pagination_get_config(cui_bridge_app *app, cui_bridge_node node,
                                                  cui_bridge_pagination_config_v1 *config) {
    return pagination_supported ? pagination_api.get_config_v1(app, node, config)
                                : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static int go_pagination_custom_supported(void) {
    return pagination_custom_supported;
}
static cui_bridge_status go_pagination_set_custom_size(cui_bridge_app *app, cui_bridge_node node,
                                                       float size) {
    return pagination_custom_supported ? pagination_api.set_custom_size(app, node, size)
                                       : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_pagination_get_custom_size(cui_bridge_app *app, cui_bridge_node node,
                                                       float *size) {
    return pagination_custom_supported ? pagination_api.get_custom_size(app, node, size)
                                       : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
