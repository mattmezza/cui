/* SPDX-License-Identifier: MIT */
#include "cui/cui_number_events.h"
#include "cui/cui_components.h"
#include <assert.h>
#include <float.h>
#include <math.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_edit *edit;
    cui_number_state state;
    double value;
    char bytes[128];
    size_t calls, policies, trace_length;
    char trace[64];
    double magnitude;
    bool replace;
    cui_number_change_event saved;
    cui_draw_list draw;
} fixture;
static void changed(void *user, const cui_number_change_event *event) {
    fixture *f = user;
    assert(event->event.kind == CUI_COMPONENT_EVENT_CHANGE && event->event.target == 2);
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.frame_sequence == cui_frame_sequence(f->ui));
    assert(event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(event->after == f->value && f->state.last_value == event->after);
    assert(f->trace_length + 1 < sizeof f->trace);
    f->trace[f->trace_length++] = 'C';
    f->saved = *event;
    f->calls++;
    if (f->replace) f->value = 42;
}
static fixture make(void) {
    fixture f = {0};
    f.ui = cui_create((cui_config){.width = 600, .height = 300});
    assert(f.ui);
    return f;
}
static void init(fixture *f, size_t capacity) {
    f->edit = cui_edit_create((cui_edit_config){.buffer = f->bytes, .capacity = capacity,
        .history_bytes = 1024, .max_history_steps = 16});
    assert(f->edit);
    f->state.edit = f->edit;
}
static void finish(fixture *f) { cui_edit_destroy(f->edit); cui_destroy(f->ui); }
static void draft(fixture *f, const char *text) {
    assert(cui_edit_select(f->edit, 0, cui_edit_get_state(f->edit).length) == CUI_EDIT_OK);
    assert(cui_edit_insert(f->edit, cui_string(text)) == CUI_EDIT_OK);
}
static void key(fixture *f, cui_key code, unsigned modifiers) {
    assert(cui_queue_event(f->ui, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = code, .modifiers = modifiers}));
}
static cui_number_event_result frame(fixture *f, cui_number_options options, bool styled,
    bool legacy, unsigned passive, cui_number_event_handler handler) {
    assert(cui_begin_frame(f->ui, 600, 300, .016f));
    cui_begin_box(f->ui, 900, (cui_box){.width = 500, .height = 250,
        .inert = passive == 1, .layout_only = passive == 2});
    cui_number_event_result r = {0};
    cui_theme theme = cui_theme_light();
    if (legacy) r.control = styled ? cui_number_input(f->ui, 2, &theme, cui_string("Value"), &f->state, &f->value, options)
        : cui_base_number_input(f->ui, 2, cui_string("Value"), &f->state, &f->value, options);
    else r = styled ? cui_number_input_events(f->ui, 2, &theme, cui_string("Value"), &f->state, &f->value, options, handler)
        : cui_base_number_input_events(f->ui, 2, cui_string("Value"), &f->state, &f->value, options, handler);
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static cui_number_event_result render(fixture *f, cui_number_options o, bool styled) {
    return frame(f, o, styled, false, 0, (cui_number_event_handler){changed, f});
}
static double policy(void *user, double value, int direction) {
    fixture *f = user;
    assert(isfinite(value) && (direction == 1 || direction == -1));
    assert(f->trace_length + 1 < sizeof f->trace);
    f->trace[f->trace_length++] = 'P';
    f->policies++;
    return f->magnitude != 0 ? f->magnitude : 1;
}
static void operations(bool styled) {
    fixture f = make(); init(&f,128); f.value = 5;
    cui_number_options o = {.step = 1, .has_maximum = true, .maximum = 10,
        .step_by = policy, .step_user = &f};
    assert(render(&f,o,styled).change_count == 0);
    cui_focus(f.ui,2); key(&f,CUI_KEY_UP,0); key(&f,CUI_KEY_UP,0);
    cui_number_event_result r = render(&f,o,styled);
    assert(r.change_count == 1 && r.control.changed && f.value == 7 && f.policies == 2);
    assert(f.saved.before == 5 && f.saved.after == 7 && !strcmp(f.trace,"PPC"));
    key(&f,CUI_KEY_UP,0); key(&f,CUI_KEY_DOWN,0);
    assert(render(&f,o,styled).change_count == 0 && f.value == 7 && f.policies == 4 && !strcmp(f.trace,"PPCPP"));
    draft(&f,"99"); r = render(&f,o,styled);
    assert(r.change_count == 1 && f.value == 99);
    key(&f,CUI_KEY_ENTER,0); r = render(&f,o,styled);
    assert(r.control.submitted && r.change_count == 1 && f.value == 10);
    key(&f,CUI_KEY_ENTER,0); r = render(&f,o,styled);
    assert(r.control.submitted && r.change_count == 0);
    key(&f,CUI_KEY_UP,0); r = render(&f,o,styled);
    assert(r.control.step_direction == 1 && r.change_count == 0 && f.policies == 5);
    f.magnitude = NAN; key(&f,CUI_KEY_DOWN,0);
    assert(render(&f,o,styled).change_count == 0 && f.value == 10 && f.policies == 6);
    f.magnitude = 1;
    draft(&f,"-"); r = render(&f,o,styled);
    assert(r.control.invalid && r.change_count == 0 && f.value == 10);
    draft(&f,"12"); cui_focus(f.ui,0); r = render(&f,o,styled);
    assert(f.value == 10 && r.change_count == 0 && !strcmp(f.bytes,"10"));
    f.value = 3; assert(render(&f,o,styled).change_count == 0 && !strcmp(f.bytes,"3"));
    cui_focus(f.ui,2); key(&f,CUI_KEY_UP,0); f.replace = true;
    r = render(&f,o,styled);
    assert(r.change_count == 1 && f.saved.after == 4 && f.value == 42 && !strcmp(f.bytes,"4"));
    if (styled) {
        bool painted = false;
        for (size_t i = 0; i < f.draw.count; ++i)
            painted |= f.draw.commands[i].type == CUI_DRAW_TEXT &&
                f.draw.commands[i].text.length == 1 && f.draw.commands[i].text.data[0] == '4';
        assert(painted);
    }
    f.replace = false; assert(render(&f,o,styled).change_count == 0 && !strcmp(f.bytes,"42"));
    key(&f,CUI_KEY_DOWN,0);
    size_t calls = f.calls;
    r = frame(&f,o,styled,false,0,(cui_number_event_handler){NULL,&f});
    assert(r.change_count == 0 && r.control.changed && f.value == 10 && f.calls == calls);
    finish(&f);
}
static void precision_and_errors(bool styled) {
    fixture f = make(); init(&f,128);
    cui_number_options o = {.step = DBL_MAX};
    render(&f,o,styled); draft(&f,"-0");
    assert(render(&f,o,styled).change_count == 0 && signbit(f.value));
    draft(&f,"5e-324"); assert(render(&f,o,styled).change_count == 1 && f.value == DBL_TRUE_MIN);
    f.value = 1; render(&f,o,styled);
    char text[96]; double adjacent = nextafter(1,2);
    assert(cui_number_format(adjacent,text,sizeof text)); draft(&f,text);
    assert(render(&f,o,styled).change_count == 1 && f.saved.after == adjacent);
    f.value = DBL_MAX; render(&f,o,styled); cui_focus(f.ui,2); key(&f,CUI_KEY_UP,0);
    assert(render(&f,o,styled).change_count == 0 && f.value == DBL_MAX);
    f.value = NAN; assert(render(&f,o,styled).control.error == CUI_EDIT_INVALID_ARGUMENT);
    f.value = INFINITY; assert(render(&f,o,styled).change_count == 0);
    finish(&f);
    fixture small = make(); init(&small,3); small.value = 5;
    o = (cui_number_options){.has_minimum = true, .minimum = 100};
    render(&small,o,styled); cui_focus(small.ui,2); draft(&small,"99"); key(&small,CUI_KEY_ENTER,0);
    cui_number_event_result r = render(&small,o,styled);
    assert(r.control.error == CUI_EDIT_CAPACITY && r.change_count == 1 && small.value == 99);
    assert(small.saved.before == 5 && small.saved.after == 99);
    finish(&small);
}
static void passive_and_legacy(bool styled) {
    for (unsigned passive = 0; passive < 3; ++passive)
        for (unsigned restricted = 0; restricted < 3; ++restricted) {
            fixture old = make(), current = make(); init(&old,128); init(&current,128);
            old.value = current.value = 5;
            cui_number_options o = {.step = 1, .disabled = restricted == 1, .readonly = restricted == 2};
            frame(&old,o,styled,true,0,(cui_number_event_handler){0});
            render(&current,o,styled);
            draft(&old,"9"); draft(&current,"9");
            cui_number_event_result a = frame(&old,o,styled,true,passive,(cui_number_event_handler){0});
            cui_number_event_result b = frame(&current,o,styled,false,passive,(cui_number_event_handler){changed,&current});
            assert(old.value == current.value && !strcmp(old.bytes,current.bytes));
            assert(cui_focused(old.ui) == cui_focused(current.ui));
            cui_rect ob = cui_bounds(old.ui, cui_numeric_part_id(2,0));
            cui_rect nb = cui_bounds(current.ui, cui_numeric_part_id(2,0));
            assert(ob.x == nb.x && ob.y == nb.y && ob.w == nb.w && ob.h == nb.h);
            size_t oc, nc;
            const cui_semantic *os = cui_semantics(old.ui,&oc), *ns = cui_semantics(current.ui,&nc);
            assert(oc == nc);
            for (size_t i = 0; i < oc; ++i)
                assert(os[i].id == ns[i].id && os[i].role == ns[i].role &&
                    os[i].value == ns[i].value && os[i].disabled == ns[i].disabled &&
                    os[i].details.invalid == ns[i].details.invalid && os[i].details.readonly == ns[i].details.readonly);
            assert(a.control.changed == b.control.changed && a.control.invalid == b.control.invalid && a.control.error == b.control.error &&
                a.control.submitted == b.control.submitted && a.control.step_direction == b.control.step_direction);
            assert(old.state.last_value == current.state.last_value && old.state.invalid == current.state.invalid &&
                old.state.initialized == current.state.initialized && old.state.was_focused == current.state.was_focused);
            assert(cui_edit_get_state(old.edit).revision == cui_edit_get_state(current.edit).revision);
            if (!restricted) assert(current.value == 9); /* Passive is not a no-mutation promise. */
            assert(b.change_count == ((!passive && !restricted) ? 1u : 0u));
            finish(&old); finish(&current);
        }
}
int main(void) {
    for (unsigned styled = 0; styled < 2; ++styled) {
        operations(styled != 0); precision_and_errors(styled != 0); passive_and_legacy(styled != 0);
    }
    return 0;
}
