/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include "cui/cui_hover_card_extended.h"
#include <assert.h>
#include <string.h>
#ifdef NDEBUG
#error Assertions must be enabled
#endif
typedef struct {
    cui_context *ui;
    cui_hover_state state;
    cui_hover_card_contact contact;
    cui_hover_card_policy policy;
    cui_hover_options options;
    cui_hover_card_extended_result result;
    cui_hover_lifecycle_event saved;
    cui_draw_list draw;
    unsigned calls, children, passive;
    bool omit, missing, consume_before, consume_after, disabled_trigger, legacy, no_handler;
    bool keyboard, fill_resource, modal;
} fixture;
static cui_semantic semantic(cui_context *ui, cui_id id) {
    size_t n;
    const cui_semantic *s = cui_semantics(ui, &n);
    for (size_t i = 0; i < n; ++i)
        if (s[i].id == id)
            return s[i];
    return (cui_semantic){0};
}
static void changed(void *user, const cui_hover_lifecycle_event *e) {
    fixture *f = user;
    assert(!f->children && e->previous_open != e->open && e->open == f->state.open);
    assert(e->event.target == 42 && e->event.kind == CUI_COMPONENT_EVENT_LIFECYCLE);
    assert(e->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(e->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(e->event.frame_sequence == cui_frame_sequence(f->ui));
    if (f->policy.relationship && !f->missing) {
        cui_semantic t = semantic(f->ui, 41);
        bool admitted = semantic(f->ui, 42).id != 0;
        assert(t.details.expanded == admitted && t.details.controls == (admitted ? 42 : 0));
    }
    f->calls++;
    f->saved = *e;
}
static fixture create(void) {
    fixture f;
    memset(&f, 0, sizeof(f));
    f.ui = cui_create((cui_config){.width = 300, .height = 220});
    assert(f.ui);
    f.policy = (cui_hover_card_policy){
        .mode = CUI_HOVER_CARD_TAP_TOGGLE, .relationship = true, .outside_tap = true};
    f.options = (cui_hover_options){.trigger = 41,
                                    .viewport = {0, 0, 300, 220},
                                    .side = CUI_POPUP_BELOW,
                                    .width = 140,
                                    .height = 70,
                                    .open_delay = 10,
                                    .close_delay = 10,
                                    .dt = .1f};
    return f;
}
static void frame(fixture *f) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(f->ui, 300, 220, .016f));
    if (f->consume_before)
        assert(cui_consume_event(f->ui, 0));
    cui_begin_box(
        f->ui, 1,
        (cui_box){
            .width = 300, .height = 220, .inert = f->passive == 1, .layout_only = f->passive == 2});
    if (f->modal) {
        assert(cui_begin_focus_scope(f->ui, 99, true));
        cui_begin_box(f->ui, 99, (cui_box){.width = 20, .height = 20});
        cui_interact(f->ui, 99, CUI_ROLE_GROUP, cui_string("Modal"), false, false, 0);
        cui_end_box(f->ui);
        cui_end_focus_scope(f->ui);
    }
    bool accepted = false;
    if (!f->missing) {
        accepted = cui_button(f->ui, 41, &theme, cui_string("Trigger"), f->disabled_trigger);
        cui_set_semantic_details(f->ui, 41,
                                 (cui_semantic_details){.required = true,
                                                        .readonly = true,
                                                        .description = cui_string("Preserve me"),
                                                        .focus_target = 41,
                                                        .release_key_activation = !f->keyboard});
    }
    if (f->consume_after)
        assert(cui_consume_event(f->ui, 0));
    f->children = 0;
    if (f->fill_resource)
        assert(cui_resource_get(f->ui, 99, 99, 1, NULL, NULL));
    if (!f->omit) {
        cui_hover_handler handler = {f->no_handler ? NULL : changed, f};
        if (f->legacy)
            f->result.lifecycle = cui_begin_hover_card_events(
                f->ui, 42, &theme, &f->state, cui_string("Details"), f->options, handler);
        else {
            f->policy.accepted_trigger_activation = accepted;
            f->result =
                cui_begin_hover_card_extended(f->ui, 42, &theme, &f->state, cui_string("Details"),
                                              f->options, &f->policy, &f->contact, handler);
            assert(f->result.lifecycle.valid);
        }
        if (f->result.lifecycle.control_result) {
            f->children++;
            (void)cui_button(f->ui, 43, &theme, cui_string("Child"), false);
            cui_end_hover_card(f->ui);
        }
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) ==
           (f->fill_resource && f->state.open ? CUI_ERROR_CAPACITY : CUI_OK));
}
static void event(fixture *f, cui_event_type type, float x, float y, uint64_t contact) {
    assert(cui_queue_event(f->ui, (cui_event){.type = type,
                                              .x = x,
                                              .y = y,
                                              .pointer_type = CUI_POINTER_TOUCH,
                                              .pointer_id = contact,
                                              .pointer_device = 2}));
}
static void tap(fixture *f, cui_id id) {
    cui_rect b = cui_bounds(f->ui, id);
    assert(b.w > 0 && b.h > 0);
    event(f, CUI_EVENT_POINTER_DOWN, b.x + b.w / 2, b.y + b.h / 2, 1);
    event(f, CUI_EVENT_POINTER_UP, b.x + b.w / 2, b.y + b.h / 2, 1);
    frame(f);
}
static void opened(fixture *f) {
    frame(f);
    tap(f, 41);
    assert(f->state.open && f->result.lifecycle.control_result && f->result.relationship_applied);
    assert(f->result.lifecycle.changed && f->result.lifecycle.change_count == 1);
    cui_semantic t = semantic(f->ui, 41), c = semantic(f->ui, 42);
    assert(t.details.required && t.details.readonly && t.details.focus_target == 41);
    assert(t.details.release_key_activation && t.details.description.length == 11);
    assert(t.details.expanded && t.details.controls == 42 && c.details.parent == 41);
    assert(c.role == CUI_ROLE_GROUP && c.name.length == 7);
}
static void mobile(void) {
    fixture f = create();
    frame(&f);
    cui_focus(f.ui, 41);
    event(&f, CUI_EVENT_POINTER_MOVE, 2, 2, 0);
    f.options.dt = 100;
    frame(&f);
    assert(!f.state.open && !f.state.pending);
    tap(&f, 41);
    assert(f.state.open);
    tap(&f, 43);
    assert(f.state.open && !f.result.lifecycle.changed);
    tap(&f, 41);
    assert(!f.state.open && f.result.lifecycle.changed);
    assert(!semantic(f.ui, 41).details.expanded && !semantic(f.ui, 41).details.controls);
    cui_destroy(f.ui);
}
static void outside_cases(void) {
    for (unsigned mode = 0; mode < 11; ++mode) {
        fixture f = create();
        opened(&f);
        unsigned before = f.calls;
        event(&f, CUI_EVENT_POINTER_DOWN, 280, 200, 1);
        frame(&f);
        assert(f.contact.active && f.contact.pending);
        switch (mode) {
        case 1:
            event(&f, CUI_EVENT_CANCEL, 0, 0, 1);
            break;
        case 2:
            event(&f, CUI_EVENT_POINTER_MOVE, 288, 200, 1);
            event(&f, CUI_EVENT_POINTER_MOVE, 280, 200, 1);
            break;
        case 3:
            f.consume_before = true;
            break;
        case 4:
            event(&f, CUI_EVENT_POINTER_UP, 280, 200, 99);
            break;
        case 5:
            event(&f, CUI_EVENT_POINTER_DOWN, 281, 200, 2);
            break;
        case 6:
            event(&f, CUI_EVENT_SCROLL, 280, 200, 1);
            break;
        case 7:
            f.omit = true;
            frame(&f);
            f.omit = false;
            break;
        case 8:
            f.policy.mode = CUI_HOVER_CARD_DESKTOP_HOVER;
            f.policy.outside_tap = false;
            frame(&f);
            f.policy.mode = CUI_HOVER_CARD_TAP_TOGGLE;
            f.policy.outside_tap = true;
            break;
        case 9:
            event(&f, CUI_EVENT_GESTURE, 280, 200, 1);
            break;
        case 10:
            f.options.disabled = true;
            break;
        default:
            break;
        }
        if (mode != 4)
            event(&f, CUI_EVENT_POINTER_UP, 280, 200, 1);
        frame(&f);
        assert(f.state.open == (mode != 0 && mode != 10));
        assert(f.calls == before + (mode == 0 || mode == 10));
        cui_destroy(f.ui);
    }
    fixture f = create();
    opened(&f);
    cui_rect b = cui_bounds(f.ui, 43);
    event(&f, CUI_EVENT_POINTER_DOWN, b.x + 2, b.y + 2, 1);
    frame(&f);
    event(&f, CUI_EVENT_POINTER_UP, 280, 200, 1);
    frame(&f);
    assert(f.state.open);
    event(&f, CUI_EVENT_POINTER_DOWN, 280, 200, 1);
    event(&f, CUI_EVENT_POINTER_UP, 280, 200, 1);
    frame(&f);
    assert(!f.state.open);
    cui_destroy(f.ui);
}
static void acceptance_and_escape(void) {
    for (unsigned mode = 0; mode < 4; ++mode) {
        fixture f = create();
        f.keyboard = mode >= 2;
        frame(&f);
        if (mode < 2) {
            assert(cui_queue_event(
                f.ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                  .accessibility_target = 41,
                                  .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
            f.consume_before = mode == 0;
            f.consume_after = mode == 1;
        } else {
            cui_focus(f.ui, 41);
            assert(cui_queue_event(f.ui,
                                   (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
            assert(
                cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_ENTER}));
            f.disabled_trigger = mode == 3;
        }
        frame(&f);
        assert(f.state.open == (mode == 1 || mode == 2));
        cui_destroy(f.ui);
    }
    fixture f = create();
    opened(&f);
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN,
                                             .key = CUI_KEY_ESCAPE,
                                             .repeat = true,
                                             .modifiers = CUI_MOD_CTRL}));
    f.consume_before = true;
    frame(&f);
    assert(!f.state.open && f.state.suppressed && f.calls == 2);
    cui_hover_lifecycle_event copy = f.saved;
    cui_destroy(f.ui);
    assert(!copy.open && copy.previous_open && copy.event.frame_sequence);
}
static void relationships_and_protection(void) {
    fixture f = create();
    opened(&f);
    f.options.viewport.w = 0;
    frame(&f);
    assert(!f.state.open && !f.result.lifecycle.control_result && f.result.relationship_applied);
    assert(!semantic(f.ui, 41).details.expanded);
    f.missing = true;
    frame(&f);
    assert(!f.result.relationship_applied);
    cui_destroy(f.ui);
    f = create();
    cui_destroy(f.ui);
    f.ui = cui_create((cui_config){.width = 300, .height = 220, .max_resources = 1});
    assert(f.ui);
    f.fill_resource = true;
    f.options.height = 0;
    frame(&f);
    tap(&f, 41);
    assert(f.state.open && !f.result.lifecycle.control_result && f.result.relationship_applied);
    assert(f.result.lifecycle.changed && !semantic(f.ui, 41).details.expanded);
    cui_destroy(f.ui);
    for (unsigned mode = 0; mode < 3; ++mode) {
        f = create();
        opened(&f);
        f.passive = mode;
        f.no_handler = mode == 0;
        f.options.disabled = true;
        unsigned before = f.calls;
        frame(&f);
        assert(!f.state.open && f.result.lifecycle.changed && !f.result.lifecycle.change_count);
        assert(f.calls == before && f.result.relationship_applied == (mode == 0));
        cui_destroy(f.ui);
    }
}
static void routed_input_and_validation(void) {
    fixture f = create();
    frame(&f);
    cui_rect b = cui_bounds(f.ui, 41);
    /* A cancelled real trigger contact does not become the caller assertion. */
    event(&f, CUI_EVENT_POINTER_DOWN, b.x + 2, b.y + 2, 1);
    event(&f, CUI_EVENT_CANCEL, 0, 0, 1);
    event(&f, CUI_EVENT_POINTER_UP, b.x + 2, b.y + 2, 1);
    frame(&f);
    assert(!f.state.open && !f.policy.accepted_trigger_activation);
    /* Desktop primary pointer can explicitly use tap mode too. */
    assert(cui_queue_event(
        f.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN, .x = b.x + 2, .y = b.y + 2}));
    assert(cui_queue_event(f.ui,
                           (cui_event){.type = CUI_EVENT_POINTER_UP, .x = b.x + 2, .y = b.y + 2}));
    frame(&f);
    assert(f.state.open && f.policy.accepted_trigger_activation);
    event(&f, CUI_EVENT_POINTER_DOWN, 280, 200, 1);
    frame(&f);
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_POINTER_UP,
                                             .x = 280,
                                             .y = 200,
                                             .pointer_type = CUI_POINTER_TOUCH,
                                             .pointer_id = 1,
                                             .pointer_device = 9}));
    frame(&f);
    assert(f.state.open);
    cui_hover_state before = f.state;
    cui_hover_card_contact old_contact = f.contact;
    cui_theme theme = cui_theme_light();
    cui_hover_card_policy invalid = {.mode = (cui_hover_card_mode)99};
    cui_hover_card_extended_result invalid_result =
        cui_begin_hover_card_extended(f.ui, 42, &theme, &f.state, cui_string("Details"), f.options,
                                      &invalid, &f.contact, (cui_hover_handler){changed, &f});
    assert(!invalid_result.lifecycle.valid);
    assert(!memcmp(&before, &f.state, sizeof(before)) &&
           !memcmp(&old_contact, &f.contact, sizeof(old_contact)));
    cui_destroy(f.ui);
    f = create();
    opened(&f);
    f.modal = true;
    frame(&f);
    event(&f, CUI_EVENT_POINTER_DOWN, 280, 200, 1);
    event(&f, CUI_EVENT_POINTER_UP, 280, 200, 1);
    frame(&f);
    assert(f.state.open && !f.result.lifecycle.changed);
    cui_destroy(f.ui);
}
static void independent_cards(void) {
    cui_context *ui = cui_create((cui_config){.width = 500, .height = 300});
    assert(ui);
    cui_theme theme = cui_theme_light();
    cui_hover_state state[2] = {{0}};
    cui_hover_card_contact contact[2] = {{0}};
    for (unsigned step = 0; step < 3; ++step) {
        if (step == 1)
            for (unsigned i = 0; i < 2; ++i)
                assert(cui_queue_event(
                    ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                    .accessibility_target = 10 + 10 * i,
                                    .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
        if (step == 2)
            assert(cui_queue_event(ui,
                                   (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
        assert(cui_begin_frame(ui, 500, 300, .016f));
        bool accepted[2];
        for (unsigned i = 0; i < 2; ++i)
            accepted[i] = cui_button(ui, 10 + 10 * i, &theme, cui_string("Trigger"), false);
        for (unsigned i = 0; i < 2; ++i) {
            cui_hover_card_policy policy = {.mode = CUI_HOVER_CARD_TAP_TOGGLE,
                                            .accepted_trigger_activation = accepted[i],
                                            .outside_tap = true,
                                            .relationship = true};
            cui_hover_card_extended_result r = cui_begin_hover_card_extended(
                ui, 11 + 10 * i, &theme, &state[i], cui_string("Details"),
                (cui_hover_options){.trigger = 10 + 10 * i,
                                    .viewport = {0, 0, 500, 300},
                                    .width = 140,
                                    .height = 70},
                &policy, &contact[i], (cui_hover_handler){0});
            assert(r.lifecycle.valid && r.relationship_applied);
            assert(r.lifecycle.control_result == (step == 1));
            if (r.lifecycle.control_result)
                cui_end_hover_card(ui);
            assert(semantic(ui, 10 + 10 * i).details.controls == (step == 1 ? 11 + 10 * i : 0));
        }
        (void)cui_end_frame(ui);
        assert(cui_last_error(ui) == CUI_OK);
    }
    cui_destroy(ui);
}
static void desktop_parity(void) {
    fixture a = create(), b = create();
    a.legacy = true;
    a.policy.relationship = false;
    b.policy = (cui_hover_card_policy){0};
    for (unsigned step = 0; step < 6; ++step) {
        if (step == 1) {
            cui_focus(a.ui, 41);
            cui_focus(b.ui, 41);
            a.options.open_delay = b.options.open_delay = 0;
        }
        if (step == 3) {
            assert(cui_queue_event(a.ui,
                                   (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
            assert(cui_queue_event(b.ui,
                                   (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ESCAPE}));
            a.consume_before = b.consume_before = true;
        }
        if (step == 4) {
            a.consume_before = b.consume_before = false;
            cui_focus(a.ui, 0);
            cui_focus(b.ui, 0);
        }
        frame(&a);
        frame(&b);
        assert(a.state.open == b.state.open && a.state.pending == b.state.pending &&
               a.state.suppressed == b.state.suppressed && a.state.target == b.state.target &&
               a.state.elapsed == b.state.elapsed);
        assert(a.result.lifecycle.control_result == b.result.lifecycle.control_result);
        assert(a.result.lifecycle.changed == b.result.lifecycle.changed && a.calls == b.calls);
        cui_rect x = cui_bounds(a.ui, 42), y = cui_bounds(b.ui, 42);
        assert(!memcmp(&x, &y, sizeof(x)) && !semantic(b.ui, 41).details.controls);
        assert(a.draw.count == b.draw.count);
        for (size_t i = 0; i < a.draw.count; ++i) {
            const cui_draw_command *p = &a.draw.commands[i], *q = &b.draw.commands[i];
            assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof(p->bounds)));
            assert(!memcmp(&p->color, &q->color, sizeof(p->color)) && p->font_size == q->font_size);
            if (p->type == CUI_DRAW_TEXT)
                assert(p->text.length == q->text.length &&
                       !memcmp(p->text.data, q->text.data, p->text.length));
        }
    }
    cui_destroy(a.ui);
    cui_destroy(b.ui);
}
int main(void) {
    mobile();
    outside_cases();
    acceptance_and_escape();
    relationships_and_protection();
    routed_input_and_validation();
    independent_cards();
    desktop_parity();
    return 0;
}
