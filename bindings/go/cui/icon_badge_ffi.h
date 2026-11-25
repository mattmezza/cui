/* SPDX-License-Identifier: MIT */
#ifndef CUI_GO_ICON_BADGE_FFI_H
#define CUI_GO_ICON_BADGE_FFI_H
#include "cui/cui_bridge.h"

void cui_go_icon_badge_init(void *library);
int cui_go_icon_presentation_supported(void);
int cui_go_badge_supported(void);

#define CUI_GO_ICON_PRESENTATION_API(X)                                                            \
    X(configure_node_v1,                                                                           \
      (cui_bridge_app * app, cui_bridge_node node, cui_bridge_icon_handle_v1 resource,             \
       const cui_bridge_icon_options_v1 *options, const char *name, uint32_t bytes),               \
      (app, node, resource, options, name, bytes))                                                 \
    X(get_node_config_v1,                                                                          \
      (cui_bridge_app * app, cui_bridge_node node, cui_bridge_icon_handle_v1 * resource,           \
       cui_bridge_icon_options_v1 * options),                                                      \
      (app, node, resource, options))                                                              \
    X(copy_node_name_v1,                                                                           \
      (cui_bridge_app * app, cui_bridge_node node, char *name, uint32_t capacity,                  \
       uint32_t *required),                                                                        \
      (app, node, name, capacity, required))                                                       \
    X(reset_node_v1, (cui_bridge_app * app, cui_bridge_node node), (app, node))                    \
    X(get_node_result_v1,                                                                          \
      (cui_bridge_app * app, cui_bridge_node node, cui_bridge_icon_result_v1 * out),               \
      (app, node, out))

#define CUI_GO_BADGE_API(X)                                                                        \
    X(default_config_v1, (cui_bridge_badge_config_v1 * out), (out))                                \
    X(create_v1,                                                                                   \
      (cui_bridge_app * app, cui_bridge_node parent, const char *key,                              \
       const cui_bridge_badge_config_v1 *config, const cui_bridge_icon_options_v1 *artwork,        \
       const char *name, uint32_t bytes, const char *named_icon, uint32_t icon_bytes,              \
       cui_bridge_node *out),                                                                      \
      (app, parent, key, config, artwork, name, bytes, named_icon, icon_bytes, out))               \
    X(configure_v1,                                                                                \
      (cui_bridge_app * app, cui_bridge_node node, const cui_bridge_badge_config_v1 *config,       \
       const cui_bridge_icon_options_v1 *artwork, const char *name, uint32_t bytes,                \
       const char *named_icon, uint32_t icon_bytes),                                               \
      (app, node, config, artwork, name, bytes, named_icon, icon_bytes))                           \
    X(get_config_v1,                                                                               \
      (cui_bridge_app * app, cui_bridge_node node, cui_bridge_badge_config_v1 * config,            \
       cui_bridge_icon_options_v1 * artwork),                                                      \
      (app, node, config, artwork))                                                                \
    X(copy_text_v1,                                                                                \
      (cui_bridge_app * app, cui_bridge_node node, uint32_t field, char *out, uint32_t capacity,   \
       uint32_t *required),                                                                        \
      (app, node, field, out, capacity, required))                                                 \
    X(get_result_v1,                                                                               \
      (cui_bridge_app * app, cui_bridge_node node, cui_bridge_badge_result_v1 * out),              \
      (app, node, out))                                                                            \
    X(indicator_bounds_v1, (cui_bridge_app * app, cui_bridge_node node, cui_bridge_rect * out),    \
      (app, node, out))

#define CUI_GO_DECLARE_ICON(name, params, args) cui_bridge_status cui_go_icon_##name params;
CUI_GO_ICON_PRESENTATION_API(CUI_GO_DECLARE_ICON)
#undef CUI_GO_DECLARE_ICON
#define CUI_GO_DECLARE_BADGE(name, params, args) cui_bridge_status cui_go_badge_##name params;
CUI_GO_BADGE_API(CUI_GO_DECLARE_BADGE)
#undef CUI_GO_DECLARE_BADGE
#endif
