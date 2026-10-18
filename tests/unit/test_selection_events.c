/* SPDX-License-Identifier: MIT */
#include "cui/cui_radio_group.h"
#include <assert.h>
#include <string.h>
static cui_context *ui;
static cui_id selected = 11;
static cui_radio_group_state state;
static cui_radio_group_item items[] = {
    {11, {"One", 3}, false}, {12, {"Two", 3}, false}, {13, {"Three", 5}, false}};
static cui_selection_change copies[16];
static size_t calls, requests;
static int mode;
static bool consume_later;
static void request(void *user, cui_id value) {
    assert(user == &requests);
    requests++;
    if (mode == 1)
        selected = value;
    if (mode == 2)
        selected = 13;
    if (mode == 3) {
        selected = value;
        size_t n;
        const cui_event *events = cui_frame_events(ui, &n);
        for (size_t i = 0; i < n; i++)
            if (events[i].type == CUI_EVENT_ACCESSIBILITY_ACTION)
                cui_consume_event(ui, i);
    }
}
static void observe(void *user, const cui_selection_change *change) {
    assert(user == &calls && calls < 16);
    assert(change->event.kind == CUI_COMPONENT_EVENT_CHANGE && change->event.target == 10);
    assert(change->value == selected);
    copies[calls++] = *change;
    if (consume_later) {
        size_t n;
        (void)cui_frame_events(ui, &n);
        for (size_t i = change->event.event_index + 1; i < n; i++)
            cui_consume_event(ui, i);
    }
}
static cui_radio_group_event_result frame(bool styled, cui_radio_group_options options,
                                          bool observer, bool passive) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    if (passive)
        cui_begin_box(ui, 99, (cui_box){.inert = true});
    cui_selection_handler handler = {observer ? observe : NULL, &calls};
    cui_radio_group_event_result result;
    if (styled) {
        cui_theme theme = cui_theme_light();
        result =
            cui_radio_group_events(ui, 10, &theme, items, 3, &selected, &state, options, handler);
    } else {
        result = cui_base_radio_group_events(ui, 10, items, 3, &selected, &state, options, handler);
        cui_begin_box(ui, 10, (cui_box){.width = 240});
        for (size_t i = 0; i < 3; i++) {
            cui_begin_box(ui, items[i].id, (cui_box){.width = 240, .height = 40});
            cui_end_box(ui);
        }
        cui_end_box(ui);
    }
    if (passive)
        cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(result.events.valid && result.legacy.valid);
    return result;
}
static void key(cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
int main(void) {
    for (unsigned styled = 0; styled < 2; styled++) {
        ui = cui_create((cui_config){.width = 400, .height = 300});
        assert(ui);
        memset(&state, 0, sizeof(state));
        calls = requests = 0;
        selected = 11;
        mode = 0;
        cui_radio_group_options options = {
            .vertical = true, .on_change = request, .change_user = &requests};
        frame(styled != 0, options, true, false);
        cui_focus(ui, 11);
        key(CUI_KEY_DOWN);
        cui_radio_group_event_result r = frame(styled != 0, options, true, false);
        assert(r.legacy.requested && !r.legacy.changed && !r.events.change_count && !calls);
        assert(cui_focused(ui) == 12 && requests == 1);
        mode = 1;
        key(CUI_KEY_END);
        key(CUI_KEY_HOME);
        r = frame(styled != 0, options, true, false);
        assert(r.events.change_count == 2 && calls == 2 && selected == 11);
        assert(copies[0].previous == 11 && copies[0].value == 13 && copies[1].previous == 13);
        assert(copies[0].reason == CUI_SELECTION_REASON_NAVIGATION);
        assert(copies[0].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
        assert(!(copies[0].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
        assert(copies[0].event.event_index < copies[1].event.event_index);
        mode = 2;
        key(CUI_KEY_DOWN);
        r = frame(styled != 0, options, true, false);
        assert(copies[2].item == 12 && copies[2].value == 13 && r.events.change_count == 1);
        /* Missing observer preserves owner mutation, without reporting callback counts. */
        mode = 1;
        key(CUI_KEY_HOME);
        r = frame(styled != 0, options, false, false);
        assert(selected == 11 && !r.events.change_count && calls == 3);
        /* Genuine source/index facts survive old consumption; no accepted snapshot
         * is available before legacy semantic registration. */
        mode = 3;
        assert(cui_queue_accessibility_action(ui, 12, CUI_ACCESSIBILITY_ACTIVATE));
        r = frame(styled != 0, options, true, false);
        assert(r.events.change_count == 1 && calls == 4);
        assert(!(copies[3].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
        assert(copies[3].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
        assert(copies[3].item == 12 && copies[3].event.target == 10);
        mode = 1;
        for (unsigned touch = 0; touch < 2; touch++) {
            cui_id item = touch ? 12 : 13;
            cui_rect bounds = cui_bounds(ui, item);
            for (unsigned up = 0; up < 2; up++)
                assert(cui_queue_event(
                    ui, (cui_event){.type = up ? CUI_EVENT_POINTER_UP : CUI_EVENT_POINTER_DOWN,
                                    .pointer_type = touch ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE,
                                    .pointer_id = touch ? 42 : 0,
                                    .x = bounds.x + 2,
                                    .y = bounds.y + 2}));
            size_t before_calls = calls;
            r = frame(styled != 0, options, true, false);
            assert(r.events.change_count == 1 && calls == before_calls + 1);
            assert(copies[before_calls].reason == CUI_SELECTION_REASON_ACTIVATION);
            assert(copies[before_calls].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
            assert(!(copies[before_calls].event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
        }
        consume_later = true;
        key(CUI_KEY_HOME);
        key(CUI_KEY_END);
        r = frame(styled != 0, options, true, false);
        assert(r.events.change_count == 1 && selected == 11);
        consume_later = false;
        options.on_change = NULL;
        key(CUI_KEY_END);
        r = frame(styled != 0, options, true, false);
        assert(selected == 13 && r.events.change_count == 1 && r.legacy.changed);
        /* Already-selected activation remains silent even with an observer. */
        assert(cui_queue_accessibility_action(ui, 13, CUI_ACCESSIBILITY_ACTIVATE));
        r = frame(styled != 0, options, true, false);
        assert(!r.events.change_count && !r.legacy.requested);
        options.disabled = true;
        key(CUI_KEY_DOWN);
        r = frame(styled != 0, options, true, false);
        assert(!r.events.change_count && r.events.submitted);
        options.disabled = false;
        r = frame(styled != 0, options, true, true);
        assert(!r.events.submitted && !r.events.change_count);
        cui_radio_group_event_result outside = cui_base_radio_group_events(
            ui, 10, items, 3, &selected, &state, options, (cui_selection_handler){observe, &calls});
        assert(outside.events.valid && !outside.events.submitted && !outside.events.change_count);
        cui_radio_group_event_result invalid = cui_base_radio_group_events(
            ui, 0, items, 3, &selected, &state, options, (cui_selection_handler){observe, &calls});
        assert(!invalid.events.valid && !invalid.legacy.valid);
        cui_selection_change saved = copies[3];
        cui_destroy(ui);
        assert(saved.event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY && saved.value == 12);
    }
    return 0;
}
