/* SPDX-License-Identifier: MIT */
#include "cui/cui_scroll.h"
#ifndef CUI_SCROLL_BASE_ONLY
#include "cui/cui_base.h"
#include "cui/cui_resizable.h"
#endif
#include <assert.h>
#include <math.h>
static cui_draw_list scene(cui_context *ui, const cui_theme *theme, cui_scroll_state *s,
                           cui_scroll_options o) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
#ifdef CUI_SCROLL_BASE_ONLY
    (void)theme;
    cui_base_scrollable(ui, 1, s, o);
    cui_scroll_geometry g = cui_scroll_layout(o);
    cui_begin_box(ui, 1, (cui_box){.width = o.width, .height = o.height});
    cui_begin_box(ui, 0, (cui_box){.width = -1, .height = g.viewport.height, .row = true});
    cui_begin_box(ui, 20,
                  (cui_box){.width = g.viewport.width,
                            .height = g.viewport.height,
                            .clip = true,
                            .offset_x = -s->x,
                            .offset_y = -s->y});
    cui_begin_box(ui, 0, (cui_box){.width = o.content_width, .height = o.content_height});
#else
    assert(cui_scrollable_begin(ui, 1, theme, s, o));
#endif
    for (unsigned i = 0; i < 10; ++i) {
        cui_begin_box(ui, 100 + i, (cui_box){.width = 180, .height = 40});
        cui_interact(ui, 100 + i, CUI_ROLE_BUTTON, cui_string("Row"), false, false, 0);
        cui_end_box(ui);
    }
