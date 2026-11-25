/* SPDX-License-Identifier: MIT */
#include "icon_badge_ffi.h"
#include <string.h>
#if defined(_WIN32)
#include <windows.h>
typedef FARPROC go_presentation_symbol;
#define GO_PRESENTATION_SYMBOL(library, name) GetProcAddress((HMODULE)(library), name)
#else
#include <dlfcn.h>
typedef void *go_presentation_symbol;
#define GO_PRESENTATION_SYMBOL(library, name) dlsym(library, name)
#endif

static struct {
    uint32_t (*versions)(void);
    uint32_t (*record_size)(uint32_t);
#define GO_PRESENTATION_MEMBER(name, params, args) cui_bridge_status(*name) params;
    CUI_GO_ICON_PRESENTATION_API(GO_PRESENTATION_MEMBER)
#undef GO_PRESENTATION_MEMBER
} icon_api;
static struct {
    uint32_t (*versions)(void);
    uint32_t (*record_size)(uint32_t);
#define GO_BADGE_MEMBER(name, params, args) cui_bridge_status(*name) params;
    CUI_GO_BADGE_API(GO_BADGE_MEMBER)
#undef GO_BADGE_MEMBER
} badge_api;

static int icon_supported, badge_supported;
_Static_assert(sizeof(cui_bridge_icon_style_v1) == 128 &&
                   sizeof(cui_bridge_icon_constraints_v1) == 32 &&
                   sizeof(cui_bridge_icon_options_v1) == 256 &&
                   sizeof(cui_bridge_icon_result_v1) == 40,
               "retained Icon V1 record sizes");
_Static_assert(sizeof(cui_bridge_badge_config_v1) == 64 && sizeof(cui_bridge_badge_result_v1) == 40,
               "configured Badge V1 record sizes");

#if defined(CUI_GO_STATIC)
#define GO_PRESENTATION_STATIC(target, name) target = name;
#else
#define GO_PRESENTATION_STATIC(target, name) ((void)0);
#endif

#define GO_PRESENTATION_LOAD(target, name)                                                         \
    if (library) {                                                                                 \
        go_presentation_symbol symbol = GO_PRESENTATION_SYMBOL(library, #name);                    \
        if (!symbol || sizeof symbol != sizeof target)                                             \
            return 0;                                                                              \
        memcpy(&target, &symbol, sizeof symbol);                                                   \
    } else {                                                                                       \
        GO_PRESENTATION_STATIC(target, name)                                                       \
    }                                                                                              \
    if (!target)                                                                                   \
        return 0;

static int go_icon_presentation_load(void *library) {
    GO_PRESENTATION_LOAD(icon_api.record_size, cui_bridge_icon_presentation_record_size_v1)
    GO_PRESENTATION_LOAD(icon_api.versions, cui_bridge_icon_presentation_versions)
#define GO_PRESENTATION_LOAD_OP(name, params, args)                                                \
    GO_PRESENTATION_LOAD(icon_api.name, cui_bridge_icon_##name)
    CUI_GO_ICON_PRESENTATION_API(GO_PRESENTATION_LOAD_OP)
#undef GO_PRESENTATION_LOAD_OP
    return (icon_api.versions() & 1u) &&
           icon_api.record_size(CUI_BRIDGE_ICON_STYLE_RECORD_V1) ==
               sizeof(cui_bridge_icon_style_v1) &&
           icon_api.record_size(CUI_BRIDGE_ICON_CONSTRAINTS_RECORD_V1) ==
               sizeof(cui_bridge_icon_constraints_v1) &&
           icon_api.record_size(CUI_BRIDGE_ICON_OPTIONS_RECORD_V1) ==
               sizeof(cui_bridge_icon_options_v1) &&
           icon_api.record_size(CUI_BRIDGE_ICON_RESULT_RECORD_V1) ==
               sizeof(cui_bridge_icon_result_v1);
}

static int go_badge_load(void *library) {
    GO_PRESENTATION_LOAD(badge_api.versions, cui_bridge_badge_versions)
    GO_PRESENTATION_LOAD(badge_api.record_size, cui_bridge_badge_record_size_v1)
#define GO_BADGE_LOAD_OP(name, params, args)                                                       \
    GO_PRESENTATION_LOAD(badge_api.name, cui_bridge_badge_##name)
    CUI_GO_BADGE_API(GO_BADGE_LOAD_OP)
#undef GO_BADGE_LOAD_OP
    return (badge_api.versions() & 1u) &&
           badge_api.record_size(CUI_BRIDGE_BADGE_CONFIG_RECORD_V1) ==
               sizeof(cui_bridge_badge_config_v1) &&
           badge_api.record_size(CUI_BRIDGE_BADGE_RESULT_RECORD_V1) ==
               sizeof(cui_bridge_badge_result_v1) &&
           icon_api.record_size &&
           icon_api.record_size(CUI_BRIDGE_ICON_OPTIONS_RECORD_V1) ==
               sizeof(cui_bridge_icon_options_v1);
}

void cui_go_icon_badge_init(void *library) {
    icon_supported = go_icon_presentation_load(library);
    badge_supported = go_badge_load(library);
}

int cui_go_icon_presentation_supported(void) {
    return icon_supported;
}

int cui_go_badge_supported(void) {
    return badge_supported;
}

#define GO_ICON_CALL(name, params, args)                                                           \
    cui_bridge_status cui_go_icon_##name params {                                                  \
        return icon_supported ? icon_api.name args : CUI_BRIDGE_BACKEND_UNAVAILABLE;               \
    }
CUI_GO_ICON_PRESENTATION_API(GO_ICON_CALL)
#undef GO_ICON_CALL
#define GO_BADGE_CALL(name, params, args)                                                          \
    cui_bridge_status cui_go_badge_##name params {                                                 \
        return badge_supported ? badge_api.name args : CUI_BRIDGE_BACKEND_UNAVAILABLE;             \
    }
CUI_GO_BADGE_API(GO_BADGE_CALL)
#undef GO_BADGE_CALL
#undef GO_PRESENTATION_LOAD
#undef GO_PRESENTATION_STATIC
#undef GO_PRESENTATION_SYMBOL
