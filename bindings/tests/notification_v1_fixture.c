/* Test-only V1 protocol injection. Link against the real bridge for ordinary
 * App APIs. These synthetic frames/capacities are NOT real queue evidence. */
#include "cui/cui_bridge.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static unsigned delivered;
static const char *mode(void) {
    const char *value = getenv("CUI_NOTIFICATION_FIXTURE");
    return value ? value : "edges";
}
uint32_t cui_bridge_notification_versions(void) {
    return strcmp(mode(), "unavailable") == 0 ? 0u : 1u;
}
uint32_t cui_bridge_notification_v1_size(void) {
    return (uint32_t)sizeof(cui_bridge_notification_v1) +
           (strcmp(mode(), "size") == 0 ? 1u : 0u);
}
cui_bridge_status cui_bridge_poll_notification_v1(
    cui_bridge_app *app, uint32_t size, cui_bridge_notification_v1 *out,
    char *text, uint32_t capacity, uint32_t *required) {
    const char *m = mode();
    uint32_t length = strcmp(m, "capacity") == 0 ? 8192u : 3u;
    static const uint64_t frames[] = {
        UINT64_C(9007199254740991), UINT64_C(9007199254740992),
        UINT64_C(9007199254740993), UINT64_MAX
    };
    (void)app;
    if (!out || !required || size != sizeof(*out)) return CUI_BRIDGE_INVALID_ARGUMENT;
    if (strcmp(m, "limit") == 0) {
        *required = 1048577u;
        return CUI_BRIDGE_CAPACITY;
    }
    if (strcmp(m, "zero") == 0) { *required = 0; return CUI_BRIDGE_CAPACITY; }
    if (strcmp(m, "repeat") == 0) { *required = capacity + 1u; return CUI_BRIDGE_CAPACITY; }
    if (delivered >= 4u) return CUI_BRIDGE_EMPTY;
    *required = length + 1u;
    if (capacity < *required) return CUI_BRIDGE_CAPACITY;
    memset(out, 0, sizeof(*out));
    out->struct_size = (uint32_t)sizeof(*out);
    out->schema_version = 1u;
    out->kind = delivered == 3u ? 3u : (delivered ? 2u : 1u);
    out->payload_kind = delivered == 0u ? 0u : delivered == 1u ? 1u : delivered == 2u ? 5u : 6u;
    out->flags = 1u;
    out->node.slot = UINT32_MAX;
    out->node.generation = UINT32_MAX;
    out->frame_sequence = frames[delivered];
    out->legacy.frame_sequence = frames[delivered];
    out->legacy.checked = UINT32_MAX;
    out->legacy.selected = out->node;
    out->text_length = length;
    if (delivered == 1u) out->payload.boolean_value = 1u;
    if (delivered == 2u) {
        out->payload.table.reason = 1u;
        out->payload.table.row = UINT32_MAX;
        out->payload.table.column = UINT32_MAX - 1u;
        out->payload.table.sort_column = UINT32_MAX;
        out->payload.table.sort_direction = 2u;
    }
    if (delivered == 3u) out->payload.surface_result = 3u;
    memset(text, 'x', length);
    text[length] = '\0';
    ++delivered;
    return CUI_BRIDGE_OK;
}
