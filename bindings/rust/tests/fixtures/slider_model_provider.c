/* Test-only Rust optional-group provider. Run beside the actual old bridge.
 * Synthetic protocol evidence only; mirrors the fixed C wire contract. */
#include <cui/cui_bridge.h>
#include <stdlib.h>
#include <string.h>
#ifndef CUI_SLIDER_OMIT
#define CUI_SLIDER_OMIT 99
#endif
static int is(const char *mode) {
    const char *actual = getenv("CUI_SLIDER_FIXTURE");
    return actual && strcmp(actual, mode) == 0;
}
#if CUI_SLIDER_OMIT != 0
uint32_t cui_bridge_slider_model_versions(void) {
    return is("unavailable") ? 0u : 1u;
}
#endif
#if CUI_SLIDER_OMIT != 1
uint32_t cui_bridge_slider_config_v1_size(void) {
    return is("config_size") ? 0u : (uint32_t)sizeof(cui_bridge_slider_config_v1);
}
#endif
#if CUI_SLIDER_OMIT != 2
uint32_t cui_bridge_slider_value_v1_size(void) {
    return is("value_size") ? 0u : (uint32_t)sizeof(cui_bridge_slider_value_v1);
}
#endif
#if CUI_SLIDER_OMIT != 3
uint32_t cui_bridge_slider_receipt_v1_size(void) {
    return is("receipt_size") ? 0u : (uint32_t)sizeof(cui_bridge_slider_receipt_v1);
}
#endif
#if CUI_SLIDER_OMIT != 4
uint32_t cui_bridge_slider_request_v1_size(void) {
    return is("request_size") ? 0u : (uint32_t)sizeof(cui_bridge_slider_request_v1);
}
#endif
#if CUI_SLIDER_OMIT != 5
uint32_t cui_bridge_slider_presentation_v1_size(void) {
    return is("presentation_size") ? 0u : (uint32_t)sizeof(cui_bridge_slider_presentation_v1);
}
#endif
#if CUI_SLIDER_OMIT != 6
cui_bridge_status
cui_bridge_slider_queue_enable_v1(cui_bridge_app *app, uint32_t size,
                                  const cui_bridge_slider_queue_options_v1 *options) {
    (void)app;
    (void)size;
    (void)options;
    return CUI_BRIDGE_OK;
}
#endif
#if CUI_SLIDER_OMIT != 7
cui_bridge_status cui_bridge_slider_model_create_v1(cui_bridge_app *app, cui_bridge_node parent,
                                                    const char *key, uint32_t cs,
                                                    const cui_bridge_slider_config_v1 *config,
                                                    uint32_t vs,
                                                    const cui_bridge_slider_value_v1 *value,
                                                    cui_bridge_node *out) {
    (void)app;
    (void)parent;
    (void)key;
    (void)cs;
    (void)config;
    (void)vs;
    (void)value;
    (void)out;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_SLIDER_OMIT != 8
cui_bridge_status cui_bridge_slider_model_configure_v1(cui_bridge_app *app, cui_bridge_node node,
                                                       uint32_t cs,
                                                       const cui_bridge_slider_config_v1 *config,
                                                       uint32_t vs,
                                                       const cui_bridge_slider_value_v1 *value) {
    (void)app;
    (void)node;
    (void)cs;
    (void)config;
    (void)vs;
    (void)value;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_SLIDER_OMIT != 9
cui_bridge_status cui_bridge_slider_model_get_config_v1(cui_bridge_app *app, cui_bridge_node node,
                                                        uint32_t size,
                                                        cui_bridge_slider_config_v1 *out) {
    (void)app;
    (void)node;
    (void)size;
    (void)out;
    if (is("config_parent")) {
        memset(out, 0, sizeof(*out));
        out->struct_size = sizeof(*out);
        out->schema_version = 1;
        out->domain.maximum = 100;
        out->domain.step = 1;
        out->rem = 16;
        out->length = 200;
        out->layout = CUI_BRIDGE_SLIDER_PARENT;
        return CUI_BRIDGE_OK;
    }
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_SLIDER_OMIT != 10
cui_bridge_status cui_bridge_slider_model_set_value_v1(cui_bridge_app *app, cui_bridge_node node,
                                                       uint32_t size,
                                                       const cui_bridge_slider_value_v1 *value) {
    (void)app;
    (void)node;
    (void)size;
    (void)value;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_SLIDER_OMIT != 11
cui_bridge_status cui_bridge_slider_model_get_value_v1(cui_bridge_app *app, cui_bridge_node node,
                                                       uint32_t size,
                                                       cui_bridge_slider_value_v1 *out) {
    (void)app;
    (void)node;
    (void)size;
    (void)out;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_SLIDER_OMIT != 12
cui_bridge_status cui_bridge_poll_slider_request_v1(cui_bridge_app *app, uint32_t size,
                                                    cui_bridge_slider_request_v1 *out) {
    (void)app;
    (void)size;
    (void)out;
    memset(out, 0, sizeof(*out));
    out->struct_size = sizeof(*out);
    out->schema_version = 1;
    out->phase = 1;
    out->thumb = 1;
    out->event_kind = 1;
    out->event_source = 3;
    out->event_flags = 7;
    out->receipt = (cui_bridge_slider_receipt_v1){
        sizeof(out->receipt), 1,          UINT64_MAX, {UINT32_MAX, UINT32_MAX},
        UINT64_MAX,           UINT64_MAX, UINT64_MAX, UINT64_MAX};
    out->event_target = out->event_frame = out->event_index = UINT64_MAX;
    out->current = (cui_bridge_slider_value_v1){1, 0, 20, 60};
    out->requested = (cui_bridge_slider_value_v1){1, 0, 20, 59};
    out->domain = (cui_bridge_slider_domain_v1){0, 100, 1, 0, 0};
    out->gesture_domain = out->domain;
    out->input =
        (cui_bridge_slider_input_v1){3, 1, 5, 15, 0, 0, 1, 0, 0, 0, UINT64_MAX, UINT64_MAX, 0, 0};
    if (is("receipt_size_bad"))
        out->receipt.struct_size = 0;
    if (is("receipt_schema"))
        out->receipt.schema_version = 2;
    if (is("phase"))
        out->phase = 99;
    if (is("reason"))
        out->reason = 99;
    if (is("source"))
        out->event_source = 99;
    if (is("flags"))
        out->event_flags = 8;
    if (is("absent_source"))
        out->event_flags = 3;
    if (is("absent_frame"))
        out->event_flags = 6;
    if (is("absent_index")) {
        out->event_flags = 5;
        out->event_index = 0;
    }
    if (is("absent_input"))
        out->input.flags = 0;
    if (is("input_flags"))
        out->input.flags = 4;
    if (is("modifiers"))
        out->input.modifiers = 16;
    if (is("key"))
        out->input.key = 999;
    if (is("input_type"))
        out->input.type = 999;
    if (is("numeric"))
        out->input.flags = 2;
    if (is("inactive_numeric"))
        out->input.accessibility_value = 0.25;
    if (is("value_reserved"))
        out->current.reserved = 1;
    if (is("domain_reserved"))
        out->domain.reserved = 1;
    return CUI_BRIDGE_OK;
}
#endif
#if CUI_SLIDER_OMIT != 13
cui_bridge_status cui_bridge_accept_slider_request_v1(cui_bridge_app *app, uint32_t rs,
                                                      const cui_bridge_slider_receipt_v1 *receipt,
                                                      uint32_t vs,
                                                      const cui_bridge_slider_value_v1 *value) {
    (void)app;
    (void)rs;
    (void)receipt;
    (void)vs;
    (void)value;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_SLIDER_OMIT != 14
cui_bridge_status
cui_bridge_slider_model_get_presentation_v1(cui_bridge_app *app, cui_bridge_node node,
                                            uint32_t size, cui_bridge_slider_presentation_v1 *out) {
    (void)app;
    (void)node;
    (void)size;
    (void)out;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
#if CUI_SLIDER_OMIT != 15
cui_bridge_status cui_bridge_slider_model_part_bounds_v1(cui_bridge_app *app, cui_bridge_node node,
                                                         uint32_t part, cui_bridge_rect *out) {
    (void)app;
    (void)node;
    (void)part;
    (void)out;
    return CUI_BRIDGE_INVALID_STATE;
}
#endif
