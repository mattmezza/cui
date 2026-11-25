/* SPDX-License-Identifier: MIT */
typedef struct {
    uint32_t (*versions)(void), (*record_size)(uint32_t);
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node, cui_bridge_icon_handle_v1,
                                   const cui_bridge_icon_options_v1 *, const char *, uint32_t);
    cui_bridge_status (*config)(cui_bridge_app *, cui_bridge_node, cui_bridge_icon_handle_v1 *,
                                cui_bridge_icon_options_v1 *);
    cui_bridge_status (*name)(cui_bridge_app *, cui_bridge_node, char *, uint32_t, uint32_t *);
    cui_bridge_status (*reset)(cui_bridge_app *, cui_bridge_node);
    cui_bridge_status (*result)(cui_bridge_app *, cui_bridge_node, cui_bridge_icon_result_v1 *);
} icon_node_api;

typedef struct {
    uint32_t (*versions)(void), (*record_size)(uint32_t);
    cui_bridge_status (*defaults)(cui_bridge_badge_config_v1 *);
    cui_bridge_status (*create)(cui_bridge_app *, cui_bridge_node, const char *,
                                const cui_bridge_badge_config_v1 *,
                                const cui_bridge_icon_options_v1 *, const char *, uint32_t,
                                const char *, uint32_t, cui_bridge_node *);
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node,
                                   const cui_bridge_badge_config_v1 *,
                                   const cui_bridge_icon_options_v1 *, const char *, uint32_t,
                                   const char *, uint32_t);
    cui_bridge_status (*config)(cui_bridge_app *, cui_bridge_node, cui_bridge_badge_config_v1 *,
                                cui_bridge_icon_options_v1 *);
    cui_bridge_status (*text)(cui_bridge_app *, cui_bridge_node, uint32_t, char *, uint32_t,
                              uint32_t *);
    cui_bridge_status (*result)(cui_bridge_app *, cui_bridge_node, cui_bridge_badge_result_v1 *);
    cui_bridge_status (*bounds)(cui_bridge_app *, cui_bridge_node, cui_bridge_rect *);
} configured_badge_api;

_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128 &&
                   sizeof(cui_bridge_icon_style_v1) == 128 &&
                   offsetof(cui_bridge_icon_style_v1, corners) == 64 &&
                   offsetof(cui_bridge_icon_style_v1, padding_edges) == 108,
               "Icon box V1 ABI");
_Static_assert(sizeof(cui_bridge_icon_constraints_v1) == 32 &&
                   sizeof(cui_bridge_icon_options_v1) == 256 &&
                   offsetof(cui_bridge_icon_options_v1, transform) == 64 &&
                   offsetof(cui_bridge_icon_options_v1, style) == 88 &&
                   offsetof(cui_bridge_icon_options_v1, constraints) == 216 &&
                   sizeof(cui_bridge_icon_result_v1) == 40,
               "Icon presentation V1 ABI");
_Static_assert(sizeof(cui_bridge_badge_config_v1) == 64 &&
                   offsetof(cui_bridge_badge_config_v1, count) == 32 &&
                   offsetof(cui_bridge_badge_config_v1, maximum) == 40 &&
                   offsetof(cui_bridge_badge_config_v1, artwork) == 56 &&
                   sizeof(cui_bridge_badge_result_v1) == 40,
               "Badge V1 ABI");

static bool icon_node_resolve(icon_node_api *api) {
    icon_resources_api resources;
#define ICON_NODE_SYMBOL(member, suffix)                                                           \
    optional_bridge_symbol(&api->member, sizeof api->member, "cui_bridge_icon_" suffix)
    return ICON_NODE_SYMBOL(versions, "presentation_versions") &&
           ICON_NODE_SYMBOL(record_size, "presentation_record_size_v1") &&
           ICON_NODE_SYMBOL(configure, "configure_node_v1") &&
           ICON_NODE_SYMBOL(config, "get_node_config_v1") &&
           ICON_NODE_SYMBOL(name, "copy_node_name_v1") &&
           ICON_NODE_SYMBOL(reset, "reset_node_v1") &&
           ICON_NODE_SYMBOL(result, "get_node_result_v1") && (api->versions() & 1) &&
           api->record_size(CUI_BRIDGE_ICON_STYLE_RECORD_V1) == sizeof(cui_bridge_icon_style_v1) &&
           api->record_size(CUI_BRIDGE_ICON_CONSTRAINTS_RECORD_V1) ==
               sizeof(cui_bridge_icon_constraints_v1) &&
           api->record_size(CUI_BRIDGE_ICON_OPTIONS_RECORD_V1) ==
               sizeof(cui_bridge_icon_options_v1) &&
           api->record_size(CUI_BRIDGE_ICON_RESULT_RECORD_V1) ==
               sizeof(cui_bridge_icon_result_v1) &&
           icon_resources_resolve(&resources);
#undef ICON_NODE_SYMBOL
}

