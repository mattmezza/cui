/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <math.h>
static cui_context *ui;
static cui_draw_list draw;
static cui_prepaint_info snapshot;
static unsigned frames, callbacks, wheel_events;
static float scroll;
static bool overlay, modal;
static bool paint(const cui_prepaint_info *info, const void *payload, cui_prepaint_writer *writer) {
    (void)payload;
    snapshot = *info;
    callbacks++;
    cui_draw_command command = {.type = CUI_DRAW_RECT,
                                .bounds = {info->bounds.x - 20, info->bounds.y - 20, 160, 80},
                                .color = info->pointer_visible ? (cui_color){0, 180, 0, 255}
                                                               : (cui_color){0, 0, 180, 255}};
    if (!cui_prepaint_emit(writer, &command, false))
        return false;
    if (info->pointer_visible) {
        command.bounds = (cui_rect){150, 20, 40, 20};
        command.color = (cui_color){240, 200, 0, 255};
        if (!cui_prepaint_emit(writer, &command, true))
            return false;
    }
    return true;
}
int probe_init(void) {
    ui = cui_create((cui_config){.max_elements = 32, .max_commands = 128, .frame_bytes = 4096});
    return ui ? 0 : 1;
}
int probe_event(int wheel, float x, float y, float delta) {
    return cui_queue_event(ui,
                           (cui_event){.type = wheel ? CUI_EVENT_SCROLL : CUI_EVENT_POINTER_MOVE,
                                       .x = x,
                                       .y = y,
                                       .dy = -delta,
                                       .scroll_precise = true});
}
void probe_options(int passively_occluded, int modal_occluded) {
    overlay = passively_occluded != 0;
    modal = modal_occluded != 0;
}
int probe_frame(float width, float height) {
    if (!cui_begin_frame(ui, width, height, .016f))
        return 1;
    size_t count = 0;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (events[i].type == CUI_EVENT_SCROLL && cui_event_target(ui, i) == 10) {
            scroll = fminf(100, fmaxf(0, scroll - events[i].dy));
            wheel_events++;
            cui_consume_event(ui, i);
        }
    cui_begin_box(
        ui, 1,
        (cui_box){
            .width = width, .height = height, .background = {255, 255, 255, 255}, .padding = 10});
    cui_begin_box(
        ui, 10,
        (cui_box){
            .width = width / 2, .height = 80, .clip = true, .background = {220, 220, 220, 255}});
    cui_interact(ui, 10, CUI_ROLE_GROUP, cui_string("Scroll viewport"), false, false, 0);
    cui_set_semantic_details(
        ui, 10,
        (cui_semantic_details){.scrollable = true, .scroll_y = scroll, .scroll_max_y = 100});
    cui_begin_absolute_box(ui, 20, (cui_box){0}, (cui_rect){0, 10 - scroll, 120, 40});
    cui_interact(ui, 20, CUI_ROLE_IMAGE, cui_string("Current paint owner"), false, false, 0);
    if (!cui_set_prepaint(ui, (cui_prepaint_options){.callback = paint,
                                                     .command_budget = 6,
                                                     .hover_hitbox = true,
                                                     .deferred_layer = 5}))
        return 2;
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_box(ui);
    if (overlay) {
        cui_begin_portal(ui, 30, (cui_box){.background = {200, 0, 0, 255}},
                         (cui_rect){20, 20, 40, 40}, 10);
        cui_set_hover_hitbox(ui, true);
        cui_end_box(ui);
    }
    if (modal) {
        cui_begin_focus_scope(ui, 40, true);
        cui_begin_portal(ui, 41, (cui_box){.background = {180, 0, 180, 255}},
                         (cui_rect){180, 100, 40, 40}, 20);
        cui_set_hover_hitbox(ui, true);
        cui_end_box(ui);
        cui_end_focus_scope(ui);
    }
    draw = cui_end_frame(ui);
    frames++;
    return cui_last_error(ui);
}
double probe_stat(unsigned field) {
    switch (field) {
    case 0:
        return frames;
    case 1:
        return callbacks;
    case 2:
        return snapshot.pointer_valid;
    case 3:
        return snapshot.pointer_visible;
    case 4:
        return snapshot.pointer_hit;
    case 5:
        return snapshot.bounds.y;
    case 6:
        return snapshot.visible_bounds.w;
    case 7:
        return snapshot.modal_allowed;
    case 8:
        return wheel_events;
    case 9:
        return scroll;
    default:
        return -1;
    }
}
unsigned probe_count(void) {
    return (unsigned)draw.count;
}
double probe_command(unsigned index, unsigned field) {
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