#ifdef CUI_SCROLL_BASE_ONLY
    cui_end_box(ui);
    cui_end_box(ui);
    if (o.vertical && !o.hide_scrollbars) {
        cui_begin_box(ui, cui_scrollbar_id(1, true),
                      (cui_box){.width = o.width - g.viewport.width, .height = g.viewport.height});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    if (o.horizontal && !o.hide_scrollbars) {
        cui_begin_box(ui, cui_scrollbar_id(1, false),
                      (cui_box){.width = g.viewport.width, .height = o.height - g.viewport.height});
        cui_end_box(ui);
    }
    cui_end_box(ui);
#else
    cui_scrollable_end(ui, 1, theme, s, o);
#endif
    cui_draw_list list = cui_end_frame(ui);
#ifdef CUI_SCROLL_BASE_ONLY
    for (size_t i = 0; i < list.count; ++i)
        assert(list.commands[i].type == CUI_DRAW_CLIP_PUSH ||
               list.commands[i].type == CUI_DRAW_CLIP_POP);
#endif
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static void key(cui_context *ui, cui_key key) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
}
static void pointer(cui_context *ui, cui_event_type type, float x, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = type, .x = x, .y = y}));
}
static void behavior(void) {
#ifdef CUI_SCROLL_BASE_ONLY
    cui_theme themes[4] = {0};
#else
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
#endif
    for (unsigned i = 0; i < 4; ++i) {
        cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
        assert(ui);
        cui_scroll_state s = {0};
        cui_scroll_options o = {.width = 200,
                                .height = 120,
                                .content_width = 400,
                                .content_height = 400,
                                .horizontal = true,
                                .vertical = true};
        cui_scroll_geometry g = cui_scroll_layout(o);
        assert(g.viewport.width == 190 && g.viewport.height == 110 && g.maximum_x == 210 &&
               g.maximum_y == 290);
#ifdef CUI_SCROLL_BASE_ONLY
        assert(cui_begin_frame(ui, 400, 300, .016f));
        cui_base_scrollable(ui, 1, &s, o);
        assert(cui_end_frame(ui).count == 0);
#endif
        scene(ui, &themes[i], &s, o);
        assert(cui_queue_event(
            ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 50, .y = 50, .dy = -1, .dx = -2}));
        scene(ui, &themes[i], &s, o);
        assert(s.x == 96 && s.y == 48);
        cui_rect row = cui_bounds(ui, 102);
        assert(row.x == -96 && row.y == 32);
        for (unsigned precise = 0; precise < 2; ++precise) {
            float delta = precise ? .5f : -.5f;
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                                   .x = 50,
                                                   .y = 50,
                                                   .dx = delta,
                                                   .dy = delta,
                                                   .scroll_precise = true,
                                                   .scroll_phase = CUI_SCROLL_PHASE_MOVED}));
            scene(ui, &themes[i], &s, o);
            assert(s.x == (precise ? 96 : 96.5f) && s.y == (precise ? 48 : 48.5f));
        }
        pointer(ui, CUI_EVENT_POINTER_DOWN, 10, 10);
        scene(ui, &themes[i], &s, o);
        assert(cui_focused(ui) == 101); /* Row 0 is outside the clipped viewport. */
        pointer(ui, CUI_EVENT_POINTER_UP, 10, 10);
        scene(ui, &themes[i], &s, o);
        cui_id bar = cui_scrollbar_id(1, true);
        cui_focus(ui, bar);
        key(ui, CUI_KEY_END);
        scene(ui, &themes[i], &s, o);
        assert(s.y == g.maximum_y);
        key(ui, CUI_KEY_PAGEUP);
        scene(ui, &themes[i], &s, o);
        assert(s.y == 180);
        key(ui, CUI_KEY_HOME);
        scene(ui, &themes[i], &s, o);
        assert(s.y == 0);
        cui_rect bounds = cui_bounds(ui, bar);
        cui_rect thumb =
            cui_scrollbar_thumb(bounds, s.y, g.viewport.height, o.content_height, true);
        pointer(ui, CUI_EVENT_POINTER_DOWN, thumb.x + 2, thumb.y + 2);
        scene(ui, &themes[i], &s, o);
        assert(s.dragging_y);
        pointer(ui, CUI_EVENT_POINTER_MOVE, thumb.x + 2, bounds.y + bounds.h - 1);
        scene(ui, &themes[i], &s, o);
        assert(s.y == g.maximum_y);
        pointer(ui, CUI_EVENT_POINTER_UP, thumb.x + 2, bounds.y + bounds.h - 1);
        scene(ui, &themes[i], &s, o);
        assert(!s.dragging_y);
        size_t count;
        const cui_semantic *nodes = cui_semantics(ui, &count);
        assert(nodes[0].details.scrollable && nodes[0].details.scroll_y == s.y &&
               nodes[0].details.scroll_max_y == 290);
        assert(nodes[2].role == CUI_ROLE_SCROLLBAR &&
               nodes[2].details.orientation == CUI_ORIENTATION_VERTICAL &&
               nodes[2].details.maximum == 290);
        o.disabled = true;
        float old = s.y;
        key(ui, CUI_KEY_HOME);
        scene(ui, &themes[i], &s, o);
        assert(s.y == old);
        s.x = NAN;
        s.y = INFINITY;
        o.content_width = o.content_height = 1;
        scene(ui, &themes[i], &s, o);
        assert(s.x == 0 && s.y == 0);
        cui_destroy(ui);
    }
}
static void consumed_wheel(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_scroll_state state = {0};
    cui_scroll_options options = {
        .width = 200, .height = 120, .content_height = 400, .vertical = true};
    cui_theme theme = {0};
    scene(ui, &theme, &state, options);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 50, .y = 50, .dy = -1}));
    assert(cui_begin_frame(ui, 400, 300, 0));
    assert(cui_event_target(ui, 0) == 1 && cui_consume_event(ui, 0));
    cui_base_scrollable(ui, 1, &state, options);
    cui_end_frame(ui);
    assert(state.y == 0);
    cui_destroy(ui);
}

static void deferred_frame(cui_context *ui, cui_scroll_state *s, bool defer) {
    cui_scroll_options o = {.width = 200, .height = 120, .content_height = 400, .vertical = true};
    assert(cui_begin_frame(ui, 400, 300, .016f));
#ifdef CUI_SCROLL_BASE_ONLY
    cui_base_scrollable(ui, 1, s, o);
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 120});
    cui_begin_box(ui, 20, (cui_box){.width = 190, .height = 120, .clip = true, .offset_y = -s->y});
#else
    cui_theme theme = cui_theme_light();
    assert(cui_scrollable_begin(ui, 1, &theme, s, o));
#endif
    cui_begin_box(ui, 30, (cui_box){.width = 100, .height = 80});
    cui_interact(ui, 30, CUI_ROLE_GROUP, cui_string("Child scroll"), false, false, 0);
    cui_set_semantic_details(ui, 30,
                             (cui_semantic_details){.scrollable = true, .scroll_max_y = 100});
    cui_end_box(ui);
    if (defer) {
        assert(s->y == 0 && cui_event_target(ui, 0) == 30);
        assert(cui_defer_scroll_event(ui, 0, 30));
    }
