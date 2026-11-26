/* SPDX-License-Identifier: MIT */
_Static_assert(sizeof(cui_bridge_disclosure_config_v1) == 172, "Disclosure config ABI");
_Static_assert(sizeof(cui_bridge_disclosure_trigger_style_v1) == 40, "Disclosure style ABI");
typedef cui_bridge_status (*disclosure_config_fn)(cui_bridge_app *, cui_bridge_node,
                                                  const cui_bridge_disclosure_config_v1 *);
typedef cui_bridge_status (*disclosure_style_fn)(cui_bridge_app *, cui_bridge_node,
                                                 const cui_bridge_disclosure_trigger_style_v1 *);

static napi_value disclosure_options(napi_env env, napi_callback_info info) {
    napi_value argv[4];
    js_app *app;
    uint32_t operation;
    if (!args(env, info, 2, argv) || !app_arg(env, argv[0], &app) ||
        !uint_arg(env, argv[1], &operation))
        return NULL;

    disclosure_config_fn configure = NULL;
    disclosure_style_fn set_style = NULL;
    bool supported = optional_bridge_symbol(&configure, sizeof configure,
                                            "cui_bridge_disclosure_configure_v1") &&
                     optional_bridge_symbol(&set_style, sizeof set_style,
                                            "cui_bridge_disclosure_set_trigger_style_v1");
    if (operation == 0) {
        napi_value result;
        return napi_get_boolean(env, supported, &result) == napi_ok ? result : NULL;
    }
    if (!supported)
        return fail(env, "disclosure presentation requires both optional bridge symbols");
    if (operation > 2)
        return fail(env, "invalid disclosure operation");

    js_node *node;
    napi_valuetype type;
    if (!args(env, info, 4, argv) || !node_arg(env, argv[2], app, &node) ||
        napi_typeof(env, argv[3], &type) != napi_ok)
        return NULL;
    union {
        cui_bridge_disclosure_config_v1 config;
        cui_bridge_disclosure_trigger_style_v1 style;
    } record;
    bool clear = type == napi_null;
    if (!clear) {
        bool buffer = false;
        void *data;
        size_t size;
        size_t expected = operation == 1 ? sizeof record.config : sizeof record.style;
        if (napi_is_buffer(env, argv[3], &buffer) != napi_ok || !buffer ||
            napi_get_buffer_info(env, argv[3], &data, &size) != napi_ok || size != expected)
            return fail(env, "invalid disclosure record");
        memcpy(&record, data, size);
    }
    cui_bridge_status code = operation == 1
                                 ? configure(app->bridge, node->node, clear ? NULL : &record.config)
                                 : set_style(app->bridge, node->node, clear ? NULL : &record.style);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
