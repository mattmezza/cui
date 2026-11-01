/* SPDX-License-Identifier: MIT */
/* Private N-API conversion. Arrays contain owned scalar copies, never C views. */
typedef struct {
    uint32_t (*versions)(void);
    uint32_t (*config_size)(void);
    uint32_t (*value_size)(void);
    uint32_t (*receipt_size)(void);
    uint32_t (*request_size)(void);
    uint32_t (*presentation_size)(void);
    cui_bridge_status (*enable)(cui_bridge_app *, uint32_t,
                                const cui_bridge_slider_queue_options_v1 *);
    cui_bridge_status (*create)(cui_bridge_app *, cui_bridge_node, const char *, uint32_t,
                                const cui_bridge_slider_config_v1 *, uint32_t,
                                const cui_bridge_slider_value_v1 *, cui_bridge_node *);
    cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                   const cui_bridge_slider_config_v1 *, uint32_t,
                                   const cui_bridge_slider_value_v1 *);
    cui_bridge_status (*get_config)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                    cui_bridge_slider_config_v1 *);
    cui_bridge_status (*set_value)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                   const cui_bridge_slider_value_v1 *);
    cui_bridge_status (*get_value)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                   cui_bridge_slider_value_v1 *);
    cui_bridge_status (*poll)(cui_bridge_app *, uint32_t, cui_bridge_slider_request_v1 *);
    cui_bridge_status (*accept)(cui_bridge_app *, uint32_t, const cui_bridge_slider_receipt_v1 *,
                                uint32_t, const cui_bridge_slider_value_v1 *);
    cui_bridge_status (*presentation)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                      cui_bridge_slider_presentation_v1 *);
    cui_bridge_status (*bounds)(cui_bridge_app *, cui_bridge_node, uint32_t, cui_bridge_rect *);
} slider_api;
static int slider_resolve(slider_api *api) {
    memset(api, 0, sizeof(*api));
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol = module ? GetProcAddress(module, "cui_bridge_slider_model_versions") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_model_versions");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->versions, &symbol, sizeof api->versions);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol = module ? GetProcAddress(module, "cui_bridge_slider_config_v1_size") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_config_v1_size");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->config_size, &symbol, sizeof api->config_size);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol = module ? GetProcAddress(module, "cui_bridge_slider_value_v1_size") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_value_v1_size");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->value_size, &symbol, sizeof api->value_size);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_receipt_v1_size") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_receipt_v1_size");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->receipt_size, &symbol, sizeof api->receipt_size);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_request_v1_size") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_request_v1_size");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->request_size, &symbol, sizeof api->request_size);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_presentation_v1_size") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_presentation_v1_size");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->presentation_size, &symbol, sizeof api->presentation_size);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_queue_enable_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_queue_enable_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->enable, &symbol, sizeof api->enable);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_create_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_model_create_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->create, &symbol, sizeof api->create);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_configure_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_model_configure_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->configure, &symbol, sizeof api->configure);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_get_config_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_model_get_config_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->get_config, &symbol, sizeof api->get_config);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_set_value_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_model_set_value_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->set_value, &symbol, sizeof api->set_value);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_get_value_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_model_get_value_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->get_value, &symbol, sizeof api->get_value);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_poll_slider_request_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_poll_slider_request_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->poll, &symbol, sizeof api->poll);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_accept_slider_request_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_accept_slider_request_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->accept, &symbol, sizeof api->accept);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_get_presentation_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_model_get_presentation_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->presentation, &symbol, sizeof api->presentation);
    }
    {
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("cui_bridge.dll");
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_part_bounds_v1") : NULL;
#else
        void *symbol = dlsym(RTLD_DEFAULT, "cui_bridge_slider_model_part_bounds_v1");
#endif
        if (!symbol)
            return 0;
        memcpy(&api->bounds, &symbol, sizeof api->bounds);
    }
    return (api->versions() & 1u) && api->config_size() == sizeof(cui_bridge_slider_config_v1) &&
           api->value_size() == sizeof(cui_bridge_slider_value_v1) &&
           api->receipt_size() == sizeof(cui_bridge_slider_receipt_v1) &&
           api->request_size() == sizeof(cui_bridge_slider_request_v1) &&
           api->presentation_size() == sizeof(cui_bridge_slider_presentation_v1);
}
static int sm_at(napi_env env, napi_value array, uint32_t index, napi_value *out) {
    bool is_array = false;
    if (napi_is_array(env, array, &is_array) != napi_ok || !is_array ||
        napi_get_element(env, array, index, out) != napi_ok) {
        fail(env, "expected Slider scalar array");
        return 0;
    }
    return 1;
}
static int sm_u(napi_env env, napi_value array, uint32_t index, uint32_t *out) {
    napi_value v;
    return sm_at(env, array, index, &v) && uint_arg(env, v, out);
}
static int sm_d(napi_env env, napi_value array, uint32_t index, double *out) {
    napi_value v;
    if (!sm_at(env, array, index, &v) || napi_get_value_double(env, v, out) != napi_ok ||
        !isfinite(*out)) {
        fail(env, "expected finite Slider double");
        return 0;
    }
    return 1;
}
static int sm_b(napi_env env, napi_value array, uint32_t index, uint64_t *out) {
    napi_value v;
    bool lossless = false;
    if (!sm_at(env, array, index, &v) ||
        napi_get_value_bigint_uint64(env, v, out, &lossless) != napi_ok || !lossless) {
        fail(env, "expected uint64 Slider BigInt");
        return 0;
    }
    return 1;
}
static int sm_value(napi_env env, napi_value array, cui_bridge_slider_value_v1 *out) {
    memset(out, 0, sizeof(*out));
    return sm_u(env, array, 0, &out->kind) && sm_d(env, array, 1, &out->start) &&
           sm_d(env, array, 2, &out->end);
}
static int sm_config(napi_env env, napi_value a, cui_bridge_slider_config_v1 *out) {
    double rem, length;
    memset(out, 0, sizeof(*out));
    out->struct_size = sizeof(*out);
    out->schema_version = 1;
    if (!(sm_u(env, a, 0, &out->flags) && sm_u(env, a, 1, &out->orientation) &&
          sm_b(env, a, 2, &out->source_generation) && sm_d(env, a, 3, &out->domain.minimum) &&
          sm_d(env, a, 4, &out->domain.maximum) && sm_d(env, a, 5, &out->domain.step) &&
          sm_u(env, a, 6, &out->domain.scale) && sm_d(env, a, 7, &rem) &&
          sm_d(env, a, 8, &length) && sm_u(env, a, 9, &out->layout)))
        return 0;
    if (fabs(rem) > FLT_MAX || fabs(length) > FLT_MAX) {
        fail(env, "Slider geometry exceeds float32");
        return 0;
    }
    out->rem = (float)rem;
    out->length = (float)length;
    return 1;
}
static int sm_receipt(napi_env env, napi_value a, cui_bridge_slider_receipt_v1 *out) {
    memset(out, 0, sizeof(*out));
    out->struct_size = sizeof(*out);
    out->schema_version = 1;
    return sm_b(env, a, 0, &out->app_epoch) && sm_u(env, a, 1, &out->node.slot) &&
           sm_u(env, a, 2, &out->node.generation) && sm_b(env, a, 3, &out->source_generation) &&
           sm_b(env, a, 4, &out->configuration_revision) &&
           sm_b(env, a, 5, &out->submission_frame) && sm_b(env, a, 6, &out->request_sequence);
}
static napi_value sm_array(napi_env env) {
    napi_value a;
    return napi_create_array(env, &a) == napi_ok ? a : NULL;
}
static int sm_put(napi_env env, napi_value a, uint32_t i, napi_value v) {
    return v && napi_set_element(env, a, i, v) == napi_ok;
}
static int sm_num(napi_env env, napi_value a, uint32_t i, double d) {
    napi_value v;
    return napi_create_double(env, d, &v) == napi_ok && sm_put(env, a, i, v);
}
static int sm_big(napi_env env, napi_value a, uint32_t i, uint64_t d) {
    napi_value v;
    return napi_create_bigint_uint64(env, d, &v) == napi_ok && sm_put(env, a, i, v);
}
static napi_value sm_value_out(napi_env env, const cui_bridge_slider_value_v1 *v) {
    if (v->reserved || v->kind > 1 || (v->kind == 0 && v->start != 0))
        return fail(env, "invalid Slider value layout");
    napi_value a = sm_array(env);
    if (!a || !sm_num(env, a, 0, v->kind) || !sm_num(env, a, 1, v->start) ||
        !sm_num(env, a, 2, v->end))
        return NULL;
    return a;
}
static napi_value sm_domain_out(napi_env env, const cui_bridge_slider_domain_v1 *v) {
    napi_value a = sm_array(env);
    if (!a || !sm_num(env, a, 0, v->minimum) || !sm_num(env, a, 1, v->maximum) ||
        !sm_num(env, a, 2, v->step) || !sm_num(env, a, 3, v->scale))
        return NULL;
    return a;
}
static napi_value sm_config_out(napi_env env, const cui_bridge_slider_config_v1 *v) {
    if (v->layout == CUI_BRIDGE_SLIDER_PARENT && v->length != 0)
        return fail(env, "invalid Slider inactive parent length");
    napi_value a = sm_array(env);
    if (!a || !sm_num(env, a, 0, v->flags) || !sm_num(env, a, 1, v->orientation) ||
        !sm_big(env, a, 2, v->source_generation) || !sm_num(env, a, 3, v->domain.minimum) ||
        !sm_num(env, a, 4, v->domain.maximum) || !sm_num(env, a, 5, v->domain.step) ||
        !sm_num(env, a, 6, v->domain.scale) || !sm_num(env, a, 7, v->rem) ||
        !sm_num(env, a, 8, v->length) || !sm_num(env, a, 9, v->layout))
        return NULL;
    return a;
}
static napi_value sm_request_out(napi_env env, const cui_bridge_slider_request_v1 *v) {
    napi_value a = sm_array(env), r = sm_array(env), input = sm_array(env);
    const cui_bridge_slider_receipt_v1 *p = &v->receipt;
    const cui_bridge_slider_input_v1 *q = &v->input;
    if (!a || !r || !input || v->struct_size != sizeof(*v) || v->schema_version != 1 ||
        p->struct_size != sizeof(*p) || p->schema_version != 1 || v->current.reserved ||
        v->requested.reserved || v->domain.reserved || v->gesture_domain.reserved)
        return fail(env, "invalid Slider request layout");
    if (!sm_num(env, a, 0, v->phase))
        return NULL;
    if (!sm_num(env, a, 1, v->reason))
        return NULL;
    if (!sm_num(env, a, 2, v->thumb))
        return NULL;
    if (!sm_num(env, a, 3, v->event_kind))
        return NULL;
    if (!sm_num(env, a, 4, v->event_source))
        return NULL;
    if (!sm_num(env, a, 5, v->event_flags))
        return NULL;
    if (!sm_big(env, r, 0, p->app_epoch))
        return NULL;
    if (!sm_num(env, r, 1, p->node.slot))
        return NULL;
    if (!sm_num(env, r, 2, p->node.generation))
        return NULL;
    if (!sm_big(env, r, 3, p->source_generation))
        return NULL;
    if (!sm_big(env, r, 4, p->configuration_revision))
        return NULL;
    if (!sm_big(env, r, 5, p->submission_frame))
        return NULL;
    if (!sm_big(env, r, 6, p->request_sequence))
        return NULL;
    if (!sm_num(env, input, 0, q->type))
        return NULL;
    if (!sm_num(env, input, 1, q->flags))
        return NULL;
    if (!sm_num(env, input, 2, q->key))
        return NULL;
    if (!sm_num(env, input, 3, q->modifiers))
        return NULL;
    if (!sm_num(env, input, 4, q->x))
        return NULL;
    if (!sm_num(env, input, 5, q->y))
        return NULL;
    if (!sm_num(env, input, 6, q->repeat))
        return NULL;
    if (!sm_num(env, input, 7, q->pointer_type))
        return NULL;
    if (!sm_num(env, input, 8, q->pointer_button))
        return NULL;
    if (!sm_num(env, input, 9, q->accessibility_action))
        return NULL;
    if (!sm_big(env, input, 10, q->pointer_id))
        return NULL;
    if (!sm_big(env, input, 11, q->pointer_device))
        return NULL;
    if (!sm_big(env, input, 12, q->accessibility_target))
        return NULL;
    if (!sm_num(env, input, 13, q->accessibility_value))
        return NULL;
    if (!sm_put(env, a, 6, r) || !sm_big(env, a, 7, v->event_target) ||
        !sm_big(env, a, 8, v->event_frame) || !sm_big(env, a, 9, v->event_index) ||
        !sm_put(env, a, 10, sm_value_out(env, &v->current)) ||
        !sm_put(env, a, 11, sm_value_out(env, &v->requested)) ||
        !sm_put(env, a, 12, sm_domain_out(env, &v->domain)) ||
        !sm_put(env, a, 13, sm_domain_out(env, &v->gesture_domain)) || !sm_put(env, a, 14, input))
        return NULL;
    return a;
}
/* Internal operation ABI: app, operation, node-or-null, first, second. */
static napi_value slider_model(napi_env env, napi_callback_info info) {
    napi_value v[5], result, x, y;
    js_app *app;
    js_node *node = NULL;
    uint32_t op;
    slider_api api;
    if (!args(env, info, 5, v) || !app_arg(env, v[0], &app) || !uint_arg(env, v[1], &op))
        return NULL;
    int supported = slider_resolve(&api);
    if (op == 0) {
        return napi_get_boolean(env, supported != 0, &result) == napi_ok ? result : NULL;
    }
    if (!supported)
        return fail(env, "controlled Slider V1 symbols/layout unavailable");
    if (op == 1) {
        uint32_t capacity;
        if (!uint_arg(env, v[3], &capacity))
            return NULL;
        cui_bridge_slider_queue_options_v1 q = {sizeof(q), 1, capacity, 0};
        cui_bridge_status code = api.enable(app->bridge, sizeof(q), &q);
        return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
    }
    if (op == 7) {
        cui_bridge_slider_request_v1 request = {0};
        cui_bridge_status code = api.poll(app->bridge, sizeof(request), &request);
        if (code == CUI_BRIDGE_EMPTY)
            return undefined(env);
        return code == CUI_BRIDGE_OK ? sm_request_out(env, &request) : status(env, code);
    }
    if (op == 8) {
        cui_bridge_slider_receipt_v1 receipt;
        cui_bridge_slider_value_v1 value;
        if (!sm_receipt(env, v[3], &receipt) || !sm_value(env, v[4], &value))
            return NULL;
        cui_bridge_status code =
            api.accept(app->bridge, sizeof(receipt), &receipt, sizeof(value), &value);
        return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
    }
    if (!node_arg(env, v[2], app, &node))
        return NULL;
    if (op == 2 || op == 3) {
        cui_bridge_slider_config_v1 config;
        cui_bridge_slider_value_v1 value;
        if (op == 2) {
            if (!sm_at(env, v[4], 0, &x) || !sm_at(env, v[4], 1, &y))
                return fail(env, "missing Slider config/value");
        } else {
            x = v[3];
            y = v[4];
        }
        if (!sm_config(env, x, &config) || !sm_value(env, y, &value))
            return NULL;
        if (op == 3) {
            cui_bridge_status code = api.configure(app->bridge, node->node, sizeof(config), &config,
                                                   sizeof(value), &value);
            return code == CUI_BRIDGE_OK ? undefined(env) : status(env, code);
        }
        char *key = NULL;
        if (!string_arg(env, v[3], &key))
            return NULL;
        js_node *created = calloc(1, sizeof(*created));
        if (!created) {
            free(key);
            return fail(env, "allocation failed");
        }
        created->owner = app;
        cui_bridge_status code = api.create(app->bridge, node->node, key, sizeof(config), &config,
                                            sizeof(value), &value, &created->node);
        free(key);
        if (code != CUI_BRIDGE_OK) {
            free(created);
            return status(env, code);
        }
        if (napi_create_reference(env, v[0], 1, &created->app_ref) != napi_ok) {
            (void)cui_bridge_node_remove(app->bridge, created->node);
            free(created);
            return fail(env, "failed to retain Slider app");
        }
        if (napi_create_external(env, created, node_final, NULL, &result) != napi_ok) {
            (void)cui_bridge_node_remove(app->bridge, created->node);
            node_final(env, created, NULL);
            return NULL;
        }
        if (napi_type_tag_object(env, result, &node_tag) != napi_ok) {
            (void)cui_bridge_node_remove(app->bridge, created->node);
            return fail(env, "failed to tag Slider node");
        }
        return result;
    }
    if (op == 4) {
        cui_bridge_slider_config_v1 config = {0};
        cui_bridge_status code = api.get_config(app->bridge, node->node, sizeof(config), &config);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (config.struct_size != sizeof(config) || config.schema_version != 1 || config.reserved ||
            config.domain.reserved ||
            (config.layout == CUI_BRIDGE_SLIDER_PARENT && config.length != 0))
            return fail(env, "invalid Slider config layout");
        return sm_config_out(env, &config);
    }
    if (op == 5 || op == 6) {
        cui_bridge_slider_value_v1 value = {0};
        cui_bridge_status code;
        if (op == 5) {
            if (!sm_value(env, v[3], &value))
                return NULL;
            code = api.set_value(app->bridge, node->node, sizeof(value), &value);
        } else
            code = api.get_value(app->bridge, node->node, sizeof(value), &value);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        return op == 5 ? undefined(env) : sm_value_out(env, &value);
    }
    if (op == 9) {
        cui_bridge_slider_presentation_v1 p = {0};
        cui_bridge_status code = api.presentation(app->bridge, node->node, sizeof(p), &p);
        if (code == CUI_BRIDGE_EMPTY)
            return undefined(env);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        if (p.struct_size != sizeof(p) || p.schema_version != 1 || p.reserved)
            return fail(env, "invalid Slider presentation layout");
        result = sm_array(env);
        return result && sm_num(env, result, 0, p.render_status) &&
                       sm_num(env, result, 1, p.core_time) &&
                       sm_num(env, result, 2, p.next_wake_seconds)
                   ? result
                   : NULL;
    }
    if (op == 10) {
        uint32_t part;
        cui_bridge_rect r;
        if (!uint_arg(env, v[3], &part))
            return NULL;
        cui_bridge_status code = api.bounds(app->bridge, node->node, part, &r);
        if (code == CUI_BRIDGE_EMPTY)
            return undefined(env);
        if (code != CUI_BRIDGE_OK)
            return status(env, code);
        result = sm_array(env);
        return result && sm_num(env, result, 0, r.x) && sm_num(env, result, 1, r.y) &&
                       sm_num(env, result, 2, r.width) && sm_num(env, result, 3, r.height)
                   ? result
                   : NULL;
    }
    return fail(env, "unknown Slider operation");
}
