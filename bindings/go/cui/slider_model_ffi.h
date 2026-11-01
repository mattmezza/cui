/* SPDX-License-Identifier: MIT */
/* Private cgo optional API group; included beside the existing explicit loader. */

#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_slider_model_versions
#pragma weak cui_bridge_slider_config_v1_size
#pragma weak cui_bridge_slider_value_v1_size
#pragma weak cui_bridge_slider_receipt_v1_size
#pragma weak cui_bridge_slider_request_v1_size
#pragma weak cui_bridge_slider_presentation_v1_size
#pragma weak cui_bridge_slider_queue_enable_v1
#pragma weak cui_bridge_slider_model_create_v1
#pragma weak cui_bridge_slider_model_configure_v1
#pragma weak cui_bridge_slider_model_get_config_v1
#pragma weak cui_bridge_slider_model_set_value_v1
#pragma weak cui_bridge_slider_model_get_value_v1
#pragma weak cui_bridge_poll_slider_request_v1
#pragma weak cui_bridge_accept_slider_request_v1
#pragma weak cui_bridge_slider_model_get_presentation_v1
#pragma weak cui_bridge_slider_model_part_bounds_v1
#endif
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
} go_slider_api;
static int go_slider_resolve(go_slider_api *api) {
    memset(api, 0, sizeof(*api));
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->versions = cui_bridge_slider_model_versions;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol = module ? GetProcAddress(module, "cui_bridge_slider_model_versions") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_model_versions");
#endif
        if (symbol)
            memcpy(&api->versions, &symbol, sizeof api->versions);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->config_size = cui_bridge_slider_config_v1_size;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol = module ? GetProcAddress(module, "cui_bridge_slider_config_v1_size") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_config_v1_size");
#endif
        if (symbol)
            memcpy(&api->config_size, &symbol, sizeof api->config_size);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->value_size = cui_bridge_slider_value_v1_size;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol = module ? GetProcAddress(module, "cui_bridge_slider_value_v1_size") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_value_v1_size");
#endif
        if (symbol)
            memcpy(&api->value_size, &symbol, sizeof api->value_size);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->receipt_size = cui_bridge_slider_receipt_v1_size;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_receipt_v1_size") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_receipt_v1_size");
#endif
        if (symbol)
            memcpy(&api->receipt_size, &symbol, sizeof api->receipt_size);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->request_size = cui_bridge_slider_request_v1_size;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_request_v1_size") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_request_v1_size");
#endif
        if (symbol)
            memcpy(&api->request_size, &symbol, sizeof api->request_size);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->presentation_size = cui_bridge_slider_presentation_v1_size;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_presentation_v1_size") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_presentation_v1_size");
#endif
        if (symbol)
            memcpy(&api->presentation_size, &symbol, sizeof api->presentation_size);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->enable = cui_bridge_slider_queue_enable_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_queue_enable_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_queue_enable_v1");
#endif
        if (symbol)
            memcpy(&api->enable, &symbol, sizeof api->enable);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->create = cui_bridge_slider_model_create_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_create_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_model_create_v1");
#endif
        if (symbol)
            memcpy(&api->create, &symbol, sizeof api->create);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->configure = cui_bridge_slider_model_configure_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_configure_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_model_configure_v1");
#endif
        if (symbol)
            memcpy(&api->configure, &symbol, sizeof api->configure);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->get_config = cui_bridge_slider_model_get_config_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_get_config_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_model_get_config_v1");
#endif
        if (symbol)
            memcpy(&api->get_config, &symbol, sizeof api->get_config);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->set_value = cui_bridge_slider_model_set_value_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_set_value_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_model_set_value_v1");
#endif
        if (symbol)
            memcpy(&api->set_value, &symbol, sizeof api->set_value);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->get_value = cui_bridge_slider_model_get_value_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_get_value_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_model_get_value_v1");
#endif
        if (symbol)
            memcpy(&api->get_value, &symbol, sizeof api->get_value);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->poll = cui_bridge_poll_slider_request_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_poll_slider_request_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_poll_slider_request_v1");
#endif
        if (symbol)
            memcpy(&api->poll, &symbol, sizeof api->poll);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->accept = cui_bridge_accept_slider_request_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_accept_slider_request_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_accept_slider_request_v1");
#endif
        if (symbol)
            memcpy(&api->accept, &symbol, sizeof api->accept);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->presentation = cui_bridge_slider_model_get_presentation_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_get_presentation_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_model_get_presentation_v1");
