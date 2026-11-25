/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>

static void begin(cui_context *ui) {
    assert(cui_begin_frame(ui, 200, 100, 0));
    cui_begin_box(ui, 1000, (cui_box){.width = 200, .height = 100});
}

static void end(cui_context *ui) {
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}

static void move(cui_context *ui, float x) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x, .y = 10}));
}

static void control(cui_context *ui, cui_id id, bool disabled) {
    (void)cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("Test control"), disabled, false, 0);
}

static void row(cui_context *ui, float width, float clip, bool disabled) {
    cui_begin_box(ui, 1, (cui_box){.width = clip, .height = 30, .clip = true});
    cui_begin_box(ui, 2, (cui_box){.width = width, .height = 30});
    control(ui, 2, disabled);
    cui_end_box(ui);
    cui_end_box(ui);
}

static void portal(cui_context *ui, cui_id id, int16_t layer, bool disabled) {
    cui_begin_portal(ui, id, (cui_box){.width = 20, .height = 20}, (cui_rect){5, 0, 20, 20},
                     layer);
    control(ui, id, disabled);
    cui_end_box(ui);
}

static void snapshot_changes(cui_context *ui) {
    move(ui, 40);
    begin(ui);
    assert(!cui_hovered(ui, 2));
    row(ui, 80, 20, true);
    end(ui);
    assert(!cui_hovered(ui, 2)); /* clipped at x=20 */

    move(ui, 10);
    begin(ui);
    assert(cui_hovered(ui, 2)); /* direct query permits disabled controls */
    row(ui, 80, 20, true);
    assert(!cui_interaction_state(ui, 2).hovered);
    cui_begin_box(ui, 3, (cui_box){.width = 50, .height = 30, .inert = true});
    assert(!cui_hovered(ui, 2));
    cui_end_box(ui);
    assert(cui_hovered(ui, 2));
    portal(ui, 4, 2, true);
    end(ui);
    assert(cui_hovered(ui, 2) && cui_hovered(ui, 4)); /* disabled overlay does not occlude */

    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_LEAVE}));
    begin(ui);
    assert(!cui_hovered(ui, 2));
    row(ui, 80, 80, false);
    portal(ui, 4, 2, false);
    end(ui);
    assert(!cui_hovered(ui, 4));

    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_ENTER}));
    begin(ui);
    assert(!cui_hovered(ui, 2) && cui_hovered(ui, 4)); /* same x/y, new snapshot */
    row(ui, 80, 80, false);
    end(ui);
    assert(cui_hovered(ui, 2) && !cui_hovered(ui, 4));

    move(ui, 40);
    begin(ui);
    assert(cui_hovered(ui, 2));
    assert(cui_event_hovered(ui, 0, 2));
    assert(cui_begin_focus_scope(ui, 100, true));
    assert(cui_hovered(ui, 2)); /* legacy hover uses the previous modal */
    assert(!cui_event_hovered(ui, 0, 2));
    portal(ui, 5, 2, false);
    cui_end_focus_scope(ui);
    row(ui, 80, 80, false);
    end(ui);
    assert(!cui_hovered(ui, 2)); /* background behind the now-published modal */
    begin(ui);
    end(ui);
    assert(!cui_hovered(ui, 5));
}

static void overlapping(cui_context *ui) {
    move(ui, 10);
    begin(ui);
    row(ui, 80, 80, true);
    /* More intersecting semantics than a small fixed cache, including the
       only focusable overlay at the end. All portals share the same layer. */
    for (cui_id id = 10; id < 81; ++id)
        portal(ui, id, 1, true);
    portal(ui, 81, 1, false);
    portal(ui, 82, 1, true);
    end(ui);
    assert(!cui_hovered(ui, 2) && !cui_hovered(ui, 79) && !cui_hovered(ui, 80));
    assert(cui_hovered(ui, 81) && cui_hovered(ui, 82) && !cui_hovered(ui, 83));
    move(ui, 40);
    begin(ui);
    assert(cui_hovered(ui, 2) && !cui_hovered(ui, 82));
    end(ui);
    assert(!cui_hovered(ui, 2));
}

static void shared_root(cui_context *ui) {
    move(ui, 10);
    begin(ui);
    cui_begin_box(ui, 2, (cui_box){.width = 80, .height = 30});
    control(ui, 2, false);
    cui_begin_box(ui, 3, (cui_box){.width = 60, .height = 20});
    control(ui, 3, false);
    cui_end_box(ui);
    cui_end_box(ui);
    portal(ui, 4, 1, false);
    cui_set_semantic_details(ui, 4, (cui_semantic_details){.blocks_pointer_activation = true,
                                                         .no_focus = true});
    end(ui);
    assert(cui_hovered(ui, 2) && cui_hovered(ui, 3) && cui_hovered(ui, 4));
}

int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_commands = 128});
    assert(ui);
    snapshot_changes(ui);
    overlapping(ui);
    shared_root(ui);
    cui_destroy(ui);
    return 0;
}
