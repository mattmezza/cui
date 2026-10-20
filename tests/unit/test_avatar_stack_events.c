/* SPDX-License-Identifier: MIT */
#include "cui/cui_avatar_events.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_avatar_item items[5];
    cui_avatar_stack_options options;
    cui_avatar_stack_activation copies[16];
    size_t calls, control;
    bool consume_later, inert, layout, duplicate;
    cui_draw_list draw;
} fixture;
static void observed(void *user, const cui_avatar_stack_activation *event) {
    fixture *f = user;
    assert(f->calls < 16 && event->index < 3 && event->event.target == 100);
    assert(event->key == f->items[event->index].key);
    if (f->duplicate)
        assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME &&
               event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    else {
        assert(event->event.flags == 15);
        assert(cui_event_consumed(f->ui, event->event.event_index));
    }
    f->copies[f->calls++] = *event;
    if (f->consume_later) {
        size_t count;
        (void)cui_frame_events(f->ui, &count);
        for (size_t i = event->event.event_index + 1; i < count; i++)
            assert(cui_consume_event(f->ui, i));
    }
}
static fixture make(void) {
    fixture f = {0};
    f.ui = cui_create((cui_config){.width = 240, .height = 100});
    assert(f.ui);
    const char *names[] = {"Zero", "Ada", "Bob", "Hidden", "Other"};
    for (size_t i = 0; i < 5; i++)
        f.items[i] = (cui_avatar_item){.key = i ? i * 11 : 0, .name = cui_string(names[i])};
    f.options =
        (cui_avatar_stack_options){.size = 48, .interactive = true, .name = cui_string("Team")};
    return f;
}
static cui_avatar_stack_event_result frame(fixture *f, bool facade, bool handler) {
    assert(cui_begin_frame(f->ui, 240, 100, .016f));
    cui_begin_box(f->ui, 1000, (cui_box){.inert = f->inert, .layout_only = f->layout});
    cui_theme theme = cui_theme_light();
    if (f->duplicate)
        cui_interact(f->ui, 100, CUI_ROLE_GROUP, cui_string("Duplicate"), false, false, 0);
    cui_avatar_stack_event_result result = {.control = SIZE_MAX};
    if (facade)
        result = cui_avatar_stack_events(f->ui, 100, &theme, f->items, 5, f->options,
                                         (cui_avatar_stack_handler){handler ? observed : NULL, f});
    else
        result.control = cui_avatar_stack(f->ui, 100, &theme, f->items, 5, f->options);
    f->control = result.control;
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    return result;
}
static void warm(fixture *f) {
    frame(f, true, false);
    frame(f, true, false);
}
static cui_id child(fixture *f, size_t index) {
    size_t count;
    const cui_semantic *sem = cui_semantics(f->ui, &count);
    for (size_t i = 0; i < count; i++)
        if (sem[i].name.length == f->items[index].name.length &&
            !memcmp(sem[i].name.data, f->items[index].name.data, sem[i].name.length))
            return sem[i].id;
    return 0;
}
static void click(fixture *f, size_t index) {
    cui_rect bounds = cui_bounds(f->ui, child(f, index));
    assert(bounds.w == 48);
    cui_event event = {.type = CUI_EVENT_POINTER_DOWN, .x = bounds.x + 12, .y = bounds.y + 24};
    assert(cui_queue_event(f->ui, event));
    event.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(f->ui, event));
}
static void order(void) {
    fixture f = make();
    warm(&f);
    cui_id targets[] = {child(&f, 0), child(&f, 1), child(&f, 2)};
    assert(!child(&f, 3));
    click(&f, 2);
    click(&f, 0);
    click(&f, 1);
    cui_avatar_stack_event_result result = frame(&f, true, true);
    assert(result.action.valid && result.action.submitted && !result.action.request_count &&
           result.action.activation_count == 3 && result.control == 2 && f.calls == 3);
    const size_t indices[] = {3, 5, 1};
    for (size_t i = 0; i < 3; i++) {
        assert(f.copies[i].index == i && f.copies[i].event.event_index == indices[i]);
        assert(f.copies[i].event.activation.target == targets[i]);
        assert(f.copies[i].event.source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    }
    assert(f.copies[0].key == 0); /* Payload preserves the original zero application key. */
    cui_focus(f.ui, targets[1]);
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(cui_queue_accessibility_action(f.ui, targets[1], CUI_ACCESSIBILITY_ACTIVATE));
    result = frame(&f, true, true);
    assert(result.control == 1 && result.action.activation_count == 2 && f.calls == 5);
    assert(f.copies[3].event.source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD &&
           f.copies[4].event.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    cui_avatar_stack_activation retained = f.copies[4];
    cui_avatar_item temporary = f.items[0];
    f.items[0] = f.items[2];
    f.items[2] = temporary;
    warm(&f);
    assert(child(&f, 0) == targets[2] && child(&f, 2) != targets[0]);
    assert(cui_queue_accessibility_action(f.ui, child(&f, 0), CUI_ACCESSIBILITY_ACTIVATE));
    result = frame(&f, true, true);
    assert(result.control == 0 && result.action.activation_count == 1 && f.copies[5].key == 22 &&
           f.copies[5].event.activation.target == targets[2]);
    cui_destroy(f.ui);
    assert(retained.index == 1 && retained.key == 11 && retained.event.target == 100 &&
           retained.event.activation.target == targets[1]);
}
static void live_eligibility(void) {
    fixture f = make();
    warm(&f);
    click(&f, 0);
    click(&f, 2);
    f.consume_later = true;
    cui_avatar_stack_event_result result = frame(&f, true, true);
    assert(result.control == 0 && result.action.activation_count == 1 && f.calls == 1);
    f.consume_later = false;
    f.options.disabled = true;
    click(&f, 1);
    result = frame(&f, true, true);
    assert(result.action.submitted && !result.action.activation_count &&
           result.control == SIZE_MAX);
    f.options.disabled = false;
    warm(&f);
    f.options.width = 48;
    result = frame(&f, true, true);
    assert(result.control == SIZE_MAX && !result.action.activation_count && !child(&f, 0));
    size_t count;
    const cui_semantic *sem = cui_semantics(f.ui, &count);
    cui_id overflow = 0;
    for (size_t i = 0; i < count; i++)
        if (sem[i].name.length == 13 && !memcmp(sem[i].name.data, "5 more people", 13))
            overflow = sem[i].id;
    assert(overflow && !cui_queue_accessibility_action(f.ui, overflow, CUI_ACCESSIBILITY_ACTIVATE));
    cui_clear_error(f.ui);
    f.options.width = 0;
    warm(&f);
    for (unsigned mode = 0; mode < 2; mode++) {
        click(&f, 0);
        f.inert = mode == 0;
        f.layout = mode == 1;
        result = frame(&f, true, true);
        assert(result.action.valid && !result.action.submitted && !result.action.activation_count);
        f.inert = f.layout = false;
        warm(&f);
    }
    assert(f.calls == 1);
    cui_destroy(f.ui);
}
static void fallback(void) {
    fixture f = make();
    warm(&f);
    click(&f, 1);
    f.duplicate = true;
    cui_avatar_stack_event_result result = frame(&f, true, true);
    assert(cui_last_error(f.ui) == CUI_ERROR_DUPLICATE_ID);
    assert(result.control == 1 && result.action.activation_count == 1 && f.calls == 1);
    assert(f.copies[0].index == 1 && f.copies[0].key == 11 && f.copies[0].event.target == 100 &&
           !f.copies[0].event.activation.target);
    cui_destroy(f.ui);
}
static void compatibility(void) {
    fixture old = make(), added = make();
    warm(&old);
    warm(&added);
    click(&old, 2);
    click(&old, 0);
    click(&added, 2);
    click(&added, 0);
    frame(&old, false, false);
    cui_avatar_stack_event_result result = frame(&added, true, false);
    assert(old.control == 2 && result.control == 2 && result.action.activation_count == 2);
    assert(!added.calls && old.draw.count == added.draw.count);
    for (size_t i = 0; i < old.draw.count; i++) {
        const cui_draw_command *a = &old.draw.commands[i], *b = &added.draw.commands[i];
        assert(a->type == b->type && !memcmp(&a->bounds, &b->bounds, sizeof a->bounds));
        assert(!memcmp(&a->color, &b->color, sizeof a->color));
    }
    size_t count;
    (void)cui_frame_events(old.ui, &count);
    assert(count == 4);
    for (size_t i = 0; i < count; i++)
        assert(cui_event_consumed(old.ui, i) == cui_event_consumed(added.ui, i));
    cui_theme theme = cui_theme_light();
    result = cui_avatar_stack_events(added.ui, 100, &theme, added.items, 5, added.options,
                                     (cui_avatar_stack_handler){0});
    assert(result.action.valid && !result.action.submitted && result.control == SIZE_MAX);
    result = cui_avatar_stack_events(added.ui, 0, &theme, added.items, 5, added.options,
                                     (cui_avatar_stack_handler){0});
    assert(!result.action.valid && result.control == SIZE_MAX);
    cui_destroy(old.ui);
    cui_destroy(added.ui);
}
int main(void) {
    order();
    live_eligibility();
    compatibility();
    fallback();
    return 0;
}
