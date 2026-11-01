/* SPDX-License-Identifier: MIT */
/* Synthetic Linux optional-symbol provider; no widget/input evidence. */
#include <cui/cui_bridge.h>
#include <stdlib.h>
#include <string.h>
#ifndef CUI_TIME_OMIT
#define CUI_TIME_OMIT 99
#endif
static int mode(const char *name) {
    const char *value = getenv("CUI_TIME_FIXTURE");
    return value && strcmp(value, name) == 0;
}
#if CUI_TIME_OMIT != 0
uint32_t cui_bridge_time_field_versions(void) {
    return mode("unavailable") ? 0 : 1;
}
#endif
#if CUI_TIME_OMIT != 1
uint32_t cui_bridge_time_field_config_size_v1(void) {
    return mode("config_size") ? 0 : 616;
}
#endif
#if CUI_TIME_OMIT != 2
uint32_t cui_bridge_time_field_result_size_v1(void) {
    return mode("result_size") ? 0 : 80;
}
#endif
#if CUI_TIME_OMIT != 3
uint32_t cui_bridge_time_field_change_size_v1(void) {
    return mode("change_size") ? 0 : 64;
}
#endif
#if CUI_TIME_OMIT != 4
cui_bridge_status cui_bridge_time_field_enable_v1(cui_bridge_app *app, uint32_t capacity) {
    (void)app;
    (void)capacity;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_TIME_OMIT != 5
cui_bridge_status
cui_bridge_time_field_configure_v1(cui_bridge_app *app, cui_bridge_node node,
                                   const cui_bridge_time_field_config_v1 *config) {
    (void)app;
    (void)node;
    (void)config;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_TIME_OMIT != 6
cui_bridge_status cui_bridge_time_field_get_config_v1(cui_bridge_app *app, cui_bridge_node node,
                                                      cui_bridge_time_field_config_v1 *out) {
    (void)app;
    (void)node;
    (void)out;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_TIME_OMIT != 7
cui_bridge_status cui_bridge_time_field_get_result_v1(cui_bridge_app *app, cui_bridge_node node,
                                                      cui_bridge_time_field_result_v1 *out) {
    (void)app;
    (void)node;
    (void)out;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_TIME_OMIT != 8
cui_bridge_status cui_bridge_time_field_clear_validation_v1(cui_bridge_app *app,
                                                            cui_bridge_node node) {
    (void)app;
    (void)node;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_TIME_OMIT != 9
cui_bridge_status cui_bridge_time_field_poll_change_v1(cui_bridge_app *app,
                                                       cui_bridge_time_field_change_v1 *out) {
    (void)app;
    (void)out;
    return CUI_BRIDGE_EMPTY;
}
#endif
