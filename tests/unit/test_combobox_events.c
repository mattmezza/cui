/* SPDX-License-Identifier: MIT */
#include "cui/cui_choice.h"
#include <assert.h>
#include <string.h>
static cui_context *ui;
static cui_edit *query;
static char text[64];
static cui_id selected;
static cui_choice_popup popup;
static cui_choice_item items[] = {{10, {"First", 5}, false},
                                  {20, {"Disabled", 8}, true},
                                  {30, {"Last", 4}, false}},
                       scratch[3];
static cui_selection_change copies[32];
static size_t calls;
static bool redirect, consume;
static void changed(void *user, const cui_selection_change *change) {
    assert(user == &calls && calls < 32 && change->event.target == 2);
    assert(change->event.kind == CUI_COMPONENT_EVENT_CHANGE && change->value == selected);
    assert(change->reason == CUI_SELECTION_REASON_ACTIVATION);
    copies[calls++] = *change;
    if (redirect)
        selected = 30;
    if (consume) {
        size_t count;
        (void)cui_frame_events(ui, &count);
        for (size_t i = change->event.event_index + 1; i < count; i++)
            cui_consume_event(ui, i);
    }
}
static cui_combobox_event_result frame(bool styled, bool observer, bool disabled, bool passive,
                                       size_t capacity, bool legacy) {
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 400, .inert = passive});
    cui_combobox_event_result result;
    memset(&result, 0, sizeof(result));
    cui_selection_handler handler = {observer ? changed : NULL, &calls};
    if (styled) {
        cui_theme theme = cui_theme_light();
        cui_choice_popup_options options = {.viewport = {0, 0, 640, 480}, .disabled = disabled};
        if (legacy)
            result.legacy = cui_combobox(ui, 2, &theme, cui_string("Find"), query, items, 3,
                                         scratch, capacity, &selected, &popup, options);
        else
            result = cui_combobox_events(ui, 2, &theme, cui_string("Find"), query, items, 3,
                                         scratch, capacity, &selected, &popup, options, handler);
    } else {
        if (legacy)
            result.legacy = cui_base_combobox(ui, 2, cui_string("Find"), query, items, 3, scratch,
                                              capacity, &selected, &popup, disabled);
        else
            result = cui_base_combobox_events(ui, 2, cui_string("Find"), query, items, 3, scratch,
                                              capacity, &selected, &popup, disabled, handler);
        cui_begin_box(ui, 2, (cui_box){.width = 300, .height = 40});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void setup(void) {
    ui = cui_create(
        (cui_config){.width = 640, .height = 480, .max_elements = 512, .max_commands = 1024});
    assert(ui);
    memset(text, 0, sizeof(text));
    query = cui_edit_create((cui_edit_config){
        .buffer = text, .capacity = sizeof(text), .history_bytes = 512, .max_history_steps = 8});
    assert(query);
    cui_choice_popup_init(&popup, 700);
    selected = 0;
    calls = 0;
    redirect = consume = false;
}
static void teardown(void) {
    cui_edit_destroy(query);
    cui_destroy(ui);
}
static void key(cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void input_text(const char *value) {
    cui_event event = {.type = CUI_EVENT_TEXT};
    assert(strlen(value) < sizeof(event.text));
    memcpy(event.text, value, strlen(value) + 1);
    assert(cui_queue_event(ui, event));
}
static void pointer(cui_id id, cui_event_type type, bool touch) {
    cui_rect bounds = cui_bounds(ui, id);
    assert(bounds.w > 0 && bounds.h > 0);
    assert(cui_queue_event(
        ui, (cui_event){.type = type,
                        .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                        .pointer_id = touch ? 42 : 0,
                        .x = bounds.x + 2,
                        .y = bounds.y + 2}));
}
int main(void) {
    for (unsigned styled = 0; styled < 2; styled++) {
        for (unsigned enter_first = 0; enter_first < 2; enter_first++) {
            setup();
            frame(styled != 0, true, false, false, 3, false);
            cui_focus(ui, 2);
            key(CUI_KEY_DOWN);
            frame(styled != 0, true, false, false, 3, false);
            assert(popup.overlay.open && popup.list.active == 30);
            /* Reset active to First to reproduce the legacy text/Enter order fixture. */
            key(CUI_KEY_HOME);
            frame(styled != 0, true, false, false, 3, false);
            if (enter_first)
                key(CUI_KEY_ENTER);
            input_text("Las");
            if (!enter_first)
                key(CUI_KEY_ENTER);
            cui_combobox_event_result result = frame(styled != 0, true, false, false, 3, false);
            assert(result.legacy.changed && result.legacy.query_changed &&
                   result.legacy.matching_count == 1);
            assert(result.events.change_count == 1 && calls == 1);
            assert(selected == (enter_first ? 10 : 30));
            assert(popup.overlay.open == (enter_first != 0));
            assert(copies[0].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
            assert(!(copies[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
            assert(copies[0].item == selected && copies[0].previous == 0);
            /* Query-only edit preserves committed selection, even when filtered out. */
            input_text("Z");
            result = frame(styled != 0, true, false, false, 3, false);
            assert(result.legacy.query_changed && !result.events.change_count &&
                   selected == (enter_first ? 10 : 30));
            key(CUI_KEY_ENTER);
            result = frame(styled != 0, true, false, false, 3, false);
            assert(!result.events.change_count && !result.legacy.changed);
            result = frame(styled != 0, true, false, false, 2, false);
            assert(result.legacy.error == CUI_EDIT_CAPACITY && !result.events.valid &&
                   !result.events.change_count);
            result = frame(styled != 0, true, true, false, 3, false);
            assert(!result.events.change_count && !popup.overlay.open);
            result = frame(styled != 0, true, false, true, 3, false);
            assert(!result.events.submitted && !result.events.change_count);
            cui_selection_change saved = copies[0];
            teardown();
            assert(saved.previous == 0 && saved.event.target == 2);
        }
        setup();
        frame(styled != 0, true, false, false, 3, false);
        cui_focus(ui, 2);
        key(CUI_KEY_DOWN);
        frame(styled != 0, true, false, false, 3, false);
        key(CUI_KEY_ENTER);
        cui_combobox_event_result result = frame(styled != 0, false, false, false, 3, false);
        assert(selected == 30 && result.legacy.changed && !result.events.change_count && !calls);
        input_text("s");
        result = frame(styled != 0, false, false, false, 3, false);
        assert(result.legacy.query_changed && !result.events.change_count && !calls);
        key(CUI_KEY_DOWN);
        frame(styled != 0, true, false, false, 3, false);
        key(CUI_KEY_END);
        key(CUI_KEY_ENTER);
        result = frame(styled != 0, true, false, false, 3, false);
        assert(!result.events.change_count); /* Same-value confirmation. */
        key(CUI_KEY_DOWN);
        frame(styled != 0, false, false, false, 3, true);
        key(CUI_KEY_HOME);
        key(CUI_KEY_ENTER);
        result = frame(styled != 0, false, false, false, 3, true);
        assert(result.legacy.changed && selected == 10); /* Legacy coexistence. */
        teardown();
    }
    setup();
    frame(true, true, false, false, 3, false);
    cui_focus(ui, 2);
    key(CUI_KEY_DOWN);
    frame(true, true, false, false, 3, false);
    assert(!cui_accessibility_action_supported(ui, 30, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_accessibility_action(ui, 30, CUI_ACCESSIBILITY_FOCUS));
    cui_combobox_event_result result = frame(true, true, false, false, 3, false);
    assert(!result.events.change_count && !selected);
    for (unsigned touch = 0; touch < 2; touch++) {
        cui_id item = touch ? 10 : 30;
        pointer(item, CUI_EVENT_POINTER_DOWN, touch != 0);
        pointer(item, CUI_EVENT_POINTER_UP, touch != 0);
        result = frame(true, true, false, false, 3, false);
        assert(result.events.change_count == 1 && selected == item);
        assert(copies[calls - 1].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
        assert(!(copies[calls - 1].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
        cui_focus(ui, 2);
        key(CUI_KEY_DOWN);
        frame(true, true, false, false, 3, false);
    }
    redirect = true;
    pointer(30, CUI_EVENT_POINTER_DOWN, false);
    pointer(30, CUI_EVENT_POINTER_UP, false);
    pointer(10, CUI_EVENT_POINTER_DOWN, false);
    pointer(10, CUI_EVENT_POINTER_UP, false);
    result = frame(true, true, false, false, 3, false);
    redirect = false;
    assert(result.events.change_count == 2 && selected == 30);
    cui_focus(ui, 2);
    key(CUI_KEY_DOWN);
    frame(true, true, false, false, 3, false);
    consume = true;
    pointer(10, CUI_EVENT_POINTER_DOWN, false);
    pointer(10, CUI_EVENT_POINTER_UP, false);
    pointer(30, CUI_EVENT_POINTER_DOWN, false);
    pointer(30, CUI_EVENT_POINTER_UP, false);
    result = frame(true, true, false, false, 3, false);
    consume = false;
    assert(result.events.change_count == 1 && selected == 10);
    size_t before = calls;
    assert(cui_begin_frame(ui, 640, 480, .016f));
    cui_end_frame(ui);
    assert(calls == before);
    teardown();
    /* Keep a real option capture across frames, then restore input focus. The
     * Base Enter phase observes later index1 before the list's earlier up0. */
    setup();
    frame(true, true, false, false, 3, false);
    cui_focus(ui, 2);
    key(CUI_KEY_DOWN);
    key(CUI_KEY_HOME);
    frame(true, true, false, false, 3, false);
    pointer(30, CUI_EVENT_POINTER_DOWN, false);
    frame(true, true, false, false, 3, false);
    cui_focus(ui, 2);
    popup.list.active = 10; /* Caller-owned cursor, distinct from live capture. */
    pointer(30, CUI_EVENT_POINTER_UP, false);
    key(CUI_KEY_ENTER);
    input_text("s");
    result = frame(true, true, false, false, 3, false);
    assert(result.events.change_count == 2 && calls == 2 && selected == 30);
    assert(copies[0].previous == 0 && copies[0].value == 10 && copies[1].value == 30);
    assert(copies[0].event.event_index > copies[1].event.event_index);
    assert(copies[0].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(copies[1].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(result.legacy.query_changed && !popup.overlay.open);
    teardown();
    return 0;
}