#endif
        if (symbol)
            memcpy(&api->presentation, &symbol, sizeof api->presentation);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        api->bounds = cui_bridge_slider_model_part_bounds_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        HMODULE module = (HMODULE)bridge_lib;
        FARPROC symbol =
            module ? GetProcAddress(module, "cui_bridge_slider_model_part_bounds_v1") : NULL;
#else
        void *symbol = dlsym(bridge_lib, "cui_bridge_slider_model_part_bounds_v1");
#endif
        if (symbol)
            memcpy(&api->bounds, &symbol, sizeof api->bounds);
    }
    if (!(api->versions && api->config_size && api->value_size && api->receipt_size &&
          api->request_size && api->presentation_size && api->enable && api->create &&
          api->configure && api->get_config && api->set_value && api->get_value && api->poll &&
          api->accept && api->presentation && api->bounds))
        return 0;
    return (api->versions() & 1u) && api->config_size() == sizeof(cui_bridge_slider_config_v1) &&
           api->value_size() == sizeof(cui_bridge_slider_value_v1) &&
           api->receipt_size() == sizeof(cui_bridge_slider_receipt_v1) &&
           api->request_size() == sizeof(cui_bridge_slider_request_v1) &&
           api->presentation_size() == sizeof(cui_bridge_slider_presentation_v1);
}

static uint32_t go_slider_supported(void) {
    go_slider_api api;
    return go_slider_resolve(&api);
}
static cui_bridge_status go_slider_enable(cui_bridge_app *a, uint32_t capacity) {
    go_slider_api api;
    if (!go_slider_resolve(&api))
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    cui_bridge_slider_queue_options_v1 q = {sizeof(q), 1, capacity, 0};
    return api.enable(a, sizeof(q), &q);
}
static cui_bridge_status go_slider_create(cui_bridge_app *a, cui_bridge_node p, const char *key,
                                          const cui_bridge_slider_config_v1 *c,
                                          const cui_bridge_slider_value_v1 *v,
                                          cui_bridge_node *out) {
    go_slider_api api;
    return go_slider_resolve(&api) ? api.create(a, p, key, sizeof(*c), c, sizeof(*v), v, out)
                                   : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_slider_configure(cui_bridge_app *a, cui_bridge_node n,
                                             const cui_bridge_slider_config_v1 *c,
                                             const cui_bridge_slider_value_v1 *v) {
    go_slider_api api;
    return go_slider_resolve(&api) ? api.configure(a, n, sizeof(*c), c, sizeof(*v), v)
                                   : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_slider_config(cui_bridge_app *a, cui_bridge_node n,
                                          cui_bridge_slider_config_v1 *out) {
    go_slider_api api;
    return go_slider_resolve(&api) ? api.get_config(a, n, sizeof(*out), out)
                                   : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_slider_set(cui_bridge_app *a, cui_bridge_node n,
                                       const cui_bridge_slider_value_v1 *v) {
    go_slider_api api;
    return go_slider_resolve(&api) ? api.set_value(a, n, sizeof(*v), v)
                                   : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_slider_get(cui_bridge_app *a, cui_bridge_node n,
                                       cui_bridge_slider_value_v1 *out) {
    go_slider_api api;
    return go_slider_resolve(&api) ? api.get_value(a, n, sizeof(*out), out)
                                   : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_slider_poll(cui_bridge_app *a, cui_bridge_slider_request_v1 *out) {
    go_slider_api api;
    return go_slider_resolve(&api) ? api.poll(a, sizeof(*out), out)
                                   : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_slider_accept(cui_bridge_app *a, const cui_bridge_slider_receipt_v1 *r,
                                          const cui_bridge_slider_value_v1 *v) {
    go_slider_api api;
    return go_slider_resolve(&api) ? api.accept(a, sizeof(*r), r, sizeof(*v), v)
                                   : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_slider_presentation(cui_bridge_app *a, cui_bridge_node n,
                                                cui_bridge_slider_presentation_v1 *out) {
    go_slider_api api;
    return go_slider_resolve(&api) ? api.presentation(a, n, sizeof(*out), out)
                                   : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_slider_bounds(cui_bridge_app *a, cui_bridge_node n, uint32_t part,
                                          cui_bridge_rect *out) {
    go_slider_api api;
    return go_slider_resolve(&api) ? api.bounds(a, n, part, out) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
