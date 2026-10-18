/* Test-only Linux interposition; synthetic protocol evidence, not UI events. */
#include <cui/cui_bridge.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static const char *mode(void) {
    const char *s = getenv("CUI_VALUE_FIXTURE");
    return s ? s : "edges";
}
static int is(const char *s) {
    return strcmp(mode(), s) == 0;
}
uint32_t cui_bridge_notification_v1_size(void) {
    return is("size") ? 0u : (uint32_t)sizeof(cui_bridge_notification_v1);
}
uint32_t cui_bridge_notification_versions(void) {
    return 1;
}
#ifndef CUI_VALUE_FIXTURE_MISSING
uint32_t cui_bridge_notification_value_payloads_v1(void) {
    return is("unavailable") ? 0x80000000u : 28u | 0x80000000u;
}
#endif
cui_bridge_status cui_bridge_poll_value_notification_v1(cui_bridge_app *app, uint32_t size,
                                                        uint32_t mask,
                                                        cui_bridge_notification_v1 *out, char *text,
                                                        uint32_t capacity, uint32_t *required) {
    static unsigned delivered;
    (void)app;
    if (size != sizeof(*out) || mask != 28u)
        return CUI_BRIDGE_INVALID_ARGUMENT;
    if (is("limit") || is("zero") || is("repeat")) {
        *required = is("limit") ? 1048577u : is("zero") ? 0u : capacity + 1u;
        return CUI_BRIDGE_CAPACITY;
    }
    if (delivered >= 4)
        return CUI_BRIDGE_EMPTY;
    uint32_t length = is("capacity") ? 8192u : 3u;
    if (capacity <= length) {
        *required = length + 1;
        return CUI_BRIDGE_CAPACITY;
    }
    memset(out, 0, sizeof(*out));
    memset(text, 'x', length);
    text[length] = 0;
    *required = length + 1;
    out->struct_size = sizeof(*out);
    out->schema_version = 1;
    out->kind = out->legacy.type = 2;
    out->payload_kind = delivered == 0 ? 2u : delivered == 1 ? 3u : 4u;
    out->flags = 1;
    out->node.slot = out->legacy.node.slot = UINT32_MAX;
    out->node.generation = out->legacy.node.generation = UINT32_MAX;
    const uint64_t frames[] = {UINT64_C(9007199254740991), UINT64_C(9007199254740992),
                               UINT64_C(9007199254740993), UINT64_MAX};
    out->frame_sequence = out->legacy.frame_sequence = frames[delivered];
    out->text_length = out->legacy.text_length = length;
    if (delivered == 0)
        out->payload.scalar_value = out->legacy.value = 16777216.5;
    if (delivered >= 2) {
        out->payload.selected.slot = out->legacy.selected.slot = UINT32_MAX;
        out->payload.selected.generation = out->legacy.selected.generation = UINT32_MAX;
    }
    if (is("tag"))
        out->payload_kind = 99;
    if (is("kind"))
        out->kind = out->legacy.type = 1;
    if (is("utf8"))
        text[0] = (char)0xff;
    if (is("nul"))
        text[length] = 'x';
    if (is("flags"))
        out->flags = 2;
    ++delivered;
    return CUI_BRIDGE_OK;
}
