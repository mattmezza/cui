/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <assert.h>
#include <math.h>
#include <string.h>

typedef struct {
    cui_context *ui;
    cui_theme theme;
    bool value, accept, toggle, legacy, readonly, disabled, inert, layout_only;
    unsigned conflict;
    bool reject_legacy, mutate_content;
    unsigned requests, legacy_calls, content_calls;
    cui_switch_state motion;
    cui_hover_state tooltip;
    cui_control_size size;
    cui_design_size design_size;
    cui_component_boolean_request copies[16];
} model;
static void requested(void *user, const cui_component_boolean_request *request) {
    model *m = user;
    assert(m->requests < 16 && request->event.target == 10);
    assert(request->event.kind == CUI_COMPONENT_EVENT_REQUEST);
    m->copies[m->requests++] = *request;
    if (m->accept)
        m->value = request->requested;
}
static void legacy(void *user, bool value) {
    model *m = user;
    ++m->legacy_calls;
    m->value = m->reject_legacy ? false : value;
}
static void content(cui_context *ui, void *user) {
    model *m = user;
    assert(ui == m->ui);
    ++m->content_calls;
    if (m->mutate_content)
        m->value = true;
    cui_begin_box(ui, 77, (cui_box){.width = 13, .height = 11,
                                   .background = {31, 32, 33, 255}});
    cui_end_box(ui);
}
static const cui_semantic *node(cui_context *ui, cui_id id) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    for (size_t n = 0; n < count; ++n)
        if (nodes[n].id == id)
            return &nodes[n];
    return NULL;
}
static bool same_color(cui_color a, cui_color b) {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}
static bool painted(cui_draw_list draw, cui_rect bounds, cui_color color) {
    for (size_t n = 0; n < draw.count; ++n)
        if (draw.commands[n].type == CUI_DRAW_RECT &&
            fabsf(draw.commands[n].bounds.x - bounds.x) < .01f &&
            fabsf(draw.commands[n].bounds.y - bounds.y) < .01f &&
            fabsf(draw.commands[n].bounds.w - bounds.w) < .01f &&
            fabsf(draw.commands[n].bounds.h - bounds.h) < .01f &&
            same_color(draw.commands[n].color, color))
            return true;
    return false;
}
static cui_component_action_result submit(model *m) {
    cui_component_boolean_handler handler = {m->readonly ? NULL : requested, m};
    if (m->toggle) {
        cui_toggle_options o = {.width = 140, .size = m->size, .design_size = m->design_size,
                                 .disabled = m->disabled, .variant = CUI_TOGGLE_OUTLINE,
                                 .accessibility_label = CUI_STR("Toggle flag"), .tooltip = CUI_STR("Details"),
                                 .content = content, .content_user = m,
                                 .on_change = m->legacy || m->conflict == 1 || m->conflict == 3 ? legacy : NULL,
                                 .change_user = m->legacy || m->conflict == 2 || m->conflict == 3 ? m : NULL};
        if (m->legacy) {
            bool changed = cui_toggle_ex(m->ui, 10, &m->theme, cui_string("Flag"), &m->value,
                                          &m->tooltip, o);
            return (cui_component_action_result){.valid = true, .submitted = true,
                                                  .activation_count = changed ? 1 : 0};
        }
        return cui_toggle_events(m->ui, 10, &m->theme, cui_string("Flag"), m->value,
                                  &m->tooltip, o, handler);
    }
    cui_switch_options o = {.width = 140, .size = m->size, .design_size = m->design_size,
                             .disabled = m->disabled, .reduced_motion = true, .label_left = true,
                             .accessibility_label = CUI_STR("Switch flag"), .tooltip = CUI_STR("Details"),
                             .content = content, .content_user = m,
                             .on_change = m->legacy || m->conflict == 1 || m->conflict == 3 ? legacy : NULL,
                             .change_user = m->legacy || m->conflict == 2 || m->conflict == 3 ? m : NULL};
    if (m->legacy) {
        bool changed = cui_switch_ex(m->ui, 10, &m->theme, cui_string("Flag"), &m->value,
                                      &m->motion, o);
        return (cui_component_action_result){.valid = true, .submitted = true,
                                              .activation_count = changed ? 1 : 0};
    }
    return cui_switch_events(m->ui, 10, &m->theme, cui_string("Flag"), m->value,
                              &m->motion, o, handler);
}
static cui_draw_list frame(model *m, cui_component_action_result *out) {
    assert(cui_begin_frame(m->ui, 350, 240, .016f));
    cui_begin_box(m->ui, 1, (cui_box){.width = 350, .height = 240,
                                     .inert = m->inert, .layout_only = m->layout_only});
    cui_component_action_result r = submit(m);
    cui_end_box(m->ui);
    cui_draw_list draw = cui_end_frame(m->ui);
    assert(cui_last_error(m->ui) == CUI_OK);
    if (out)
        *out = r;
    return draw;
}
static model create(bool toggle) {
    model m = {.ui = cui_create((cui_config){0}), .theme = cui_theme_light(), .toggle = toggle};
    assert(m.ui);
    frame(&m, NULL);
    frame(&m, NULL);
    m.content_calls = 0;
    return m;
}
static void accessibility(cui_context *ui) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                         .accessibility_target = 10,
                                         .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
}
static void click(cui_context *ui) {
    cui_rect b = cui_bounds(ui, 10);
    cui_event e = {.type = CUI_EVENT_POINTER_DOWN, .x = b.x + b.w / 2, .y = b.y + b.h / 2};
    assert(cui_queue_event(ui, e));
    e.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(ui, e));
}
static void snapshot(bool toggle, bool accept, bool initial) {
    model m = create(toggle);
    m.accept = accept;
    m.value = initial;
    frame(&m, NULL);
    m.content_calls = 0;
    accessibility(m.ui);
    accessibility(m.ui);
    cui_component_action_result r;
    cui_draw_list draw = frame(&m, &r);
    assert(r.valid && r.submitted && r.activation_count == 2 && r.request_count == 2);
    assert(m.requests == 2 && m.value == (accept ? !initial : initial) && m.content_calls == 1 && !m.legacy_calls);
    for (unsigned i = 0; i < 2; ++i) {
        assert(m.copies[i].current == initial && m.copies[i].requested == !initial);
        assert(m.copies[i].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    }
    const cui_semantic *s = node(m.ui, 10);
    assert(s && s->checked == initial && s->value == (initial ? 1 : 0) && s->role == (toggle ? CUI_ROLE_BUTTON : CUI_ROLE_SWITCH));
    assert(s->name.length == (toggle ? strlen("Toggle flag") : strlen("Switch flag")));
    cui_rect b = cui_bounds(m.ui, toggle ? 10 : cui_switch_track_id(10));
    assert(painted(draw, b, initial ? (toggle ? m.theme.accent : m.theme.primary)
                                   : (toggle ? m.theme.background : m.theme.switch_background)));
    assert(toggle || (initial ? m.motion.target > 0 : m.motion.target == 0));
    frame(&m, NULL);
    assert(node(m.ui, 10)->checked == (accept ? !initial : initial));
    cui_destroy(m.ui);
    assert(m.copies[1].event.target == 10);
}
static void input_and_readonly(bool toggle) {
    model m = create(toggle);
    click(m.ui);
    click(m.ui);
    cui_component_action_result r;
    frame(&m, &r);
    assert(r.activation_count == 2 && m.requests == 2 && !m.value);
    cui_focus(m.ui, 10);
    assert(cui_queue_event(m.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(m.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
    assert(cui_queue_event(m.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    frame(&m, &r);
    assert(r.activation_count == 2 && m.requests == 4);
    m.readonly = true;
    accessibility(m.ui);
    frame(&m, &r);
    assert(r.activation_count == 1 && !r.request_count && m.requests == 4 && !m.value);
    m.disabled = true;
    frame(&m, &r);
    assert(r.submitted && !r.activation_count && m.requests == 4);
    click(m.ui);
    frame(&m, &r);
    assert(!r.activation_count && m.requests == 4);
    cui_destroy(m.ui);
}
static void sizes(bool toggle) {
    model m = create(toggle);
    for (int size = CUI_CONTROL_MEDIUM; size <= CUI_CONTROL_XSMALL; ++size) {
        m.size = (cui_control_size)size;
        frame(&m, NULL);
        cui_rect b = cui_bounds(m.ui, toggle ? 10 : cui_switch_track_id(10));
        float height = size == CUI_CONTROL_XSMALL ? (toggle ? 20 : 16)
                       : size == CUI_CONTROL_SMALL ? (toggle ? 24 : 16)
                       : size == CUI_CONTROL_LARGE ? (toggle ? 36 : 24)
                                                  : (toggle ? 32 : 20);
        assert(b.h == height && cui_bounds(m.ui, 10).w == 140 && cui_bounds(m.ui, 77).w == 13);
    }
    m.design_size = CUI_DESIGN_LG;
    cui_design_metrics design;
    assert(cui_design_measure(m.design_size, m.theme.font_size / 16, &design));
    frame(&m, NULL);
    assert(cui_bounds(m.ui, toggle ? 10 : cui_switch_track_id(10)).h ==
           (toggle ? design.control_height : design.indicator_size + 4));
    cui_destroy(m.ui);
}
static void passive_conflict(bool toggle) {
    model m = create(toggle);
    cui_switch_state before_motion = m.motion;
    cui_hover_state before_tooltip = m.tooltip;
    cui_component_action_result r = submit(&m); /* Outside frame is passive. */
    assert(r.valid && !r.submitted && !m.requests && !m.content_calls);
    for (unsigned conflict = 1; conflict <= 3; ++conflict) {
        m.conflict = conflict;
        cui_draw_list draw = frame(&m, &r);
        assert(!r.valid && !r.submitted && !m.requests && !m.content_calls && !draw.count);
        assert(!node(m.ui, 10));
        assert(memcmp(&m.motion, &before_motion, sizeof before_motion) == 0);
        assert(memcmp(&m.tooltip, &before_tooltip, sizeof before_tooltip) == 0);
    }
    m.conflict = false;
    for (unsigned mode = 0; mode < 2; ++mode) {
        m.inert = mode == 0;
        m.layout_only = mode == 1;
        frame(&m, &r);
        assert(r.valid && !r.submitted && !m.requests && !m.legacy_calls);
        assert(memcmp(&m.motion, &before_motion, sizeof before_motion) == 0);
        assert(memcmp(&m.tooltip, &before_tooltip, sizeof before_tooltip) == 0);
    }
    cui_destroy(m.ui);
}
static void legacy_timing(bool toggle) {
    model m = create(toggle);
    m.legacy = true;
    accessibility(m.ui);
    accessibility(m.ui);
    cui_component_action_result r;
    cui_draw_list draw = frame(&m, &r);
    assert(r.activation_count == 1 && m.legacy_calls == 1 && m.value && !m.requests);
    assert(node(m.ui, 10)->checked);
    assert(painted(draw, cui_bounds(m.ui, toggle ? 10 : cui_switch_track_id(10)),
                   toggle ? m.theme.accent : m.theme.primary));
    assert(toggle || m.motion.target > 0); /* Legacy callback assignment precedes spring. */
    if (!toggle) {
        m.value = false;
        m.reject_legacy = true;
        frame(&m, NULL);
        m.mutate_content = true;
        accessibility(m.ui);
        draw = frame(&m, &r);
        assert(m.value && !node(m.ui, 10)->checked && m.motion.target == 0);
        assert(painted(draw, cui_bounds(m.ui, cui_switch_track_id(10)), m.theme.primary));
        /* Label-left content still runs between target calculation and color read. */
    }
    cui_destroy(m.ui);
}
int main(void) {
    for (unsigned toggle = 0; toggle < 2; ++toggle) {
        snapshot(toggle != 0, false, false);
        snapshot(toggle != 0, true, false);
        snapshot(toggle != 0, false, true);
        snapshot(toggle != 0, true, true);
        input_and_readonly(toggle != 0);
        sizes(toggle != 0);
        passive_conflict(toggle != 0);
        legacy_timing(toggle != 0);
    }
    return 0;
}
