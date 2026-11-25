/* SPDX-License-Identifier: MIT */
#include "icon_resources_ffi.h"
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
typedef FARPROC go_icon_symbol;
#define GO_ICON_SYMBOL(library, name) GetProcAddress((HMODULE)(library), name)
#else
#include <dlfcn.h>
typedef void *go_icon_symbol;
#define GO_ICON_SYMBOL(library, name) dlsym(library, name)
#endif

#define GO_ICON_QUERIES(X)                                                                         \
    X(versions, uint32_t, (void))                                                                  \
    X(record_size_v1, uint32_t, (uint32_t))                                                        \
    X(catalog_count_v1, uint32_t, (void))

typedef struct {
#define GO_ICON_MEMBER(name, ret, params) ret(*name) params;
    GO_ICON_QUERIES(GO_ICON_MEMBER)
#define GO_ICON_STATUS_MEMBER(name, params, actions) GO_ICON_MEMBER(name, cui_bridge_status, params)
    CUI_GO_ICON_RESOURCE_API(GO_ICON_STATUS_MEMBER)
#undef GO_ICON_STATUS_MEMBER
#undef GO_ICON_MEMBER
} go_icon_resource_api;

static go_icon_resource_api api;
static int supported;
_Static_assert(sizeof(cui_bridge_icon_limits_v1) == 48 && sizeof(cui_bridge_icon_info_v1) == 80 &&
                   sizeof(cui_bridge_icon_bitmap_v1) == 64,
               "owned Icon V1 record sizes");

#if defined(CUI_GO_STATIC)
/* The bundled source SDK is built alongside this file. Dynamic providers are
 * queried by name so older shared libraries never acquire mandatory imports. */
#define GO_ICON_STATIC(name) api.name = cui_bridge_icon_##name;
#else
#define GO_ICON_STATIC(name) ((void)0);
#endif

void cui_go_icon_resources_init(void *library) {
#define GO_ICON_LOAD(name, ret, params)                                                            \
    if (library) {                                                                                 \
        go_icon_symbol symbol = GO_ICON_SYMBOL(library, "cui_bridge_icon_" #name);                 \
        if (!symbol || sizeof symbol != sizeof api.name)                                           \
            return;                                                                                \
        memcpy(&api.name, &symbol, sizeof symbol);                                                 \
    } else {                                                                                       \
        GO_ICON_STATIC(name)                                                                       \
    }                                                                                              \
    if (!api.name)                                                                                 \
        return;
    GO_ICON_QUERIES(GO_ICON_LOAD)
#define GO_ICON_LOAD_STATUS(name, params, actions) GO_ICON_LOAD(name, cui_bridge_status, params)
    CUI_GO_ICON_RESOURCE_API(GO_ICON_LOAD_STATUS)
#undef GO_ICON_LOAD_STATUS
#undef GO_ICON_LOAD
    supported =
        (api.versions() & 1u) &&
        api.record_size_v1(CUI_BRIDGE_ICON_LIMITS_RECORD_V1) == sizeof(cui_bridge_icon_limits_v1) &&
        api.record_size_v1(CUI_BRIDGE_ICON_INFO_RECORD_V1) == sizeof(cui_bridge_icon_info_v1) &&
        api.record_size_v1(CUI_BRIDGE_ICON_BITMAP_RECORD_V1) == sizeof(cui_bridge_icon_bitmap_v1);
}

int cui_go_icon_resources_supported(void) {
    return supported;
}

#define GO_ICON_CALL(name, params, actions)                                                        \
    cui_bridge_status cui_go_icon_##name params {                                                  \
        return supported ? api.name actions : CUI_BRIDGE_BACKEND_UNAVAILABLE;                      \
    }
CUI_GO_ICON_RESOURCE_API(GO_ICON_CALL)
#undef GO_ICON_CALL

uint32_t cui_go_icon_catalog_count_v1(void) {
    return supported ? api.catalog_count_v1() : 0;
}
#undef GO_ICON_STATIC
#undef GO_ICON_SYMBOL
#undef GO_ICON_QUERIES
