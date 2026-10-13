/* SPDX-License-Identifier: MIT */
#define probe_frame inherited_probe_frame
#define probe_init inherited_probe_init
#define probe_event inherited_probe_event
#define probe_key inherited_probe_key
#include "button_blocker_probe.c"
#undef probe_frame
#undef probe_init
#undef probe_event
#undef probe_key
static cui_button_click_event events_copied[32];
static unsigned callbacks, order_errors;
static bool selected;
static cui_button_group_style_workspace group_workspace;
static void clicked_event(void *user, const cui_button_click_event *event) {
    (void)user;
    if (children != callbacks + 1)
        ++order_errors;
    if (callbacks < 32)
        events_copied[callbacks] = *event;
    ++callbacks;
}
static void changed(void *user, const cui_button_group_request *request) {
    (void)user;
    (void)request;
    ++parents;
}
int probe_init(unsigned state) {
    int result = inherited_probe_init(state);
    callbacks = order_errors = 0;
    selected = false;
    group_workspace = (cui_button_group_style_workspace){0};
    return result;
}
int probe_event(int type, float x, float y, unsigned id, int touch, float dy, unsigned modifiers,
                unsigned count) {
    return cui_queue_event(
        ui, (cui_event){.type = (cui_event_type)type,
                        .x = x,
                        .y = y,
                        .dy = dy,
                        .pointer_id = id,
                        .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                        .modifiers = modifiers,
                        .click_count = count});
}
int probe_key(int key, int up, int repeat, unsigned modifiers) {
    return cui_queue_event(ui, (cui_event){.type = up ? CUI_EVENT_KEY_UP : CUI_EVENT_KEY_DOWN,
                                           .key = (cui_key)key,
                                           .repeat = repeat != 0,
                                           .modifiers = modifiers});
}
double click_stat(unsigned index, unsigned field) {
    if (field == 0)
        return callbacks;
    if (field == 1)
        return order_errors;
    if (index >= callbacks || index >= 32)
        return -1;
    const cui_button_click_event *e = &events_copied[index];
    switch (field) {
    case 2:
        return e->origin;
    case 3:
        return e->target;
    case 4:
        return e->frame_sequence;
    case 5:
        return e->event_index;
    case 6:
        return e->modifiers;
    case 7:
        return e->click_count;
    case 8:
        return e->pointer_type;
    case 9:
        return e->pointer_id;
    case 10:
        return e->key;
    case 11:
        return e->has_position;
    case 12:
        return e->x;
    case 13:
        return e->y;
    default:
        return -1;
    }
}
int probe_frame(void) {
    cui_theme t = cui_theme_light();
    if (!cui_begin_frame(ui, 360, 300, .016f))
        return 1;
    cui_begin_box(ui, 1,
                  (cui_box){.width = 360,
                            .height = 300,
                            .padding = 10,
                            .background = {245, 245, 245, 255},
                            .inert = mode == 5,
                            .layout_only = mode == 6});
    cui_button_options o = {.pixel_size = 80,
                            .disabled = mode == 1,
                            .loading = mode == 2,
                            .on_click = child_click,
                            .on_click_event = clicked_event,
                            .action_user = &children};
    if (mode == 3 || mode == 4) {
        cui_button_group_styled_item item = {.id = 20, .label = {"Click", 5}, .options = o};
        cui_button_group_result result;
        if (!cui_button_group_ex(ui, 100, &t, &item, &selected, 1, &group_workspace,
                                 (cui_button_group_style_options){
                                     .behavior = {.on_change = mode == 4 ? changed : NULL}},
                                 &result))
            return 2;
    } else
        cui_button_ex(ui, 20, &t, cui_string("Click"), o);
    cui_end_box(ui);
    draw = cui_end_frame(ui);
    return 0;
}
