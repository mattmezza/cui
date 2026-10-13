/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
static cui_prepaint_info info;
static cui_interaction interaction;
static unsigned activations;
static bool paint(const cui_prepaint_info *current, const void *payload,
                  cui_prepaint_writer *writer) {
    (void)payload;
    info = *current;
    cui_draw_command command = {.type = CUI_DRAW_RECT,
                                .bounds = current->bounds,
                                .color = current->pointer_visible ? (cui_color){0, 180, 0, 255}
                                                                  : (cui_color){0, 0, 180, 255}};
    return cui_prepaint_emit(writer, &command, false);
}
static void scene(cui_context *ui, bool overlay, bool modal) {
    assert(cui_begin_frame(ui, 200, 120, .016f));
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (events[i].type == CUI_EVENT_POINTER_ENTER ||
            events[i].type == CUI_EVENT_POINTER_LEAVE) {
            assert(cui_event_consumed(ui, i) && cui_event_target(ui, i) == 0 &&
                   cui_event_activation(ui, i) == 0);
            assert(events[i].x == 0 && events[i].y == 0 && events[i].pointer_id == 0);
        }
    assert(cui_begin_focus_scope(ui, 10, false));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 120});
    cui_begin_box(ui, 20, (cui_box){.width = 80, .height = 40});
    interaction = cui_interact(ui, 20, CUI_ROLE_BUTTON, cui_string("Action"), false, false, 0);
    activations += interaction.activated;
    cui_set_cursor(ui, 20, CUI_CURSOR_CROSSHAIR);
    assert(cui_set_prepaint(
        ui, (cui_prepaint_options){.callback = paint, .command_budget = 3, .hover_hitbox = true}));
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_focus_scope(ui);
    if (overlay) {
        cui_begin_portal(ui, 30, (cui_box){0}, (cui_rect){0, 0, 80, 40}, 10);
        assert(cui_set_hover_hitbox(ui, true));
        cui_end_box(ui);
    }
    if (modal) {
        assert(cui_begin_focus_scope(ui, 40, true));
        cui_begin_portal(ui, 41, (cui_box){0}, (cui_rect){150, 80, 40, 20}, 20);
        cui_end_box(ui);
        cui_end_focus_scope(ui);
    }
    assert(cui_end_frame(ui).count > 0);
    assert(cui_last_error(ui) == CUI_OK);
}
static void event(cui_context *ui, cui_event_type type, float x, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = type, .x = x, .y = y, .pointer_id = 99}));
}
int main(void) {
    cui_context *ui =
        cui_create((cui_config){.max_elements = 32, .max_commands = 128, .frame_bytes = 4096});
    assert(ui);
    scene(ui, false, false);
    assert(!info.pointer_present && !info.pointer_valid && !interaction.hovered &&
           !cui_hovered(ui, 20) && !cui_hover_within(ui, 10));
    assert(cui_pointer_cursor(ui) == CUI_CURSOR_DEFAULT);
    event(ui, CUI_EVENT_POINTER_ENTER, NAN, NAN);
    scene(ui, false, false);
    assert(info.pointer_present && !info.pointer_valid && !info.pointer_inside &&
           !info.pointer_visible && info.pointer_hit == 0);
    assert(!interaction.hovered && !cui_hovered(ui, 20) && !cui_hover_within(ui, 10) &&
           cui_pointer_cursor(ui) == CUI_CURSOR_DEFAULT);
    event(ui, CUI_EVENT_POINTER_MOVE, 15, 15);
    scene(ui, false, false);
    assert(info.pointer_present && info.pointer_valid && info.pointer_visible &&
           info.pointer_hit == 20);
    assert(interaction.hovered && cui_hovered(ui, 20) && cui_hover_within(ui, 10) &&
           cui_pointer_cursor(ui) == CUI_CURSOR_CROSSHAIR);
    event(ui, CUI_EVENT_POINTER_DOWN, 15, 15);
    event(ui, CUI_EVENT_POINTER_LEAVE, 999, 999);
    scene(ui, false, false);
    assert(!info.pointer_present && !info.pointer_valid && !info.pointer_inside &&
           !info.pointer_visible && info.pointer_hit == 0);
    assert(info.pointer_x == 15 && info.pointer_y == 15 && info.capture == 20 &&
           cui_pressed(ui, 20) && cui_focused(ui) == 20);
    assert(!interaction.hovered && !cui_hovered(ui, 20) && !cui_hover_within(ui, 10) &&
           cui_pointer_cursor(ui) == CUI_CURSOR_CROSSHAIR);
    event(ui, CUI_EVENT_POINTER_ENTER, 0, 0);
    scene(ui, false, false);
    assert(info.pointer_valid && info.pointer_visible && info.capture == 20 &&
           info.pointer_x == 15 && cui_focused(ui) == 20);
    event(ui, CUI_EVENT_CANCEL, 0, 0);
    scene(ui, false, false);
    assert(info.pointer_present && info.pointer_valid && info.pointer_visible &&
           !cui_pressed(ui, 20) && info.capture == 0 && cui_focused(ui) == 20);
    event(ui, CUI_EVENT_POINTER_DOWN, 15, 15);
    event(ui, CUI_EVENT_CANCEL, 0, 0);
    event(ui, CUI_EVENT_POINTER_LEAVE, 0, 0);
    scene(ui, false, false);
    assert(!info.pointer_valid && !info.pointer_present && !cui_pressed(ui, 20) &&
           cui_focused(ui) == 20 && cui_pointer_cursor(ui) == CUI_CURSOR_DEFAULT);
    event(ui, CUI_EVENT_POINTER_ENTER, 0, 0);
    scene(ui, true, false);
    assert(info.pointer_valid && !info.pointer_visible && info.pointer_hit == 30);
    scene(ui, false, true);
    assert(info.pointer_valid && !info.modal_allowed && !info.pointer_visible);
    scene(ui, false, false);
    event(ui, CUI_EVENT_POINTER_MOVE, 15, 15);
    event(ui, CUI_EVENT_POINTER_DOWN, 15, 15);
    scene(ui, false, false);
    event(ui, CUI_EVENT_POINTER_MOVE, 220, 15);
    event(ui, CUI_EVENT_POINTER_LEAVE, 0, 0);
    scene(ui, false, false);
    assert(info.capture == 20 && !info.pointer_valid && info.pointer_x == 220 &&
           cui_pressed(ui, 20));
    event(ui, CUI_EVENT_POINTER_UP, 220, 15);
    event(ui, CUI_EVENT_POINTER_LEAVE, 0, 0);
    scene(ui, false, false);
    assert(!info.pointer_valid && !cui_pressed(ui, 20) && activations == 0);
    event(ui, CUI_EVENT_POINTER_ENTER, 0, 0);
    event(ui, CUI_EVENT_POINTER_MOVE, 15, 15);
    event(ui, CUI_EVENT_POINTER_DOWN, 15, 15);
    event(ui, CUI_EVENT_POINTER_UP, 15, 15);
    scene(ui, false, false);
    assert(activations == 1 && info.pointer_valid && !cui_pressed(ui, 20));
    /* Presence at the previous activation's event slot must not replay its metadata. */
    event(ui, CUI_EVENT_POINTER_ENTER, 0, 0);
    event(ui, CUI_EVENT_POINTER_ENTER, 0, 0);
    event(ui, CUI_EVENT_POINTER_ENTER, 0, 0);
    event(ui, CUI_EVENT_POINTER_LEAVE, 0, 0);
    scene(ui, false, false);
    assert(activations == 1 && !info.pointer_valid);
    event(ui, CUI_EVENT_POINTER_LEAVE, 0, 0);
    event(ui, CUI_EVENT_POINTER_MOVE, 15, 15);
    scene(ui, false, false);
    assert(info.pointer_valid &&
           info.pointer_visible); /* Legacy positional input implies presence. */
    event(ui, CUI_EVENT_POINTER_MOVE, 15, 15);
    event(ui, CUI_EVENT_POINTER_LEAVE, 0, 0);
    scene(ui, false, false);
    assert(!info.pointer_valid && !info.pointer_visible && activations == 1);
    /* Surface leave does not retire a live touch contact or cancel its capture. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 9,
                                           .x = 15,
                                           .y = 15}));
    event(ui, CUI_EVENT_POINTER_LEAVE, 0, 0);
    scene(ui, false, false);
    assert(info.capture == 20 && !info.pointer_valid && cui_pressed(ui, 20));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 9,
                                           .x = 15,
                                           .y = 15}));
    scene(ui, false, false);
    assert(info.capture == 0 && activations == 2 && info.pointer_valid);
    cui_destroy(ui);
    puts("pointer presence: ok");
}
