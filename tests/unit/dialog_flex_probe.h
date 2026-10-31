/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEST_DIALOG_FLEX_PROBE_H
#define CUI_TEST_DIALOG_FLEX_PROBE_H
#include "cui/cui.h"
/* Test-only inspection; no layout algorithms or public API shims. */
size_t dialog_flex_base_resource_bytes(void);
size_t dialog_flex_sidecar_resource_bytes(void);
void dialog_flex_resources(cui_context *, size_t *slots, size_t *bytes);
void dialog_flex_corrupt_cache(cui_context *, unsigned mode);
typedef struct {
    size_t base_alloc, sidecar_alloc, base_free, sidecar_free, sidecar_attempt;
} dialog_flex_allocation_counts;
void dialog_flex_allocations_begin(bool fail_sidecar);
dialog_flex_allocation_counts dialog_flex_allocations_read(void);
void test_scroll_regions_contract(void);
#endif
