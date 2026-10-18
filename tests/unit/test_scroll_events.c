/* SPDX-License-Identifier: MIT */
#include "cui/cui_scroll_events.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_scroll_change saved;
static size_t calls;
static void changed(void *user, const cui_scroll_change *event) {
    assert(user == &calls);
    calls++;
    saved = *event;
    assert(event->event.target == 10 && event->event.kind == CUI_COMPONENT_EVENT_CHANGE);
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.frame_sequence &&
           event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
}
static cui_scroll_options options(void) {
    return (cui_scroll_options){.width = 200,
                                .height = 120,
                                .content_width = 400,
                                .content_height = 600,
                                .horizontal = true,
                                .vertical = true,
                                .hide_scrollbars = true};
}
static void boxes(cui_context *ui, cui_scroll_state *s) {
    cui_begin_box(
        ui, 10,
        (cui_box){.width = 200, .height = 120, .clip = true, .offset_x = -s->x, .offset_y = -s->y});
    cui_end_box(ui);
}
static cui_scroll_event_result frame(cui_context *ui, cui_scroll_state *s, cui_scroll_options o,
                                     unsigned passive, bool observe) {
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 250, .height = 160, .inert = passive == 1, .layout_only = passive == 2});
    cui_scroll_event_scope scope = {0};
    (void)cui_base_scrollable_events(ui, 10, s, o, &scope);
    boxes(ui, s);
    cui_scroll_event_result result = cui_base_scrollable_finish_events(
        ui, 10, s, o, &scope, (cui_scroll_handler){observe ? changed : NULL, &calls});
    assert(!cui_base_scrollable_finish_events(ui, 10, s, o, &scope,
                                              (cui_scroll_handler){changed, &calls})
                .valid);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void wheel(cui_context *ui, float dx, float dy, bool precise) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 25,
                                           .y = 25,
                                           .dx = dx,
                                           .dy = dy,
                                           .scroll_precise = precise}));
}
static void test_changes(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_scroll_state s = {0};
    cui_scroll_options o = options();
    calls = 0;
    cui_scroll_event_result r = frame(ui, &s, o, 0, true);
    assert(r.valid && !r.changed && !r.change_count && !r.count_overflow);
    wheel(ui, -1, -2, false);
    wheel(ui, -.25f, -.5f, true);
    r = frame(ui, &s, o, 0, true);
    assert(r.changed && r.change_count == 1 && calls == 1);
    assert(saved.has_previous && saved.previous.x == 0 && saved.previous.y == 0);
    assert(saved.value.x == 48.25f && saved.value.y == 96.5f);
    wheel(ui, 0, -1, true);
    wheel(ui, 0, 1, true);
    r = frame(ui, &s, o, 0, true);
    assert(!r.changed && !r.change_count && calls == 1);
    wheel(ui, 0, -1, true);
    r = frame(ui, &s, o, 0, false);
    assert(r.changed && !r.change_count && calls == 1);
    s.x = 500;
    s.y = -20;
    o.disabled = true;
    r = frame(ui, &s, o, 0, true);
    assert(r.changed && r.change_count == 1 && s.x == 200 && s.y == 0);
    assert(saved.previous.x == 500 && saved.previous.y == -20);
    s.x = NAN;
    s.y = INFINITY;
    r = frame(ui, &s, o, 0, true);
    assert(r.changed && r.change_count == 1 && !saved.has_previous);
    assert(saved.previous.x == 0 && saved.previous.y == 0 && s.x == 0 && s.y == 0);
    for (unsigned passive = 1; passive <= 2; passive++) {
        s.x = 900;
        s.y = -1;
        size_t before = calls;
        r = frame(ui, &s, o, passive, true);
        assert(r.valid && r.changed && !r.change_count && calls == before);
        assert(s.x == 200 && s.y == 0);
    }
    cui_scroll_change copy = saved;
    cui_destroy(ui);
    assert(copy.event.target == 10 && copy.event.frame_sequence && !copy.has_previous);
}
static void test_pair_checks(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    cui_context *other = cui_create((cui_config){.max_elements = 128});
    assert(ui && other);
    cui_scroll_state s = {0}, other_state = {0};
    cui_scroll_event_scope scope = {0};
    cui_scroll_options o = options();
    assert(!cui_base_scrollable_events(ui, 10, &s, o, &scope)); /* Outside frame. */
    assert(cui_begin_frame(ui, 300, 200, .016f));
    assert(!cui_base_scrollable_events(ui, 0, &s, o, &scope));
    assert(
        !cui_base_scrollable_finish_events(ui, 10, &s, o, &scope, (cui_scroll_handler){0}).valid);
    assert(!cui_base_scrollable_events(ui, 10, &s, o, &scope)); /* Valid/no change. */
    cui_scroll_state snapshot = s;
    assert(!cui_base_scrollable_events(ui, 10, &s, o, &scope)); /* Already active. */
    assert(!memcmp(&s, &snapshot, sizeof(s)));
    assert(
        !cui_base_scrollable_finish_events(ui, 11, &s, o, &scope, (cui_scroll_handler){0}).valid);
    assert(
        !cui_base_scrollable_finish_events(ui, 10, &other_state, o, &scope, (cui_scroll_handler){0})
             .valid);
    assert(!cui_base_scrollable_finish_events(other, 10, &s, o, &scope, (cui_scroll_handler){0})
                .valid);
    cui_scroll_event_scope nested = {0};
    assert(!cui_base_scrollable_events(ui, 20, &other_state, o, &nested));
    cui_begin_box(ui, 20, (cui_box){.width = 50, .height = 50});
    cui_end_box(ui);
    assert(
        cui_base_scrollable_finish_events(ui, 20, &other_state, o, &nested, (cui_scroll_handler){0})
            .valid);
    boxes(ui, &s);
    cui_end_frame(ui);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    snapshot = s;
    assert(!cui_base_scrollable_finish_events(ui, 10, &s, o, &scope,
                                              (cui_scroll_handler){changed, &calls})
                .valid); /* Genuine nonzero wrong frame. */
    assert(!memcmp(&s, &snapshot, sizeof(s)));
    cui_end_frame(ui);
    cui_destroy(ui);
    cui_destroy(other);
}
static void test_legacy_equivalence(void) {
    cui_context *old = cui_create((cui_config){.max_elements = 128});
    cui_context *fresh = cui_create((cui_config){.max_elements = 128});
    assert(old && fresh);
    cui_scroll_state a = {0}, b = {0};
    cui_scroll_options o = options();
    for (unsigned pass = 0; pass < 4; pass++) {
        if (pass) {
            wheel(old, -1, -1, pass == 2);
            wheel(fresh, -1, -1, pass == 2);
        }
        if (pass == 3) {
            o.content_height = 130;
            o.content_width = 210;
        }
        assert(cui_begin_frame(old, 300, 200, .016f));
        assert(cui_begin_frame(fresh, 300, 200, .016f));
        cui_scroll_event_scope scope = {0};
        assert(cui_base_scrollable(old, 10, &a, o) ==
               cui_base_scrollable_events(fresh, 10, &b, o, &scope));
        boxes(old, &a);
        boxes(fresh, &b);
        (void)cui_base_scrollable_finish(old, 10, &a, o);
        assert(cui_base_scrollable_finish_events(fresh, 10, &b, o, &scope, (cui_scroll_handler){0})
                   .valid);
        assert(!memcmp(&a, &b, sizeof(a)));
        size_t n1, n2;
        const cui_semantic *m1 = cui_semantics(old, &n1), *m2 = cui_semantics(fresh, &n2);
        assert(n1 == n2 && n1 == 1);
        assert(m1[0].id == m2[0].id && m1[0].details.scroll_x == m2[0].details.scroll_x &&
               m1[0].details.scroll_y == m2[0].details.scroll_y);
        if (pass)
            assert(cui_event_consumed(old, 0) == cui_event_consumed(fresh, 0));
        cui_end_frame(old);
        cui_end_frame(fresh);
        assert(cui_last_error(old) == CUI_OK && cui_last_error(fresh) == CUI_OK);
    }
    cui_destroy(old);
    cui_destroy(fresh);
}
static void test_deferred(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_scroll_state s = {0};
    cui_scroll_options o = options();
    calls = 0;
    for (unsigned pass = 0; pass < 2; pass++) {
        if (pass)
            wheel(ui, 0, -.5f, true);
        assert(cui_begin_frame(ui, 300, 200, .016f));
        cui_scroll_event_scope scope = {0};
        (void)cui_base_scrollable_events(ui, 10, &s, o, &scope);
        cui_begin_box(ui, 10, (cui_box){.width = 200, .height = 120, .clip = true});
        cui_begin_box(ui, 20, (cui_box){.width = 180, .height = 100});
        cui_interact(ui, 20, CUI_ROLE_GROUP, cui_string("Child scroll"), false, false, 0);
        cui_set_semantic_details(ui, 20,
                                 (cui_semantic_details){.scrollable = true, .scroll_max_y = 100});
        cui_end_box(ui);
        if (pass) {
            assert(cui_event_target(ui, 0) == 20);
            assert(cui_defer_scroll_event(ui, 0, 20));
        }
        if (pass) {
            assert(s.y == 0 && cui_event_target(ui, 0) == 10 && calls == 0);
        }
        cui_scroll_event_result r = cui_base_scrollable_finish_events(
            ui, 10, &s, o, &scope, (cui_scroll_handler){changed, &calls});
        assert(r.valid && r.change_count == pass);
        if (pass)
            assert(s.y == .5f && calls == 1 && cui_event_consumed(ui, 0));
        cui_end_box(ui);
        cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}

static cui_scroll_event_result bar_frame(cui_context *ui, cui_scroll_state *state) {
    cui_scroll_options o = options();
    o.hide_scrollbars = false;
    o.horizontal = false;
    cui_scroll_geometry geometry = cui_scroll_layout(o);
    assert(cui_begin_frame(ui, 300, 200, .016f));
    cui_scroll_event_scope scope = {0};
    (void)cui_base_scrollable_events(ui, 10, state, o, &scope);
    cui_begin_box(ui, 10, (cui_box){.width = 200, .height = 120, .row = true});
    cui_begin_box(ui, 0,
                  (cui_box){.width = geometry.viewport.width, .height = geometry.viewport.height});
    cui_end_box(ui);
    cui_begin_box(ui, cui_scrollbar_id(10, true),
                  (cui_box){.width = 10, .height = geometry.viewport.height});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_scroll_event_result result = cui_base_scrollable_finish_events(
        ui, 10, state, o, &scope, (cui_scroll_handler){changed, &calls});
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void test_bar(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    calls = 0;
    cui_scroll_state state = {0};
    assert(!bar_frame(ui, &state).changed);
    cui_id bar = cui_scrollbar_id(10, true);
    cui_focus(ui, bar);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
    assert(bar_frame(ui, &state).change_count == 1 && state.y == 480);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    assert(bar_frame(ui, &state).change_count == 1 && state.y == 0);
    cui_rect bounds = cui_bounds(ui, bar);
    assert(bounds.w == 10 && bounds.h == 120);
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 2, .y = bounds.y + 2}));
    assert(!bar_frame(ui, &state).changed && state.dragging_y);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = bounds.x + 2,
                                           .y = bounds.y + bounds.h - 1}));
    assert(bar_frame(ui, &state).change_count == 1 && state.y == 480);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = bounds.x + 2,
                                           .y = bounds.y + bounds.h - 1}));
    assert(!bar_frame(ui, &state).changed && !state.dragging_y);
    cui_destroy(ui);
}
int main(void) {
    test_changes();
    test_pair_checks();
    test_legacy_equivalence();
    test_deferred();
    test_bar();
    return 0;
}
