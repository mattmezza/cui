/* SPDX-License-Identifier: MIT */
#include "cui/cui_toolbar_events.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_toolbar_command command;
    cui_toolbar_presentation presentation;
    cui_toolbar_command_event_result result;
    cui_component_event_metadata copy;
    size_t old_calls, new_calls, icons;
    bool legacy, handler, mutate, remove;
    cui_toolbar_size size;
    char trace[16];
    size_t n;
    cui_draw_list draw;
} fixture;
static void old(void *user) {
    fixture *f = user;
    f->old_calls++;
    f->trace[f->n++] = 'O';
    if (f->mutate)
        cui_focus(f->ui, 0);
}
static void changed(void *user, const cui_component_event_metadata *event) {
    fixture *f = user;
    assert(!f->command.on_activate || (f->n && f->trace[f->n - 1] == 'O'));
    assert(event->kind == CUI_COMPONENT_EVENT_ACTIVATE && event->target == 10);
    assert(event->flags & CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->frame_sequence == cui_frame_sequence(f->ui));
    f->copy = *event;
    f->new_calls++;
    f->trace[f->n++] = 'N';
    f->remove = true;
}
static void icon(cui_context *ui, float size, cui_color color, void *user) {
    fixture *f = user;
    (void)size;
    (void)color;
    f->icons++;
    if (f->mutate) {
        size_t count = 0;
        (void)cui_frame_events(ui, &count);
        for (size_t i = 0; i < count; i++)
            (void)cui_consume_event(ui, i);
    }
}
static void render(cui_context *ui, const cui_theme *theme, const cui_toolbar_presentation *p,
                   void *user) {
    fixture *f = user;
    f->presentation = *p;
    if (f->legacy)
        cui_toolbar_command_render(ui, theme, p, &f->command);
    else
        f->result = cui_toolbar_command_events(
            ui, theme, p, &f->command,
            (cui_component_activation_handler){f->handler ? changed : NULL, f});
}
static void init(fixture *f, bool legacy) {
    memset(f, 0, sizeof *f);
    f->legacy = legacy;
    f->handler = true;
    f->ui = cui_create(
        (cui_config){.width = 400, .height = 120, .max_elements = 128, .max_commands = 256});
    assert(f->ui);
    f->command = (cui_toolbar_command){.id = 10,
                                       .label = {"Save", 4},
                                       .icon = icon,
                                       .icon_user = f,
                                       .on_activate = old,
                                       .activate_user = f};
}
static void frame(fixture *f, unsigned passive, bool disabled_bar) {
    assert(cui_begin_frame(f->ui, 400, 120, .016f));
    cui_begin_box(f->ui, 200, (cui_box){.inert = passive == 1, .layout_only = passive == 2});
    cui_toolbar_item item = {.id = 11, .sized = true, .render = render, .user = f};
    cui_theme theme = cui_theme_light();
    assert(cui_toolbar(
        f->ui, 20, &theme, &item, 1,
        (cui_toolbar_options){.disabled = disabled_bar, .size = f->size, .custom_size = 37}));
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
}
static void key(fixture *f) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_ENTER}));
}
static void pointer(fixture *f, cui_pointer_type type) {
    cui_rect b = cui_bounds(f->ui, 10);
    assert(b.w > 0 && b.h > 0);
    cui_event e = {.type = CUI_EVENT_POINTER_DOWN,
                   .x = b.x + b.w / 2,
                   .y = b.y + b.h / 2,
                   .pointer_button = CUI_POINTER_PRIMARY,
                   .pointer_type = type,
                   .pointer_id = 4};
    assert(cui_queue_event(f->ui, e));
    e.type = CUI_EVENT_POINTER_UP;
    assert(cui_queue_event(f->ui, e));
}
static void compare(fixture *a, fixture *b) {
    assert(a->old_calls == b->old_calls && a->icons == b->icons &&
           cui_focused(a->ui) == cui_focused(b->ui));
    assert(!memcmp(&a->presentation.metrics, &b->presentation.metrics,
                   sizeof a->presentation.metrics));
    cui_rect x = cui_bounds(a->ui, 10), y = cui_bounds(b->ui, 10);
    assert(!memcmp(&x, &y, sizeof x));
    size_t an = 0, bn = 0;
    const cui_semantic *as = cui_semantics(a->ui, &an), *bs = cui_semantics(b->ui, &bn);
    assert(an == bn);
    for (size_t i = 0; i < an; i++)
        assert(as[i].id == bs[i].id && as[i].role == bs[i].role &&
               as[i].disabled == bs[i].disabled);
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; i++) {
        assert(a->draw.commands[i].type == b->draw.commands[i].type);
        assert(!memcmp(&a->draw.commands[i].color, &b->draw.commands[i].color, sizeof(cui_color)));
        assert(a->draw.commands[i].font_size == b->draw.commands[i].font_size);
        assert(a->draw.commands[i].text.length == b->draw.commands[i].text.length);
        if (a->draw.commands[i].text.length)
            assert(!memcmp(a->draw.commands[i].text.data, b->draw.commands[i].text.data,
                           a->draw.commands[i].text.length));
        assert(!memcmp(&a->draw.commands[i].bounds, &b->draw.commands[i].bounds, sizeof(cui_rect)));
    }
}
int main(void) {
    for (unsigned input = 0; input < 5; input++) {
        fixture a, b;
        init(&a, true);
        init(&b, false);
        frame(&a, 0, false);
        frame(&b, 0, false);
        a.mutate = b.mutate = true;
        cui_focus(a.ui, 10);
        cui_focus(b.ui, 10);
        if (input < 2) {
            pointer(&a, input ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE);
            pointer(&b, input ? CUI_POINTER_TOUCH : CUI_POINTER_MOUSE);
        } else if (input == 2) {
            assert(cui_queue_event(
                a.ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                  .accessibility_target = 10,
                                  .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
            assert(cui_queue_event(
                b.ui, (cui_event){.type = CUI_EVENT_ACCESSIBILITY_ACTION,
                                  .accessibility_target = 10,
                                  .accessibility_action = CUI_ACCESSIBILITY_ACTIVATE}));
        } else {
            key(&a);
            key(&b);
            if (input == 4) {
                key(&a);
                key(&b);
            }
        }
        frame(&a, 0, true);
        frame(&b, 0, true);
        compare(&a, &b);
        assert(b.result.valid && b.result.submitted && b.result.activation_count == 1 &&
               b.new_calls == 1 && b.old_calls == 1 && b.remove);
        assert(b.n == 2 && !memcmp(b.trace, "ON", 2));
        if (input == 4)
            assert(b.copy.flags == CUI_COMPONENT_EVENT_HAS_FRAME &&
                   b.copy.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
        else
            assert((b.copy.flags & CUI_COMPONENT_EVENT_HAS_ACTIVATION) &&
                   b.copy.activation.target == 10);
        if (input < 4) {
            cui_component_event_source source = input < 2 ? CUI_COMPONENT_EVENT_SOURCE_POINTER
                : input == 2 ? CUI_COMPONENT_EVENT_SOURCE_ACCESSIBILITY
                             : CUI_COMPONENT_EVENT_SOURCE_KEYBOARD;
            assert(b.copy.source == source && (b.copy.flags & CUI_COMPONENT_EVENT_HAS_INDEX));
            assert(b.copy.activation.event_index == b.copy.event_index);
            if (input == 1)
                assert(b.copy.activation.release.type == CUI_POINTER_TOUCH);
        }
        cui_destroy(a.ui);
        cui_destroy(b.ui);
        assert(b.copy.target == 10);
    }
    for (unsigned policy = 0; policy < 5; policy++) {
        fixture a, b;
        init(&a, true);
        init(&b, false);
        frame(&a, 0, false);
        frame(&b, 0, false);
        cui_focus(a.ui, 10);
        cui_focus(b.ui, 10);
        key(&a);
        key(&b);
        if (policy == 0)
            a.command.disabled = b.command.disabled = true;
        if (policy == 3)
            b.handler = false;
        if (policy == 4)
            a.command.selected = b.command.selected = true;
        unsigned passive = policy == 1 ? 1 : policy == 2 ? 2 : 0;
        frame(&a, passive, false);
        frame(&b, passive, false);
        compare(&a, &b);
        assert(b.result.valid && b.result.submitted == (passive == 0));
        assert(b.new_calls == (policy == 4 ? 1u : 0u));
        cui_destroy(a.ui);
        cui_destroy(b.ui);
    }
    for (unsigned size = 0; size <= CUI_TOOLBAR_CUSTOM; size++) {
        fixture a, b;
        init(&a, true);
        init(&b, false);
        a.size = b.size = (cui_toolbar_size)size;
        frame(&a, 0, false);
        frame(&b, 0, false);
        compare(&a, &b);
        cui_focus(a.ui, 10);
        cui_focus(b.ui, 10);
        key(&a);
        key(&b);
        frame(&a, 0, false);
        frame(&b, 0, false);
        compare(&a, &b);
        assert(b.new_calls == 1);
        cui_destroy(a.ui);
        cui_destroy(b.ui);
    }
    fixture f;
    init(&f, false);
    cui_theme theme = cui_theme_light();
    cui_toolbar_presentation p = {0};
    assert(!cui_toolbar_command_events(NULL, &theme, &p, &f.command,
                                       (cui_component_activation_handler){0})
                .valid);
    f.command.label = (cui_str){0};
    assert(!cui_toolbar_command_events(f.ui, &theme, &p, &f.command,
                                       (cui_component_activation_handler){0})
                .valid);
    f.command.label = cui_string("Save");
    cui_toolbar_command_event_result outside = cui_toolbar_command_events(
        f.ui, &theme, &p, &f.command, (cui_component_activation_handler){0});
    assert(outside.valid && !outside.submitted && !outside.activation_count);
    cui_destroy(f.ui);
    /* An absent legacy callback does not disable interaction; selected is paint only. */
    init(&f, false);
    f.command.on_activate = NULL;
    f.command.selected = true;
    frame(&f, 0, false);
    assert(!f.new_calls);
    cui_focus(f.ui, 10);
    key(&f);
    frame(&f, 0, false);
    assert(f.new_calls == 1 && !f.old_calls && f.command.selected);
    cui_destroy(f.ui);
    /* Cancelled touch never becomes aggregate activation. */
    init(&f, false);
    frame(&f, 0, false);
    cui_rect bounds = cui_bounds(f.ui, 10);
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                             .x = bounds.x + 2,
                                             .y = bounds.y + 2,
                                             .pointer_type = CUI_POINTER_TOUCH,
                                             .pointer_id = 4,
                                             .pointer_button = CUI_POINTER_PRIMARY}));
    assert(cui_queue_event(f.ui, (cui_event){.type = CUI_EVENT_CANCEL}));
    frame(&f, 0, false);
    assert(!f.old_calls && !f.new_calls);
    cui_destroy(f.ui);
    return 0;
}
