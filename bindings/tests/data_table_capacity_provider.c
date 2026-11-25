/* SPDX-License-Identifier: MIT */
/* Tests interpose one V1 export, keeping the selected provider's remaining ABI. */
#include "cui/cui_bridge.h"
#include <stdlib.h>
#include <string.h>

#ifdef CUI_TEST_CAPACITY_SETTER_ONLY
cui_bridge_status cui_bridge_data_table_set_keyed_capacity_v1(
    cui_bridge_app *app, cui_bridge_node node, const char *const *headers, uint32_t columns,
    const char *const *cells, const uint32_t *keys, uint32_t rows, uint32_t capacity) {
    (void)app;
    (void)node;
    (void)headers;
    (void)columns;
    (void)cells;
    (void)keys;
    (void)rows;
    (void)capacity;
    return CUI_BRIDGE_CORE_ERROR;
}
#else
uint32_t cui_bridge_data_table_max_row_capacity_v1(void) {
    const char *mode = getenv("CUI_DATA_TABLE_CAPACITY_FIXTURE");
    if (mode && strcmp(mode, "zero") == 0)
        return 0;
    if (mode && strcmp(mode, "large") == 0)
        return UINT32_MAX;
    return 48;
}
#endif
