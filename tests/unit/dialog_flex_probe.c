/* SPDX-License-Identifier: MIT */
/* Real production implementation; only private size/resource/corruption probes. */
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
static bool allocation_tracking, fail_sidecar_once;
static size_t allocation_base_size, allocation_counts[5];
static struct {
    void *pointer;
    unsigned kind;
} allocation_pointers[64];
static void *dialog_flex_calloc(size_t count, size_t bytes) {
    bool base = allocation_tracking && count == 1 && bytes == allocation_base_size;
    bool side = allocation_tracking && count == 1 && bytes == 2560;
    if (side) {
        ++allocation_counts[4];
        if (fail_sidecar_once) {
            fail_sidecar_once = false;
            return NULL;
        }
    }
    void *p = calloc(count, bytes);
    if (p && (base || side)) {
        ++allocation_counts[side ? 1 : 0];
        size_t slot = 0;
        while (slot < 64 && allocation_pointers[slot].pointer)
            ++slot;
        assert(slot < 64);
        allocation_pointers[slot].pointer = p;
        allocation_pointers[slot].kind = side ? 1 : 0;
    }
    return p;
}
static void dialog_flex_free(void *pointer) {
    if (allocation_tracking && pointer)
        for (size_t i = 0; i < 64; ++i)
            if (allocation_pointers[i].pointer == pointer) {
                ++allocation_counts[allocation_pointers[i].kind + 2];
                allocation_pointers[i].pointer = NULL;
                break;
            }
    free(pointer);
}
#define calloc dialog_flex_calloc
#define free dialog_flex_free
#ifdef CUI_DIALOG_REGIONS_DISTRIBUTION
#define CUI_IMPLEMENTATION
#include "cui.h"
#else
#include "../../src/core/cui.c"
#endif
#undef calloc
#undef free
#include "dialog_flex_probe.h"
#include <assert.h>
size_t dialog_flex_base_resource_bytes(void) {
    return sizeof(cui_core_sl_state);
}
size_t dialog_flex_sidecar_resource_bytes(void) {
    return sizeof(cui_core_sl_regions);
}
void dialog_flex_resources(cui_context *ui, size_t *slots, size_t *bytes) {
    *slots = *bytes = 0;
    for (size_t i = 0; i < ui->config.max_resources; ++i)
        if (ui->resources[i].payload) {
            ++*slots;
            *bytes += ui->resources[i].bytes;
        }
    assert(*bytes == ui->resource_bytes);
}
void dialog_flex_corrupt_cache(cui_context *ui, unsigned mode) {
    assert(ui->scroll_layout && ui->scroll_layout != &cui_core_sl_failed);
    cui_core_sl_state *state = (void *)ui->scroll_layout;
    cui_core_sl_regions *regions;
    assert(cui_core_sl_regions_read(ui, state, &regions) && regions);
    switch (mode) {
    case 0:
        state->header.regions_slot = 0;
        break;
    case 1:
        state->header.regions_slot = UINT32_MAX;
        break;
    case 2:
        for (size_t i = 0; i < ui->config.max_resources; ++i)
            if (ui->resources[i].payload == state)
                state->header.regions_slot = (uint32_t)i + 1;
        break;
    case 3:
        state->header.flags &= ~CUI_CORE_SL_REGIONS;
        break;
    case 4:
        regions->body[0].version = 0;
        break;
    case 5:
        state->bodies[0].record.version = 1;
        break;
    default:
        assert(false);
    }
}

void dialog_flex_allocations_begin(bool fail_sidecar) {
    allocation_tracking = true;
    fail_sidecar_once = fail_sidecar;
    allocation_base_size = sizeof(cui_core_sl_state);
    memset(allocation_counts, 0, sizeof allocation_counts);
    memset(allocation_pointers, 0, sizeof allocation_pointers);
}
dialog_flex_allocation_counts dialog_flex_allocations_read(void) {
    return (dialog_flex_allocation_counts){allocation_counts[0], allocation_counts[1],
                                           allocation_counts[2], allocation_counts[3],
                                           allocation_counts[4]};
}
