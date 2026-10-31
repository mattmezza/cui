/* SPDX-License-Identifier: MIT */
/* Independent frozen pre-extraction surface reducer versus current source. */
#include "../../src/base/cui_dialog_internal.h"
#include "cui/cui_surface.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
void legacy_cui_surface_init(cui_surface_state *, cui_id, cui_surface_kind);
void legacy_cui_surface_open(cui_context *, cui_surface_state *, cui_id);
void legacy_cui_surface_request_close(cui_context *, cui_surface_state *, cui_surface_result);
bool legacy_cui_base_surface_update(cui_context *, cui_surface_state *, cui_surface_options);
typedef struct {
    cui_context *ui;
    cui_surface_state state;
    unsigned decisions, closes;
    cui_id focus_on_close;
    cui_surface_result decision_reason, close_reason;
    bool veto, composed;
} driver;
static bool decide(void *user, cui_surface_result reason) {
    driver *d = user;
    ++d->decisions;
    d->decision_reason = reason;
    return !d->veto;
}
static void closed(void *user, cui_surface_result reason) {
    driver *d = user;
    ++d->closes;
    d->close_reason = reason;
    d->focus_on_close = cui_focused(d->ui);
    assert(!d->state.overlay.open);
}
static void button(driver *d, cui_id id, float x) {
    cui_begin_absolute_box(d->ui, id, (cui_box){0}, (cui_rect){x, 0, 30, 20});
    cui_interact(d->ui, id, CUI_ROLE_BUTTON, cui_string("Action"), false, false, 0);
    cui_end_box(d->ui);
}
static void frame(driver *d, bool legacy, unsigned index, cui_surface_options o) {
    assert(cui_begin_frame(d->ui, 400, 300, .016f));
    o.decide = decide;
    o.decision_user = d;
    o.on_close = closed;
    o.close_user = d;
    bool changed = legacy        ? legacy_cui_base_surface_update(d->ui, &d->state, o)
                   : d->composed ? cui_dialog_surface_update(d->ui, &d->state, o)
                                 : cui_base_surface_update(d->ui, &d->state, o);
    assert(changed == !d->state.overlay.open);
    cui_begin_box(d->ui, 1, (cui_box){.width = 20, .height = 20});
    cui_interact(d->ui, 1, CUI_ROLE_BUTTON, cui_string("Trigger"), false, false, 0);
    cui_end_box(d->ui);
    if (d->state.overlay.open) {
        assert(cui_overlay_begin(d->ui, &d->state.overlay));
        cui_begin_portal(d->ui, d->state.overlay.id, (cui_box){0}, (cui_rect){40, 40, 200, 180},
                         100);
        cui_track_bounds(d->ui, d->state.overlay.id, &d->state.overlay.bounds);
        cui_interact(d->ui, d->state.overlay.id, CUI_ROLE_DIALOG, cui_string("Dialog"), false,
                     false, 0);
        button(d, cui_surface_part_id(50, 1), 0);
        button(d, cui_surface_part_id(50, 2), 35);
        button(d, cui_surface_part_id(50, 3), 70);
        cui_begin_absolute_box(d->ui, cui_surface_part_id(50, 6),
                               (cui_box){.clip = true, .offset_y = -d->state.body_scroll},
                               (cui_rect){0, 30, 180, 100});
        cui_base_surface_submit_body(d->ui, &d->state);
        cui_begin_box(d->ui, cui_surface_part_id(50, 7),
                      (cui_box){.width = 180, .height = 500, .fixed_height = true});
        cui_end_box(d->ui);
        cui_end_box(d->ui);
        cui_end_box(d->ui);
        cui_overlay_end(d->ui);
    }
    cui_draw_list list = cui_end_frame(d->ui);
    assert(cui_last_error(d->ui) == CUI_OK);
    (void)list;
    (void)index;
}
static bool rect_equal(cui_rect a, cui_rect b) {
    return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h;
}
static bool state_equal(const cui_surface_state *a, const cui_surface_state *b) {
#define EQ(field) (a->field == b->field)
    return EQ(overlay.id) && EQ(overlay.trigger) &&
           rect_equal(a->overlay.bounds, b->overlay.bounds) && EQ(overlay.open) &&
           EQ(overlay.modal) && EQ(overlay.dismiss_on_escape) && EQ(overlay.dismiss_on_outside) &&
           EQ(overlay.has_bounds) && EQ(kind) && EQ(result) && EQ(sheet_size) &&
           EQ(resize_origin) && EQ(resize_size) && EQ(resizing) && EQ(pending_close) &&
           EQ(on_close) && EQ(body_scroll) && EQ(body_maximum) &&
           rect_equal(a->body_viewport, b->body_viewport) &&
           rect_equal(a->body_content, b->body_content) && EQ(sheet_progress) &&
           EQ(sheet_motion_from) && EQ(sheet_animation_seconds) && EQ(sheet_motion_start) &&
           EQ(sheet_motion_enabled) && EQ(sheet_closing) && EQ(sheet_close_result);
#undef EQ
}
static void comparator_oracle(void) {
    cui_surface_state a, b;
    unsigned char member[sizeof a] = {0};
    memset(&a, 0x55, sizeof a);
    memset(&b, 0xaa, sizeof b);
#define SET(field, value)                                                                          \
    do {                                                                                           \
        a.field = b.field = value;                                                                 \
        memset(member + offsetof(cui_surface_state, field), 1, sizeof a.field);                    \
    } while (0)
    SET(overlay.id, 50);
    SET(overlay.trigger, 1);
    SET(overlay.bounds.x, 40);
    SET(overlay.bounds.y, 40);
    SET(overlay.bounds.w, 200);
    SET(overlay.bounds.h, 180);
    SET(overlay.open, true);
    SET(overlay.modal, true);
    SET(overlay.dismiss_on_escape, true);
    SET(overlay.dismiss_on_outside, true);
    SET(overlay.has_bounds, true);
    SET(kind, CUI_SURFACE_DIALOG);
    SET(result, CUI_SURFACE_NONE);
    SET(sheet_size, 0);
    SET(resize_origin, 0);
    SET(resize_size, 0);
    SET(resizing, false);
    SET(pending_close, CUI_SURFACE_NONE);
    SET(on_close, closed);
    a.close_user = &a;
    b.close_user = &b; /* Deliberately independent userdata. */
    memset(member + offsetof(cui_surface_state, close_user), 1, sizeof a.close_user);
    SET(body_scroll, 0);
    SET(body_maximum, 0);
    SET(body_viewport.x, 0);
    SET(body_viewport.y, 0);
    SET(body_viewport.w, 100);
    SET(body_viewport.h, 100);
    SET(body_content.x, 0);
    SET(body_content.y, 0);
    SET(body_content.w, 100);
    SET(body_content.h, 200);
    SET(sheet_progress, 0);
    SET(sheet_motion_from, 0);
    SET(sheet_animation_seconds, 0);
    SET(sheet_motion_start, 0);
    SET(sheet_motion_enabled, false);
    SET(sheet_closing, false);
    SET(sheet_close_result, CUI_SURFACE_NONE);
#undef SET
    unsigned padding = 0;
    for (size_t i = 0; i < sizeof a; ++i)
        if (!member[i]) {
            ((unsigned char *)&a)[i] = 0x55;
            ((unsigned char *)&b)[i] = 0xaa;
            assert(((const unsigned char *)&a)[i] != ((const unsigned char *)&b)[i]);
            ++padding;
        }
    printf("DIALOG_COMPARE padding-distinct bytes%u; semantic mutation rejection PASS\n", padding);
    assert(state_equal(&a, &b));
    b.sheet_progress = .5f;
    assert(!state_equal(&a, &b));
    b.sheet_progress = a.sheet_progress;
    b.overlay.modal = false;
    assert(!state_equal(&a, &b));
    b.overlay.modal = a.overlay.modal;
    b.body_content.w += 1;
    assert(!state_equal(&a, &b));
    b.body_content.w = a.body_content.w;
    b.on_close = NULL;
    assert(!state_equal(&a, &b));
    b.on_close = a.on_close;
    assert(state_equal(&a, &b));
}
static void compare(const driver *a, const driver *b) {
    assert(a->state.close_user == a && b->state.close_user == b);
    assert(state_equal(&a->state, &b->state));
    assert(a->decisions == b->decisions && a->closes == b->closes);
    assert(a->decision_reason == b->decision_reason && a->close_reason == b->close_reason);
    assert(a->focus_on_close == b->focus_on_close);
    assert(cui_focused(a->ui) == cui_focused(b->ui));
    size_t na, nb;
    const cui_event *ea = cui_frame_events(a->ui, &na);
    const cui_event *eb = cui_frame_events(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i) {
        assert(ea[i].type == eb[i].type);
        assert(cui_event_target(a->ui, i) == cui_event_target(b->ui, i));
        assert(cui_event_activation(a->ui, i) == cui_event_activation(b->ui, i));
        assert(cui_event_consumed(a->ui, i) == cui_event_consumed(b->ui, i));
    }
    const cui_semantic *sa = cui_semantics(a->ui, &na), *sb = cui_semantics(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i) {
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role);
        assert(rect_equal(sa[i].bounds, sb[i].bounds));
    }
}
static void queue(driver *d, unsigned step) {
    if (step == 2) {
        assert(cui_queue_event(d->ui,
                               (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = 100, .y = 100}));
        assert(cui_queue_event(
            d->ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 100, .y = 100, .dy = -2}));
    }
    if (step == 4 || step == 6)
        assert(
            cui_queue_event(d->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
    if (step == 8 || step == 10 || step == 12 || step == 14) {
        float x = step == 8 ? 50 : step == 10 ? 80 : step == 12 ? 120 : 300;
        assert(
            cui_queue_event(d->ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = x, .y = 45}));
        assert(cui_queue_event(d->ui, (cui_event){.type = CUI_EVENT_POINTER_UP, .x = x, .y = 45}));
    }
}
static void composed_route_gate(void) {
    driver old = {0}, composed = {.composed = true};
    old.ui = cui_create((cui_config){0});
    composed.ui = cui_create((cui_config){0});
    assert(old.ui && composed.ui);
    cui_surface_init(&old.state, 50, CUI_SURFACE_DIALOG);
    cui_surface_init(&composed.state, 50, CUI_SURFACE_DIALOG);
    cui_surface_options o = {.viewport = {0, 0, 400, 300}};
    cui_surface_open(old.ui, &old.state, 1);
    cui_surface_open(composed.ui, &composed.state, 1);
    frame(&old, false, 0, o);
    frame(&composed, false, 0, o);
    /* Legacy max publishes from previous retained bounds on its next update. */
    frame(&old, false, 1, o);
    frame(&composed, false, 1, o);
    queue(&old, 2);
    queue(&composed, 2);
    frame(&old, false, 2, o);
    frame(&composed, false, 2, o);
    assert(cui_event_target(old.ui, 1) == cui_surface_part_id(50, 6));
    assert(cui_event_target(composed.ui, 1) == cui_surface_part_id(50, 6));
    assert(old.state.body_scroll == 96 && composed.state.body_scroll == 0);
    queue(&old, 8);
    queue(&composed, 8);
    frame(&old, false, 8, o);
    frame(&composed, false, 8, o);
    assert(!old.state.overlay.open && composed.state.overlay.open);
    assert(old.closes == 1 && composed.closes == 0);
    queue(&composed, 6);
    frame(&composed, false, 6, o);
    assert(!composed.state.overlay.open && composed.closes == 1);
    assert(composed.close_reason == CUI_SURFACE_CANCELLED);
    cui_surface_open(composed.ui, &composed.state, 1);
    frame(&composed, false, 0, o);
    cui_surface_request_close(composed.ui, &composed.state, CUI_SURFACE_ACCEPTED);
    frame(&composed, false, 15, o);
    assert(!composed.state.overlay.open && composed.closes == 2);
    assert(composed.close_reason == CUI_SURFACE_ACCEPTED);
    cui_destroy(old.ui);
    cui_destroy(composed.ui);
    puts("DIALOG_COMPOSED private wheel/action gates; Escape/request timing PASS");
}
int main(void) {
    comparator_oracle();
    unsigned frames = 0;
    for (unsigned kind = CUI_SURFACE_DIALOG; kind <= CUI_SURFACE_POPOVER; ++kind) {
        driver a = {0}, b = {0};
        a.ui = cui_create((cui_config){0});
        b.ui = cui_create((cui_config){0});
        assert(a.ui && b.ui);
        cui_surface_init(&a.state, 50, (cui_surface_kind)kind);
        legacy_cui_surface_init(&b.state, 50, (cui_surface_kind)kind);
        cui_surface_options o = {
            .viewport = {0, 0, 400, 300}, .show_alert_close = true, .alert_outside_dismiss = true};
        for (unsigned step = 0; step < 16; ++step) {
            if (!a.state.overlay.open) {
                cui_surface_open(a.ui, &a.state, 1);
                legacy_cui_surface_open(b.ui, &b.state, 1);
            }
            a.veto = b.veto = step == 3 || step == 4;
            if (step == 3 || step == 15) {
                cui_surface_request_close(a.ui, &a.state, CUI_SURFACE_ACCEPTED);
                legacy_cui_surface_request_close(b.ui, &b.state, CUI_SURFACE_ACCEPTED);
            }
            queue(&a, step);
            queue(&b, step);
            frame(&a, false, step, o);
            frame(&b, true, step, o);
            compare(&a, &b);
            ++frames;
        }
        assert(a.decisions > 0 && a.closes > 0);
        cui_destroy(a.ui);
        cui_destroy(b.ui);
    }
    composed_route_gate();
    printf("DIALOG_LEGACY frozen four-kind reducer trace %u frames PASS\n", frames);
    return 0;
}
