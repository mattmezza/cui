/* SPDX-License-Identifier: MIT */
/* Instrument only this TU; no production/header allocator hooks. */
#include <stdlib.h>
static unsigned allocations, releases, numeric_releases;
static void *numeric_owned;
static size_t allocated_bytes;
static int fail_next;
static void *numeric_calloc(size_t n, size_t size) {
    ++allocations;
    allocated_bytes = n * size;
    if (fail_next) {
        fail_next = 0;
        return NULL;
    }
    return calloc(n, size);
}
static void numeric_free(void *p) {
    if (p)
        ++releases;
    if (p && p == numeric_owned)
        ++numeric_releases;
    free(p);
}
#define calloc numeric_calloc
#define free numeric_free
#ifdef CUI_NUMERIC_DISTRIBUTION
#define CUI_IMPLEMENTATION
#include "cui.h"
#else
#include "../../src/core/cui.c"
#endif
#undef calloc
#undef free
#include <assert.h>
static void begin(cui_context *ui) {
    assert(cui_begin_frame(ui, 300, 100, 0));
}
static void slider(cui_context *ui) {
    cui_begin_box(ui, 10, (cui_box){.width = 100, .height = 20});
    (void)cui_interact(ui, 10, CUI_ROLE_SLIDER, (cui_str){0}, false, false, 0);
    cui_end_box(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 100, .max_elements = 32});
    assert(ui);
    assert(!ui->numeric && !cui_accessibility_numeric_snapshot_ready(ui));
    begin(ui);
    slider(ui);
    unsigned before = allocations;
    assert(cui_set_accessibility_numeric_actions(ui, 10, 0));
    assert(allocations == before && !ui->numeric);
    (void)cui_end_frame(ui);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    begin(ui);
    slider(ui);
    before = allocations;
    fail_next = 1;
    assert(!cui_set_accessibility_numeric_actions(ui, 10, 7));
    assert(allocations == before + 1 && !ui->numeric && !fail_next);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    (void)cui_end_frame(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    begin(ui);
    slider(ui);
    before = allocations;
    assert(cui_set_accessibility_numeric_actions(ui, 10, 7));
    assert(allocations == before + 1 && ui->numeric);
    assert(allocated_bytes == sizeof(cui_core_numeric_state) + 2 * 32 * sizeof(uint32_t));
    cui_core_numeric_state *owned = ui->numeric;
    numeric_owned = owned;
    assert(cui_set_accessibility_numeric_actions(ui, 10, 0));
    assert(cui_set_accessibility_numeric_actions(ui, 10, 4));
    assert(ui->numeric == owned && allocations == before + 1);
    (void)cui_end_frame(ui);
    assert(cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_SET_VALUE));
    begin(ui);
    slider(ui); /* no registration: no inherited capability */
    (void)cui_end_frame(ui);
    assert(!cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_SET_VALUE));
    /* Private typed origin/presence corruption must not become a value. */
    begin(ui);
    slider(ui);
    assert(cui_set_accessibility_numeric_actions(ui, 10, 7));
    (void)cui_end_frame(ui);
    cui_accessibility_set_value_v1 value = {1, 0, .75};
    assert(cui_queue_accessibility_set_value_v1(ui, 10, sizeof value, &value));
    begin(ui);
    slider(ui);
    assert(cui_set_accessibility_numeric_actions(ui, 10, 7));
    cui_accessibility_numeric_input_v1 out, saved;
    memset(&out, 0xa5, sizeof out);
    saved = out;
    size_t origin = ui->event_origin[0];
    ui->event_origin[0] = SIZE_MAX;
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, 0, sizeof out, &out));
    assert(!memcmp(&out, &saved, sizeof out) && !ui->event_consumed[0]);
    ui->event_origin[0] = origin;
    ui->numeric->frame[origin].present = false;
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, 0, sizeof out, &out));
    assert(!memcmp(&out, &saved, sizeof out));
    (void)cui_end_frame(ui);
    printf("numeric context=%zu baseline_unchanged_tail=%zu idle_delta=%zu numeric_fixed=%zu "
           "slot=%zu N32_allocation=%zu\n",
           sizeof(cui_context), sizeof(cui_context) - offsetof(cui_context, scroll_layout),
           offsetof(cui_context, scroll_layout), sizeof(cui_core_numeric_state),
           sizeof(cui_core_numeric_slot), sizeof(cui_core_numeric_state) + 64 * sizeof(uint32_t));
    unsigned free_before = releases;
    cui_destroy(ui);
    assert(releases > free_before && numeric_releases == 1);
    return 0;
}
