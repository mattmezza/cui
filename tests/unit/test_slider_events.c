/* SPDX-License-Identifier: MIT */
#include "cui/cui_slider_events.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_component_slider_request events[64];
    size_t count;
    bool accept;
    double owner;
} recorder;
static void record(void *user, const cui_component_slider_request *event) {
    recorder *r = user;
    assert(r->count < 64);
    r->events[r->count++] = *event;
    if (r->accept && event->phase == CUI_SLIDER_UPDATE)
        r->owner = event->requested;
}
static cui_component_slider_result scene(cui_context *ui, cui_slider_state *state,
                                         cui_slider_options options, recorder *r, bool handler) {
    assert(cui_begin_frame(ui, 400, 100, .016f));
    cui_begin_box(ui, 10,
                  (cui_box){.width = 200, .height = 40, .fixed_width = true, .fixed_height = true});
    cui_component_slider_result result =
        cui_base_slider_events(ui, 10, cui_string("Position"), r->owner, state, options,
                               (cui_component_slider_handler){handler ? record : NULL, r});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void pointer(cui_context *ui, cui_event_type type, float x) {
    assert(cui_queue_event(ui, (cui_event){.type = type,
                                           .x = x,
                                           .y = 20,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 7,
                                           .pointer_device = 9}));
}
static void key(cui_context *ui, cui_key key_value) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key_value}));
}
static void evidence(const cui_component_slider_request *e, cui_component_event_source source,
                     size_t index) {
    assert(e->has_input && e->event.target == 10 && e->event.source == source);
    assert(e->event.event_index == index && e->event.frame_sequence > 0);
    assert((e->event.flags & (CUI_COMPONENT_EVENT_HAS_INDEX | CUI_COMPONENT_EVENT_HAS_FRAME)) ==
           (CUI_COMPONENT_EVENT_HAS_INDEX | CUI_COMPONENT_EVENT_HAS_FRAME));
    assert(!(e->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
}
static void phases(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    assert(ui);
    cui_slider_state state = {0};
    recorder r = {.owner = .25};
    cui_slider_options o = {.minimum = 0, .maximum = 1, .step = .125};
    cui_component_slider_result result = scene(ui, &state, o, &r, true);
    assert(result.valid && result.submitted && !result.request_count);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 50);
    pointer(ui, CUI_EVENT_POINTER_MOVE, 150);
    pointer(ui, CUI_EVENT_POINTER_UP, 100);
    result = scene(ui, &state, o, &r, true);
    assert(result.begin_count == 1 && result.request_count == 3 && result.commit_count == 1 &&
           !result.cancel_count);
    assert(r.count == 5 && r.owner == .25 && state.preview == .5 && !state.dragging);
    const cui_slider_phase phases[] = {CUI_SLIDER_BEGIN, CUI_SLIDER_UPDATE, CUI_SLIDER_UPDATE,
                                       CUI_SLIDER_UPDATE, CUI_SLIDER_COMMIT};
    const size_t indices[] = {0, 0, 1, 2, 2};
    for (size_t i = 0; i < 5; ++i) {
        assert(r.events[i].phase == phases[i] && r.events[i].current == .25);
        evidence(&r.events[i], CUI_COMPONENT_EVENT_SOURCE_POINTER, indices[i]);
        assert(r.events[i].input.pointer_id == 7 && r.events[i].input.pointer_device == 9);
        assert(r.events[i].input.pointer_type == CUI_POINTER_TOUCH);
    }
    assert(r.events[0].event.kind == CUI_COMPONENT_EVENT_LIFECYCLE);
    assert(r.events[1].event.kind == CUI_COMPONENT_EVENT_REQUEST);
    assert(r.events[4].event.kind == CUI_COMPONENT_EVENT_COMMIT);
    cui_component_slider_request saved = r.events[4];
    result = scene(ui, &state, o, &r, true);
    assert(state.preview == r.owner && !result.request_count);
    assert(saved.requested == .5 &&
           saved.input.type == CUI_EVENT_POINTER_UP); /* Copy survives frame. */
    r.count = 0;
    r.accept = true;
    cui_focus(ui, 10);
    key(ui, CUI_KEY_RIGHT);
    key(ui, CUI_KEY_RIGHT);
    result = scene(ui, &state, o, &r, true);
    assert(r.count == 6 && result.begin_count == 2 && result.request_count == 2 &&
           result.commit_count == 2);
    assert(r.owner == .375); /* Each key uses immutable entry .25, not mutated owner. */
    for (size_t i = 0; i < 6; ++i) {
        assert(r.events[i].current == .25);
        evidence(&r.events[i], CUI_COMPONENT_EVENT_SOURCE_KEYBOARD, i / 3);
    }
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].value == .25f); /* Same-call semantics retain entry snapshot. */
    scene(ui, &state, o, &r, true);
    assert(state.preview == .375);
    r.count = 0;
    pointer(ui, CUI_EVENT_POINTER_DOWN, 150);
    scene(ui, &state, o, &r, true);
    key(ui, CUI_KEY_ESCAPE);
    result = scene(ui, &state, o, &r, true);
    assert(result.cancel_count == 1 && r.events[r.count - 1].phase == CUI_SLIDER_CANCEL);
    assert(r.events[r.count - 1].requested == .375 && !state.dragging);
    evidence(&r.events[r.count - 1], CUI_COMPONENT_EVENT_SOURCE_KEYBOARD, 0);
    assert(r.events[r.count - 1].event.kind == CUI_COMPONENT_EVENT_LIFECYCLE);
    cui_destroy(ui);
}
static void cancellation_and_validation(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    assert(ui);
    cui_slider_state state = {0};
    recorder r = {.owner = .5};
    cui_slider_options o = {.minimum = 0, .maximum = 1};
    scene(ui, &state, o, &r, true);
    for (unsigned reason = 0; reason < 3; ++reason) {
        o = (cui_slider_options){.minimum = 0, .maximum = 1};
        scene(ui, &state, o, &r, true);
        pointer(ui, CUI_EVENT_POINTER_DOWN, 80);
        scene(ui, &state, o, &r, true);
        assert(state.dragging);
        r.count = 0;
        if (!reason)
            o.disabled = true;
        else if (reason == 1)
            o.readonly = true;
        else
            o.minimum = o.maximum = .5;
        cui_component_slider_result result = scene(ui, &state, o, &r, true);
        assert(result.cancel_count == 1 && r.count == 1 && !state.dragging);
        assert(r.events[0].reason == (cui_slider_event_reason)(reason + 1));
        assert(!r.events[0].has_input && r.events[0].event.flags == 0);
        assert(r.events[0].event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
        /* End the physical contact too; option cancellation only ends reducer
         * drag presentation and does not synthesize a host pointer release. */
        pointer(ui, CUI_EVENT_POINTER_UP, 80);
        scene(ui, &state, o, &r, true);
    }
    o = (cui_slider_options){.minimum = 0, .maximum = 1};
    scene(ui, &state, o, &r, true);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 40);
    scene(ui, &state, o, &r, true);
    pointer(ui, CUI_EVENT_CANCEL, 40);
    r.count = 0;
    cui_component_slider_result result = scene(ui, &state, o, &r, true);
    assert(result.cancel_count == 1 && r.count == 1);
    evidence(&r.events[0], CUI_COMPONENT_EVENT_SOURCE_UNKNOWN, 0);
    assert(!(r.events[0].event.flags & CUI_COMPONENT_EVENT_HAS_SOURCE));
    /* Removing the handler cancels capture silently, just like the old reducer. */
    pointer(ui, CUI_EVENT_POINTER_DOWN, 40);
    scene(ui, &state, o, &r, true);
    r.count = 0;
    result = scene(ui, &state, o, &r, false);
    assert(!state.dragging && !r.count && !result.cancel_count);
    size_t n;
    const cui_semantic *sem = cui_semantics(ui, &n);
    assert(n == 1 && sem[0].details.readonly);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 40);
    result = scene(ui, &state, o, &r, false);
    assert(!state.dragging && !result.request_count);
    assert(cui_begin_frame(ui, 400, 100, 0));
    cui_slider_state before = state;
    result = cui_base_slider_events(ui, 10, cui_string("Position"), NAN, &state, o,
                                    (cui_component_slider_handler){record, &r});
    assert(!result.valid && !memcmp(&before, &state, sizeof state));
    o.user = &r; /* No ambiguous coexistence with legacy userdata, even without callback. */
    result = cui_base_slider_events(ui, 10, cui_string("Position"), .5, &state, o,
                                    (cui_component_slider_handler){record, &r});
    assert(!result.valid && !memcmp(&before, &state, sizeof state));
    cui_end_frame(ui);
    cui_destroy(ui);
}
static void precise_values(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 32, .max_commands = 64});
    assert(ui);
    recorder r = {.owner = 10000000000.125, .accept = true};
    cui_slider_state state = {0};
    cui_slider_options o = {.minimum = 10000000000, .maximum = 10000000001, .step = .125};
    scene(ui, &state, o, &r, true);
    cui_focus(ui, 10);
    key(ui, CUI_KEY_RIGHT);
    cui_component_slider_result result = scene(ui, &state, o, &r, true);
    assert(result.request_count == 1 && result.commit_count == 1 && r.count == 3);
    assert(r.events[1].current == 10000000000.125 && r.events[1].requested == 10000000000.25);
    assert(r.owner == 10000000000.25);
    /* Omission dispatches nothing; explicit reintroduction observes the owner. */
    assert(cui_begin_frame(ui, 400, 100, 0));
    cui_end_frame(ui);
    assert(r.count == 3);
    scene(ui, &state, o, &r, true);
    assert(state.preview == r.owner && r.count == 3);
    cui_destroy(ui);
}
int main(void) {
    phases();
    cancellation_and_validation();
    precise_values();
    puts("PASS slider phases, genuine input metadata, owner snapshots, cancellation and passive "
         "counts");
    return 0;
}
