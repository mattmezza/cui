/* SPDX-License-Identifier: MIT */
#include <cui/cui_bridge.h>
#include <stdlib.h>
#include <string.h>

static int fixture_is(const char *name) {
    const char *mode = getenv("CUI_DATA_TABLE_PAGING_FIXTURE");
    return mode && strcmp(mode, name) == 0;
}

uint32_t cui_bridge_data_table_load_more_size_v1(void) {
    return sizeof(cui_bridge_data_table_load_more_v1) - (fixture_is("size") ? 1u : 0u);
}

#ifndef CUI_TEST_PARTIAL_PAGING_PROVIDER
cui_bridge_status cui_bridge_data_table_configure_load_more_v1(cui_bridge_app *app,
                                                               cui_bridge_node node,
                                                               uint32_t enabled,
                                                               uint32_t threshold) {
    (void)app;
    (void)node;
    (void)enabled;
    (void)threshold;
    return CUI_BRIDGE_OK;
}
#endif

cui_bridge_status cui_bridge_data_table_poll_load_more_v1(cui_bridge_app *app, cui_bridge_node node,
                                                          uint32_t size,
                                                          cui_bridge_data_table_load_more_v1 *out) {
    (void)app;
    (void)node;
    if (!out || size != sizeof(*out))
        return CUI_BRIDGE_INVALID_ARGUMENT;
    *out = (cui_bridge_data_table_load_more_v1){
        .frame = UINT64_MAX - 1u,
        .row_count = 8,
        .visible_begin = 4,
        .visible_end = 8,
        .last_row_key = UINT32_MAX,
        .capacity = 32,
    };
    if (fixture_is("reserved"))
        out->reserved = 1;
    if (fixture_is("frame"))
        out->frame = 0;
    if (fixture_is("window"))
        out->visible_begin = 9;
    if (fixture_is("visible"))
        out->visible_end = 9;
    if (fixture_is("capacity"))
        out->capacity = 8;
    if (fixture_is("key"))
        out->last_row_key = 0;
    return CUI_BRIDGE_OK;
}
