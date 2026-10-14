/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){text.length * size / 2, size};
}
static void queue(cui_context *ui, cui_event_type type, cui_key key, unsigned modifiers) {
    assert(cui_queue_event(ui, (cui_event){.type = type, .key = key, .modifiers = modifiers}));
}
static bool render(cui_context *ui, bool opt, bool disabled, bool gestures) {
    assert(cui_begin_frame(ui, 200, 100, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    if (gestures) {
        cui_interact(ui, 1, CUI_ROLE_REGION, cui_string("Viewport"), false, false, 0);
        assert(cui_set_gestures(ui, 1, true, false));
    }
    cui_begin_absolute_box(ui, 10, (cui_box){0}, (cui_rect){10, 10, 80, 30});
    cui_interaction result =
        cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Button"), disabled, false, 0);
    cui_semantic_details details = {0};
    details.release_key_activation = opt;
    cui_set_semantic_details(ui, 10, details);
    cui_end_box(ui);
    cui_end_box(ui);
    assert(cui_end_frame(ui).commands);
    return result.activated;
}
static bool scene(cui_context *ui, bool opt, bool disabled) {
    return render(ui, opt, disabled, false);
}
static bool key_frame(cui_context *ui, cui_event_type type, cui_key key, unsigned mods, bool opt) {
    queue(ui, type, key, mods);
    return scene(ui, opt, false);
}
static void guard_frame(cui_context *ui, bool disable_policy, bool no_focus, bool change_role,
                        bool change_focus) {
    queue(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0);
    assert(cui_begin_frame(ui, 200, 100, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    cui_begin_absolute_box(ui, 10, (cui_box){0}, (cui_rect){10, 10, 80, 30});
    assert(cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Button"), false, false, 0).activated);
    cui_set_semantic_details(ui, 10, (cui_semantic_details){.release_key_activation = true});
    assert(cui_event_accepted_activation(ui, 0) == 10);
    cui_activation_snapshot snapshot = {0}, canary, before;
    assert(cui_event_activation_snapshot(ui, 0, &snapshot));
    memset(&canary, 0xa5, sizeof canary);
    before = canary;
    cui_begin_box(ui, 90, (cui_box){.inert = true});
    assert(cui_event_accepted_activation(ui, 0) == 0);
    cui_end_box(ui);
    cui_begin_box(ui, 91, (cui_box){.layout_only = true});
    assert(cui_event_accepted_activation(ui, 0) == 0);
    cui_end_box(ui);
    cui_set_semantic_details(
        ui, 10,
        (cui_semantic_details){.release_key_activation = !disable_policy, .no_focus = no_focus});
    if (change_role)
        assert(cui_set_semantic_role(ui, 10, CUI_ROLE_LINK));
    if (change_focus)
        cui_focus(ui, 0);
    if (disable_policy || no_focus || change_role || change_focus) {
        assert(cui_event_accepted_activation(ui, 0) == 0);
        assert(!cui_event_activation_snapshot(ui, 0, &canary));
        assert(memcmp(&canary, &before, sizeof canary) == 0);
    }
    assert(cui_begin_focus_scope(ui, 92, true));
    assert(cui_event_accepted_activation(ui, 0) == 0);
    cui_end_focus_scope(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    assert(cui_end_frame(ui).commands);
}
static void guard_regressions(void) {
    for (int kind = 0; kind < 5; kind++) {
        cui_context *ui = cui_create((cui_config){.measure = measure});
        assert(ui);
        assert(!scene(ui, true, false));
        cui_focus(ui, 10);
        assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
        guard_frame(ui, kind == 0, kind == 1, kind == 2, kind == 4);
        cui_destroy(ui);
    }
    cui_context *ui = cui_create((cui_config){.measure = measure});
    assert(ui);
    assert(!scene(ui, true, false));
    cui_focus(ui, 10);
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    assert(!scene(ui, false, false));
    assert(!scene(ui, true, false));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    assert(!scene(ui, false, false));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_LEFT, 0, false));
    assert(!scene(ui, true, false));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    cui_destroy(ui);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    assert(ui);
    assert(!scene(ui, true, false));
    cui_focus(ui, 10);
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    assert(key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    assert(!scene(ui, true, false));
    assert(key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    assert(key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_SPACE, 0, true)); /* source mixed-key quirk */
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_SPACE, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_TAB, CUI_MOD_CTRL, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_SPACE, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, CUI_MOD_SHIFT, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_SPACE, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ESCAPE, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_SPACE, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    cui_focus(ui, 0);
    cui_focus(ui, 10);
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true)); /* ABA */
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_ENTER, .ime_composing = true}));
    assert(!scene(ui, true, false));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP,
                                           .key = CUI_KEY_ENTER,
                                           .prefer_character_input = true}));
    assert(!scene(ui, true, false));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    assert(!scene(ui, true, false));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    assert(!scene(ui, true, true));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    cui_focus(ui, 10);
    assert(!scene(ui, true, false));
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    queue(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0);
    assert(cui_begin_frame(ui, 200, 100, 0));
    assert(cui_event_activation(ui, 0) == 10);
    assert(cui_consume_event(ui, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    cui_interaction result =
        cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Button"), false, false, 0);
    assert(!result.activated);
    cui_semantic_details details = {.release_key_activation = true};
    cui_set_semantic_details(ui, 10, details);
    cui_end_box(ui);
    assert(cui_end_frame(ui).commands);
    cui_focus(ui, 10);
    assert(!key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, true));
    assert(cui_begin_frame(ui, 200, 100, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    cui_begin_absolute_box(ui, 10, (cui_box){0}, (cui_rect){10, 10, 80, 30});
    assert(!cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Button"), false, false, 0).activated);
    cui_set_semantic_details(
        ui, 10, (cui_semantic_details){.no_focus = true, .release_key_activation = true});
    cui_end_box(ui);
    cui_end_box(ui);
    assert(cui_end_frame(ui).commands);
    assert(cui_focused(ui) == 0);
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, true));
    /* Zero configuration keeps legacy timing. */
    cui_focus(ui, 10);
    assert(key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, false) == false);
    assert(key_frame(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0, false));
    assert(!key_frame(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0, false));
    /* A synthetic touch cancellation shifts dispatch indices before a key
     * down/up pair. Consuming the shifted down must cancel its release slot. */
    assert(!render(ui, true, false, true));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_button = CUI_POINTER_PRIMARY,
                                           .pointer_id = 77,
                                           .pointer_device = 88,
                                           .x = 20,
                                           .y = 20}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 77,
                                           .pointer_device = 88,
                                           .x = 70,
                                           .y = 20}));
    queue(ui, CUI_EVENT_KEY_DOWN, CUI_KEY_ENTER, 0);
    queue(ui, CUI_EVENT_KEY_UP, CUI_KEY_ENTER, 0);
    assert(cui_begin_frame(ui, 200, 100, 0));
    size_t count, down_index = SIZE_MAX, up_index = SIZE_MAX;
    const cui_event *events = cui_frame_events(ui, &count);
    bool saw_cancel = false;
    for (size_t i = 0; i < count; i++) {
        saw_cancel |= events[i].type == CUI_EVENT_CANCEL;
        if (events[i].type == CUI_EVENT_KEY_DOWN)
            down_index = i;
        if (events[i].type == CUI_EVENT_KEY_UP)
            up_index = i;
    }
    assert(saw_cancel && down_index != SIZE_MAX && up_index != SIZE_MAX);
    assert(cui_event_activation(ui, up_index) == 10);
    assert(cui_consume_event(ui, down_index));
    assert(cui_event_activation(ui, up_index) == 0);
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    cui_interact(ui, 1, CUI_ROLE_REGION, cui_string("Viewport"), false, false, 0);
    assert(cui_set_gestures(ui, 1, true, false));
    cui_begin_absolute_box(ui, 10, (cui_box){0}, (cui_rect){10, 10, 80, 30});
    assert(!cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Button"), false, false, 0).activated);
    cui_set_semantic_details(ui, 10, (cui_semantic_details){.release_key_activation = true});
    cui_end_box(ui);
    cui_end_box(ui);
    assert(cui_end_frame(ui).commands);
    cui_destroy(ui);
    guard_regressions();
    puts("release key activation ok");
}
