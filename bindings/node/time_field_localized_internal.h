/* SPDX-License-Identifier: MIT */
/* Optional complete group; JS wires are explicit LE scalar copies, never C views. */
typedef struct {
    uint32_t (*versions)(void), (*config_size)(void), (*result_size)(void), (*change_size)(void);
    cui_bridge_status (*enable)(cui_bridge_app *, uint32_t);
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node,
                                   const cui_bridge_time_field_config_v1 *);
    cui_bridge_status (*config)(cui_bridge_app *, cui_bridge_node,
                                cui_bridge_time_field_config_v1 *);
    cui_bridge_status (*result)(cui_bridge_app *, cui_bridge_node,
                                cui_bridge_time_field_result_v1 *);
    cui_bridge_status (*clear)(cui_bridge_app *, cui_bridge_node);
    cui_bridge_status (*poll)(cui_bridge_app *, cui_bridge_time_field_change_v1 *);
} localized_time_api;
_Static_assert(sizeof(cui_bridge_time_field_config_v1) == 616, "Time config wire");
_Static_assert(sizeof(cui_bridge_time_field_result_v1) == 80, "Time result wire");
_Static_assert(sizeof(cui_bridge_time_field_change_v1) == 64, "Time change wire");
_Static_assert(offsetof(cui_bridge_time_field_config_v1, edit_generation) == 56, "Time generation");
_Static_assert(offsetof(cui_bridge_time_field_config_v1, tokens) == 64, "Time tokens");
_Static_assert(offsetof(cui_bridge_time_field_config_v1, names) == 344, "Time names");
_Static_assert(offsetof(cui_bridge_time_field_result_v1, frame_sequence) == 24,
               "Time result frame");
_Static_assert(offsetof(cui_bridge_time_field_change_v1, frame_sequence) == 24,
               "Time change frame");
