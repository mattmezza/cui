/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static unsigned actions, hovers, slots;
static cui_measure_fn fixture_measure;
static void *fixture_measure_user;
static void action(void *p) {
    (void)p;
    ++actions;
}
static void hover(void *p, bool value) {
    (void)p;
    (void)value;
    ++hovers;
}
static void content(cui_context *ui, void *p) {
    (void)p;
    ++slots;
    cui_begin_box(ui, 0, (cui_box){.width = 8, .height = 8, .background = {200, 20, 80, 255}});
    cui_end_box(ui);
}
static cui_draw_list frame(cui_context *ui, cui_button_state *state, unsigned scope, bool nested,
                           cui_id id, float dt, cui_button_options o) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 360, 160, dt));
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 160, .height = 100, .inert = scope == 1, .layout_only = scope == 2});
    if (nested)
        cui_begin_box(ui, 2, (cui_box){.width = 160, .height = 80});
    assert(cui_frame_paint_enabled(ui) == (scope != 2));
    o.state = state;
    o.on_click = action;
    o.on_hover = hover;
    o.tooltip = cui_string("Passive tooltip");
    o.tooltip_options =
        (cui_hover_options){.viewport = {0, 0, 360, 160}, .open_delay = .1f, .close_delay = .1f};
    bool result =
        cui_button_ex(ui, id, &theme, cui_string("A long label requiring an ellipsis"), o);
    if (scope)
        assert(!result);
    if (nested)
        cui_end_box(ui);
    cui_end_box(ui);
    return cui_end_frame(ui);
}
typedef struct {
    cui_context *ui;
    cui_button_state *state;
    unsigned measure_calls, paint_calls;
} phase_data;
static void phase_attempt(phase_data *p) {
    cui_button_state before;
    memcpy(&before, p->state, sizeof before);
    cui_theme theme = cui_theme_light();
    unsigned a = actions, h = hovers;
    assert(!cui_frame_is_submitting(p->ui));
    assert(!cui_frame_paint_enabled(p->ui));
    assert(!cui_button_ex(
        p->ui, 777, &theme, cui_string("Forbidden phase"),
        (cui_button_options){
            .state = p->state, .on_hover = hover, .on_click = action, .tooltip = {"Tip", 3}}));
    cui_button_paint(
        p->ui, 778, &theme, cui_string("Forbidden paint phase"),
        (cui_button_options){.state = p->state, .label_width = 30, .allow_content_overflow = true});
    assert(!memcmp(&before, p->state, sizeof before) && actions == a && hovers == h);
}
static cui_size guarded_measure(void *user, cui_str text, float font) {
    phase_data *p = user;
    ++p->measure_calls;
    phase_attempt(p);
    return (cui_size){text.length * font * .6f, font * 1.2f};
}
static bool guarded_paint(const cui_prepaint_info *info, const void *payload,
                          cui_prepaint_writer *writer) {
    (void)info;
    (void)writer;
    phase_data *p = *(phase_data *const *)payload;
    ++p->paint_calls;
    phase_attempt(p);
    return true;
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128,
                                              .max_commands = 512,
                                              .frame_bytes = 16384,
                                              .measure = fixture_measure,
                                              .measure_user = fixture_measure_user});
    assert(ui);
    /* Whole caller state is unchanged, including ID reset and tooltip deadlines. */
    for (unsigned scope = 1; scope <= 2; scope++)
        for (unsigned nested = 0; nested < 2; nested++)
            for (unsigned changed = 0; changed < 2; changed++)
                for (unsigned lifecycle = 0; lifecycle < 4; lifecycle++)
                    for (unsigned grow = 0; grow < 2; grow++)
                        for (unsigned nonzero_dt = 0; nonzero_dt < 2; nonzero_dt++) {
                            cui_button_state state = {.id = changed ? 999 : 10,
                                                      .initialized = true,
                                                      .hovered = true,
                                                      .tooltip = {.open = lifecycle == 0,
                                                                  .pending = lifecycle == 1,
                                                                  .target = true,
                                                                  .suppressed = lifecycle == 2,
                                                                  .elapsed = .075f},
                                                      .next_wake_seconds = 42};
                            memset(state.label, 'q', sizeof state.label - 1);
                            cui_button_state before;
                            memcpy(&before, &state, sizeof before);
                            unsigned a = actions, h = hovers, s = slots;
                            cui_button_options o = {.width = grow ? -1 : 160,
                                                    .icon = {"check", 5},
                                                    .dropdown_caret = true,
                                                    .leading_content = content,
                                                    .trailing_content = content};
                            cui_draw_list d =
                                frame(ui, &state, scope, nested, 10, nonzero_dt ? .016f : 0, o);
                            assert(cui_last_error(ui) == CUI_OK);
                            assert(!memcmp(&state, &before, sizeof state));
                            assert(actions == a && hovers == h && slots == s + 2);
                            size_t count;
                            cui_semantics(ui, &count);
                            assert(!count);
                            cui_rect b = cui_bounds(ui, 10);
                            assert(fabsf(b.w - 160) < .01f && b.h > 0);
                            if (scope == 2)
                                assert(!d.count);
                            else {
                                unsigned texts = 0, images = 0;
                                for (size_t i = 0; i < d.count; i++) {
                                    texts += d.commands[i].type == CUI_DRAW_TEXT;
                                    images += d.commands[i].type == CUI_DRAW_IMAGE;
                                }
                                /* leading_content replaces the named leading icon; caret remains.
                                 */
                                assert(texts && images == 1);
                            }
                        }
    /* Legacy overflow opt-out plus explicit capped ellipsis is also read-only. */
    for (unsigned scope = 1; scope <= 2; ++scope) {
        cui_button_state legacy = {
            .id = 10, .initialized = true, .hovered = true, .next_wake_seconds = 42};
        strcpy(legacy.label, "Preserve the caller buffer");
        cui_button_state copy;
        memcpy(&copy, &legacy, sizeof copy);
        frame(
            ui, &legacy, scope, true, 10, .016f,
            (cui_button_options){.width = 160, .label_width = 30, .allow_content_overflow = true});
        assert(!memcmp(&legacy, &copy, sizeof copy));
    }
    /* Hidden layout-only loading must not schedule visual animation redraw. */
    for (unsigned named = 0; named < 2; ++named) {
        cui_button_state loading_state = {.id = 10, .initialized = true, .next_wake_seconds = 42};
        cui_button_state copy;
        memcpy(&copy, &loading_state, sizeof copy);
        cui_button_options loading = {.width = 160,
                                      .loading = true,
                                      .loading_icon = named ? cui_string("check") : (cui_str){0}};
        cui_draw_list hidden = frame(ui, &loading_state, 2, true, 10, .016f, loading);
        assert(!hidden.count && !cui_needs_redraw(ui));
        cui_rect hidden_bounds = cui_bounds(ui, 10);
        assert(!memcmp(&copy, &loading_state, sizeof copy));
        cui_draw_list visible = frame(ui, &loading_state, 1, true, 10, .016f, loading);
        assert(visible.count && cui_needs_redraw(ui));
        cui_rect visible_bounds = cui_bounds(ui, 10);
        assert(!memcmp(&hidden_bounds, &visible_bounds, sizeof hidden_bounds));
        assert(!memcmp(&copy, &loading_state, sizeof copy));
        frame(ui, &loading_state, 0, true, 10, .016f, loading);
        assert(cui_needs_redraw(ui));
    }
    /* Ordinary disabled/loading still reconcile hover and tooltip state. */
    for (unsigned loading = 0; loading < 2; loading++) {
        cui_button_state state = {.id = 10,
                                  .initialized = true,
                                  .hovered = true,
                                  .tooltip = {.open = true, .pending = true},
                                  .next_wake_seconds = 42};
        unsigned a = actions, h = hovers;
        frame(ui, &state, 0, false, 10, .016f,
              (cui_button_options){.width = 160, .disabled = !loading, .loading = loading});
        assert(cui_last_error(ui) == CUI_OK && !state.hovered && !state.tooltip.open);
        assert(actions == a && hovers == h);
        size_t count;
        const cui_semantic *s = cui_semantics(ui, &count);
        assert(count == 1 && s[0].disabled);
    }
    /* Passive ID reuse does not reset state; ordinary reuse does. */
    cui_button_state state = {.id = 999, .initialized = true, .hovered = true};
    frame(ui, &state, 1, true, 10, 0, (cui_button_options){.width = -1});
    assert(state.id == 999);
    frame(ui, &state, 0, true, 10, 0, (cui_button_options){.width = -1});
    assert(state.id == 10);
    /* Genuine touch tap after passive frames activates once, drag-cancel never does. */
    cui_rect b = cui_bounds(ui, 10);
    float x = b.x + b.w * .5f, y = b.y + b.h * .5f;
    unsigned a = actions;
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = x,
                                           .y = y,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 7}));
    frame(ui, &state, 0, true, 10, .016f, (cui_button_options){.width = -1});
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                           .x = x,
                                           .y = y,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 7}));
    frame(ui, &state, 0, true, 10, .016f, (cui_button_options){.width = -1});
    assert(actions == a + 1);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                           .x = x,
                                           .y = y,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 8}));
    frame(ui, &state, 1, true, 10, .016f, (cui_button_options){.width = -1});
    assert(cui_queue_event(
        ui,
        (cui_event){.type = CUI_EVENT_CANCEL, .pointer_type = CUI_POINTER_TOUCH, .pointer_id = 8}));
    frame(ui, &state, 0, true, 10, .016f, (cui_button_options){.width = -1});
    assert(actions == a + 1);
    /* Outside submission never mutates a reused state. */
    cui_button_state before;
    memcpy(&before, &state, sizeof before);
    cui_theme t = cui_theme_light();
    assert(!cui_button_ex(
        ui, 10, &t, cui_string("Outside"),
        (cui_button_options){
            .state = &state, .on_hover = hover, .on_click = action, .tooltip = {"Tooltip", 7}}));
    assert(!memcmp(&state, &before, sizeof state));
    cui_button_paint(
        ui, 10, &t, cui_string("Outside paint"),
        (cui_button_options){.state = &state, .label_width = 30, .allow_content_overflow = true});
    assert(!memcmp(&state, &before, sizeof state));
    cui_destroy(ui);
    /* Measurement/prepaint cannot start metadata, callbacks or retained state work. */
    phase_data phases = {.state = &state};
    ui = cui_create((cui_config){.max_elements = 128,
                                 .max_commands = 512,
                                 .frame_bytes = 16384,
                                 .measure = guarded_measure,
                                 .measure_user = &phases});
    assert(ui);
    phases.ui = ui;
    assert(cui_begin_frame(ui, 120, 80, 0));
    cui_begin_box(ui, 1, (cui_box){.width = 120, .height = 40});
    phase_data *borrowed = &phases;
    assert(cui_set_prepaint(ui, (cui_prepaint_options){.callback = guarded_paint,
                                                       .payload = &borrowed,
                                                       .payload_bytes = sizeof borrowed,
                                                       .command_budget = 1}));
    cui_text(ui, cui_string("Measure"), 14, t.text);
    cui_end_box(ui);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK && phases.measure_calls && phases.paint_calls == 1);
    cui_destroy(ui);
    return 0;
}