static bool configured_badge_resolve(configured_badge_api *api) {
    icon_node_api icons;
#define BADGE_SYMBOL(member, suffix)                                                               \
    optional_bridge_symbol(&api->member, sizeof api->member, "cui_bridge_badge_" suffix)
    return BADGE_SYMBOL(versions, "versions") && BADGE_SYMBOL(record_size, "record_size_v1") &&
           BADGE_SYMBOL(defaults, "default_config_v1") && BADGE_SYMBOL(create, "create_v1") &&
           BADGE_SYMBOL(configure, "configure_v1") && BADGE_SYMBOL(config, "get_config_v1") &&
           BADGE_SYMBOL(text, "copy_text_v1") && BADGE_SYMBOL(result, "get_result_v1") &&
           BADGE_SYMBOL(bounds, "indicator_bounds_v1") && (api->versions() & 1) &&
           api->record_size(CUI_BRIDGE_BADGE_CONFIG_RECORD_V1) ==
               sizeof(cui_bridge_badge_config_v1) &&
           api->record_size(CUI_BRIDGE_BADGE_RESULT_RECORD_V1) ==
               sizeof(cui_bridge_badge_result_v1) &&
           icon_node_resolve(&icons);
#undef BADGE_SYMBOL
}

static napi_value icon_bool(napi_env env, bool value) {
    napi_value result;
    return napi_get_boolean(env, value, &result) == napi_ok ? result : NULL;
}

static bool icon_wire_buffer(napi_env env, napi_value v, size_t size, unsigned char **out) {
    void *bytes;
    size_t actual;
    if (napi_get_buffer_info(env, v, &bytes, &actual) != napi_ok || actual != size || !bytes) {
        fail(env, "invalid Icon or Badge input record size");
        return false;
    }
    *out = bytes;
    return true;
}

/* The private JS records encode scalar fields as little-endian. Convert each
 * aligned 32-bit field (including float bits) to the host order. Badge's two
 * 64-bit counters have their own conversions. No C pointers enter the records. */
static bool icon_options_arg(napi_env env, napi_value value, cui_bridge_icon_options_v1 *options) {
    unsigned char *wire;
    if (!icon_wire_buffer(env, value, sizeof *options, &wire))
        return false;
    for (size_t at = 0; at < sizeof *options; at += 4)
        wire_word_in(options, wire, at);
    return true;
}

static napi_value icon_options_snapshot(napi_env env, const cui_bridge_icon_options_v1 *options) {
    unsigned char wire[sizeof *options];
    for (size_t at = 0; at < sizeof *options; at += 4)
        wire_word_out(wire, options, at);
    napi_value result;
    return napi_create_buffer_copy(env, sizeof wire, wire, NULL, &result) == napi_ok ? result
                                                                                     : NULL;
}

static bool icon_expected_handle(napi_env env, napi_value value, js_app *app,
                                 cui_bridge_icon_handle_v1 *handle) {
    napi_valuetype type;
    if (napi_typeof(env, value, &type) != napi_ok)
        return false;
    *handle = (cui_bridge_icon_handle_v1){0};
    if (type == napi_null)
        return true;
    js_icon *icon;
    if (!icon_arg(env, value, app, &icon))
        return false;
    *handle = icon->handle;
    return true;
}

static bool equal_icon_handle(cui_bridge_icon_handle_v1 a, cui_bridge_icon_handle_v1 b) {
    return a.slot == b.slot && a.generation == b.generation;
}

