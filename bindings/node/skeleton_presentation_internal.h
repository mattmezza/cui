/* SPDX-License-Identifier: MIT */
/* Optional V1 calls build and return value records from copied scalars. */
typedef struct {
    uint32_t (*versions)(void), (*config_size)(void), (*presentation_size)(void);
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node,
                                   const cui_bridge_skeleton_config_v1 *);
    cui_bridge_status (*config)(cui_bridge_app *, cui_bridge_node, cui_bridge_skeleton_config_v1 *);
    cui_bridge_status (*sample)(cui_bridge_app *, cui_bridge_node,
                                cui_bridge_skeleton_presentation_v1 *);
} skeleton_api;

_Static_assert(sizeof(cui_bridge_skeleton_config_v1) == 40, "Skeleton configuration V1 ABI");
_Static_assert(sizeof(cui_bridge_skeleton_presentation_v1) == 40, "Skeleton presentation V1 ABI");

static bool skeleton_resolve(skeleton_api *api) {
#define SKELETON_SYMBOL(field, suffix)                                                             \
    optional_bridge_symbol(&api->field, sizeof api->field, "cui_bridge_skeleton_" suffix)
    return SKELETON_SYMBOL(versions, "versions") &&
           SKELETON_SYMBOL(config_size, "config_size_v1") &&
           SKELETON_SYMBOL(presentation_size, "presentation_size_v1") &&
           SKELETON_SYMBOL(configure, "configure_v1") && SKELETON_SYMBOL(config, "get_config_v1") &&
           SKELETON_SYMBOL(sample, "get_presentation_v1") &&
           (api->versions() & CUI_BRIDGE_SKELETON_V1) && api->config_size() == 40 &&
           api->presentation_size() == 40;
#undef SKELETON_SYMBOL
}

static napi_value skeleton_configure(napi_env env, napi_value *v, const skeleton_api *api,
                                     js_app *app, js_node *node) {
    cui_bridge_skeleton_config_v1 config = {.struct_size = sizeof config,
                                            .schema_version = CUI_BRIDGE_SKELETON_V1};
    if (!uint_arg(env, v[3], &config.flags) || !uint_arg(env, v[4], &config.width_mode) ||
        !uint_arg(env, v[10], &config.background_rgba))
        return NULL;
    float *floats[] = {&config.width, &config.height, &config.max_width, &config.radius,
                       &config.duration};
    for (size_t i = 0; i < sizeof floats / sizeof floats[0]; i++) {
        double value;
        if (!number_arg(env, v[5 + i], &value))
            return NULL;
        *floats[i] = (float)value;
        if (value != 0 && *floats[i] == 0)
            return fail(env, "Skeleton configuration value underflows a bridge float");
    }
    cui_bridge_status code = api->configure(app->bridge, node->node, &config);
    return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
}

/* op0 probe, op1 configure, op2 copied configuration, op3 copied frame sample. */
static napi_value skeleton_presentation(napi_env env, napi_callback_info info) {
    napi_value v[11];
    js_app *app;
    uint32_t op;
    if (!args(env, info, 2, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &op))
        return NULL;
    if (op > 3)
        return fail(env, "unknown Skeleton operation");
    skeleton_api api;
    bool supported = skeleton_resolve(&api);
    if (op == 0) {
        napi_value result;
        return napi_get_boolean(env, supported, &result) == napi_ok ? result : NULL;
    }
    if (!supported)
        return fail(env,
                    "responsive Skeleton V1 requires a complete, compatible bridge symbol group");

    js_node *node;
    if (!args(env, info, op == 1 ? 11 : 3, v) || !node_arg(env, v[2], app, &node))
        return NULL;
    if (op == 1)
        return skeleton_configure(env, v, &api, app, node);
    if (op == 2) {
        cui_bridge_skeleton_config_v1 config = {.struct_size = sizeof config,
                                                .schema_version = CUI_BRIDGE_SKELETON_V1};
        cui_bridge_status code = api.config(app->bridge, node->node, &config);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (config.struct_size != sizeof config || config.schema_version != CUI_BRIDGE_SKELETON_V1)
            return fail(env, "invalid copied Skeleton configuration header");
        const char *const names[] = {"flags",    "widthMode", "width",    "height",
                                     "maxWidth", "radius",    "duration", "backgroundRgba"};
        const double values[] = {config.flags,    config.width_mode,     config.width,
                                 config.height,   config.max_width,      config.radius,
                                 config.duration, config.background_rgba};
        return numeric_object(env, names, values, sizeof values / sizeof values[0]);
    }

    cui_bridge_skeleton_presentation_v1 sample = {.struct_size = sizeof sample,
                                                  .schema_version = CUI_BRIDGE_SKELETON_V1};
    cui_bridge_status code = api.sample(app->bridge, node->node, &sample);
    if (code == CUI_BRIDGE_EMPTY)
        return undefined(env);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (sample.struct_size != sizeof sample || sample.schema_version != CUI_BRIDGE_SKELETON_V1)
        return fail(env, "invalid copied Skeleton sample header");
    const char *const names[] = {"coreTime", "nextWakeSeconds", "phase", "opacity"};
    const double values[] = {sample.core_time, sample.next_wake_seconds, sample.phase,
                             sample.opacity};
    napi_value result = numeric_object(env, names, values, sizeof values / sizeof values[0]);
    napi_value frame;
    if (!result || napi_create_bigint_uint64(env, sample.frame_sequence, &frame) != napi_ok ||
        napi_set_named_property(env, result, "frameSequence", frame) != napi_ok)
        return NULL;
    return result;
}
