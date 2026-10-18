/* SPDX-License-Identifier: MIT */
#include "cui/cui_multiline_events.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_context *ui;
    cui_edit *edit;
    char bytes[128], copies[16][128];
    cui_textarea_state state;
    cui_textarea_line lines[32];
    cui_textarea_options options;
    cui_component_text_event events[16];
    unsigned count;
    bool omit;
    cui_draw_list draw;
} fixture;
static void observe(void *user, const cui_component_text_event *event) {
    fixture *f = user;
    unsigned i = f->count++;
    assert(i < 16);
    f->events[i] = *event;
    assert(event->after.revision == cui_edit_get_state(f->edit).revision);
    assert(event->event.flags == CUI_COMPONENT_EVENT_HAS_FRAME);
    assert(event->event.source == CUI_COMPONENT_EVENT_SOURCE_UNKNOWN);
    assert(event->event.frame_sequence == cui_frame_sequence(f->ui));
    assert(event->event.kind == CUI_COMPONENT_EVENT_CHANGE && event->event.target == 10);
    if (event->text.length) {
        char too_small[1] = {'Q'};
        cui_str untouched = {"sentinel", 8};
        assert(cui_component_event_copy_utf8(event->text, 127, too_small, 1, &untouched) ==
               CUI_COMPONENT_EVENT_COPY_CAPACITY);
        assert(too_small[0] == 'Q' && untouched.length == 8);
    }
    if (event->has_text) {
        assert(cui_component_event_copy_utf8(event->text, 127, f->copies[i], 128,
                                             &f->events[i].text) == CUI_COMPONENT_EVENT_COPY_OK);
    } else
        assert(!event->text.data && !event->text.length);
    f->omit = true; /* Application flag only; omission is deferred to a later frame. */
}
static cui_multiline_event_result frame(fixture *f, bool facade, bool observer, bool omit) {
    assert(cui_begin_frame(f->ui, 400, 100, .016f));
    cui_theme theme = cui_theme_light();
    cui_multiline_event_result r = {0};
    cui_begin_box(f->ui, 1, (cui_box){.width = -1, .height = -1});
    if (!omit) {
        if (facade)
            r = cui_textarea_events(f->ui, 10, &theme, cui_string("Name"), f->edit, &f->state,
                                    f->options,
                                    (cui_component_text_handler){observer ? observe : NULL, f});
        else
            r.control =
                cui_textarea(f->ui, 10, &theme, cui_string("Name"), f->edit, &f->state, f->options);
    }
    cui_end_box(f->ui);
    f->draw = cui_end_frame(f->ui);
    assert(cui_last_error(f->ui) == CUI_OK);
    return r;
}
static void init(fixture *f) {
    memset(f, 0, sizeof *f);
    f->ui = cui_create((cui_config){.max_elements = 128, .max_commands = 256});
    f->edit = cui_edit_create((cui_edit_config){.buffer = f->bytes,
                                                .capacity = 128,
                                                .multiline = true,
                                                .history_bytes = 4096,
                                                .max_history_steps = 16});
    assert(f->ui && f->edit);
    assert(cui_textarea_state_init(&f->state, f->lines, 32));
    f->options = (cui_textarea_options){.width = 220, .rows = 3};
    frame(f, false, false, false);
    cui_focus(f->ui, 10);
}
static void finish(fixture *f) {
    cui_destroy(f->ui);
    cui_edit_destroy(f->edit);
}
static void queue(fixture *f, cui_event_type type, const char *text) {
    assert(cui_queue_event(f->ui, (cui_event){.type = type,
                                              .key = CUI_KEY_ENTER,
                                              .text_data = text,
                                              .text_length = text ? strlen(text) : 0}));
}
static void equal(fixture *a, fixture *b, cui_textarea_result x, cui_textarea_result y) {
    assert(x.changed == y.changed && x.pressed_enter == y.pressed_enter && x.error == y.error &&
           x.edit_error == y.edit_error && x.selection_changed == y.selection_changed &&
           x.scrolled == y.scrolled && x.enter_shift == y.enter_shift &&
           x.enter_secondary == y.enter_secondary && x.visual_error == y.visual_error &&
           !strcmp(a->bytes, b->bytes));
    cui_edit_state sa = cui_edit_get_state(a->edit), sb = cui_edit_get_state(b->edit);
    assert(sa.revision == sb.revision && sa.cursor == sb.cursor && sa.anchor == sb.anchor);
    assert(sa.can_undo == sb.can_undo && sa.can_redo == sb.can_redo);
    size_t na, nb;
    const cui_semantic *ma = cui_semantics(a->ui, &na), *mb = cui_semantics(b->ui, &nb);
    assert(na == nb);
    for (size_t i = 0; i < na; ++i) {
        assert(ma[i].id == mb[i].id && ma[i].role == mb[i].role && ma[i].value == mb[i].value);
    }
    assert(a->draw.count == b->draw.count);
    for (size_t i = 0; i < a->draw.count; ++i) {
        const cui_draw_command *p = a->draw.commands + i, *q = b->draw.commands + i;
        assert(p->type == q->type && !memcmp(&p->bounds, &q->bounds, sizeof p->bounds));
        assert(!memcmp(&p->color, &q->color, sizeof p->color));
        assert(p->text.length == q->text.length);
        if (p->text.length)
            assert(!memcmp(p->text.data, q->text.data, p->text.length));
    }
}
static void pair_event(fixture *a, fixture *b, cui_event_type type, const char *text) {
    queue(a, type, text);
    queue(b, type, text);
}
static cui_multiline_event_result pair_frame(fixture *a, fixture *b, bool observer) {
    cui_multiline_event_result x = frame(a, false, false, false);
    cui_multiline_event_result y = frame(b, true, observer, false);
    equal(a, b, x.control, y.control);
    return y;
}
int main(void) {
    fixture a, b;
    init(&a);
    init(&b);
    pair_event(&a, &b, CUI_EVENT_TEXT, "a");
    pair_event(&a, &b, CUI_EVENT_KEY_DOWN, NULL);
    pair_event(&a, &b, CUI_EVENT_TEXT, "b");
    cui_multiline_event_result y = pair_frame(&a, &b, true);
    assert(y.change_count == 1 && b.count == 1 && !strcmp(b.copies[0], "a\nb"));
    assert(b.events[0].before.length == 0 && b.events[0].after.length == 3);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_START, NULL);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_UPDATE, "hi");
    y = pair_frame(&a, &b, true);
    assert(!y.change_count);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_COMMIT, "hi");
    y = pair_frame(&a, &b, true);
    assert(y.change_count == 1);
    assert(!strcmp(b.copies[0], "a\nb")); /* Owned copy survives later edits. */
    pair_event(&a, &b, CUI_EVENT_TEXT, "z");
    y = pair_frame(&a, &b, false);
    assert(y.control.changed && !y.change_count);
    y = pair_frame(&a, &b, true);
    assert(!y.change_count);
    assert(cui_edit_select(a.edit, 0, 1) == CUI_EDIT_OK);
    assert(cui_edit_select(b.edit, 0, 1) == CUI_EDIT_OK);
    y = pair_frame(&a, &b, true);
    assert(!y.change_count); /* External/selection-only. */
    assert(cui_queue_event(
        a.ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL}));
    assert(cui_queue_event(
        b.ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL}));
    y = pair_frame(&a, &b, true);
    assert(y.change_count == 1);
    a.options.readonly = b.options.readonly = true;
    pair_event(&a, &b, CUI_EVENT_TEXT, "ignored");
    y = pair_frame(&a, &b, true);
    assert(!y.change_count);
    a.options.readonly = b.options.readonly = false;
    a.options.submit_on_enter = b.options.submit_on_enter = true;
    pair_event(&a, &b, CUI_EVENT_KEY_DOWN, NULL);
    y = pair_frame(&a, &b, true);
    assert(y.control.pressed_enter && !y.change_count);
    assert(b.omit && b.draw.count);
    frame(&b, true, true, b.omit);
    size_t n;
    const cui_semantic *items = cui_semantics(b.ui, &n);
    for (size_t i = 0; i < n; ++i)
        assert(items[i].id != 10);
    finish(&a);
    finish(&b);
    /* A committed newline can exceed row storage: still notify despite layout error. */
    init(&a);
    init(&b);
    a.state.capacity = b.state.capacity = 1;
    pair_event(&a, &b, CUI_EVENT_TEXT, "x\ny");
    y = pair_frame(&a, &b, true);
    assert(y.control.error == CUI_TEXTAREA_CAPACITY && y.change_count == 1 &&
           !strcmp(b.copies[0], "x\ny"));
    /* Existing invalid layout before reduction must not apply another queued edit. */
    pair_event(&a, &b, CUI_EVENT_TEXT, "ignored");
    y = pair_frame(&a, &b, true);
    assert(!y.change_count && !strcmp(b.bytes, "x\ny"));
    finish(&a);
    finish(&b);
    /* Failed IME commit exceeds document capacity, without committed CHANGE. */
    init(&a);
    init(&b);
    char long_text[200];
    memset(long_text, 'x', sizeof long_text - 1);
    long_text[199] = 0;
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_START, NULL);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_UPDATE, long_text);
    y = pair_frame(&a, &b, true);
    assert(!y.change_count);
    pair_event(&a, &b, CUI_EVENT_COMPOSITION_COMMIT, long_text);
    y = pair_frame(&a, &b, true);
    assert(!y.change_count && y.control.edit_error != CUI_EDIT_OK && !b.bytes[0]);
    a.options.disabled = b.options.disabled = true;
    pair_event(&a, &b, CUI_EVENT_TEXT, "ignored");
    y = pair_frame(&a, &b, true);
    assert(!y.change_count);
    finish(&a);
    finish(&b);
    /* Public Base facade: aggregation is one revision notification even when
     * text followed by Undo returns to the original bytes within one batch. */
    init(&b);
    queue(&b, CUI_EVENT_TEXT, "one");
    assert(cui_queue_event(
        b.ui,
        (cui_event){.type = CUI_EVENT_KEY_DOWN, .key = CUI_KEY_Z, .modifiers = CUI_MOD_CTRL}));
    assert(cui_begin_frame(b.ui, 400, 100, .016f));
    cui_textarea_metrics metrics;
    assert(cui_textarea_measure(b.options, 16, &metrics));
    y = cui_base_textarea_events(b.ui, 10, cui_string("Name"), b.edit, &b.state, b.options, metrics,
                                 (cui_component_text_handler){observe, &b});
    b.draw = cui_end_frame(b.ui);
    assert(y.change_count == 1 && y.control.changed && !b.bytes[0]);
    assert(b.events[0].before.revision != b.events[0].after.revision);
    assert(!b.events[0].before.length && !b.events[0].after.length);
    finish(&b);
    puts("PASS aggregate Textarea CHANGE, legacy equivalence, IME and post-mutation errors");
    return 0;
}
