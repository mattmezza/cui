/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static cui_button_click_event copied[32];
static unsigned legacy_count, event_count, group_count, only_count;
static cui_button_click_event only_event;
static cui_context *active_context;
static bool consume_in_legacy;
static cui_measure_fn fixture_measure;
static void *fixture_measure_user;
static void legacy(void *user) {
    assert(user == copied);
    ++legacy_count;
    if (consume_in_legacy) {
        size_t n;
        const cui_event *events = cui_frame_events(active_context, &n);
        for (size_t i = 0; i < n; i++)
            if (events[i].type == CUI_EVENT_ACCESSIBILITY_ACTION)
                assert(cui_consume_event(active_context, i));
    }
}
static void clicked(void *user, const cui_button_click_event *event) {
    assert(user == copied);
    assert(legacy_count == event_count + 1);
    assert(event_count < 32);
    copied[event_count++] = *event;
}
static void event_only(void *user, const cui_button_click_event *event) {
    (void)user;
    assert(event->target == 10);
    only_event = *event;
    ++only_count;
}
static void changed(void *user, const cui_button_group_request *request) {
    (void)user;
    assert(request->selected_count == 1);
    ++group_count;
}
static void frame(cui_context *ui, cui_button_options options, bool inert, bool layout) {
    cui_theme t = cui_theme_light();
    assert(cui_begin_frame(ui, 1000, 500, 0));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 800, .height = 400, .inert = inert, .layout_only = layout});
    cui_button_ex(ui, 10, &t, cui_string("Click"), options);
    cui_end_box(ui);
    cui_end_frame(ui);
}
static void pointer_for(cui_context *ui, cui_id id, cui_event_type type, cui_pointer_type kind,
                        unsigned count) {
    cui_rect b = cui_bounds(ui, id);
    assert(cui_queue_event(ui, (cui_event){.type = type,
                                           .x = b.x + b.w / 2,
                                           .y = b.y + b.h / 2,
                                           .pointer_type = kind,
                                           .pointer_id = kind == CUI_POINTER_MOUSE ? 0 : 71,
                                           .pointer_device = kind == CUI_POINTER_MOUSE ? 0 : 8,
                                           .click_count = count,
                                           .modifiers = CUI_MOD_SHIFT}));
}
static void pointer(cui_context *ui, cui_event_type type, cui_pointer_type kind, unsigned count) {
    pointer_for(ui, 10, type, kind, count);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 512,
                                              .frame_bytes = 65536,
                                              .measure = fixture_measure,
                                              .measure_user = fixture_measure_user});
    assert(ui);
    active_context = ui;
    cui_button_options options = {
        .on_click = legacy, .on_click_event = clicked, .action_user = copied};
    frame(ui, options, false, false);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 2);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 2);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 3);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 3);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .modifiers = CUI_MOD_CTRL}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    frame(ui, options, false, false);
    assert(event_count == 4 && legacy_count == 4);
    const size_t indices[] = {1, 3, 4, 5};
    for (unsigned i = 0; i < 4; i++) {
        assert(copied[i].event_index == indices[i] && copied[i].target == 10);
        assert(copied[i].frame_sequence && copied[i].frame_sequence == copied[0].frame_sequence);
        assert(!copied[i].repeat);
    }
    assert(copied[0].origin == CUI_BUTTON_CLICK_POINTER && copied[0].click_count == 2);
    assert(copied[1].click_count == 3 && copied[1].modifiers == CUI_MOD_SHIFT);
    assert(copied[2].origin == CUI_BUTTON_CLICK_KEYBOARD && copied[2].key == CUI_KEY_ENTER);
    assert(copied[2].modifiers == CUI_MOD_CTRL && !copied[2].has_position &&
           !copied[2].click_count);
    cui_button_click_event saved;
    memcpy(&saved, &copied[0], sizeof saved);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_TOUCH, 0);
    frame(ui, options, false, false);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_TOUCH, 0);
    frame(ui, options, false, false);
    assert(event_count == 5 && copied[4].pointer_type == CUI_POINTER_TOUCH);
    assert(copied[4].pointer_id == 71 && copied[4].pointer_device == 8 &&
           copied[4].click_count == 1);
    assert(!memcmp(&saved, &copied[0], sizeof saved));
    for (unsigned mode = 0; mode < 4; mode++) {
        frame(ui, options, false, false);
        pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 1);
        pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 1);
        options.disabled = mode == 0;
        options.loading = mode == 1;
        frame(ui, options, mode == 2, mode == 3);
        assert(event_count == 5 && legacy_count == 5);
        options.disabled = options.loading = false;
    }
    frame(ui, options, false, false);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 1);
    pointer(ui, CUI_EVENT_CANCEL, CUI_POINTER_MOUSE, 1);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 1);
    frame(ui, options, false, false);
    assert(event_count == 5);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 10,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    frame(ui, options, false, false);
    assert(event_count == 6 && copied[5].origin == CUI_BUTTON_CLICK_ACCESSIBILITY);
    cui_button_group_style_workspace *w = calloc(1, sizeof *w);
    assert(w);
    cui_button_group_styled_item item = {.id = 10, .label = {"Click", 5}, .options = options};
    bool selected = false;
    cui_theme t = cui_theme_light();
    for (unsigned override = 0; override < 2; override++) {
        for (unsigned phase = 0; phase < 2; phase++) {
            if (phase) {
                pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 1);
                pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 1);
            }
            assert(cui_begin_frame(ui, 1000, 500, 0));
            cui_button_group_result result;
            assert(cui_button_group_ex(ui, 100, &t, &item, &selected, 1, w,
                                       (cui_button_group_style_options){
                                           .behavior = {.on_change = override ? changed : NULL}},
                                       &result));
            cui_end_frame(ui);
            if (phase)
                assert(result.callbacks == (override ? 1 : 2));
        }
    }
    assert(event_count == 7 && legacy_count == 7 && group_count == 1);
    /* Cooperative consumption during legacy notification cannot erase the
     * already accepted payload notification for this event. */
    assert(cui_begin_frame(ui, 1000, 500, 0));
    cui_button_group_result result;
    assert(cui_button_group_ex(ui, 100, &t, &item, &selected, 1, w,
                               (cui_button_group_style_options){0}, &result));
    cui_end_frame(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = 10,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    consume_in_legacy = true;
    assert(cui_begin_frame(ui, 1000, 500, 0));
    assert(cui_button_group_ex(ui, 100, &t, &item, &selected, 1, w,
                               (cui_button_group_style_options){0}, &result));
    cui_end_frame(ui);
    consume_in_legacy = false;
    assert(event_count == 8 && legacy_count == 8 &&
           copied[7].origin == CUI_BUTTON_CLICK_ACCESSIBILITY);
    options.on_click = NULL;
    options.on_click_event = event_only;
    frame(ui, options, false, false);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 1);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 1);
    frame(ui, options, false, false);
    assert(only_count == 1 && legacy_count == 8);
    pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_PEN, 2);
    pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_PEN, 2);
    frame(ui, options, false, false);
    assert(only_count == 2 && only_event.pointer_type == CUI_POINTER_PEN &&
           only_event.click_count == 2);
    cui_button_group_styled_item pair[2] = {
        item, {.id = 11, .label = {"Second", 6}, .options = item.options}};
    bool pair_selected[2] = {false, false};
    assert(cui_begin_frame(ui, 1000, 500, 0));
    assert(cui_button_group_ex(ui, 100, &t, pair, pair_selected, 2, w,
                               (cui_button_group_style_options){0}, &result));
    cui_end_frame(ui);
    const cui_id order[] = {11, 10, 11};
    unsigned start = event_count;
    for (unsigned i = 0; i < 3; i++) {
        pointer_for(ui, order[i], CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 1);
        pointer_for(ui, order[i], CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 1);
    }
    assert(cui_begin_frame(ui, 1000, 500, 0));
    assert(cui_button_group_ex(ui, 100, &t, pair, pair_selected, 2, w,
                               (cui_button_group_style_options){0}, &result));
    cui_end_frame(ui);
    assert(result.callbacks == 6 && event_count == start + 3);
    for (unsigned i = 0; i < 3; i++) {
        assert(copied[start + i].target == order[i] && copied[start + i].event_index == i * 2 + 1);
        assert(copied[start + i].frame_sequence == copied[start].frame_sequence);
    }
    for (unsigned mode = 0; mode < 4; mode++) {
        assert(cui_begin_frame(ui, 1000, 500, 0));
        pair[0].options.loading = false;
        assert(cui_button_group_ex(ui, 100, &t, pair, pair_selected, 2, w,
                                   (cui_button_group_style_options){0}, &result));
        cui_end_frame(ui);
        pointer(ui, CUI_EVENT_POINTER_DOWN, CUI_POINTER_MOUSE, 1);
        pointer(ui, CUI_EVENT_POINTER_UP, CUI_POINTER_MOUSE, 1);
        pair[0].options.loading = mode == 1;
        assert(cui_begin_frame(ui, 1000, 500, 0));
        assert(cui_button_group_ex(
            ui, 100, &t, pair, pair_selected, 2, w,
            (cui_button_group_style_options){.behavior = {.disabled = mode == 0},
                                             .box = {.inert = mode == 2, .layout_only = mode == 3}},
            &result));
        cui_end_frame(ui);
        assert(result.callbacks == 0 && event_count == start + 3);
    }
    free(w);
    cui_destroy(ui);
    return 0;
}
