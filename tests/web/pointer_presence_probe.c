/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
static cui_context *ui;
static cui_prepaint_info info;
static cui_draw_list draw;
static unsigned frames, calls, activations, cancels, enters, leaves;
static bool paint(const cui_prepaint_info *current, const void *payload,
                  cui_prepaint_writer *writer) {
    (void)payload;
    info = *current;
    calls++;
    cui_draw_command command = {.type = CUI_DRAW_RECT,
                                .bounds = current->bounds,
                                .color = current->pointer_visible ? (cui_color){0, 180, 0, 255}
                                                                  : (cui_color){0, 0, 180, 255}};
    if (!cui_prepaint_emit(writer, &command, false))
        return false;
    if (current->pointer_visible) {
        command.bounds = (cui_rect){150, 20, 40, 20};
        command.color = (cui_color){240, 200, 0, 255};
        return cui_prepaint_emit(writer, &command, true);
    }
    return true;
}
int presence_init(void) {
    ui = cui_create((cui_config){.max_elements = 32, .max_commands = 128, .frame_bytes = 4096});
    return ui ? 0 : 1;
}
int presence_event(int type, float x, float y) {
    return cui_queue_event(ui, (cui_event){.type = (cui_event_type)type, .x = x, .y = y});
}
int presence_frame(void) {
    if (!cui_begin_frame(ui, 240, 160, .016f))
        return 1;
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count; i++) {
        cancels += events[i].type == CUI_EVENT_CANCEL;
        enters += events[i].type == CUI_EVENT_POINTER_ENTER;
        leaves += events[i].type == CUI_EVENT_POINTER_LEAVE;
    }
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 240, .height = 160, .padding = 20, .background = {255, 255, 255, 255}});
    cui_begin_box(ui, 20, (cui_box){.width = 80, .height = 40});
    cui_interaction interaction =
        cui_interact(ui, 20, CUI_ROLE_BUTTON, cui_string("Presence action"), false, false, 0);
    activations += interaction.activated;
    cui_set_cursor(ui, 20, CUI_CURSOR_CROSSHAIR);
    if (!cui_set_prepaint(ui, (cui_prepaint_options){.callback = paint,
                                                     .command_budget = 6,
                                                     .hover_hitbox = true,
                                                     .deferred_layer = 5}))
        return 2;
    cui_end_box(ui);
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    frames++;
    return cui_last_error(ui);
}
double presence_stat(unsigned field) {
    switch (field) {
    case 0:
        return frames;
    case 1:
        return calls;
    case 2:
        return info.pointer_present;
    case 3:
        return info.pointer_valid;
    case 4:
        return info.pointer_visible;
    case 5:
        return info.pointer_x;
    case 6:
        return info.pointer_y;
    case 7:
        return info.capture;
    case 8:
        return cui_focused(ui);
    case 9:
        return cui_hovered(ui, 20);
    case 10:
        return cui_pointer_cursor(ui);
    case 11:
        return activations;
    case 12:
        return cancels;
    case 13:
        return enters;
    case 14:
        return leaves;
    default:
        return -1;
    }
}
unsigned presence_count(void) {
    return (unsigned)draw.count;
}
double presence_command(unsigned index, unsigned field) {
    if (index >= draw.count)
        return -1;
    const cui_draw_command *c = &draw.commands[index];
    switch (field) {
    case 0:
        return c->type;
    case 1:
        return c->bounds.x;
    case 2:
        return c->bounds.y;
    case 3:
        return c->bounds.w;
    case 4:
        return c->bounds.h;
    case 5:
        return c->color.r;
    case 6:
        return c->color.g;
    case 7:
        return c->color.b;
    case 8:
        return c->color.a;
    default:
        return -1;
    }
}
