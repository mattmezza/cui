/* SPDX-License-Identifier: MIT */
/* Independent frozen pre-extraction surface reducer versus current source. */
#include "cui/cui_surface.h"
#include <assert.h>
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
    bool veto;
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
    bool changed = legacy ? legacy_cui_base_surface_update(d->ui, &d->state, o)
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
static void compare(const driver *a, const driver *b) {
    cui_surface_state x = a->state, y = b->state;
    x.close_user = y.close_user = NULL;
    assert(memcmp(&x, &y, sizeof x) == 0);
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
        assert(memcmp(&sa[i].bounds, &sb[i].bounds, sizeof sa[i].bounds) == 0);
    }
}
static void queue(driver *d, unsigned step) {
    if (step == 2)
        assert(cui_queue_event(
            d->ui, (cui_event){.type = CUI_EVENT_SCROLL, .x = 100, .y = 100, .dy = -2}));
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
int main(void) {
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
    printf("DIALOG_LEGACY frozen four-kind reducer trace %u frames PASS\n", frames);
    return 0;
}
