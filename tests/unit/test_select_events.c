/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include <assert.h>
#include <string.h>
static cui_context *ui;
static cui_id selected = 10;
static cui_choice_popup popup;
static cui_choice_item items[] = {{10, {"First", 5}, false}, {20, {"Disabled", 8}, true},
                                   {30, {"Last", 4}, false}};
static cui_selection_change copies[32];
static size_t calls;
static bool redirect, consume;
static float max_height;
static void changed(void *user, const cui_selection_change *change) {
    assert(user == &calls && calls < 32);
    assert(change->event.target == 2 && change->event.kind == CUI_COMPONENT_EVENT_CHANGE);
    assert(change->reason == CUI_SELECTION_REASON_ACTIVATION && change->value == selected);
    copies[calls++] = *change;
    if (redirect) selected = 10;
    if (consume) {
        size_t count;
        (void)cui_frame_events(ui, &count);
        for (size_t i = change->event.event_index + 1; i < count; i++) cui_consume_event(ui, i);
    }
}
static cui_select_event_result frame(bool observer, bool disabled, bool passive, bool legacy) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 160, .height = 40, .inert = passive});
    cui_choice_popup_options options = {.viewport = {0, 0, 400, 300}, .width = 240, .disabled = disabled, .max_height = max_height};
    cui_select_event_result result;
    memset(&result, 0, sizeof(result));
    if (legacy)
        result.legacy_changed = cui_select(ui, 2, &theme, cui_string("Choice"), cui_string("Choose"),
            items, 3, &selected, &popup, options);
    else
        result = cui_select_events(ui, 2, &theme, cui_string("Choice"), cui_string("Choose"),
            items, 3, &selected, &popup, options,
            (cui_selection_handler){observer ? changed : NULL, &calls});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    if (!legacy) assert(result.events.valid);
    return result;
}
static void click(cui_id id, bool touch) {
    cui_rect bounds = cui_bounds(ui, id);
    assert(bounds.w > 0 && bounds.h > 0);
    for (unsigned up = 0; up < 2; up++)
        assert(cui_queue_event(ui, (cui_event){.type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
            .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE, .pointer_id = touch ? 42 : 0,
            .x = bounds.x + 2, .y = bounds.y + 2}));
}
static void open_popup(void) {
    assert(!popup.overlay.open);
    click(2, false);
    frame(true, false, false, false);
    assert(popup.overlay.open);
}
static void key(cui_key value) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = value}));
}
int main(void) {
    ui = cui_create((cui_config){.width = 400, .height = 300, .max_elements = 512, .max_commands = 1024});
    assert(ui); cui_choice_popup_init(&popup, 700);
    frame(true, false, false, false);
    open_popup();
    click(30, false); click(10, false);
    cui_select_event_result result = frame(true, false, false, false);
    assert(!result.legacy_changed && result.events.change_count == 2 && selected == 10);
    assert(!popup.overlay.open && calls == 2);
    assert(copies[0].previous == 10 && copies[0].value == 30 && copies[1].previous == 30);
    assert(copies[0].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(!(copies[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    open_popup(); click(30, true);
    result = frame(true, false, false, false);
    assert(result.legacy_changed && selected == 30 && result.events.change_count == 1);
    open_popup(); click(30, false);
    result = frame(true, false, false, false);
    assert(!result.legacy_changed && !result.events.change_count && !popup.overlay.open);
    open_popup(); key(CUI_KEY_HOME); key(CUI_KEY_ENTER);
    result = frame(true, false, false, false);
    assert(selected == 10 && result.events.change_count == 1);
    assert(copies[calls-1].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    open_popup();
    assert(!cui_accessibility_action_supported(ui, 30, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_accessibility_action(ui, 30, CUI_ACCESSIBILITY_FOCUS));
    result = frame(true, false, false, false);
    assert(!result.events.change_count && selected == 10 && cui_focused(ui) == 30);
    assert(popup.list.active == 10); /* Legacy cursor is independent of a11y focus. */
    key(CUI_KEY_END); key(CUI_KEY_ENTER);
    result = frame(true, false, false, false);
    assert(selected == 30 && result.events.change_count == 1);
    assert(copies[calls-1].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(!(copies[calls-1].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    open_popup(); click(10, false);
    result = frame(false, false, false, false);
    assert(selected == 10 && result.legacy_changed && !result.events.change_count);
    open_popup(); redirect = true; click(30, false); click(10, false);
    result = frame(true, false, false, false); redirect = false;
    assert(selected == 10 && result.events.change_count == 1 && !result.legacy_changed);
    open_popup(); consume = true; click(30, false); click(10, false);
    result = frame(true, false, false, false); consume = false;
    assert(selected == 30 && result.events.change_count == 1);
    open_popup(); click(10, false);
    result = frame(true, true, false, false);
    assert(selected == 30 && !result.events.change_count && !popup.overlay.open);
    result = frame(true, false, true, false);
    assert(!result.events.submitted && !result.events.change_count);
    frame(true, false, false, false);
    open_popup(); click(10, false);
    result = frame(false, false, false, true);
    assert(result.legacy_changed && selected == 10); /* Legacy entry point still works. */
    open_popup(); click(20, false);
    result = frame(true, false, false, false);
    assert(!result.events.change_count && popup.overlay.open && selected == 10);
    click(2, false); frame(true, false, false, false);
    assert(!popup.overlay.open);
    max_height = 60;
    open_popup();
    cui_rect row = cui_bounds(ui, 10);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
        .x = row.x + 2, .y = row.y + 2, .dy = -2}));
    click(10, false);
    result = frame(true, false, false, false);
    assert(!result.events.change_count && selected == 10 && popup.overlay.open);
    size_t before = calls;
    assert(cui_begin_frame(ui, 400, 300, .016f)); cui_end_frame(ui);
    assert(calls == before); /* Omitting immediate submission dispatches nothing. */
    cui_theme theme = cui_theme_light();
    cui_select_event_result invalid = cui_select_events(ui, 0, &theme, cui_string("Choice"),
        cui_string("Choose"), items, 3, &selected, &popup,
        (cui_choice_popup_options){.viewport = {0, 0, 400, 300}},
        (cui_selection_handler){changed, &calls});
    assert(!invalid.events.valid && !invalid.events.change_count);
    cui_selection_change saved = copies[0];
    cui_destroy(ui);
    assert(saved.previous == 10 && saved.value == 30 && saved.event.target == 2);
    return 0;
}