static bool equal_node_handle(cui_bridge_node a, cui_bridge_node b) {
    return a.slot == b.slot && a.generation == b.generation;
}

static napi_value icon_frame_result(napi_env env, cui_bridge_node expected, cui_bridge_node node,
                                    uint64_t frame, uint32_t error, uint32_t frame_status) {
    if (!equal_node_handle(expected, node) || !frame || error > 5 ||
        frame_status > CUI_BRIDGE_CORE_ERROR)
        return fail(env, "invalid copied Icon or Badge result");
    const char *const names[] = {"iconError", "frameStatus"};
    const double values[] = {error, frame_status};
    napi_value out = numeric_object(env, names, values, 2);
    return out && icon_bigint_property(env, out, "frameSequence", frame) ? out : NULL;
}

enum icon_node_op {
    ICON_NODE_PROBE,
    ICON_NODE_CONFIGURE,
    ICON_NODE_CONFIG,
    ICON_NODE_RESET,
    ICON_NODE_RESULT
};

static napi_value icon_node(napi_env env, napi_callback_info info) {
    static const size_t arity[] = {2, 6, 4, 3, 3};
    napi_value v[6];
    size_t count = sizeof v / sizeof v[0];
    uint32_t op;
    js_app *owner;
    if (napi_get_cb_info(env, info, &count, v, NULL, NULL) != napi_ok || count < 2)
        return fail(env, "missing Icon presentation arguments");
    if (!app_arg(env, v[0], &owner) || !uint_arg(env, v[1], &op))
        return NULL;
    if (op >= sizeof arity / sizeof arity[0] || count < arity[op])
        return fail(env, "invalid Icon presentation operation");
    icon_node_api api;
    bool supported = icon_node_resolve(&api);
    if (op == ICON_NODE_PROBE)
        return icon_bool(env, supported);
    if (!supported)
        return fail(env, "Icon presentation V1 requires a complete, compatible bridge");
    js_node *node;
    if (!node_arg(env, v[2], owner, &node))
        return NULL;
    cui_bridge_app *app = owner->bridge;
    cui_bridge_status code;
    if (op == ICON_NODE_CONFIGURE) {
        js_icon *artwork;
        cui_bridge_icon_options_v1 options;
        const char *name;
        uint32_t name_bytes;
        if (!icon_arg(env, v[3], owner, &artwork) || !icon_options_arg(env, v[4], &options) ||
            !icon_bytes(env, v[5], ICON_TEXT_MAX, &name, &name_bytes))
            return NULL;
        code = api.configure(app, node->node, artwork->handle, &options, name, name_bytes);
        return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
    }
    if (op == ICON_NODE_CONFIG) {
        cui_bridge_icon_handle_v1 expected, resource;
        if (!icon_expected_handle(env, v[3], owner, &expected))
            return NULL;
        cui_bridge_icon_options_v1 options = {.struct_size = sizeof options, .schema_version = 1};
        code = api.config(app, node->node, &resource, &options);
        if (code == CUI_BRIDGE_EMPTY)
            return undefined(env);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (!expected.slot || !equal_icon_handle(expected, resource))
            return fail(env, "invalid copied Icon resource handle");
        char name[ICON_TEXT_MAX + 1] = {0};
        uint32_t size = 0;
        code = api.name(app, node->node, name, sizeof name, &size);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        napi_value out;
        if (napi_create_object(env, &out) != napi_ok ||
            !icon_property(env, out, "options", icon_options_snapshot(env, &options)) ||
            !icon_property(env, out, "name", icon_string(env, name, size, sizeof name)))
            return NULL;
        return out;
    }
    if (op == ICON_NODE_RESET) {
        code = api.reset(app, node->node);
        return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
    }
    cui_bridge_icon_result_v1 result = {.struct_size = sizeof result, .schema_version = 1};
    code = api.result(app, node->node, &result);
    if (code == CUI_BRIDGE_EMPTY)
        return undefined(env);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (result.struct_size != sizeof result || result.schema_version != 1 || result.reserved)
        return fail(env, "invalid copied Icon result header");
    return icon_frame_result(env, node->node, result.node, result.frame_sequence, result.icon_error,
                             result.frame_status);
}

