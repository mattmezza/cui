/* SPDX-License-Identifier: MIT */
#include "cui/cui.h"
#include <assert.h>
static unsigned parent_clicks, child_clicks, later_clicks, modal_clicks;
static cui_prepaint_info paint_info;
static bool hit_barrier;
typedef struct {
    bool blocker, enabled_blocker, clipped, later, elevated, modal, prepaint;
} options;
static bool paint(const cui_prepaint_info *info, const void *payload, cui_prepaint_writer *writer) {
    (void)payload;
    (void)writer;
    paint_info = *info;
    return true;
}
static void scene(cui_context *ui, options o) {
    assert(cui_begin_frame(ui, 320, 240, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 320, .height = 240});
    cui_begin_absolute_box(ui, 90, (cui_box){0}, (cui_rect){220, 160, 80, 24});
    cui_interact(ui, 90, CUI_ROLE_TEXTBOX, cui_string("Editor"), false, false, 0);
    cui_end_box(ui);
    cui_begin_absolute_box(ui, 10, (cui_box){0}, (cui_rect){10, 10, 180, 120});
    parent_clicks +=
        cui_interact(ui, 10, CUI_ROLE_BUTTON, cui_string("Parent"), false, false, 0).activated;
    assert(cui_set_cursor(ui, 10, CUI_CURSOR_GRAB));
    if (o.prepaint)
        assert(cui_set_prepaint(
            ui,
            (cui_prepaint_options){.callback = paint, .command_budget = 1, .hover_hitbox = true}));
    assert(cui_begin_focus_scope(ui, 55, false));
    if (o.clipped)
        cui_begin_absolute_box(ui, 21, (cui_box){.clip = true}, (cui_rect){0, 0, 10, 10});
    if (o.elevated)
        cui_begin_portal(ui, 20, (cui_box){0}, (cui_rect){30, 30, 80, 40}, 10);
    else
        cui_begin_absolute_box(ui, 20, (cui_box){0}, (cui_rect){20, 20, 80, 40});
    child_clicks +=
        cui_interact(ui, 20, CUI_ROLE_BUTTON, cui_string("Barrier"), !o.enabled_blocker, false, 0)
            .activated;
    cui_set_semantic_details(ui, 20,
                             (cui_semantic_details){.blocks_pointer_activation = o.blocker});
    cui_end_box(ui);
    if (o.clipped)
        cui_end_box(ui);
    cui_end_focus_scope(ui);
    cui_end_box(ui);
    if (o.later) {
        cui_begin_absolute_box(ui, 30, (cui_box){0}, (cui_rect){30, 30, 80, 40});
        later_clicks +=
            cui_interact(ui, 30, CUI_ROLE_BUTTON, cui_string("Later"), false, false, 0).activated;
        cui_end_box(ui);
    }
    cui_end_box(ui);
    if (o.modal) {
        assert(cui_begin_focus_scope(ui, 99, true));
        cui_begin_portal(ui, 96, (cui_box){0}, (cui_rect){30, 30, 80, 40}, 20);
        modal_clicks +=
            cui_interact(ui, 96, CUI_ROLE_BUTTON, cui_string("Modal"), false, false, 0).activated;
        cui_end_box(ui);
        cui_end_focus_scope(ui);
    }
    hit_barrier = cui_event_hit_within_scope(ui, 0, 55);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void event(cui_context *ui, cui_event_type type, float x, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = type, .x = x, .y = y}));
}
static void click(cui_context *ui, float x, float y, options o) {
    event(ui, CUI_EVENT_POINTER_DOWN, x, y);
    event(ui, CUI_EVENT_POINTER_UP, x, y);
    scene(ui, o);
}
static void reset(void) {
    parent_clicks = child_clicks = later_clicks = modal_clicks = 0;
}
int main(void) {
    const options cases[] = {{0},
                             {.blocker = true},
                             {.blocker = true, .enabled_blocker = true},
                             {.blocker = true, .clipped = true},
                             {.blocker = true, .later = true},
                             {.blocker = true, .later = true, .elevated = true},
                             {.blocker = true, .modal = true}};
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        cui_context *ui = cui_create((cui_config){0});
        assert(ui);
        reset();
        scene(ui, cases[i]);
        click(ui, 40, 40, cases[i]);
        assert(parent_clicks == (i == 0 || i == 3 ? 1u : 0u));
        assert(child_clicks == 0 && later_clicks == (i == 4 ? 1u : 0u));
        assert(modal_clicks == (i == 6 ? 1u : 0u));
        if (i == 1 || i == 2 || i == 5) {
            assert(cui_event_target(ui, 0) == 20 && cui_event_consumed(ui, 0));
            assert(cui_event_activation(ui, 1) == 0);
            assert(hit_barrier);
        }
        if (i == 2) {
            cui_focus(ui, 20);
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
            scene(ui, cases[i]);
            assert(child_clicks == 1 && parent_clicks == 0 && cui_focused(ui) == 20);
        }
        cui_destroy(ui);
    }
    cui_context *ui = cui_create((cui_config){0});
    assert(ui);
    reset();
    options o = {.blocker = true, .prepaint = true};
    scene(ui, o);
    cui_focus(ui, 90);
    event(ui, CUI_EVENT_POINTER_DOWN, 40, 40);
    scene(ui, o);
    assert(cui_focused(ui) == 90 && !cui_pressed(ui, 10));
    /* Activation-only barriers do not silently become hover/prepaint occluders. */
    assert(paint_info.pointer_visible && paint_info.pointer_hit == 10);
    event(ui, CUI_EVENT_POINTER_UP, 40, 40);
    scene(ui, o);
    assert(parent_clicks == 0 && cui_focused(ui) == 90);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_COMPOSITION_START, .composition_target = 90}));
    scene(ui, o);
    event(ui, CUI_EVENT_POINTER_DOWN, 40, 40);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    scene(ui, o);
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    assert(count == 2 && events[1].ime_composing && cui_focused(ui) == 90);
    event(ui, CUI_EVENT_CANCEL, 40, 40);
    scene(ui, o);
    event(ui, CUI_EVENT_POINTER_DOWN, 150, 90);
    scene(ui, o);
    assert(cui_pressed(ui, 10));
    event(ui, CUI_EVENT_POINTER_MOVE, 40, 40);
    scene(ui, o);
    assert(cui_event_target(ui, 0) == 10 && !cui_event_consumed(ui, 0) && cui_pressed(ui, 10));
    event(ui, CUI_EVENT_POINTER_LEAVE, 0, 0);
    scene(ui, o);
    assert(cui_pressed(ui, 10) && cui_pointer_cursor(ui) == CUI_CURSOR_GRAB);
    event(ui, CUI_EVENT_POINTER_UP, 40, 40);
    scene(ui, o);
    assert(!cui_pressed(ui, 10) && parent_clicks == 0);
    assert(cui_event_target(ui, 0) == 10 && !cui_event_consumed(ui, 0));
    assert(cui_event_activation(ui, 0) == 0);
    event(ui, CUI_EVENT_POINTER_DOWN, 150, 90);
    scene(ui, o);
    event(ui, CUI_EVENT_CANCEL, 150, 90);
    event(ui, CUI_EVENT_POINTER_UP, 150, 90);
    scene(ui, o);
    assert(parent_clicks == 0);
    click(ui, 150, 90, o);
    assert(parent_clicks == 1);
    cui_focus(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    scene(ui, o);
    assert(parent_clicks == 2 && cui_focused(ui) == 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
    scene(ui, o);
    assert(cui_pressed(ui, 10));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    scene(ui, o);
    assert(parent_clicks == 3);
    cui_destroy(ui);
}
