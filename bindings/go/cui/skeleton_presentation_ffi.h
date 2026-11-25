/* SPDX-License-Identifier: MIT */
/* Optional group so legacy library admission stays unchanged. */
#define GO_SKELETON_API(X)                                                                         \
    X(versions, uint32_t, (void))                                                                  \
    X(config_size_v1, uint32_t, (void))                                                            \
    X(presentation_size_v1, uint32_t, (void))                                                      \
    X(configure_v1, cui_bridge_status,                                                             \
      (cui_bridge_app *, cui_bridge_node, const cui_bridge_skeleton_config_v1 *))                  \
    X(get_config_v1, cui_bridge_status,                                                            \
      (cui_bridge_app *, cui_bridge_node, cui_bridge_skeleton_config_v1 *))                        \
    X(get_presentation_v1, cui_bridge_status,                                                      \
      (cui_bridge_app *, cui_bridge_node, cui_bridge_skeleton_presentation_v1 *))

#if defined(CUI_GO_STATIC)
#pragma weak cui_bridge_skeleton_versions
#pragma weak cui_bridge_skeleton_config_size_v1
#pragma weak cui_bridge_skeleton_presentation_size_v1
#pragma weak cui_bridge_skeleton_configure_v1
#pragma weak cui_bridge_skeleton_get_config_v1
#pragma weak cui_bridge_skeleton_get_presentation_v1
#define GO_SKELETON_STATIC(name) a->name = cui_bridge_skeleton_##name;
#else
#define GO_SKELETON_STATIC(name) ((void)0);
#endif

typedef struct {
#define GO_SKELETON_MEMBER(name, ret, params) ret(*name) params;
    GO_SKELETON_API(GO_SKELETON_MEMBER)
#undef GO_SKELETON_MEMBER
} go_skeleton_api;

_Static_assert(sizeof(cui_bridge_skeleton_config_v1) == 40 &&
                   sizeof(cui_bridge_skeleton_presentation_v1) == 40,
               "Skeleton V1 record sizes");

#if defined(_WIN32)
#define GO_SKELETON_SYMBOL(name) GetProcAddress((HMODULE)bridge_lib, "cui_bridge_skeleton_" #name)
typedef FARPROC go_skeleton_symbol;
#else
#define GO_SKELETON_SYMBOL(name) dlsym(bridge_lib, "cui_bridge_skeleton_" #name)
typedef void *go_skeleton_symbol;
#endif

static int go_skeleton_resolve(go_skeleton_api *a) {
    *a = (go_skeleton_api){0};
#define GO_SKELETON_LOAD(name, ret, params)                                                        \
    if (bridge_lib) {                                                                              \
        go_skeleton_symbol sym = GO_SKELETON_SYMBOL(name);                                         \
        if (sym)                                                                                   \
            memcpy(&a->name, &sym, sizeof a->name);                                                \
    } else {                                                                                       \
        GO_SKELETON_STATIC(name)                                                                   \
    }                                                                                              \
    if (!a->name)                                                                                  \
        return 0;
    GO_SKELETON_API(GO_SKELETON_LOAD)
#undef GO_SKELETON_LOAD
    return (a->versions() & CUI_BRIDGE_SKELETON_V1) &&
           a->config_size_v1() == sizeof(cui_bridge_skeleton_config_v1) &&
           a->presentation_size_v1() == sizeof(cui_bridge_skeleton_presentation_v1);
}

#undef GO_SKELETON_SYMBOL
#undef GO_SKELETON_STATIC
#undef GO_SKELETON_API

static go_skeleton_api skeleton_api;
static int skeleton_supported;

/* The provider is pinned for the process lifetime. bridge_open holds loaderMu. */
static void go_skeleton_initialize(void) {
    skeleton_supported = go_skeleton_resolve(&skeleton_api);
}

static int go_skeleton_supported(void) {
    return skeleton_supported;
}

static cui_bridge_status go_skeleton_configure(cui_bridge_app *app, cui_bridge_node node,
                                               const cui_bridge_skeleton_config_v1 *config) {
    return skeleton_supported ? skeleton_api.configure_v1(app, node, config)
                              : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status go_skeleton_get_config(cui_bridge_app *app, cui_bridge_node node,
                                                cui_bridge_skeleton_config_v1 *config) {
    return skeleton_supported ? skeleton_api.get_config_v1(app, node, config)
                              : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status go_skeleton_get_presentation(cui_bridge_app *app, cui_bridge_node node,
                                                      cui_bridge_skeleton_presentation_v1 *out) {
    return skeleton_supported ? skeleton_api.get_presentation_v1(app, node, out)
                              : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
