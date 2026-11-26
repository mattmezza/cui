/* SPDX-License-Identifier: MIT */
typedef struct {
    uint32_t (*versions)(void), (*config_size)(void);
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node,
                                   const cui_bridge_pagination_config_v1 *);
    cui_bridge_status (*config)(cui_bridge_app *, cui_bridge_node, cui_bridge_pagination_config_v1 *);
} pagination_api;
_Static_assert(sizeof(cui_bridge_pagination_config_v1) == 916, "Pagination V1 ABI");
static bool pagination_resolve(pagination_api *api) {
#define PAGINATION_SYMBOL(field, suffix) \
    optional_bridge_symbol(&api->field, sizeof api->field, "cui_bridge_pagination_" suffix)
    return PAGINATION_SYMBOL(versions, "versions") && PAGINATION_SYMBOL(config_size, "config_size_v1") &&
           PAGINATION_SYMBOL(configure, "configure_v1") && PAGINATION_SYMBOL(config, "get_config_v1") &&
           (api->versions() & CUI_BRIDGE_PAGINATION_V1) && api->config_size() == 916;
#undef PAGINATION_SYMBOL
}
/* op0 probe, op1 configure, op2 copied configuration, op3 clear override. */
static napi_value pagination_options(napi_env env, napi_callback_info info) {
    napi_value v[13];
    js_app *app;
    uint32_t op;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &op))
        return NULL;
    if (op > 3) return fail(env, "unknown Pagination operation");
    pagination_api api;
    bool supported = pagination_resolve(&api);
    if (op == 0) {
        napi_value result;
        return napi_get_boolean(env, supported, &result) == napi_ok ? result : NULL;
    }
    if (!supported) return fail(env, "Pagination V1 requires a complete, compatible bridge symbol group");
    js_node *node;
    if (!args(env, info, op == 1 ? 13 : 3, v) || !node_arg(env, v[2], app, &node)) return NULL;
    cui_bridge_pagination_config_v1 config = {.struct_size = sizeof config,
                                               .schema_version = CUI_BRIDGE_PAGINATION_V1};
    char *fields[] = {config.group, config.first, config.previous, config.next, config.last,
                      config.earlier_pages, config.later_pages};
    if (op == 1) {
        double width;
        if (!uint_arg(env, v[3], &config.flags) || !uint_arg(env, v[4], &config.visible_pages) ||
            !number_arg(env, v[5], &width)) return NULL;
        if (width < 0 || width > 32768)
            return fail(env, "Pagination width must be in [0,32768]");
        config.available_width = (float)width;
        if (width != 0 && config.available_width == 0)
            return fail(env, "Pagination width underflows a bridge float");
        for (size_t i = 0; i < 7; i++) {
            char *text = NULL;
            if (!string_arg(env, v[6 + i], &text)) return NULL;
            size_t length = strlen(text);
            if (length >= CUI_BRIDGE_PAGINATION_LABEL_BYTES) {
                free(text);
                return fail(env, "Pagination label exceeds 127 UTF-8 bytes");
            }
            memcpy(fields[i], text, length + 1);
            free(text);
        }
    }
    cui_bridge_status code = op == 2 ? api.config(app->bridge, node->node, &config) :
        api.configure(app->bridge, node->node, op == 3 ? NULL : &config);
    if (op == 2 && code == CUI_BRIDGE_EMPTY) return undefined(env);
    if (code != CUI_BRIDGE_OK) return status(env, code);
    if (op != 2) return undefined(env);
    if (config.struct_size != sizeof config || config.schema_version != CUI_BRIDGE_PAGINATION_V1)
        return fail(env, "invalid copied Pagination configuration header");
    const char *const names[] = {"flags", "visiblePages", "availableWidth"};
    const double values[] = {config.flags, config.visible_pages, config.available_width};
    napi_value result = numeric_object(env, names, values, 3);
    const char *const label_names[] = {"group", "first", "previous", "next", "last", "earlierPages", "laterPages"};
    if (!result) return NULL;
    for (size_t i = 0; i < 7; i++) {
        const char *end = memchr(fields[i], '\0', CUI_BRIDGE_PAGINATION_LABEL_BYTES);
        if (!end || !strict_utf8(fields[i], (uint32_t)(end - fields[i])))
            return fail(env, "invalid copied Pagination label");
        napi_value text;
        if (napi_create_string_utf8(env, fields[i], (size_t)(end - fields[i]), &text) != napi_ok ||
            napi_set_named_property(env, result, label_names[i], text) != napi_ok) return NULL;
    }
    return result;
}