/* Conversion offsets are binding-local schema, checked against native C layout. */
#define TF_OFFSET(type, field, at)                                                                 \
    _Static_assert(offsetof(type, field) == at, "Time native offset: " #field)
TF_OFFSET(cui_bridge_time_field_config_v1, struct_size, 0);
TF_OFFSET(cui_bridge_time_field_config_v1, version, 4);
TF_OFFSET(cui_bridge_time_field_config_v1, flags, 8);
TF_OFFSET(cui_bridge_time_field_config_v1, cycle, 12);
TF_OFFSET(cui_bridge_time_field_config_v1, precision, 16);
TF_OFFSET(cui_bridge_time_field_config_v1, bounds_flags, 20);
TF_OFFSET(cui_bridge_time_field_config_v1, reserved, 24);
TF_OFFSET(cui_bridge_time_field_config_v1, minimum, 32);
TF_OFFSET(cui_bridge_time_field_config_v1, maximum, 44);
TF_OFFSET(cui_bridge_time_field_result_v1, struct_size, 0);
TF_OFFSET(cui_bridge_time_field_result_v1, version, 4);
TF_OFFSET(cui_bridge_time_field_result_v1, reserved, 8);
TF_OFFSET(cui_bridge_time_field_result_v1, node, 16);
TF_OFFSET(cui_bridge_time_field_result_v1, status, 32);
TF_OFFSET(cui_bridge_time_field_result_v1, validation, 36);
TF_OFFSET(cui_bridge_time_field_result_v1, changed, 40);
TF_OFFSET(cui_bridge_time_field_result_v1, rejected_count, 44);
TF_OFFSET(cui_bridge_time_field_result_v1, previous, 48);
TF_OFFSET(cui_bridge_time_field_result_v1, value, 60);
TF_OFFSET(cui_bridge_time_field_result_v1, queued_change_count, 72);
TF_OFFSET(cui_bridge_time_field_result_v1, frame_status, 76);
TF_OFFSET(cui_bridge_time_field_change_v1, struct_size, 0);
TF_OFFSET(cui_bridge_time_field_change_v1, version, 4);
TF_OFFSET(cui_bridge_time_field_change_v1, frame_status, 8);
TF_OFFSET(cui_bridge_time_field_change_v1, reserved, 12);
TF_OFFSET(cui_bridge_time_field_change_v1, node, 16);
TF_OFFSET(cui_bridge_time_field_change_v1, previous, 32);
TF_OFFSET(cui_bridge_time_field_change_v1, value, 44);
TF_OFFSET(cui_bridge_time_field_change_v1, validation, 56);
TF_OFFSET(cui_bridge_time_field_change_v1, rejected_count, 60);
TF_OFFSET(cui_bridge_time, hour, 0);
TF_OFFSET(cui_bridge_time, minute, 4);
TF_OFFSET(cui_bridge_time, second, 8);
TF_OFFSET(cui_bridge_node, slot, 0);
TF_OFFSET(cui_bridge_node, generation, 4);
TF_OFFSET(cui_bridge_time_token_v1, length, 0);
TF_OFFSET(cui_bridge_time_token_v1, bytes, 4);
TF_OFFSET(cui_bridge_time_name_v1, length, 0);
TF_OFFSET(cui_bridge_time_name_v1, bytes, 4);
_Static_assert(sizeof(cui_bridge_time) == 12 && sizeof(cui_bridge_node) == 8,
               "Time scalar records");
_Static_assert(sizeof(cui_bridge_time_token_v1) == 20 && sizeof(cui_bridge_time_name_v1) == 68,
               "Time strings");
#undef TF_OFFSET
static int tf_symbol(void *target, size_t size, const char *name) {
#if defined(_WIN32)
    HMODULE module = GetModuleHandleA("cui_bridge.dll");
    FARPROC symbol = module ? GetProcAddress(module, name) : NULL;
#else
    void *symbol = dlsym(RTLD_DEFAULT, name);
#endif
    if (!symbol || size != sizeof(symbol))
        return 0;
    memcpy(target, &symbol, size);
    return 1;
}
static int tf_resolve(localized_time_api *a) {
    memset(a, 0, sizeof(*a));
#define TF_SYMBOL(field, suffix)                                                                   \
    if (!tf_symbol(&a->field, sizeof a->field, "cui_bridge_time_field_" suffix))                   \
    return 0
    TF_SYMBOL(versions, "versions");
    TF_SYMBOL(config_size, "config_size_v1");
    TF_SYMBOL(result_size, "result_size_v1");
    TF_SYMBOL(change_size, "change_size_v1");
    TF_SYMBOL(enable, "enable_v1");
    TF_SYMBOL(configure, "configure_v1");
    TF_SYMBOL(config, "get_config_v1");
    TF_SYMBOL(result, "get_result_v1");
    TF_SYMBOL(clear, "clear_validation_v1");
    TF_SYMBOL(poll, "poll_change_v1");
#undef TF_SYMBOL
    return (a->versions() & 1u) && a->config_size() == 616 && a->result_size() == 80 &&
           a->change_size() == 64;
}
static uint32_t tf_load32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void tf_word_in(void *record, const unsigned char *wire, size_t at) {
    uint32_t word = tf_load32(wire + at);
    memcpy((unsigned char *)record + at, &word, sizeof word);
}
static void tf_word_out(unsigned char *wire, const void *record, size_t at) {
    uint32_t word;
    memcpy(&word, (const unsigned char *)record + at, sizeof word);
    for (unsigned i = 0; i < 4; ++i)
        wire[at + i] = (unsigned char)(word >> (8 * i));
}
static void tf_big_in(void *record, const unsigned char *wire, size_t at) {
    uint64_t value = (uint64_t)tf_load32(wire + at) | (uint64_t)tf_load32(wire + at + 4) << 32;
    memcpy((unsigned char *)record + at, &value, sizeof value);
}
static void tf_big_out(unsigned char *wire, const void *record, size_t at) {
    uint64_t value;
    memcpy(&value, (const unsigned char *)record + at, sizeof value);
    for (unsigned i = 0; i < 8; ++i)
        wire[at + i] = (unsigned char)(value >> (8 * i));
}
static void tf_config_in(cui_bridge_time_field_config_v1 *out, const unsigned char *wire) {
    memcpy(out, wire, sizeof(*out));
    for (size_t at = 0; at < 56; at += 4)
        tf_word_in(out, wire, at);
    tf_big_in(out, wire, 56);
    for (size_t i = 0; i < 14; ++i)
        tf_word_in(out, wire, 64 + 20 * i);
    for (size_t i = 0; i < 4; ++i)
        tf_word_in(out, wire, 344 + 68 * i);
}
static napi_value tf_copy(napi_env env, const void *record, size_t size) {
    unsigned char wire[616];
    memcpy(wire, record, size);
    if (size == 616) {
        for (size_t at = 0; at < 56; at += 4)
            tf_word_out(wire, record, at);
        tf_big_out(wire, record, 56);
        for (size_t i = 0; i < 14; ++i)
            tf_word_out(wire, record, 64 + 20 * i);
        for (size_t i = 0; i < 4; ++i)
            tf_word_out(wire, record, 344 + 68 * i);
    } else {
        for (size_t at = 0; at < size; at += 4)
            if (at != 24 && at != 28)
                tf_word_out(wire, record, at);
        tf_big_out(wire, record, 24);
    }
    napi_value result;
    return napi_create_buffer_copy(env, size, wire, NULL, &result) == napi_ok ? result : NULL;
}
/* op0 support,1 enable,2 configure,3 copied config,4 copied latest result,
 * 5 clear transient validation,6 poll copied applied change. */
static napi_value time_field_localized(napi_env env, napi_callback_info info) {
    napi_value v[5], result;
    js_app *app;
    js_node *node;
    uint32_t op;
    localized_time_api api;
    if (!args(env, info, 5, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &op))
        return NULL;
    int supported = tf_resolve(&api);
    if (op == 0)
        return napi_get_boolean(env, supported != 0, &result) == napi_ok ? result : NULL;
    if (!supported)
        return fail(env, "localized TimeField V1 symbols/layout unavailable");
    if (op == 1) {
        uint32_t capacity;
        if (!uint_arg(env, v[3], &capacity))
            return NULL;
        cui_bridge_status code = api.enable(app->bridge, capacity);
        return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
    }
    if (op == 6) {
        cui_bridge_time_field_change_v1 change = {.struct_size = sizeof(change), .version = 1};
        cui_bridge_status code = api.poll(app->bridge, &change);
        if (code == CUI_BRIDGE_EMPTY)
            return undefined(env);
        return code == CUI_BRIDGE_OK ? tf_copy(env, &change, sizeof(change)) : status(env, code);
    }
    if (!node_arg(env, v[2], app, &node))
        return NULL;
    if (op == 2) {
        void *data;
        size_t size;
        bool buffer = false;
        if (napi_is_buffer(env, v[3], &buffer) != napi_ok || !buffer ||
            napi_get_buffer_info(env, v[3], &data, &size) != napi_ok || size != 616)
            return fail(env, "expected exact 616-byte localized TimeField configuration");
        unsigned char wire[616];
        memcpy(wire, data, sizeof wire);
        cui_bridge_time_field_config_v1 config;
        tf_config_in(&config, wire);
        cui_bridge_status code = api.configure(app->bridge, node->node, &config);
        return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
    }
    if (op == 3) {
        cui_bridge_time_field_config_v1 config = {.struct_size = sizeof(config), .version = 1};
        cui_bridge_status code = api.config(app->bridge, node->node, &config);
        return code == CUI_BRIDGE_OK ? tf_copy(env, &config, sizeof(config)) : status(env, code);
    }
    if (op == 4) {
        cui_bridge_time_field_result_v1 record = {.struct_size = sizeof(record), .version = 1};
        cui_bridge_status code = api.result(app->bridge, node->node, &record);
        return code == CUI_BRIDGE_OK ? tf_copy(env, &record, sizeof(record)) : status(env, code);
    }
    if (op == 5) {
        cui_bridge_status code = api.clear(app->bridge, node->node);
        return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
    }
    return fail(env, "unknown localized TimeField operation");
}
