/* SPDX-License-Identifier: MIT */
/* Optional group so legacy library admission stays unchanged. */
#define GO_SPINNER_API(X)                                                                          \
    X(versions, uint32_t, (void))                                                                  \
    X(config_size_v1, uint32_t, (void))                                                            \
    X(presentation_size_v1, uint32_t, (void))                                                      \
    X(configure_v1, cui_bridge_status,                                                             \
      (cui_bridge_app *, cui_bridge_node, const cui_bridge_spinner_config_v1 *))                   \
    X(get_config_v1, cui_bridge_status,                                                            \
      (cui_bridge_app *, cui_bridge_node, cui_bridge_spinner_config_v1 *))                         \
    X(get_presentation_v1, cui_bridge_status,                                                      \
      (cui_bridge_app *, cui_bridge_node, cui_bridge_spinner_presentation_v1 *))

#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_spinner_versions
#pragma weak cui_bridge_spinner_config_size_v1
#pragma weak cui_bridge_spinner_presentation_size_v1
#pragma weak cui_bridge_spinner_configure_v1
#pragma weak cui_bridge_spinner_get_config_v1
#pragma weak cui_bridge_spinner_get_presentation_v1
#define GO_SPINNER_STATIC(name) a->name = cui_bridge_spinner_##name;
#else
#define GO_SPINNER_STATIC(name) ((void)0);
#endif

typedef struct {
#define GO_SPINNER_MEMBER(name, ret, params) ret(*name) params;
    GO_SPINNER_API(GO_SPINNER_MEMBER)
#undef GO_SPINNER_MEMBER
} go_spinner_api;

_Static_assert(sizeof(cui_bridge_spinner_config_v1) == 100 &&
                   sizeof(cui_bridge_spinner_presentation_v1) == 40,
               "Spinner V1 record sizes");

#if defined(_WIN32)
#define GO_SPINNER_SYMBOL(name) GetProcAddress((HMODULE)bridge_lib, "cui_bridge_spinner_" #name)
typedef FARPROC go_spinner_symbol;
#else
#define GO_SPINNER_SYMBOL(name) dlsym(bridge_lib, "cui_bridge_spinner_" #name)
typedef void *go_spinner_symbol;
#endif

static int go_spinner_resolve(go_spinner_api *a) {
    *a = (go_spinner_api){0};
#define GO_SPINNER_LOAD(name, ret, params)                                                         \
    if (bridge_lib) {                                                                              \
        go_spinner_symbol sym = GO_SPINNER_SYMBOL(name);                                           \
        if (sym)                                                                                   \
            memcpy(&a->name, &sym, sizeof a->name);                                                \
    } else {                                                                                       \
        GO_SPINNER_STATIC(name)                                                                    \
    }                                                                                              \
    if (!a->name)                                                                                  \
        return 0;
    GO_SPINNER_API(GO_SPINNER_LOAD)
#undef GO_SPINNER_LOAD
    return (a->versions() & CUI_BRIDGE_SPINNER_V1) &&
           a->config_size_v1() == sizeof(cui_bridge_spinner_config_v1) &&
           a->presentation_size_v1() == sizeof(cui_bridge_spinner_presentation_v1);
}

#undef GO_SPINNER_SYMBOL
#undef GO_SPINNER_STATIC
#undef GO_SPINNER_API

static go_spinner_api spinner_api;
static int spinner_supported;

/* The provider is pinned for the process lifetime. bridge_open holds loaderMu. */
static void go_spinner_initialize(void) {
    spinner_supported = go_spinner_resolve(&spinner_api);
}

static int go_spinner_supported(void) {
    return spinner_supported;
}

static cui_bridge_status go_spinner_configure(cui_bridge_app *app, cui_bridge_node node,
                                              const cui_bridge_spinner_config_v1 *config) {
    return spinner_supported ? spinner_api.configure_v1(app, node, config)
                             : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status go_spinner_get_config(cui_bridge_app *app, cui_bridge_node node,
                                               cui_bridge_spinner_config_v1 *config) {
    return spinner_supported ? spinner_api.get_config_v1(app, node, config)
                             : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status go_spinner_get_presentation(cui_bridge_app *app, cui_bridge_node node,
                                                     cui_bridge_spinner_presentation_v1 *out) {
    return spinner_supported ? spinner_api.get_presentation_v1(app, node, out)
                             : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
