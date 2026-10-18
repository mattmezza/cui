/* SPDX-License-Identifier: MIT */
#include "cui/cui_hover_events.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static size_t calls;
static bool children_started;
static cui_hover_lifecycle_event saved;
static void observed(void *user, const cui_hover_lifecycle_event *event) {
    assert(user == &calls && !children_started);
    assert(event->previous_open != event->open && event->event.target == 42);
    assert(event->event.kind == CUI_COMPONENT_EVENT_LIFECYCLE);
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME && event->event.frame_sequence);
    assert(event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    saved = *event;
    calls++;
}
static cui_hover_options options(void) {
    return (cui_hover_options){.trigger = 41,
                               .viewport = {0, 0, 300, 180},
                               .side = CUI_POPUP_BELOW,
                               .width = 140,
                               .height = 60,
                               .dt = .1f,
                               .open_delay = .2f,
                               .close_delay = .2f};
}
typedef struct {
    cui_hover_event_result result;
    cui_draw_list draw;
} output;
static output scene(cui_context *ui, cui_hover_state *state, bool card, bool legacy, bool observer,
                    cui_hover_options o, unsigned passive, bool consume, bool omit,
                    bool fill_resource) {
    output out;
    memset(&out, 0, sizeof(out));
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 300, 180, .016f));
    if (consume)
        assert(cui_consume_event(ui, 0));
    cui_begin_box(
        ui, 1,
        (cui_box){.width = 300, .height = 180, .inert = passive == 1, .layout_only = passive == 2});
    cui_begin_box(ui, 41, (cui_box){.width = 100, .height = 30});
    cui_interact(ui, 41, CUI_ROLE_BUTTON, cui_string("Trigger"), false, false, 0);
    cui_end_box(ui);
    if (fill_resource)
        assert(cui_resource_get(ui, 99, 99, 1, NULL, NULL));
    children_started = false;
    if (!omit) {
        bool previous = state->open;
        if (legacy) {
            out.result.control_result =
                card ? cui_begin_hover_card(ui, 42, &theme, state, cui_string("Details"), o)
                     : cui_tooltip(ui, 42, &theme, state, cui_string("Help"), o);
            out.result.changed = previous != state->open;
        } else {
            cui_hover_handler handler = {observer ? observed : NULL, &calls};
            out.result =
                card ? cui_begin_hover_card_events(ui, 42, &theme, state, cui_string("Details"), o,
                                                   handler)
                     : cui_tooltip_events(ui, 42, &theme, state, cui_string("Help"), o, handler);
            assert(out.result.valid && out.result.changed == (previous != state->open));
            assert(!out.result.count_overflow);
        }
        if (card && out.result.control_result) {
            children_started = true;
            cui_begin_box(ui, 43,
                          (cui_box){.width = 70, .height = 20, .background = {10, 80, 160, 255}});
            cui_interact(ui, 43, CUI_ROLE_BUTTON, cui_string("Child"), false, false, 0);
            cui_end_box(ui);
            cui_end_hover_card(ui);
        }
    }
    cui_end_box(ui);
    out.draw = cui_end_frame(ui);
    assert(cui_last_error(ui) == (fill_resource ? CUI_ERROR_CAPACITY : CUI_OK));
    return out;
}
static void same(cui_context *a, cui_context *b, cui_hover_state x, cui_hover_state y, output p,
                 output q) {
    assert(x.open == y.open && x.pending == y.pending && x.target == y.target &&
           x.suppressed == y.suppressed && x.elapsed == y.elapsed);
    assert(p.result.control_result == q.result.control_result &&
           p.result.changed == q.result.changed);
    assert(p.draw.count == q.draw.count);
    for (size_t i = 0; i < p.draw.count; i++) {
        const cui_draw_command *u = &p.draw.commands[i], *v = &q.draw.commands[i];
        assert(u->type == v->type && !memcmp(&u->bounds, &v->bounds, sizeof(u->bounds)));
        assert(!memcmp(&u->color, &v->color, sizeof(u->color)) && u->font_size == v->font_size);
    }
    cui_rect ra = cui_bounds(a, 42), rb = cui_bounds(b, 42);
    assert(!memcmp(&ra, &rb, sizeof(ra)));
    size_t na, nb;
    const cui_semantic *sa = cui_semantics(a, &na), *sb = cui_semantics(b, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; i++)
        assert(sa[i].id == sb[i].id && sa[i].role == sb[i].role &&
               sa[i].details.parent == sb[i].details.parent &&
               sa[i].details.description.length == sb[i].details.description.length);
}
static void move(cui_context *ui, float x, float y) {
    assert(cui_queue_event(ui, (cui_event){.type = CUI_EVENT_POINTER_MOVE, .x = x, .y = y}));
}
static void test_equivalence(void) {
    for (unsigned card = 0; card < 2; card++) {
        cui_context *a = cui_create((cui_config){.max_elements = 128});
        cui_context *b = cui_create((cui_config){.max_elements = 128});
        assert(a && b);
        cui_hover_state x = {0}, y = {0};
        cui_hover_options o = options();
        calls = 0;
        for (unsigned step = 0; step < 10; step++) {
            bool consume = false;
            if (step == 1) {
                move(a, 2, 2);
                move(b, 2, 2);
            }
            if (step == 3 && card) {
                cui_focus(a, 43);
                cui_focus(b, 43);
                move(a, 290, 170);
                move(b, 290, 170);
            }
            if (step == 4) {
                cui_event escape = {.type = CUI_EVENT_KEY_DOWN,
                                    .key = CUI_KEY_ESCAPE,
                                    .repeat = true,
                                    .modifiers = CUI_MOD_CTRL};
                assert(cui_queue_event(a, escape) && cui_queue_event(b, escape));
                consume = true; /* Legacy hover still sees consumed/repeated global Escape. */
            }
            if (step == 5) {
                cui_focus(a, 0);
                cui_focus(b, 0);
                move(a, 290, 170);
                move(b, 290, 170);
            }
            if (step == 6) {
                cui_focus(a, 41);
                cui_focus(b, 41);
            }
            if (step == 8)
                o.disabled = true;
            size_t before = calls;
            output p = scene(a, &x, card, true, false, o, 0, consume, false, false);
            output q = scene(b, &y, card, false, true, o, 0, consume, false, false);
            same(a, b, x, y, p, q);
            assert(q.result.change_count == (size_t)q.result.changed &&
                   calls == before + q.result.change_count);
            if (step == 1)
                assert(!y.open && y.pending);
            if (step == 2)
                assert(y.open && q.result.control_result && q.result.changed);
            if (step == 3 && card)
                assert(y.open && !q.result.changed);
            if (step == 4)
                assert(!y.open && y.suppressed && q.result.changed);
            if (step == 5)
                assert(!y.suppressed);
            if (step == 7)
                assert(y.open);
            if (step == 8)
                assert(!y.open && q.result.changed);
        }
        o.disabled = false;
        o.dt = 1;
        cui_focus(a, 41);
        cui_focus(b, 41);
        output p = scene(a, &x, card, true, false, o, 0, false, false, false);
        output q = scene(b, &y, card, false, false, o, 0, false, false, false);
        same(a, b, x, y, p, q);
        assert(q.result.changed && !q.result.change_count && y.open);
        size_t before = calls;
        cui_hover_state frozen = y;
        scene(b, &y, card, false, true, o, 0, false, true, false);
        assert(y.open == frozen.open && y.elapsed == frozen.elapsed && calls == before);
        assert(cui_bounds(b, 42).w == 0); /* Omission does not close the model. */
        cui_hover_lifecycle_event copy = saved;
        cui_destroy(a);
        cui_destroy(b);
        assert(copy.event.target == 42 && copy.event.frame_sequence);
    }
}
static void test_fallback_and_passive(void) {
    for (unsigned card = 0; card < 2; card++) {
        cui_context *ui = cui_create((cui_config){.max_elements = 128});
        assert(ui);
        cui_hover_state s = {0};
        cui_hover_options o = options();
        o.dt = 1;
        scene(ui, &s, card, false, true, o, 0, false, false, false);
        cui_focus(ui, 41);
        o.viewport.w = 0;
        output r = scene(ui, &s, card, false, true, o, 0, false, false, false);
        assert(!r.result.control_result && !r.result.changed &&
               !s.open); /* Open then fallback close. */
        o.viewport.w = 300;
        r = scene(ui, &s, card, false, true, o, 0, false, false, false);
        assert(r.result.changed && r.result.control_result && s.open);
        o.viewport.w = NAN;
        r = scene(ui, &s, card, false, true, o, 0, false, false, false);
        assert(r.result.changed && !r.result.control_result && !saved.open);
        o.viewport.w = 300;
        o.trigger = 999;
        o.dt = 0;
        s.open = true;
        r = scene(ui, &s, card, false, true, o, 0, false, false, false);
        assert(r.result.changed && !r.result.control_result && !s.open); /* Missing anchor. */
        o.trigger = 41;
        o.dt = 1;
        o.disabled = true;
        for (unsigned passive = 1; passive <= 2; passive++) {
            s.open = true;
            size_t before = calls;
            r = scene(ui, &s, card, false, true, o, passive, false, false, false);
            assert(r.result.valid && r.result.changed && !r.result.change_count && !s.open &&
                   calls == before);
        }
        cui_hover_state before = s;
        cui_theme theme = cui_theme_light();
        assert(!cui_tooltip_events(ui, 0, &theme, &s, cui_string("Help"), o,
                                   (cui_hover_handler){observed, &calls})
                    .valid);
        assert(!cui_begin_hover_card_events(ui, 42, NULL, &s, cui_string("Details"), o,
                                            (cui_hover_handler){observed, &calls})
                    .valid);
        assert(s.open == before.open && s.elapsed == before.elapsed);
        cui_destroy(ui);
    }
    cui_context *ui = cui_create((cui_config){.max_elements = 128, .max_resources = 1});
    assert(ui);
    cui_context *old = cui_create((cui_config){.max_elements = 128, .max_resources = 1});
    assert(old);
    cui_hover_state s = {0}, prior = {0};
    cui_hover_options o = options();
    o.dt = 1;
    o.height = 0;
    scene(ui, &s, true, false, true, o, 0, false, false, false);
    scene(old, &prior, true, true, false, o, 0, false, false, false);
    cui_focus(ui, 41);
    cui_focus(old, 41);
    output baseline = scene(old, &prior, true, true, false, o, 0, false, false, true);
    output r = scene(ui, &s, true, false, true, o, 0, false, false, true);
    same(old, ui, prior, s, baseline, r);
    assert(r.result.changed && !r.result.control_result && r.result.change_count == 1 && s.open);
    assert(saved.open); /* Resource failure is not a model close or successful composition. */
    cui_destroy(old);
    cui_destroy(ui);
}
typedef struct {
    cui_context *ui;
    cui_hover_state state;
    bool legacy, armed;
    cui_hover_event_result result;
} measure_probe;
static cui_size measure(void *user, cui_str text, float size) {
    measure_probe *p = user;
    if (p->armed) {
        p->armed = false;
        cui_theme theme = cui_theme_light();
        cui_hover_options o = options();
        o.disabled = true;
        assert(cui_frame_sequence(p->ui) == 0);
        if (p->legacy)
            p->result.control_result =
                cui_tooltip(p->ui, 42, &theme, &p->state, cui_string("Help"), o);
        else
            p->result = cui_tooltip_events(p->ui, 42, &theme, &p->state, cui_string("Help"), o,
                                           (cui_hover_handler){observed, &calls});
    }
    return (cui_size){(float)text.length * size * .5f, size};
}
static void test_protected_measure(void) {
    for (unsigned legacy = 0; legacy < 2; legacy++) {
        measure_probe p = {.state = {.open = true}, .legacy = legacy != 0};
        p.ui =
            cui_create((cui_config){.max_elements = 128, .measure = measure, .measure_user = &p});
        assert(p.ui && cui_begin_frame(p.ui, 300, 180, .016f));
        p.armed = true;
        size_t before = calls;
        (void)cui_measure_text(p.ui, cui_string("Probe"), 12);
        assert(!p.armed && !p.state.open && calls == before);
        if (!legacy)
            assert(p.result.valid && p.result.changed && !p.result.change_count);
        cui_end_frame(p.ui);
        assert(cui_last_error(p.ui) ==
               CUI_ERROR_LIFECYCLE); /* Preserve legacy invalidation diagnostic. */
        cui_destroy(p.ui);
    }
}
int main(void) {
    test_equivalence();
    test_fallback_and_passive();
    test_protected_measure();
    return 0;
}
