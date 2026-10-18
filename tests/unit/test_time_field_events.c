/* SPDX-License-Identifier: MIT */
#include "cui/cui_picker_events.h"
#include <assert.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_time time;
    cui_time_state state;
    cui_time_options options;
    bool styled, inert, layout, omit, request_omit;
    size_t calls;
    cui_time_change copy;
    cui_draw_list draw;
} fixture;
static bool equal(cui_time a, cui_time b) {
    return a.hour == b.hour && a.minute == b.minute && a.second == b.second;
}
static void observe(void *user, const cui_time_change *event) {
    fixture *f = user;
    assert(equal(event->value, f->time) && !equal(event->previous, event->value));
    assert(event->event.kind == CUI_COMPONENT_EVENT_CHANGE && event->event.target == 20);
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME && event->event.frame_sequence);
    assert(event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    size_t count;
    const cui_semantic *semantics = cui_semantics(f->ui, &count);
    bool found = false;
    for (size_t i = 0; i < count; ++i)
        if (semantics[i].id == cui_time_segment_id(20, 0)) {
            unsigned hour = f->time.hour;
            if (f->options.twelve_hour)
                hour = hour % 12 ? hour % 12 : 12;
            assert(semantics[i].value == (float)hour);
            found = true;
        }
    assert(found); /* Existing final semantics precede the observer. */
    f->copy = *event;
    ++f->calls;
    f->request_omit = true; /* Unrelated owner flag; apply only after outer return. */
}
static cui_picker_event_result frame(fixture *f, bool facade, bool handler) {
    assert(cui_begin_frame(f->ui, 600, 300, .016f));
    cui_begin_box(f->ui, 1000, (cui_box){.inert = f->inert, .layout_only = f->layout});
    cui_picker_event_result out = {0};
    cui_theme theme = cui_theme_light();
    if (!f->omit) {
        if (facade) {
            cui_time_handler h = {.on_change = handler ? observe : NULL, .user = f};
            out = f->styled ? cui_time_field_events(f->ui, 20, &theme, cui_string("Time"),
                                                     &f->state, &f->time, f->options, h)
                            : cui_base_time_field_events(f->ui, 20, cui_string("Time"),
                                                          &f->state, &f->time, f->options, h);
        } else
            out.changed = f->styled ? cui_time_field(f->ui, 20, &theme, cui_string("Time"),
                                                       &f->state, &f->time, f->options)
                                   : cui_base_time_field(f->ui, 20, cui_string("Time"),
                                                            &f->state, &f->time, f->options);
        if (!f->styled) {
            size_t count;
            const cui_semantic *semantics = cui_semantics(f->ui, &count);
            for (size_t i = 0; i < count; ++i) {
                cui_begin_box(f->ui, semantics[i].id, (cui_box){.width = 60, .height = 24});
                cui_end_box(f->ui);
            }
        }
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return out;
}
static cui_picker_event_result pair(fixture *a, fixture *b, bool handler) {
    cui_picker_event_result old = frame(a, false, false), now = frame(b, true, handler);
    assert(old.changed == now.changed && equal(a->time, b->time));
    assert(a->state.segment == b->state.segment && a->state.draft == b->state.draft &&
           a->state.digit_count == b->state.digit_count);
    assert(cui_focused(a->ui) == cui_focused(b->ui));
    assert(cui_needs_redraw(a->ui) == cui_needs_redraw(b->ui));
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a->ui, &na), *sb = cui_semantics(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i) {
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role && sa[i].value == sb[i].value);
        assert(sa[i].disabled == sb[i].disabled && sa[i].focused == sb[i].focused);
        assert(sa[i].tab_index == sb[i].tab_index && sa[i].tab_ordered == sb[i].tab_ordered &&
               sa[i].tab_stop_disabled == sb[i].tab_stop_disabled);
        assert(sa[i].details.readonly == sb[i].details.readonly &&
               sa[i].details.minimum == sb[i].details.minimum && sa[i].details.maximum == sb[i].details.maximum);
    }
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *x = a->draw.commands + i, *y = b->draw.commands + i;
        assert(x->type == y->type && !memcmp(&x->bounds, &y->bounds, sizeof x->bounds));
        assert(!memcmp(&x->color, &y->color, sizeof x->color) && x->text.length == y->text.length);
        if (x->text.length)
            assert(!memcmp(x->text.data, y->text.data, x->text.length));
    }
    return now;
}
static void focus(fixture *a, fixture *b, unsigned segment) {
    cui_focus(a->ui, cui_time_segment_id(20, segment));
    cui_focus(b->ui, cui_time_segment_id(20, segment));
}
static void send(fixture *a, fixture *b, cui_event event) {
    assert(cui_queue_event(a->ui, event));
    assert(cui_queue_event(b->ui, event));
}
static void key(fixture *a, fixture *b, cui_key value) {
    send(a, b, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = value});
}
static void exercise(bool styled) {
    fixture a = {.styled = styled, .time = {23, 59, 47}, .options = {.seconds = true}}, b = a;
    a.ui = cui_create((cui_config){.max_elements = 64, .max_commands = 128});
    b.ui = cui_create((cui_config){.max_elements = 64, .max_commands = 128});
    assert(a.ui && b.ui);
    cui_time_handler admitted = {.on_change = observe, .user = &b};
    assert(!cui_base_time_field_events(b.ui, 0, cui_string("Time"), &b.state, &b.time,
                                     b.options, admitted).changed);
    assert(!cui_base_time_field_events(NULL, 20, cui_string("Time"), &b.state, &b.time,
                                     b.options, admitted).change_count);
    assert(!cui_base_time_field_events(b.ui, 20, cui_string("Time"), NULL, &b.time,
                                     b.options, admitted).change_count);
    assert(!cui_base_time_field_events(b.ui, 20, cui_string("Time"), &b.state, NULL,
                                     b.options, admitted).change_count);
    assert(!cui_time_field_events(b.ui, 20, NULL, cui_string("Time"), &b.state, &b.time,
                                b.options, admitted).change_count);
    assert(!b.calls && b.time.hour == 23 && !b.state.segment);
    assert(!pair(&a, &b, true).change_count);
    cui_rect ra = cui_bounds(a.ui, cui_time_segment_id(20, 1));
    cui_rect rb = cui_bounds(b.ui, cui_time_segment_id(20, 1));
    assert(ra.w > 0 && rb.w > 0);
    assert(cui_queue_event(a.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
        .x = ra.x + ra.w / 2, .y = ra.y + ra.h / 2, .pointer_button = CUI_POINTER_PRIMARY}));
    assert(cui_queue_event(b.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
        .x = rb.x + rb.w / 2, .y = rb.y + rb.h / 2, .pointer_button = CUI_POINTER_PRIMARY}));
    assert(!pair(&a, &b, true).change_count && b.state.segment == 1);
    key(&a, &b, CUI_KEY_UP);
    assert(pair(&a, &b, true).change_count == 1 && b.time.minute == 0);
    a.time.minute = b.time.minute = 59; /* External assignments are the next baseline. */
    assert(!pair(&a, &b, true).change_count);
    focus(&a, &b, 0);
    key(&a, &b, CUI_KEY_UP);
    assert(pair(&a, &b, true).change_count == 1 && b.time.hour == 0);
    key(&a, &b, CUI_KEY_UP);
    key(&a, &b, CUI_KEY_DOWN);
    assert(!pair(&a, &b, true).change_count); /* Net-zero aggregate. */
    key(&a, &b, CUI_KEY_RIGHT);
    assert(!pair(&a, &b, true).change_count && b.state.segment == 1);
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "12"});
    assert(pair(&a, &b, true).change_count == 1 && b.time.minute == 12 && b.state.segment == 2);
    key(&a, &b, CUI_KEY_END);
    assert(pair(&a, &b, true).change_count == 1 && b.time.second == 59);
    key(&a, &b, CUI_KEY_HOME);
    assert(pair(&a, &b, true).change_count == 1 && b.time.second == 0);
    /* Invalid two-digit draft leaves minute unchanged but advances selection. */
    focus(&a, &b, 1);
    a.time.minute = b.time.minute = 9;
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "99"});
    assert(!pair(&a, &b, true).change_count && b.time.minute == 9);
    a.options.seconds = b.options.seconds = false;
    a.options.twelve_hour = b.options.twelve_hour = true;
    a.time.second = b.time.second = 47;
    focus(&a, &b, 3);
    send(&a, &b, (cui_event){.type = CUI_EVENT_TEXT, .text = "p"});
    assert(pair(&a, &b, true).change_count == 1 && b.time.hour == 12 && b.time.second == 47);
    focus(&a, &b, 0);
    key(&a, &b, CUI_KEY_HOME);
    assert(pair(&a, &b, true).change_count == 1 && b.time.hour == 13);
    key(&a, &b, CUI_KEY_END);
    assert(pair(&a, &b, true).change_count == 1 && b.time.hour == 12);
    key(&a, &b, CUI_KEY_DOWN);
    assert(!pair(&a, &b, false).change_count && b.time.hour == 11); /* Missing observer edits. */
    a.options.readonly = b.options.readonly = true;
    a.state.segment = b.state.segment = 9;
    a.state.digit_count = b.state.digit_count = 9;
    key(&a, &b, CUI_KEY_UP);
    assert(!pair(&a, &b, true).change_count && b.time.hour == 11 && !b.state.segment && !b.state.digit_count);
    a.options.readonly = b.options.readonly = false;
    a.options.disabled = b.options.disabled = true;
    key(&a, &b, CUI_KEY_UP);
    assert(!pair(&a, &b, true).change_count && !cui_focused(b.ui));
    a.options.disabled = b.options.disabled = false;
    focus(&a, &b, 0);
    a.inert = b.inert = true;
    key(&a, &b, CUI_KEY_UP);
    assert(!pair(&a, &b, true).change_count && b.time.hour == 11);
    a.inert = b.inert = false;
    a.layout = b.layout = true;
    key(&a, &b, CUI_KEY_UP);
    assert(!pair(&a, &b, true).change_count);
    a.layout = b.layout = false;
    /* Whole model validity includes hidden seconds; no repair or callback. */
    a.time.second = b.time.second = 60;
    a.state.segment = b.state.segment = 9;
    key(&a, &b, CUI_KEY_UP);
    assert(!pair(&a, &b, true).change_count && b.time.second == 60 && b.state.segment == 9);
    a.time = b.time = (cui_time){24, 0, 0};
    assert(!pair(&a, &b, true).change_count);
    a.time = b.time = (cui_time){9, 0, 0};
    a.state = b.state = (cui_time_state){0};
    pair(&a, &b, true);
    focus(&a, &b, 0);
    key(&a, &b, CUI_KEY_UP);
    assert(pair(&a, &b, true).change_count == 1);
    cui_time_change copy = b.copy;
    assert(b.request_omit);
    a.omit = b.omit = true;
    assert(!pair(&a, &b, true).change_count);
    cui_destroy(a.ui);
    cui_destroy(b.ui);
    assert(equal(copy.previous, (cui_time){9, 0, 0}) && equal(copy.value, (cui_time){10, 0, 0}));
}
int main(void) {
    exercise(false);
    exercise(true);
    return 0;
}
