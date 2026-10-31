/* SPDX-License-Identifier: MIT */
#ifdef CUI_SLIDER_UI_DISTRIBUTION
#define CUI_IMPLEMENTATION
#define CUI_BASE_SLIDER_MODEL_IMPLEMENTATION
#ifdef CUI_SLIDER_UI_UMBRELLA
#include "cui_all.h"
#else
#include "cui_base_slider_model.h"
#endif
#include "cui_draw_v2.h"
#else
#include "cui/cui_base_slider_model.h"
#include "cui/cui_draw_v2.h"
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define OWNER UINT64_C(100)
typedef struct {
    cui_slider_model_request_v1 events[96];
    size_t count;
    cui_slider_model_value_v1 owner;
    unsigned accept; /* 0 reject, 1 accept every update, 2 mutate unrelated external owner. */
    bool invalidate;
    cui_context *ui;
} recorder;
static void record(void *p, const cui_slider_model_request_v1 *e) {
    recorder *r = p;
    assert(r->count < 96);
    r->events[r->count++] = *e;
    if (e->phase == CUI_SLIDER_MODEL_UPDATE && r->accept == 1)
        r->owner = e->requested;
    if (e->phase == CUI_SLIDER_MODEL_UPDATE && r->accept == 2)
        r->owner.end = .9; /* Legal external state mutation; never reread by reducer. */
    if (r->invalidate) {
        cui_report_diagnostic(r->ui, &(cui_diagnostic){.error = CUI_ERROR_LAYOUT});
        cui_clear_error(r->ui);
    }
}
static cui_slider_model_options_v1 options(void) {
    return (cui_slider_model_options_v1){.version = 1,
                                         .struct_size = sizeof(cui_slider_model_options_v1),
                                         .domain = {0, 1, .125, CUI_SLIDER_MODEL_LINEAR, 0},
                                         .name = cui_string("Position"),
                                         .start_name = cui_string("Start"),
                                         .end_name = cui_string("End")};
}
static cui_base_slider_model_layout_v1 layout(void) {
    return (cui_base_slider_model_layout_v1){.version = 1,
                                             .struct_size = sizeof(cui_base_slider_model_layout_v1),
                                             .length = 200,
                                             .cross_extent = 24,
                                             .track_extent = 6,
                                             .thumb_extent = 16,
                                             .root = {.background = {20, 0, 0, 255}},
                                             .track = {.background = {40, 0, 0, 255}},
                                             .fill = {.background = {60, 0, 0, 255}},
                                             .start_thumb = {.background = {80, 0, 0, 255}},
                                             .end_thumb = {.background = {100, 0, 0, 255}}};
}
static cui_context *context(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 64, .max_commands = 128});
    assert(ui);
    return ui;
}
static cui_slider_model_state_v1 state_init(void) {
    cui_slider_model_state_v1 s;
    assert(cui_slider_model_state_init_v1(sizeof s, &s));
    return s;
}
static cui_id part(uint32_t p) {
    return cui_base_slider_model_part_id_v1(OWNER, p);
}
static cui_slider_model_result_v1 submit(cui_context *ui, cui_slider_model_state_v1 *s,
                                         cui_slider_model_options_v1 o,
                                         cui_base_slider_model_layout_v1 l, recorder *r,
                                         bool handler) {
    r->ui = ui;
    return cui_base_slider_model_events_v1(
        ui, OWNER, r->owner, s, &o, &l, (cui_slider_model_handler_v1){handler ? record : NULL, r});
}
static cui_slider_model_result_v1 scene(cui_context *ui, cui_slider_model_state_v1 *s,
                                        cui_slider_model_options_v1 o,
                                        cui_base_slider_model_layout_v1 l, recorder *r,
                                        bool handler) {
    assert(cui_begin_frame(ui, 400, 400, .016f));
    cui_slider_model_result_v1 out = submit(ui, s, o, l, r, handler);
    assert(out.status == CUI_SLIDER_MODEL_OK);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    return out;
}
static void pointer(cui_context *ui, cui_event_type type, float x, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = type,
                                           .x = x,
                                           .y = y,
                                           .pointer_type = CUI_POINTER_TOUCH,
                                           .pointer_id = 7,
                                           .pointer_device = 9}));
}
static void key(cui_context *ui, cui_key k) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k}));
}
static void set(cui_context *ui, cui_id target, double value) {
    cui_accessibility_set_value_v1 v = {1, 0, value};
    assert(cui_queue_accessibility_set_value_v1(ui, target, sizeof v, &v));
}
static void rect(cui_rect r, float x, float y, float w, float h) {
    assert(fabsf(r.x - x) < .002f && fabsf(r.y - y) < .002f && fabsf(r.w - w) < .002f &&
           fabsf(r.h - h) < .002f);
}
static const cui_semantic *semantic(cui_context *ui, cui_id id) {
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; ++i)
        if (s[i].id == id)
            return &s[i];
    return NULL;
}
static void painted(cui_draw_list d, unsigned red, float x, float y, float w, float h) {
    unsigned found = 0;
    for (size_t i = 0; i < d.count; ++i)
        if (d.commands[i].type == CUI_DRAW_RECT && d.commands[i].color.r == red &&
            d.commands[i].color.a == 255) {
            rect(d.commands[i].bounds, x, y, w, h);
            ++found;
        }
    assert(found == 1);
}
static void geometry(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {CUI_SLIDER_MODEL_SINGLE, 0, NAN, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    for (unsigned mode = 0; mode < 4; ++mode) {
        r.owner.kind = mode ? CUI_SLIDER_MODEL_RANGE : CUI_SLIDER_MODEL_SINGLE;
        r.owner.start = .25;
        r.owner.end = mode ? .75 : .25;
        o.orientation = mode == 2 ? CUI_SLIDER_MODEL_VERTICAL : CUI_SLIDER_MODEL_HORIZONTAL;
        o.flags = mode == 3 ? CUI_SLIDER_MODEL_REVERSE_FILL_BIT : 0;
        assert(cui_begin_frame(ui, 400, 400, 0));
        assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
        cui_draw_list d = cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
        if (!mode) {
            painted(d, 40, 0, 9, 200, 6);
            painted(d, 60, 0, 9, 50, 6);
            painted(d, 100, 42, 4, 16, 16);
            assert(!semantic(ui, part(CUI_SLIDER_MODEL_START_THUMB)));
        } else if (mode == 2) {
            painted(d, 40, 9, 0, 6, 200);
            painted(d, 60, 9, 50, 6, 100);
            painted(d, 80, 4, 142, 16, 16);
            painted(d, 100, 4, 42, 16, 16);
        } else {
            painted(d, 60, 50, 9, 100, 6);
            painted(d, 80, 42, 4, 16, 16);
            painted(d, 100, 142, 4, 16, 16);
        }
        assert(semantic(ui, part(CUI_SLIDER_MODEL_END_THUMB))->role == CUI_ROLE_SLIDER);
        assert(semantic(ui, OWNER)->role == CUI_ROLE_GROUP);
        assert(!cui_accessibility_action_supported(ui, OWNER, CUI_ACCESSIBILITY_ACTIVATE));
        assert(cui_accessibility_action_supported(ui, part(CUI_SLIDER_MODEL_END_THUMB),
                                                  CUI_ACCESSIBILITY_SET_VALUE));
    }
    r.owner = (cui_slider_model_value_v1){0, 0, 0, .25};
    o.flags = CUI_SLIDER_MODEL_REVERSE_FILL_BIT;
    assert(cui_begin_frame(ui, 400, 400, 0));
    submit(ui, &s, o, l, &r, true);
    painted(cui_end_frame(ui), 60, 50, 9, 150, 6);
    /* Pure IDs do not assert presence, and every owner/salt fallback is nonzero. */
    assert(!cui_base_slider_model_part_id_v1(0, 0));
    assert(!cui_base_slider_model_part_id_v1(OWNER, 5));
    assert(part(CUI_SLIDER_MODEL_ROOT) == OWNER && part(CUI_SLIDER_MODEL_START_THUMB));
    r.owner = (cui_slider_model_value_v1){CUI_SLIDER_MODEL_RANGE, 0, .25, .75};
    o.flags = 0;
    assert(cui_begin_frame(ui, 400, 400, 0));
    submit(ui, &s, o, l, &r, true);
    cui_box_shadow shadow = {.layer = {.offset_x = 2, .offset_y = 3, .color = {10, 20, 30, 255}}};
    for (uint32_t p = 0; p < 5; ++p)
        assert(cui_element_set_box_shadows(ui, part(p), &shadow, 1, (cui_corner_radii){0}));
    cui_draw_list d = cui_end_frame(ui);
    unsigned shadows = 0;
    for (size_t i = 0; i < d.count; ++i)
        shadows += d.commands[i].type == CUI_DRAW_DROP_SHADOW;
    assert(shadows == 5 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
}
static void evidence(const cui_slider_model_request_v1 *e, uint32_t source, size_t index) {
    assert(e->event.source == source && e->event.event_index == index && e->submission_frame > 0);
    assert(e->input.flags & CUI_SLIDER_MODEL_HAS_INPUT);
    assert(!(e->event.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION));
    assert(e->version == 1 && e->struct_size == sizeof(*e));
}
static void keyboard(void) {
    for (unsigned accept = 0; accept < 3; ++accept) {
        cui_context *ui = context();
        cui_slider_model_state_v1 s = state_init();
        recorder r = {.owner = {0, 0, 0, .25}, .accept = accept};
        cui_slider_model_options_v1 o = options();
        cui_base_slider_model_layout_v1 l = layout();
        scene(ui, &s, o, l, &r, true);
        cui_focus(ui, part(CUI_SLIDER_MODEL_END_THUMB));
        key(ui, CUI_KEY_RIGHT);
        key(ui, CUI_KEY_RIGHT);
        cui_slider_model_result_v1 out = scene(ui, &s, o, l, &r, true);
        assert(out.begin_count == 2 && out.request_count == 2 && out.commit_count == 2 &&
               r.count == 6);
        assert(r.events[1].requested.end == .375 && r.events[4].requested.end == .5);
        for (size_t i = 0; i < 6; ++i) {
            assert(r.events[i].current.end == .25 && r.events[i].current.start == 0);
            evidence(&r.events[i], CUI_COMPONENT_EVENT_SOURCE_KEYBOARD, i / 3);
        }
        assert(r.owner.end == (accept == 1 ? .5 : accept == 2 ? .9 : .25));
        assert(semantic(ui, part(CUI_SLIDER_MODEL_END_THUMB))->value == .25f);
        rect(cui_bounds(ui, part(CUI_SLIDER_MODEL_END_THUMB)), 42, 4, 16,
             16); /* Owner entry paint. */
        cui_destroy(ui);
    }
}
static void pointer_order(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {0, 0, 0, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    scene(ui, &s, o, l, &r, true);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
    pointer(ui, CUI_EVENT_POINTER_MOVE, 150, 12);
    pointer(ui, CUI_EVENT_POINTER_UP, 100, 12);
    cui_slider_model_result_v1 out = scene(ui, &s, o, l, &r, true);
    assert(r.count == 5 && out.request_count == 3 && out.commit_count == 1 && !out.cancel_count);
    assert(!(s.flags & CUI_SLIDER_MODEL_DRAGGING) && r.owner.end == .25 && s.preview.end == .5);
    const uint32_t phases[] = {0, 1, 1, 1, 2};
    const size_t indices[] = {0, 0, 1, 2, 2};
    for (size_t i = 0; i < 5; ++i) {
        assert(r.events[i].phase == phases[i] && r.events[i].current.end == .25);
        evidence(&r.events[i], CUI_COMPONENT_EVENT_SOURCE_POINTER, indices[i]);
        assert(r.events[i].input.pointer_id == 7 && r.events[i].event.target == OWNER);
    }
    assert(cui_focused(ui) == part(CUI_SLIDER_MODEL_END_THUMB));
    assert(cui_interaction_state(ui, part(CUI_SLIDER_MODEL_END_THUMB)).focus_visible);
    /* Old Range START is pruned by a completed Single frame, including its
     * nonsemantic Clay fallback; do not infer semantic presence from bounds. */
    r.owner = (cui_slider_model_value_v1){CUI_SLIDER_MODEL_RANGE, 0, .125, .75};
    scene(ui, &s, o, l, &r, true);
    r.owner = (cui_slider_model_value_v1){0, 0, 0, .25};
    scene(ui, &s, o, l, &r, true);
    rect(cui_bounds(ui, part(CUI_SLIDER_MODEL_START_THUMB)), 0, 0, 0, 0);
    /* New Range START has no previous semantic: root still selects/focuses it. */
    r.count = 0;
    r.owner = (cui_slider_model_value_v1){CUI_SLIDER_MODEL_RANGE, 0, .125, .75};
    pointer(ui, CUI_EVENT_POINTER_DOWN, 10, 12);
    key(ui, CUI_KEY_RIGHT);
    pointer(ui, CUI_EVENT_POINTER_UP, 10, 12);
    out = scene(ui, &s, o, l, &r, true);
    assert(out.begin_count == 1 && out.commit_count == 1 && !out.cancel_count);
    assert(cui_focused(ui) == part(CUI_SLIDER_MODEL_START_THUMB));
    assert(r.events[0].thumb == CUI_SLIDER_MODEL_START && r.events[0].event.target == OWNER);
    /* Root keyboard event dispatched in the same batch is busy, not replayed. */
    assert(out.request_count == 2);
    /* Idle ROOT-target key delegates to active endpoint and preserves raw target. */
    cui_focus(ui, OWNER);
    r.count = 0;
    key(ui, CUI_KEY_RIGHT);
    out = scene(ui, &s, o, l, &r, true);
    assert(out.request_count == 1 && r.events[1].thumb == CUI_SLIDER_MODEL_START);
    assert(r.events[1].event.target == OWNER && r.events[1].requested.start == .25);
    cui_destroy(ui);
}
static void numeric_order(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {0, 0, 0, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    scene(ui, &s, o, l, &r, true);
    /* Real prior idle capability permits enqueue, then local drag rejects once. */
    pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), .875);
    pointer(ui, CUI_EVENT_POINTER_UP, 100, 12);
    assert(cui_begin_frame(ui, 400, 400, 0));
    cui_slider_model_result_v1 out = submit(ui, &s, o, l, &r, true);
    assert(out.status == CUI_SLIDER_MODEL_OK && out.request_count == 2 && out.commit_count == 1);
    assert(cui_event_consumed(ui, 1));
    cui_accessibility_numeric_input_v1 sentinel, copy;
    memset(&sentinel, 0xa5, sizeof sentinel);
    copy = sentinel;
    assert(!cui_claim_accessibility_numeric_input_v1(ui, part(CUI_SLIDER_MODEL_END_THUMB), 1,
                                                     sizeof sentinel, &sentinel));
    assert(!memcmp(&sentinel, &copy, sizeof copy));
    (void)cui_end_frame(ui);
    assert(cui_accessibility_action_supported(ui, part(CUI_SLIDER_MODEL_END_THUMB),
                                              CUI_ACCESSIBILITY_SET_VALUE));
    r.count = 0;
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), .625);
    key(ui, CUI_KEY_RIGHT);
    out = scene(ui, &s, o, l, &r, true);
    assert(out.request_count == 2 && r.events[1].requested.end == .625 &&
           r.events[4].requested.end == .75);
    evidence(&r.events[1], CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY, 0);
    assert(r.events[1].input.accessibility_value == .625);
    /* A published dragging snapshot has cap0. Queue rejection is atomic, not a
     * fabricated prior-capability fixture for otherwise unreachable events. */
    pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
    r.count = 0;
    scene(ui, &s, o, l, &r, true);
    assert(s.flags & CUI_SLIDER_MODEL_DRAGGING);
    cui_accessibility_set_value_v1 v = {1, 0, .5};
    assert(
        !cui_queue_accessibility_set_value_v1(ui, part(CUI_SLIDER_MODEL_END_THUMB), sizeof v, &v));
    assert(cui_last_error(ui) == CUI_ERROR_INVALID_ACTION);
    cui_clear_error(ui); /* Outside-frame rejection retains valid prior snapshot. */
    assert(!cui_accessibility_action_supported(ui, part(CUI_SLIDER_MODEL_END_THUMB),
                                               CUI_ACCESSIBILITY_SET_VALUE));
    /* Changed domain invalidates captured start: CANCEL keeps old payload, next
     * key uses current confirmed. SET becomes reachable only next idle frame. */
    o.domain = (cui_slider_model_domain_v1){10, 20, 1, 0, 0};
    r.owner.end = 12;
    r.count = 0;
    key(ui, CUI_KEY_RIGHT);
    out = scene(ui, &s, o, l, &r, true);
    assert(out.cancel_count == 1 && out.request_count == 1 && r.count == 4);
    assert(r.events[0].reason == CUI_SLIDER_MODEL_DOMAIN_CHANGED &&
           r.events[0].requested.end == .25);
    assert(r.events[0].gesture_domain.maximum == 1 && r.events[0].domain.minimum == 10);
    assert(r.events[2].current.end == 12 && r.events[2].requested.end == 13);
    r.count = 0;
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), .5);
    key(ui, CUI_KEY_RIGHT);
    out = scene(ui, &s, o, l, &r, true);
    assert(out.request_count == 2 && r.events[1].requested.end == 15 &&
           r.events[4].requested.end == 16);
    pointer(ui, CUI_EVENT_POINTER_UP, 80, 12);
    scene(ui, &s, o, l, &r, true);
    cui_destroy(ui);
}
static void cancellation_intent(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {CUI_SLIDER_MODEL_RANGE, 0, .125, .75}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    scene(ui, &s, o, l, &r, true);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 150, 12);
    scene(ui, &s, o, l, &r, true);
    r.count = 0;
    r.owner = (cui_slider_model_value_v1){0, 0, NAN, .25};
    key(ui, CUI_KEY_RIGHT);
    cui_slider_model_result_v1 out = scene(ui, &s, o, l, &r, true);
    assert(out.cancel_count == 1 && out.request_count == 1 && r.count == 4);
    assert(r.events[0].reason == CUI_SLIDER_MODEL_MODE_CHANGED &&
           r.events[0].requested.kind == CUI_SLIDER_MODEL_RANGE &&
           r.events[0].requested.start == .125);
    assert(r.events[2].current.kind == CUI_SLIDER_MODEL_SINGLE &&
           r.events[2].requested.end == .375);
    r.count = 0;
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), .5);
    key(ui, CUI_KEY_RIGHT);
    assert(scene(ui, &s, o, l, &r, true).request_count == 2);
    assert(r.events[1].requested.end == .5 && r.events[4].requested.end == .625);
    pointer(ui, CUI_EVENT_POINTER_UP, 150, 12);
    scene(ui, &s, o, l, &r, true);
    /* Source generation only: captured pair is valid, keep its trajectory. */
    pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
    scene(ui, &s, o, l, &r, true);
    r.count = 0;
    o.source_generation = 1;
    r.owner.end = .75;
    key(ui, CUI_KEY_RIGHT);
    out = scene(ui, &s, o, l, &r, true);
    assert(out.cancel_count == 1 && r.events[0].reason == CUI_SLIDER_MODEL_SOURCE_REPLACED);
    assert(r.events[0].requested.end == .25 && r.events[2].requested.end == .375 &&
           r.events[2].current.end == .75);
    pointer(ui, CUI_EVENT_POINTER_UP, 80, 12);
    scene(ui, &s, o, l, &r, true);
    /* INPUT cancel+SET can genuinely queue from previous idle capabilities. */
    r.owner.end = .25;
    scene(ui, &s, o, l, &r, true);
    r.count = 0;
    pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
    pointer(ui, CUI_EVENT_CANCEL, 80, 12);
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), .625);
    out = scene(ui, &s, o, l, &r, true);
    assert(out.cancel_count == 1 && out.request_count == 2 && out.commit_count == 1);
    assert(r.events[2].phase == CUI_SLIDER_MODEL_CANCEL &&
           r.events[2].reason == CUI_SLIDER_MODEL_INPUT);
    assert(r.events[4].requested.end == .625 && r.events[4].input.accessibility_value == .625);
    cui_destroy(ui);
}
static void axes_and_limits(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {CUI_SLIDER_MODEL_RANGE, 0, .25, .75}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    o.orientation = CUI_SLIDER_MODEL_VERTICAL;
    scene(ui, &s, o, l, &r, true);
    cui_focus(ui, part(CUI_SLIDER_MODEL_START_THUMB));
    key(ui, CUI_KEY_END);
    scene(ui, &s, o, l, &r, true);
    assert(r.events[1].requested.start == .75 && r.events[1].requested.end == .75);
    r.count = 0;
    pointer(ui, CUI_EVENT_POINTER_DOWN, 12, 100);
    scene(ui, &s, o, l, &r, true);
    assert(s.active_thumb == CUI_SLIDER_MODEL_END &&
           s.preview.end == .5); /* Exact midpoint -> END. */
    pointer(ui, CUI_EVENT_POINTER_UP, 12, 0);
    scene(ui, &s, o, l, &r, true);
    o.domain = (cui_slider_model_domain_v1){1, 10000, 0, CUI_SLIDER_MODEL_LOGARITHMIC, 0};
    r.owner = (cui_slider_model_value_v1){0, 0, 0, 100};
    o.orientation = 0;
    scene(ui, &s, o, l, &r, true);
    r.count = 0;
    pointer(ui, CUI_EVENT_POINTER_DOWN, 100, 12);
    pointer(ui, CUI_EVENT_POINTER_UP, 100, 12);
    scene(ui, &s, o, l, &r, true);
    assert(fabs(r.events[1].requested.end - 100) < 1e-10);
    /* Semantic SET is domain-normalized, independent of logarithmic geometry. */
    r.count = 0;
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), .5);
    scene(ui, &s, o, l, &r, true);
    assert(r.events[1].requested.end == 5000.5);
    cui_destroy(ui);
}
static void precise_numeric_and_ime(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {0, 0, 0, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    o.domain.step = 0;
    scene(ui, &s, o, l, &r, true);
    double raw = .123456789012345;
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), raw);
    scene(ui, &s, o, l, &r, true);
    assert(r.events[1].input.accessibility_value == raw && r.events[1].requested.end == raw);
    r.count = 0;
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), -2);
    scene(ui, &s, o, l, &r, true);
    assert(r.events[1].requested.end == 0 && r.events[1].input.accessibility_value == -2);
    r.count = 0;
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), 2);
    scene(ui, &s, o, l, &r, true);
    assert(r.events[1].requested.end == 1 && r.events[1].input.accessibility_value == 2);
    cui_focus(ui, part(CUI_SLIDER_MODEL_END_THUMB));
    r.count = 0;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT, .ime_composing = true}));
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                           .key = CUI_KEY_RIGHT,
                                           .prefer_character_input = true}));
    assert(cui_begin_frame(ui, 400, 400, 0));
    assert(!submit(ui, &s, o, l, &r, true).request_count && !r.count);
    assert(!cui_event_consumed(ui, 0) && !cui_event_consumed(ui, 1));
    (void)cui_end_frame(ui);
    o.domain.step = .125;
    assert(cui_queue_event(
        ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_RIGHT, .repeat = true}));
    scene(ui, &s, o, l, &r, true);
    assert(r.count == 3 && r.events[1].input.repeat && r.events[1].requested.end == .375);
    cui_destroy(ui);
}
static void blocking_and_recovery(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {0, 0, 0, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    scene(ui, &s, o, l, &r, true);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
    scene(ui, &s, o, l, &r, true);
    assert(s.flags);
    r.count = 0;
    o.flags = CUI_SLIDER_MODEL_DISABLED_BIT;
    assert(scene(ui, &s, o, l, &r, true).cancel_count == 1 &&
           r.events[0].reason == CUI_SLIDER_MODEL_DISABLED);
    assert(semantic(ui, part(CUI_SLIDER_MODEL_END_THUMB))->disabled);
    pointer(ui, CUI_EVENT_POINTER_UP, 80, 12);
    scene(ui, &s, o, l, &r, true);
    o.flags = 0;
    r.count = 0;
    scene(ui, &s, o, l, &r, false);
    assert(semantic(ui, part(CUI_SLIDER_MODEL_END_THUMB))->details.readonly);
    assert(!cui_accessibility_action_supported(ui, part(CUI_SLIDER_MODEL_END_THUMB),
                                               CUI_ACCESSIBILITY_SET_VALUE));
    scene(ui, &s, o, l, &r, true);
    cui_slider_model_state_v1 before = s;
    for (unsigned layout_only = 0; layout_only < 2; ++layout_only) {
        assert(cui_begin_frame(ui, 400, 400, 0));
        cui_begin_box(ui, 900,
                      (cui_box){.width = 200,
                                .height = 24,
                                .inert = !layout_only,
                                .layout_only = layout_only != 0});
        assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
        cui_end_box(ui);
        (void)cui_end_frame(ui);
        assert(!memcmp(&s, &before, sizeof s));
    }
    scene(ui, &s, o, l, &r, true);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
    scene(ui, &s, o, l, &r, true);
    r.count = 0;
    /* Diagnostic cleared inside failed frame does not admit or renew state. */
    before = s;
    assert(cui_begin_frame(ui, 400, 400, 0));
    cui_report_diagnostic(ui, &(cui_diagnostic){.error = CUI_ERROR_LAYOUT});
    cui_clear_error(ui);
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
    assert(!memcmp(&s, &before, sizeof s) && !r.count);
    (void)cui_end_frame(ui);
    cui_clear_error(ui);
    scene(ui, &s, o, l, &r, true);
    assert(!memcmp(&s, &before, sizeof s) && !r.count);
    assert(scene(ui, &s, o, l, &r, true).cancel_count == 1);
    assert(r.count == 1 && r.events[0].reason == CUI_SLIDER_MODEL_OMITTED);
    pointer(ui, CUI_EVENT_POINTER_UP, 80, 12);
    scene(ui, &s, o, l, &r, true);
    /* A callback-created diagnostic stops every later callback, even cleared. */
    r.count = 0;
    r.invalidate = true;
    cui_focus(ui, part(CUI_SLIDER_MODEL_END_THUMB));
    key(ui, CUI_KEY_RIGHT);
    key(ui, CUI_KEY_RIGHT);
    assert(cui_begin_frame(ui, 400, 400, 0));
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_CORE_FAILURE && r.count == 1);
    (void)cui_end_frame(ui);
    cui_clear_error(ui);
    cui_destroy(ui);
}
static void cancellation_policies(void) {
    for (unsigned policy = 0; policy < 4; ++policy) {
        cui_context *ui = context();
        cui_slider_model_state_v1 s = state_init();
        recorder r = {.owner = {0, 0, 0, .25}};
        cui_slider_model_options_v1 o = options();
        cui_base_slider_model_layout_v1 l = layout();
        scene(ui, &s, o, l, &r, true);
        pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
        scene(ui, &s, o, l, &r, true);
        r.count = 0;
        if (policy == 0)
            o.flags = CUI_SLIDER_MODEL_READONLY_BIT;
        if (policy == 1)
            o.domain.minimum = o.domain.maximum = .25;
        if (policy == 2)
            o.orientation = CUI_SLIDER_MODEL_VERTICAL;
        cui_slider_model_result_v1 out = scene(ui, &s, o, l, &r, policy != 3);
        assert(!s.flags);
        if (policy == 3)
            assert(!r.count && !out.cancel_count);
        else {
            const uint32_t reasons[] = {CUI_SLIDER_MODEL_READONLY, CUI_SLIDER_MODEL_ZERO_SPAN,
                                        CUI_SLIDER_MODEL_AXIS_CHANGED};
            assert(out.cancel_count == 1 && r.count == 1 && r.events[0].reason == reasons[policy]);
            assert(!r.events[0].input.flags && r.events[0].requested.end == .25);
        }
        cui_destroy(ui);
    }
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {0, 0, 0, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    for (unsigned frame = 0; frame < 3; ++frame) {
        if (frame == 1)
            pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
        if (frame == 2) {
            r.count = 0;
            key(ui, CUI_KEY_ESCAPE);
        }
        assert(cui_begin_frame(ui, 400, 400, 0));
        assert(cui_begin_focus_scope(ui, 778, false));
        cui_slider_model_result_v1 out = submit(ui, &s, o, l, &r, true);
        if (frame == 2) {
            assert(out.cancel_count == 1 && r.count == 1 && !s.flags);
            assert(r.events[0].reason == CUI_SLIDER_MODEL_CAPTURE_LOST && !r.events[0].input.flags);
            assert(cui_event_target(ui, 0) == 778 && !cui_event_consumed(ui, 0));
        }
        cui_end_focus_scope(ui);
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}
static void modal_and_foreign(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {0, 0, 0, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    scene(ui, &s, o, l, &r, true);
    /* A foreign numeric semantic may share the batch; our busy path must not
     * consume it. It keeps its own Core claim after our control. */
    assert(cui_begin_frame(ui, 400, 400, 0));
    submit(ui, &s, o, l, &r, true);
    cui_begin_box(ui, 777, (cui_box){.width = 20, .height = 20});
    (void)cui_interact(ui, 777, CUI_ROLE_SLIDER, cui_string("Foreign"), false, false, .5);
    assert(cui_set_accessibility_numeric_actions(ui, 777, 7));
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
    set(ui, 777, .5);
    assert(cui_begin_frame(ui, 400, 400, 0));
    submit(ui, &s, o, l, &r, true);
    assert(!cui_event_consumed(ui, 1));
    cui_begin_box(ui, 777, (cui_box){.width = 20, .height = 20});
    (void)cui_interact(ui, 777, CUI_ROLE_SLIDER, cui_string("Foreign"), false, false, .5);
    assert(cui_set_accessibility_numeric_actions(ui, 777, 7));
    cui_accessibility_numeric_input_v1 claimed;
    assert(cui_claim_accessibility_numeric_input_v1(ui, 777, 1, sizeof claimed, &claimed));
    assert(claimed.value == .5);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    pointer(ui, CUI_EVENT_POINTER_UP, 80, 12);
    scene(ui, &s, o, l, &r, true);
    /* Queue against valid prior idle caps; current readonly/disabled leaves
     * input untouched here rather than consuming through busy rejection. */
    for (unsigned flag = 1; flag <= 2; ++flag) {
        scene(ui, &s, o, l, &r, true);
        r.count = 0;
        set(ui, part(CUI_SLIDER_MODEL_END_THUMB), .75);
        o.flags = flag;
        assert(cui_begin_frame(ui, 400, 400, 0));
        assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
        assert(!cui_event_consumed(ui, 0) && !r.count);
        (void)cui_end_frame(ui);
        o.flags = 0;
    }
    scene(ui, &s, o, l, &r, true);
    set(ui, part(CUI_SLIDER_MODEL_END_THUMB), .75);
    cui_slider_model_state_v1 before = s;
    r.count = 0;
    assert(cui_begin_frame(ui, 400, 400, 0));
    assert(cui_begin_focus_scope(ui, 888, true));
    cui_end_focus_scope(ui); /* Newly current modal excludes outer composition. */
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
    assert(!cui_event_consumed(ui, 0) && !r.count && !memcmp(&before, &s, sizeof s));
    (void)cui_end_frame(ui);
    /* Previous modal also blocks outer composition until scope publication. */
    scene(ui, &s, o, l, &r, true);
    assert(!r.count && !memcmp(&before, &s, sizeof s));
    scene(ui, &s, o, l, &r, true);
    assert(s.last_painted_frame != before.last_painted_frame);
    cui_destroy(ui);
}
static void prior_axis_and_capture(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {0, 0, 0, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    scene(ui, &s, o, l, &r, true);
    /* Direct thumb capture retains Core pointer focus (not explicit root focus). */
    pointer(ui, CUI_EVENT_POINTER_DOWN, 50, 12);
    scene(ui, &s, o, l, &r, true);
    assert(s.captured_target == part(CUI_SLIDER_MODEL_END_THUMB));
    assert(!cui_interaction_state(ui, part(CUI_SLIDER_MODEL_END_THUMB)).focus_visible);
    r.count = 0;
    pointer(ui, CUI_EVENT_POINTER_MOVE, 100, 12);
    l.length = 300;
    l.axis_inset = 20;
    assert(cui_begin_frame(ui, 400, 400, 0));
    cui_begin_box(ui, 901, (cui_box){.width = 350, .height = 80, .padding = 10});
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
    cui_end_box(ui);
    cui_draw_list d = cui_end_frame(ui);
    assert(r.count == 1 && r.events[0].requested.end == .5); /* Prior axis 0..200. */
    painted(d, 100, 152, 14, 16, 16); /* Current parent10 + inset20 + .5*260 -8. */
    /* Secondary pointer does not become local owner or change captured identity. */
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE,
                                           .x = 290,
                                           .y = 12,
                                           .pointer_type = CUI_POINTER_PEN,
                                           .pointer_id = 99,
                                           .pointer_device = 4}));
    r.count = 0;
    /* Use identical parent placement so next frame still uses real prior axis. */
    assert(cui_begin_frame(ui, 400, 400, 0));
    cui_begin_box(ui, 901, (cui_box){.width = 350, .height = 80, .padding = 10});
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(!r.count && s.pointer_id == 7 && s.preview.end == .5);
    pointer(ui, CUI_EVENT_CANCEL, 100, 12);
    r.count = 0;
    assert(scene(ui, &s, o, l, &r, true).cancel_count == 1 && r.count == 1);
    assert(r.events[0].event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN &&
           r.events[0].input.type == CUI_EVENT_CANCEL);
    cui_destroy(ui);
}
static void v2_layout_failure(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {0, 0, 0, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    scene(ui, &s, o, l, &r, true);
    cui_frame_v2_options v = {
        .size = sizeof v,
        .support = {.size = sizeof(cui_draw_v2_support),
                    .abi_version = CUI_DRAW_ABI_V2,
                    .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                    .features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
                    .max_commands = 128,
                    .max_backgrounds = 1,
                    .max_pixels = 1024,
                    .max_pixels_per_background = 1024,
                    .cpu_bytes = 8192,
                    .texture_bytes = 8192,
                    .transfer_bytes = 8192,
                    .metadata_bytes = 8192},
        .required_features = CUI_DRAW_V2_FEATURE_BACKGROUNDS,
        .command_bytes = 128 * sizeof(cui_draw_command_v2),
        .background_bytes = 8192};
    pointer(ui, CUI_EVENT_POINTER_DOWN, 80, 12);
    assert(cui_begin_frame_v2(ui, 400, 400, 0, &v));
    cui_begin_box(ui, 500, (cui_box){.width = 240, .height = 40, .clip = true});
    cui_begin_box(ui, 501,
                  (cui_box){.width = 240, .height = 80, .fixed_width = true, .fixed_height = true});
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK && r.count == 2);
    cui_end_box(ui);
    cui_end_box(ui);
    float offset = 0;
    cui_scroll_clamp_v1 clamp = {sizeof clamp, 1, CUI_SCROLL_CLAMP_Y, 0, 500, 501};
    assert(cui_layout_scroll_clamp_v1(ui, &clamp, &offset));
    offset = NAN;
    cui_draw_list_v2 output;
    assert(!cui_end_frame_v2(ui, &output) && !output.count &&
           cui_last_error(ui) == CUI_ERROR_LAYOUT);
    assert(r.count == 2 && s.flags); /* Already delivered requests are not rolled back. */
    cui_clear_error(ui);
    r.count = 0;
    cui_slider_model_state_v1 before = s;
    assert(cui_begin_frame_v2(ui, 400, 400, 0, &v));
    assert(!cui_frame_input_snapshot_ready(ui));
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
    assert(!memcmp(&s, &before, sizeof s) && !r.count);
    assert(cui_end_frame_v2(ui, &output) && output.count);
    assert(cui_begin_frame_v2(ui, 400, 400, 0, &v));
    assert(submit(ui, &s, o, l, &r, true).cancel_count == 1 && r.count == 1);
    assert(r.events[0].reason == CUI_SLIDER_MODEL_OMITTED);
    assert(cui_end_frame_v2(ui, &output));
    cui_destroy(ui);
}
static void validation(void) {
    cui_context *ui = context();
    cui_slider_model_state_v1 s = state_init();
    recorder r = {.owner = {0, 0, NAN, .25}};
    cui_slider_model_options_v1 o = options();
    cui_base_slider_model_layout_v1 l = layout();
    l.start_thumb.radius = NAN; /* Inactive Single. */
    l.root.width = NAN;         /* All caller layout fields ignored. */
    scene(ui, &s, o, l, &r, true);
    assert(cui_begin_frame(ui, 400, 400, 0));
    cui_slider_model_state_v1 before = s;
    l.axis_inset = 100;
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_INVALID_ARGUMENT);
    assert(!memcmp(&s, &before, sizeof s));
    size_t n;
    (void)cui_semantics(ui, &n);
    assert(!n);
    l.axis_inset = 0;
    cui_slider_model_options_v1 good = o;
    for (unsigned invalid = 0; invalid < 5; ++invalid) {
        o = good;
        if (invalid == 0)
            o.version = 2;
        if (invalid == 1)
            --o.struct_size;
        if (invalid == 2)
            o.flags = 8;
        if (invalid == 3)
            o.orientation = 2;
        if (invalid == 4)
            o.end_name = (cui_str){.length = 1};
        assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_INVALID_ARGUMENT);
        assert(!memcmp(&s, &before, sizeof s));
        (void)cui_semantics(ui, &n);
        assert(!n);
    }
    o = good;
    s.request_sequence = UINT64_MAX;
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_SEQUENCE_EXHAUSTED);
    s = before;
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
    before = s;
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_INVALID_STATE);
    assert(!memcmp(&s, &before, sizeof s));
    (void)cui_end_frame(ui);
    cui_focus(ui, part(CUI_SLIDER_MODEL_END_THUMB));
    key(ui, CUI_KEY_RIGHT);
    s = state_init();
    r.count = 0;
    assert(cui_begin_frame(ui, 400, 400, 0));
    assert(submit(ui, &s, o, l, &r, true).status == CUI_SLIDER_MODEL_OK);
    assert(s.owner == OWNER && !r.count && !cui_event_consumed(ui, 0));
    (void)cui_end_frame(ui);
    cui_destroy(ui);
}
int main(void) {
    geometry();
    keyboard();
    pointer_order();
    numeric_order();
    cancellation_intent();
    axes_and_limits();
    precise_numeric_and_ime();
    blocking_and_recovery();
    cancellation_policies();
    modal_and_foreign();
    prior_axis_and_capture();
    v2_layout_failure();
    validation();
    printf("Owned state=%zu, request=%zu, input=%zu, result=%zu bytes\n",
           sizeof(cui_slider_model_state_v1), sizeof(cui_slider_model_request_v1),
           sizeof(cui_slider_model_input_v1), sizeof(cui_slider_model_result_v1));
    puts("PASS typed Base Slider geometry, ordering, explicit rejection, ranges and readiness");
    return 0;
}
