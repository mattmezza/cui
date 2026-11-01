/* SPDX-License-Identifier: MIT */
#include "cui/cui_software.h"
#if defined(CUI_TIME_FIELD_DISTRIBUTION)
#define CUI_TIME_FIELD_V1_IMPLEMENTATION
#if defined(CUI_TIME_FIELD_UMBRELLA)
#include "cui_all.h"
#else
#include "cui_time_field_v1.h"
#endif
#else
#include "cui/cui_time_field_v1.h"
#endif
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    cui_context *ui;
    cui_theme theme;
    cui_time time;
    cui_time_field_options_v1 options;
    cui_time_field_state_v1 state;
    cui_draw_list draw;
    size_t calls, measurements;
    bool styled, inert, layout, omit, handler, modal;
    cui_time_change copy;
    cui_id raw_targets[16];
    size_t raw_count;
    uint32_t consume_mask;
} fixture;
static bool same(cui_time a, cui_time b) {
    return a.hour == b.hour && a.minute == b.minute && a.second == b.second;
}
static cui_size measure(void *user, cui_str text, float font) {
    fixture *f = user;
    f->measurements++;
    float width = 0;
    for (size_t i = 0; i < text.length; i++)
        if (((unsigned char)text.data[i] & 0xc0) != 0x80)
            width += font * (text.data[i] == '1' ? .4f : .6f);
    return (cui_size){width, font};
}
static void observe(void *user, const cui_time_change *event) {
    fixture *f = user;
    assert(same(event->value, f->time));
    assert(!same(event->previous, event->value));
    assert(event->event.target == 20 && event->event.kind == CUI_COMPONENT_EVENT_CHANGE);
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME && event->event.frame_sequence);
    size_t n;
    const cui_semantic *semantics = cui_semantics(f->ui, &n);
    bool found = false;
    for (size_t i = 0; i < n; i++)
        if (semantics[i].id == cui_time_segment_id(20, 1)) {
            found = true;
            assert(semantics[i].value == f->time.minute);
        }
    assert(found);
    if (f->styled)
        assert(f->measurements);
    f->calls++;
    f->copy = *event;
}
static void init(fixture *f, bool styled) {
    memset(f, 0, sizeof(*f));
    f->styled = styled;
    f->handler = true;
    f->theme = cui_theme_light();
    f->time = (cui_time){9, 30, 0};
    assert(cui_time_field_options_init_v1(&f->options) == 0);
    assert(cui_time_field_state_init_v1(&f->state, f->time, &f->options) == 0);
    f->ui = cui_create(
        (cui_config){.width = 1200, .height = 400, .measure = measure, .measure_user = f});
    assert(f->ui);
}
static void neighbor(cui_context *ui, cui_id id) {
    cui_interact(ui, id, CUI_ROLE_BUTTON, cui_string("Neighbor"), false, false, 0);
    cui_begin_box(ui, id, (cui_box){.width = 50, .height = 30});
    cui_end_box(ui);
}
static cui_time_field_result_v1 frame(fixture *f) {
    assert(cui_begin_frame(f->ui, 1200, 400, .016f));
    size_t event_count;
    (void)cui_frame_events(f->ui, &event_count);
    assert(event_count <= 16);
    f->raw_count = event_count;
    for (size_t i = 0; i < event_count; ++i) {
        f->raw_targets[i] = cui_event_target(f->ui, i);
        if (f->consume_mask & (UINT32_C(1) << i))
            assert(cui_consume_event(f->ui, i));
    }
    cui_begin_box(f->ui, 1000, (cui_box){.inert = f->inert, .layout_only = f->layout});
    neighbor(f->ui, 10);
    if (f->modal) {
        assert(cui_begin_focus_scope(f->ui, 900, true));
        neighbor(f->ui, 900);
        cui_end_focus_scope(f->ui);
    }
    cui_time_field_result_v1 r = {0};
    if (!f->omit) {
        cui_time_handler h = {.on_change = f->handler ? observe : NULL, .user = f};
        r = f->styled ? cui_time_field_v1(f->ui, 20, &f->theme, cui_string("Time"), &f->state,
                                          &f->time, &f->options, h)
                      : cui_base_time_field_v1(f->ui, 20, cui_string("Time"), &f->state, &f->time,
                                               &f->options, h);
        assert(r.status == CUI_TIME_TEXT_OK);
        if (!f->styled) {
            cui_begin_box(f->ui, 20, (cui_box){.row = true});
            for (unsigned i = 0; i < 4; i++)
                if (i < 2 || (i == 2 && f->options.text.precision) ||
                    (i == 3 && f->options.text.cycle)) {
                    cui_begin_box(f->ui, cui_time_segment_id(20, i),
                                  (cui_box){.width = 60, .height = 30});
                    cui_end_box(f->ui);
                }
            cui_end_box(f->ui);
        }
    }
    /* The Core target accessor deliberately returns0 under current inert
     * ancestry. Outside that policy, routing evidence remains unchanged. */
    if (!f->inert && !f->layout)
        for (size_t i = 0; i < event_count; ++i)
            assert(f->raw_targets[i] == cui_event_target(f->ui, i));
    neighbor(f->ui, 30);
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static void focus(fixture *f, unsigned segment) {
    cui_focus(f->ui, cui_time_segment_id(20, segment));
}
static void key(fixture *f, cui_key k, unsigned modifiers) {
    assert(cui_queue_event(
        f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = k, .modifiers = modifiers}));
}
static void text(fixture *f, cui_str value) {
    assert(cui_queue_event(
        f->ui,
        (cui_event){.type = CUI_EVENT_TEXT, .text_data = value.data, .text_length = value.length}));
}
static void warm(fixture *f) {
    frame(f);
    frame(f);
}
static void editing(bool styled) {
    fixture f;
    init(&f, styled);
    warm(&f);
    focus(&f, 1);
    f.measurements = 0;
    f.theme.font_size = 17; /* New font key proves Styled measurements precede callback. */
    text(&f, cui_string("7"));
    cui_time_field_result_v1 r = frame(&f);
    assert(r.changed && r.change_count == 1 && f.time.minute == 7 &&
           f.state.segment == 1); /* last visible stops */
    text(&f, cui_string("2"));
    frame(&f);
    assert(f.time.minute == 2 && f.state.digit_count == 1);
    text(&f, cui_string("9"));
    frame(&f);
    assert(f.time.minute == 29 && f.state.digit_count == 0);
    focus(&f, 0);
    text(&f, cui_string("2"));
    frame(&f);
    text(&f, cui_string("9"));
    frame(&f);
    assert(f.time.hour == 9 && f.state.segment == 1); /* invalid29 restarts9, completes */
    f.handler = false;
    focus(&f, 1);
    key(&f, CUI_KEY_UP, 0);
    r = frame(&f);
    assert(r.changed && r.change_count == 0);
    f.handler = true;
    size_t calls = f.calls;
    key(&f, CUI_KEY_UP, 0);
    key(&f, CUI_KEY_DOWN, 0);
    r = frame(&f);
    assert(!r.changed && !r.change_count && f.calls == calls);
    f.options.text.cycle = 1;
    f.time = (cui_time){23, 30, 0};
    frame(&f);
    focus(&f, 0);
    key(&f, CUI_KEY_UP, 0);
    frame(&f);
    assert(f.time.hour == 12);
    key(&f, CUI_KEY_DELETE, 0);
    frame(&f);
    assert(f.time.hour == 12);
    focus(&f, 3);
    text(&f, cui_string("a"));
    frame(&f);
    assert(f.time.hour == 0);
    f.options.flags = CUI_TIME_FIELD_READONLY_V1;
    key(&f, CUI_KEY_UP, 0);
    r = frame(&f);
    assert(!r.changed && f.time.hour == 0);
    f.options.flags = 0;
    frame(&f);
    frame(&f);
    focus(&f, 1);
    text(&f, cui_string("1"));
    frame(&f);
    assert(f.state.digit_count == 1);
    f.time = (cui_time){10, 42, 0};
    frame(&f);
    assert(f.state.digit_count == 0);
    f.omit = true;
    frame(&f);
    f.omit = false;
    text(&f, cui_string("5"));
    r = frame(&f);
    assert(!r.changed && f.time.minute == 42);
    cui_time_change saved = f.copy;
    cui_destroy(f.ui);
    assert(saved.event.target == 20);
}
static void original_order(bool styled) {
    fixture split, joined;
    init(&split, styled);
    init(&joined, styled);
    warm(&split);
    warm(&joined);
    focus(&split, 0);
    focus(&joined, 0);
    const char *digits[] = {"1", "2", "3", "4"};
    for (size_t i = 0; i < 4; ++i)
        text(&split, cui_string(digits[i]));
    text(&joined, cui_string("1234"));
    cui_time_field_result_v1 a = frame(&split), b = frame(&joined);
    assert(split.raw_count == 4 && joined.raw_count == 1);
    for (size_t i = 0; i < 4; ++i)
        assert(split.raw_targets[i] == cui_time_segment_id(20, 0));
    assert(joined.raw_targets[0] == cui_time_segment_id(20, 0));
    assert(same(split.time, (cui_time){12, 34, 0}) && same(split.time, joined.time));
    assert(a.changed && b.changed && a.change_count == 1 && b.change_count == 1);
    assert(!a.rejected_count && !b.rejected_count);
    assert(split.state.segment == 1 && joined.state.segment == 1);
    assert(cui_focused(split.ui) == cui_time_segment_id(20, 1));
    /* Foreign TEXT between genuine focus actions must not reset the draft. */
    focus(&split, 0);
    size_t calls = split.calls;
    text(&split, cui_string("1"));
    assert(cui_queue_accessibility_action(split.ui, 10, CUI_ACCESSIBILITY_FOCUS));
    text(&split, cui_string("9"));
    assert(cui_queue_accessibility_action(split.ui, cui_time_segment_id(20, 0),
                                          CUI_ACCESSIBILITY_FOCUS));
    text(&split, cui_string("2"));
    a = frame(&split);
    assert(split.raw_count == 5 && split.raw_targets[2] == 10);
    assert(split.raw_targets[0] == cui_time_segment_id(20, 0) &&
           split.raw_targets[4] == split.raw_targets[0]);
    assert(same(split.time, (cui_time){12, 34, 0}) && !a.changed && !a.change_count &&
           split.calls == calls);
    /* Consumed TEXT at another visible segment also leaves last target intact. */
    focus(&split, 0);
    text(&split, cui_string("1"));
    assert(cui_queue_accessibility_action(split.ui, cui_time_segment_id(20, 1),
                                          CUI_ACCESSIBILITY_FOCUS));
    text(&split, cui_string("9"));
    assert(cui_queue_accessibility_action(split.ui, cui_time_segment_id(20, 0),
                                          CUI_ACCESSIBILITY_FOCUS));
    text(&split, cui_string("2"));
    split.consume_mask = UINT32_C(1) << 2;
    a = frame(&split);
    split.consume_mask = 0;
    assert(split.raw_targets[2] == cui_time_segment_id(20, 1));
    assert(same(split.time, (cui_time){12, 34, 0}) && !a.changed && !a.change_count &&
           split.calls == calls);
    focus(&split, 0);
    text(&split, cui_string("1"));
    key(&split, CUI_KEY_TAB, 0);
    text(&split, cui_string("2"));
    frame(&split);
    assert(split.raw_count == 3 && split.raw_targets[0] == cui_time_segment_id(20, 0));
    assert(split.raw_targets[1] == cui_time_segment_id(20, 0));
    assert(split.raw_targets[2] == cui_time_segment_id(20, 1));
    assert(same(split.time, (cui_time){1, 2, 0}) && split.state.segment == 1);
    focus(&joined, 0);
    cui_rect minute = cui_bounds(joined.ui, cui_time_segment_id(20, 1));
    text(&joined, cui_string("1"));
    assert(cui_queue_event(joined.ui, (cui_event){.type = CUI_EVENT_POINTER_DOWN,
                                                  .x = minute.x + minute.w * .5f,
                                                  .y = minute.y + minute.h * .5f}));
    text(&joined, cui_string("2"));
    frame(&joined);
    assert(joined.raw_count == 3 && joined.raw_targets[0] == cui_time_segment_id(20, 0));
    assert(joined.raw_targets[1] == cui_time_segment_id(20, 1));
    assert(joined.raw_targets[2] == cui_time_segment_id(20, 1));
    assert(same(joined.time, (cui_time){1, 2, 0}) && joined.state.segment == 1);
    cui_destroy(split.ui);
    cui_destroy(joined.ui);
}
static void focus_transition(bool styled, bool accessibility) {
    fixture f;
    init(&f, styled);
    warm(&f);
    focus(&f, 1);
    text(&f, cui_string("1"));
    frame(&f);
    assert(f.time.minute == 1 && f.state.digit_count == 1 && f.state.segment == 1);
    if (accessibility)
        assert(cui_queue_accessibility_action(f.ui, cui_time_segment_id(20, 0),
                                              CUI_ACCESSIBILITY_FOCUS));
    else
        focus(&f, 0);
    text(&f, cui_string("2"));
    frame(&f);
    assert(same(f.time, (cui_time){2, 1, 0}) && f.state.digit_count == 1 && f.state.segment == 0);
    assert(f.raw_targets[f.raw_count - 1] == cui_time_segment_id(20, 0));
    cui_destroy(f.ui);

    init(&f, styled);
    cui_time_locale_v1 locale;
    cui_time_locale_init_v1(&locale);
    locale.digits[1] = (cui_str){"١", 2};
    f.options.text.locale = &locale;
    f.options.edit_generation++;
    cui_time_field_state_init_v1(&f.state, f.time, &f.options);
    warm(&f);
    focus(&f, 1);
    text(&f, (cui_str){"\xd9", 1});
    frame(&f);
    assert(f.state.pending_length == 1 && f.state.segment == 1);
    if (accessibility)
        assert(cui_queue_accessibility_action(f.ui, cui_time_segment_id(20, 0),
                                              CUI_ACCESSIBILITY_FOCUS));
    else
        focus(&f, 0);
    text(&f, (cui_str){"\xa1", 1});
    cui_time_field_result_v1 r = frame(&f);
    assert(!r.changed && r.rejected_count == 1 && r.validation == CUI_TIME_TEXT_INVALID_SYNTAX);
    assert(same(f.time, (cui_time){9, 30, 0}) && f.state.pending_length == 0);
    text(&f, cui_string("2"));
    frame(&f);
    assert(f.time.hour == 2);
    cui_destroy(f.ui);

    /* A transient host focus excursion that ends on the original segment does
     * not constitute a different active editor at the next reduction. */
    init(&f, styled);
    warm(&f);
    focus(&f, 1);
    text(&f, cui_string("1"));
    frame(&f);
    cui_focus(f.ui, 10);
    focus(&f, 1);
    text(&f, cui_string("2"));
    frame(&f);
    assert(f.time.minute == 12 && f.state.digit_count == 0);
    cui_destroy(f.ui);
}
static void tab_exit_draft(bool styled, bool prefix) {
    fixture f;
    init(&f, styled);
    cui_time_locale_v1 locale;
    if (prefix) {
        cui_time_locale_init_v1(&locale);
        locale.digits[1] = (cui_str){"١", 2};
        f.options.text.locale = &locale;
        f.options.edit_generation++;
        cui_time_field_state_init_v1(&f.state, f.time, &f.options);
    }
    warm(&f);
    focus(&f, 1);
    text(&f, prefix ? (cui_str){"\xd9", 1} : cui_string("1"));
    key(&f, CUI_KEY_TAB, 0);
    frame(&f);
    assert(f.raw_count == 2 && f.raw_targets[0] == cui_time_segment_id(20, 1));
    assert(f.raw_targets[1] == cui_time_segment_id(20, 1));
    assert(cui_focused(f.ui) == 30 && f.state.digit_count == 0 && f.state.pending_length == 0);
    cui_destroy(f.ui);
}
static void tabs(bool styled) {
    fixture f;
    init(&f, styled);
    f.options.text.precision = 1;
    f.options.text.cycle = 1;
    cui_time_field_state_init_v1(&f.state, f.time, &f.options);
    warm(&f);
    cui_focus(f.ui, 10);
    const unsigned order[] = {0, 1, 2, 3};
    for (size_t i = 0; i < 4; i++) {
        key(&f, CUI_KEY_TAB, 0);
        frame(&f);
        assert(cui_focused(f.ui) == cui_time_segment_id(20, order[i]));
        assert(f.state.segment == order[i]);
    }
    key(&f, CUI_KEY_TAB, 0);
    frame(&f);
    assert(cui_focused(f.ui) == 30);
    for (unsigned i = 4; i > 0; i--) {
        key(&f, CUI_KEY_TAB, CUI_MOD_SHIFT);
        frame(&f);
        assert(cui_focused(f.ui) == cui_time_segment_id(20, i - 1));
        assert(f.state.segment == i - 1);
    }
    key(&f, CUI_KEY_TAB, CUI_MOD_SHIFT);
    frame(&f);
    assert(cui_focused(f.ui) == 10);
    key(&f, CUI_KEY_TAB, 0);
    key(&f, CUI_KEY_TAB, 0);
    frame(&f);
    assert(cui_focused(f.ui) == cui_time_segment_id(20, 1));
    cui_destroy(f.ui);
}
static void paste_and_bounds(bool styled) {
    fixture f;
    init(&f, styled);
    cui_time_locale_v1 locale;
    cui_time_locale_init_v1(&locale);
    char digit[10][16];
    for (unsigned i = 0; i < 10; i++) {
        memset(digit[i], 'a' + (int)i, 16);
        locale.digits[i] = (cui_str){digit[i], 16};
    }
    f.options.text.locale = &locale;
    f.options.text.precision = 1;
    f.options.edit_generation++;
    f.time = (cui_time){9, 30, 0};
    cui_time_field_state_init_v1(&f.state, f.time, &f.options);
    warm(&f);
    focus(&f, 0);
    char payload[161];
    size_t n;
    assert(cui_time_format_v1((cui_time){11, 22, 33}, &f.options.text, payload, sizeof payload,
                              &n) == 0 &&
           n > 64);
    text(&f, (cui_str){payload, n});
    memset(payload, '!', sizeof payload);
    cui_time_field_result_v1 r = frame(&f);
    assert(r.changed && r.change_count == 1 && same(f.time, (cui_time){11, 22, 33}));
    f.options.text.flags = 3;
    f.options.text.minimum = (cui_time){10, 0, 0};
    f.options.text.maximum = (cui_time){12, 0, 0};
    f.options.edit_generation++;
    frame(&f);
    /* Complete grammar takes precedence over a previously split locale token. */
    text(&f, (cui_str){"a", 1});
    r = frame(&f);
    assert(!r.changed && f.state.pending_length == 1);
    assert(cui_time_format_v1((cui_time){10, 11, 12}, &f.options.text, payload, sizeof payload,
                              &n) == 0);
    text(&f, (cui_str){payload, n});
    memset(payload, '!', sizeof payload);
    r = frame(&f);
    assert(r.changed && r.change_count == 1 && same(f.time, (cui_time){10, 11, 12}) &&
           f.state.pending_length == 0 && f.state.digit_count == 0);
    text(&f, (cui_str){"a", 1});
    r = frame(&f);
    assert(!r.changed && f.state.pending_length == 1);
    cui_time before = f.time;
    unsigned segment = f.state.segment;
    unsigned digit_count = f.state.digit_count, draft = f.state.draft;
    /* Formatting with unrestricted options creates an independently rejected paste. */
    cui_time_text_options_v1 free_options = f.options.text;
    free_options.flags = 0;
    assert(cui_time_format_v1((cui_time){13, 0, 0}, &free_options, payload, sizeof payload, &n) ==
           0);
    text(&f, (cui_str){payload, n});
    r = frame(&f);
    if (r.changed || r.rejected_count != 1 || r.validation != CUI_TIME_TEXT_OUT_OF_RANGE ||
        !same(f.time, before) || f.state.segment != segment)
        fprintf(stderr,
                "paste rejection: changed=%d rejected=%zu validation=%u value=%u:%u:%u segment=%u "
                "expected=%u\n",
                r.changed ? 1 : 0, r.rejected_count, (unsigned)r.validation, f.time.hour,
                f.time.minute, f.time.second, f.state.segment, segment);
    assert(!r.changed && r.rejected_count == 1 && r.validation == CUI_TIME_TEXT_OUT_OF_RANGE &&
           same(f.time, before) && f.state.segment == segment && f.state.pending_length == 0 &&
           f.state.digit_count == digit_count && f.state.draft == draft);
    assert(cui_time_field_clear_validation_v1(&f.state) == 0);
    frame(&f);
    assert(f.state.validation == 0);
    /* Split a multi-byte UTF8 digit token at a scalar byte boundary. */
    f.options.text.locale = NULL;
    f.options.text.flags = 0;
    f.options.text.precision = 0;
    f.options.edit_generation++;
    f.time.second = 0;
    frame(&f);
    cui_time_locale_init_v1(&locale);
    locale.digits[1] = (cui_str){"١", 2};
    f.options.text.locale = &locale;
    f.options.edit_generation++;
    frame(&f);
    focus(&f, 1);
    text(&f, (cui_str){"\xd9", 1});
    r = frame(&f);
    assert(!r.changed && f.state.pending_length == 1);
    text(&f, (cui_str){"\xa1", 1});
    frame(&f);
    assert(f.time.minute == 1 && f.state.pending_length == 0 && f.state.digit_count == 1);
    cui_destroy(f.ui);
}
static void inactive_and_generation(bool styled) {
    for (unsigned mode = 0; mode < 5; ++mode) {
        fixture f;
        init(&f, styled);
        warm(&f);
        focus(&f, 1);
        text(&f, cui_string("1"));
        frame(&f);
        unsigned char state[sizeof f.state];
        memcpy(state, &f.state, sizeof state);
        size_t calls = f.calls;
        if (mode == 0)
            f.options.flags = CUI_TIME_FIELD_DISABLED_V1;
        else if (mode == 1)
            f.options.flags = CUI_TIME_FIELD_READONLY_V1;
        else if (mode == 2)
            f.inert = true;
        else if (mode == 3)
            f.layout = true;
        else
            f.modal = true;
        key(&f, CUI_KEY_UP, 0);
        cui_time_field_result_v1 r = frame(&f);
        assert(!r.changed && !r.change_count && f.calls == calls && f.time.minute == 1);
        assert(!memcmp(state, &f.state, sizeof state));
        f.options.flags = 0;
        f.inert = false;
        f.layout = false;
        f.modal = false;
        frame(&f);
        frame(&f);
        focus(&f, 1);
        key(&f, CUI_KEY_UP, 0);
        r = frame(&f);
        assert(r.changed && r.change_count == 1 && f.time.minute == 2);
        cui_destroy(f.ui);
    }
    fixture f;
    init(&f, styled);
    warm(&f);
    focus(&f, 1);
    text(&f, cui_string("1"));
    frame(&f);
    assert(f.state.digit_count == 1);
    f.options.edit_generation++;
    f.options.segment_names[0] = cui_string("ساعة");
    text(&f, cui_string("2"));
    frame(&f);
    assert(f.time.minute == 2 && f.state.digit_count == 1);
    size_t n;
    const cui_semantic *semantics = cui_semantics(f.ui, &n);
    bool named = false;
    for (size_t i = 0; i < n; i++)
        if (semantics[i].id == cui_time_segment_id(20, 0)) {
            cui_str name = semantics[i].name;
            named = name.length == strlen("ساعة") && !memcmp(name.data, "ساعة", name.length);
        }
    assert(named);
    cui_destroy(f.ui);
}
static void readiness_recovery(bool styled) {
    fixture f;
    init(&f, styled);
    unsigned char initial[sizeof f.state];
    memcpy(initial, &f.state, sizeof initial);
    cui_time_field_result_v1 outside =
        cui_base_time_field_v1(f.ui, 20, cui_string("Time"), &f.state, &f.time, &f.options,
                               (cui_time_handler){observe, &f});
    assert(outside.status == CUI_TIME_TEXT_INVALID_ARGUMENT &&
           !memcmp(initial, &f.state, sizeof initial));
    key(&f, CUI_KEY_UP, 0);
    frame(&f);
    assert(f.time.minute == 30 && !f.calls);
    frame(&f);
    focus(&f, 1);
    key(&f, CUI_KEY_UP, 0);
    assert(cui_begin_frame(f.ui, 1200, 400, .016f));
    cui_begin_portal(f.ui, 999, (cui_box){0}, (cui_rect){.w = -1}, 1);
    cui_end_box(f.ui);
    assert(cui_last_error(f.ui) == CUI_ERROR_LAYOUT);
    cui_clear_error(f.ui);
    assert(!cui_frame_input_snapshot_ready(f.ui));
    cui_time_field_result_v1 r =
        styled ? cui_time_field_v1(f.ui, 20, &f.theme, cui_string("Time"), &f.state, &f.time,
                                   &f.options, (cui_time_handler){observe, &f})
               : cui_base_time_field_v1(f.ui, 20, cui_string("Time"), &f.state, &f.time, &f.options,
                                        (cui_time_handler){observe, &f});
    assert(!r.changed && !r.change_count && f.time.minute == 30 && !f.calls &&
           !cui_event_consumed(f.ui, 0));
    cui_end_frame(f.ui);
    cui_clear_error(f.ui);
    key(&f, CUI_KEY_UP, 0);
    r = frame(&f);
    assert(!r.changed && !r.change_count && f.time.minute == 30);
    focus(&f, 1);
    key(&f, CUI_KEY_UP, 0);
    r = frame(&f);
    assert(r.changed && r.change_count == 1 && f.time.minute == 31);
    cui_destroy(f.ui);
}
static void atomic_preflight(void) {
    fixture f;
    init(&f, true);
    warm(&f);
    focus(&f, 0);
    key(&f, CUI_KEY_UP, 0);
    unsigned char state[sizeof f.state], value[sizeof f.time];
    memcpy(state, &f.state, sizeof state);
    memcpy(value, &f.time, sizeof value);
    f.options.reserved = 1;
    assert(cui_begin_frame(f.ui, 1200, 400, .016f));
    cui_time_field_result_v1 r =
        cui_time_field_v1(f.ui, 20, &f.theme, cui_string("Time"), &f.state, &f.time, &f.options,
                          (cui_time_handler){observe, &f});
    assert(r.status == CUI_TIME_TEXT_INVALID_CONFIG && !r.changed && !r.change_count);
    assert(!memcmp(state, &f.state, sizeof state) && !memcmp(value, &f.time, sizeof value));
    assert(!cui_event_consumed(f.ui, 0));
    cui_end_frame(f.ui);
    cui_destroy(f.ui);
}
static void poison_stack(void) {
    volatile unsigned char bytes[4096];
    for (size_t i = 0; i < sizeof bytes; i++)
        bytes[i] = 0xa5;
}
static void pixels_and_text(void) {
    fixture f;
    init(&f, true);
    f.theme.radius = 0;
    f.options.flags = CUI_TIME_FIELD_OWNER_INVALID_V1;
    warm(&f);
    poison_stack();
    bool saw_hour = false;
    size_t borders = 0;
    cui_draw_command border = {0};
    cui_rect root = cui_bounds(f.ui, 20);
    for (size_t i = 0; i < f.draw.count; i++) {
        cui_draw_command c = f.draw.commands[i];
        if (c.type == CUI_DRAW_TEXT && c.text.length == 2 && !memcmp(c.text.data, "09", 2))
            saw_hour = true;
        if (c.type == CUI_DRAW_BORDER && c.bounds.x == root.x && c.bounds.y == root.y &&
            c.bounds.w == root.w) {
            border = c;
            borders++;
        }
    }
    assert(saw_hour && borders == 1 && border.color.r == f.theme.danger.r &&
           border.color.g == f.theme.danger.g);
    cui_software *renderer = cui_software_create(1200, 400, NULL);
    assert(renderer);
    cui_draw_list only = {.commands = &border, .count = 1};
    assert(cui_software_render(renderer, &only, (cui_color){0}));
    unsigned w, h;
    const unsigned char *pixels = cui_software_pixels(renderer, &w, &h);
    unsigned x = (unsigned)(root.x + root.w * .5f), y = (unsigned)root.y;
    assert(x < w && y < h);
    const unsigned char *p = pixels + ((size_t)y * w + x) * 4;
    assert(p[0] == f.theme.danger.r && p[1] == f.theme.danger.g && p[2] == f.theme.danger.b &&
           p[3] == 255);
    f.options.flags = 0;
    frame(&f);
    bool cleared = false;
    for (size_t i = 0; i < f.draw.count; i++)
        if (f.draw.commands[i].type == CUI_DRAW_BORDER && f.draw.commands[i].bounds.w == root.w) {
            cui_color c = f.draw.commands[i].color;
            cleared = c.r == f.theme.border.r && c.g == f.theme.border.g && c.b == f.theme.border.b;
        }
    assert(cleared);
    cui_software_destroy(renderer);
    cui_destroy(f.ui);
}
int main(void) {
    for (unsigned styled = 0; styled < 2; styled++) {
        editing(styled != 0);
        original_order(styled != 0);
        focus_transition(styled != 0, false);
        focus_transition(styled != 0, true);
        tab_exit_draft(styled != 0, false);
        tab_exit_draft(styled != 0, true);
        tabs(styled != 0);
        paste_and_bounds(styled != 0);
        inactive_and_generation(styled != 0);
        readiness_recovery(styled != 0);
    }
    atomic_preflight();
    pixels_and_text();
    printf("localized TimeField: PASS state=%zu options=%zu result=%zu\n",
           sizeof(cui_time_field_state_v1), sizeof(cui_time_field_options_v1),
           sizeof(cui_time_field_result_v1));
    return 0;
}
