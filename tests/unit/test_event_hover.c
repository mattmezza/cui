/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <math.h>
#include <string.h>
bool cui_event_hovered(cui_context *, size_t, cui_id);
bool event_hover_nonfinite(cui_context *, size_t, cui_id);
#ifdef CUI_EVENT_HOVER_TOPOLOGY_WITNESS
bool event_hover_topology(const cui_context *, cui_id, cui_id, uint32_t[2], int16_t[2], bool *);
#endif
/* Public submission only: real prior clipped geometry and interactive portals. */
static void tree(cui_context *ui, unsigned overlay, bool disabled, bool modal) {
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    assert(cui_begin_focus_scope(ui, 100, false));
    cui_begin_box(ui, 2, (cui_box){.width = 60, .height = 40, .clip = true});
    cui_begin_box(ui, 10, (cui_box){.width = 100, .height = 40});
    (void)cui_interact(ui, 10, CUI_ROLE_GROUP, cui_string("Table"), disabled, false, 0);
    cui_end_box(ui);
    cui_end_box(ui);
    if (overlay) {
        if (modal)
            assert(cui_begin_focus_scope(ui, 200, true));
        cui_begin_portal(ui, 20, (cui_box){.width = 20, .height = 40}, (cui_rect){20, 0, 20, 40},
                         1);
        (void)cui_interact(ui, 20, CUI_ROLE_GROUP, cui_string("Overlay"), overlay == 3, false, 0);
        cui_set_semantic_details(ui, 20,
                                 (cui_semantic_details){.pointer_activatable = overlay == 1,
                                                        .blocks_pointer_activation = overlay >= 2});
        cui_end_box(ui);
        if (modal)
            cui_end_focus_scope(ui);
    }
    cui_end_focus_scope(ui);
    cui_end_box(ui);
}
static void run(unsigned overlay, bool disabled, bool modal) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    assert(ui);
    assert(cui_begin_frame(ui, 200, 100, .016f));
    tree(ui, overlay, disabled, modal);
    (void)cui_end_frame(ui);
    const float x[] = {10, 30, 80, 190};
    for (size_t i = 0; i < 4; ++i)
        assert(cui_queue_event(
            ui, (cui_event){.type = i == 0   ? CUI_EVENT_POINTER_DOWN
                                    : i == 2 ? CUI_EVENT_POINTER_UP
                                             : CUI_EVENT_POINTER_MOVE,
                            .pointer_type = i == 3 ? CUI_POINTER_MOUSE : CUI_POINTER_TOUCH,
                            .pointer_id = i == 3 ? 0 : 7,
                            .pointer_device = i == 3 ? 0 : 9,
                            .x = x[i],
                            .y = 10}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(cui_begin_focus_scope(ui, 100, false));
    if (modal) {
        /* The prior modal is active now too; Table remains outside that scope. */
        assert(cui_begin_focus_scope(ui, 200, true));
    }
    assert(cui_event_hovered(ui, 0, 10) == (!disabled && !modal));
    assert(cui_event_hovered(ui, 1, 10) == (!disabled && !modal && !overlay));
    assert(!cui_event_hovered(ui, 2, 10)); /* layout bounds extend beyond ancestor clip */
    assert(!cui_event_hovered(ui, 3, 10)); /* later mouse position cannot alter earlier samples */
    assert(!cui_event_hovered(ui, 4, 10));
    assert(!cui_event_hovered(ui, 100, 10));
    assert(!cui_event_hovered(ui, 0, 999));
    assert(event_hover_nonfinite(ui, 0, 10));
    assert(!cui_event_hovered(ui, 0, 0));
    assert(!cui_event_hovered(NULL, 0, 10));
    cui_begin_box(ui, 30, (cui_box){.width = 10, .height = 10, .inert = true});
    assert(!cui_event_hovered(ui, 0, 10));
    cui_end_box(ui);
    cui_begin_box(ui, 31, (cui_box){.width = 10, .height = 10, .layout_only = true});
    assert(!cui_event_hovered(ui, 0, 10));
    cui_end_box(ui);
    if (modal)
        cui_end_focus_scope(ui);
    cui_end_focus_scope(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(!cui_event_hovered(ui, 0, 10));
    cui_destroy(ui);
}
static void current_modal_transition(bool scoped, bool retained_modal) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    assert(ui && cui_begin_frame(ui, 200, 100, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    if (scoped)
        assert(cui_begin_focus_scope(ui, 100, false));
    if (retained_modal)
        assert(cui_begin_focus_scope(ui, 200, true));
    cui_begin_box(ui, 10, (cui_box){.width = 50, .height = 30});
    (void)cui_interact(ui, 10, CUI_ROLE_GROUP, cui_string("Prior target"), false, false, 0);
    cui_end_box(ui);
    if (retained_modal)
        cui_end_focus_scope(ui);
    if (scoped)
        cui_end_focus_scope(ui);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 10, .y = 10}));
    assert(cui_begin_frame(ui, 200, 100, .016f));
    cui_begin_box(ui, 2, (cui_box){.width = 200, .height = 100});
    if (scoped)
        assert(cui_begin_focus_scope(ui, 100, false));
    assert(cui_event_hovered(ui, 0, 10) == !retained_modal);
    assert(cui_begin_focus_scope(ui, 200, true));
    /* Current modal has disjoint paint geometry: ancestry must deny the prior
     * background target independently of the query caller's current stack. */
    cui_begin_portal(ui, 20, (cui_box){.width = 20, .height = 20}, (cui_rect){150, 60, 20, 20}, 1);
    (void)cui_interact(ui, 20, CUI_ROLE_BUTTON, cui_string("New modal"), false, false, 0);
    assert(cui_event_hovered(ui, 0, 10) == retained_modal);
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    assert(cui_event_hovered(ui, 0, 10) == retained_modal);
    if (scoped)
        cui_end_focus_scope(ui);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void overlap_leaf(cui_context *ui, unsigned mode) {
    if (mode == 3)
        cui_begin_portal(ui, 20, (cui_box){.width = 20, .height = 40}, (cui_rect){20, 0, 20, 40},
                         1);
    else if (mode == 4)
        cui_begin_absolute_box(ui, 20, (cui_box){0}, (cui_rect){20, 0, 20, 40});
    else
        cui_begin_box(ui, 20, (cui_box){.width = 40, .height = 40});
    (void)cui_interact(ui, 20, CUI_ROLE_GROUP, cui_string("Overlap"), false, false, 0);
    cui_set_semantic_details(ui, 20, (cui_semantic_details){.pointer_activatable = mode != 2});
    cui_end_box(ui);
}
static void same_root(unsigned mode) {
    /* 0 descendant;1 unrelated interactive sibling;2 passive sibling;
     * 3 higher-layer popup descendant;4 absolute higher-root descendant. */
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 100});
    assert(ui && cui_begin_frame(ui, 200, 100, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    assert(cui_begin_focus_scope(ui, 100, false));
    /* A zero-height ordinary wrapper permits real un-clipped child overflow.
     * The following ordinary sibling therefore overlaps in the SAME paint root;
     * absolute boxes would create a new paint root and test a different rule. */
    cui_begin_box(ui, 9, (cui_box){.width = 100, .height = 0, .fixed_height = true});
    cui_begin_box(ui, 10, (cui_box){.width = 100, .height = 40});
    (void)cui_interact(ui, 10, CUI_ROLE_GROUP, cui_string("Table"), false, false, 0);
    if (mode == 0 || mode == 3 || mode == 4)
        overlap_leaf(ui, mode);
    cui_end_box(ui);
    cui_end_box(ui); /* ordinary zero-height wrapper */
    if (mode == 1 || mode == 2)
        overlap_leaf(ui, mode);
    cui_end_focus_scope(ui);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    cui_rect target = cui_bounds(ui, 10), overlap = cui_bounds(ui, 20);
    assert(target.w == 100 && target.h == 40 && target.x <= 25 && target.y <= 10);
    assert(overlap.x <= 25 && overlap.x + overlap.w > 25 && overlap.y <= 10 &&
           overlap.y + overlap.h > 10);
    if (mode < 3)
        assert(cui_semantic_parent(ui, 20) == (mode == 0 ? 10 : 0));
#ifdef CUI_EVENT_HOVER_TOPOLOGY_WITNESS
    uint32_t roots[2];
    int16_t layers[2];
    bool descendant;
    assert(event_hover_topology(ui, 10, 20, roots, layers, &descendant));
    if (mode < 3) {
        assert(roots[0] == roots[1] && layers[0] == layers[1]);
        assert(descendant == (mode == 0));
    } else {
        assert(roots[1] > roots[0]);
        assert(mode == 3 ? layers[1] > layers[0] : layers[1] == layers[0]);
        assert(descendant == (mode == 4));
    }
#endif
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 25, .y = 10}));
    assert(cui_begin_frame(ui, 200, 100, .016f));
    assert(cui_begin_focus_scope(ui, 100, false));
    assert(cui_event_hovered(ui, 0, 10) == (mode == 0 || mode == 2));
    cui_end_focus_scope(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--current-modal-root")) {
        current_modal_transition(false, false);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--current-modal-scope")) {
        current_modal_transition(true, false);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--retained-modal-root")) {
        current_modal_transition(false, true);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--retained-modal-scope")) {
        current_modal_transition(true, true);
        return 0;
    }
    const char *overlap_modes[] = {"--overlap-descendant", "--overlap-sibling", "--overlap-passive",
                                   "--overlap-popup", "--overlap-absolute"};
    _Static_assert(sizeof overlap_modes / sizeof overlap_modes[0] == 5, "five overlap modes");
    const size_t overlap_count = sizeof overlap_modes / sizeof overlap_modes[0];
    if (argc == 2)
        for (unsigned i = 0; i < overlap_count; ++i)
            if (!strcmp(argv[1], overlap_modes[i])) {
                same_root(i);
                return 0;
            }
    if (argc == 2 && !strcmp(argv[1], "--same-root")) {
        for (unsigned i = 0; i < overlap_count; ++i)
            same_root(i);
        return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--current-modal")) {
        current_modal_transition(false, false);
        current_modal_transition(true, false);
        current_modal_transition(false, true);
        current_modal_transition(true, true);
        return 0;
    }
    for (unsigned i = 0; i < overlap_count; ++i)
        same_root(i);

    current_modal_transition(false, false);
    current_modal_transition(true, false);
    current_modal_transition(false, true);
    current_modal_transition(true, true);
    run(0, false, false);
    run(0, true, false);
    run(1, false, false); /* passive role + pointer activation, not focusability */
    run(2, false, false); /* explicit barrier */
    run(3, false, false); /* disabled barrier still blocks pointer activation */
    run(1, false, true);
    return 0;
}
