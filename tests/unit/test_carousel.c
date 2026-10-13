/* SPDX-License-Identifier: MIT */
#include "cui/cui_carousel.h"
#ifndef CUI_CAROUSEL_BASE_ONLY
#include "cui/cui_components.h"
#include "cui/cui_scroll.h"
#endif
#include <assert.h>
#include <math.h>
#include <string.h>
static unsigned changes;
static size_t last_index;
static void changed(size_t index, void *user) {
    assert(user == &changes);
    changes++;
    last_index = index;
}
static void near(float a, float b) {
    assert(fabsf(a - b) < .03f);
}
static void setup(cui_carousel_state *s, cui_carousel_geometry *workspace, size_t count) {
    assert(cui_carousel_state_init(s, workspace, 8, count));
    s->on_change = changed;
    s->change_user = &changes;
    changes = 0;
}
static void test_model(void) {
    cui_carousel_state s;
    cui_carousel_geometry workspace[8];
    setup(&s, workspace, 0);
    assert(s.selected == CUI_CAROUSEL_NO_SELECTION);
    assert(!cui_carousel_step(&s, true));
    assert(!cui_carousel_set_count(&s, 9));
    assert(cui_carousel_set_count(&s, 3));
    assert(s.selected == 0);
    cui_carousel_set_looping(&s, true);
    assert(cui_carousel_navigation(&s, false) == 2);
    cui_carousel_set_looping(&s, false);
    cui_carousel_geometry g[] = {{1, 0, 80}, {2, 96, 200}, {3, 312, 40}};
    assert(cui_carousel_set_geometry(&s, g, 3, 100));
    near(s.maximum, 252);
    near(cui_carousel_snap(&s, 0), 0);
    near(cui_carousel_snap(&s, 1), -96);
    near(cui_carousel_snap(&s, 2), -252);
    assert(cui_carousel_nearest(&s, -230) == 2);
    cui_carousel_set_selected(&s, 99);
    assert(s.selected == 2 && changes == 0);
    assert(!cui_carousel_select(&s, 2));
    assert(!cui_carousel_select(&s, 9));
    assert(cui_carousel_select(&s, 1) && changes == 1 && last_index == 1);
    assert(cui_carousel_tick(&s, 0, false));
    assert(cui_carousel_tick(&s, .05, false));
    assert(s.offset < 0 && s.offset > -96);
    assert(!cui_carousel_tick(&s, 5, false));
    near(s.offset, -96);
    cui_carousel_state before = s;
    cui_carousel_geometry invalid[] = {{1, 0, NAN}, {2, 96, 200}, {3, 312, 40}};
    assert(!cui_carousel_set_geometry(&s, invalid, 3, 100));
    assert(!memcmp(&before, &s, sizeof(s)));
    invalid[0] = g[0];
    invalid[1].id = 1;
    assert(!cui_carousel_set_geometry(&s, invalid, 3, 100));
    assert(!cui_carousel_set_axis(&s, CUI_ORIENTATION_UNSPECIFIED));
    assert(cui_carousel_set_axis(&s, CUI_ORIENTATION_VERTICAL));
    assert(!s.geometry_ready && s.offset == 0);
    assert(cui_carousel_set_geometry(&s, g, 3, 400));
    assert(cui_carousel_navigation(&s, false) == CUI_CAROUSEL_NO_SELECTION);
    cui_carousel_set_looping(&s, true);
    assert(!s.runtime_looping);
    assert(cui_carousel_set_geometry(&s, g, 3, 100));
    assert(s.runtime_looping);
    near(s.cycle, 368);
    cui_carousel_set_selected(&s, 2);
    cui_carousel_tick(&s, 6, true);
    assert(cui_carousel_step(&s, true));
    near(s.target, -368);
    assert(changes == 2 && s.selected == 0);
    near(cui_carousel_item_position(&s, 0), 56);
    cui_carousel_tick(&s, 7, true);
    near(s.offset, 0);
    assert(changes == 2);
    assert(cui_carousel_step(&s, false));
    near(s.target, 56);
    cui_carousel_tick(&s, 8, true);
    near(s.offset, -312);
    cui_carousel_set_selected(&s, 0);
    cui_carousel_tick(&s, 9, true);
    assert(cui_carousel_step(&s, false));
    cui_carousel_tick(&s, 9.03, false);
    float old_visual = cui_carousel_item_position(&s, 2);
    assert(cui_carousel_step(&s, true));
    near(cui_carousel_item_position(&s, 2), old_visual);
    assert(fabsf(s.target - s.offset) < s.cycle / 2);
    assert(cui_carousel_set_count(&s, 0) && s.selected == CUI_CAROUSEL_NO_SELECTION);
    assert(cui_carousel_set_geometry(&s, NULL, 0, 100));
}
static void test_gestures(void) {
    cui_carousel_state s;
    cui_carousel_geometry workspace[8];
    setup(&s, workspace, 3);
    cui_carousel_geometry g[] = {{1, 0, 100}, {2, 100, 100}, {3, 200, 100}};
    assert(cui_carousel_set_geometry(&s, g, 3, 100));
    assert(cui_carousel_begin_drag(&s, 50, 20));
    assert(!cui_carousel_update_drag(&s, 48, 22));
    assert(!s.axis_locked);
    assert(!cui_carousel_update_drag(&s, 49, 30));
    assert(!s.dragging && s.suppress_click);
    assert(!cui_carousel_finish_drag(&s));
    assert(!s.suppress_click && changes == 0);
    assert(cui_carousel_begin_drag(&s, 80, 20));
    assert(cui_carousel_update_drag(&s, 20, 20));
    assert(s.axis_locked);
    near(s.offset, -60);
    assert(cui_carousel_finish_drag(&s));
    assert(s.selected == 1 && changes == 1);
    cui_carousel_tick(&s, 0, true);
    assert(cui_carousel_begin_drag(&s, 50, 20));
    assert(cui_carousel_update_drag(&s, 5, 20));
    cui_carousel_cancel(&s);
    near(s.offset, -100);
    assert(!s.dragging && changes == 1);
    assert(cui_carousel_wheel(&s, -10, 1));
    assert(s.selected == 2 && changes == 2);
    assert(cui_carousel_wheel(&s, -10, 1.01));
    assert(s.selected == 2 && changes == 2);
    assert(!cui_carousel_wheel(&s, -10, 1.04));
    assert(!cui_carousel_wheel(&s, 10, 1.05)); /* Deferred entire burst. */
    assert(cui_carousel_wheel(&s, 10, 1.08));
    assert(s.selected == 1 && changes == 3);
    cui_carousel_tick(&s, 2, true);
    assert(cui_carousel_scroll(&s, -60, 5, CUI_SCROLL_PHASE_STARTED, 2));
    near(s.offset, -160);
    assert(cui_carousel_scroll(&s, 0, 70, CUI_SCROLL_PHASE_MOVED, 2.01));
    near(s.offset, -160); /* Initial dominant-axis lock remains horizontal. */
    assert(cui_carousel_scroll(&s, 0, 0, CUI_SCROLL_PHASE_CANCELLED, 2.02));
    near(s.offset, -100);
    assert(changes == 3 && !s.scrolling);
    assert(cui_carousel_scroll(&s, -60, 0, CUI_SCROLL_PHASE_STARTED, 3));
    assert(cui_carousel_scroll(&s, -100, 0, CUI_SCROLL_PHASE_MOVED, 3.01));
    near(s.offset, -200);
    assert(cui_carousel_scroll(&s, 0, 0, CUI_SCROLL_PHASE_ENDED, 3.02));
    assert(s.selected == 2 && changes == 4);
    cui_carousel_tick(&s, 4, true);
    assert(!cui_carousel_scroll(&s, -60, 0, CUI_SCROLL_PHASE_STARTED, 4));
    assert(s.scroll_deferred);
    assert(!cui_carousel_scroll(&s, 60, 0, CUI_SCROLL_PHASE_MOVED, 4.01));
    assert(!cui_carousel_scroll(&s, 0, 0, CUI_SCROLL_PHASE_ENDED, 4.02));
    assert(!s.scroll_deferred);
    assert(cui_carousel_scroll(&s, 60, 0, CUI_SCROLL_PHASE_MOVED, 5));
    assert(cui_carousel_tick(&s, 5.04, true) == false);
    assert(s.selected == 1 && changes == 5);
    cui_carousel_set_looping(&s, true);
    cui_carousel_set_selected(&s, 0);
    cui_carousel_tick(&s, 6, true);
    assert(cui_carousel_begin_drag(&s, 20, 20));
    assert(cui_carousel_update_drag(&s, 80, 20));
    assert(cui_carousel_finish_drag(&s));
    assert(s.selected == 2);
    assert(fabsf(s.target - s.offset) < s.cycle / 2);
    cui_carousel_tick(&s, 7, true);
    assert(cui_carousel_begin_drag(&s, 20, 20));
    assert(cui_carousel_update_drag(&s, -1000000, 20));
    assert(isfinite(s.offset) && s.offset >= -s.cycle && s.offset < 0);
    cui_carousel_cancel(&s);
}
static void test_programmatic_cancel(void) {
    cui_carousel_state s;
    cui_carousel_geometry workspace[8];
    setup(&s, workspace, 3);
    cui_carousel_geometry g[] = {{1, 0, 100}, {2, 100, 100}, {3, 200, 100}};
    assert(cui_carousel_set_geometry(&s, g, 3, 100));
    assert(cui_carousel_scroll(&s, -60, 0, CUI_SCROLL_PHASE_STARTED, 0));
    cui_carousel_set_selected(&s, 2);
    assert(!s.scrolling && s.scroll_deferred && changes == 0);
    assert(!cui_carousel_scroll(&s, -60, 0, CUI_SCROLL_PHASE_MOVED, .01));
    assert(!cui_carousel_scroll(&s, 0, 0, CUI_SCROLL_PHASE_ENDED, .02));
    assert(!s.scroll_deferred && s.selected == 2 && changes == 0);
    cui_carousel_tick(&s, 1, true);
    assert(cui_carousel_wheel(&s, 1, 1));
    assert(s.selected == 1 && changes == 1);
    cui_carousel_set_selected(&s, 0);
    assert(cui_carousel_wheel(&s, -1, 1.01));
    assert(s.selected == 1 && changes == 2);
}
static void test_duplicate_snaps(void) {
    cui_carousel_state s;
    cui_carousel_geometry workspace[8];
    setup(&s, workspace, 4);
    cui_carousel_geometry g[] = {{1, 0, 100}, {2, 100, 50}, {3, 150, 25}, {4, 175, 25}};
    assert(cui_carousel_set_geometry(&s, g, 4, 100));
    near(cui_carousel_snap(&s, 1), -100);
    near(cui_carousel_snap(&s, 3), -100);
    cui_carousel_set_selected(&s, 3);
    assert(cui_carousel_navigation(&s, false) == 0);
    assert(cui_carousel_navigation(&s, true) == CUI_CAROUSEL_NO_SELECTION);
    assert(cui_carousel_set_axis(&s, CUI_ORIENTATION_VERTICAL));
    assert(cui_carousel_set_geometry(&s, g, 4, 100));
    cui_carousel_set_selected(&s, 0);
    cui_carousel_tick(&s, 0, true);
    assert(cui_carousel_begin_drag(&s, 10, 80));
    assert(cui_carousel_update_drag(&s, 10, 20));
    assert(cui_carousel_finish_drag(&s) && s.selected == 1);
    cui_carousel_tick(&s, 1, true);
    assert(!cui_carousel_scroll(&s, -60, -60, CUI_SCROLL_PHASE_STARTED, 1));
    assert(s.scroll_deferred); /* Ties choose vertical and begin at its edge. */
    cui_carousel_cancel(&s);
}
#ifndef CUI_CAROUSEL_BASE_ONLY
static unsigned child_clicks, renders[3];
static void render_item(cui_context *ui, const cui_theme *theme, size_t index, void *user) {
    (void)user;
    renders[index]++;
    cui_interaction r =
        cui_interact(ui, 1000 + index, CUI_ROLE_BUTTON, cui_string("Child"), false, false, 0);
    cui_begin_box(ui, 1000 + index, (cui_box){.width = -1, .height = -1});
    cui_text(ui, cui_string("Slide content"), 14, theme->text);
    cui_end_box(ui);
    if (r.activated)
        child_clicks++;
}
static const cui_semantic *find(cui_context *ui, cui_id id) {
    size_t count = 0;
    const cui_semantic *all = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; ++i)
        if (all[i].id == id)
            return &all[i];
    return NULL;
}
static cui_draw_list frame(cui_context *ui, const cui_theme *theme, cui_carousel_state *s,
                           cui_carousel_options o) {
    assert(cui_begin_frame(ui, 500, 300, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 500, .height = 300, .padding = 60});
    cui_carousel_item items[] = {{.id = 10, .render = render_item},
                                 {.id = 11, .render = render_item},
                                 {.id = 12, .render = render_item}};
    assert(cui_carousel(ui, 2, theme, s, items, 3, o));
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
static void pointer(cui_context *ui, cui_event_type type, float x, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = type, .x = x, .y = y}));
}
static void controls(cui_context *ui, const cui_theme *theme, cui_carousel_state *s, cui_id root,
                     void *user) {
    cui_carousel_control_options options =
        user ? *(cui_carousel_control_options *)user : (cui_carousel_control_options){0};
    cui_carousel_control(ui, 2000, theme, s, root, false, options);
    cui_carousel_control(ui, 2001, theme, s, root, true, options);
}
static void pagination(cui_context *ui, const cui_theme *theme, cui_carousel_state *s, cui_id root,
                       void *user) {
    (void)user;
    assert(cui_carousel_pagination_begin(ui, 2002, (cui_str){0}, NULL));
    for (size_t i = 0; i < s->count; ++i)
        cui_carousel_pagination_item(ui, 2010 + i, theme, s, root, i,
                                     (cui_carousel_control_options){0});
    cui_carousel_pagination_end(ui);
}
static void test_controls_during_motion(void) {
    for (unsigned axis = 0; axis < 2; axis++) {
        cui_context *ui = cui_create((cui_config){.max_elements = 256});
        cui_theme theme = cui_theme_light();
        cui_carousel_state state;
        cui_carousel_geometry workspace[8];
        setup(&state, workspace, 3);
        cui_carousel_set_axis(&state, axis ? CUI_ORIENTATION_VERTICAL : CUI_ORIENTATION_HORIZONTAL);
        cui_carousel_set_looping(&state, true);
        cui_carousel_options options = cui_carousel_default();
        options.width = 200;
        options.height = 80;
        options.controls = controls;
        options.pagination = pagination;
        frame(ui, &theme, &state, options);
        frame(ui, &theme, &state, options);
        assert(cui_carousel_select(&state, 2));
        frame(ui, &theme, &state, options);
        assert(fabsf(state.offset - state.target) > 1);
        cui_rect next = find(ui, 2001)->bounds;
        pointer(ui, CUI_EVENT_POINTER_DOWN, next.x + next.w / 2, next.y + next.h / 2);
        frame(ui, &theme, &state, options);
        assert(!state.dragging && state.selected == 2);
        pointer(ui, CUI_EVENT_POINTER_UP, next.x + next.w / 2, next.y + next.h / 2);
        frame(ui, &theme, &state, options);
        assert(state.selected == 0);
        cui_destroy(ui);
    }
}
static void test_controls(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    cui_theme theme = cui_theme_dark();
    cui_carousel_state s;
    cui_carousel_geometry workspace[8];
    setup(&s, workspace, 3);
    cui_carousel_options o = cui_carousel_default();
    o.width = 200;
    o.height = 80;
    o.reduced_motion = true;
    o.controls = controls;
    o.pagination = pagination;
    frame(ui, &theme, &s, o);
    near(find(ui, 2001)->bounds.x, 276); /* Fixed dimensions position on first frame. */
    near(find(ui, 2001)->bounds.y, 84);
    cui_draw_list list = frame(ui, &theme, &s, o);
    assert(find(ui, 2000)->disabled && !find(ui, 2001)->disabled);
    near(find(ui, 2000)->bounds.x, 12);
    near(find(ui, 2001)->bounds.x, 276);
    near(find(ui, 2001)->bounds.w, 32);
    near(find(ui, 2001)->bounds.y, 84);
    near(find(ui, 2010)->bounds.w, 20);
    assert(find(ui, 2010)->details.selected && !find(ui, 2011)->details.selected);
    unsigned images = 0;
    for (size_t i = 0; i < list.count; ++i)
        images += list.commands[i].type == CUI_DRAW_IMAGE;
    assert(images == 2);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 285, 95);
    frame(ui, &theme, &s, o);
    pointer(ui, CUI_EVENT_POINTER_UP, 285, 95);
    frame(ui, &theme, &s, o);
    assert(s.selected == 1 && changes == 1 && cui_focused(ui) == 2 && s.focus_ring_suppressed);
    cui_focus(ui, 2001);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    frame(ui, &theme, &s, o);
    assert(s.selected == 2 && changes == 2 && cui_focused(ui) == 2001);
    cui_focus(ui, 2010);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    frame(ui, &theme, &s, o);
    assert(s.selected == 0 && changes == 3 && cui_focused(ui) == 2010);
    cui_carousel_control_options sizes[] = {
        {.size = CUI_CAROUSEL_CONTROL_XSMALL},
        {.size = CUI_CAROUSEL_CONTROL_SMALL},
        {.size = CUI_CAROUSEL_CONTROL_MEDIUM},
        {.size = CUI_CAROUSEL_CONTROL_LARGE},
        {.size = CUI_CAROUSEL_CONTROL_CUSTOM, .custom_size = 40}};
    float expected[] = {20, 24, 32, 32, 40}, icon_sizes[] = {12, 14, 16, 24, 30};
    for (size_t i = 0; i < 5; ++i) {
        o.controls_user = &sizes[i];
        list = frame(ui, &theme, &s, o);
        near(find(ui, 2001)->bounds.w, expected[i]);
        near(find(ui, 2001)->bounds.h, expected[i]);
        unsigned image_count = 0;
        for (size_t j = 0; j < list.count; ++j)
            if (list.commands[j].type == CUI_DRAW_IMAGE) {
                near(list.commands[j].bounds.w, icon_sizes[i]);
                near(list.commands[j].bounds.h, icon_sizes[i]);
                image_count++;
            }
        assert(image_count == 2);
    }
    o.controls_user = NULL;
    assert(cui_carousel_set_axis(&s, CUI_ORIENTATION_VERTICAL));
    frame(ui, &theme, &s, o);
    frame(ui, &theme, &s, o);
    near(find(ui, 2000)->bounds.y, 12);
    near(find(ui, 2001)->bounds.y, 156);
    near(find(ui, 2001)->bounds.x, 144);
    cui_focus(ui, 2);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    frame(ui, &theme, &s, o);
    assert(s.selected == 0 && !cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_DOWN}));
    frame(ui, &theme, &s, o);
    assert(s.selected == 1 && cui_event_consumed(ui, 0));
    cui_destroy(ui);
}
static void nested_frame(cui_context *ui, const cui_theme *theme, cui_carousel_state *s,
                         cui_carousel_options o, float *parent) {
    assert(cui_begin_frame(ui, 300, 300, .016f));
    cui_interact(ui, 1, CUI_ROLE_GROUP, cui_string("Parent scroller"), false, false, 0);
    cui_begin_box(ui, 1, (cui_box){.width = 300, .height = 300, .padding = 20, .clip = true});
    cui_carousel_item items[] = {{.id = 10, .render = render_item},
                                 {.id = 11, .render = render_item},
                                 {.id = 12, .render = render_item}};
    assert(cui_carousel(ui, 2, theme, s, items, 3, o));
    size_t n = 0;
    const cui_event *events = cui_frame_events(ui, &n);
    for (size_t i = 0; i < n; ++i)
        if (events[i].type == CUI_EVENT_SCROLL && !cui_event_consumed(ui, i) &&
            cui_event_target(ui, i) == 1) {
            float step = events[i].scroll_precise ? 1 : 48;
            *parent = fmaxf(0, fminf(600, *parent - events[i].dy * step));
            cui_consume_event(ui, i);
        }
    cui_set_semantic_details(ui, 1,
                             (cui_semantic_details){.scrollable = true,
                                                    .orientation = CUI_ORIENTATION_VERTICAL,
                                                    .scroll_y = *parent,
                                                    .scroll_max_y = 600});
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void nested_wheel(cui_context *ui, float delta, bool precise, cui_scroll_phase phase) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                           .x = 30,
                                           .y = 30,
                                           .dy = delta,
                                           .scroll_precise = precise,
                                           .scroll_phase = phase}));
}
static void test_parent_handoff(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    cui_theme theme = cui_theme_light();
    cui_carousel_state s;
    cui_carousel_geometry workspace[8];
    setup(&s, workspace, 3);
    assert(cui_carousel_set_axis(&s, CUI_ORIENTATION_VERTICAL));
    cui_carousel_options o = cui_carousel_default();
    o.width = 100;
    o.height = 80;
    o.reduced_motion = true;
    float parent = 0;
    nested_frame(ui, &theme, &s, o, &parent);
    nested_frame(ui, &theme, &s, o, &parent);
    o.now_seconds = 1;
    nested_wheel(ui, -1, false, CUI_SCROLL_PHASE_NONE);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 1 && parent == 0);
    o.now_seconds = 1.01;
    nested_wheel(ui, -1, false, CUI_SCROLL_PHASE_NONE);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 1 && parent == 0);
    cui_carousel_set_selected(&s, 2);
    o.now_seconds = 2;
    nested_frame(ui, &theme, &s, o, &parent);
    nested_frame(ui, &theme, &s, o, &parent);
    o.now_seconds = 2.1;
    nested_wheel(ui, -1, false, CUI_SCROLL_PHASE_NONE);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && parent == 48 && cui_event_target(ui, 0) == 1);
    o.now_seconds = 2.11;
    nested_wheel(ui, 1, false, CUI_SCROLL_PHASE_NONE);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && parent == 0 && cui_event_target(ui, 0) == 1);
    o.now_seconds = 2.2;
    nested_wheel(ui, 1, false, CUI_SCROLL_PHASE_NONE);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 1 && parent == 0);
    o.now_seconds = 3;
    nested_wheel(ui, -1, false, CUI_SCROLL_PHASE_NONE);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && parent == 0);
    /* No intervening frame clears owned-burst routing metadata after quiet. */
    o.now_seconds = 3.1;
    nested_wheel(ui, -1, false, CUI_SCROLL_PHASE_NONE);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && parent == 48 && cui_event_target(ui, 0) == 1);
    o.now_seconds = 4;
    nested_wheel(ui, -60, true, CUI_SCROLL_PHASE_STARTED);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(!s.scrolling && s.scroll_deferred && parent == 108);
    o.now_seconds = 4.01;
    nested_wheel(ui, 60, true, CUI_SCROLL_PHASE_MOVED);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && !s.scrolling && parent == 48);
    o.now_seconds = 4.02;
    nested_wheel(ui, 0, true, CUI_SCROLL_PHASE_ENDED);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(!s.scroll_deferred);
    cui_carousel_set_selected(&s, 0);
    o.now_seconds = 5;
    nested_frame(ui, &theme, &s, o, &parent);
    nested_frame(ui, &theme, &s, o, &parent);
    o.now_seconds = 5.1;
    nested_wheel(ui, -60, true, CUI_SCROLL_PHASE_STARTED);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.scrolling && parent == 48);
    o.now_seconds = 5.11;
    nested_wheel(ui, -1000, true, CUI_SCROLL_PHASE_MOVED);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.scrolling && parent == 48);
    o.now_seconds = 5.12;
    nested_wheel(ui, 0, true, CUI_SCROLL_PHASE_ENDED);
    nested_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && parent == 48 && !s.scrolling);
    cui_destroy(ui);
}
static void actual_scroll_frame(cui_context *ui, const cui_theme *theme, cui_carousel_state *s,
                                cui_carousel_options o, cui_scroll_state *parent) {
    assert(cui_begin_frame(ui, 300, 300, .016f));
    cui_scroll_options scroll = {
        .width = 300, .height = 300, .content_height = 900, .vertical = true, .bar_thickness = 10};
    assert(cui_scrollable_begin(ui, 1, theme, parent, scroll));
    cui_begin_box(ui, 8, (cui_box){.width = -1, .height = 120, .padding = 20});
    cui_carousel_item items[] = {{.id = 10, .render = render_item},
                                 {.id = 11, .render = render_item},
                                 {.id = 12, .render = render_item}};
    assert(cui_carousel(ui, 2, theme, s, items, 3, o));
    cui_end_box(ui);
    cui_scrollable_end_ex(ui, 1, theme, parent, scroll);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_actual_scroll_parent(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    cui_theme theme = cui_theme_light();
    cui_carousel_state s;
    cui_carousel_geometry workspace[8];
    setup(&s, workspace, 3);
    assert(cui_carousel_set_axis(&s, CUI_ORIENTATION_VERTICAL));
    cui_carousel_options o = cui_carousel_default();
    o.width = 100;
    o.height = 80;
    o.reduced_motion = true;
    cui_scroll_state parent = {0};
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    o.now_seconds = 1;
    nested_wheel(ui, -1, false, CUI_SCROLL_PHASE_NONE);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 1 && parent.y == 0);
    o.now_seconds = 2;
    nested_wheel(ui, -1, false, CUI_SCROLL_PHASE_NONE);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && parent.y == 0);
    o.now_seconds = 2.01;
    nested_wheel(ui, -1, false, CUI_SCROLL_PHASE_NONE);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && parent.y == 0); /* Owned edge burst stays in child. */
    o.now_seconds = 2.1;
    nested_wheel(ui, -1, false, CUI_SCROLL_PHASE_NONE);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && parent.y == 48 && cui_event_target(ui, 0) == 1);
    near(find(ui, 2)->bounds.y, -28); /* End-phase updates this frame, not next. */
    near(find(ui, 1002)->bounds.y, -28);
    o.now_seconds = 2.11;
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(parent.y == 48); /* Begin/end reduction did not replay the event. */
    o.now_seconds = 3;
    nested_wheel(ui, -.01f, false, CUI_SCROLL_PHASE_NONE);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    near(parent.y, 48.48f);
    o.now_seconds = 3.01;
    nested_wheel(ui, .01f, false, CUI_SCROLL_PHASE_NONE);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2);
    near(parent.y, 48);
    cui_carousel_set_selected(&s, 0);
    o.now_seconds = 4;
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    o.now_seconds = 4.09;
    nested_wheel(ui, 0, true, CUI_SCROLL_PHASE_STARTED);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(!s.scroll_deferred && !s.scrolling && parent.y == 48);
    o.now_seconds = 4.1;
    nested_wheel(ui, -60, true, CUI_SCROLL_PHASE_STARTED);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(s.scrolling && parent.y == 48);
    o.now_seconds = 4.11;
    nested_wheel(ui, -1000, true, CUI_SCROLL_PHASE_MOVED);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(s.scrolling && parent.y == 48);
    o.now_seconds = 4.12;
    nested_wheel(ui, 0, true, CUI_SCROLL_PHASE_ENDED);
    actual_scroll_frame(ui, &theme, &s, o, &parent);
    assert(s.selected == 2 && !s.scrolling && parent.y == 48);
    cui_destroy(ui);
}
static void render_static(cui_context *ui, const cui_theme *theme, size_t index, void *user) {
    (void)index;
    (void)user;
    cui_text(ui, cui_string("Content"), 14, theme->text);
}
static void dual_frame(cui_context *ui, const cui_theme *theme, cui_carousel_state *a,
                       cui_carousel_state *b, double now) {
    assert(cui_begin_frame(ui, 300, 120, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 300, .height = 120, .row = true, .gap = 20});
    cui_carousel_options o = cui_carousel_default();
    o.width = 100;
    o.height = 100;
    o.now_seconds = now;
    o.reduced_motion = true;
    cui_carousel_item items[] = {{.id = 10, .render = render_static},
                                 {.id = 11, .render = render_static},
                                 {.id = 12, .render = render_static}};
    assert(cui_carousel(ui, 2, theme, a, items, 3, o));
    for (size_t i = 0; i < 3; ++i)
        items[i].id += 10;
    assert(cui_carousel(ui, 3, theme, b, items, 3, o));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_disjoint_bursts(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    cui_theme theme = cui_theme_light();
    cui_carousel_state a, b;
    cui_carousel_geometry wa[8], wb[8];
    setup(&a, wa, 3);
    setup(&b, wb, 3);
    dual_frame(ui, &theme, &a, &b, 0);
    dual_frame(ui, &theme, &a, &b, 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 20, .y = 20, .dx = -1}));
    dual_frame(ui, &theme, &a, &b, 1);
    assert(a.selected == 1 && b.selected == 0);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 140, .y = 20, .dx = -1}));
    dual_frame(ui, &theme, &a, &b, 1.01);
    assert(a.selected == 1 && b.selected == 1);
    cui_destroy(ui);
}
typedef struct {
    cui_edit *edit;
    cui_input_state input;
} input_child;
static void render_input(cui_context *ui, const cui_theme *theme, size_t index, void *user) {
    input_child *child = user;
    if (index == 0)
        cui_input_ex(ui, 1100, theme, cui_string("Editor"), child->edit, &child->input,
                     (cui_input_options){.width = 180});
    else
        cui_text(ui, cui_string("Other slide"), 14, theme->text);
}
static void input_frame(cui_context *ui, const cui_theme *theme, cui_carousel_state *s,
                        input_child *child) {
    assert(cui_begin_frame(ui, 500, 300, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 500, .height = 300, .padding = 60});
    cui_carousel_options o = cui_carousel_default();
    o.width = 200;
    o.height = 80;
    o.reduced_motion = true;
    cui_carousel_item items[] = {{.id = 10, .render = render_input, .user = child},
                                 {.id = 11, .render = render_input, .user = child},
                                 {.id = 12, .render = render_input, .user = child}};
    assert(cui_carousel(ui, 2, theme, s, items, 3, o));
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
static void test_input_child(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 256});
    cui_theme theme = cui_theme_light();
    cui_carousel_state s;
    cui_carousel_geometry workspace[8];
    setup(&s, workspace, 3);
    char text[64] = "abcdef";
    input_child child = {.edit = cui_edit_create((cui_edit_config){.buffer = text,
                                                                   .capacity = sizeof(text),
                                                                   .history_bytes = 512,
                                                                   .max_history_steps = 8})};
    assert(ui && child.edit);
    input_frame(ui, &theme, &s, &child);
    input_frame(ui, &theme, &s, &child);
    cui_rect bounds = find(ui, 1100)->bounds;
    float x = bounds.x + 25, y = bounds.y + bounds.h / 2;
    pointer(ui, CUI_EVENT_POINTER_DOWN, x, y);
    input_frame(ui, &theme, &s, &child);
    assert(child.input.behavior.dragging && cui_focused(ui) == 1100);
    pointer(ui, CUI_EVENT_POINTER_MOVE, x + 30, y);
    input_frame(ui, &theme, &s, &child);
    assert(s.axis_locked);
    pointer(ui, CUI_EVENT_POINTER_UP, x + 30, y);
    input_frame(ui, &theme, &s, &child);
    assert(!child.input.behavior.dragging && !s.dragging && !cui_pressed(ui, 1100));
    assert(cui_edit_select(child.edit, 0, 0) == CUI_EDIT_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    input_frame(ui, &theme, &s, &child);
    assert(cui_edit_get_state(child.edit).cursor == 1 && s.selected == 0);
    assert(cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
    input_frame(ui, &theme, &s, &child);
    assert(cui_edit_get_state(child.edit).cursor == 6 && s.selected == 0);
    assert(cui_event_consumed(ui, 0));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    input_frame(ui, &theme, &s, &child);
    assert(cui_edit_get_state(child.edit).cursor == 0 && s.selected == 0);
    pointer(ui, CUI_EVENT_POINTER_DOWN, x, y);
    input_frame(ui, &theme, &s, &child);
    pointer(ui, CUI_EVENT_POINTER_UP, x, y);
    input_frame(ui, &theme, &s, &child);
    assert(!child.input.behavior.dragging && !strcmp(text, "abcdef"));
    cui_edit_destroy(child.edit);
    cui_destroy(ui);
}
static void test_widget(void) {
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (unsigned variant = 0; variant < 4; ++variant) {
        cui_context *ui = cui_create((cui_config){.max_elements = 256});
        assert(ui);
        cui_theme theme = themes[variant];
        cui_carousel_state s;
        cui_carousel_geometry workspace[8];
        setup(&s, workspace, 3);
        memset(renders, 0, sizeof(renders));
        child_clicks = 0;
        cui_carousel_options o = cui_carousel_default();
        o.width = 200;
        o.height = 80;
        o.reduced_motion = true;
        frame(ui, &theme, &s, o);
        frame(ui, &theme, &s, o);
        assert(renders[0] == 2 && renders[1] == 2 && renders[2] == 2);
        assert(find(ui, 2)->role == CUI_ROLE_REGION);
        assert(find(ui, 10)->details.pos_in_set == 1 && find(ui, 12)->details.set_size == 3);
        near(find(ui, 1000)->bounds.x, 60);
        near(find(ui, 1000)->bounds.w, 200);
        pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 80);
        frame(ui, &theme, &s, o);
        assert(s.dragging && cui_pressed(ui, 1000));
        pointer(ui, CUI_EVENT_POINTER_UP, 80, 80);
        frame(ui, &theme, &s, o);
        assert(child_clicks == 1 && changes == 0);
        pointer(ui, CUI_EVENT_POINTER_DOWN, 220, 80);
        frame(ui, &theme, &s, o);
        pointer(ui, CUI_EVENT_POINTER_MOVE, 80, 80);
        frame(ui, &theme, &s, o);
        assert(s.axis_locked && child_clicks == 1);
        pointer(ui, CUI_EVENT_POINTER_UP, 80, 80);
        frame(ui, &theme, &s, o);
        assert(child_clicks == 1 && changes == 1 && s.selected == 1);
        assert(!cui_pressed(ui, 1000));
        pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 80);
        frame(ui, &theme, &s, o);
        pointer(ui, CUI_EVENT_POINTER_UP, 80, 80);
        frame(ui, &theme, &s, o);
        assert(child_clicks == 2);
        pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 80);
        frame(ui, &theme, &s, o);
        pointer(ui, CUI_EVENT_POINTER_MOVE, 80, 110);
        frame(ui, &theme, &s, o);
        assert(s.suppress_click && !s.dragging);
        pointer(ui, CUI_EVENT_POINTER_UP, 80, 110);
        frame(ui, &theme, &s, o);
        assert(child_clicks == 2 && s.selected == 1);
        cui_focus(ui, 2);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
        frame(ui, &theme, &s, o);
        assert(s.selected == 2 && changes == 2);
        assert(cui_event_consumed(ui, 0));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
        frame(ui, &theme, &s, o);
        assert(!cui_event_consumed(ui, 0));
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
        frame(ui, &theme, &s, o);
        assert(s.selected == 0 && changes == 3);
        frame(ui, &theme, &s, o);
        o.now_seconds = 1;
        assert(cui_queue_event(ui,
                               (cui_event){.type = CUI_EVENT_SCROLL, .x = 80, .y = 80, .dx = -25}));
        frame(ui, &theme, &s, o);
        assert(s.selected == 1);
        o.now_seconds = 1.01;
        assert(cui_queue_event(ui,
                               (cui_event){.type = CUI_EVENT_SCROLL, .x = 80, .y = 80, .dx = -25}));
        frame(ui, &theme, &s, o);
        assert(s.selected == 1);
        o.now_seconds = 2;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .x = 80,
                                               .y = 80,
                                               .dx = -140,
                                               .scroll_precise = true,
                                               .scroll_phase = CUI_SCROLL_PHASE_STARTED}));
        frame(ui, &theme, &s, o);
        assert(s.scrolling && s.selected == 1);
        o.now_seconds = 2.01;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL,
                                               .scroll_precise = true,
                                               .scroll_phase = CUI_SCROLL_PHASE_ENDED}));
        frame(ui, &theme, &s, o);
        assert(s.selected == 2 && !s.scrolling);
        cui_carousel_set_looping(&s, true);
        o.now_seconds = 3;
        frame(ui, &theme, &s, o);
        assert(cui_carousel_step(&s, true));
        frame(ui, &theme, &s, o);
        assert(s.selected == 0 && s.runtime_looping);
        near(find(ui, 1000)->bounds.x, 60);
        for (unsigned i = 0; i < 5; ++i)
            frame(ui, &theme, &s, o);
        assert(!cui_needs_redraw(ui));
        o.disabled = true;
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
        frame(ui, &theme, &s, o);
        assert(s.selected == 0);
        cui_destroy(ui);
    }
}
#endif
static void test_contact_lifecycle(void) {
    cui_carousel_state s;
    cui_carousel_geometry workspace[8], geometry[] = {{1, 0, 100}, {2, 100, 100}, {3, 200, 100}};
    setup(&s, workspace, 3);
    assert(cui_carousel_set_geometry(&s, geometry, 3, 100));
    for (unsigned i = 0; i < 4; ++i) {
        double now = i * .05;
        assert(cui_carousel_touch_scroll(
            &s, -16.25f, 0, i ? CUI_SCROLL_PHASE_MOVED : CUI_SCROLL_PHASE_STARTED, now));
        cui_carousel_tick(&s, now + .049, true);
        assert(s.scrolling && s.scroll_contact && s.selected == 0 && changes == 0);
        near(s.offset, -16.25f * (i + 1));
    }
    assert(cui_carousel_touch_scroll(&s, 0, 0, CUI_SCROLL_PHASE_ENDED, .3));
    assert(!s.scrolling && !s.scroll_contact && s.selected == 1 && changes == 1);
    cui_carousel_tick(&s, .3, true);
    near(s.offset, -100);
    assert(cui_carousel_touch_scroll(&s, 20, 0, CUI_SCROLL_PHASE_STARTED, 1));
    cui_carousel_tick(&s, 2, true);
    near(s.offset, -80);
    assert(cui_carousel_touch_scroll(&s, 0, 0, CUI_SCROLL_PHASE_CANCELLED, 3));
    near(s.offset, -100);
    assert(s.selected == 1 && changes == 1 && !s.scroll_contact);
    assert(!cui_carousel_touch_scroll(&s, 0, 20, CUI_SCROLL_PHASE_STARTED, 4));
    cui_carousel_tick(&s, 5, true);
    assert(!cui_carousel_touch_scroll(&s, -65, 0, CUI_SCROLL_PHASE_MOVED, 6));
    assert(!cui_carousel_touch_scroll(&s, 0, 0, CUI_SCROLL_PHASE_ENDED, 7));
    assert(!s.scroll_contact && s.scroll_axis == CUI_ORIENTATION_UNSPECIFIED);
    assert(cui_carousel_touch_scroll(&s, -20, 0, CUI_SCROLL_PHASE_STARTED, 8));
    cui_carousel_cancel(&s);
    assert(!s.scroll_contact && !s.scrolling);
    /* Mouse/trackpad burst timeout remains distinct from a live contact. */
    assert(cui_carousel_scroll(&s, -20, 0, CUI_SCROLL_PHASE_STARTED, 9));
    cui_carousel_tick(&s, 9.05, true);
    assert(!s.scrolling && !s.scroll_contact);
}
#ifndef CUI_CAROUSEL_BASE_ONLY
static void test_scrollbar_skin(void) {
    for (unsigned axis = 0; axis < 2; axis++) {
        cui_context *ui = cui_create((cui_config){.width = 500, .height = 300});
        cui_theme theme = cui_theme_dark();
        theme.scrollbar = (cui_scrollbar_style){
            .visibility = CUI_SCROLLBAR_VISIBLE, .thickness = 12, .no_track = true};
        cui_carousel_state state;
        cui_carousel_geometry workspace[8];
        setup(&state, workspace, 3);
        cui_carousel_set_axis(&state, axis ? CUI_ORIENTATION_VERTICAL : CUI_ORIENTATION_HORIZONTAL);
        cui_carousel_options options = cui_carousel_default();
        options.width = 200;
        options.height = 80;
        options.reduced_motion = true;
        for (unsigned i = 0; i < 3; i++)
            frame(ui, &theme, &state, options);
        cui_id bar = cui_scrollbar_id(2, axis != 0);
        cui_rect bounds = cui_bounds(ui, bar);
        assert((axis ? bounds.w : bounds.h) == 12);
        pointer(ui, CUI_EVENT_POINTER_DOWN, bounds.x + (axis ? bounds.w / 2 : bounds.w - 2),
                bounds.y + (axis ? bounds.h - 2 : bounds.h / 2));
        frame(ui, &theme, &state, options);
        assert(state.selected > 0 && changes == 1);
        pointer(ui, CUI_EVENT_POINTER_UP, bounds.x, bounds.y);
        theme.scrollbar.visibility = CUI_SCROLLBAR_HIDDEN;
        frame(ui, &theme, &state, options);
        size_t count;
        const cui_semantic *semantics = cui_semantics(ui, &count);
        for (size_t i = 0; i < count; i++)
            assert(semantics[i].id != bar);
        cui_destroy(ui);
    }
}
#endif
int main(void) {
    test_model();
    test_gestures();
    test_contact_lifecycle();
    test_duplicate_snaps();
    test_programmatic_cancel();
#ifndef CUI_CAROUSEL_BASE_ONLY
    test_scrollbar_skin();
    test_widget();
    test_controls();
    test_controls_during_motion();
    test_parent_handoff();
    test_disjoint_bursts();
    test_actual_scroll_parent();
    test_input_child();
#endif
    return 0;
}
