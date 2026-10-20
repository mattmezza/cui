/* SPDX-License-Identifier: MIT */
#include "cui/cui_avatar_events.h"
#include <assert.h>
#include <string.h>

typedef struct {
    cui_context *ui;
    cui_avatar_options options;
    cui_component_event_metadata copies[16];
    size_t calls;
    bool consume_later, omit, inert, layout, preconsume, duplicate;
    bool legacy;
    cui_draw_list draw;
} fixture;
static void observed(void *user, const cui_component_event_metadata *event) {
    fixture *f = user;
    assert(event->kind == CUI_COMPONENT_EVENT_ACTIVATE && event->target == 20);
    assert(event->flags & CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->frame_sequence && f->calls < 16);
    if (event->flags & CUI_COMPONENT_EVENT_HAS_INDEX)
        assert(cui_event_consumed(f->ui, event->event_index));
    f->copies[f->calls++] = *event;
    f->omit = true; /* Caller may omit a later sibling after this operation returns. */
    if (f->consume_later) {
        size_t count;
        (void)cui_frame_events(f->ui, &count);
        for (size_t i = event->event_index + 1; i < count; i++)
            assert(cui_consume_event(f->ui, i));
    }
}
static fixture make(void) {
    fixture f = {0};
    f.ui = cui_create((cui_config){.width = 200, .height = 100});
    f.options = (cui_avatar_options){.size = 48, .interactive = true};
    assert(f.ui);
    return f;
}
static cui_component_action_result frame(fixture *f, bool facade, bool handler) {
    assert(cui_begin_frame(f->ui, 200, 100, .016f));
    cui_begin_box(f->ui, 1000, (cui_box){.inert = f->inert, .layout_only = f->layout});
    if (f->preconsume) {
        size_t count;
        (void)cui_frame_events(f->ui, &count);
        for (size_t i = 0; i < count; i++)
            assert(cui_consume_event(f->ui, i));
    }
    if (f->duplicate)
        cui_interact(f->ui, 20, CUI_ROLE_BUTTON, cui_string("Duplicate"), false, false, 0);
    cui_theme theme = cui_theme_light();
    cui_component_action_result result = {0};
    if (facade)
        result =
            cui_avatar_events(f->ui, 20, &theme, cui_string("Alex Morgan"), f->options,
                              (cui_component_activation_handler){handler ? observed : NULL, f});
    else
        f->legacy = cui_avatar_ex(f->ui, 20, &theme, cui_string("Alex Morgan"), f->options);
    if (!f->omit)
        cui_avatar_ex(f->ui, 21, &theme, cui_string("Later sibling"),
                      (cui_avatar_options){.size = 24, .interactive = true});
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    return result;
}
static void warm(fixture *f) {
    frame(f, true, false);
    frame(f, true, false);
}
static void pointer(fixture *f, cui_pointer_type type) {
    cui_rect b = cui_bounds(f->ui, 20);
    cui_event event = {.type = CUI_EVENT_POINTER_DOWN,
                       .x = b.x + b.w / 2,
                       .y = b.y + b.h / 2,
                       .pointer_type = type,
                       .pointer_id = 9,
                       .pointer_device = 7};
    assert(cui_queue_event(f->ui, event));
    event.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(f->ui, event));
}
static void at(fixture *f) {
    assert(cui_queue_accessibility_action(f->ui, 20, CUI_ACCESSIBILITY_ACTIVATE));
}
static void sources(void) {
    fixture f = make();
    assert(!cui_queue_accessibility_action(f.ui, 20, CUI_ACCESSIBILITY_ACTIVATE));
    cui_clear_error(f.ui);
    warm(&f);
    pointer(&f, CUI_POINTER_MOUSE);
    pointer(&f, CUI_POINTER_TOUCH);
    at(&f);
    cui_component_action_result result = frame(&f, true, true);
    assert(result.valid && result.submitted && result.activation_count == 3 &&
           !result.request_count && !result.count_overflow && f.calls == 3);
    assert(f.copies[0].source == CUI_COMPONENT_EVENT_SOURCE_POINTER);
    assert(f.copies[1].activation.release.type == CUI_POINTER_TOUCH);
    assert(f.copies[1].activation.release.contact == 9 &&
           f.copies[1].activation.release.device == 7);
    assert(f.copies[2].source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY);
    for (size_t i = 0; i < f.calls; i++) {
        assert(f.copies[i].flags == 15 && f.copies[i].activation.target == 20);
        assert(f.copies[i].event_index == f.copies[i].activation.event_index);
        if (i)
            assert(f.copies[i - 1].event_index < f.copies[i].event_index);
    }
    size_t count;
    const cui_semantic *semantics = cui_semantics(f.ui, &count);
    for (size_t i = 0; i < count; i++)
        assert(semantics[i].id != 21); /* Observer requested deferred sibling omission. */
    cui_component_event_metadata copy = f.copies[2];
    cui_destroy(f.ui);
    assert(copy.source == CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY && copy.activation.target == 20);
    assert(result.activation_count == 3); /* Passive result copy does not dispatch. */
}
static void keyboard(void) {
    fixture f = make();
    warm(&f);
    cui_focus(f.ui, 20);
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
    assert(frame(&f, true, true).activation_count == 1);
    assert(f.copies[0].activation.event_type == CUI_EVENT_KEY_DOWN);
    assert(cui_queue_event(
        f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER, .repeat = true}));
    assert(frame(&f, true, true).activation_count == 0);
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_SPACE}));
    assert(frame(&f, true, true).activation_count == 0);
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_KEY_UP, .key = CUI_KEY_SPACE}));
    assert(frame(&f, true, true).activation_count == 1);
    assert(f.copies[1].source == CUI_COMPONENT_EVENT_SOURCE_KEYBOARD &&
           f.copies[1].activation.event_type == CUI_EVENT_KEY_UP);
    cui_destroy(f.ui);
}
static void eligibility(void) {
    fixture f = make();
    warm(&f);
    at(&f);
    at(&f);
    f.consume_later = true;
    assert(frame(&f, true, true).activation_count == 1 && f.calls == 1);
    assert(f.copies[0].flags == 15); /* AT evidence survived original consumption. */
    f.consume_later = false;
    f.preconsume = true;
    at(&f);
    assert(frame(&f, true, true).activation_count == 0 && f.calls == 1);
    f.preconsume = false;
    cui_rect bounds = cui_bounds(f.ui, 20);
    cui_event touch = {.type = CUI_EVENT_POINTER_DOWN,
                       .x = bounds.x + 10,
                       .y = bounds.y + 10,
                       .pointer_type = CUI_POINTER_TOUCH,
                       .pointer_id = 9,
                       .pointer_device = 7};
    assert(cui_queue_event(f.ui, touch));
    touch.type = CUI_EVENT_CANCEL;
    assert(cui_queue_event(f.ui, touch));
    touch.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(f.ui, touch));
    assert(frame(&f, true, true).activation_count == 0 && f.calls == 1);
    f.options.disabled = true;
    pointer(&f, CUI_POINTER_MOUSE);
    cui_component_action_result result = frame(&f, true, true);
    assert(result.valid && result.submitted && !result.activation_count && f.calls == 1);
    f.options.disabled = false;
    f.options.interactive = false;
    result = frame(&f, true, true);
    assert(result.submitted && !result.activation_count);
    assert(!cui_queue_accessibility_action(f.ui, 20, CUI_ACCESSIBILITY_ACTIVATE));
    cui_clear_error(f.ui);
    f.options.interactive = true;
    warm(&f);
    for (unsigned mode = 0; mode < 2; mode++) {
        at(&f);
        f.inert = mode == 0;
        f.layout = mode == 1;
        result = frame(&f, true, true);
        assert(result.valid && !result.submitted && !result.activation_count && f.calls == 1);
        f.inert = f.layout = false;
        warm(&f);
    }
    cui_theme theme = cui_theme_light();
    result = cui_avatar_events(f.ui, 20, &theme, cui_string("Alex"), f.options,
                               (cui_component_activation_handler){observed, &f});
    assert(result.valid && !result.submitted && !result.activation_count);
    result = cui_avatar_events(f.ui, 0, &theme, cui_string("Alex"), f.options,
                               (cui_component_activation_handler){observed, &f});
    assert(!result.valid && !result.submitted);
    cui_destroy(f.ui);
}
static void fallback(void) {
    fixture f = make();
    warm(&f);
    pointer(&f, CUI_POINTER_MOUSE);
    /* A real legacy gate match with a duplicate-semantic error: Core rejects
     * accepted-evidence queries, so no cause/index may be invented. */
    f.duplicate = true;
    cui_component_action_result result = frame(&f, true, true);
    assert(cui_last_error(f.ui) == CUI_ERROR_DUPLICATE_ID);
    assert(result.valid && result.submitted && result.activation_count == 1 && f.calls == 1);
    assert(f.copies[0].flags == CUI_COMPONENT_EVENT_HAS_FRAME &&
           f.copies[0].source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    cui_destroy(f.ui);
}
static void compatibility(void) {
    fixture old = make(), added = make();
    warm(&old);
    warm(&added);
    pointer(&old, CUI_POINTER_MOUSE);
    pointer(&old, CUI_POINTER_MOUSE);
    pointer(&added, CUI_POINTER_MOUSE);
    pointer(&added, CUI_POINTER_MOUSE);
    frame(&old, false, false);
    cui_component_action_result result = frame(&added, true, false);
    assert(old.legacy && result.activation_count == 2 && !added.calls);
    assert(old.draw.count == added.draw.count);
    for (size_t i = 0; i < old.draw.count; i++) {
        const cui_draw_command *a = &old.draw.commands[i], *b = &added.draw.commands[i];
        assert(a->type == b->type && !memcmp(&a->bounds, &b->bounds, sizeof a->bounds));
        assert(!memcmp(&a->color, &b->color, sizeof a->color));
        assert(a->font_size == b->font_size && a->text.length == b->text.length);
        if (a->text.length)
            assert(!memcmp(a->text.data, b->text.data, a->text.length));
    }
    size_t count;
    (void)cui_frame_events(old.ui, &count);
    assert(count == 4);
    for (size_t i = 0; i < count; i++)
        assert(cui_event_consumed(old.ui, i) == cui_event_consumed(added.ui, i));
    assert(cui_focused(old.ui) == cui_focused(added.ui));
    size_t old_count, added_count;
    const cui_semantic *old_sem = cui_semantics(old.ui, &old_count);
    const cui_semantic *added_sem = cui_semantics(added.ui, &added_count);
    assert(old_count == added_count && old_count == 2);
    for (size_t i = 0; i < old_count; i++) {
        assert(old_sem[i].id == added_sem[i].id && old_sem[i].role == added_sem[i].role);
        assert(old_sem[i].disabled == added_sem[i].disabled &&
               old_sem[i].details.pointer_activatable == added_sem[i].details.pointer_activatable &&
               old_sem[i].details.focusable == added_sem[i].details.focusable);
        assert(old_sem[i].name.length == added_sem[i].name.length &&
               !memcmp(old_sem[i].name.data, added_sem[i].name.data, old_sem[i].name.length));
    }
    size_t saturated = SIZE_MAX;
    bool overflow = false;
    cui_control_events_private_increment(&saturated, &overflow);
    assert(saturated == SIZE_MAX && overflow);
    cui_destroy(old.ui);
    cui_destroy(added.ui);
}
int main(void) {
    sources();
    keyboard();
    eligibility();
    fallback();
    compatibility();
    return 0;
}
