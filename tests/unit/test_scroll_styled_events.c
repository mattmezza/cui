/* SPDX-License-Identifier: MIT */
#include "cui/cui_scroll_events.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static size_t calls;
static bool content_finished;
static cui_scroll_change saved;
static void changed(void *user, const cui_scroll_change *event) {
    assert(user == &calls && content_finished);
    assert(event->event.target == 10 && event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.kind == CUI_COMPONENT_EVENT_CHANGE && event->event.frame_sequence);
    assert(event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    saved = *event;
    calls++;
}
static cui_scroll_options options(void) {
    return (cui_scroll_options){.width = 200,
                                .height = 120,
                                .content_width = 400,
                                .content_height = 600,
                                .horizontal = true,
                                .vertical = true};
}
typedef struct {
    cui_draw_list draw;
    cui_scroll_event_result event;
} scene_result;
static scene_result scene(cui_context *ui, cui_scroll_state *state, cui_scroll_options o,
                          bool legacy, bool observer, unsigned passive, bool deferred) {
    cui_theme theme = cui_theme_light();
    scene_result r;
    memset(&r, 0, sizeof(r));
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_begin_box(ui, 1,
                  (cui_box){.width = 350,
                            .height = 200,
                            .row = true,
                            .inert = passive == 1,
                            .layout_only = passive == 2});
    cui_scroll_event_scope scope = {0};
    content_finished = false;
    size_t before = calls;
    assert(legacy ? cui_scrollable_begin(ui, 10, &theme, state, o)
                  : cui_scrollable_begin_events(ui, 10, &theme, state, o, &scope));
    assert(calls == before);
    cui_begin_box(ui, 20, (cui_box){.width = 100, .height = 80, .background = {90, 20, 180, 255}});
    if (deferred) {
        cui_interact(ui, 20, CUI_ROLE_GROUP, cui_string("Nested"), false, false, 0);
        cui_set_semantic_details(ui, 20,
                                 (cui_semantic_details){.scrollable = true, .scroll_max_y = 100});
        size_t count;
        cui_frame_events(ui, &count);
        if (count) {
            assert(cui_event_target(ui, 0) == 20);
            assert(cui_defer_scroll_event(ui, 0, 20));
        }
    }
    cui_end_box(ui);
    content_finished = true;
    if (legacy)
        cui_scrollable_end_ex(ui, 10, &theme, state, o);
    else {
        r.event =
            cui_scrollable_end_events(ui, 10, &theme, state, o, &scope,
                                      (cui_scroll_handler){observer ? changed : NULL, &calls});
        assert(r.event.valid && calls == before + r.event.change_count);
        assert(!cui_scrollable_end_events(ui, 10, &theme, state, o, &scope,
                                          (cui_scroll_handler){changed, &calls})
                    .valid);
    }
    cui_begin_box(ui, 30, (cui_box){.width = 25, .height = 20, .background = {10, 40, 70, 255}});
    cui_end_box(ui);
    cui_end_box(ui);
    r.draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_bounds(ui, 30).x == 200); /* New end closed the owner, not just content. */
    return r;
}
static void wheel(cui_context *ui, float dy) {
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_SCROLL, .x = 25, .y = 25, .dy = dy, .scroll_precise = true}));
}
static void compare(cui_context *a, cui_context *b, scene_result x, scene_result y) {
    assert(x.draw.count == y.draw.count);
    for (size_t i = 0; i < x.draw.count; i++) {
        const cui_draw_command *p = &x.draw.commands[i], *q = &y.draw.commands[i];
        assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof(p->bounds)));
        assert(!memcmp(&p->color, &q->color, sizeof(p->color)) && p->radius == q->radius);
    }
    assert(!memcmp(&(cui_rect){cui_bounds(a, 20).x, cui_bounds(a, 20).y, cui_bounds(a, 20).w,
                               cui_bounds(a, 20).h},
                   &(cui_rect){cui_bounds(b, 20).x, cui_bounds(b, 20).y, cui_bounds(b, 20).w,
                               cui_bounds(b, 20).h},
                   sizeof(cui_rect)));
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a, &na), *sb = cui_semantics(b, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; i++) {
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role);
        assert(sa[i].details.scroll_x == sb[i].details.scroll_x &&
               sa[i].details.scroll_y == sb[i].details.scroll_y);
    }
}
static void test_equivalence(void) {
    cui_context *old = cui_create((cui_config){.max_elements = 128});
    cui_context *fresh = cui_create((cui_config){.max_elements = 128});
    assert(old && fresh);
    cui_scroll_state a = {0}, b = {0};
    cui_scroll_options o = options();
    calls = 0;
    for (unsigned pass = 0; pass < 4; pass++) {
        if (pass == 1) {
            wheel(old, -.5f);
            wheel(fresh, -.5f);
        }
        if (pass == 2) {
            wheel(old, -1);
            wheel(old, 1);
            wheel(fresh, -1);
            wheel(fresh, 1);
        }
        if (pass == 3) {
            o.disabled = true;
            a.y = b.y = 1000;
        }
        scene_result x = scene(old, &a, o, true, false, 0, false);
        scene_result y = scene(fresh, &b, o, false, true, 0, false);
        compare(old, fresh, x, y);
        assert(!memcmp(&a, &b, sizeof(a)));
        assert(y.event.change_count == (pass == 1 || pass == 3));
        if (pass == 1)
            assert(cui_event_consumed(old, 0) && cui_event_consumed(fresh, 0));
    }
    o.disabled = false;
    for (unsigned passive = 1; passive <= 2; passive++) {
        a.y = b.y = INFINITY;
        size_t before = calls;
        scene_result x = scene(old, &a, o, true, false, passive, false);
        scene_result y = scene(fresh, &b, o, false, true, passive, false);
        compare(old, fresh, x, y);
        assert(y.event.changed && !y.event.change_count && calls == before);
        assert(a.y == 0 && b.y == 0);
    }
    a.y = b.y = 0;
    scene(old, &a, o, true, false, 0, true);
    scene(fresh, &b, o, false, true, 0, true);
    wheel(old, -.5f);
    wheel(fresh, -.5f);
    scene_result x = scene(old, &a, o, true, false, 0, true);
    scene_result y = scene(fresh, &b, o, false, true, 0, true);
    compare(old, fresh, x, y);
    assert(y.event.change_count == 1 && a.y == .5f && b.y == .5f);
    assert(saved.previous.y == 0 && saved.value.y == .5f);
    cui_scroll_change copy = saved;
    cui_destroy(old);
    cui_destroy(fresh);
    assert(copy.event.frame_sequence && copy.event.target == 10 && copy.value.y == .5f);
}
static void test_layers(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_scroll_state state = {0};
    cui_scroll_options o = options();
    cui_theme theme = cui_theme_light();
    cui_scroll_event_scope scope = {0};
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(!cui_scrollable_begin_events(ui, 0, &theme, &state, o, &scope));
    assert(!cui_scrollable_end_events(ui, 10, &theme, &state, o, &scope, (cui_scroll_handler){0})
                .valid);
    assert(cui_scrollable_begin_events(ui, 10, &theme, &state, o, &scope));
    assert(!cui_scrollable_begin_events(ui, 10, &theme, &state, o, &scope));
    assert(!cui_base_scrollable_finish_events(ui, 10, &state, o, &scope, (cui_scroll_handler){0})
                .valid);
    assert(!cui_scrollable_end_events(ui, 11, &theme, &state, o, &scope, (cui_scroll_handler){0})
                .valid);
    assert(cui_scrollable_end_events(ui, 10, &theme, &state, o, &scope, (cui_scroll_handler){0})
               .valid);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_begin_frame(ui, 400, 300, .016f));
    (void)cui_base_scrollable_events(ui, 10, &state, o, &scope);
    assert(!cui_scrollable_end_events(ui, 10, &theme, &state, o, &scope, (cui_scroll_handler){0})
                .valid);
    cui_begin_box(ui, 10, (cui_box){.width = 200, .height = 120});
    cui_end_box(ui);
    assert(cui_base_scrollable_finish_events(ui, 10, &state, o, &scope, (cui_scroll_handler){0})
               .valid);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void test_keyboard_drag(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    cui_scroll_state state = {0};
    cui_scroll_options o = options();
    calls = 0;
    scene(ui, &state, o, false, true, 0, false);
    cui_id bar = cui_scrollbar_id(10, true);
    cui_focus(ui, bar);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
    assert(scene(ui, &state, o, false, true, 0, false).event.change_count == 1 && state.y == 490);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    assert(scene(ui, &state, o, false, true, 0, false).event.change_count == 1 && state.y == 0);
    cui_rect bar_bounds = cui_bounds(ui, bar);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = bar_bounds.x + 2, .y = bar_bounds.y + 2}));
    assert(!scene(ui, &state, o, false, true, 0, false).event.changed && state.dragging_y);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = bar_bounds.x + 2,
                                           .y = bar_bounds.y + bar_bounds.h - 1}));
    assert(scene(ui, &state, o, false, true, 0, false).event.change_count == 1 && state.y == 490);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = bar_bounds.x + 2,
                                           .y = bar_bounds.y + bar_bounds.h - 1}));
    assert(!scene(ui, &state, o, false, true, 0, false).event.changed && !state.dragging_y);
    wheel(ui, .5f);
    assert(!scene(ui, &state, o, false, false, 0, false).event.change_count);
    cui_destroy(ui);
}
int main(void) {
    test_equivalence();
    test_layers();
    test_keyboard_drag();
    return 0;
}