#ifdef CUI_SCROLL_BASE_ONLY
    cui_base_scrollable_finish(ui, 1, s, o);
    assert(cui_set_box_scroll_offset(ui, 20, 0, -s->y));
    cui_end_box(ui);
    cui_end_box(ui);
#else
    cui_scrollable_end_ex(ui, 1, &theme, s, o);
#endif
    if (defer) {
        assert(s->y == .5f && cui_event_consumed(ui, 0));
        assert(!cui_base_scrollable_finish(ui, 1, s, o)); /* No replay. */
        assert(s->y == .5f);
    }
    assert(!cui_set_box_scroll_offset(ui, 30, 0, 1)); /* Not clipped. */
    assert(!cui_set_box_scroll_offset(ui, 999, 0, 1));
    cui_end_frame(ui);
    assert(fabsf(cui_bounds(ui, 30).y + s->y) < .001f);
    assert(!cui_set_box_scroll_offset(ui, 20, 0, 1)); /* Outside frame. */
    assert(cui_last_error(ui) == CUI_OK);
}
static void deferred_scroll(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_scroll_state s = {0};
    deferred_frame(ui, &s, false);
    assert(cui_queue_event(
        ui, (cui_event){
                .type = CUI_EVENT_SCROLL, .x = 50, .y = 50, .dy = -.5f, .scroll_precise = true}));
    deferred_frame(ui, &s, true);
    cui_destroy(ui);
}
static void hidden_tracks(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(ui);
    cui_theme theme = {0};
    cui_scroll_state state = {.dragging_x = true, .dragging_y = true};
    cui_scroll_options o = {.width = 200,
                            .height = 100,
                            .content_width = 600,
                            .content_height = 400,
                            .horizontal = true,
                            .hide_scrollbars = true,
                            .axis_lock = true};
    cui_scroll_geometry g = cui_scroll_layout(o);
    assert(g.viewport.width == 200 && g.viewport.height == 100 && g.maximum_x == 400);
    scene(ui, &theme, &state, o);
    assert(!state.dragging_x && !state.dragging_y);
    size_t count;
    const cui_semantic *sem = cui_semantics(ui, &count);
    bool found = false;
    for (size_t i = 0; i < count; i++) {
        assert(sem[i].id != cui_scrollbar_id(1, true));
        assert(sem[i].id != cui_scrollbar_id(1, false));
        if (sem[i].id == 1) {
            found = true;
            assert(sem[i].details.scroll_axis_lock);
            assert(sem[i].details.orientation == CUI_ORIENTATION_HORIZONTAL);
        }
    }
    assert(found);
    assert(cui_queue_event(
        ui, (cui_event){
                .type = CUI_EVENT_SCROLL, .x = 50, .y = 50, .dx = -25, .scroll_precise = true}));
    scene(ui, &theme, &state, o);
    assert(state.x == 25 && cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 50,
                                           .y = 50,
                                           .dx = -2,
                                           .dy = -25,
                                           .scroll_precise = true}));
    scene(ui, &theme, &state, o);
    assert(state.x == 25 && !cui_event_consumed(ui, 0));
    /* Finish must preserve the routing policy rather than replace it with zeroes. */
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_base_scrollable(ui, 1, &state, o);
    cui_begin_box(ui, 1, (cui_box){.width = 200, .height = 100});
    cui_base_scrollable_finish(ui, 1, &state, o);
    cui_end_box(ui);
    cui_end_frame(ui);
    sem = cui_semantics(ui, &count);
    assert(count == 1 && sem[0].details.scroll_axis_lock &&
           sem[0].details.orientation == CUI_ORIENTATION_HORIZONTAL);
    o.horizontal = false;
    o.vertical = true;
    scene(ui, &theme, &state, o);
    sem = cui_semantics(ui, &count);
    assert(sem[0].details.orientation == CUI_ORIENTATION_VERTICAL);
    cui_destroy(ui);
}
static void horizontal_mask_edges(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(ui);
    cui_theme theme = {0};
    cui_scroll_state state = {0};
    cui_scroll_options o = {.width = 200,
                            .height = 100,
                            .content_width = 200,
                            .content_height = 100,
                            .horizontal = true,
                            .hide_scrollbars = true,
                            .axis_lock = true,
                            .trap_horizontal = true};
    for (int overflow = 0; overflow < 2; overflow++) {
        o.content_width = overflow ? 600 : 200;
        state.x = overflow ? 400 : 0;
        scene(ui, &theme, &state, o);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .x = 50,
                                               .y = 50,
                                               .dx = -40,
                                               .dy = -10,
                                               .scroll_precise = true}));
        scene(ui, &theme, &state, o);
        assert(state.x == (overflow ? 400 : 0));
        assert(cui_event_target(ui, 0) == 1 && cui_event_consumed(ui, 0));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .x = 50,
                                               .y = 50,
                                               .dx = -10,
                                               .dy = -40,
                                               .scroll_precise = true}));
        scene(ui, &theme, &state, o);
        assert(!cui_event_consumed(ui, 0));
    }
    o.trap_horizontal = false;
    scene(ui, &theme, &state, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 50,
                                           .y = 50,
                                           .dx = -40,
                                           .dy = -10,
                                           .scroll_precise = true}));
    scene(ui, &theme, &state, o);
    assert(!cui_event_consumed(ui, 0));
    cui_destroy(ui);
}
static void gesture_filter(void) {
    cui_scroll_gesture state = {0};
    cui_event e = {.type = CUI_EVENT_SCROLL, .dx = -40, .dy = -10, .scroll_precise = true};
    assert(cui_scroll_gesture_filter(&state, &e, 1));
    assert(e.dx == -40 && e.dy == 0);
    e.dx = -10;
    e.dy = -15;
    assert(cui_scroll_gesture_filter(&state, &e, 1.01));
    assert(e.dx == -10 && e.dy == 0);
    e.dx = -10;
    e.dy = -25;
    assert(cui_scroll_gesture_filter(&state, &e, 1.02));
    assert(e.dx == -10 && e.dy == -25 && !state.locked);
    e.dx = -40;
    e.dy = -10;
    assert(cui_scroll_gesture_filter(&state, &e, 1.03));
    assert(e.dy == -10); /* Released gestures do not relock without a boundary. */
    assert(cui_scroll_gesture_filter(&state, &e, 1.07));
    assert(e.dy == 0 && state.locked);
    e.scroll_phase = CUI_SCROLL_PHASE_ENDED;
    assert(cui_scroll_gesture_filter(&state, &e, 1.08));
    assert(!state.has_last);
    e.scroll_phase = CUI_SCROLL_PHASE_MOVED;
    e.dx = -3;
    e.dy = -3;
    assert(cui_scroll_gesture_filter(&state, &e, 1.09));
    assert(e.dx == 0 && e.dy == -3); /* Ties are vertical. */
    e.dx = -5;
    e.dy = -1;
    assert(cui_scroll_gesture_filter(&state, &e, 1.10));
    assert(e.dx == 0); /* Sub6px jitter cannot unlock. */
    e.scroll_phase = CUI_SCROLL_PHASE_CANCELLED;
    assert(cui_scroll_gesture_filter(&state, &e, 1.11) && !state.has_last);
    e.scroll_phase = CUI_SCROLL_PHASE_NONE;
    e.scroll_precise = false;
    e.dx = -40;
    e.dy = -10;
    assert(cui_scroll_gesture_filter(&state, &e, 1.12));
    assert(e.dx == -40 && e.dy == -10 && !state.has_last);
    assert(!cui_scroll_gesture_filter(&state, &e, NAN));
    assert(e.dx == -40 && e.dy == -10 && !state.has_last);
}
static void persistent_owner(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(ui);
    cui_theme theme = {0};
    cui_scroll_state state = {0};
    cui_scroll_options o = {.width = 200,
                            .height = 100,
                            .content_width = 600,
                            .content_height = 100,
                            .horizontal = true,
                            .hide_scrollbars = true,
                            .axis_lock = true,
                            .trap_horizontal = true,
                            .gesture_lock = true};
    scene(ui, &theme, &state, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 50,
                                           .y = 50,
                                           .dx = -40,
                                           .dy = -10,
                                           .scroll_precise = true}));
    scene(ui, &theme, &state, o);
    assert(state.x == 40);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 50,
                                           .y = 50,
                                           .dx = -10,
                                           .dy = -15,
                                           .scroll_precise = true}));
    scene(ui, &theme, &state, o);
