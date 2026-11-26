/* SPDX-License-Identifier: MIT */
_Static_assert(sizeof(cui_bridge_attachment_config_v1) == 80, "Attachment config ABI");
static napi_value attachment_options(napi_env env, napi_callback_info info) {
    napi_value v[7];
    js_app *app;
    uint32_t op;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &op))
        return NULL;
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node,
                                   const cui_bridge_attachment_config_v1 *) = NULL;
    cui_bridge_status (*preview)(cui_bridge_app *, cui_bridge_node, const uint8_t *, uint32_t,
                                 uint32_t, uint32_t, uint32_t) = NULL;
    bool supported = op == 0 || op == 2 || op == 3
                         ? optional_bridge_symbol(&configure, sizeof configure,
                                                  "cui_bridge_attachment_configure_v1")
                         : optional_bridge_symbol(&preview, sizeof preview,
                                                  "cui_bridge_attachment_set_preview_rgba_v1");
    if (op <= 1) {
        napi_value result;
        return napi_get_boolean(env, supported, &result) == napi_ok ? result : NULL;
    }
    if (!supported)
        return fail(env, "Attachment requires optional bridge symbol");
    js_node *node;
    if (!args(env, info, op == 2 ? 4 : op == 4 ? 7 : 3, v) || !node_arg(env, v[2], app, &node))
        return NULL;
    cui_bridge_status code;
    if (op == 2) {
        bool is_buffer = false;
        void *data;
        size_t length;
        cui_bridge_attachment_config_v1 config;
        if (napi_is_buffer(env, v[3], &is_buffer) != napi_ok || !is_buffer ||
            napi_get_buffer_info(env, v[3], &data, &length) != napi_ok || length != sizeof config)
            return fail(env, "Attachment config must be an 80-byte Buffer");
        memcpy(&config, data, sizeof config);
        code = configure(app->bridge, node->node, &config);
    } else if (op == 3) {
        code = configure(app->bridge, node->node, NULL);
    } else if (op == 4) {
        bool typed = false, ordinary_buffer = false;
        napi_typedarray_type type;
        size_t length, offset;
        void *data;
        napi_value buffer;
        uint32_t w, h, stride;
        if (!uint_arg(env, v[4], &w) || !uint_arg(env, v[5], &h) || !uint_arg(env, v[6], &stride))
            return NULL;
        if (napi_is_typedarray(env, v[3], &typed) != napi_ok || !typed ||
            napi_get_typedarray_info(env, v[3], &type, &length, &data, &buffer, &offset) !=
                napi_ok ||
            (type != napi_uint8_array && type != napi_uint8_clamped_array) || !data ||
            napi_is_arraybuffer(env, buffer, &ordinary_buffer) != napi_ok || !ordinary_buffer ||
            length > 16777216)
            return fail(env, "Attachment pixels require a bounded non-shared Uint8Array");
        code = preview(app->bridge, node->node, data, w, h, stride, (uint32_t)length);
    } else if (op == 5) {
        code = preview(app->bridge, node->node, NULL, 0, 0, 0, 0);
    } else
        return fail(env, "unknown Attachment operation");
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}
