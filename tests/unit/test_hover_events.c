/* SPDX-License-Identifier: MIT */
#include "cui/cui_hover_events.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
static size_t calls;
static cui_hover_lifecycle_event saved;
static cui_hover_state *active;
static void observed(void *user, const cui_hover_lifecycle_event *event) {
    assert(user == &calls);
    assert(active && event->open == active->open);
    assert(event->previous_open != event->open);
    assert(event->event.target == UINT64_C(0xfedcba9876543210));
    assert(event->event.kind == CUI_COMPONENT_EVENT_LIFECYCLE);
    assert(event->event.flags == 0 && event->event.frame_sequence == 0 &&
           event->event.event_index == 0);
    assert(event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    cui_activation_snapshot empty;
    memset(&empty, 0, sizeof(empty));
    assert(!memcmp(&event->event.activation, &empty, sizeof(empty)));
    saved = *event;
    calls++;
}
static cui_hover_event_result update(cui_hover_state *state, bool trigger, bool content, bool focus,
                                     bool escape, bool disabled, float dt, float opening,
                                     float closing, bool observer) {
    active = state;
    return cui_hover_update_events(UINT64_C(0xfedcba9876543210), state, trigger, content, focus,
                                   escape, disabled, dt, opening, closing,
                                   (cui_hover_handler){observer ? observed : NULL, &calls});
}
static void same_state(cui_hover_state a, cui_hover_state b) {
    assert(a.open == b.open && a.pending == b.pending && a.target == b.target &&
           a.suppressed == b.suppressed);
    assert(a.elapsed == b.elapsed || (isnan(a.elapsed) && isnan(b.elapsed)));
}
static void test_lifecycle(void) {
    cui_hover_state state = {0};
    calls = 0;
    cui_hover_event_result r =
        update(&state, true, false, false, false, false, .1f, .3f, .2f, true);
    assert(r.valid && !r.control_result && !r.change_count && !state.open && state.pending);
    r = update(&state, true, false, false, false, false, .21f, .3f, .2f, true);
    assert(r.control_result && r.change_count == 1 && !r.count_overflow && state.open);
    assert(!saved.previous_open && saved.open && calls == 1);
    r = update(&state, false, true, false, false, false, 10, .3f, .2f, true);
    assert(!r.control_result && !r.change_count && state.open && calls == 1);
    r = update(&state, false, false, false, false, false, .1f, .3f, .2f, true);
    assert(!r.control_result && state.pending && state.open);
    r = update(&state, true, false, false, false, false, .1f, .3f, .2f, true);
    assert(!r.control_result && !state.pending && state.elapsed == 0);
    r = update(&state, false, false, false, true, false, 0, .3f, .2f, true);
    assert(r.control_result && r.change_count == 1 && !state.open && state.suppressed);
    assert(saved.previous_open && !saved.open && calls == 2);
    r = update(&state, true, true, true, false, false, 100, 0, 0, true);
    assert(!r.control_result && !state.open && calls == 2); /* Suppressed until leave. */
    r = update(&state, false, false, false, false, false, 0, 0, 0, true);
    assert(!r.control_result && !state.suppressed);
    r = update(&state, false, false, true, false, false, 0, 0, 0, true);
    assert(r.change_count == 1 && state.open && calls == 3);
    r = update(&state, true, true, true, false, true, 0, 0, 0, true);
    assert(r.change_count == 1 && !state.open && !state.suppressed && calls == 4);
    r = update(&state, false, false, false, false, true, 0, 0, 0, true);
    assert(!r.control_result && !r.change_count && calls == 4);
    r = update(&state, true, false, false, false, false, 0, 0, 0, false);
    assert(r.valid && r.control_result && !r.change_count && state.open && calls == 4);
    r = update(&state, false, false, false, false, false, .2f, .3f, .2f, true);
    assert(r.control_result && r.change_count == 1 && calls == 5);
}
static void test_legacy_equivalence(void) {
    const float timing[] = {0, .1f, .21f, -1, NAN, INFINITY};
    const float delays[] = {0, .2f, -1, NAN, INFINITY};
    cui_hover_state old = {0}, fresh = {0};
    for (unsigned pass = 0; pass < 3; pass++)
        for (unsigned flags = 0; flags < 32; flags++)
            for (size_t t = 0; t < sizeof(timing) / sizeof(timing[0]); t++) {
                float dt = timing[t], opening = delays[(flags + pass) % 5],
                      closing = delays[(t + pass) % 5];
                bool trigger = flags & 1, content = flags & 2, focus = flags & 4;
                bool escape = flags & 8, disabled = flags & 16;
                bool expected = cui_hover_update(&old, trigger, content, focus, escape, disabled,
                                                 dt, opening, closing);
                size_t before = calls;
                bool observer = (t % 2) != 0;
                cui_hover_event_result r = update(&fresh, trigger, content, focus, escape, disabled,
                                                  dt, opening, closing, observer);
                assert(r.valid && r.control_result == expected && !r.count_overflow);
                assert(r.change_count == (size_t)(expected && observer));
                assert(calls == before + r.change_count);
                same_state(old, fresh);
            }
}
static void test_validation_and_lifetime(void) {
    cui_hover_state state = {
        .open = true, .pending = true, .target = false, .suppressed = true, .elapsed = .125f};
    cui_hover_state before = state;
    size_t count = calls;
    cui_hover_event_result r = cui_hover_update_events(0, &state, true, true, true, true, true, 10,
                                                       0, 0, (cui_hover_handler){observed, &calls});
    assert(!r.valid && !r.control_result && !r.change_count);
    same_state(state, before);
    r = cui_hover_update_events(10, NULL, true, true, true, true, true, 10, 0, 0,
                                (cui_hover_handler){observed, &calls});
    assert(!r.valid && !r.control_result && !r.change_count && calls == count);
    cui_hover_state *owned = calloc(1, sizeof(*owned));
    assert(owned);
    r = update(owned, true, false, false, false, false, 0, 0, 0, true);
    assert(r.change_count == 1);
    cui_hover_lifecycle_event copy = saved;
    free(owned);
    active = NULL;
    assert(copy.open && !copy.previous_open && copy.event.flags == 0);
    assert(copy.event.target == UINT64_C(0xfedcba9876543210));
}
int main(void) {
    test_lifecycle();
    test_legacy_equivalence();
    test_validation_and_lifetime();
    return 0;
}