#if defined(__wasm__)
    assert(state.x == 40);
#else
    assert(state.x == 50 && cui_event_consumed(ui, 0));
#endif
    float before = state.x;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 50,
                                           .y = 50,
                                           .dx = -10,
                                           .dy = -25,
                                           .scroll_precise = true}));
    scene(ui, &theme, &state, o);
    assert(state.x == before && !cui_event_consumed(ui, 0));
    /* Two idle16ms frames delimit the next gesture. */
    scene(ui, &theme, &state, o);
    scene(ui, &theme, &state, o);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 50,
                                           .y = 50,
                                           .dx = -40,
                                           .dy = -10,
                                           .scroll_precise = true}));
    scene(ui, &theme, &state, o);
    assert(state.x == before + 40);
    cui_destroy(ui);
}
static void touch_pan_pixels(void) {
#ifdef CUI_SCROLL_BASE_ONLY
    cui_theme theme = {0};
#else
    cui_theme theme = cui_theme_light();
#endif
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(ui);
    cui_scroll_state state = {0};
    cui_scroll_options options = {.width = 200,
                                  .height = 120,
                                  .content_width = 400,
                                  .content_height = 400,
                                  .horizontal = true,
                                  .vertical = true};
    scene(ui, &theme, &state, options);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 7,
                                           .x = 60,
                                           .y = 60}));
    scene(ui, &theme, &state, options);
    assert(cui_interaction_state(ui, 101).pressed);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 7,
                                           .x = 55,
                                           .y = 56}));
    scene(ui, &theme, &state, options);
    assert(state.x == 0 && state.y == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 7,
                                           .x = 40,
                                           .y = 30}));
    scene(ui, &theme, &state, options);
    assert(state.x == 20 && state.y == 30);
    assert(!cui_interaction_state(ui, 101).pressed);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 7,
                                           .x = 40,
                                           .y = 30}));
    scene(ui, &theme, &state, options);
    assert(state.x == 20 && state.y == 30);
    for (unsigned row = 100; row < 110; row++)
        assert(!cui_interaction_state(ui, row).activated);
    cui_id bar = cui_scrollbar_id(1, true);
    cui_rect bounds = cui_bounds(ui, bar);
    cui_scroll_geometry geometry = cui_scroll_layout(options);
    cui_rect thumb = cui_scrollbar_thumb(bounds, state.y, geometry.viewport.height,
                                         options.content_height, true);
    float x = thumb.x + thumb.w / 2, y = thumb.y + thumb.h / 2;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 8,
                                           .x = x,
                                           .y = y}));
    scene(ui, &theme, &state, options);
    assert(state.dragging_y);
    float before_thumb = state.y;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 8,
                                           .x = x,
                                           .y = y + 15}));
    scene(ui, &theme, &state, options);
    assert(cui_frame_events(ui, NULL)[0].type == CUI_EVENT_POINTER_MOVE);
    assert(state.y > before_thumb && state.x == 20);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 8,
                                           .x = x,
                                           .y = y + 15}));
    scene(ui, &theme, &state, options);
    assert(!state.dragging_y);
    cui_destroy(ui);
}
static void full_touch_queue_handoff(void) {
#ifdef CUI_SCROLL_BASE_ONLY
    cui_theme theme = {0};
#else
    cui_theme theme = cui_theme_light();
#endif
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    cui_scroll_state state = {0};
    cui_scroll_options options = {.width = 200,
                                  .height = 120,
                                  .content_width = 400,
                                  .content_height = 400,
                                  .horizontal = true,
                                  .vertical = true};
    scene(ui, &theme, &state, options);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 17,
                                           .x = 60,
                                           .y = 60}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 17,
                                           .x = 60,
                                           .y = 30}));
    for (unsigned n = 2; n < 256; n++)
        assert(
            cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = -100, .y = -100}));
    scene(ui, &theme, &state, options);
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    assert(count == 257 && events[1].type == CUI_EVENT_CANCEL);
    assert(cui_event_target(ui, 1) == 101 && events[1].pointer_id == 17);
    assert(events[2].type == CUI_EVENT_SCROLL &&
           events[2].scroll_phase == CUI_SCROLL_PHASE_STARTED);
    assert(events[2].dy == -30 && cui_event_target(ui, 2) == 1);
    assert(state.y == 30 && !cui_interaction_state(ui, 101).pressed);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 17,
                                           .x = 60,
                                           .y = 30}));
    scene(ui, &theme, &state, options);
    assert(state.y == 30 && !cui_interaction_state(ui, 101).activated);
    cui_destroy(ui);
}
#ifndef CUI_SCROLL_BASE_ONLY
static void nested_drag_scene(cui_context *ui, cui_scroll_state *scroll,
                              cui_resizable_state *resize, float *slider, int divider) {
    cui_theme theme = cui_theme_light();
    cui_scroll_options options = {.width = 200,
                                  .height = 120,
                                  .content_width = 400,
                                  .content_height = 400,
                                  .horizontal = true,
                                  .vertical = true};
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_scrollable_begin(ui, 1, &theme, scroll, options));
    cui_begin_box(ui, 2, (cui_box){.width = 180, .height = 30});
    if (divider == 1)
        cui_base_resize_handle(ui, 2, resize, 0, false, 1);
    else
        cui_base_slider(ui, 2, cui_string("Slider"), slider, 0, 100, 1, false);
    if (divider == 2) {
        assert(cui_set_semantic_role(ui, 2, CUI_ROLE_BUTTON));
        assert(cui_set_touch_drag_priority(ui, 2, true));
    }
    cui_end_box(ui);
    cui_scrollable_end(ui, 1, &theme, scroll, options);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void nested_touch_drag_controls(void) {
    for (int divider = 0; divider < 3; divider++) {
        cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
        cui_scroll_state scroll = {0};
        cui_resizable_state resize;
        cui_resizable_slot slots[2];
        cui_resizable_panel panels[] = {cui_resizable_panel_default(10),
                                        cui_resizable_panel_default(11)};
        assert(cui_resizable_state_init(&resize, slots, 2));
        assert(cui_resizable_sync(&resize, panels, 2, CUI_RESIZABLE_HORIZONTAL, 400) ==
               CUI_RESIZABLE_OK);
        float slider = 0;
        nested_drag_scene(ui, &scroll, &resize, &slider, divider);
        cui_rect bounds = cui_bounds(ui, 2);
        float x = bounds.x + 60, y = bounds.y + 10;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                               .pointer_type = CUI_POINTER_TOUCH,
                                               .pointer_id = 1,
                                               .x = x,
                                               .y = y}));
        nested_drag_scene(ui, &scroll, &resize, &slider, divider);
        float before = divider == 1 ? slots[0].size : slider;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                               .pointer_type = CUI_POINTER_TOUCH,
                                               .pointer_id = 1,
                                               .x = x + 40,
                                               .y = y}));
        nested_drag_scene(ui, &scroll, &resize, &slider, divider);
        assert(cui_frame_events(ui, NULL)[0].type == CUI_EVENT_POINTER_MOVE);
        assert((divider == 1 ? slots[0].size : slider) != before);
        assert(scroll.x == 0 && scroll.y == 0);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                               .pointer_type = CUI_POINTER_TOUCH,
                                               .pointer_id = 1,
                                               .x = x + 40,
                                               .y = y}));
        nested_drag_scene(ui, &scroll, &resize, &slider, divider);
        assert(!resize.dragging && scroll.x == 0 && scroll.y == 0);
        cui_destroy(ui);
    }
}
#endif
int main(void) {
    cui_rect thumb = cui_scrollbar_thumb((cui_rect){10, 20, 10, 100}, 450, 100, 1000, true);
    assert(thumb.y == 64 && thumb.h == 12);
    thumb = cui_scrollbar_thumb((cui_rect){0, 0, 100, 10}, -1, 100, 50, false);
    assert(thumb.x == 0 && thumb.w == 100);
#ifndef CUI_SCROLL_BASE_ONLY
    nested_touch_drag_controls();
#endif
    full_touch_queue_handoff();
    touch_pan_pixels();
    persistent_owner();
    gesture_filter();
    horizontal_mask_edges();
    hidden_tracks();
    deferred_scroll();
    consumed_wheel();
    behavior();
    return 0;
}