typedef struct {
    cui_bridge_badge_config_v1 config;
    cui_bridge_icon_options_v1 artwork;
    const char *name, *named_icon;
    uint32_t name_bytes, named_icon_bytes;
} badge_input;

static bool badge_config_arg(napi_env env, const napi_value v[], js_app *app, badge_input *input) {
    unsigned char *wire;
    cui_bridge_badge_config_v1 *config = &input->config;
    if (!icon_expected_handle(env, v[0], app, &config->artwork) ||
        !icon_wire_buffer(env, v[1], sizeof *config, &wire))
        return false;
    if (load_le32(wire + 56) || load_le32(wire + 60)) {
        fail(env, "use a tagged Icon resource for Badge artwork");
        return false;
    }
    if (!config->artwork.slot) {
        napi_valuetype options_type;
        if (napi_typeof(env, v[2], &options_type) != napi_ok || options_type != napi_null) {
            fail(env, "Badge artwork options require an owned Icon resource");
            return false;
        }
    }
    for (size_t at = 0; at < 32; at += 4)
        wire_word_in(config, wire, at);
    wire_big_in(config, wire, 32);
    wire_big_in(config, wire, 40);
    wire_word_in(config, wire, 48);
    wire_word_in(config, wire, 52);
    return (!config->artwork.slot || icon_options_arg(env, v[2], &input->artwork)) &&
           icon_bytes(env, v[3], ICON_TEXT_MAX, &input->name, &input->name_bytes) &&
           icon_bytes(env, v[4], ICON_TEXT_MAX, &input->named_icon, &input->named_icon_bytes);
}

static napi_value badge_make_node(napi_env env, napi_value owner_value, js_node *parent,
                                  const configured_badge_api *api, const char *key,
                                  const badge_input *i) {
    js_node *node = calloc(1, sizeof *node);
    if (!node)
        return fail(env, "allocation failed");
    node->owner = parent->owner;
    napi_value out;
    if (napi_create_reference(env, owner_value, 1, &node->app_ref) != napi_ok ||
        napi_create_external(env, node, node_final, NULL, &out) != napi_ok) {
        node_final(env, node, NULL);
        return fail(env, "could not create configured Badge handle");
    }
    if (napi_type_tag_object(env, out, &node_tag) != napi_ok)
        return fail(env, "could not tag configured Badge handle");
    cui_bridge_status code =
        api->create(node->owner->bridge, parent->node, key, &i->config,
                    i->config.artwork.slot ? &i->artwork : NULL, i->name, i->name_bytes,
                    i->named_icon, i->named_icon_bytes, &node->node);
    return code == CUI_BRIDGE_OK ? out : status(env, code);
}

static napi_value badge_config_snapshot(napi_env env, const cui_bridge_badge_config_v1 *config) {
    unsigned char wire[sizeof *config] = {0};
    for (size_t at = 0; at < 32; at += 4)
        wire_word_out(wire, config, at);
    wire_big_out(wire, config, 32);
    wire_big_out(wire, config, 40);
    wire_word_out(wire, config, 48);
    wire_word_out(wire, config, 52);
    napi_value out;
    return napi_create_buffer_copy(env, sizeof wire, wire, NULL, &out) == napi_ok ? out : NULL;
}

static napi_value badge_read_config(napi_env env, const configured_badge_api *api, js_node *node,
                                    cui_bridge_icon_handle_v1 expected) {
    cui_bridge_app *app = node->owner->bridge;
    cui_bridge_badge_config_v1 config = {.struct_size = sizeof config, .schema_version = 1};
    cui_bridge_icon_options_v1 artwork = {.struct_size = sizeof artwork, .schema_version = 1};
    cui_bridge_status code = api->config(app, node->node, &config, &artwork);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (!equal_icon_handle(config.artwork, expected))
        return fail(env, "invalid copied Badge artwork handle");
    char names[2][ICON_TEXT_MAX + 1] = {{0}};
    uint32_t sizes[2] = {0};
    for (uint32_t field = 0; field < 2; ++field) {
        code = api->text(app, node->node, field, names[field], sizeof names[field], &sizes[field]);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
    }
    napi_value out;
    if (napi_create_object(env, &out) != napi_ok ||
        !icon_property(env, out, "config", badge_config_snapshot(env, &config)) ||
        !icon_property(env, out, "options", icon_options_snapshot(env, &artwork)) ||
        !icon_property(env, out, "name", icon_string(env, names[0], sizes[0], sizeof names[0])) ||
        !icon_property(env, out, "namedIcon",
                       icon_string(env, names[1], sizes[1], sizeof names[1])))
        return NULL;
    return out;
}

