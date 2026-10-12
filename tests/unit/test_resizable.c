/* SPDX-License-Identifier: MIT */
#include "cui/cui_resizable.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static unsigned completed;
static void resize_completed(void *user, const cui_resizable_state *state) {
    assert(user == &completed && state->active_handle == 0 && !state->dragging);
    completed++;
}
static void near(float a, float b) {
    assert(fabsf(a - b) < .01f);
}
static void test_handle_geometry(void) {
    for (int axis = 0; axis < 2; axis++)
        for (int edge = 0; edge < 3; edge++) {
            cui_rect band, line;
            assert(cui_resize_handle_rectangles((cui_resizable_axis)axis, (cui_resize_edge)edge,
                                                (cui_size){200, 200}, &band, &line));
            float size = edge == CUI_RESIZE_STRADDLE ? 9 : 5;
            float start = edge == CUI_RESIZE_STRADDLE ? -4 : edge == CUI_RESIZE_LEADING ? 0 : 195;
            near(axis == 0 ? band.w : band.h, size);
            near(axis == 0 ? band.x : band.y, start);
            near(axis == 0 ? line.x : line.y, edge == CUI_RESIZE_TRAILING ? 199 : 0);
            near(axis == 0 ? line.w : line.h, 1);
        }
    cui_rect band = {1, 2, 3, 4}, line = {5, 6, 7, 8};
    assert(!cui_resize_handle_rectangles(CUI_RESIZABLE_HORIZONTAL, CUI_RESIZE_LEADING,
                                         (cui_size){NAN, 200}, &band, &line));
    assert(band.x == 1 && line.x == 5);
}
static void test_model(void) {
    cui_resizable_state state;
    cui_resizable_slot storage[4];
    assert(!cui_resizable_state_init(NULL, storage, 4));
    assert(!cui_resizable_state_init(&state, storage, 0));
    assert(cui_resizable_state_init(&state, storage, 4));
    cui_resizable_panel panels[] = {cui_resizable_panel_default(10),
                                    cui_resizable_panel_default(11)};
    assert(cui_resizable_sync(&state, panels, 2, CUI_RESIZABLE_HORIZONTAL, 400) ==
           CUI_RESIZABLE_OK);
    near(storage[0].size, 200);
    near(storage[1].size, 200);
    state.on_resize = resize_completed;
    state.resize_user = &completed;
    completed = 0;
    assert(cui_resizable_resize_panel(&state, 0, 220));
    near(storage[0].size, 220);
    near(storage[1].size, 180);
    assert(completed == 1);
    assert(cui_resizable_resize_panel(&state, 1, 210));
    near(storage[0].size, 190);
    near(storage[1].size, 210);
    assert(cui_resizable_sync(&state, panels, 2, CUI_RESIZABLE_HORIZONTAL, 800) ==
           CUI_RESIZABLE_OK);
    near(storage[0].size, 380);
    near(storage[1].size, 420);
    panels[0].custom_range = panels[1].custom_range = true;
    panels[0].minimum = panels[1].minimum = 150;
    panels[0].maximum = panels[1].maximum = 300;
    assert(cui_resizable_sync(&state, panels, 2, CUI_RESIZABLE_HORIZONTAL, 800) ==
           CUI_RESIZABLE_INFEASIBLE);
    assert(!state.fit_constraints);
    near(storage[0].size, 300);
    near(storage[1].size, 300);
    assert(cui_resizable_sync(&state, panels, 2, CUI_RESIZABLE_HORIZONTAL, 200) ==
           CUI_RESIZABLE_INFEASIBLE);
    near(storage[0].size, 150);
    near(storage[1].size, 150);
    cui_resizable_clear(&state);
    cui_resizable_panel triple[] = {{.id = 20,
                                     .has_initial_size = true,
                                     .initial_size = 150,
                                     .custom_range = true,
                                     .minimum = 100,
                                     .maximum = 400},
                                    {.id = 21,
                                     .has_initial_size = true,
                                     .initial_size = 150,
                                     .custom_range = true,
                                     .minimum = 100,
                                     .maximum = 400},
                                    {.id = 22,
                                     .has_initial_size = true,
                                     .initial_size = 300,
                                     .custom_range = true,
                                     .minimum = 100,
                                     .maximum = 400}};
    assert(cui_resizable_sync(&state, triple, 3, CUI_RESIZABLE_HORIZONTAL, 600) ==
           CUI_RESIZABLE_OK);
    assert(cui_resizable_resize_panel(&state, 1, 50));
    near(storage[0].size, 100);
    near(storage[1].size, 100);
    near(storage[2].size, 400);
    assert(cui_resizable_resize_panel(&state, 0, 400));
    near(storage[0].size, 400);
    near(storage[1].size, 100);
    near(storage[2].size, 100);
    assert(cui_resizable_sync(&state, triple, 3, CUI_RESIZABLE_VERTICAL, 450) == CUI_RESIZABLE_OK);
    near(storage[0].size, 250);
    near(storage[1].size, 100);
    near(storage[2].size, 100);
    assert(!cui_resizable_resize_panel(&state, 0, 300));
    float saved[4];
    assert(cui_resizable_save(&state, saved, 4));
    assert(!cui_resizable_save(&state, saved, 2));
    float bad[] = {200, NAN, 250};
    assert(cui_resizable_restore(&state, bad, 3) == CUI_RESIZABLE_INVALID);
    near(storage[0].size, 250);
    float restored[] = {150, 150, 150};
    assert(cui_resizable_restore(&state, restored, 3) == CUI_RESIZABLE_OK);
    near(storage[0].size, 150);
    assert(cui_resizable_restore(&state, saved, 3) == CUI_RESIZABLE_OK);
    near(storage[0].size, 250);
    assert(cui_resizable_insert(&state, 1, cui_resizable_panel_default(23)) == CUI_RESIZABLE_OK);
    assert(state.count == 4 && storage[1].id == 23);
    assert(cui_resizable_insert(&state, 4, cui_resizable_panel_default(24)) ==
           CUI_RESIZABLE_CAPACITY);
    assert(cui_resizable_remove(&state, 1) && state.count == 3);
    assert(cui_resizable_reset(&state, 0) && storage[0].minimum == 100);
    cui_resizable_panel reorder[] = {triple[2], triple[0], triple[1]};
    float original = storage[2].size;
    assert(cui_resizable_sync(&state, reorder, 3, CUI_RESIZABLE_VERTICAL, 450) == CUI_RESIZABLE_OK);
    assert(storage[0].id == 22);
    near(storage[0].size, original);
    reorder[1].hidden = true;
    assert(cui_resizable_sync(&state, reorder, 3, CUI_RESIZABLE_VERTICAL, 450) == CUI_RESIZABLE_OK);
    near(storage[0].size + storage[2].size, 450);
    assert(!cui_resizable_resize_panel(&state, 1, 120));
    cui_resizable_slot before[4];
    memcpy(before, storage, sizeof(before));
    reorder[2].id = 22;
    assert(cui_resizable_sync(&state, reorder, 3, CUI_RESIZABLE_VERTICAL, 450) ==
           CUI_RESIZABLE_INVALID);
    assert(memcmp(before, storage, sizeof(before)) == 0);
    assert(cui_resizable_sync(&state, triple, 3, CUI_RESIZABLE_VERTICAL, NAN) ==
           CUI_RESIZABLE_INVALID);
    cui_resizable_clear(&state);
    assert(state.count == 0);
    assert(cui_resizable_sync(&state, NULL, 0, CUI_RESIZABLE_HORIZONTAL, 400) == CUI_RESIZABLE_OK);
    cui_resizable_panel collapsed[] = {{.id = 30,
                                        .has_initial_size = true,
                                        .initial_size = 0,
                                        .custom_range = true,
                                        .minimum = 0,
                                        .maximum = 400},
                                       {.id = 31,
                                        .has_initial_size = true,
                                        .initial_size = 400,
                                        .custom_range = true,
                                        .minimum = 0,
                                        .maximum = 400}};
    assert(cui_resizable_sync(&state, collapsed, 2, CUI_RESIZABLE_HORIZONTAL, 400) ==
           CUI_RESIZABLE_OK);
    assert(cui_resizable_reset(&state, 0));
    near(storage[0].size, 0);
    assert(storage[0].minimum == 0);
}
static cui_resize_result base_frame(cui_context *ui, cui_resizable_state *s, bool disabled) {
    assert(cui_begin_frame(ui, 400, 100, .016f));
    cui_begin_box(ui, 100, (cui_box){.width = 400, .height = 100});
    cui_resize_result result = cui_base_resize_handle(ui, 101, s, 0, disabled, 10);
    cui_begin_absolute_box(ui, 101, (cui_box){0}, (cui_rect){s->slots[0].size - 4, 0, 9, -1});
    cui_end_box(ui);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return result;
}
static void test_base_input(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 100});
    assert(ui);
    cui_resizable_slot slots[2];
    cui_resizable_state state;
    assert(cui_resizable_state_init(&state, slots, 2));
    cui_resizable_panel panels[] = {cui_resizable_panel_default(10),
                                    cui_resizable_panel_default(11)};
    assert(cui_resizable_sync(&state, panels, 2, CUI_RESIZABLE_HORIZONTAL, 400) ==
           CUI_RESIZABLE_OK);
    state.on_resize = resize_completed;
    state.resize_user = &completed;
    completed = 0;
    base_frame(ui, &state, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 198, .y = 50}));
    cui_resize_result r = base_frame(ui, &state, false);
    assert(r.phase == CUI_RESIZE_PRESSED);
    assert(cui_pointer_cursor(ui) == CUI_CURSOR_RESIZE_HORIZONTAL);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 220, .y = 50}));
    r = base_frame(ui, &state, false);
    assert(r.changed && r.phase == CUI_RESIZE_DRAGGING);
    near(slots[0].size, 220);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 350, .y = 50}));
    base_frame(ui, &state, false);
    near(slots[0].size, 300);
    near(slots[1].size, 100);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 330, .y = 50}));
    base_frame(ui, &state, false);
    near(slots[0].size, 300); /* Pointer still beyond constraint. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 330, .y = 50}));
    r = base_frame(ui, &state, false);
    assert(r.completed && completed == 1 && r.phase == CUI_RESIZE_IDLE);
    cui_focus(ui, 101);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
    r = base_frame(ui, &state, false);
    assert(r.changed && r.completed);
    near(slots[0].size, 290);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_HOME}));
    base_frame(ui, &state, false);
    near(slots[0].size, 100);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
    base_frame(ui, &state, false);
    near(slots[0].size, 300);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
    base_frame(ui, &state, true);
    near(slots[0].size, 300);
    base_frame(ui, &state, false);
    cui_rect band = cui_bounds(ui, 101);
    assert(
        cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = band.x + 2, .y = 50}));
    base_frame(ui, &state, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    r = base_frame(ui, &state, false);
    assert(r.phase != CUI_RESIZE_PRESSED && r.phase != CUI_RESIZE_DRAGGING && !r.completed &&
           !state.active_handle);
    state.axis = CUI_RESIZABLE_VERTICAL;
    cui_focus(ui, 101);
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP, .modifiers = CUI_MOD_SHIFT}));
    base_frame(ui, &state, false);
    near(slots[0].size, 200);
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    assert(count == 1 && nodes[0].role == CUI_ROLE_SEPARATOR &&
           nodes[0].details.orientation == CUI_ORIENTATION_HORIZONTAL);
    assert(nodes[0].details.minimum == 100 && nodes[0].details.controls == 10);
    float old_size = slots[0].size;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_UP}));
    assert(cui_begin_frame(ui, 400, 100, .016f));
    cui_begin_box(ui, 100, (cui_box){.width = 400, .height = 100, .inert = true});
    r = cui_base_resize_handle(ui, 101, &state, 0, false, 10);
    assert(!r.changed && !r.completed);
    near(slots[0].size, old_size);
    cui_end_box(ui);
    cui_end_frame(ui);
    cui_semantics(ui, &count);
    assert(count == 0);
    cui_destroy(ui);
}
#ifndef CUI_RESIZABLE_BASE_ONLY
static bool declined(cui_context *ui, cui_id id, cui_resizable_axis axis, cui_resize_phase phase,
                     cui_rect band, double length, double opacity, void *user) {
    (void)ui;
    (void)id;
    (void)axis;
    (void)phase;
    (void)band;
    (void)length;
    (void)opacity;
    (*(unsigned *)user)++;
    return false;
}
static cui_draw_list styled_frame(cui_context *ui, cui_theme *t, cui_resizable_state *s,
                                  cui_resizable_panel *panels, cui_resizable_options o,
                                  bool overlay) {
    assert(cui_begin_frame(ui, 400, 300, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 400, .height = 300});
    assert(cui_resizable(ui, 100, t, s, panels, 2, o) == CUI_RESIZABLE_OK);
    if (overlay) {
        cui_interact(ui, 999, CUI_ROLE_BUTTON, cui_string("Cover"), false, false, 0);
        cui_begin_portal(ui, 999, (cui_box){.background = {40, 50, 60, 255}},
                         (cui_rect){0, 0, 400, 300}, 10);
        cui_end_box(ui);
    }
    cui_end_box(ui);
    cui_draw_list list = cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return list;
}
typedef struct {
    cui_resizable_state state;
    cui_resizable_slot slots[2];
    cui_resizable_panel panels[2];
    cui_resizable_options options;
    cui_theme *theme;
} nested_group;
static void nested_content(cui_context *ui, void *user) {
    nested_group *n = user;
    assert(cui_resizable(ui, 200, n->theme, &n->state, n->panels, 2, n->options) ==
           CUI_RESIZABLE_OK);
}
static void under_handle_control(cui_context *ui, void *user) {
    unsigned *activations = user;
    cui_interaction interaction =
        cui_interact(ui, 888, CUI_ROLE_BUTTON, cui_string("Pane control"), false, false, 0);
    *activations += interaction.activated;
    cui_begin_box(ui, 888, (cui_box){.width = -1, .height = -1});
    cui_end_box(ui);
}
static void test_edge_appearance(void) {
    cui_context *ui = cui_create((cui_config){.width = 200, .height = 200});
    assert(ui);
    cui_theme theme = cui_theme_light();
    for (int axis = 0; axis < 2; axis++)
        for (int edge = 0; edge < 3; edge++) {
            cui_rect band, line;
            assert(cui_resize_handle_rectangles((cui_resizable_axis)axis, (cui_resize_edge)edge,
                                                (cui_size){200, 200}, &band, &line));
            assert(cui_begin_frame(ui, 200, 200, .016f));
            cui_begin_box(ui, 500, (cui_box){.width = 200, .height = 200, .clip = true});
            cui_begin_absolute_box(ui, 501, (cui_box){0}, band);
            cui_resize_handle_appearance(ui, &theme, (cui_resizable_axis)axis,
                                         (cui_resize_edge)edge, 20, .35);
            cui_end_box(ui);
            cui_end_box(ui);
            cui_draw_list list = cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            bool found = false;
            unsigned clips = 0;
            for (size_t i = 0; i < list.count; i++) {
                const cui_draw_command *command = &list.commands[i];
                clips += command->type == CUI_DRAW_CLIP_PUSH;
                if (command->type == CUI_DRAW_RECT &&
                    (axis == 0 ? command->bounds.w : command->bounds.h) == 1) {
                    near(command->bounds.x, line.x);
                    near(command->bounds.y, line.y);
                    near(command->bounds.w, line.w);
                    near(command->bounds.h, line.h);
                    found = true;
                }
            }
            assert(found && clips > 0);
        }
    cui_destroy(ui);
}
static void test_styled(void) {
    cui_context *ui = cui_create((cui_config){.width = 400, .height = 300});
    assert(ui);
    cui_theme themes[] = {cui_theme_light(), cui_theme_dark(), cui_theme_high_contrast(),
                          cui_theme_terminal()};
    for (size_t ti = 0; ti < 4; ti++)
        for (int axis = 0; axis < 2; axis++) {
            cui_resizable_slot slots[2];
            cui_resizable_state state;
            assert(cui_resizable_state_init(&state, slots, 2));
            cui_resizable_panel panels[] = {cui_resizable_panel_default(10),
                                            cui_resizable_panel_default(11)};
            cui_resizable_options o = cui_resizable_options_default();
            o.axis = (cui_resizable_axis)axis;
            o.extent = 400;
            o.cross_size = 100;
            o.reduced_motion = true;
            cui_draw_list list = styled_frame(ui, &themes[ti], &state, panels, o, false);
            cui_id handle = cui_resizable_handle_id(100, 10);
            cui_rect band = cui_bounds(ui, handle);
            assert((axis == 0 ? band.w : band.h) == 9 && (axis == 0 ? band.h : band.w) == 100);
            bool hairline = false;
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_RECT) {
                    cui_rect b = list.commands[i].bounds;
                    if ((axis == 0 ? b.w : b.h) == 1 && (axis == 0 ? b.h : b.w) == 100) {
                        hairline = true;
                        assert(memcmp(&list.commands[i].color, &themes[ti].border,
                                      sizeof(cui_color)) == 0);
                    }
                }
            assert(hairline);
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = band.x + 4, .y = band.y + 4}));
            list = styled_frame(ui, &themes[ti], &state, panels, o, false);
            assert(slots[0].phase == CUI_RESIZE_HOVERED);
            near((float)slots[0].indicator_length, 20);
            near((float)slots[0].indicator_opacity, .35f);
            bool pill = false;
            for (size_t i = 0; i < list.count; i++)
                if (list.commands[i].type == CUI_DRAW_RECT) {
                    cui_rect b = list.commands[i].bounds;
                    if ((axis == 0 ? b.w : b.h) == 3 && (axis == 0 ? b.h : b.w) == 20) {
                        pill = true;
                        near(axis == 0 ? b.y - band.y : b.x - band.x, 40);
                    }
                }
            assert(pill);
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = band.x + 4, .y = band.y + 4}));
            styled_frame(ui, &themes[ti], &state, panels, o, false);
            assert(slots[0].phase == CUI_RESIZE_PRESSED);
            near((float)slots[0].indicator_length, 28);
            assert(cui_queue_event(
                ui,
                (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = band.x + 20, .y = band.y + 20}));
            styled_frame(ui, &themes[ti], &state, panels, o, false);
            assert(slots[0].phase == CUI_RESIZE_DRAGGING);
            near((float)slots[0].indicator_length, 44);
            assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_CANCEL}));
            styled_frame(ui, &themes[ti], &state, panels, o, true);
            assert(cui_queue_event(
                ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = band.x + 4, .y = band.y + 4}));
            styled_frame(ui, &themes[ti], &state, panels, o, true);
            assert(slots[0].phase == CUI_RESIZE_IDLE);
            near((float)slots[0].indicator_length, 0);
        }
    cui_resizable_slot slots[2];
    cui_resizable_state state;
    assert(cui_resizable_state_init(&state, slots, 2));
    cui_resizable_panel panels[] = {{.id = 10, .has_initial_size = true, .initial_size = 240},
                                    {.id = 11}};
    cui_resizable_options o = cui_resizable_options_default();
    unsigned appearances = 0;
    o.appearance = declined;
    o.appearance_user = &appearances;
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near(slots[0].bounds.w, 240);
    near(slots[1].bounds.w, 160);
    cui_rect band = cui_bounds(ui, cui_resizable_handle_id(100, 10));
    near(band.x, 236);
    near(band.h, 300);
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near(slots[0].size, 240);
    near(slots[1].size, 160);
    assert(cui_resizable_resize_panel(&state, 0, 220));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near(slots[0].size, 220);
    near(slots[0].bounds.w, 220);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 220, .y = 50}));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near((float)slots[0].indicator_length, 0);
    o.now_seconds = .06;
    styled_frame(ui, &themes[0], &state, panels, o, false);
    assert(slots[0].indicator_length > 0 && slots[0].indicator_length < 20);
    o.now_seconds = .3;
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near((float)slots[0].indicator_length, 20);
    assert(appearances >= 5);

    /* No motion or geometry changes leave the UI idle after measurement settles. */
    styled_frame(ui, &themes[0], &state, panels, o, false);
    assert(!cui_needs_redraw(ui));
    nested_group nested = {.theme = &themes[0]};
    assert(cui_resizable_state_init(&nested.state, nested.slots, 2));
    nested.panels[0] = cui_resizable_panel_default(20);
    nested.panels[1] = cui_resizable_panel_default(21);
    nested.options = cui_resizable_options_default();
    nested.options.axis = CUI_RESIZABLE_VERTICAL;
    cui_resizable_clear(&state);
    panels[0] = (cui_resizable_panel){.id = 10, .content = nested_content, .content_user = &nested};
    panels[1] = cui_resizable_panel_default(11);
    o = cui_resizable_options_default();
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near(slots[0].bounds.w, 200);
    near(nested.slots[0].bounds.h, 150);
    cui_id inner_handle = cui_resizable_handle_id(200, 20);
    cui_rect inner = cui_bounds(ui, inner_handle);
    near(inner.y, 146);
    near(inner.w, 200);
    styled_frame(ui, &themes[0], &state, panels, o, false);
    assert(cui_resizable_resize_panel(&state, 0, 250));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near(cui_bounds(ui, inner_handle).w, 250); /* Same-frame nested cross-axis growth. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 50, .y = 148}));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 50, .y = 180}));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near(nested.slots[0].size, 180);
    near(nested.slots[1].size, 120);
    near(slots[0].size, 250); /* Inner capture never resizes its parent. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 50, .y = 180}));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    panels[1].hidden = true;
    styled_frame(ui, &themes[0], &state, panels, o, false);
    assert(cui_bounds(ui, cui_resizable_handle_id(100, 10)).w == 0);
    near(slots[0].size, 400);
    cui_resizable_clear(&state);
    unsigned child_activations = 0;
    panels[0] = cui_resizable_panel_default(10);
    panels[1] = (cui_resizable_panel){
        .id = 11, .content = under_handle_control, .content_user = &child_activations};
    o = cui_resizable_options_default();
    o.extent = 400;
    o.cross_size = 100;
    styled_frame(ui, &themes[0], &state, panels, o, false);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 202, .y = 50}));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    assert(state.active_handle == cui_resizable_handle_id(100, 10));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 220, .y = 50}));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near(slots[0].size, 220);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = 220, .y = 50}));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    assert(child_activations == 0);
    /* Following-panel clipping limits the straddling band and its input. */
    panels[1].box.clip = true;
    styled_frame(ui, &themes[0], &state, panels, o, false);
    cui_rect clipped = cui_bounds(ui, cui_resizable_handle_id(100, 10));
    near(clipped.x, 216);
    near(clipped.w, 9); /* Layout bounds stay full; inherited clip constrains hit region. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = 218, .y = 50}));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    assert(!state.active_handle);
    panels[1].box.clip = false;
    panels[1].box.inert = true;
    cui_focus(ui, cui_resizable_handle_id(100, 10));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT}));
    styled_frame(ui, &themes[0], &state, panels, o, false);
    near(slots[0].size, 220);
    panels[1].box.inert = false;
    /* Invalid declarations leave retained state and semantic output untouched. */
    float old_size = slots[0].size;
    panels[0].custom_range = true;
    panels[0].minimum = NAN;
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_resizable(ui, 100, &themes[0], &state, panels, 2, o) == CUI_RESIZABLE_INVALID);
    cui_end_frame(ui);
    near(slots[0].size, old_size);
    size_t count;
    cui_semantics(ui, &count);
    assert(count == 0);
    panels[0] = cui_resizable_panel_default(10);
    panels[1] = cui_resizable_panel_default(cui_resizable_handle_id(100, 10));
    assert(cui_begin_frame(ui, 400, 300, .016f));
    assert(cui_resizable(ui, 100, &themes[0], &state, panels, 2, o) == CUI_RESIZABLE_INVALID);
    cui_end_frame(ui);
    cui_semantics(ui, &count);
    assert(count == 0);
    cui_destroy(ui);
}
#endif
int main(void) {
    test_handle_geometry();
    test_model();
    test_base_input();
#ifndef CUI_RESIZABLE_BASE_ONLY
    test_edge_appearance();
    test_styled();
#endif
    return 0;
}
