/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static cui_context *observed;
static size_t observed_index;
static void absent(cui_context *ui, size_t index) {
    cui_activation_snapshot value, before;
    memset(&value, 0xa5, sizeof value);
    before = value;
    assert(!cui_event_activation_snapshot(ui, index, &value));
    assert(memcmp(&value, &before, sizeof value) == 0);
}
static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    absent(observed, observed_index);
    return (cui_size){text.length * size / 2, size};
}
static bool paint(const cui_prepaint_info *info, const void *payload, cui_prepaint_writer *writer) {
    (void)info;
    (void)payload;
    (void)writer;
    absent(observed, observed_index);
    return true;
}
static cui_activation_snapshot snapshot(cui_context *ui, size_t index) {
    cui_activation_snapshot out;
    assert(cui_event_activation_snapshot(ui, index, &out));
    assert(out.target == 10 && out.event_index == index);
    assert(out.frame_sequence == cui_frame_sequence(ui));
    return out;
}
static void scene(cui_context *ui, float x, bool disabled, bool gestures) {
    assert(cui_begin_frame(ui, 200, 100, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    if (gestures) {
        cui_interact(ui, 1, CUI_ROLE_REGION, cui_string("Viewport"), false, false, 0);
        assert(cui_set_gestures(ui, 1, true, false));
    }
    cui_begin_absolute_box(ui, 10, (cui_box){0}, (cui_rect){x, 10, 80, 30});
    cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Ten"), disabled, false, 0);
}
static void begin(cui_context *ui, float x, bool disabled) {
    scene(ui, x, disabled, false);
}
static void end(cui_context *ui) {
    cui_end_box(ui);
    cui_end_box(ui);
    assert(cui_end_frame(ui).commands != NULL);
    absent(ui, observed_index);
}
static void queue(cui_context *ui, cui_event e) {
    assert(cui_queue_event(ui, e));
}
static cui_event pointer(cui_event_type type, float x, cui_pointer_type kind, uint64_t id,
                         uint64_t device) {
    return (cui_event){.type = type,
                       .x = x,
                       .y = 20,
                       .pointer_type = kind,
                       .pointer_id = id,
                       .pointer_device = device};
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.measure = measure});
    assert(ui);
    observed = ui;
    absent(NULL, 0);
    absent(ui, 0);
    begin(ui, 10, false);
    end(ui);
    cui_event down = pointer(CUI_EVENT_POINTER_DOWN, 25, CUI_POINTER_MOUSE, 7, 9);
    down.modifiers = CUI_MOD_CTRL;
    down.click_count = 0;
    queue(ui, down);
    cui_event up = pointer(CUI_EVENT_POINTER_UP, 31, CUI_POINTER_MOUSE, 7, 9);
    up.modifiers = CUI_MOD_SHIFT;
    up.click_count = 3;
    queue(ui, up);
    down.x = 40;
    down.click_count = 2;
    queue(ui, down);
    up.x = 48;
    queue(ui, up);
    begin(ui, 10, false);
    cui_activation_snapshot first = snapshot(ui, 1), second = snapshot(ui, 3);
    assert(first.has_press && first.has_release && !first.has_key_bounds);
    assert(first.press.x == 25 && first.release.x == 31);
    assert(first.press.modifiers == CUI_MOD_CTRL && first.release.modifiers == CUI_MOD_SHIFT);
    assert(first.press.click_count == 0 && first.release.click_count == 3);
    assert(first.press.contact == 7 && first.press.device == 9);
    assert(second.press.x == 40 && second.release.x == 48 && second.press.click_count == 2);
    absent(ui, 0);
    absent(ui, 4);
    assert(!cui_event_activation_snapshot(ui, 1, NULL));
    observed_index = 1;
    cui_measure_text(ui, cui_string("phase guard"), 14);
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = paint, .command_budget = 1}));
    cui_begin_box(ui, 20, (cui_box){.inert = true});
    absent(ui, 1);
    cui_end_box(ui);
    cui_begin_box(ui, 21, (cui_box){.layout_only = true});
    absent(ui, 1);
    cui_end_box(ui);
    assert(snapshot(ui, 1).press.x == 25);
    end(ui);
    /* Press survives frames and leave/enter; raw contact/device metadata stays copied. */
    queue(ui, pointer(CUI_EVENT_POINTER_DOWN, 35, CUI_POINTER_PEN, 11, 12));
    begin(ui, 10, false);
    end(ui);
    queue(ui, (cui_event){.type = CUI_EVENT_POINTER_LEAVE});
    queue(ui, (cui_event){.type = CUI_EVENT_POINTER_ENTER});
    queue(ui, pointer(CUI_EVENT_POINTER_UP, 39, CUI_POINTER_PEN, 11, 12));
    begin(ui, 10, false);
    cui_activation_snapshot pen = snapshot(ui, 2);
    assert(pen.has_press && pen.press.x == 35 && pen.release.x == 39);
    assert(pen.press.type == CUI_POINTER_PEN && pen.press.contact == 11 && pen.press.device == 12);
    end(ui);
    /* Preserve legacy mismatched-release acceptance without fabricating its press. */
    queue(ui, pointer(CUI_EVENT_POINTER_DOWN, 35, CUI_POINTER_MOUSE, 7, 9));
    queue(ui, pointer(CUI_EVENT_POINTER_UP, 36, CUI_POINTER_MOUSE, 8, 9));
    begin(ui, 10, false);
    assert(!snapshot(ui, 1).has_press && snapshot(ui, 1).has_release);
    end(ui);
    queue(ui, pointer(CUI_EVENT_POINTER_DOWN, 35, CUI_POINTER_MOUSE, 7, 9));
    queue(ui, (cui_event){.type = CUI_EVENT_CANCEL});
    queue(ui, pointer(CUI_EVENT_POINTER_UP, 36, CUI_POINTER_MOUSE, 7, 9));
    begin(ui, 10, false);
    absent(ui, 2);
    end(ui);
    /* Bounds come from rendered routing, even if this composition moves the widget. */
    queue(ui,
          (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .modifiers = CUI_MOD_ALT});
    begin(ui, 100, false);
    cui_activation_snapshot key = snapshot(ui, 0);
    assert(key.has_key_bounds && !key.has_press && !key.has_release);
    assert(key.key_bounds.x == 10 && key.key_bounds.y == 10 && key.key_bounds.w == 80);
    assert(key.key == CUI_KEY_ENTER && key.modifiers == CUI_MOD_ALT);
    assert(cui_begin_focus_scope(ui, 200, true));
    cui_interact(ui, 201, CUI_ROLE_BUTTON, cui_string("Modal"), false, false, 0);
    absent(ui, 0);
    cui_end_focus_scope(ui);
    end(ui);
    begin(ui, 100, false);
    end(ui);
    queue(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                          .accessibility_target = 10,
                          .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE});
    begin(ui, 100, false);
    cui_activation_snapshot accessibility = snapshot(ui, 0);
    assert(!accessibility.has_press && !accessibility.has_release && !accessibility.has_key_bounds);
    assert(accessibility.event_type == CUI_EVENT_ACCESSIBILITY_ACTION);
    assert(cui_consume_event(ui, 0));
    absent(ui, 0);
    end(ui);
    /* Current disabled state rejects evidence; a retained copy remains independent. */
    queue(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER});
    begin(ui, 100, true);
    absent(ui, 0);
    end(ui);
    /* Synthetic touch cancellation reorders event indices, not immutable slot evidence. */
    scene(ui, 10, false, true);
    end(ui);
    queue(ui, pointer(CUI_EVENT_POINTER_DOWN, 25, CUI_POINTER_TOUCH, 33, 44));
    queue(ui, pointer(CUI_EVENT_POINTER_MOVE, 45, CUI_POINTER_TOUCH, 33, 44));
    queue(ui, pointer(CUI_EVENT_POINTER_DOWN, 29, CUI_POINTER_MOUSE, 7, 9));
    queue(ui, pointer(CUI_EVENT_POINTER_UP, 32, CUI_POINTER_MOUSE, 7, 9));
    scene(ui, 10, false, true);
    size_t count, release_index = SIZE_MAX;
    bool cancellation = false;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count; ++i) {
        cancellation |= events[i].type == CUI_EVENT_CANCEL;
        if (events[i].type == CUI_EVENT_POINTER_UP && events[i].pointer_type == CUI_POINTER_MOUSE)
            release_index = i;
        else
            absent(ui, i);
    }
    assert(cancellation && release_index != SIZE_MAX && release_index != 3);
    cui_activation_snapshot reordered = snapshot(ui, release_index);
    assert(reordered.press.x == 29 && reordered.release.x == 32);
    assert(reordered.press.type == CUI_POINTER_MOUSE && reordered.press.contact == 7);
    end(ui);
    cui_destroy(ui);
    assert(first.press.x == 25 && first.release.x == 31 && key.key_bounds.x == 10);
    printf("snapshot %zu bytes, pointer sample %zu bytes; copied evidence and phase guards PASS\n",
           sizeof(cui_activation_snapshot), sizeof(cui_pointer_activation_sample));
}
