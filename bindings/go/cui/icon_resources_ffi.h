/* SPDX-License-Identifier: MIT */
#ifndef CUI_GO_ICON_RESOURCES_FFI_H
#define CUI_GO_ICON_RESOURCES_FFI_H
#include "cui/cui_bridge.h"

/* Initialization is serialized by the Go loader. The provider is pinned for
 * the lifetime of the process, so calls need no loader lock. */
void cui_go_icon_resources_init(void *library);
int cui_go_icon_resources_supported(void);

/* Each call borrows only scalar records and byte ranges until it returns. */
#define CUI_GO_ICON_RESOURCE_API(X)                                                                \
    X(default_limits_v1, (cui_bridge_icon_limits_v1 * out), (out))                                 \
    X(create_v1,                                                                                   \
      (cui_bridge_app * app, const cui_bridge_icon_limits_v1 *limits,                              \
       cui_bridge_icon_handle_v1 *out),                                                            \
      (app, limits, out))                                                                          \
    X(destroy_v1, (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle), (app, handle))         \
    X(get_info_v1,                                                                                 \
      (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle, cui_bridge_icon_info_v1 * out),     \
      (app, handle, out))                                                                          \
    X(get_limits_v1,                                                                               \
      (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle, cui_bridge_icon_limits_v1 * out),   \
      (app, handle, out))                                                                          \
    X(set_limits_v1,                                                                               \
      (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle,                                     \
       const cui_bridge_icon_limits_v1 *limits),                                                   \
      (app, handle, limits))                                                                       \
    X(set_source_v1,                                                                               \
      (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle, uint32_t kind, const char *text,    \
       uint32_t bytes),                                                                            \
      (app, handle, kind, text, bytes))                                                            \
    X(get_source_v1,                                                                               \
      (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle, uint32_t *kind, char *out,          \
       uint32_t capacity, uint32_t *required),                                                     \
      (app, handle, kind, out, capacity, required))                                                \
    X(reload_v1, (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle), (app, handle))          \
    X(collect_v1, (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle), (app, handle))         \
    X(asset_put_v1,                                                                                \
      (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle, const char *path,                   \
       uint32_t path_bytes, const char *svg, uint32_t svg_bytes),                                  \
      (app, handle, path, path_bytes, svg, svg_bytes))                                             \
    X(asset_remove_v1,                                                                             \
      (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle, const char *path, uint32_t bytes),  \
      (app, handle, path, bytes))                                                                  \
    X(asset_clear_v1, (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle), (app, handle))     \
    X(catalog_entry_v1,                                                                            \
      (uint32_t index, uint32_t *embedded, char *name, uint32_t name_capacity,                     \
       uint32_t *name_required, char *path, uint32_t path_capacity, uint32_t *path_required),      \
      (index, embedded, name, name_capacity, name_required, path, path_capacity, path_required))   \
    X(query_v1, (const char *name, uint32_t length, uint32_t *out), (name, length, out))           \
    X(asset_copy_v1,                                                                               \
      (const char *name, uint32_t length, char *out, uint32_t capacity, uint32_t *required),       \
      (name, length, out, capacity, required))                                                     \
    X(prepare_v1,                                                                                  \
      (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle, float width, float height,          \
       const float *transform, uint32_t *error),                                                   \
      (app, handle, width, height, transform, error))                                              \
    X(bitmap_copy_v1,                                                                              \
      (cui_bridge_app * app, cui_bridge_icon_handle_v1 handle, cui_bridge_icon_bitmap_v1 * bitmap, \
       unsigned char *out, uint32_t capacity, uint32_t *required),                                 \
      (app, handle, bitmap, out, capacity, required))
#define CUI_GO_DECLARE_ICON(name, params, arguments) cui_bridge_status cui_go_icon_##name params;
CUI_GO_ICON_RESOURCE_API(CUI_GO_DECLARE_ICON)
#undef CUI_GO_DECLARE_ICON
uint32_t cui_go_icon_catalog_count_v1(void);
#endif
