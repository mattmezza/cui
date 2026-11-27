/* SPDX-License-Identifier: MIT */
static napi_value popover_options(napi_env env, napi_callback_info info) {
    napi_value v[10];
    js_app *app;
    uint32_t op;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &op))
        return NULL;
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                   const cui_bridge_popover_config_v1 *) = NULL;
    bool supported =
        optional_bridge_symbol(&configure, sizeof configure, "cui_bridge_popover_configure_v1");
    if (op == 0) {
        napi_value result;
        return napi_get_boolean(env, supported, &result) == napi_ok ? result : NULL;
    }
    if (op > 2)
        return fail(env, "unknown Popover operation");
    if (!supported)
        return fail(env, "Popover options require a compatible bridge");
    js_node *node;
    if (!args(env, info, op == 1 ? 10 : 3, v) || !node_arg(env, v[2], app, &node))
        return NULL;
    cui_bridge_popover_config_v1 config = {0};
    if (op == 1) {
        if (!uint_arg(env, v[3], &config.anchor) || !uint_arg(env, v[4], &config.flags))
            return NULL;
        float *fields[] = {&config.width, &config.height, &config.margin, &config.offset,
                           &config.reference_rem};
        for (size_t i = 0; i < 5; i++) {
            double value;
            if (!number_arg(env, v[5 + i], &value))
                return NULL;
            if (!isfinite(value) || value < (i == 3 ? -4096 : 0) || value > (i == 4 ? 256 : 4096))
                return fail(env, "Popover option out of range");
            *fields[i] = (float)value;
        }
        if (config.anchor > CUI_BRIDGE_POPOVER_RIGHT_CENTER_V1 || config.flags > 127 ||
            (!(config.flags & CUI_BRIDGE_POPOVER_CUSTOM_OFFSET_V1) && config.offset != 0))
            return fail(env, "invalid Popover options");
    }
    return status(env, configure(app->bridge, node->node, op == 2 ? 0 : sizeof(config),
                                 op == 2 ? NULL : &config));
}
