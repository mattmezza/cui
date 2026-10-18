/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static cui_context *ui;
static cui_theme theme;
static unsigned legacy_count, typed_count, activation_count, request_count, hover_count;
static char trace[64];
static size_t trace_count;
static cui_component_event_metadata retained;
static cui_component_check_request requested;
static cui_check_state owner = CUI_CHECK_MIXED;
static bool accept, consume, disabled, loading, inert, layout_only, base, readonly;
static cui_component_action_result button_result, check_result;
static void legacy(void *user) {
    assert(user == &legacy_count);
    ++legacy_count;
    trace[trace_count++] = 'L';
    if (consume) {
        size_t count;
        const cui_event *events = cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; ++i)
            if (events[i].type == CUI_EVENT_ACCESSIBILITY_ACTION)
                assert(cui_consume_event(ui, i));
    }
}
static void old_typed(void *user, const cui_button_click_event *event) {
    assert(user == &legacy_count && event->has_evidence);
    ++typed_count;
    trace[trace_count++] = 'T';
}
static void activated(void *user, const cui_component_event_metadata *event) {
    assert(user == &activation_count && event->kind == CUI_COMPONENT_EVENT_ACTIVATE);
    ++activation_count;
    trace[trace_count++] = 'N';
    retained = *event;
}
static void hovered(void *user, bool value) {
    assert(user == &legacy_count);
    (void)value;
    ++hover_count;
}
static void check_request(void *user, const cui_component_check_request *request) {
    assert(user == &owner && request->event.kind == CUI_COMPONENT_EVENT_REQUEST);
    ++request_count;
    requested = *request;
    if (accept)
        owner = request->requested;
}
static void begin(void) {
    assert(cui_begin_frame(ui, 400, 220, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 350, .height = 200, .inert = inert,
                                 .layout_only = layout_only});
}
static void finish(void) {
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void frame(void) {
    static cui_button_state state;
    begin();
    button_result = cui_button_events(ui, 10, &theme, cui_string("Run"),
        (cui_button_options){.width = 120, .disabled = disabled, .loading = loading,
                             .on_click = legacy, .on_click_event = old_typed,
                             .on_hover = hovered, .state = &state, .action_user = &legacy_count},
        (cui_component_activation_handler){activated, &activation_count});
    cui_component_check_handler handler = {readonly ? NULL : check_request, &owner};
    if (base) {
        cui_begin_box(ui, 20, (cui_box){.width = 120, .height = 36});
        check_result = cui_base_check_events(ui, 20, cui_string("Check"), owner,
                                             (cui_check_options){.disabled = disabled}, handler);
        cui_end_box(ui);
    } else
        check_result = cui_checkbox_events(ui, 20, &theme, cui_string("Check"), owner,
                                            (cui_checkbox_options){.disabled = disabled}, handler);
    finish();
}
static void click(cui_id id, cui_pointer_type type) {
    cui_rect bounds = cui_bounds(ui, id);
    assert(bounds.w > 0 && bounds.h > 0);
    cui_event event = {.type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 4, .y = bounds.y + 4,
                       .pointer_type = type, .pointer_id = 9, .pointer_device = 7};
    assert(cui_queue_event(ui, event));
    event.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, event));
}
static void key(cui_key key_value) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key_value}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = key_value}));
}
static const cui_semantic *semantic(cui_id id) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (nodes[i].id == id)
            return nodes + i;
    assert(0);
    return NULL;
}
int main(void) {
    ui = cui_create((cui_config){0});
    assert(ui);
    theme = cui_theme_light();
    frame();
    click(10, CUI_POINTER_MOUSE);
    click(10, CUI_POINTER_MOUSE);
    frame();
    assert(button_result.valid && button_result.submitted && button_result.activation_count == 2);
    assert(!button_result.request_count && legacy_count == 2 && typed_count == 2 && activation_count == 2);
    assert(trace_count == 6 && !memcmp(trace, "LTNLTN", 6));
    assert(hover_count && retained.activation.press.contact == 9);
    assert(retained.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                 .accessibility_target = 10, .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    consume = true;
    frame();
    consume = false;
    assert(activation_count == 3 && retained.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    assert(!retained.activation.has_press && !retained.activation.has_key_bounds);
    cui_focus(ui, 10);
    key(CUI_KEY_ENTER);
    frame();
    assert(activation_count == 4 && retained.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    cui_focus(ui, 10);
    key(CUI_KEY_SPACE);
    frame();
    assert(activation_count == 5);
    cui_focus(ui, 10);
    frame();
    click(20, CUI_POINTER_TOUCH);
    frame();
    assert(check_result.activation_count == 1 && check_result.request_count == 1);
    assert(owner == CUI_CHECK_MIXED && requested.current == CUI_CHECK_MIXED && requested.requested == CUI_CHECK_CHECKED);
    assert(requested.event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(cui_focused(ui) == 10); /* Styled source preserves pointer focus. */
    accept = true;
    click(20, CUI_POINTER_MOUSE);
    click(20, CUI_POINTER_MOUSE);
    frame();
    assert(check_result.request_count == 2 && owner == CUI_CHECK_CHECKED);
    assert(requested.current == CUI_CHECK_MIXED && requested.requested == CUI_CHECK_CHECKED);
    assert(semantic(20)->value == (float)CUI_CHECK_MIXED); /* Entry snapshot, not owner writeback. */
    frame();
    assert(semantic(20)->checked && semantic(20)->value == (float)CUI_CHECK_CHECKED);
    cui_focus(ui, 20);
    key(CUI_KEY_ENTER);
    key(CUI_KEY_ENTER);
    frame();
    assert(check_result.request_count == 2 && owner == CUI_CHECK_UNCHECKED);
    assert(requested.current == CUI_CHECK_CHECKED && requested.requested == CUI_CHECK_UNCHECKED);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                 .accessibility_target = 20, .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    frame();
    assert(check_result.request_count == 1 && owner == CUI_CHECK_CHECKED);
    assert(requested.event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    assert(!requested.event.activation.has_press && !requested.event.activation.has_key_bounds);
    readonly = true;
    unsigned before = request_count;
    click(20, CUI_POINTER_MOUSE);
    frame();
    assert(check_result.activation_count == 1 && !check_result.request_count && request_count == before);
    assert(owner == CUI_CHECK_CHECKED);
    readonly = false;
    disabled = true;
    click(10, CUI_POINTER_MOUSE);
    click(20, CUI_POINTER_MOUSE);
    frame();
    assert(!button_result.activation_count && !check_result.activation_count);
    disabled = false;
    loading = true;
    frame();
    click(10, CUI_POINTER_MOUSE);
    frame();
    assert(!button_result.activation_count);
    loading = false;
    for (unsigned pass = 0; pass < 2; ++pass) {
        frame();
        click(10, CUI_POINTER_MOUSE);
        click(20, CUI_POINTER_MOUSE);
        inert = pass == 0;
        layout_only = pass == 1;
        frame();
        assert(button_result.valid && !button_result.submitted && !button_result.activation_count);
        assert(check_result.valid && !check_result.submitted && !check_result.activation_count);
        inert = layout_only = false;
    }
    base = true;
    frame();
    cui_focus(ui, 10);
    click(20, CUI_POINTER_MOUSE);
    frame();
    assert(cui_focused(ui) == 20 && check_result.request_count == 1);
    frame();
    unsigned activations_before = activation_count;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                 .accessibility_target = 10, .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    begin();
    cui_component_action_result no_handler = cui_button_events(ui, 10, &theme, cui_string("Run"),
        (cui_button_options){0}, (cui_component_activation_handler){0});
    assert(no_handler.valid && no_handler.submitted && no_handler.activation_count == 1);
    assert(activation_count == activations_before);
    finish();
    /* Conflicting old/new handlers reject before semantics or callbacks. */
    begin();
    cui_check_options conflict = {.change_user = &owner};
    cui_component_action_result invalid = cui_base_check_events(ui, 99, cui_string("Bad"), owner,
                conflict, (cui_component_check_handler){check_request, &owner});
    assert(!invalid.valid && !invalid.submitted);
    invalid = cui_checkbox_events(ui, 98, &theme, cui_string("Bad"), owner,
        (cui_checkbox_options){.change_user = &owner}, (cui_component_check_handler){0});
    assert(!invalid.valid);
    finish();
    cui_component_action_result outside = cui_button_events(ui, 10, &theme, cui_string("Run"),
        (cui_button_options){0}, (cui_component_activation_handler){activated, &activation_count});
    assert(outside.valid && !outside.submitted && !outside.activation_count);
    cui_checkbox_state untouched = {.id = 20};
    cui_checkbox_state untouched_before = untouched;
    outside = cui_checkbox_events(ui, 20, &theme, cui_string("Check"), owner,
        (cui_checkbox_options){.state = &untouched},
        (cui_component_check_handler){check_request, &owner});
    assert(outside.valid && !outside.submitted && !outside.request_count);
    assert(memcmp(&untouched, &untouched_before, sizeof untouched) == 0);
    size_t saturated = SIZE_MAX;
    bool overflow = false;
    cui_control_events_private_increment(&saturated, &overflow);
    assert(overflow && saturated == SIZE_MAX);
    cui_component_action_result copy = check_result;
    assert(copy.request_count == check_result.request_count && request_count >= 5);
    cui_destroy(ui); /* Teardown after frame, never from active dispatch. */
    assert(retained.target == 10 && requested.event.target == 20);
    puts("control events: legacy order, controlled snapshots, evidence, passive and readonly PASS");
    return 0;
}
