/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>

static cui_interaction frame(cui_context *ui, bool suppress, bool disabled) {
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(ui, 10, (cui_box){.width = 300, .height = 200, .gap = 10});
    cui_begin_box(ui, 1, (cui_box){.width = 100, .height = 30});
    cui_interact(ui, 1, CUI_ROLE_TEXTBOX, cui_string("Draft"), false, false, 0);
    cui_end_box(ui);
    cui_begin_box(ui, 2, (cui_box){.width = 100, .height = 30});
    cui_interaction result =
        cui_interact(ui, 2, CUI_ROLE_BUTTON, cui_string("Save"), disabled, false, 0);
    cui_set_semantic_details(
        ui, 2, (cui_semantic_details){.pointer_activatable = true, .no_pointer_focus = suppress});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    return result;
}
static void pointer(cui_context *ui, cui_event_type type, cui_pointer_type device) {
    cui_rect box = cui_bounds(ui, 2);
    assert(box.w > 0 && box.h > 0);
    assert(cui_queue_event(ui, (cui_event){.type = type,
                                           .x = box.x + box.w / 2,
                                           .y = box.y + box.h / 2,
                                           .pointer_type = device,
                                           .pointer_id = 7}));
}
int main(void) {
    cui_context *ui = cui_create((cui_config){
        .width = 300, .height = 200, .max_elements = 32, .max_commands = 64, .frame_bytes = 4096});
    assert(ui);
    frame(ui, true, false);
    cui_focus(ui, 1);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE);
    assert(frame(ui, true, false).pressed);
    assert(cui_focused(ui) == 1);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE);
    assert(frame(ui, true, false).activated);
    assert(cui_focused(ui) == 1);
    assert(!frame(ui, true, false).activated);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    assert(frame(ui, true, false).focused);
    assert(cui_focused(ui) == 2);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(frame(ui, true, false).activated);
    cui_focus(ui, 2);
    assert(frame(ui, true, false).focused);
    cui_focus(ui, 1);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_TOUCH);
    assert(frame(ui, true, false).pressed);
    assert(cui_focused(ui) == 1);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_TOUCH);
    assert(frame(ui, true, false).activated);
    assert(cui_focused(ui) == 1);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE);
    assert(frame(ui, true, false).pressed);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    assert(!frame(ui, true, false).activated);
    assert(!frame(ui, true, false).pressed);
    frame(ui, true, true);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE);
    assert(!frame(ui, true, true).activated);
    assert(cui_focused(ui) != 2);
    frame(ui, false, false);
    cui_focus(ui, 1);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE);
    assert(frame(ui, false, false).pressed);
    assert(cui_focused(ui) == 2);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE);
    assert(frame(ui, false, false).activated);
    cui_destroy(ui);
    return 0;
}
