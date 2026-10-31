/* SPDX-License-Identifier: MIT */
/* Golden trace recorded from signed b0724c68's pre-hook Base source, with the
 * same cached Core/math objects. Fieldwise IEEE-value hashing, no object padding,
 * addresses, native host, platform sizeof or renderer output participates. */
#include "cui/cui_base_slider_model.h"
#ifndef CUI_SLIDER_TRACE_RECORD
#include "../../src/base/cui_base_slider_model_internal.h"
#include "../fixtures/slider_style_null_trace.inc"
#endif
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
static uint64_t trace;
static cui_context *active;
static cui_slider_model_state_v1 *model;
static void u(uint64_t n) {
    for (unsigned i = 0; i < 8; ++i) {
        trace = (trace ^ (n & 255)) * UINT64_C(1099511628211);
        n >>= 8;
    }
}
static void f(float n) {
    uint32_t bits;
    memcpy(&bits, &n, sizeof bits);
    u(bits);
}
static void d(double n) {
    uint64_t bits;
    memcpy(&bits, &n, sizeof bits);
    u(bits);
}
static void rect(cui_rect r) {
    f(r.x);
    f(r.y);
    f(r.w);
    f(r.h);
}
static void value(cui_slider_model_value_v1 v) {
    u(v.kind);
    d(v.start);
    d(v.end);
}
static void callback(void *user, const cui_slider_model_request_v1 *r) {
    assert(user == model);
    u(1000);
    u(r->phase);
    u(r->reason);
    u(r->thumb);
    u(r->submission_frame);
    u(r->request_sequence);
    u(r->source_generation);
    u(r->event.kind);
    u(r->event.target);
    u(r->event.flags);
    u(r->event.frame_sequence);
    u((r->event.flags & CUI_COMPONENT_EVENT_HAS_INDEX) ? r->event.event_index : 0);
    u(r->event.source);
    value(r->current);
    value(r->requested);
    u(r->input.type);
    u(r->input.flags);
    u(r->input.key);
    f(r->input.x);
    f(r->input.y);
    u(model->flags);
    u(model->active_thumb);
    value(model->preview);
    rect(cui_bounds(active, cui_base_slider_model_part_id_v1(100, CUI_SLIDER_MODEL_TRACK)));
    size_t count;
    (void)cui_semantics(active, &count);
    u(count);
}
static void queue(cui_context *ui, cui_event_type t, bool vertical, float x) {
    assert(cui_queue_event(ui, (cui_event){.type = t,
                                           .x = vertical ? 12 : x,
                                           .y = vertical ? x : 12,
                                           .pointer_type = CUI_POINTER_MOUSE}));
}
static uint64_t run(unsigned variant, bool internal) {
    trace = UINT64_C(14695981039346656037);
    cui_context *ui = cui_create((cui_config){.max_elements = 64, .max_commands = 128});
    assert(ui);
    active = ui;
    cui_slider_model_state_v1 s;
    assert(cui_slider_model_state_init_v1(sizeof s, &s));
    model = &s;
    bool vertical = (variant & 1) != 0, range = (variant & 2) != 0;
    cui_slider_model_value_v1 v = {
        .kind = range ? CUI_SLIDER_MODEL_RANGE : CUI_SLIDER_MODEL_SINGLE, .start = .25, .end = .75};
    cui_slider_model_options_v1 o = {.version = 1,
                                     .struct_size = sizeof o,
                                     .orientation = vertical ? CUI_SLIDER_MODEL_VERTICAL
                                                             : CUI_SLIDER_MODEL_HORIZONTAL,
                                     .flags = (variant & 4) ? CUI_SLIDER_MODEL_REVERSE_FILL_BIT : 0,
                                     .domain = {0, 1, .125, CUI_SLIDER_MODEL_LINEAR, 0},
                                     .name = CUI_STR("Selection"),
                                     .start_name = CUI_STR("Start"),
                                     .end_name = CUI_STR("End")};
    cui_base_slider_model_layout_v1 l = {
        .version = 1,
        .struct_size = sizeof l,
        .length = 200,
        .cross_extent = 24,
        .track_extent = 6,
        .thumb_extent = 16,
        .root = {.background = {20, 3, 5, 255}},
        .track = {.background = {40, 5, 7, 255}, .radius = 3},
        .fill = {.background = {60, 7, 9, 255}, .radius = 2},
        .start_thumb = {.background = {80, 9, 11, 255}, .radius = 8},
        .end_thumb = {.background = {100, 11, 13, 255}, .radius = 8}};
    for (unsigned frame = 0; frame < 6; ++frame) {
        if (frame == 1) {
            cui_focus(ui, cui_base_slider_model_part_id_v1(100, CUI_SLIDER_MODEL_END_THUMB));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
        }
        if (frame == 2) {
            queue(ui, CUI_EVENT_POINTER_DOWN, vertical, 80);
            queue(ui, CUI_EVENT_POINTER_MOVE, vertical, 150);
            queue(ui, CUI_EVENT_POINTER_UP, vertical, 100);
        }
        if (frame == 3) {
            o.flags |= CUI_SLIDER_MODEL_DISABLED_BIT;
        }
        if (frame == 4) {
            o.flags &= ~CUI_SLIDER_MODEL_DISABLED_BIT;
            o.flags |= CUI_SLIDER_MODEL_READONLY_BIT;
        }
        assert(cui_begin_frame(ui, 400, 400, .016f));
        cui_slider_model_result_v1 result;
#ifdef CUI_SLIDER_TRACE_RECORD
        (void)internal;
        result = cui_base_slider_model_events_v1(ui, 100, v, &s, &o, &l,
                                                 (cui_slider_model_handler_v1){callback, &s});
#else
        if (internal)
            result = cui_base_slider_model_present_internal(
                ui, 100, v, &s, &o, &l, (cui_slider_model_handler_v1){callback, &s}, NULL);
        else
            result = cui_base_slider_model_events_v1(ui, 100, v, &s, &o, &l,
                                                     (cui_slider_model_handler_v1){callback, &s});
#endif
        cui_draw_list list = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        u(result.status);
        u(result.flags);
        u(result.submission_frame);
        u(result.begin_count);
        u(result.request_count);
        u(result.commit_count);
        u(result.cancel_count);
        u(s.flags);
        u(s.owner);
        u(s.captured_target);
        u(s.active_thumb);
        u(s.last_painted_frame);
        u(s.request_sequence);
        value(s.start);
        value(s.preview);
        u(cui_focused(ui));
        u(list.count);
        for (size_t i = 0; i < list.count; ++i) {
            const cui_draw_command *c = &list.commands[i];
            u(c->type);
            rect(c->bounds);
            u(c->color.r);
            u(c->color.g);
            u(c->color.b);
            u(c->color.a);
            f(c->radius);
            f(c->stroke);
            u(c->custom_corners);
            u(c->custom_border_edges);
            f(c->corner_radii.top_left);
            f(c->corner_radii.top_right);
            f(c->corner_radii.bottom_left);
            f(c->corner_radii.bottom_right);
            f(c->border_edges.left);
            f(c->border_edges.right);
            f(c->border_edges.top);
            f(c->border_edges.bottom);
            assert(!c->text.length && !c->bitmap.bitmap.pixels && !c->mesh.vertex_count);
        }
        size_t count;
        const cui_semantic *sem = cui_semantics(ui, &count);
        u(count);
        for (size_t i = 0; i < count; ++i) {
            u(sem[i].id);
            u(sem[i].role);
            rect(sem[i].bounds);
            u(sem[i].disabled);
            u(sem[i].focused);
            f(sem[i].value);
            u(sem[i].details.parent);
            f(sem[i].details.minimum);
            f(sem[i].details.maximum);
            u(sem[i].details.readonly);
            u(sem[i].details.focusable);
            u(sem[i].details.touch_drag_priority);
            for (size_t j = 0; j < sem[i].name.length; ++j)
                u((unsigned char)sem[i].name.data[j]);
        }
        for (unsigned i = 0; i < 5; ++i)
            rect(cui_bounds(ui, cui_base_slider_model_part_id_v1(100, i)));
    }
    cui_destroy(ui);
    return trace;
}
#ifndef CUI_SLIDER_TRACE_RECORD
typedef struct {
    unsigned callbacks, composed;
    double expected;
    cui_slider_model_value_v1 external;
} hook_record;
static void hook_request(void *user, const cui_slider_model_request_v1 *r) {
    hook_record *record = user;
    assert(!record->composed && r->current.end == .75);
    const unsigned phases[11] = {0, 1, 2, 0, 1, 2, 0, 1, 1, 1, 2};
    const double proposals[11] = {.75, .625, .625, .625, .5, .5, .75, .75, .5, .5, .5};
    assert(record->callbacks < 11 && r->phase == phases[record->callbacks] &&
           r->requested.end == proposals[record->callbacks]);
    ++record->callbacks;
    record->external = r->requested; /* legal acceptance, never reread for paint */
}
static void hook_compose(cui_context *ui, const cui_id ids[5], cui_slider_model_value_v1 paint,
                         const cui_slider_model_options_v1 *o,
                         const cui_base_slider_model_layout_v1 *l, void *user) {
    (void)o;
    (void)l;
    hook_record *record = user;
    assert(!record->composed && paint.end == record->expected);
    ++record->composed;
    cui_begin_absolute_box(ui, ids[CUI_SLIDER_MODEL_TRACK], (cui_box){0}, (cui_rect){0, 9, 200, 6});
    cui_end_box(ui);
    cui_begin_absolute_box(ui, ids[CUI_SLIDER_MODEL_END_THUMB], (cui_box){0},
                           (cui_rect){(float)paint.end * 200 - 8, 4, 16, 16});
    cui_begin_box(ui, 1234,
                  (cui_box){.width = 14,
                            .height = 14,
                            .fixed_width = true,
                            .fixed_height = true,
                            .background = {10, 20, 30, 255}});
    cui_end_box(ui);
    cui_end_box(ui);
}
static void hook_order(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 64, .max_commands = 128});
    assert(ui);
    cui_slider_model_state_v1 state;
    assert(cui_slider_model_state_init_v1(sizeof state, &state));
    cui_slider_model_options_v1 o = {
        .version = 1, .struct_size = sizeof o, .domain = {0, 1, .125, CUI_SLIDER_MODEL_LINEAR, 0}};
    cui_base_slider_model_layout_v1 l = {.version = 1,
                                         .struct_size = sizeof l,
                                         .length = 200,
                                         .cross_extent = 24,
                                         .thumb_extent = 16,
                                         .track_extent = 6};
    hook_record record = {0};
    cui_slider_model_presentation_internal p = {
        .root = {.width = 200, .height = 24, .fixed_width = true, .fixed_height = true},
        .compose = hook_compose,
        .user = &record};
    for (unsigned frame = 0; frame < 5; ++frame) {
        if (frame == 1) {
            cui_focus(ui, cui_base_slider_model_part_id_v1(100, CUI_SLIDER_MODEL_END_THUMB));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
            assert(
                cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_LEFT}));
        }
        if (frame == 2)
            queue(ui, CUI_EVENT_POINTER_DOWN, false, 150);
        if (frame == 3)
            queue(ui, CUI_EVENT_POINTER_MOVE, false, 100);
        if (frame == 4)
            queue(ui, CUI_EVENT_POINTER_UP, false, 100);
        record.composed = 0;
        record.expected = frame == 3 ? .5 : .75;
        assert(cui_begin_frame(ui, 400, 100, 0));
        cui_slider_model_result_v1 result = cui_base_slider_model_present_internal(
            ui, 100, (cui_slider_model_value_v1){.kind = CUI_SLIDER_MODEL_SINGLE, .end = .75},
            &state, &o, &l, (cui_slider_model_handler_v1){hook_request, &record}, &p);
        assert(result.status == CUI_SLIDER_MODEL_OK && record.composed == 1);
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        cui_rect child = cui_bounds(ui, 1234),
                 thumb = cui_bounds(
                     ui, cui_base_slider_model_part_id_v1(100, CUI_SLIDER_MODEL_END_THUMB));
        assert(child.x == thumb.x && child.y == thumb.y && child.w == 14 && child.h == 14);
    }
    assert(record.callbacks == 11 && record.external.end == .5);
    cui_destroy(ui);
}
#endif
int main(void) {
#ifndef CUI_SLIDER_TRACE_RECORD
    hook_order();
#endif
    for (unsigned i = 0; i < 8; ++i) {
        uint64_t actual = run(i, false);
#ifdef CUI_SLIDER_TRACE_RECORD
        printf("UINT64_C(0x%016" PRIx64 "),\n", actual);
#else
        assert(actual == slider_null_trace[i]);
        assert(run(i, true) == slider_null_trace[i]);
#endif
    }
    return 0;
}
