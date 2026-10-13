/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>

static bool scroll_hit, overlay_hit;
static cui_event_type event_type;
static void scene(cui_context *ui, bool barrier) {
    assert(cui_begin_frame(ui, 200, 160, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 160});
    assert(cui_begin_focus_scope(ui, 100, false));
    cui_begin_absolute_box(ui, 11, (cui_box){0}, (cui_rect){10, 10, 150, 120});
    cui_interact(ui, 11, CUI_ROLE_GROUP, cui_string("Scroll owner"), false, false, 0);
    cui_set_semantic_details(ui, 11,
                             (cui_semantic_details){.scrollable = true, .scroll_max_y = 100});
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    assert(cui_begin_focus_scope(ui, 200, false));
    cui_begin_absolute_box(ui, 20, (cui_box){0}, (cui_rect){20, 20, 80, 40});
    cui_interact(ui, 20, CUI_ROLE_BUTTON, cui_string("Disabled overlay"), true, false, 0);
    cui_set_semantic_details(ui, 20, (cui_semantic_details){.blocks_pointer_activation = barrier});
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    scroll_hit = cui_event_hit_within_scope(ui, 0, 100);
    overlay_hit = cui_event_hit_within_scope(ui, 0, 200);
    size_t count = 0;
    const cui_event *events = cui_frame_events(ui, &count);
    event_type = count ? events[0].type : CUI_EVENT_POINTER_ENTER;
    cui_end_box(ui);
    cui_end_frame(ui);
}
int main(void) {
    cui_context *touch = cui_create((cui_config){0});
    assert(touch);
    scene(touch, true);
    scene(touch, true);
    assert(cui_queue_event(touch, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                              .x = 40,
                                              .y = 50,
                                              .pointer_type = CUI_POINTER_TOUCH,
                                              .pointer_id = 7}));
    scene(touch, true);
    assert(overlay_hit && !scroll_hit);
    assert(cui_queue_event(touch, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                              .x = 40,
                                              .y = 30,
                                              .pointer_type = CUI_POINTER_TOUCH,
                                              .pointer_id = 7}));
    scene(touch, true);
    assert(event_type == CUI_EVENT_SCROLL && cui_event_target(touch, 0) == 11);
    assert(scroll_hit && !overlay_hit);
    assert(cui_queue_event(touch, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                              .x = 40,
                                              .y = 30,
                                              .pointer_type = CUI_POINTER_TOUCH,
                                              .pointer_id = 7}));
    scene(touch, true);
    assert(event_type == CUI_EVENT_SCROLL && cui_event_activation(touch, 0) == 0);
    assert(scroll_hit && !overlay_hit);
    cui_destroy(touch);
    for (unsigned barrier = 0; barrier < 2; barrier++) {
        cui_context *ui = cui_create((cui_config){0});
        assert(ui);
        scene(ui, barrier);
        scene(ui, barrier);
        assert(cui_queue_event(
            ui,
            (cui_event){
                .type = CUI_EVENT_SCROLL, .x = 40, .y = 40, .dy = -20, .scroll_precise = true}));
        scene(ui, barrier);
        assert(cui_event_target(ui, 0) == 11);
        assert(scroll_hit && !overlay_hit);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 40, .y = 40}));
        scene(ui, barrier);
        assert(scroll_hit == !barrier);
        assert(overlay_hit == (bool)barrier);
        cui_destroy(ui);
    }
}
