/* SPDX-License-Identifier: MIT */
#include "cui/cui_chart.h"
#include "cui/cui_plot_view.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_size measure(void *user, cui_str text, float font, cui_text_style style) {
    (void)user;
    (void)style;
    return (cui_size){text.length * font * .5f, font};
}
static bool popup(cui_draw_list draw) {
    for (size_t i = 0; i < draw.count; i++)
        if (draw.commands[i].type == CUI_DRAW_TEXT &&
            draw.commands[i].text_style.weight == CUI_TEXT_WEIGHT_SEMIBOLD &&
            draw.commands[i].text.length == 3 && !memcmp(draw.commands[i].text.data, "One", 3))
            return true;
    return false;
}
static cui_draw_list frame(cui_context *ui, cui_chart_state *state, cui_chart_options *o,
                           float scroll, float left, bool overlay, bool modal) {
    assert(cui_begin_frame(ui, 400, 240, .016f));
    size_t event_count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t i = 0; i < event_count; i++)
        if (events[i].type == CUI_EVENT_SCROLL) {
            assert(cui_event_target(ui, i) == 1);
            assert(cui_consume_event(ui, i));
        }
    cui_begin_box(ui, 100, (cui_box){.width = 400, .height = 240});
    cui_begin_absolute_box(ui, 1, (cui_box){.clip = true, .custom_opacity = true, .opacity = .5f},
                           (cui_rect){left, 10, 180, 100});
    cui_interact(ui, 1, CUI_ROLE_GROUP, cui_string("scroll"), false, false, 0);
    cui_set_semantic_details(
        ui, 1, (cui_semantic_details){.scrollable = true, .scroll_y = scroll, .scroll_max_y = 100});
    cui_begin_absolute_box(ui, 2, (cui_box){0}, (cui_rect){0, -scroll, 128, 96});
    assert(cui_chart(ui, 3, state, o, cui_string("Chart")));
    cui_end_box(ui);
    cui_end_box(ui);
    if (overlay) {
        cui_begin_portal(ui, 9, (cui_box){.inert = true, .background = {250, 0, 0, 255}},
                         (cui_rect){left, 10, 180, 100}, 10);
        assert(cui_set_hover_hitbox(ui, true));
        cui_end_box(ui);
    }
    if (modal) {
        assert(cui_begin_focus_scope(ui, 10, true));
        cui_begin_portal(ui, 11, (cui_box){.background = {0, 250, 0, 255}},
                         (cui_rect){300, 150, 40, 40}, 20);
        cui_end_box(ui);
        cui_end_focus_scope(ui);
    }
    cui_end_box(ui);
    cui_draw_list draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == 0);
    return draw;
}
static void chart_geometry(void) {
    unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.label = {"One", 3}, .value = 20},
                              {.label = {"Two", 3}, .value = 40}};
    cui_chart_options o = {.kind = CUI_CHART_LINE,
                           .data = data,
                           .count = 2,
                           .width = 128,
                           .height = 96,
                           .disable_appear = true,
                           .reduced_motion = true,
                           .interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME,
                           .hide_axes = true,
                           .background = {255, 255, 255, 255},
                           .foreground = {0, 0, 0, 255},
                           .grid = {80, 80, 80, 255},
                           .series = {{0, 100, 240, 255}}};
    cui_chart_state s = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    cui_context *ui = cui_create(
        (cui_config){.max_commands = 1024, .frame_bytes = 65536, .measure_styled = measure});
    assert(ui);
    cui_draw_list d = frame(ui, &s, &o, 0, 10, false, false);
    assert(s.hovered == SIZE_MAX && !popup(d));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 40, .y = 40}));
    d = frame(ui, &s, &o, 0, 10, false, false);
    assert(s.hovered == 0 && popup(d));
    assert(s.bounds.x == 10 && s.bounds.y == 10 && s.tooltip_cursor_y == 30);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 40, .y = 40, .dy = -15}));
    d = frame(ui, &s, &o, 15, 10, false, false);
    assert(s.hovered == 0 && popup(d));
    assert(s.bounds.y == -5 && s.tooltip_cursor_y == 45); /* no pointer event */
    bool clipped = false, opacity = false;
    for (size_t i = 0; i < d.count; i++) {
        const cui_draw_command *command = d.commands + i;
        clipped |= command->type == CUI_DRAW_CLIP_PUSH && command->bounds.y == 10 &&
                   command->bounds.h == 81;
        opacity |= command->type == CUI_DRAW_IMAGE && command->bitmap.opacity == .5f;
    }
    assert(clipped && opacity);
    d = frame(ui, &s, &o, 15, 25, false, false);
    assert(s.bounds.x == 25 && s.tooltip_cursor_x == 15 && popup(d));
    d = frame(ui, &s, &o, 15, 25, true, false);
    assert(s.hovered == SIZE_MAX && !popup(d));
    d = frame(ui, &s, &o, 15, 25, false, false);
    assert(s.hovered == 0 && popup(d));
    d = frame(ui, &s, &o, 15, 25, false, true);
    assert(s.hovered == SIZE_MAX && !popup(d));
    /* AUTO preserves explicit/manual legacy hover even when Core pointer is known. */
    o.interaction_mode = CUI_PLOT_INTERACTION_AUTO;
    assert(cui_chart_update_hover(&s, &o, 100, 30).hovered == 1);
    d = frame(ui, &s, &o, 15, 25, false, false);
    assert(s.hovered == 1);
    cui_destroy(ui);
}
typedef struct {
    cui_prepaint_info info;
    unsigned calls;
    bool fail;
    cui_plot_motion_pointer_state motion;
} paint_user;
static cui_plot_result resolve(cui_plot_point point, cui_plot_rect bounds,
                               cui_plot_tooltip_state *out, void *user) {
    (void)bounds;
    (void)user;
    *out = (cui_plot_tooltip_state){.cross_line = point};
    return CUI_PLOT_OK;
}
static bool marks(const cui_prepaint_info *info, cui_prepaint_writer *writer,
                  const cui_theme *theme, const cui_plot_hover_memory *hover, void *user) {
    (void)theme;
    paint_user *p = user;
    p->info = *info;
    p->calls++;
    if (p->fail)
        return false;
    if (!hover)
        return true;
    cui_plot_spring_value value;
    assert(cui_plot_motion_follow_at(info->owner, &p->motion, cui_plot_spring_defaults(.12),
                                     hover->state.cross_line.x, info->frame_time, true, false,
                                     &value) == CUI_PLOT_OK);
    if (value.request_frame)
        cui_prepaint_request_frame(writer);
    cui_draw_command mark = {.type = CUI_DRAW_RECT,
                             .bounds = {info->bounds.x + (float)value.value, info->bounds.y, 3, 20},
                             .color = {240, 0, 0, 255}};
    return cui_prepaint_emit(writer, &mark, false);
}
static bool tooltip(const cui_prepaint_info *info, cui_prepaint_writer *writer,
                    const cui_theme *theme, const cui_plot_hover_memory *hover, void *user) {
    (void)theme;
    (void)hover;
    (void)user;
    cui_draw_command c = {.type = CUI_DRAW_RECT,
                          .bounds = {info->bounds.x + 140, info->bounds.y, 20, 20},
                          .color = {0, 240, 0, 255}};
    return cui_prepaint_emit(writer, &c, true);
}
static void plot_geometry(void) {
    static unsigned char pixels[128 * 96 * 4];
    cui_context *ui = cui_create((cui_config){.max_commands = 256, .frame_bytes = 65536});
    assert(ui);
    cui_set_reduced_motion(ui, true);
    cui_theme theme = {0};
    paint_user user = {0};
    cui_plot_element_state state = {0};
    cui_plot_view_options view = {
        .bitmap = {pixels, 128, 96, 128 * 4, 1}, .interactive = true, .name = {"Plot", 4}};
    cui_plot_element_options options = {.interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME,
                                        .paint_marks = marks,
                                        .paint_tooltip = tooltip,
                                        .paint_command_budget = 2,
                                        .resolve = resolve,
                                        .user = &user};
    for (unsigned frame_index = 0; frame_index < 5; frame_index++) {
        if (frame_index == 1)
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 40, .y = 40}));
        assert(cui_begin_frame(ui, 400, 240, .016f));
        cui_begin_box(ui, 1, (cui_box){.width = 400, .height = 240});
        cui_begin_absolute_box(ui, 2, (cui_box){.clip = true},
                               (cui_rect){frame_index >= 2 ? 25.5f : 10, 10, 100, 100});
        assert(cui_plot_element(ui, 3, &theme, &view, &state, &options) == CUI_PLOT_OK);
        cui_end_box(ui);
        if (frame_index == 3) {
            cui_begin_portal(ui, 9, (cui_box){.inert = true}, (cui_rect){20, 20, 40, 40}, 10);
            assert(cui_set_hover_hitbox(ui, true));
            cui_end_box(ui);
        }
        if (frame_index == 4) {
            assert(cui_begin_focus_scope(ui, 10, true));
            cui_begin_portal(ui, 11, (cui_box){0}, (cui_rect){300, 150, 40, 40}, 20);
            cui_end_box(ui);
            cui_end_focus_scope(ui);
        }
        cui_end_box(ui);
        cui_draw_list d = cui_end_frame(ui);
        assert(d.count && !cui_last_error(ui));
        assert(user.calls == frame_index + 1);
        assert(user.info.pointer_visible == (frame_index == 1 || frame_index == 2));
        if (frame_index == 2)
            assert(state.hover.has_state && state.hover.state.cross_line.x == 14.5 &&
                   state.current_bounds.x == 25.5f);
        if (frame_index >= 3)
            assert(!state.hover.has_state);
    }
    /* A custom command budget exhaustion does not publish prepared hover/state. */
    options.paint_command_budget = 1;
    cui_rect previous = state.current_bounds;
    assert(cui_begin_frame(ui, 400, 240, .016f));
    assert(cui_plot_element(ui, 3, &theme, &view, &state, &options) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(!state.hover.has_state && state.current_bounds.x == previous.x);
    cui_clear_error(ui);
    /* Explicit host geometry remains authoritative under AUTO, even with known Core pointer. */
    options = (cui_plot_element_options){
        .bounds = {0, 0, 128, 96}, .pointer = {100, 30}, .hovered = true, .resolve = resolve};
    assert(cui_begin_frame(ui, 400, 240, .016f));
    assert(cui_plot_element(ui, 3, &theme, &view, &state, &options) == CUI_PLOT_OK);
    (void)cui_end_frame(ui);
    assert(state.hover.has_state && state.hover.state.cross_line.x == 100);
    cui_destroy(ui);
}
static void current_motion(void) {
    static unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {{.label = {"One", 3}, .value = 20},
                              {.label = {"Two", 3}, .value = 40}};
    cui_chart_options o = {.kind = CUI_CHART_LINE,
                           .data = data,
                           .count = 2,
                           .width = 128,
                           .height = 96,
                           .interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME,
                           .custom_motion = true,
                           .hide_axes = true,
                           .background = {255, 255, 255, 255},
                           .foreground = {0, 0, 0, 255},
                           .series = {{0, 100, 240, 255}}};
    cui_theme_motion_tokens tokens = cui_theme_motion_default();
    assert(cui_plot_motion_from_theme(&tokens, &o.motion) == CUI_PLOT_OK);
    cui_chart_state state = {.pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
    cui_context *ui = cui_create(
        (cui_config){.max_commands = 1024, .frame_bytes = 65536, .measure_styled = measure});
    assert(ui);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 30, .y = 30}));
    const float deltas[] = {0, .999f, .002f, .2f, .2f};
    for (size_t i = 0; i < 5; i++) {
        assert(cui_begin_frame(ui, 300, 200, deltas[i]));
        assert(cui_chart(ui, 3, &state, &o, cui_string("Chart")));
        cui_draw_list d = cui_end_frame(ui);
        assert(d.count && !cui_last_error(ui));
        if (i == 1)
            assert(state.rendered_reveal_progress == 1 && state.hovered == SIZE_MAX &&
                   cui_needs_redraw(ui));
        if (i >= 2)
            assert(state.hovered == 0);
        if (i == 4)
            assert(popup(d) && !cui_needs_redraw(ui));
    }
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 250, .y = 150}));
    for (unsigned i = 0; i < 3; i++) {
        assert(cui_begin_frame(ui, 300, 200, .2f));
        assert(cui_chart(ui, 3, &state, &o, cui_string("Chart")));
        cui_draw_list d = cui_end_frame(ui);
        if (i == 2)
            assert(!popup(d) && !cui_needs_redraw(ui));
    }
    /* A restarted appearance must not resurrect the now-outside stationary pointer. */
    o.appear_key++;
    const float restart_deltas[] = {0, .999f, .002f};
    for (size_t i = 0; i < 3; i++) {
        assert(cui_begin_frame(ui, 300, 200, restart_deltas[i]));
        assert(cui_chart(ui, 3, &state, &o, cui_string("Chart")));
        cui_draw_list d = cui_end_frame(ui);
        assert(state.hovered == SIZE_MAX && !state.appear_hover_pending && !popup(d));
        if (i == 2)
            assert(!cui_needs_redraw(ui));
    }
    cui_plot_motion_pointer_state spring = {0};
    cui_plot_spring_value value = {.value = 42};
    assert(cui_plot_motion_follow_at(3, &spring, o.motion.pointer, 1, NAN, false, false, &value) ==
           CUI_PLOT_INVALID);
    assert(!spring.owner && value.value == 42);
    cui_destroy(ui);
}
static void chart_kind_paint(void) {
    const cui_chart_kind kinds[] = {CUI_CHART_BAR, CUI_CHART_PIE, CUI_CHART_CANDLESTICK,
                                    CUI_CHART_RADAR};
    static unsigned char pixels[128 * 96 * 4];
    cui_chart_datum data[] = {
        {.label = {"One", 3}, .value = 20, .open = 10, .high = 30, .low = 5, .close = 20},
        {.label = {"Two", 3}, .value = 40, .open = 20, .high = 50, .low = 10, .close = 40},
        {.label = {"Tri", 3}, .value = 30, .open = 20, .high = 40, .low = 10, .close = 30}};
    for (size_t k = 0; k < sizeof kinds / sizeof *kinds; k++) {
        cui_chart_options o = cui_chart_theme(
            (cui_chart_options){.kind = kinds[k],
                                .data = data,
                                .count = 3,
                                .width = 128,
                                .height = 96,
                                .disable_appear = true,
                                .reduced_motion = true,
                                .hide_axes = true,
                                .interaction_mode = CUI_PLOT_INTERACTION_CURRENT_FRAME},
            &(cui_theme){.background = {255, 255, 255, 255},
                         .text = {0, 0, 0, 255},
                         .chart_2 = {0, 100, 240, 255}});
        cui_chart_state state = {
            .pixels = pixels, .byte_capacity = sizeof pixels, .hovered = SIZE_MAX};
        assert(cui_chart_render(&state, &o) == CUI_PLOT_OK);
        cui_chart_hit hit = {0};
        float hx = 0, hy = 0;
        for (unsigned y = 2; y < 90 && !hit.valid; y += 2)
            for (unsigned x = 2; x < 126 && !hit.valid; x += 2) {
                hit = cui_chart_hit_test(&o, (float)x, (float)y);
                if (hit.valid) {
                    hx = (float)x;
                    hy = (float)y;
                }
            }
        assert(hit.valid);
        cui_context *ui = cui_create((cui_config){.max_commands = 1024, .frame_bytes = 65536});
        assert(ui);
        assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = hx, .y = hy}));
        assert(cui_begin_frame(ui, 300, 200, .016f));
        assert(cui_chart(ui, 3, &state, &o, cui_string("Chart")));
        cui_draw_list draw = cui_end_frame(ui);
        assert(draw.count && !cui_last_error(ui));
        assert(state.hovered == hit.hovered && state.tooltip_focus == 1 && !cui_needs_redraw(ui));
        bool image = false;
        for (size_t i = 0; i < draw.count; i++)
            image |= draw.commands[i].type == CUI_DRAW_IMAGE;
        assert(image);
        cui_destroy(ui);
    }
}
int main(void) {
    chart_geometry();
    plot_geometry();
    current_motion();
    chart_kind_paint();
    return 0;
}
