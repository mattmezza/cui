/* SPDX-License-Identifier: MIT */
/* Private optional group: never part of the mandatory default loader. */
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_versions
#endif
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_config_size_v1
#endif
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_result_size_v1
#endif
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_change_size_v1
#endif
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_enable_v1
#endif
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_configure_v1
#endif
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_get_config_v1
#endif
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_get_result_v1
#endif
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_clear_validation_v1
#endif
#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_time_field_poll_change_v1
#endif
typedef struct {
    uint32_t (*versions)(void);
    uint32_t (*config_size_v1)(void);
    uint32_t (*result_size_v1)(void);
    uint32_t (*change_size_v1)(void);
    cui_bridge_status (*enable_v1)(cui_bridge_app *, uint32_t);
    cui_bridge_status (*configure_v1)(cui_bridge_app *, cui_bridge_node,
                                      const cui_bridge_time_field_config_v1 *);
    cui_bridge_status (*get_config_v1)(cui_bridge_app *, cui_bridge_node,
                                       cui_bridge_time_field_config_v1 *);
    cui_bridge_status (*get_result_v1)(cui_bridge_app *, cui_bridge_node,
                                       cui_bridge_time_field_result_v1 *);
    cui_bridge_status (*clear_validation_v1)(cui_bridge_app *, cui_bridge_node);
    cui_bridge_status (*poll_change_v1)(cui_bridge_app *, cui_bridge_time_field_change_v1 *);
} go_time_api;
static int go_time_resolve(go_time_api *a) {
    memset(a, 0, sizeof(*a));
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->versions = cui_bridge_time_field_versions;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_versions");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_versions");
#endif
        if (p)
            memcpy(&a->versions, &p, sizeof a->versions);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->config_size_v1 = cui_bridge_time_field_config_size_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_config_size_v1");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_config_size_v1");
#endif
        if (p)
            memcpy(&a->config_size_v1, &p, sizeof a->config_size_v1);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->result_size_v1 = cui_bridge_time_field_result_size_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_result_size_v1");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_result_size_v1");
#endif
        if (p)
            memcpy(&a->result_size_v1, &p, sizeof a->result_size_v1);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->change_size_v1 = cui_bridge_time_field_change_size_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_change_size_v1");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_change_size_v1");
#endif
        if (p)
            memcpy(&a->change_size_v1, &p, sizeof a->change_size_v1);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->enable_v1 = cui_bridge_time_field_enable_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_enable_v1");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_enable_v1");
#endif
        if (p)
            memcpy(&a->enable_v1, &p, sizeof a->enable_v1);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->configure_v1 = cui_bridge_time_field_configure_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_configure_v1");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_configure_v1");
#endif
        if (p)
            memcpy(&a->configure_v1, &p, sizeof a->configure_v1);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->get_config_v1 = cui_bridge_time_field_get_config_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_get_config_v1");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_get_config_v1");
#endif
        if (p)
            memcpy(&a->get_config_v1, &p, sizeof a->get_config_v1);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->get_result_v1 = cui_bridge_time_field_get_result_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_get_result_v1");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_get_result_v1");
#endif
        if (p)
            memcpy(&a->get_result_v1, &p, sizeof a->get_result_v1);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->clear_validation_v1 = cui_bridge_time_field_clear_validation_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p =
            GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_clear_validation_v1");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_clear_validation_v1");
#endif
        if (p)
            memcpy(&a->clear_validation_v1, &p, sizeof a->clear_validation_v1);
    }
#if defined(CUI_GO_STATIC)
    if (!bridge_lib)
        a->poll_change_v1 = cui_bridge_time_field_poll_change_v1;
#endif
    if (bridge_lib) {
#if defined(_WIN32)
        FARPROC p = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_time_field_poll_change_v1");
#else
        void *p = dlsym(bridge_lib, "cui_bridge_time_field_poll_change_v1");
#endif
        if (p)
            memcpy(&a->poll_change_v1, &p, sizeof a->poll_change_v1);
    }
    if (!a->versions || !a->config_size_v1 || !a->result_size_v1 || !a->change_size_v1 ||
        !a->enable_v1 || !a->configure_v1 || !a->get_config_v1 || !a->get_result_v1 ||
        !a->clear_validation_v1 || !a->poll_change_v1)
        return 0;
    return (a->versions() & 1u) && a->config_size_v1() == 616 && a->result_size_v1() == 80 &&
           a->change_size_v1() == 64;
}
static int go_time_supported(void) {
    go_time_api a;
    return go_time_resolve(&a);
}
/* Copies through aligned local records. No Go memory is retained. */
static cui_bridge_status go_time_call(cui_bridge_app *app, cui_bridge_node node, uint32_t op,
                                      uint32_t capacity, unsigned char *bytes) {
    go_time_api a;
    if (!go_time_resolve(&a))
        return CUI_BRIDGE_BACKEND_UNAVAILABLE;
    if (op == 1)
        return a.enable_v1(app, capacity);
    if (op == 5)
        return a.clear_validation_v1(app, node);
    if (op == 2 || op == 3) {
        cui_bridge_time_field_config_v1 r;
        memcpy(&r, bytes, sizeof r);
        cui_bridge_status s =
            op == 2 ? a.configure_v1(app, node, &r) : a.get_config_v1(app, node, &r);
        if (s == CUI_BRIDGE_OK && op == 3) {
            memcpy(bytes, &r, sizeof r);
        }
        return s;
    }
    if (op == 4) {
        cui_bridge_time_field_result_v1 r;
        memcpy(&r, bytes, sizeof r);
        cui_bridge_status s = a.get_result_v1(app, node, &r);
        if (s == CUI_BRIDGE_OK) {
            memcpy(bytes, &r, sizeof r);
        }
        return s;
    }
    if (op == 6) {
        cui_bridge_time_field_change_v1 r;
        memcpy(&r, bytes, sizeof r);
        cui_bridge_status s = a.poll_change_v1(app, &r);
        if (s == CUI_BRIDGE_OK) {
            memcpy(bytes, &r, sizeof r);
        }
        return s;
    }
    return CUI_BRIDGE_INVALID_ARGUMENT;
}
