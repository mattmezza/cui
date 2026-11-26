/* SPDX-License-Identifier: MIT */
/* Optional Rating presentation uses the existing Icon resource owner checks. */
_Static_assert(sizeof(cui_bridge_rating_config_v1) == 48, "Rating config ABI");

typedef struct {
    uint32_t (*size)(void);
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node,
                                   const cui_bridge_rating_config_v1 *);
    cui_bridge_status (*get)(cui_bridge_app *, cui_bridge_node, cui_bridge_rating_config_v1 *);
} rating_api;

static bool rating_api_load(rating_api *api) {
    return optional_bridge_symbol(&api->size, sizeof api->size,
                                  "cui_bridge_rating_config_size_v1") &&
           optional_bridge_symbol(&api->configure, sizeof api->configure,
                                  "cui_bridge_rating_configure_v1") &&
           optional_bridge_symbol(&api->get, sizeof api->get, "cui_bridge_rating_get_config_v1") &&
           api->size() == sizeof(cui_bridge_rating_config_v1);
}

static napi_value rating_options(napi_env env, napi_callback_info info) {
    napi_value argv[11];
    js_app *app;
    uint32_t op;
    if (!args(env, info, 2, argv) || !app_arg(env, argv[0], &app) || !uint_arg(env, argv[1], &op))
        return NULL;
    rating_api api;
    bool supported = rating_api_load(&api);
    if (op == 0) {
        napi_value result;
        return napi_get_boolean(env, supported, &result) == napi_ok ? result : NULL;
    }
    if (!supported)
        return fail(env, "Rating options require a complete, compatible bridge symbol group");
    if (op > 3)
        return fail(env, "unknown Rating operation");
    js_node *node;
    if (!args(env, info, op == 1 ? 11 : 3, argv) || !node_arg(env, argv[2], app, &node))
        return NULL;
    cui_bridge_rating_config_v1 config = {.struct_size = sizeof config, .schema_version = 1};
    if (op == 1) {
        double width;
        if (!uint_arg(env, argv[3], &config.flags) ||
            !uint_arg(env, argv[4], &config.design_size) || !number_arg(env, argv[5], &width) ||
            !uint_arg(env, argv[6], &config.fill_rgba) ||
            !uint_arg(env, argv[7], &config.outline_rgba) ||
            !uint_arg(env, argv[8], &config.preview_rgba))
            return NULL;
        if (!(width == 0 || (width >= 1 && width <= 4096)))
            return fail(env, "invalid Rating width");
        config.width = (float)width;
        cui_bridge_icon_handle_v1 *handles[] = {&config.outline_icon, &config.fill_icon};
        for (size_t i = 0; i < sizeof handles / sizeof handles[0]; ++i) {
            napi_valuetype type;
            if (napi_typeof(env, argv[9 + i], &type) != napi_ok)
                return NULL;
            if (type != napi_null && type != napi_undefined) {
                js_icon *icon;
                if (!icon_arg(env, argv[9 + i], app, &icon))
                    return NULL;
                *handles[i] = icon->handle;
            }
        }
    }
    if (op != 2) {
        cui_bridge_status code = api.configure(app->bridge, node->node, op == 3 ? NULL : &config);
        return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
    }
    cui_bridge_status code = api.get(app->bridge, node->node, &config);
    if (code == CUI_BRIDGE_EMPTY)
        return undefined(env);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (config.struct_size != sizeof config || config.schema_version != 1 ||
        (config.flags & ~255u) || config.design_size > CUI_BRIDGE_SIZE_9XL ||
        (!(config.flags & CUI_BRIDGE_RATING_CUSTOM_FILL_V1) && config.fill_rgba) ||
        (!(config.flags & CUI_BRIDGE_RATING_CUSTOM_OUTLINE_V1) && config.outline_rgba) ||
        (!(config.flags & CUI_BRIDGE_RATING_CUSTOM_PREVIEW_V1) && config.preview_rgba))
        return fail(env, "invalid Rating configuration");
    const char *names[] = {"flags",    "designSize",  "width",
                           "fillRgba", "outlineRgba", "previewRgba"};
    double values[] = {config.flags,     config.design_size,  config.width,
                       config.fill_rgba, config.outline_rgba, config.preview_rgba};
    return numeric_object(env, names, values, sizeof values / sizeof values[0]);
}
