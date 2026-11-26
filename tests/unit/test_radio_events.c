/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>

enum { RADIO = 10, INDICATOR = 11, SIBLING = 20 };
typedef struct {
    cui_context *ui;
    cui_theme theme;
    bool checked, accept, consume_later, close_pending;
    bool painted_checked;
    size_t count, paints;
    cui_component_boolean_request requests[8];
    cui_radio_motion motion;
    cui_radio_tooltip_state tooltip;
} owner;

typedef struct {
    bool styled, disabled, inert, layout_only, omit, read_only, conflict;
} scene_options;

static void request_selection(void *user, const cui_component_boolean_request *request) {
    owner *state = user;
    assert(state->count < sizeof state->requests / sizeof state->requests[0]);
    assert(request->event.kind == CUI_COMPONENT_EVENT_REQUEST && request->event.target == RADIO);
    assert(request->event.frame_sequence == cui_frame_sequence(state->ui));
    assert(!request->current && request->requested);
    state->requests[state->count++] = *request;
    if (state->consume_later)
        assert(cui_consume_event(state->ui, request->event.event_index + 1));
    if (state->accept) {
        state->checked = request->requested;
        state->close_pending = true; /* Apply application changes without destroying the frame. */
        cui_invalidate(state->ui);
    }
}

static void style(cui_context *ui, void *user, cui_id id, bool checked, bool disabled,
                  cui_radio_paint *paint) {
    owner *state = user;
    assert(ui == state->ui && id == RADIO);
    (void)disabled;
    (void)paint;
    state->painted_checked = checked;
    ++state->paints;
}

static const cui_semantic *semantic(cui_context *ui, cui_id id) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (nodes[i].id == id)
            return &nodes[i];
    return NULL;
}

static cui_component_action_result scene(owner *state, scene_options o) {
    cui_context *ui = state->ui;
    assert(cui_begin_frame(ui, 200, 120, 0));
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 200, .height = 120, .inert = o.inert, .layout_only = o.layout_only});
    cui_component_action_result result = {0};
    if (!o.omit) {
        cui_component_boolean_handler handler = {o.read_only ? NULL : request_selection, state};
        cui_str label = cui_string("Primary");
        if (o.styled) {
            cui_radio_tooltip tooltip = {
                .id = 12, .state = &state->tooltip, .text = cui_string("Pick this option")};
            result = cui_radio_events(
                ui, RADIO, &state->theme, label, state->checked,
                (cui_radio_options){.disabled = o.disabled,
                                    .required = true,
                                    .invalid = true,
                                    .accessibility_label = cui_string("Primary choice"),
                                    .change_user = o.conflict ? state : NULL},
                (cui_radio_presentation){.motion = &state->motion,
                                         .tooltip = &tooltip,
                                         .custom_tab_index = true,
                                         .tab_index = -4,
                                         .parts = {.indicator = INDICATOR}},
                (cui_radio_styles){.instance = style, .user = state}, handler);
        } else {
            cui_begin_box(ui, RADIO, (cui_box){.width = 100, .height = 30});
            result = cui_base_radio_events(
                ui, RADIO, label, state->checked,
                (cui_radio_event_options){.interaction = {.disabled = o.disabled,
                                                          .skip_tab = true,
                                                          .no_pointer_focus = true,
                                                          .tab_index = -4},
                                          .required = true,
                                          .invalid = true,
                                          .pos_in_set = 2,
                                          .set_size = 3},
                handler);
            cui_end_box(ui);
        }
    }
    if (!state->close_pending)
        cui_button(ui, SIBLING, &state->theme, cui_string("Sibling"), false);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}

static owner make_owner(void) {
    owner state = {.ui = cui_create((cui_config){0}), .theme = cui_theme_light()};
    assert(state.ui);
    return state;
}

static void click(cui_context *ui, cui_pointer_type type) {
    const cui_semantic *node = semantic(ui, RADIO);
    assert(node && node->bounds.w > 0 && node->bounds.h > 0);
    cui_event event = {.type = CUI_EVENT_POINTER_DOWN,
                       .x = node->bounds.x + node->bounds.w / 2,
                       .y = node->bounds.y + node->bounds.h / 2,
                       .pointer_type = type,
                       .pointer_id = 7,
                       .pointer_device = 9};
    assert(cui_queue_event(ui, event));
    event.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, event));
}

static void activate_accessibly(cui_context *ui) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = RADIO,
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
}

