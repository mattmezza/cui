/* SPDX-License-Identifier: MIT */
#define main message_legacy_main
#include "test_message.c"
#undef main
#include "cui/cui_message_events.h"
#include <stdio.h>
typedef struct {
    size_t offsets, tails, order;
    cui_message_offset_change offset;
    cui_message_tail_change tail;
    rows *rendered;
} event_probe;
static void offset_observed(void *user, const cui_message_offset_change *event) {
    event_probe *p = user;
    if (p->rendered)
        assert(p->rendered->calls > 0);
    assert(p->order == 0);
    p->order = 1;
    ++p->offsets;
    p->offset = *event;
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.target == 22 && event->event.kind == CUI_COMPONENT_EVENT_CHANGE);
}
static void tail_observed(void *user, const cui_message_tail_change *event) {
    event_probe *p = user;
    assert(p->order <= 1);
    p->order = 2;
    ++p->tails;
    p->tail = *event;
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME && event->event.target == 22);
}
static void result_equal(cui_message_scroller_result a, cui_message_scroller_result b) {
#define EQ(field) assert(a.field == b.field)
    EQ(valid);
    EQ(changed);
    EQ(jumped);
    EQ(jump_disabled);
    EQ(jump_opacity);
    EQ(fade_opacity);
    EQ(next_wake_seconds);
    EQ(view.begin);
    EQ(view.end);
    EQ(view.scroll);
    EQ(view.total);
    EQ(view.before);
    EQ(view.following_tail);
    EQ(view.scrolled_up);
#undef EQ
}
static void paired(bool styled) {
    cui_context *a = cui_create((cui_config){.width = 600, .height = 400});
    cui_context *b = cui_create((cui_config){.width = 600, .height = 400});
    cui_message_scroller_state *sa =
        cui_message_scroller_create((cui_message_scroller_config){100, 40, 50});
    cui_message_scroller_state *sb =
        cui_message_scroller_create((cui_message_scroller_config){100, 40, 50});
    cui_message_scroller_options o = cui_message_scroller_options_default();
    o.height = 200;
    o.scrollbar = false;
    cui_theme theme = cui_theme_light();
    rows ra = {0}, rb = {0};
    ra.height = rb.height = 35;
    event_probe p = {.rendered = styled ? &rb : NULL};
    cui_message_scroller_handler handler = {offset_observed, tail_observed, &p};
    double prior_scroll = 0;
    for (unsigned step = 0; step < 12; step++) {
        o.now_seconds = step * .1;
        if (step == 1) {
            assert(cui_message_scroller_scroll(sa, 100.25));
            assert(cui_message_scroller_scroll(sb, 100.25));
        }
        if (step == 2) {
            assert(cui_message_scroller_prepend(sa, 2));
            assert(cui_message_scroller_prepend(sb, 2));
        }
        if (step == 3) {
            o.width -= 40;
        }
        if (step == 4) {
            o.row_gap += 7;
            o.padding_top += 3;
        }
        if (step == 5) {
            cui_focus(a, 22);
            cui_focus(b, 22);
            assert(cui_queue_event(a, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
            assert(cui_queue_event(b, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
        }
        if (step == 6) {
            assert(cui_queue_event(
                a,
                (cui_event){
                    .type = CUI_EVENT_SCROLL, .x = 5, .y = 5, .dy = .5f, .scroll_precise = true}));
            assert(cui_queue_event(
                b,
                (cui_event){
                    .type = CUI_EVENT_SCROLL, .x = 5, .y = 5, .dy = .5f, .scroll_precise = true}));
        }
        if (step == 7) {
            assert(cui_message_scroller_reset(sa, 2));
            assert(cui_message_scroller_reset(sb, 2));
            assert(cui_message_scroller_to_item(sa, 0));
            assert(cui_message_scroller_to_item(sb, 0));
            cui_focus(a, 22);
            cui_focus(b, 22);
            assert(cui_queue_event(a, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
            assert(cui_queue_event(b, (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_END}));
        }
        if (step == 8) {
            assert(cui_message_scroller_append(sa, 10));
            assert(cui_message_scroller_append(sb, 10));
        }
        if (step == 9) {
            o.height = 180;
        }
        p.order = 0;
        ra.calls = rb.calls = 0;
        size_t offsets = p.offsets, tails = p.tails;
        assert(cui_begin_frame(a, 600, 400, .016f));
        assert(cui_begin_frame(b, 600, 400, .016f));
        cui_message_scroller_result old = styled
                                              ? cui_message_scroller(a, 22, &theme, sa, row, &ra, o)
                                              : cui_base_message_scroller(a, 22, sa, o);
        cui_message_scroller_handler h = step == 9 ? (cui_message_scroller_handler){0} : handler;
        cui_message_scroller_event_result r =
            styled ? cui_message_scroller_events(b, 22, &theme, sb, row, &rb, o, h)
                   : cui_base_message_scroller_events(b, 22, sb, o, h);
        result_equal(old, r.control_result);
        assert(r.valid && !r.count_overflow);
        assert(ra.calls == rb.calls && r.offset_change_count == p.offsets - offsets &&
               r.tail_change_count == p.tails - tails);
        assert(r.offset_change_count <= 1 && r.tail_change_count <= 1);
        if (step == 3 && styled)
            assert(r.offset_changed && p.offset.previous == prior_scroll);
        if (step == 5)
            assert(r.offset_changed && r.tail_changed && p.order == 2);
        if (step == 7) {
            assert(!r.offset_changed && r.tail_changed && r.tail_change_count == 1);
            assert(!p.tail.previous && p.tail.value);
        }
        if (step == 9)
            assert(!r.offset_change_count && !r.tail_change_count);
        if (!styled && (step == 10 || step == 11))
            assert(!r.offset_changed && !r.tail_changed);
        prior_scroll = r.control_result.view.scroll;
        size_t ac, bc;
        (void)cui_frame_events(a, &ac);
        (void)cui_frame_events(b, &bc);
        assert(ac == bc);
        for (size_t i = 0; i < ac; i++)
            assert(cui_event_consumed(a, i) == cui_event_consumed(b, i));
        if (!styled) {
            cui_begin_box(a, 22, (cui_box){.width = o.width, .height = o.height});
            cui_end_box(a);
            cui_begin_box(b, 22, (cui_box){.width = o.width, .height = o.height});
            cui_end_box(b);
        }
        cui_end_frame(a);
        cui_end_frame(b);
        assert(cui_last_error(a) == CUI_OK && cui_last_error(b) == CUI_OK);
    }
    size_t count = p.offsets + p.tails;
    assert(!cui_base_message_scroller_events(b, 22, sb, o, handler).valid);
    assert(cui_begin_frame(b, 600, 400, .016f));
    assert(!cui_base_message_scroller_events(b, 0, sb, o, handler).valid);
    cui_message_scroller_options invalid = o;
    invalid.height = NAN;
    assert(!cui_base_message_scroller_events(b, 22, sb, invalid, handler).valid);
    cui_end_frame(b);
    assert(p.offsets + p.tails == count); /* omitted work retains no handler */
    cui_message_offset_change retained = p.offset;
    cui_destroy(a);
    cui_destroy(b);
    cui_message_scroller_destroy(sa);
    cui_message_scroller_destroy(sb);
    assert(retained.event.target == 22 && retained.event.frame_sequence > 0);
}
static void precision_and_passive(void) {
    for (unsigned styled = 0; styled < 2; styled++)
        for (unsigned passive = 0; passive < 3; passive++) {
            cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
            cui_message_scroller_state *s =
                cui_message_scroller_create((cui_message_scroller_config){100, 100, 1000000});
            cui_message_scroller_view view;
            assert(cui_message_scroller_layout(s, (cui_message_scroller_metrics){200, 8, 8, 32},
                                               &view));
            assert(cui_message_scroller_scroll(s, 16777216.25));
            cui_message_scroller_options o = cui_message_scroller_options_default();
            o.height = 200;
            o.scrollbar = false;
            /* Change top padding inside observed layout: anchor restoration is double. */
            o.padding_top = 8.5f;
            event_probe p = {0};
            rows r = {.height = 100};
            cui_theme t = cui_theme_light();
            assert(cui_begin_frame(ui, 600, 400, .016f));
            cui_begin_box(ui, 1,
                          (cui_box){.width = 550,
                                    .height = 350,
                                    .inert = passive == 1,
                                    .layout_only = passive == 2});
            cui_message_scroller_event_result result =
                styled ? cui_message_scroller_events(
                             ui, 22, &t, s, row, &r, o,
                             (cui_message_scroller_handler){offset_observed, tail_observed, &p})
                       : cui_base_message_scroller_events(
                             ui, 22, s, o,
                             (cui_message_scroller_handler){offset_observed, tail_observed, &p});
            assert(result.valid && result.offset_changed && !result.tail_changed);
            assert(result.control_result.view.scroll == 16777216.75);
            assert(p.offsets == (passive == 0 ? 1U : 0U));
            if (!passive)
                assert(p.offset.previous == 16777216.25 && p.offset.value == 16777216.75);
            cui_end_box(ui);
            cui_end_frame(ui);
            assert(cui_last_error(ui) == CUI_OK);
            cui_destroy(ui);
            cui_message_scroller_destroy(s);
        }
}
static void animation_only(void) {
    cui_context *ui = cui_create((cui_config){.width = 600, .height = 400});
    cui_message_scroller_state *state =
        cui_message_scroller_create((cui_message_scroller_config){50, 40, 50});
    cui_message_scroller_options o = cui_message_scroller_options_default();
    o.height = 200;
    o.scrollbar = false;
    event_probe p = {0};
    cui_message_scroller_handler h = {offset_observed, tail_observed, &p};
    bool animation = false;
    for (unsigned step = 0; step < 5; step++) {
        if (step == 1)
            assert(cui_message_scroller_scroll(state, 0));
        o.now_seconds = step * .05;
        p.order = 0;
        assert(cui_begin_frame(ui, 600, 400, .016f));
        cui_message_scroller_event_result result =
            cui_base_message_scroller_events(ui, 22, state, o, h);
        if (step >= 2) {
            assert(!result.offset_changed && !result.tail_changed && !result.offset_change_count &&
                   !result.tail_change_count);
            animation |=
                result.control_result.changed && result.control_result.next_wake_seconds > 0;
        }
        cui_end_frame(ui);
    }
    assert(animation);
    cui_destroy(ui);
    cui_message_scroller_destroy(state);
}
int main(void) {
    animation_only();
    assert(message_legacy_main() == 0);
    paired(false);
    paired(true);
    precision_and_passive();
    puts("message events ok");
    return 0;
}
