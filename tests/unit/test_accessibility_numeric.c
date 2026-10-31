/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static const char *test_phase;
#include <string.h>
static cui_context *make(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300, .max_elements = 64});
    assert(ui);
    return ui;
}
static void begin(cui_context *ui) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
}
static void node(cui_context *ui, cui_id id, cui_role role, uint32_t mask, bool disabled,
                 bool readonly, bool layout_only) {
    cui_begin_box(ui, id + 1000, (cui_box){.width = 400, .height = 30});
    cui_begin_absolute_box(ui, id, (cui_box){.layout_only = layout_only},
                           (cui_rect){id == 10 ? 220 : 300, 0, 70, 30});
    (void)cui_interact(ui, id, role, cui_string("Numeric"), disabled, false, .25f);
    cui_set_semantic_details(
        ui, id, (cui_semantic_details){.minimum = 0, .maximum = 1, .readonly = readonly});
    if (role == CUI_ROLE_SLIDER && !layout_only)
        if (!cui_set_accessibility_numeric_actions(ui, id, mask)) {
            fprintf(
                stderr, "registration failed phase=%s id=%llu error=%d disabled=%d readonly=%d\n",
                test_phase, (unsigned long long)id, (int)cui_last_error(ui), disabled, readonly);
            assert(false);
        }
    cui_end_box(ui);
    cui_end_box(ui);
}
static void scene(cui_context *ui) {
    node(ui, 10, CUI_ROLE_SLIDER, 7, false, false, false);
    node(ui, 20, CUI_ROLE_SLIDER, 7, false, false, false);
}
static void finish(cui_context *ui) {
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void set(cui_context *ui, cui_id id, double value) {
    cui_accessibility_set_value_v1 p = {1, 0, value};
    assert(cui_queue_accessibility_set_value_v1(ui, id, sizeof(p), &p));
    p.value = NAN; /* queue owns its copy */
}
static cui_accessibility_numeric_input_v1 claim(cui_context *ui, cui_id id, size_t index) {
    cui_accessibility_numeric_input_v1 p;
    memset(&p, 0xa5, sizeof p);
    assert(cui_claim_accessibility_numeric_input_v1(ui, id, index, sizeof p, &p));
    assert(p.version == 1 && p.target == id && !p.reserved);
    assert(cui_event_consumed(ui, index));
    cui_accessibility_numeric_input_v1 saved = p;
    assert(!cui_claim_accessibility_numeric_input_v1(ui, id, index, sizeof p, &p));
    assert(!memcmp(&p, &saved, sizeof p));
    assert(!cui_event_activation(ui, index));
    cui_activation_snapshot activation;
    assert(!cui_event_activation_snapshot(ui, index, &activation));
    return p;
}
static void normalization(void) {
    cui_context *ui = make();
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    begin(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    scene(ui);
    finish(ui);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    assert(cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_INCREMENT));
    cui_focus(ui, 20);
    set(ui, 10, .123456789012345);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 20,
                                           .accessibility_action = CUI_ACCESSIBILITY_INCREMENT,
                                           .x = 99,
                                           .key = CUI_KEY_RIGHT,
                                           .pointer_id = 999,
                                           .text_data = "junk",
                                           .text_length = 4}));
    assert(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_DECREMENT));
    begin(ui);
    scene(ui);
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    assert(count == 3 && events[1].x == 0 && events[1].key == 0 && !events[1].pointer_id &&
           !events[1].text_data);
    assert(cui_focused(ui) == 20 && !cui_pressed(ui, 10) && !cui_pressed(ui, 20));
    cui_accessibility_numeric_input_v1 a = claim(ui, 10, 0), b = claim(ui, 20, 1),
                                       c = claim(ui, 10, 2);
    assert(a.action == CUI_ACCESSIBILITY_SET_VALUE &&
           a.flags == CUI_ACCESSIBILITY_NUMERIC_HAS_VALUE);
    assert(a.value == .123456789012345);
    assert(b.action == CUI_ACCESSIBILITY_INCREMENT && !b.flags && b.value == 0);
    assert(c.action == CUI_ACCESSIBILITY_DECREMENT && !c.flags && c.value == 0);
    finish(ui);
    cui_destroy(ui);
    assert(a.value == .123456789012345); /* copied beyond context lifetime */
}
static void invalid_and_capacity(void) {
    cui_context *ui = make();
    begin(ui);
    scene(ui);
    finish(ui);
    const cui_accessibility_set_value_v1 bad[] = {
        {0, 0, 0}, {1, 1, 0}, {1, 0, NAN}, {1, 0, INFINITY}};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; i++) {
        assert(!cui_queue_accessibility_set_value_v1(ui, 10, sizeof bad[i], &bad[i]));
        assert(cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
        cui_clear_error(ui);
    }
    cui_accessibility_set_value_v1 good = {1, 0, 0};
    assert(!cui_queue_accessibility_set_value_v1(ui, 10, sizeof good - 1, &good));
    cui_clear_error(ui);
    assert(!cui_queue_accessibility_set_value_v1(ui, 10, sizeof good, NULL));
    cui_clear_error(ui);
    assert(!cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_SET_VALUE));
    cui_clear_error(ui);
    assert(!cui_queue_accessibility_action(ui, 10, (cui_accessibility_action)99));
    cui_clear_error(ui);
    for (unsigned i = 0; i < 256; i++)
        set(ui, 10, (double)i + .125);
    assert(!cui_queue_accessibility_set_value_v1(ui, 10, sizeof good, &good));
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_clear_error(ui);
    begin(ui);
    scene(ui);
    for (unsigned i = 0; i < 256; i++)
        assert(claim(ui, 10, i).value == (double)i + .125);
    finish(ui);
    set(ui, 10, 0);
    begin(ui);
    scene(ui);
    assert(claim(ui, 10, 0).value == 0);
    finish(ui);
    begin(ui);
    scene(ui);
    size_t count;
    cui_frame_events(ui, &count);
    assert(!count);
    finish(ui);
    cui_destroy(ui);
}
static void gating(void) {
    cui_context *ui = make();
    begin(ui);
    scene(ui);
    finish(ui);
    for (unsigned mode = 0; mode < 6; mode++) {
        set(ui, 10, .5);
        begin(ui);
        if (mode != 4)
            node(ui, 10, mode == 3 ? CUI_ROLE_BUTTON : CUI_ROLE_SLIDER, mode == 2 ? 0 : 7,
                 mode == 0, mode == 1, mode == 5);
        cui_accessibility_numeric_input_v1 out, saved;
        memset(&out, 0xa5, sizeof out);
        saved = out;
        assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, 0, sizeof out, &out));
        assert(!memcmp(&out, &saved, sizeof out));
        finish(ui);
        assert(!cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_SET_VALUE));
        assert(!cui_queue_accessibility_set_value_v1(ui, 10, sizeof(cui_accessibility_set_value_v1),
                                                     &(cui_accessibility_set_value_v1){1, 0, .5}));
        cui_clear_error(ui);
        begin(ui);
        scene(ui);
        finish(ui);
    }
    set(ui, 10, .5);
    begin(ui);
    scene(ui);
    cui_accessibility_numeric_input_v1 out, saved;
    memset(&out, 0xa5, sizeof out);
    saved = out;
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 20, 0, sizeof out, &out));
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, 0, sizeof out - 1, &out));
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, SIZE_MAX, sizeof out, &out));
    assert(!memcmp(&out, &saved, sizeof out) && !cui_event_consumed(ui, 0));
    assert(claim(ui, 10, 0).value == .5);
    finish(ui);
    cui_destroy(ui);
}
static void scroll(cui_context *ui) {
    (void)cui_interact(ui, 71, CUI_ROLE_REGION, cui_string("Pan"), false, false, 0);
    cui_set_semantic_details(
        ui, 71, (cui_semantic_details){.focusable = true, .scrollable = true, .scroll_max_y = 200});
    cui_begin_box(ui, 71, (cui_box){.width = 200, .height = 160});
    cui_end_box(ui);
}
static void origin_and_compaction(void) {
    cui_context *ui = make();
    begin(ui);
    scroll(ui);
    scene(ui);
    finish(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 101,
                                           .x = 20,
                                           .y = 80}));
    begin(ui);
    scroll(ui);
    scene(ui);
    finish(ui);
    set(ui, 10, .125);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 101,
                                           .x = 20,
                                           .y = 50}));
    set(ui, 20, .875);
    begin(ui);
    scroll(ui);
    scene(ui);
    size_t count;
    const cui_event *e = cui_frame_events(ui, &count);
    assert(count == 4 && e[1].type == CUI_EVENT_CANCEL && e[2].type == CUI_EVENT_SCROLL);
    assert(claim(ui, 10, 0).value == .125 && claim(ui, 20, 3).value == .875);
    cui_accessibility_numeric_input_v1 out;
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, 1, sizeof out, &out));
    finish(ui);
    /* Filtered layout-only first semantic must not move its mask onto survivor. */
    begin(ui);
    node(ui, 5, CUI_ROLE_SLIDER, 0, false, false, true);
    node(ui, 10, CUI_ROLE_SLIDER, CUI_ACCESSIBILITY_NUMERIC_INCREMENT, false, false, false);
    finish(ui);
    assert(cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_INCREMENT));
    assert(!cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_SET_VALUE));
    assert(!cui_accessibility_action_supported(ui, 5, CUI_ACCESSIBILITY_INCREMENT));
    /* Dismissal compaction preserves survivor's own SET-only mask. */
    begin(ui);
    assert(cui_begin_focus_scope(ui, 90, false));
    node(ui, 10, CUI_ROLE_SLIDER, 1, false, false, false);
    cui_end_focus_scope(ui);
    node(ui, 20, CUI_ROLE_SLIDER, 4, false, false, false);
    assert(cui_dismiss_focus_scope(ui, 90, 0));
    finish(ui);
    assert(!cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_INCREMENT));
    assert(cui_accessibility_action_supported(ui, 20, CUI_ACCESSIBILITY_SET_VALUE));
    assert(!cui_accessibility_action_supported(ui, 20, CUI_ACCESSIBILITY_INCREMENT));
    cui_destroy(ui);
}
static void modal(void) {
    cui_context *ui = make();
    begin(ui);
    scene(ui);
    finish(ui);
    set(ui, 10, .5);
    begin(ui);
    node(ui, 10, CUI_ROLE_SLIDER, 7, false, false, false);
    assert(cui_begin_focus_scope(ui, 90, true));
    node(ui, 20, CUI_ROLE_SLIDER, 7, false, false, false);
    cui_end_focus_scope(ui);
    cui_accessibility_numeric_input_v1 out;
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, 0, sizeof out, &out));
    finish(ui);
    assert(!cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_SET_VALUE));
    assert(cui_accessibility_action_supported(ui, 20, CUI_ACCESSIBILITY_SET_VALUE));
    cui_destroy(ui);
}
static void legacy_and_readiness(void) {
    cui_context *ui = make();
    begin(ui);
    node(ui, 10, CUI_ROLE_SLIDER, 0, false, false, false);
    finish(ui);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    assert(!cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_INCREMENT));
    assert(cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_FOCUS));
    assert(!cui_begin_frame(ui, NAN, 100, 0));
    cui_clear_error(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    begin(ui);
    scene(ui);
    finish(ui);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    set(ui, 10, .5);
    begin(ui);
    scene(ui);
    /* Real layout opt-in failure, and clearing error INSIDE failed frame. */
    assert(!cui_layout_scroll_clamp_v1(ui, &(cui_scroll_clamp_v1){.struct_size = 1}, NULL));
    cui_clear_error(ui);
    cui_accessibility_numeric_input_v1 out;
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, 0, sizeof out, &out));
    assert(cui_end_frame(ui).count == 0 && cui_last_error(ui) != CUI_OK);
    cui_clear_error(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    assert(!cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_SET_VALUE));
    begin(ui);
    scene(ui);
    finish(ui);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    /* A valid plan can fail later at real constraint resolution. Already claimed
     * intent remains copied; failed completed geometry grants no new authority. */
    set(ui, 10, .625);
    begin(ui);
    scene(ui);
    cui_accessibility_numeric_input_v1 copied = claim(ui, 10, 0);
    cui_begin_box(ui, 50, (cui_box){.width = 50, .height = 30, .clip = true});
    cui_begin_box(ui, 51, (cui_box){.width = 50, .height = 100});
    cui_end_box(ui);
    cui_end_box(ui);
    float offset = 0;
    cui_scroll_clamp_v1 plan = {sizeof(plan), 1, CUI_SCROLL_CLAMP_Y, 0, 50, 51};
    assert(cui_layout_scroll_clamp_v1(ui, &plan, &offset));
    offset = NAN;
    assert(cui_end_frame(ui).count == 0 && cui_last_error(ui) == CUI_ERROR_LAYOUT);
    assert(copied.value == .625);
    cui_clear_error(ui);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    begin(ui);
    scene(ui);
    finish(ui);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    /* Generic geometry invalidity is persistent even without scroll opt-in's
     * sticky failure marker; clearing error must not reauthorize this frame. */
    set(ui, 10, .25);
    begin(ui);
    scene(ui);
    cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_LAYOUT});
    cui_clear_error(ui);
    assert(!cui_claim_accessibility_numeric_input_v1(ui, 10, 0, sizeof out, &out));
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(!cui_accessibility_numeric_snapshot_ready(ui));
    assert(!cui_accessibility_action_supported(ui, 10, CUI_ACCESSIBILITY_INCREMENT));
    begin(ui);
    scene(ui);
    finish(ui);
    assert(cui_accessibility_numeric_snapshot_ready(ui));
    /* Old Focus/Activate do not depend on numeric readiness/capabilities. */
    begin(ui);
    node(ui, 10, CUI_ROLE_BUTTON, 0, false, false, false);
    finish(ui);
    assert(cui_queue_accessibility_action(ui, 10, CUI_ACCESSIBILITY_ACTIVATE));
    begin(ui);
    cui_interaction a = cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Run"), false, false, 0);
    assert(a.activated);
    cui_begin_box(ui, 10, (cui_box){.width = 70, .height = 30});
    cui_end_box(ui);
    finish(ui);
    cui_destroy(ui);
}
int main(void) {
    assert(CUI_ACCESSIBILITY_FOCUS == 0 && CUI_ACCESSIBILITY_ACTIVATE == 1);
    test_phase = "normalization";
    normalization();
    test_phase = "invalid_and_capacity";
    invalid_and_capacity();
    test_phase = "gating";
    gating();
    test_phase = "origin_and_compaction";
    origin_and_compaction();
    test_phase = "modal";
    modal();
    test_phase = "legacy_and_readiness";
    legacy_and_readiness();
    return 0;
}
