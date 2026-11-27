/* SPDX-License-Identifier: MIT */
/* Optional V1 calls build and return value records from copied scalars. */
typedef struct {
    uint32_t (*versions)(void), (*config_size)(void), (*presentation_size)(void);
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node,
                                   const cui_bridge_spinner_config_v1 *);
    cui_bridge_status (*config)(cui_bridge_app *, cui_bridge_node, cui_bridge_spinner_config_v1 *);
    cui_bridge_status (*sample)(cui_bridge_app *, cui_bridge_node,
                                cui_bridge_spinner_presentation_v1 *);
} spinner_api;

_Static_assert(sizeof(cui_bridge_spinner_config_v1) == 100, "Spinner configuration V1 ABI");
_Static_assert(sizeof(cui_bridge_spinner_presentation_v1) == 40, "Spinner presentation V1 ABI");

static bool spinner_resolve(spinner_api *api) {
#define SPINNER_SYMBOL(field, suffix)                                                              \
    optional_bridge_symbol(&api->field, sizeof api->field, "cui_bridge_spinner_" suffix)
    return SPINNER_SYMBOL(versions, "versions") && SPINNER_SYMBOL(config_size, "config_size_v1") &&
           SPINNER_SYMBOL(presentation_size, "presentation_size_v1") &&
           SPINNER_SYMBOL(configure, "configure_v1") && SPINNER_SYMBOL(config, "get_config_v1") &&
           SPINNER_SYMBOL(sample, "get_presentation_v1") &&
           (api->versions() & CUI_BRIDGE_SPINNER_V1) && api->config_size() == 100 &&
           api->presentation_size() == 40;
#undef SPINNER_SYMBOL
}

static napi_value spinner_configure(napi_env env, napi_value *v, const spinner_api *api,
                                    js_app *app, js_node *node) {
    cui_bridge_spinner_config_v1 config = {.struct_size = sizeof config,
                                           .schema_version = CUI_BRIDGE_SPINNER_V1};
    if (!uint_arg(env, v[3], &config.flags) || !uint_arg(env, v[4], &config.size) ||
        !uint_arg(env, v[8], &config.rgba) || !uint_arg(env, v[9], &config.easing))
        return NULL;
    float *floats[] = {&config.pixels, &config.rem_size, &config.duration};
    for (size_t i = 0; i < 3; i++) {
        double value;
        if (!number_arg(env, v[5 + i], &value))
            return NULL;
        *floats[i] = (float)value;
        if (value != 0 && *floats[i] == 0)
            return fail(env, "Spinner value underflows a bridge float");
    }
    char *icon = NULL;
    if (!string_arg_limit(env, v[10], &icon, 63))
        return NULL;
    memcpy(config.icon, icon, strlen(icon) + 1);
    free(icon);
    cui_bridge_status code = api->configure(app->bridge, node->node, &config);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}

/* op0 probe, op1 configure, op2 copied configuration, op3 copied frame sample. */
static napi_value spinner_presentation(napi_env env, napi_callback_info info) {
    napi_value v[11];
    js_app *app;
    uint32_t op;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &op))
        return NULL;
    if (op > 3)
        return fail(env, "unknown Spinner operation");
    spinner_api api;
    bool supported = spinner_resolve(&api);
    if (op == 0) {
        napi_value result;
        return napi_get_boolean(env, supported, &result) == napi_ok ? result : NULL;
    }
    if (!supported)
        return fail(env, "rotating Spinner V1 requires a complete, compatible bridge symbol group");

    js_node *node;
    if (!args(env, info, op == 1 ? 11 : 3, v) || !node_arg(env, v[2], app, &node))
        return NULL;
    if (op == 1)
        return spinner_configure(env, v, &api, app, node);
    if (op == 2) {
        cui_bridge_spinner_config_v1 config = {.struct_size = sizeof config,
                                               .schema_version = CUI_BRIDGE_SPINNER_V1};
        cui_bridge_status code = api.config(app->bridge, node->node, &config);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (config.struct_size != sizeof config || config.schema_version != CUI_BRIDGE_SPINNER_V1)
            return fail(env, "invalid copied Spinner configuration header");
        if (!memchr(config.icon, 0, sizeof config.icon))
            return fail(env, "invalid copied Spinner icon");
        const char *const names[] = {"flags",    "size", "pixels", "remSize",
                                     "duration", "rgba", "easing"};
        const double values[] = {config.flags,    config.size, config.pixels, config.rem_size,
                                 config.duration, config.rgba, config.easing};
        napi_value result = numeric_object(env, names, values, 7), icon;
        if (!result ||
            napi_create_string_utf8(env, config.icon, NAPI_AUTO_LENGTH, &icon) != napi_ok ||
            napi_set_named_property(env, result, "icon", icon) != napi_ok)
            return NULL;
        return result;
    }

    cui_bridge_spinner_presentation_v1 sample = {.struct_size = sizeof sample,
                                                 .schema_version = CUI_BRIDGE_SPINNER_V1};
    cui_bridge_status code = api.sample(app->bridge, node->node, &sample);
    if (code == CUI_BRIDGE_EMPTY)
        return undefined(env);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (sample.struct_size != sizeof sample || sample.schema_version != CUI_BRIDGE_SPINNER_V1 ||
        sample.reserved != 0)
        return fail(env, "invalid copied Spinner sample header");
    const char *const names[] = {"coreTime", "nextWakeSeconds", "phase"};
    const double values[] = {sample.core_time, sample.next_wake_seconds, sample.phase};
    napi_value result = numeric_object(env, names, values, sizeof values / sizeof values[0]);
    napi_value frame;
    if (!result || napi_create_bigint_uint64(env, sample.frame_sequence, &frame) != napi_ok ||
        napi_set_named_property(env, result, "frameSequence", frame) != napi_ok)
        return NULL;
    return result;
}
