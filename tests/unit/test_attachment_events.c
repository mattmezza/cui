/* SPDX-License-Identifier: MIT */
#include "cui/cui_media_events.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_attachment_state state;
    cui_attachment_options options;
    cui_attachment_child children[3];
    unsigned old_mask, typed_mask, ambiguous_mask;
    unsigned magic;
    size_t old_calls[3], calls[3], child_calls;
    char order[32];
    size_t order_count;
    bool inert, layout, omit, request_omit, change_focus, consume;
    cui_id child_id;
    cui_attachment_action_event copies[3];
    cui_draw_list draw;
} fixture;
static void old_action(fixture *f, cui_id id, unsigned action) {
    assert(f->magic == 1234 && id == 20);
    ++f->old_calls[action];
    f->order[f->order_count++] = (char)('A' + action);
    if (f->change_focus) {
        cui_focus(f->ui, f->child_id);
        if (!action) {
            size_t count;
            (void)cui_frame_events(f->ui, &count);
            for (size_t e = 0; e < count; ++e)
                assert(cui_consume_event(f->ui, e));
            assert(!cui_event_accepted_activation(f->ui, 0));
        }
    }
}
static void old_open(void *user, cui_id id) {
    old_action(user, id, 0);
}
static void old_remove(void *user, cui_id id) {
    old_action(user, id, 1);
}
static void old_retry(void *user, cui_id id) {
    old_action(user, id, 2);
}
static void observed(void *user, const cui_attachment_action_event *event) {
    fixture *f = user;
    unsigned action = event->action;
    assert(f->magic == 1234 && action < 3);
    assert(event->event.kind == CUI_COMPONENT_EVENT_ACTIVATE && event->event.target == 20);
    assert(event->event.frame_sequence);
    if (f->ambiguous_mask & (1u << action)) {
        assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
        assert(event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    } else {
        assert(event->event.flags ==
               (CUI_COMPONENT_EVENT_HAS_FRAME | CUI_COMPONENT_EVENT_HAS_INDEX |
                CUI_COMPONENT_EVENT_HAS_SOURCE | CUI_COMPONENT_EVENT_HAS_ACTIVATION));
        assert(event->event.activation.frame_sequence == event->event.frame_sequence);
        assert(event->event.activation.event_index == event->event.event_index);
        if (!f->change_focus) {
            cui_activation_snapshot actual;
            assert(cui_event_activation_snapshot(f->ui, event->event.event_index, &actual));
            assert(!memcmp(&actual, &event->event.activation, sizeof actual));
        }
    }
    if (f->old_mask & (1u << action))
        assert(f->order_count && f->order[f->order_count - 1] == (char)('A' + action));
    f->order[f->order_count++] = (char)('a' + action);
    f->copies[action] = *event;
    ++f->calls[action];
    f->request_omit = true;
}
static void child(cui_context *ui, cui_id id, void *user, float width, float height) {
    fixture *f = user;
    (void)height;
    f->child_id = id;
    cui_interaction i =
        cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("Custom"), false, false, 0);
    cui_begin_box(ui, id, (cui_box){.width = width, .height = 24});
    cui_text(ui, cui_string("custom"), 12, (cui_color){0, 0, 0, 255});
    cui_end_box(ui);
    if (i.activated)
        ++f->child_calls;
}
static cui_attachment_event_result frame(fixture *f, bool facade) {
    assert(cui_begin_frame(f->ui, 600, 400, .016f));
    cui_begin_box(f->ui, 1000, (cui_box){.inert = f->inert, .layout_only = f->layout});
    f->order_count = 0;
    if (f->consume) {
        size_t count;
        (void)cui_frame_events(f->ui, &count);
        for (size_t e = 0; e < count; ++e)
            assert(cui_consume_event(f->ui, e));
    }
    cui_theme theme = cui_theme_light();
    cui_attachment_options options = f->options;
    options.children = f->children;
    options.on_click = f->old_mask & 1 ? old_open : NULL;
    options.on_remove = f->old_mask & 2 ? old_remove : NULL;
    options.on_retry = f->old_mask & 4 ? old_retry : NULL;
    options.action_user = f;
    options.actions_user = f;
    cui_attachment_event_result out = {0};
    if (!f->omit) {
        if (facade)
            out = cui_attachment_events(
                f->ui, 20, &theme, &f->state, options,
                (cui_attachment_handler){.user = f,
                                         .on_activate = f->typed_mask & 1 ? observed : NULL,
                                         .on_remove = f->typed_mask & 2 ? observed : NULL,
                                         .on_retry = f->typed_mask & 4 ? observed : NULL});
        else
            out.control = cui_attachment(f->ui, 20, &theme, &f->state, options);
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return out;
}
static cui_attachment_event_result pair(fixture *a, fixture *b) {
    cui_attachment_event_result x = frame(a, false), y = frame(b, true);
    assert(x.control.valid == y.control.valid && x.control.clicked == y.control.clicked &&
           x.control.removed == y.control.removed && x.control.retried == y.control.retried);
    assert(x.control.plan.width == y.control.plan.width &&
           x.control.plan.height == y.control.plan.height);
    assert(x.control.plan.remove_visible == y.control.plan.remove_visible &&
           x.control.plan.retry_media == y.control.plan.retry_media &&
           x.control.plan.retry_description == y.control.plan.retry_description);
    assert(!memcmp(&a->state.card, &b->state.card, sizeof a->state.card));
    assert(!memcmp(&a->state.media, &b->state.media, sizeof a->state.media));
    assert(cui_focused(a->ui) == cui_focused(b->ui));
    assert(cui_needs_redraw(a->ui) == cui_needs_redraw(b->ui));
    assert(a->child_calls == b->child_calls);
    if (a->old_mask == b->old_mask)
        assert(!memcmp(a->old_calls, b->old_calls, sizeof a->old_calls));
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a->ui, &na), *sb = cui_semantics(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i) {
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role && sa[i].value == sb[i].value);
        assert(sa[i].disabled == sb[i].disabled && sa[i].focused == sb[i].focused);
        assert(sa[i].details.pointer_activatable == sb[i].details.pointer_activatable);
    }
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *p = a->draw.commands + i, *q = b->draw.commands + i;
        assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof p->bounds));
        assert(!memcmp(&p->color, &q->color, sizeof p->color) && p->text.length == q->text.length);
        if (p->text.length)
            assert(!memcmp(p->text.data, q->text.data, p->text.length));
    }
    return y;
}
static void init(fixture *f, unsigned old, unsigned typed) {
    memset(f, 0, sizeof *f);
    f->ui = cui_create((cui_config){.max_elements = 256, .max_commands = 512});
    assert(f->ui);
    assert(!cui_queue_accessibility_action(f->ui, 20, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_last_error(f->ui) == CUI_ERROR_INVALID_ACTION);
    cui_clear_error(f->ui); /* Before first semantics, action admission is unavailable. */
    f->magic = 1234;
    f->old_mask = old;
    f->typed_mask = typed;
    f->options = cui_attachment_options_default();
    f->options.name = cui_string("Attachment");
    f->options.model.identified = true;
    f->options.model.touch = true;
    f->options.model.media = true;
    f->options.model.status = CUI_ATTACHMENT_FAILED;
    f->options.child_count = 3;
    f->children[0] = (cui_attachment_child){.kind = CUI_ATTACHMENT_TITLE, .text = {"File", 4}};
    f->children[1] = (cui_attachment_child){.kind = CUI_ATTACHMENT_DESCRIPTION,
                                            .text = {"Details", 7},
                                            .status_override = true,
                                            .status = CUI_ATTACHMENT_COMPLETE};
    f->children[2] =
        (cui_attachment_child){.kind = CUI_ATTACHMENT_DESCRIPTION, .text = {"More", 4}};
    f->options.actions = child;
    f->options.actions_width = 48;
}
static void activate(fixture *a, fixture *b, cui_id id) {
    assert(cui_queue_accessibility_action(a->ui, id, CUI_ACCESSIBILITY_ACTIVATE));
    assert(cui_queue_accessibility_action(b->ui, id, CUI_ACCESSIBILITY_ACTIVATE));
}
static cui_id description_retry(fixture *f) {
    size_t count;
    const cui_semantic *s = cui_semantics(f->ui, &count);
    cui_id found = 0;
    for (size_t i = 0; i < count; ++i)
        if (s[i].role == CUI_ROLE_LINK) {
            assert(!found); /* Only first eligible description gets retry. */
            found = s[i].id;
        }
    assert(found && found != cui_attachment_retry_id(20));
    return found;
}
static void actions_and_evidence(bool typed_only) {
    fixture a, b;
    init(&a, 7, 0);
    init(&b, typed_only ? 0 : 7, 7);
    pair(&a, &b);
    pair(&a, &b);
    cui_id description = description_retry(&b);
    assert(description == description_retry(&a));
    /* Inputs reverse category order; callbacks stay category ordered. */
    activate(&a, &b, cui_attachment_retry_id(20));
    activate(&a, &b, cui_attachment_remove_id(20));
    activate(&a, &b, 20);
    a.change_focus = b.change_focus =
        !typed_only; /* Later callback invalidates live focus evidence. */
    cui_attachment_event_result r = pair(&a, &b);
    assert(r.activation_count == 1 && r.remove_count == 1 && r.retry_count == 1);
    assert(r.control.clicked && r.control.removed && r.control.retried);
    assert(b.order_count == (typed_only ? 3u : 6u));
    assert(!memcmp(b.order, typed_only ? "abc" : "AaBbCc", b.order_count));
    assert(b.copies[0].event.event_index == 2 && b.copies[1].event.event_index == 1 &&
           b.copies[2].event.event_index == 0);
    assert(b.copies[0].event.activation.target == 20);
    assert(b.copies[1].event.activation.target == cui_attachment_remove_id(20));
    assert(b.copies[2].event.activation.target == cui_attachment_retry_id(20));
    a.change_focus = b.change_focus = false;
    activate(&a, &b, description);
    r = pair(&a, &b);
    assert(r.retry_count == 1 && !r.activation_count && !r.remove_count);
    assert(b.copies[2].event.activation.target == description);
    assert(b.copies[2].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    b.ambiguous_mask = 4;
    activate(&a, &b, description);
    activate(&a, &b, cui_attachment_retry_id(20));
    r = pair(&a, &b);
    assert(r.retry_count == 1); /* Media + description still one aggregate. */
    b.ambiguous_mask = 2;
    activate(&a, &b, cui_attachment_remove_id(20));
    activate(&a, &b, cui_attachment_remove_id(20));
    assert(pair(&a, &b).remove_count == 1);
    b.ambiguous_mask = 0;
    activate(&a, &b, b.child_id);
    r = pair(&a, &b);
    assert(!r.activation_count && !r.remove_count && !r.retry_count && b.child_calls == 1);
    /* Real pointer card action preserves genuine pointer evidence. */
    cui_rect card = b.state.card;
    cui_event pointer = {.type = CUI_EVENT_POINTER_DOWN,
                         .x = card.x + 8,
                         .y = card.y + 8,
                         .pointer_button = CUI_POINTER_PRIMARY};
    assert(cui_queue_event(a.ui, pointer) && cui_queue_event(b.ui, pointer));
    pointer.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(a.ui, pointer) && cui_queue_event(b.ui, pointer));
    r = pair(&a, &b);
    assert(r.activation_count == 1 &&
           b.copies[0].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    cui_focus(a.ui, cui_attachment_remove_id(20));
    cui_focus(b.ui, cui_attachment_remove_id(20));
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(pair(&a, &b).remove_count == 1 &&
           b.copies[1].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD);
    cui_attachment_action_event copy = b.copies[1];
    a.omit = b.omit = b.request_omit;
    r = pair(&a, &b);
    assert(!r.activation_count && !r.remove_count && !r.retry_count);
    cui_destroy(a.ui);
    cui_destroy(b.ui);
    assert(copy.action == CUI_ATTACHMENT_ACTION_REMOVE &&
           copy.event.activation.target == cui_attachment_remove_id(20));
}
static void availability_and_suppression(void) {
    fixture a, b;
    init(&a, 0, 0);
    init(&b, 0, 0);
    for (unsigned mask = 0; mask < 8; ++mask) {
        a.old_mask = mask; /* Legacy callbacks are availability-equivalent to typed-only. */
        b.typed_mask = mask;
        for (unsigned identified = 0; identified < 2; ++identified)
            for (unsigned touch = 0; touch < 2; ++touch)
                for (unsigned media = 0; media < 2; ++media)
                    for (unsigned status = CUI_ATTACHMENT_PENDING;
                         status <= CUI_ATTACHMENT_COMPLETE; ++status) {
                        a.options.model.identified = b.options.model.identified = identified != 0;
                        a.options.model.touch = b.options.model.touch = touch != 0;
                        a.options.model.media = b.options.model.media = media != 0;
                        a.options.model.status = b.options.model.status =
                            (cui_attachment_status)status;
                        cui_attachment_event_result r = pair(&a, &b);
                        assert(r.control.valid && !r.activation_count && !r.remove_count &&
                               !r.retry_count);
                        assert(r.control.plan.remove_visible ==
                               (identified && touch && (mask & 2)));
                        assert(r.control.plan.retry_media == (identified && media && (mask & 4) &&
                                                              status == CUI_ATTACHMENT_FAILED));
                    }
    }
    a.old_mask = 7;
    b.old_mask = 0;
    b.typed_mask = 7;
    a.options.model.touch = b.options.model.touch = false;
    pair(&a, &b);
    cui_rect hover = b.state.card;
    cui_event move = {
        .type = CUI_EVENT_POINTER_MOVE, .x = hover.x + hover.w / 2, .y = hover.y + hover.h / 2};
    assert(cui_queue_event(a.ui, move) && cui_queue_event(b.ui, move));
    assert(pair(&a, &b).control.plan.remove_visible);
    move.x = 590;
    move.y = 390;
    assert(cui_queue_event(a.ui, move) && cui_queue_event(b.ui, move));
    assert(!pair(&a, &b).control.plan.remove_visible);
    a.old_mask = b.old_mask = 7;
    b.typed_mask = 0; /* Exact direct legacy route, original userdata for all categories. */
    a.options.model.identified = b.options.model.identified = true;
    a.options.model.touch = b.options.model.touch = true;
    a.options.model.media = b.options.model.media = true;
    a.options.model.status = b.options.model.status = CUI_ATTACHMENT_FAILED;
    pair(&a, &b);
    pair(&a, &b);
    activate(&a, &b, 20);
    activate(&a, &b, cui_attachment_remove_id(20));
    activate(&a, &b, cui_attachment_retry_id(20));
    cui_attachment_event_result r = pair(&a, &b);
    assert(r.control.clicked && r.control.removed && r.control.retried);
    assert(!r.activation_count && !r.remove_count && !r.retry_count);
    b.typed_mask = 2; /* Observe one category without disturbing other old userdata. */
    activate(&a, &b, 20);
    activate(&a, &b, cui_attachment_remove_id(20));
    activate(&a, &b, cui_attachment_retry_id(20));
    r = pair(&a, &b);
    assert(!r.activation_count && r.remove_count == 1 && !r.retry_count);
    assert(b.order_count == 4 && !memcmp(b.order, "ABbC", 4));
    b.typed_mask = 7;
    a.consume = b.consume = true;
    activate(&a, &b, cui_attachment_remove_id(20));
    r = pair(&a, &b);
    assert(!r.control.removed && !r.remove_count);
    a.consume = b.consume = false;
    a.inert = b.inert = true;
    activate(&a, &b, 20);
    r = pair(&a, &b);
    assert(!r.activation_count && !r.remove_count && !r.retry_count);
    a.inert = b.inert = false;
    pair(&a, &b);
    a.layout = b.layout = true;
    activate(&a, &b, cui_attachment_remove_id(20));
    r = pair(&a, &b);
    assert(!r.activation_count && !r.remove_count && !r.retry_count);
    a.layout = b.layout = false;
    /* Uploading retains genuine OPEN and REMOVE actions, without RETRY. */
    a.options.model.status = b.options.model.status = CUI_ATTACHMENT_UPLOADING;
    pair(&a, &b);
    pair(&a, &b);
    activate(&a, &b, 20);
    activate(&a, &b, cui_attachment_remove_id(20));
    r = pair(&a, &b);
    assert(r.activation_count == 1 && r.remove_count == 1 && !r.retry_count);
    a.options.actions_width = b.options.actions_width = -1;
    r = pair(&a, &b);
    assert(!r.control.valid && !r.activation_count && !r.remove_count && !r.retry_count);
    cui_destroy(a.ui);
    cui_destroy(b.ui);
}

int main(void) {
    availability_and_suppression();
    actions_and_evidence(false);
    actions_and_evidence(true);
    return 0;
}
