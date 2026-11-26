/* Synthetic protocol evidence only; no claims about input routing. */
#include "cui/cui_bridge.h"
#include <stdlib.h>
#include <string.h>
uint32_t cui_bridge_notification_value_payloads_v1(void) {
    return 256u | 28u | (1u << 31);
}
cui_bridge_status cui_bridge_poll_value_notification_v1(cui_bridge_app *app, uint32_t size,
                                                        uint32_t enabled,
                                                        cui_bridge_notification_v1 *out, char *text,
                                                        uint32_t capacity, uint32_t *required) {
    (void)app;
    (void)size;
    if (enabled != 284u || capacity < 1)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    memset(out, 0, sizeof *out);
    text[0] = 0;
    *required = 1;
    out->struct_size = sizeof *out;
    out->schema_version = 1;
    out->kind = 1;
    out->payload_kind = 8;
    out->node_kind = CUI_BRIDGE_ATTACHMENT;
    out->node.slot = UINT32_MAX;
    out->node.generation = UINT32_MAX - 1;
    out->flags = 1;
    out->frame_sequence = UINT64_MAX;
    out->legacy.type = 1;
    out->legacy.node = out->node;
    out->legacy.frame_sequence = out->frame_sequence;
    out->legacy.value = 4;
    cui_bridge_attachment_action_v1 *a = &out->payload.attachment_action;
    *a = (cui_bridge_attachment_action_v1){4, 3, 2, 6, UINT32_MAX};
    const char *mode = getenv("CUI_ATTACHMENT_FIXTURE");
    if (mode && !strcmp(mode, "flags"))
        a->flags = 8;
    if (mode && !strcmp(mode, "action"))
        a->action = 3;
    if (mode && !strcmp(mode, "part"))
        a->part = 2;
    if (mode && !strcmp(mode, "source"))
        a->source = 1;
    if (mode && !strcmp(mode, "index"))
        a->flags = 4;
    if (mode && !strcmp(mode, "node"))
        out->node_kind = 4;
    if (mode && !strcmp(mode, "frame"))
        out->flags = 0;
    if (mode && !strcmp(mode, "legacy"))
        out->legacy.value = 1;
    if (mode && !strcmp(mode, "checked"))
        out->legacy.checked = 1;
    if (mode && !strcmp(mode, "selected"))
        out->legacy.selected.slot = 1;
    if (mode && !strcmp(mode, "index-only")) {
        a->flags = CUI_BRIDGE_ACTIVATION_HAS_INDEX_V1;
        a->source = 0;
    }
    if (mode && !strcmp(mode, "source-only")) {
        a->flags = CUI_BRIDGE_ACTIVATION_HAS_SOURCE_V1;
        a->event_index = 0;
    }
    return CUI_BRIDGE_OK;
}
