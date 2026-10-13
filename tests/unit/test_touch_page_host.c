/* SPDX-License-Identifier: MIT */
#include "../../web/touch_page_host.h"
#include <assert.h>
#include <stdio.h>

typedef struct {
    cui_role role;
    bool drag, viewport, modal, disabled, scroll, consume, parent;
    float offset;
} scene;
static void frame(cui_context *ui, cui_touch_page_host *host, scene s) {
    assert(cui_begin_frame(ui, 300, 300, .016f));
    if (s.modal)
        assert(cui_begin_focus_scope(ui, 50, true));
    if (s.parent) {
        cui_interact(ui, 99, CUI_ROLE_GROUP, cui_string("Protected parent"), false, false, 0);
        cui_set_touch_drag_priority(ui, 99, true);
        cui_begin_box(ui, 99, (cui_box){.width = 200, .height = 200});
    }
    cui_interact(ui, 10, s.role, cui_string("Control"), s.disabled, false, 0);
    cui_set_semantic_details(ui, 10,
                             (cui_semantic_details){.touch_drag_priority = s.drag,
                                                    .gesture_pan = s.viewport,
                                                    .gesture_zoom = s.viewport,
                                                    .scrollable = s.scroll,
                                                    .focusable = s.role == CUI_ROLE_SEPARATOR ||
                                                                 s.role == CUI_ROLE_SCROLLBAR,
                                                    .scroll_y = s.offset,
                                                    .scroll_max_y = s.scroll ? 100 : 0});
    cui_begin_box(ui, 10, (cui_box){.width = 200, .height = 200});
    cui_end_box(ui);
    if (s.parent)
        cui_end_box(ui);
    if (s.modal)
        cui_end_focus_scope(ui);
    if (s.consume) {
        size_t count;
        const cui_event *events = cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; i++)
            if (events[i].type == CUI_EVENT_POINTER_MOVE ||
                (events[i].type == CUI_EVENT_SCROLL && cui_event_target(ui, i)))
                cui_consume_event(ui, i);
    }
    cui_touch_page_dispatch(host, ui);
    cui_end_frame(ui);
    cui_touch_page_finish(host, ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void touch(cui_context *ui, cui_event_type type, unsigned id, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = type,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = id,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .x = 60,
                                           .y = y}));
}
static void check(scene s, bool available) {
    cui_context *ui = cui_create((cui_config){.width = 300, .height = 300});
    assert(ui);
    cui_touch_page_host host = {0};
    frame(ui, &host, s);
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_DOWN, 1, 100);
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_MOVE, 1, 70);
    frame(ui, &host, s);
    if (cui_touch_page_available(&host, 1) != available)
        fprintf(stderr, "role=%d modal=%d disabled=%d scroll=%d offset=%g expected=%d\n", s.role,
                s.modal, s.disabled, s.scroll, s.offset, available);
    assert(cui_touch_page_available(&host, 1) == available);
    cui_touch_page_end(&host, 1);
    touch(ui, CUI_EVENT_CANCEL, 1, 0);
    frame(ui, &host, s);
    assert(!cui_touch_page_available(&host, 1));
    cui_destroy(ui);
}
int main(void) {
    check((scene){.role = CUI_ROLE_BUTTON}, true);
    check((scene){.role = CUI_ROLE_IMAGE}, true);
    check((scene){.role = CUI_ROLE_SLIDER}, false);
    check((scene){.role = CUI_ROLE_SEPARATOR}, false);
    check((scene){.role = CUI_ROLE_SCROLLBAR}, false);
    check((scene){.role = CUI_ROLE_BUTTON, .drag = true}, false);
    check((scene){.role = CUI_ROLE_BUTTON, .parent = true}, false);
    check((scene){.role = CUI_ROLE_IMAGE, .viewport = true}, false);
    check((scene){.role = CUI_ROLE_BUTTON, .modal = true}, false);
    check((scene){.role = CUI_ROLE_BUTTON, .modal = true, .disabled = true}, false);
    check((scene){.role = CUI_ROLE_BUTTON, .consume = true}, false);
    check((scene){.role = CUI_ROLE_REGION, .scroll = true, .consume = true}, false);
    check((scene){.role = CUI_ROLE_REGION, .scroll = true, .offset = 100}, true);

    cui_context *ui = cui_create((cui_config){.width = 300, .height = 300});
    assert(ui);
    cui_touch_page_host host = {0};
    scene s = {.role = CUI_ROLE_REGION, .scroll = true, .consume = true};
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_DOWN, 1, 100);
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_MOVE, 1, 70);
    frame(ui, &host, s);
    assert(!cui_touch_page_available(&host, 1));
    touch(ui, CUI_EVENT_POINTER_UP, 1, 70);
    frame(ui, &host, s); /* Core converts this terminal event to SCROLL/ENDED. */
    s.offset = 100;
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_DOWN, 2, 100);
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_MOVE, 2, 70);
    frame(ui, &host, s);
    assert(cui_touch_page_available(&host, 2));
    touch(ui, CUI_EVENT_POINTER_UP, 2, 70);
    frame(ui, &host, s);
    s = (scene){.role = CUI_ROLE_IMAGE, .viewport = true};
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_DOWN, 4, 100);
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_MOVE, 4, 70);
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_UP, 4, 70);
    frame(ui, &host, s); /* GESTURE/ENDED also retires the physical contact. */
    s = (scene){.role = CUI_ROLE_BUTTON};
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_DOWN, 1, 100);
    touch(ui, CUI_EVENT_POINTER_DOWN, 2, 110);
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_UP, 2, 110);
    touch(ui, CUI_EVENT_POINTER_MOVE, 1, 60);
    frame(ui, &host, s);
    assert(!cui_touch_page_available(&host, 1));
    touch(ui, CUI_EVENT_POINTER_UP, 1, 60);
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_DOWN, 3, 100);
    frame(ui, &host, s);
    touch(ui, CUI_EVENT_POINTER_MOVE, 3, 60);
    frame(ui, &host, s);
    assert(cui_touch_page_available(&host, 3));
    for (unsigned id = 4; id < 20; id++)
        touch(ui, CUI_EVENT_POINTER_DOWN, id, 100);
    frame(ui, &host, s);
    assert(!cui_touch_page_available(&host, 3));
    cui_destroy(ui);
    return 0;
}
