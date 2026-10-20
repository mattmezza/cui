/* SPDX-License-Identifier: MIT */
#include "cui/cui_numeric_selection_events.h"
#include "cui/cui_stepper_navigation.h"
#include <assert.h>
#include <string.h>
#ifdef NDEBUG
#error Assertions must be enabled
#endif
static const cui_step steps[] = {
    {.title = {"Start", 5}, .status = CUI_STEP_ACTIVE},
    {.title = {"Payment", 7}, .description = {"Details", 7}, .status = CUI_STEP_ERROR},
    {.title = {"Finish", 6}, .status = CUI_STEP_COMPLETE}};
typedef struct {
    cui_context *ui;
    size_t owner, captured, calls;
    bool accept, no_handler, disabled, item_disabled, consume, cancel, mutate_after_copy;
    bool base, vertical, legacy, omit;
    unsigned passive;
    cui_stepper_navigation_request saved[8];
    cui_draw_list draw;
    cui_stepper_navigation_result result;
} fixture;
static cui_semantic item(cui_context *ui, cui_id id) {
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; ++i)
        if (s[i].id == id)
            return s[i];
    return (cui_semantic){0};
}
static void request(void *user, const cui_stepper_navigation_request *r) {
    fixture *f = user;
    assert(f->calls < 8 && r->selected == f->captured && r->count == 3);
    assert(r->event.kind == CUI_COMPONENT_EVENT_REQUEST && r->event.target == 10);
    assert(r->event.flags == 15 && r->event.frame_sequence == cui_frame_sequence(f->ui));
    assert(r->event.activation.target == cui_numeric_part_id(10, 1 + (unsigned)r->requested));
    assert(r->event.event_index == r->event.activation.event_index);
    assert(item(f->ui, cui_numeric_part_id(10, 1 + (unsigned)f->captured)).details.selected);
    /* All current semantics are registered before dispatch. */
    size_t count;
    (void)cui_semantics(f->ui, &count);
    assert(count == 4);
    f->saved[f->calls++] = *r;
    if (f->mutate_after_copy) {
        assert(cui_consume_event(f->ui, r->event.event_index));
        assert(r->event.flags == 15 && r->event.activation.target);
    }
    if (f->accept)
        f->owner = r->requested; /* Application-owned validation acceptance. */
}
static fixture create(void) {
    fixture f;
    memset(&f, 0, sizeof(f));
    f.ui = cui_create((cui_config){.width = 800, .height = 400});
    assert(f.ui);
    return f;
}
static void frame(fixture *f) {
    assert(cui_begin_frame(f->ui, 800, 400, .016f));
    cui_begin_box(
        f->ui, 1,
        (cui_box){
            .width = 800, .height = 400, .inert = f->passive == 1, .layout_only = f->passive == 2});
    if (f->consume)
        assert(cui_consume_event(f->ui, 0));
    if (f->cancel)
        assert(cui_cancel_event_activation(f->ui, 0));
    cui_theme t = cui_theme_light();
    cui_step copy[3];
    memcpy(copy, steps, sizeof(copy));
    copy[1].disabled = f->item_disabled;
    f->captured = f->owner;
    if (!f->omit) {
        if (f->legacy) {
            size_t current = f->owner;
            if (f->base)
                (void)cui_base_stepper(f->ui, 10, copy, 3, &current, f->disabled);
            else
                (void)cui_stepper(f->ui, 10, &t, copy, 3, &current, f->vertical, f->disabled);
        } else {
            cui_stepper_navigation_handler h = {f->no_handler ? NULL : request, f};
            f->result =
                f->base ? cui_base_stepper_navigation(f->ui, 10, copy, 3, f->owner, f->disabled, h)
                        : cui_stepper_navigation(f->ui, 10, &t, copy, 3, f->owner, f->vertical,
                                                 f->disabled, h);
            assert(f->result.valid);
        }
    }
    assert(copy[0].status == CUI_STEP_ACTIVE && copy[1].status == CUI_STEP_ERROR &&
           copy[2].status == CUI_STEP_COMPLETE);
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
}
static void activate(fixture *f, unsigned index) {
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE,
                           .accessibility_target = cui_numeric_part_id(10, 1 + index)}));
}
static void controlled(bool base) {
    fixture f = create();
    f.base = base;
    frame(&f);
    activate(&f, 1);
    frame(&f);
    assert(f.result.request_count == 1 && f.calls == 1 && f.owner == 0);
    assert(item(f.ui, cui_numeric_part_id(10, 1)).details.selected);
    assert(!item(f.ui, cui_numeric_part_id(10, 2)).details.selected);
    assert(f.saved[0].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    /* A request for the already selected item is still delivered. */
    activate(&f, 0);
    frame(&f);
    assert(f.result.request_count == 1 && f.calls == 2 && f.saved[1].requested == 0);
    f.accept = true;
    activate(&f, 1);
    activate(&f, 2);
    activate(&f, 0);
    frame(&f);
    assert(f.result.request_count == 3 && f.calls == 5 && f.owner == 0);
    assert(f.saved[2].selected == 0 && f.saved[3].selected == 0 && f.saved[4].selected == 0);
    assert(f.saved[2].requested == 1 && f.saved[3].requested == 2 && f.saved[4].requested == 0);
    assert(f.saved[2].event.event_index < f.saved[3].event.event_index);
    activate(&f, 1);
    frame(&f);
    assert(f.owner == 1 && item(f.ui, cui_numeric_part_id(10, 1)).details.selected);
    frame(&f);
    assert(!f.result.request_count && item(f.ui, cui_numeric_part_id(10, 2)).details.selected);
    f.no_handler = true;
    activate(&f, 2);
    frame(&f);
    assert(f.result.request_count == 1 && f.calls == 6 && f.owner == 1);
    f.no_handler = false;
    f.mutate_after_copy = true;
    activate(&f, 0);
    frame(&f);
    assert(f.calls == 7 && f.owner == 0);
    cui_stepper_navigation_request copied = f.saved[6];
    cui_destroy(f.ui);
    assert(copied.selected == 1 && copied.requested == 0 && copied.event.activation.target);
}
static void genuine_inputs(void) {
    for (unsigned mode = 0; mode < 5; ++mode) {
        fixture f = create();
        frame(&f);
        cui_id target = cui_numeric_part_id(10, 2);
        if (mode < 3) {
            cui_rect r = cui_bounds(f.ui, target);
            assert(r.w > 0 && r.h > 0);
            cui_pointer_type type = mode == 1 ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE;
            cui_event e = {.type = CUI_EVENT_POINTER_DOWN,
                           .x = r.x + r.w / 2,
                           .y = r.y + r.h / 2,
                           .pointer_type = type,
                           .pointer_id = 6,
                           .pointer_device = 8};
            assert(cui_queue_event(f.ui, e));
            if (mode == 2) {
                e.type = CUI_EVENT_CANCEL;
                assert(cui_queue_event(f.ui, e));
            }
            e.type = CUI_EVENT_POINTER_UP;
            assert(cui_queue_event(f.ui, e));
        } else {
            cui_focus(f.ui, target);
            cui_key key = mode == 3 ? CUI_KEY_ENTER : CUI_KEY_SPACE;
            assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = key}));
            if (key == CUI_KEY_SPACE)
                assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = key}));
        }
        frame(&f);
        assert(f.result.request_count == (mode == 2 ? 0 : 1));
        assert(f.owner == 0);
        if (mode != 2) {
            assert(f.saved[0].requested == 1);
            assert(f.saved[0].event.source == (mode < 2 ? CUI_COMPONENT_EVENT_SOURCE_POINTER
                                                        : CUI_COMPONENT_EVENT_SOURCE_KEYBOARD));
            if (mode < 2)
                assert(f.saved[0].event.activation.has_press &&
                       f.saved[0].event.activation.has_release);
        }
        cui_destroy(f.ui);
    }
}
static void rejection(void) {
    for (unsigned mode = 0; mode < 7; ++mode) {
        fixture f = create();
        frame(&f);
        if (mode == 6) {
            cui_focus(f.ui, cui_numeric_part_id(10, 2));
            assert(cui_queue_event(f.ui,
                                   (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
        } else
            activate(&f, 1);
        f.disabled = mode == 0;
        f.item_disabled = mode == 1;
        f.passive = mode == 2 ? 1 : mode == 3 ? 2 : 0;
        f.consume = mode == 4 || mode == 6;
        f.cancel = mode == 5;
        frame(&f);
        /* Consumed ordinary Enter remains accepted under Core's existing policy. */
        assert(f.result.request_count == (mode == 6 ? 1 : 0));
        assert(f.calls == (mode == 6 ? 1 : 0) && f.owner == 0);
        cui_destroy(f.ui);
    }
    fixture f = create();
    frame(&f);
    activate(&f, 1);
    f.omit = true;
    frame(&f);
    f.omit = false;
    frame(&f);
    assert(!f.result.request_count && !f.calls);
    cui_destroy(f.ui);
}
static void invalid(void) {
    fixture f = create();
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(f.ui, 800, 400, .016f));
    cui_stepper_navigation_handler h = {request, &f};
    assert(!cui_stepper_navigation(f.ui, 10, NULL, steps, 3, 0, false, false, h).valid);
    assert(!cui_base_stepper_navigation(f.ui, 0, steps, 3, 0, false, h).valid);
    assert(!cui_base_stepper_navigation(f.ui, 10, steps, 3, 3, false, h).valid);
    assert(!cui_base_stepper_navigation(f.ui, 10, NULL, 1, 0, false, h).valid);
    assert(!cui_base_stepper_navigation(f.ui, 10, steps, 101, 0, false, h).valid);
    assert(!cui_base_stepper_navigation(f.ui, 10, NULL, 0, 1, false, h).valid);
    cui_step bad = {.title = {NULL, 1}};
    assert(!cui_base_stepper_navigation(f.ui, 10, &bad, 1, 0, false, h).valid);
    bad = steps[0];
    bad.description = (cui_str){NULL, 1};
    assert(!cui_base_stepper_navigation(f.ui, 10, &bad, 1, 0, false, h).valid);
    bad = steps[0];
    bad.status = (cui_step_status)99;
    assert(!cui_base_stepper_navigation(f.ui, 10, &bad, 1, 0, false, h).valid);
    size_t n;
    (void)cui_semantics(f.ui, &n);
    assert(n == 0 && !f.calls);
    assert(cui_stepper_navigation(f.ui, 10, &theme, NULL, 0, 0, false, false, h).valid);
    (void)cui_end_frame(f.ui);
    assert(cui_last_error(f.ui) == CUI_OK);
    cui_destroy(f.ui);
}
static void parity(void) {
    for (unsigned vertical = 0; vertical < 2; ++vertical)
        for (unsigned selected = 0; selected < 3; ++selected) {
            fixture a = create(), b = create();
            a.legacy = true;
            a.vertical = b.vertical = vertical != 0;
            a.owner = b.owner = selected;
            frame(&a);
            frame(&b);
            assert(a.draw.count == b.draw.count);
            for (size_t i = 0; i < a.draw.count; ++i) {
                const cui_draw_command *p = &a.draw.commands[i], *q = &b.draw.commands[i];
                assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof(p->bounds)));
                assert(!memcmp(&p->color, &q->color, sizeof(p->color)) &&
                       p->font_size == q->font_size);
                if (p->type == CUI_DRAW_TEXT)
                    assert(p->text.length == q->text.length &&
                           !memcmp(p->text.data, q->text.data, p->text.length));
            }
            for (unsigned i = 0; i < 3; ++i) {
                cui_semantic p = item(a.ui, cui_numeric_part_id(10, 1 + i));
                cui_semantic q = item(b.ui, cui_numeric_part_id(10, 1 + i));
                assert(p.role == q.role && p.details.selected == q.details.selected &&
                       p.details.parent == q.details.parent);
                assert(!memcmp(&p.bounds, &q.bounds, sizeof(p.bounds)));
            }
            cui_destroy(a.ui);
            cui_destroy(b.ui);
        }
}
static void capacity_rejection(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 8, .width = 800, .height = 400});
    assert(ui);
    assert(cui_begin_frame(ui, 800, 400, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 800, .height = 400});
    assert(
        cui_base_stepper_navigation(ui, 10, steps, 3, 0, false, (cui_stepper_navigation_handler){0})
            .valid);
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                           .accessibility_target = cui_numeric_part_id(10, 2),
                                           .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
    assert(cui_begin_frame(ui, 800, 400, .016f));
    cui_begin_box(ui, 1, (cui_box){.width = 800, .height = 400});
    for (unsigned i = 0; i < 8; ++i)
        cui_interact(ui, 100 + i, CUI_ROLE_GROUP, cui_string("Fill"), false, false, 0);
    cui_stepper_navigation_result r = cui_base_stepper_navigation(
        ui, 10, steps, 3, 0, false, (cui_stepper_navigation_handler){0});
    assert(r.valid && r.request_count == 0); /* Valid arguments cannot bypass admission failure. */
    cui_end_box(ui);
    (void)cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_ERROR_CAPACITY);
    cui_destroy(ui);
}
int main(void) {
    controlled(false);
    controlled(true);
    genuine_inputs();
    rejection();
    invalid();
    parity();
    capacity_rejection();
    return 0;
}
