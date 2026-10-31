/* SPDX-License-Identifier: MIT */
/* TU-local failure injection; no production allocator hook or context-create OOM. */
#include <stdlib.h>
static size_t fail_bytes, allocations;
static void *expired_commands;
static unsigned expired_frees;
static void *snapshot_malloc(size_t size) {
    ++allocations;
    if (fail_bytes && size == fail_bytes) {
        fail_bytes = 0;
        return NULL;
    }
    return malloc(size);
}
static void snapshot_free(void *p) {
    if (p && p == expired_commands)
        ++expired_frees;
    free(p);
}
#define malloc snapshot_malloc
#define free snapshot_free
#ifdef CUI_SNAPSHOT_DISTRIBUTION
#define CUI_IMPLEMENTATION
#include "cui.h"
#else
#include "../../src/core/cui.c"
#endif
#undef malloc
#undef free
#include <assert.h>
#include <float.h>
#include <stdio.h>
static void good(cui_context *ui) {
    assert(cui_begin_frame(ui, 200, 100, 0));
    cui_begin_box(ui, 10, (cui_box){.width = 80, .height = 30});
    (void)cui_interact(ui, 10, CUI_ROLE_BUTTON, (cui_str){0}, false, false, 0);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    assert(!cui_frame_input_snapshot_ready(ui));
}
static cui_frame_v2_options options(size_t commands) {
    return (cui_frame_v2_options){
        .size = sizeof(cui_frame_v2_options),
        .support = {.size = sizeof(cui_draw_v2_support), .abi_version = CUI_DRAW_ABI_V2,
                    .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                    .features = CUI_DRAW_V2_FEATURE_BACKGROUNDS, .max_commands = commands,
                    .max_backgrounds = 1, .max_pixels = 1024, .max_pixels_per_background = 1024,
                    .cpu_bytes = 8192, .texture_bytes = 8192, .transfer_bytes = 8192,
                    .metadata_bytes = 8192},
        .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
        .command_bytes = commands * sizeof(cui_draw_command_v2), .background_bytes = 8192};
}
static void lifecycle(cui_context *ui) {
    assert(!cui_frame_input_snapshot_ready(NULL));
    assert(!cui_frame_input_snapshot_ready(ui));
    assert(!ui->numeric);
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(!cui_frame_input_snapshot_ready(ui));
    (void)cui_end_frame(ui);
    good(ui);
    size_t before = allocations;
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(cui_frame_input_snapshot_ready(ui));
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    assert(!ui->numeric && allocations == before);
    /* Authority is not current modal eligibility and does not imply a semantic. */
    assert(cui_begin_focus_scope(ui, 50, true));
    assert(cui_frame_input_snapshot_ready(ui) && cui_pointer_scope_allowed(ui, 0));
    cui_end_focus_scope(ui);
    assert(cui_frame_input_snapshot_ready(ui) && !cui_pointer_scope_allowed(ui, 0));
    (void)cui_end_frame(ui); /* Omitted controls/empty semantics are still a valid snapshot. */
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(cui_frame_input_snapshot_ready(ui));
    cui_begin_box(ui, 12, (cui_box){.width = 10, .height = 10, .inert = true});
    assert(!cui_frame_sequence(ui) && cui_frame_input_snapshot_ready(ui));
    cui_end_box(ui);
    cui_begin_box(ui, 13, (cui_box){.width = 10, .height = 10, .layout_only = true});
    assert(!cui_frame_sequence(ui) && cui_frame_input_snapshot_ready(ui));
    cui_end_box(ui);
    cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_LAYOUT});
    assert(!cui_frame_input_snapshot_ready(ui));
    cui_clear_error(ui);
    assert(!cui_frame_input_snapshot_ready(ui));
    (void)cui_end_frame(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(!cui_frame_input_snapshot_ready(ui));
    (void)cui_end_frame(ui);
    good(ui);
    assert(!cui_begin_frame(ui, NAN, 100, 0));
    cui_clear_error(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(!cui_frame_input_snapshot_ready(ui));
    (void)cui_end_frame(ui);
    good(ui);
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(!cui_layout_scroll_clamp_v1(ui, &(cui_scroll_clamp_v1){.struct_size = 1}, NULL));
    cui_clear_error(ui);
    assert(!cui_frame_input_snapshot_ready(ui));
    assert(!cui_end_frame(ui).count && cui_last_error(ui) == CUI_ERROR_LAYOUT);
    cui_clear_error(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    good(ui);
    assert(!ui->numeric);
}
static void v2(cui_context *ui) {
    cui_frame_v2_options o = options(8);
    assert(cui_begin_frame_v2(ui, 200, 100, 0, &o));
    assert(cui_frame_input_snapshot_ready(ui));
    cui_begin_box(ui, 10, (cui_box){.width = 80, .height = 30});
    (void)cui_interact(ui, 10, CUI_ROLE_BUTTON, (cui_str){0}, false, false, 0);
    cui_background brush = {.kind = CUI_BACKGROUND_SOLID, .solid = {1, 0, 0, 1}};
    assert(cui_set_background(ui, &brush));
    cui_end_box(ui);
    cui_draw_list_v2 list;
    assert(cui_end_frame_v2(ui, &list) && list.count && list.commands[0].kind == CUI_DRAW_V2_BACKGROUND);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    for (unsigned failure = 0; failure < 3; ++failure) {
        good(ui);
        cui_frame_v2_options bad = o;
        if (failure == 0)
            assert(!cui_begin_frame_v2(ui, NAN, 100, 0, &bad));
        else if (failure == 1) {
            bad.size = 1;
            assert(!cui_begin_frame_v2(ui, 200, 100, 0, &bad));
        } else {
            bad.command_bytes = 1;
            assert(!cui_begin_frame_v2(ui, 200, 100, 0, &bad));
        }
        cui_clear_error(ui);
        assert(!cui_accessibility_numeric_snapshot_ready(ui));
        assert(cui_begin_frame_v2(ui, 200, 100, 0, &o));
        assert(!cui_frame_input_snapshot_ready(ui));
        assert(cui_end_frame_v2(ui, &list));
    }
    good(ui);
    assert(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_FOCUS));
    assert(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    /* Real V2 buffer growth fails after the old list expires; input stays queued. */
    expired_commands = ui->v2->commands;
    o = options(16);
    fail_bytes = 16 * sizeof(cui_draw_command_v2);
    assert(!cui_begin_frame_v2(ui, 200, 100, 0, &o));
    assert(!fail_bytes && expired_frees == 1);
    expired_commands = NULL; /* Expired list/buffer must never be dereferenced. */
    assert(ui->queued_count == 3 && !ui->in_frame);
    cui_clear_error(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    assert(cui_begin_frame_v2(ui, 200, 100, 0, &o));
    assert(!cui_frame_input_snapshot_ready(ui));
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    assert(count == 3 && events[0].type == CUI_EVENT_ACCESSIBILITY_ACTION &&
           events[1].accessibility_action == CUI_ACCESSIBILITY_ACTIVATE &&
           events[2].key == CUI_KEY_RIGHT && cui_focused(ui) == 10);
    assert(cui_interact(ui, 10, CUI_ROLE_BUTTON, (cui_str){0}, false, false, 0).activated);
    cui_begin_box(ui, 10, (cui_box){.width = 80, .height = 30});
    cui_end_box(ui);
    assert(cui_end_frame_v2(ui, &list));
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    assert(cui_begin_frame_v2(ui, 200, 100, 0, &o));
    assert(cui_frame_input_snapshot_ready(ui));
    assert(cui_end_frame_v2(ui, &list));
    good(ui);
    assert(cui_begin_frame_v2(ui, 200, 100, 0, &o));
    assert(cui_frame_input_snapshot_ready(ui));
    cui_begin_box(ui, 50, (cui_box){.width = 50, .height = 30, .clip = true});
    cui_begin_box(ui, 51, (cui_box){.width = 50, .height = 100});
    cui_end_box(ui);
    cui_end_box(ui);
    float offset = 0;
    cui_scroll_clamp_v1 plan = {sizeof(plan), 1, CUI_SCROLL_CLAMP_Y, 0, 50, 51};
    assert(cui_layout_scroll_clamp_v1(ui, &plan, &offset));
    offset = NAN; /* Actual late constraint failure in matching V2 end. */
    assert(!cui_end_frame_v2(ui, &list) && !list.count && list.error == CUI_ERROR_LAYOUT);
    cui_clear_error(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    assert(cui_begin_frame_v2(ui, 200, 100, 0, &o));
    assert(!cui_frame_input_snapshot_ready(ui));
    assert(cui_end_frame_v2(ui, &list));
    assert(cui_begin_frame_v2(ui, 200, 100, 0, &o));
    assert(cui_frame_input_snapshot_ready(ui));
    (void)cui_end_frame(ui); /* Wrong-end lifecycle does not close V2 frame. */
    assert(ui->in_frame && !cui_frame_input_snapshot_ready(ui));
    cui_clear_error(ui);
    assert(!cui_frame_input_snapshot_ready(ui));
    assert(cui_end_frame_v2(ui, &list));
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    good(ui);
    assert(!ui->numeric);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100,
                                              .max_elements = 32, .max_commands = 32});
    assert(ui);
    lifecycle(ui);
    v2(ui);
    printf("context=%zu numeric_block=%zu observer_extra_context=0 numeric_fixed=%zu N32=%zu\n",
           sizeof(cui_context), offsetof(cui_context, scroll_layout) - offsetof(cui_context, numeric),
           sizeof(cui_core_numeric_state), sizeof(cui_core_numeric_state) + 64 * sizeof(uint32_t));
    cui_destroy(ui);
    return 0;
}
