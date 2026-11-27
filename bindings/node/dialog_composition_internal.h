/* SPDX-License-Identifier: MIT */
/* Resolve the optional functions against the loaded bridge. */
typedef cui_bridge_status (*dialog_configure_fn)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                                 const cui_bridge_dialog_config_v1 *);
typedef cui_bridge_status (*dialog_slot_fn)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                            const char *, cui_bridge_node *);
typedef cui_bridge_status (*dialog_action_fn)(cui_bridge_app *, cui_bridge_node, uint32_t);
enum { DIALOG_PROBE, DIALOG_CONFIGURE, DIALOG_CLEAR, DIALOG_ACTION };

static napi_value dialog_options(napi_env env, napi_callback_info info) {
    napi_value values[7], result;
    js_app *app;
    uint32_t operation;
    if (!args(env, info, 2, values) || !app_arg(env, values[0], &app) ||
        !uint_arg(env, values[1], &operation))
        return NULL;
    dialog_configure_fn configure = NULL;
    dialog_action_fn action = NULL;
    dialog_slot_fn slot = NULL;
    bool supported =
        optional_bridge_symbol(&configure, sizeof configure, "cui_bridge_dialog_configure_v1") &&
        optional_bridge_symbol(&action, sizeof action, "cui_bridge_dialog_set_action_v1") &&
        optional_bridge_symbol(&slot, sizeof slot, "cui_bridge_dialog_slot_create_v1");
    if (operation == DIALOG_PROBE)
        return napi_get_boolean(env, supported, &result) == napi_ok ? result : NULL;
    if (!supported)
        return fail(env, "Dialog composition requires a compatible bridge");
    if (operation > DIALOG_ACTION)
        return fail(env, "Invalid Dialog operation");

    size_t count = operation == DIALOG_CONFIGURE ? 7 : operation == DIALOG_ACTION ? 4 : 3;
    js_node *node;
    if (!args(env, info, count, values) || !node_arg(env, values[2], app, &node))
        return NULL;
    if (operation == DIALOG_ACTION) {
        uint32_t value;
        if (!uint_arg(env, values[3], &value))
            return NULL;
        return status(env, action(app->bridge, node->node, value));
    }
    if (operation == DIALOG_CLEAR)
        return status(env, configure(app->bridge, node->node, 0, NULL));
    cui_bridge_dialog_config_v1 config = {0};
    if (!uint_arg(env, values[3], &config.flags))
        return NULL;
    float *dimensions[] = {&config.width, &config.height, &config.margin};
    for (size_t i = 0; i < 3; i++) {
        double value;
        if (!number_arg(env, values[4 + i], &value))
            return NULL;
        if (!isfinite(value) || value < 0 || value > 4096)
            return fail(env, "Dialog dimension out of range");
        *dimensions[i] = (float)value;
    }
    return status(env, configure(app->bridge, node->node, sizeof config, &config));
}

static napi_value dialog_slot(napi_env env, napi_callback_info info) {
    napi_value values[4];
    js_app *app;
    js_node *parent;
    uint32_t role;
    if (!args(env, info, 4, values) || !app_arg(env, values[0], &app) ||
        !node_arg(env, values[1], app, &parent) || !uint_arg(env, values[2], &role))
        return NULL;
    dialog_slot_fn create = NULL;
    if (!optional_bridge_symbol(&create, sizeof create, "cui_bridge_dialog_slot_create_v1"))
        return fail(env, "Dialog slots unavailable");
    char *key = NULL;
    if (!string_arg(env, values[3], &key))
        return NULL;
    js_node *node = calloc(1, sizeof *node);
    if (!node) {
        free(key);
        return fail(env, "allocation failed");
    }
    node->owner = app;
    cui_bridge_status code = create(app->bridge, parent->node, role, key, &node->node);
    free(key);
    if (code != CUI_BRIDGE_OK) {
        free(node);
        return status(env, code);
    }
    return expose_node(env, values[0], node);
}
