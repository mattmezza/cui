/* SPDX-License-Identifier: MIT */
#include "cui/cui_base.h"
#include "cui/cui_overlay.h"
#include <assert.h>
static cui_context *ui;
static cui_overlay outer, inner;
static bool trigger_visible = true, trigger_disabled;
static unsigned trigger_activations;
static cui_id last_activation;
static void button(cui_id id, bool disabled) {
    cui_begin_box(ui, id, (cui_box){.width = 100, .height = 30});
    cui_interaction s = cui_base_button(ui, id, cui_string("Control"), disabled);
    if (s.activated)
        last_activation = id;
    if (id == 1 && s.activated)
        trigger_activations++;
    cui_end_box(ui);
}
static void frame(void) {
    assert(cui_begin_frame(ui, 400, 400, .016f));
    cui_overlay_update(ui, &inner);
    cui_overlay_update(ui, &outer);
    cui_begin_box(ui, 1000, (cui_box){.width = 400, .gap = 5});
    if (trigger_visible)
        button(1, trigger_disabled);
    if (cui_overlay_begin(ui, &outer)) {
        cui_begin_box(ui, 200, (cui_box){.width = 250, .padding = 10, .gap = 5});
        button(201, false);
        button(202, true);
        button(203, false);
        if (cui_overlay_begin(ui, &inner)) {
            cui_begin_box(ui, 300, (cui_box){.width = 200, .padding = 10, .gap = 5});
            button(301, false);
            button(302, false);
            cui_end_box(ui);
            cui_overlay_end(ui);
        }
        cui_end_box(ui);
        cui_overlay_end(ui);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    if (outer.open)
        cui_overlay_set_bounds(&outer, cui_bounds(ui, 200));
    if (inner.open)
        cui_overlay_set_bounds(&inner, cui_bounds(ui, 300));
}
static void key(cui_key key, unsigned modifiers) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key, .modifiers = modifiers}));
    frame();
}
static void click(cui_rect r) {
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = r.x + 5, .y = r.y + 5}));
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = r.x + 5, .y = r.y + 5}));
    frame();
}
int main(void) {
    ui = cui_create((cui_config){.width = 400, .height = 400});
    assert(ui);
    cui_overlay_init(&outer, 200, true);
    cui_overlay_init(&inner, 300, true);
    frame();
    cui_focus(ui, 1);
    cui_overlay_open(ui, &outer, 1);
    frame();
    assert(cui_focused(ui) == 201);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_TAB}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    frame();
    assert(cui_focused(ui) == 203 && last_activation == 203);
    key(CUI_KEY_TAB, 0);
    assert(cui_focused(ui) == 201);
    key(CUI_KEY_TAB, CUI_MOD_SHIFT);
    assert(cui_focused(ui) == 203);
    cui_overlay_open(ui, &inner, 203);
    frame();
    assert(cui_focused(ui) == 301);
    key(CUI_KEY_TAB, CUI_MOD_SHIFT);
    assert(cui_focused(ui) == 302);
    key(CUI_KEY_TAB, 0);
    assert(cui_focused(ui) == 301);
    key(CUI_KEY_ESCAPE, 0);
    assert(!inner.open && outer.open && cui_focused(ui) == 203);
    key(CUI_KEY_ESCAPE, 0);
    assert(!outer.open && cui_focused(ui) == 1);
    cui_overlay_open(ui, &outer, 1);
    frame();
    click(cui_bounds(ui, 1));
    assert(!outer.open && trigger_activations == 0 && cui_focused(ui) == 1);
    click(cui_bounds(ui, 1));
    assert(trigger_activations == 1);
    cui_overlay_open(ui, &outer, 1);
    trigger_disabled = true;
    frame();
    key(CUI_KEY_ESCAPE, 0);
    assert(!outer.open && cui_focused(ui) == 0);
    trigger_disabled = false;
    frame();
    cui_overlay_open(ui, &outer, 1);
    trigger_visible = false;
    frame();
    key(CUI_KEY_ESCAPE, 0);
    assert(!outer.open && cui_focused(ui) == 0);
    trigger_visible = true;
    outer.modal = false;
    frame();
    cui_overlay_open(ui, &outer, 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 390, .y = 390}));
    frame();
    assert(outer.open); /* Opening has no previous panel bounds to dismiss against. */
    click(cui_bounds(ui, 1));
    assert(!outer.open && trigger_activations == 2);
    cui_overlay_open(ui, &outer, 1);
    frame();
    key(CUI_KEY_ESCAPE, 0);
    assert(!outer.open);
    cui_destroy(ui);
    return 0;
}
