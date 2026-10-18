/* SPDX-License-Identifier: MIT */
#include "cui/cui_boolean_events.h"
#include <assert.h>
#include <string.h>

typedef struct {
    cui_context *ui;
    bool value, accept, consume_later, cancel_later, omit_sibling, close_pending;
    unsigned calls, unrelated;
    cui_boolean_event_options *mutate_options;
    cui_component_boolean_request requests[16];
} owner;
typedef struct {
    bool toggle, legacy, inert, layout_only, omit, no_handler, cancel_first;
    cui_boolean_event_options options;
} configuration;
static void request(void *user, const cui_component_boolean_request *event) {
    owner *o = user;
    assert(o->calls < 16 && event->event.target == 10);
    assert(event->event.kind == CUI_COMPONENT_EVENT_REQUEST);
    assert(event->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION);
    assert(event->event.frame_sequence == cui_frame_sequence(o->ui));
    if (o->calls && event->event.frame_sequence == o->requests[o->calls - 1].event.frame_sequence)
        assert(event->event.event_index > o->requests[o->calls - 1].event.event_index);
    o->requests[o->calls++] = *event;
    ++o->unrelated;
    if (o->mutate_options)
        o->mutate_options->disabled = true;
    o->omit_sibling = o->close_pending = true; /* Schedule, never destroy active context. */
    if (o->accept)
        o->value = event->requested;
    if (o->consume_later)
        assert(cui_consume_event(o->ui, event->event.event_index + 1));
    if (o->cancel_later)
        assert(!cui_cancel_event_activation(o->ui, event->event.event_index + 1));
}
static const cui_semantic *semantic(cui_context *ui, cui_id id) {
    size_t count = 0;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    for (size_t n = 0; n < count; ++n)
        if (nodes[n].id == id)
            return &nodes[n];
    return NULL;
}
static cui_component_action_result scene(owner *o, configuration c, bool *parent) {
    cui_context *ui = o->ui;
    assert(cui_begin_frame(ui, 200, 120, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 120});
    bool activated =
        cui_interact(ui, 1, CUI_ROLE_BUTTON, cui_string("Parent"), false, false, 0).activated;
    if (parent)
        *parent = activated;
    cui_begin_absolute_box(ui, 10, (cui_box){.inert = c.inert, .layout_only = c.layout_only},
                           (cui_rect){10, 10, 80, 30});
    cui_component_action_result result = {0};
    if (c.cancel_first)
        assert(cui_cancel_event_activation(ui, 1));
    if (!c.omit) {
        if (c.legacy) {
            bool changed =
                c.toggle
                    ? cui_base_toggle(ui, 10, cui_string("Flag"), &o->value, c.options.disabled)
                    : cui_base_switch(ui, 10, cui_string("Flag"), &o->value, c.options.disabled);
            result.activation_count = changed ? 1 : 0;
        } else {
            cui_component_boolean_handler handler = {c.no_handler ? NULL : request, o};
            result = c.toggle ? cui_base_toggle_events(ui, 10, cui_string("Flag"), o->value,
                                                       c.options, handler)
                              : cui_base_switch_events(ui, 10, cui_string("Flag"), o->value,
                                                       c.options, handler);
        }
    }
    cui_end_box(ui);
    if (!o->omit_sibling) {
        cui_begin_absolute_box(ui, 20, (cui_box){0}, (cui_rect){110, 10, 60, 30});
        cui_base_button(ui, 20, cui_string("Sibling"), false);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static owner create(configuration c) {
    owner o = {.ui = cui_create((cui_config){0})};
    assert(o.ui);
    scene(&o, c, NULL);
    scene(&o, c, NULL);
    return o;
}
static void queue(cui_context *ui, cui_event event) {
    assert(cui_queue_event(ui, event));
}
static void click(cui_context *ui, cui_pointer_type type) {
    cui_event event = {.type = CUI_EVENT_POINTER_DOWN,
                       .x = 25,
                       .y = 20,
                       .pointer_type = type,
                       .pointer_id = 7,
                       .pointer_device = 9};
    queue(ui, event);
    event.type = CUI_EVENT_POINTER_UP;
    queue(ui, event);
}
static void accessibility(cui_context *ui) {
    queue(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                          .accessibility_target = 10,
                          .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE});
}
static void values_and_metadata(bool toggle, bool accept, bool initial) {
    configuration c = {.toggle = toggle};
    owner o = create(c);
    o.accept = accept;
    o.value = initial;
    o.mutate_options = &c.options;
    scene(&o, c, NULL);
    click(o.ui, CUI_POINTER_MOUSE);
    click(o.ui, CUI_POINTER_TOUCH);
    cui_component_action_result result = scene(&o, c, NULL);
    assert(result.valid && result.submitted && result.activation_count == 2 &&
           result.request_count == 2 && !result.count_overflow && o.calls == 2);
    assert(o.value == (accept ? !initial : initial) && o.unrelated == 2 && o.close_pending);
    for (unsigned n = 0; n < o.calls; ++n) {
        assert(o.requests[n].current == initial && o.requests[n].requested == !initial);
        assert(o.requests[n].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
        assert(o.requests[n].event.activation.press.contact == 7);
        assert(o.requests[n].event.activation.release.device == 9);
    }
    assert(o.requests[0].event.activation.press.type == CUI_POINTER_MOUSE);
    assert(o.requests[1].event.activation.press.type == CUI_POINTER_TOUCH);
    const cui_semantic *s = semantic(o.ui, 10);
    assert(s && s->role == (toggle ? CUI_ROLE_BUTTON : CUI_ROLE_SWITCH) && s->checked == initial);
    assert(!s->disabled && c.options.disabled); /* Callback changed caller descriptor only. */
    assert(!s->details.required && !s->details.focusable && !s->details.blocks_pointer_activation);
    assert(!semantic(o.ui, 20)); /* Callback can omit later siblings safely. */
    cui_component_boolean_request copy = o.requests[1];
    c.options.disabled = false;
    scene(&o, c, NULL);
    assert(semantic(o.ui, 10)->checked == (accept ? !initial : initial));
    cui_destroy(o.ui); /* Payload remains copyable after completed teardown. */
    assert(copy.event.target == 10 && copy.event.activation.release.device == 9);
}
static void keys(bool toggle) {
    configuration c = {.toggle = toggle};
    owner o = create(c);
    cui_focus(o.ui, 10);
    queue(o.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER});
    queue(o.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true});
    queue(o.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE});
    queue(o.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE});
    cui_component_action_result result = scene(&o, c, NULL);
    assert(result.activation_count == 2 && o.calls == 2 && !o.value);
    assert(o.requests[0].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(o.requests[1].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(!o.requests[0].event.activation.has_press);
    cui_destroy(o.ui);
}
static void accessibility_mutation(bool toggle, bool consume, bool cancel) {
    configuration c = {.toggle = toggle};
    owner o = create(c);
    o.accept = true;
    o.consume_later = consume;
    o.cancel_later = cancel;
    accessibility(o.ui);
    accessibility(o.ui);
    cui_component_action_result result = scene(&o, c, NULL);
    assert(result.activation_count == (consume ? 1u : 2u));
    assert(result.request_count == result.activation_count && o.calls == result.request_count);
    assert(o.value && o.requests[0].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    assert(!o.requests[0].current && o.requests[0].requested);
    if (o.calls == 2)
        assert(!o.requests[1].current && o.requests[1].requested);
    cui_destroy(o.ui);
}
static void suppression_and_legacy(bool toggle) {
    configuration c = {.toggle = toggle};
    owner o = create(c);
    c.no_handler = true;
    click(o.ui, CUI_POINTER_MOUSE);
    cui_component_action_result r = scene(&o, c, NULL);
    assert(r.activation_count == 1 && !r.request_count && !o.value && !o.calls);
    c.no_handler = false;
    c.cancel_first = true;
    click(o.ui, CUI_POINTER_MOUSE);
    r = scene(&o, c, NULL);
    assert(!r.activation_count && !r.request_count && !o.calls);
    c.cancel_first = false;
    accessibility(o.ui); /* Previous active target; current disabled submission rejects it. */
    cui_focus(o.ui, 10);
    queue(o.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER});
    c.options.disabled = true;
    r = scene(&o, c, NULL);
    assert(r.submitted && !r.activation_count && !o.calls);
    click(o.ui, CUI_POINTER_MOUSE);
    bool parent = false;
    r = scene(&o, c, &parent);
    assert(r.valid && r.submitted && !r.activation_count && !r.request_count && !o.calls);
    bool new_parent = parent;
    cui_destroy(o.ui);
    c.legacy = true;
    o = create(c);
    click(o.ui, CUI_POINTER_MOUSE);
    scene(&o, c, &parent);
    assert(parent == new_parent && parent); /* Disabled Base child preserves parent routing. */
    cui_destroy(o.ui);
    c = (configuration){.toggle = toggle};
    o = create(c);
    for (unsigned mode = 0; mode < 3; ++mode) {
        click(o.ui, CUI_POINTER_MOUSE);
        configuration passive = {
            .toggle = toggle, .inert = mode == 0, .layout_only = mode == 1, .omit = mode == 2};
        r = scene(&o, passive, NULL);
        assert(!r.submitted && !r.activation_count && !o.calls);
        scene(&o, c, NULL);
    }
    c.legacy = true;
    click(o.ui, CUI_POINTER_MOUSE);
    click(o.ui, CUI_POINTER_MOUSE);
    r = scene(&o, c, NULL);
    assert(r.activation_count == 1 && o.value && !o.calls); /* Legacy aggregate path unchanged. */
    cui_destroy(o.ui);
}
static void focus_policy(bool toggle) {
    configuration c = {.toggle = toggle,
                       .options = {.no_pointer_focus = true, .skip_tab = true, .tab_index = 7}};
    owner o = create(c);
    click(o.ui, CUI_POINTER_MOUSE);
    cui_component_action_result r = scene(&o, c, NULL);
    const cui_semantic *s = semantic(o.ui, 10);
    assert(r.activation_count == 1 && !s->focused && s->tab_stop_disabled && s->tab_index == 7 &&
           s->details.no_pointer_focus);
    cui_focus(o.ui, 10); /* Pointer suppression does not suppress explicit/key focus. */
    queue(o.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER});
    r = scene(&o, c, NULL);
    assert(r.activation_count == 1 && semantic(o.ui, 10)->focused);
    cui_destroy(o.ui);
}
static cui_component_action_result protected_result(cui_context *ui) {
    return cui_base_toggle_events(ui, 99, cui_string("Guarded"), true,
                                  (cui_boolean_event_options){0},
                                  (cui_component_boolean_handler){0});
}
static cui_size guarded_measure(void *user, cui_str text, float size) {
    cui_context **ui = user;
    if (*ui) {
        cui_component_action_result r = protected_result(*ui);
        assert(r.valid && !r.submitted && !r.activation_count);
    }
    return (cui_size){text.length * size / 2, size};
}
static bool guarded_paint(const cui_prepaint_info *info, const void *payload,
                          cui_prepaint_writer *writer) {
    (void)info;
    (void)writer;
    cui_context *ui = *(cui_context *const *)payload;
    cui_component_action_result r = protected_result(ui);
    assert(r.valid && !r.submitted && !r.activation_count);
    return true;
}
static void preflight_and_phases(void) {
    cui_context *ui = NULL;
    ui = cui_create((cui_config){.measure = guarded_measure, .measure_user = &ui});
    assert(ui);
    cui_component_action_result r = protected_result(ui);
    assert(r.valid && !r.submitted);
    assert(cui_begin_frame(ui, 200, 120, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 120});
    assert(!cui_base_switch_events(ui, 0, cui_string("Invalid"), false,
                                   (cui_boolean_event_options){0},
                                   (cui_component_boolean_handler){0})
                .valid);
    assert(!cui_base_switch_events(ui, 9, (cui_str){NULL, 1}, false, (cui_boolean_event_options){0},
                                   (cui_component_boolean_handler){0})
                .valid);
    (void)cui_measure_text(ui, cui_string("Measure"), 14);
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = guarded_paint,
                                                       .payload = &ui,
                                                       .payload_bytes = sizeof ui,
                                                       .command_budget = 1}));
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && !semantic(ui, 9) && !semantic(ui, 99));
    cui_destroy(ui);
}
int main(void) {
    preflight_and_phases();
    assert(!cui_base_switch_events(NULL, 1, cui_string("Flag"), false,
                                   (cui_boolean_event_options){0},
                                   (cui_component_boolean_handler){0})
                .valid);
    for (unsigned role = 0; role < 2; ++role) {
        values_and_metadata(role != 0, false, false);
        values_and_metadata(role != 0, true, false);
        values_and_metadata(role != 0, false, true);
        values_and_metadata(role != 0, true, true);
        keys(role != 0);
        accessibility_mutation(role != 0, false, false);
        accessibility_mutation(role != 0, true, false);
        accessibility_mutation(role != 0, false, true);
        suppression_and_legacy(role != 0);
        focus_policy(role != 0);
    }
    return 0;
}