enum badge_op {
    BADGE_PROBE,
    BADGE_CREATE,
    BADGE_CONFIGURE,
    BADGE_CONFIG,
    BADGE_RESULT,
    BADGE_BOUNDS
};

static napi_value configured_badge(napi_env env, napi_callback_info info) {
    static const size_t arity[] = {2, 9, 8, 4, 3, 3};
    napi_value v[9];
    size_t count = sizeof v / sizeof v[0];
    uint32_t op;
    js_app *owner;
    if (napi_get_cb_info(env, info, &count, v, NULL, NULL) != napi_ok || count < 2)
        return fail(env, "missing configured Badge arguments");
    if (!app_arg(env, v[0], &owner) || !uint_arg(env, v[1], &op))
        return NULL;
    if (op >= sizeof arity / sizeof arity[0] || count < arity[op])
        return fail(env, "invalid configured Badge operation");
    configured_badge_api api;
    bool supported = configured_badge_resolve(&api);
    if (op == BADGE_PROBE)
        return icon_bool(env, supported);
    if (!supported)
        return fail(env, "configured Badge V1 requires a complete, compatible bridge");
    js_node *node;
    if (!node_arg(env, v[2], owner, &node))
        return NULL;
    cui_bridge_app *app = owner->bridge;
    cui_bridge_status code;
    if (op == BADGE_CREATE || op == BADGE_CONFIGURE) {
        badge_input input;
        if (!badge_config_arg(env, v + (op == BADGE_CREATE ? 4 : 3), owner, &input))
            return NULL;
        if (op == BADGE_CREATE) {
            char *key;
            if (!string_arg(env, v[3], &key))
                return NULL;
            napi_value out = badge_make_node(env, v[0], node, &api, key, &input);
            free(key);
            return out;
        }
        code = api.configure(app, node->node, &input.config,
                             input.config.artwork.slot ? &input.artwork : NULL, input.name,
                             input.name_bytes, input.named_icon, input.named_icon_bytes);
        return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
    }
    if (op == BADGE_CONFIG) {
        cui_bridge_icon_handle_v1 handle;
        return icon_expected_handle(env, v[3], owner, &handle)
                   ? badge_read_config(env, &api, node, handle)
                   : NULL;
    }
    if (op == BADGE_BOUNDS) {
        cui_bridge_rect bounds;
        code = api.bounds(app, node->node, &bounds);
        if (code == CUI_BRIDGE_EMPTY)
            return undefined(env);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (!isfinite(bounds.x) || !isfinite(bounds.y) || !isfinite(bounds.width) ||
            !isfinite(bounds.height) || bounds.width <= 0 || bounds.height <= 0)
            return fail(env, "invalid copied Badge bounds");
        const char *const names[] = {"x", "y", "width", "height"};
        const double values[] = {bounds.x, bounds.y, bounds.width, bounds.height};
        return numeric_object(env, names, values, 4);
    }
    cui_bridge_badge_result_v1 result = {.struct_size = sizeof result, .schema_version = 1};
    code = api.result(app, node->node, &result);
    if (code == CUI_BRIDGE_EMPTY)
        return undefined(env);
    if (code != CUI_BRIDGE_OK)
        return status(env, code);
    if (result.struct_size != sizeof result || result.schema_version != 1 || result.submitted > 1 ||
        (result.flags & ~1u) || (!result.flags && result.icon_error))
        return fail(env, "invalid copied Badge result header");
    napi_value out = icon_frame_result(env, node->node, result.node, result.frame_sequence,
                                       result.icon_error, result.frame_status);
    if (!out || !icon_property(env, out, "submitted", icon_bool(env, result.submitted != 0)))
        return NULL;
    if (!result.flags) {
        napi_value null_value;
        if (napi_get_null(env, &null_value) != napi_ok ||
            !icon_property(env, out, "iconError", null_value))
            return NULL;
    }
    return out;
}