static void pointer_and_keyboard(bool styled) {
    owner state = make_owner();
    scene_options o = {.styled = styled};
    assert(scene(&state, o).submitted);
    click(state.ui, CUI_POINTER_MOUSE);
    click(state.ui, CUI_POINTER_TOUCH);
    cui_component_action_result r = scene(&state, o);
    assert(r.valid && r.submitted && r.activation_count == 2 && r.request_count == 2);
    assert(!state.checked && state.count == 2 && semantic(state.ui, SIBLING));
    assert(state.requests[0].event.event_index < state.requests[1].event.event_index);
    for (size_t i = 0; i < 2; ++i) {
        cui_component_event_metadata event = state.requests[i].event;
        assert(event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
        assert(event.activation.has_press && event.activation.press.contact == 7);
        assert(event.activation.release.device == 9);
        assert(event.activation.press.type == (i ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE));
    }
    const cui_semantic *node = semantic(state.ui, RADIO);
    assert(node && node->role == CUI_ROLE_RADIO && !node->checked && !node->details.selected &&
           node->value == 0 && node->details.required && node->details.invalid &&
           node->tab_index == -4);
    if (styled) {
        assert(node->name.length == sizeof("Primary choice") - 1);
        assert(!state.painted_checked);
    } else {
        assert(node->details.pos_in_set == 2 && node->details.set_size == 3 &&
               node->tab_stop_disabled && !node->focused && node->details.no_pointer_focus);
    }

    state.accept = true;
    cui_focus(state.ui, RADIO);
    assert(
        cui_queue_event(state.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(
        state.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    activate_accessibly(state.ui);
    r = scene(&state, o);
    cui_component_action_result copied_result = r;
    assert(copied_result.request_count == 2 && r.activation_count == 2 && state.count == 4);
    assert(state.requests[2].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    assert(state.requests[3].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    assert(!state.requests[2].event.activation.has_press &&
           !state.requests[3].event.activation.has_press);
    assert(state.checked && !semantic(state.ui, RADIO)->checked && !semantic(state.ui, SIBLING));
    if (styled)
        assert(!state.painted_checked);

    assert(!scene(&state, o).request_count);
    node = semantic(state.ui, RADIO);
    assert(node->checked && node->details.selected && node->value == 1);
    if (styled)
        assert(state.painted_checked);
    activate_accessibly(state.ui);
    r = scene(&state, o);
    assert(r.activation_count == 1 && !r.request_count && state.count == 4);
    cui_component_boolean_request copied_request = state.requests[3];
    cui_destroy(state.ui);
    assert(copied_request.event.target == RADIO && copied_request.requested);
}

static void suppression(bool styled) {
    owner state = make_owner();
    scene_options o = {.styled = styled};
    scene(&state, o);
    state.consume_later = true;
    activate_accessibly(state.ui);
    activate_accessibly(state.ui);
    cui_component_action_result r = scene(&state, o);
    assert(r.activation_count == 1 && r.request_count == 1 && state.count == 1 && !state.checked);
    state.consume_later = false;
    click(state.ui, CUI_POINTER_MOUSE);
    o.read_only = true;
    r = scene(&state, o);
    assert(r.activation_count == 1 && !r.request_count);
    o.read_only = false;
    activate_accessibly(state.ui);
    o.disabled = true;
    r = scene(&state, o);
    assert(r.submitted && !r.activation_count && !r.request_count);
    o.disabled = false;
    for (unsigned mode = 0; mode < 3; ++mode) {
        scene(&state, o);
        activate_accessibly(state.ui);
        state.motion.next_wake_seconds = 42;
        r = scene(&state, (scene_options){.styled = styled,
                                          .inert = mode == 0,
                                          .layout_only = mode == 1,
                                          .omit = mode == 2});
        assert(!r.submitted && !r.request_count && !r.activation_count && state.count == 1);
        assert(state.motion.next_wake_seconds == 42);
    }
    if (styled) {
        scene(&state, o);
        activate_accessibly(state.ui);
        size_t paints = state.paints;
        o.conflict = true;
        r = scene(&state, o);
        assert(!r.valid && !r.submitted && state.count == 1 && state.paints == paints);
    }
    cui_destroy(state.ui);
}

int main(void) {
    owner state = make_owner();
    assert(cui_base_radio_events(state.ui, RADIO, cui_string("Primary"), false,
                                 (cui_radio_event_options){0}, (cui_component_boolean_handler){0})
               .valid);
    assert(!cui_base_radio_events(state.ui, RADIO, cui_string("Primary"), false,
                                  (cui_radio_event_options){.set_size = 1},
                                  (cui_component_boolean_handler){0})
                .valid);
    cui_destroy(state.ui);
    for (unsigned styled = 0; styled < 2; ++styled) {
        pointer_and_keyboard(styled != 0);
        suppression(styled != 0);
    }
    return 0;
}
